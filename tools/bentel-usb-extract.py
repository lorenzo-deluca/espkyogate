#!/usr/bin/env python3
"""
Bentel KYO USB protocol extractor.

Parses USB capture logs exported from HHD Device Monitoring Studio (UTF-16LE
text exports), reassembles fragmented USB bulk transfers into complete KYO
protocol messages, validates checksums, and decodes known commands.

Supports all KYO panel variants: KYO4, KYO8, KYO8G, KYO32, KYO32G.

Output modes:
  --changes   Show only messages where response data changed (default)
  --raw       Show all protocol messages including repeated polls
  --json      Export as JSON Lines for programmatic analysis
  --summary   High-level overview of the capture session
"""

import argparse
import json
import re
import sys
from collections import Counter, OrderedDict

# ---------------------------------------------------------------------------
# Known KYO protocol commands
# ---------------------------------------------------------------------------

KNOWN_COMMANDS = OrderedDict([
    ("f0 00 00 0b 00 fb", {
        "name": "GET_SOFTWARE_VERSION",
        "desc": "Query panel software/firmware version",
    }),
    ("f0 04 f0 0a 00 ee", {
        "name": "GET_SENSOR_STATUS",
        "desc": "Read zone status, tamper flags and warning indicators",
    }),
    ("f0 02 15 12 00 19", {
        "name": "GET_PARTITION_STATUS_KYO32G",
        "desc": "Read partition arm/bypass/memory status (KYO32G variant)",
    }),
    ("f0 ec 14 12 00 02", {
        "name": "GET_PARTITION_STATUS_KYO32",
        "desc": "Read partition arm/bypass/memory status (KYO32 variant)",
    }),
    ("f0 68 0e 09 00 6f", {
        "name": "GET_PARTITION_STATUS_KYO8",
        "desc": "Read partition arm/bypass/memory status (KYO8 variant)",
    }),
])

# ---------------------------------------------------------------------------
# Response field definitions for known commands
# ---------------------------------------------------------------------------

SENSOR_STATUS_FIELDS = [
    ("zone_status_25_32", "Zone status 25-32"),
    ("zone_status_17_24", "Zone status 17-24"),
    ("zone_status_9_16",  "Zone status 9-16"),
    ("zone_status_1_8",   "Zone status 1-8"),
    ("zone_tamper_25_32", "Zone tamper 25-32"),
    ("zone_tamper_17_24", "Zone tamper 17-24"),
    ("zone_tamper_9_16",  "Zone tamper 9-16"),
    ("zone_tamper_1_8",   "Zone tamper 1-8"),
    ("warnings",          "Warning flags"),
    ("area_alarm",        "Area alarm"),
    ("tamper_sabotage",   "Tamper/sabotage flags"),
    ("checksum",          "Checksum"),
]

PARTITION_STATUS_FIELDS = [
    ("arm_total",          "Total arm (bit=area armed)"),
    ("arm_partial",        "Partial arm"),
    ("arm_partial_delay0", "Partial arm delay-0"),
    ("disarmed",           "Disarmed areas"),
    ("siren_misc",         "Siren (bit5) + misc"),
    ("unknown_11",         "Unknown byte Rx[11]"),
    ("output_status",      "Output status / unknown"),
    ("bypass_25_32",       "Bypassed zones 25-32"),
    ("bypass_17_24",       "Bypassed zones 17-24"),
    ("bypass_9_16",        "Bypassed zones 9-16"),
    ("bypass_1_8",         "Bypassed zones 1-8"),
    ("alarm_mem_25_32",    "Zone alarm memory 25-32"),
    ("alarm_mem_17_24",    "Zone alarm memory 17-24"),
    ("alarm_mem_9_16",     "Zone alarm memory 9-16"),
    ("alarm_mem_1_8",      "Zone alarm memory 1-8"),
    ("tamper_mem_25_32",   "Zone tamper memory 25-32"),
    ("tamper_mem_17_24",   "Zone tamper memory 17-24"),
    ("tamper_mem_9_16",    "Zone tamper memory 9-16"),
    ("tamper_mem_1_8",     "Zone tamper memory 1-8"),
    ("checksum",           "Checksum"),
]

# Map command hex strings to their response field definitions.
COMMAND_FIELDS = {
    "f0 04 f0 0a 00 ee": SENSOR_STATUS_FIELDS,
    "f0 02 15 12 00 19": PARTITION_STATUS_FIELDS,
    "f0 ec 14 12 00 02": PARTITION_STATUS_FIELDS,
    "f0 68 0e 09 00 6f": PARTITION_STATUS_FIELDS,
}

# ---------------------------------------------------------------------------
# Timestamp regex — DD.MM.YYYY HH:MM:SS.mmm (milliseconds optional)
# ---------------------------------------------------------------------------

_TS_RE = re.compile(r"\d{2}\.\d{2}\.\d{4}\s+\d{2}:\d{2}:\d{2}(?:\.\d+)?")


# ---------------------------------------------------------------------------
# Parsing
# ---------------------------------------------------------------------------

def parse_log(filepath):
    """Parse an HHD Device Monitoring Studio text export.

    Reads the UTF-16LE (or UTF-8 fallback) export and extracts every USB
    payload entry with its sequence number, direction, timestamp and raw
    hex bytes.

    Returns:
        list[dict]: One dict per USB payload with keys *seq*, *dir*, *ts*,
        and *hex* (list of lowercase two-character hex strings).
    """
    lines = _read_lines(filepath)
    entries = []
    seq = ""
    direction = ""
    timestamp = ""
    in_payload = False

    for line in lines:
        fields = line.rstrip("\n").split("\t")

        # --- Direction + timestamp row ---
        if "$print instruction" in line and "Direction" in line:
            seq = fields[0].strip()
            for field in fields:
                stripped = field.strip().strip('"')
                if stripped in ("Up", "Down"):
                    direction = stripped
            for field in reversed(fields):
                match = _TS_RE.search(field.strip())
                if match:
                    timestamp = match.group(0)
                    break

        # --- TransferBufferLength ---
        if "TransferBufferLength" in line:
            pass  # Kept for potential future use

        # --- Payload marker ---
        if "UsbPayload" in line:
            in_payload = True
            continue

        # --- Hex dump lines following the payload marker ---
        if in_payload:
            stripped = line.strip()
            match = re.match(
                r"^([0-9a-fA-F]{8})\s+((?:[0-9a-fA-F]{2}[\s.]?)+)", stripped
            )
            if match:
                hex_bytes = re.findall(r"[0-9a-fA-F]{2}", match.group(2))
                hex_bytes = [b.lower() for b in hex_bytes]
                entries.append({
                    "seq": int(seq) if seq.isdigit() else 0,
                    "dir": direction,
                    "ts": timestamp,
                    "hex": hex_bytes,
                })
                in_payload = False
            elif stripped == "":
                in_payload = False

    return entries


def _read_lines(filepath):
    """Read a text file trying UTF-16LE first, then UTF-8."""
    for encoding in ("utf-16-le", "utf-8-sig", "utf-8"):
        try:
            with open(filepath, "r", encoding=encoding) as fh:
                return fh.readlines()
        except (UnicodeDecodeError, UnicodeError):
            continue
    raise ValueError(
        f"Cannot decode {filepath} — expected UTF-16LE or UTF-8 text export"
    )


# ---------------------------------------------------------------------------
# Message reassembly
# ---------------------------------------------------------------------------

def reassemble_messages(entries):
    """Reassemble fragmented USB transfers into complete protocol messages.

    Protocol framing
    ~~~~~~~~~~~~~~~~
    * **OUT (Down)**: raw command bytes (typically 6 bytes), no USB header.
    * **IN (Up)**: 2-byte header ``[0x01, status]`` followed by optional
      protocol data.

      - ``status == 0x60`` — idle / ready
      - ``status == 0x00`` — busy / ack; data bytes follow

    Responses are fragmented across multiple IN transfers.  A complete
    response consists of an echo of the command bytes followed by the
    response payload and a trailing checksum byte (sum of payload bytes
    mod 256).

    Returns:
        list[dict]: Reassembled protocol messages.
    """
    messages = []
    i = 0

    while i < len(entries):
        entry = entries[i]

        if entry["dir"] == "Down":
            msg, i = _reassemble_command(entries, i)
            messages.append(msg)

        elif entry["dir"] == "Up" and len(entry["hex"]) > 2:
            msg, i = _reassemble_unsolicited(entries, i)
            messages.append(msg)

        else:
            i += 1

    return messages


def _reassemble_command(entries, i):
    """Reassemble a host-to-panel command and its response fragments."""
    entry = entries[i]
    cmd_bytes = entry["hex"]
    cmd_ts = entry["ts"]
    cmd_seq = entry["seq"]

    resp_raw = []
    status_changes = []
    j = i + 1
    last_ts = cmd_ts

    while j < len(entries):
        r = entries[j]
        if r["dir"] == "Down":
            break
        if r["dir"] == "Up" and len(r["hex"]) >= 2:
            status = r["hex"][1]
            last_ts = r["ts"]
            if status != "60":
                status_changes.append(status)
            if len(r["hex"]) > 2:
                resp_raw.extend(r["hex"][2:])  # strip 01 XX header
        j += 1

    resp_echo, resp_data, resp_chk, chk_valid = _parse_response(
        cmd_bytes, resp_raw
    )

    return {
        "type": "cmd",
        "seq": cmd_seq,
        "ts": cmd_ts,
        "ts_end": last_ts,
        "cmd": cmd_bytes,
        "resp_echo": resp_echo,
        "resp_data": resp_data,
        "resp_chk": resp_chk,
        "chk_valid": chk_valid,
        "status_changes": status_changes,
    }, j


def _reassemble_unsolicited(entries, i):
    """Reassemble an unsolicited panel-to-host message."""
    entry = entries[i]
    resp_raw = list(entry["hex"][2:])  # strip 01 XX header
    first_ts = entry["ts"]
    j = i + 1

    while j < len(entries):
        r = entries[j]
        if r["dir"] == "Down":
            break
        if r["dir"] == "Up" and len(r["hex"]) > 2:
            resp_raw.extend(r["hex"][2:])
        elif r["dir"] == "Up" and len(r["hex"]) == 2 and r["hex"][1] == "60":
            j += 1
            break  # back to idle
        j += 1

    chk_valid = None
    resp_chk = None
    if resp_raw:
        chk_valid, resp_chk = _verify_checksum(resp_raw)

    return {
        "type": "unsolicited",
        "seq": entry["seq"],
        "ts": first_ts,
        "ts_end": entries[j - 1]["ts"] if j > i + 1 else first_ts,
        "data": resp_raw,
        "chk": resp_chk,
        "chk_valid": chk_valid,
    }, j


def _parse_response(cmd_bytes, resp_raw):
    """Split raw response bytes into echo, data, checksum."""
    resp_echo = []
    resp_data = []
    resp_chk = None
    chk_valid = None

    if not resp_raw:
        return resp_echo, resp_data, resp_chk, chk_valid

    if resp_raw[: len(cmd_bytes)] == cmd_bytes:
        resp_echo = resp_raw[: len(cmd_bytes)]
        resp_data = resp_raw[len(cmd_bytes) :]
    else:
        resp_data = resp_raw

    if resp_data:
        chk_valid, resp_chk = _verify_checksum(resp_data)

    return resp_echo, resp_data, resp_chk, chk_valid


def _verify_checksum(data_bytes):
    """Verify trailing checksum byte (sum of preceding bytes & 0xFF).

    Returns:
        tuple[bool | None, str | None]: (is_valid, checksum_hex)
    """
    if len(data_bytes) < 2:
        return None, None
    try:
        payload_sum = sum(int(b, 16) for b in data_bytes[:-1]) & 0xFF
        actual = int(data_bytes[-1], 16)
        return payload_sum == actual, data_bytes[-1]
    except ValueError:
        return None, None


# ---------------------------------------------------------------------------
# Command decoding helpers
# ---------------------------------------------------------------------------

def get_command_name(cmd_hex):
    """Return a human-readable name for a command hex string."""
    info = KNOWN_COMMANDS.get(cmd_hex)
    return info["name"] if info else "UNKNOWN"


def get_command_label(cmd_hex):
    """Return 'NAME — description' for known commands, else 'UNKNOWN'."""
    info = KNOWN_COMMANDS.get(cmd_hex)
    if info:
        return f"{info['name']} — {info['desc']}"
    return "UNKNOWN"


def decode_response(cmd_hex_list, data_bytes):
    """Decode response data bytes based on the command type.

    Args:
        cmd_hex_list: List of hex byte strings for the command.
        data_bytes: List of hex byte strings for the response payload.

    Returns:
        OrderedDict or None: Field name -> {hex, dec, bin, desc} mappings.
    """
    cmd_key = " ".join(cmd_hex_list)
    fields = COMMAND_FIELDS.get(cmd_key)
    if not fields or not data_bytes:
        return None

    decoded = OrderedDict()
    for idx, (key, desc) in enumerate(fields):
        if idx < len(data_bytes):
            try:
                val = int(data_bytes[idx], 16)
            except ValueError:
                continue
            decoded[key] = {
                "hex": data_bytes[idx],
                "dec": val,
                "bin": f"{val:08b}",
                "desc": desc,
            }
    return decoded


# ---------------------------------------------------------------------------
# Arm/disarm state tracking (for --summary)
# ---------------------------------------------------------------------------

_PARTITION_CMDS = {
    "f0 02 15 12 00 19",
    "f0 ec 14 12 00 02",
    "f0 68 0e 09 00 6f",
}


def _extract_arm_state(decoded):
    """Return a simplified arm state string from decoded partition data."""
    if not decoded:
        return None

    arm_total = decoded.get("arm_total", {}).get("dec", 0)
    arm_partial = decoded.get("arm_partial", {}).get("dec", 0)
    disarmed = decoded.get("disarmed", {}).get("dec", 0)

    parts = []
    if arm_total:
        parts.append(f"armed-total=0x{arm_total:02x}")
    if arm_partial:
        parts.append(f"armed-partial=0x{arm_partial:02x}")
    if disarmed:
        parts.append(f"disarmed=0x{disarmed:02x}")

    return ", ".join(parts) if parts else "all-disarmed"


# ---------------------------------------------------------------------------
# Output formatters
# ---------------------------------------------------------------------------

def format_raw(messages):
    """Format all messages as human-readable text, including repeated polls."""
    lines = []
    prev_cmd = None

    for msg in messages:
        if msg["type"] == "unsolicited":
            data_hex = " ".join(msg["data"])
            chk = _chk_label(msg.get("chk_valid"))
            lines.append(f"[{msg['ts']}] <<< UNSOLICITED ({len(msg['data'])}B){chk}")
            lines.append(f"    {data_hex}")
            lines.append("")
            continue

        cmd_hex = " ".join(msg["cmd"])
        cmd_name = get_command_name(cmd_hex)
        data_hex = " ".join(msg["resp_data"]) if msg["resp_data"] else "(no data)"
        chk = _chk_label(msg.get("chk_valid"))

        # Collapse consecutive identical polls
        is_repeat = (
            prev_cmd
            and prev_cmd["cmd"] == msg["cmd"]
            and prev_cmd["resp_data"] == msg["resp_data"]
        )

        if is_repeat:
            if lines and lines[-1].startswith("    ... repeated"):
                count = int(lines[-1].split()[2].replace("x", "")) + 1
                lines[-1] = f"    ... repeated {count}x (last at {msg['ts']})"
            else:
                lines.append(f"    ... repeated 2x (last at {msg['ts']})")
        else:
            lines.append(f"[{msg['ts']}] >>> {cmd_hex}  [{cmd_name}]")
            lines.append(f"[{msg['ts_end']}] <<< {data_hex}{chk}")
            _append_decoded_fields(lines, msg)
            lines.append("")

        prev_cmd = msg

    return "\n".join(lines)


def format_changes(messages):
    """Show only messages where the response data changed from the previous."""
    lines = []
    last_data = {}

    for msg in messages:
        if msg["type"] == "unsolicited":
            lines.append(f"[{msg['ts']}] <<< UNSOLICITED: {' '.join(msg['data'])}")
            lines.append("")
            continue

        cmd_hex = " ".join(msg["cmd"])
        cmd_name = get_command_name(cmd_hex)
        data = msg["resp_data"]

        if cmd_hex in last_data and last_data[cmd_hex] == data:
            continue  # identical — skip

        prev = last_data.get(cmd_hex)
        last_data[cmd_hex] = data

        lines.append(f"[{msg['ts']}] >>> {cmd_hex}  [{cmd_name}]")

        if prev and data and len(prev) == len(data):
            diff_parts = []
            for idx, (old, new) in enumerate(zip(prev, data)):
                if old != new:
                    diff_parts.append(f"    byte[{idx}]: {old} -> {new}")
            if diff_parts:
                lines.append(f"  CHANGED ({len(diff_parts)} byte(s)):")
                lines.extend(diff_parts)
        else:
            lines.append(f"  DATA: {' '.join(data) if data else '(empty)'}")

        _append_decoded_fields(lines, msg)
        lines.append("")

    return "\n".join(lines)


def format_json(messages):
    """Format messages as JSON Lines for programmatic analysis."""
    output = []
    for msg in messages:
        obj = {
            "seq": msg["seq"],
            "ts": msg.get("ts", ""),
        }

        if msg["type"] == "unsolicited":
            obj["type"] = "unsolicited"
            obj["data"] = msg["data"]
            obj["chk_valid"] = msg["chk_valid"]
        else:
            cmd_hex = " ".join(msg["cmd"])
            obj["type"] = "cmd"
            obj["cmd"] = cmd_hex
            obj["cmd_name"] = get_command_name(cmd_hex)
            obj["resp_data"] = msg["resp_data"]
            obj["chk_valid"] = msg["chk_valid"]

            decoded = decode_response(msg["cmd"], msg["resp_data"])
            if decoded:
                obj["decoded"] = {
                    k: {"hex": v["hex"], "val": v["dec"]}
                    for k, v in decoded.items()
                    if k != "checksum"
                }

        output.append(json.dumps(obj))
    return "\n".join(output)


def format_summary(messages):
    """Produce a high-level overview of the capture session."""
    lines = []

    total = len(messages)
    cmd_msgs = [m for m in messages if m["type"] == "cmd"]
    unsolicited = [m for m in messages if m["type"] == "unsolicited"]

    # Command frequency
    cmd_counter = Counter()
    for m in cmd_msgs:
        cmd_counter[" ".join(m["cmd"])] += 1

    # Checksum statistics
    chk_ok = sum(1 for m in messages if m.get("chk_valid") is True)
    chk_fail = sum(1 for m in messages if m.get("chk_valid") is False)

    # Timestamp range
    timestamps = [m["ts"] for m in messages if m.get("ts")]
    ts_first = timestamps[0] if timestamps else "N/A"
    ts_last = timestamps[-1] if timestamps else "N/A"

    lines.append("=" * 60)
    lines.append("CAPTURE SUMMARY")
    lines.append("=" * 60)
    lines.append("")
    lines.append(f"Time range:          {ts_first}")
    lines.append(f"                  -> {ts_last}")
    lines.append("")
    lines.append(f"Total messages:      {total}")
    lines.append(f"  Commands (host):   {len(cmd_msgs)}")
    lines.append(f"  Unsolicited:       {len(unsolicited)}")
    lines.append(f"  Checksum OK:       {chk_ok}")
    if chk_fail:
        lines.append(f"  Checksum FAIL:     {chk_fail}")
    lines.append("")

    lines.append("Commands by frequency:")
    for cmd_hex, count in cmd_counter.most_common():
        label = get_command_label(cmd_hex)
        lines.append(f"  {count:>5}x  {cmd_hex}  [{label}]")
    lines.append("")

    # State transitions (arm/disarm)
    transitions = _collect_state_transitions(messages)
    if transitions:
        lines.append("Arm/disarm state transitions:")
        for ts, state in transitions:
            lines.append(f"  [{ts}] {state}")
        lines.append("")

    # Unique data snapshots per command
    unique_data = {}
    for m in cmd_msgs:
        key = " ".join(m["cmd"])
        data_key = tuple(m["resp_data"]) if m["resp_data"] else ()
        unique_data.setdefault(key, set()).add(data_key)

    lines.append("Unique response payloads per command:")
    for cmd_hex in sorted(unique_data):
        name = get_command_name(cmd_hex)
        lines.append(f"  {name}: {len(unique_data[cmd_hex])} unique")
    lines.append("")
    lines.append("=" * 60)

    return "\n".join(lines)


# ---------------------------------------------------------------------------
# Shared formatting helpers
# ---------------------------------------------------------------------------

def _chk_label(chk_valid):
    """Return a short checksum status string."""
    if chk_valid is True:
        return " chk=OK"
    elif chk_valid is False:
        return " chk=FAIL"
    return ""


def _append_decoded_fields(lines, msg):
    """Append decoded field lines for known commands."""
    decoded = decode_response(msg["cmd"], msg.get("resp_data", []))
    if not decoded:
        return
    for key, info in decoded.items():
        if key == "checksum":
            continue
        if info["dec"] != 0:
            lines.append(
                f"    {info['desc']:.<40s} 0x{info['hex']} ({info['bin']})"
            )


def _collect_state_transitions(messages):
    """Track arm/disarm state changes across partition status responses."""
    transitions = []
    prev_state = None

    for msg in messages:
        if msg["type"] != "cmd":
            continue
        cmd_hex = " ".join(msg["cmd"])
        if cmd_hex not in _PARTITION_CMDS:
            continue
        decoded = decode_response(msg["cmd"], msg.get("resp_data", []))
        state = _extract_arm_state(decoded)
        if state and state != prev_state:
            transitions.append((msg["ts"], state))
            prev_state = state

    return transitions


# ---------------------------------------------------------------------------
# CLI entry point
# ---------------------------------------------------------------------------

def build_parser():
    """Build the argument parser."""
    parser = argparse.ArgumentParser(
        prog="bentel-usb-extract",
        description=(
            "Parse USB capture logs from HHD Device Monitoring Studio and "
            "extract Bentel KYO protocol messages. Reassembles fragmented "
            "USB bulk transfers, validates checksums, and decodes known "
            "commands (sensor status, partition status, software version, "
            "configuration read/write)."
        ),
        epilog=(
            "The input file must be a UTF-16LE text export produced by "
            "HHD Device Monitoring Studio's Text Exporter. Works with all "
            "KYO panel variants (KYO4, KYO8, KYO8G, KYO32, KYO32G)."
        ),
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        "logfile",
        help="path to the HHD Device Monitoring Studio text export (.log)",
    )
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument(
        "--changes",
        action="store_true",
        default=False,
        help="show only messages where response data changed (default mode)",
    )
    mode.add_argument(
        "--raw",
        action="store_true",
        default=False,
        help="show all protocol messages including repeated identical polls",
    )
    mode.add_argument(
        "--json",
        action="store_true",
        default=False,
        help="export reassembled messages as JSON Lines (one object per line)",
    )
    mode.add_argument(
        "--summary",
        action="store_true",
        default=False,
        help=(
            "print a high-level session overview: message counts, unique "
            "commands, time range, and arm/disarm state transitions"
        ),
    )
    return parser


def main():
    """CLI entry point."""
    parser = build_parser()
    args = parser.parse_args()

    try:
        entries = parse_log(args.logfile)
    except FileNotFoundError:
        sys.stderr.write(f"Error: file not found: {args.logfile}\n")
        sys.exit(1)
    except ValueError as exc:
        sys.stderr.write(f"Error: {exc}\n")
        sys.exit(1)

    sys.stderr.write(f"Parsing {args.logfile}...\n")
    sys.stderr.write(f"  {len(entries)} USB payload entries found\n")

    messages = reassemble_messages(entries)
    sys.stderr.write(f"  {len(messages)} protocol messages reassembled\n")

    # Per-command counts on stderr
    cmd_counts = Counter()
    for m in messages:
        if m["type"] == "cmd":
            cmd_counts[" ".join(m["cmd"])] += 1
    for cmd_hex, count in cmd_counts.most_common():
        name = get_command_name(cmd_hex)
        sys.stderr.write(f"    {cmd_hex} [{name}]: {count}x\n")

    unsol = sum(1 for m in messages if m["type"] == "unsolicited")
    if unsol:
        sys.stderr.write(f"    unsolicited messages: {unsol}\n")

    # Format output
    if args.json:
        print(format_json(messages))
    elif args.raw:
        print(format_raw(messages))
    elif args.summary:
        print(format_summary(messages))
    else:
        # Default: changes mode
        print(format_changes(messages))


if __name__ == "__main__":
    main()
