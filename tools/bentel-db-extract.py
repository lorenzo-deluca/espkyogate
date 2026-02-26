#!/usr/bin/env python3
"""
Bentel KyoUnit Paradox database extractor.

Parses Borland Paradox 7 (.DB) database files created by KyoUnit, Bentel's
alarm panel configuration software. Extracts zone configuration, keyfob
enrollment, user codes, partition settings, output assignments, and other
panel parameters from the backup data directory.

Supports all KYO panel families: Spring (KYO4/8/32), Kyo300, NORMA, BGsm,
EASYGATE, GTCOM, OMNIA.

Output modes:
  (default)       Panel summary with key configuration counts
  --table NAME    Show a specific configuration table
  --json          Export all tables as JSON
  --raw           Parse a single .DB file and dump all records
"""

import argparse
import json
import os
import struct
import sys
from collections import OrderedDict

# ---------------------------------------------------------------------------
# Known panel models (tipocentrale field in CLIENTI.DB)
# ---------------------------------------------------------------------------

PANEL_MODELS = {
    0: "Unknown",
    1: "KYO4",
    2: "KYO8",
    3: "KYO32",
    4: "KYO8G",
    5: "KYO32G",
    10: "KYO320",
    20: "KYO8W",
    21: "KYO8GW",
    22: "KYO32W",
    23: "KYO32GW",
    24: "KYO32M",
    25: "KYO32GM",
    26: "KYO32M",
    27: "KYO32GM",
    30: "NORMA4",
    31: "NORMA8",
    32: "NORMA16",
    40: "BGsm-120",
    50: "EASYGATE",
    60: "GTCOM",
    70: "OMNIA",
}

# ---------------------------------------------------------------------------
# Panel family directory mapping
# ---------------------------------------------------------------------------

PANEL_FAMILY_DIRS = [
    "Spring",
    "Kyo300",
    "NORMA",
    "NORMA2",
    "BGsm",
    "EASYGATE",
    "GTCOM",
    "OMNIA",
]

# ---------------------------------------------------------------------------
# Table name to .DB filename mapping
# ---------------------------------------------------------------------------

TABLE_FILES = OrderedDict([
    ("zones", "Zone.DB"),
    ("keyfobs", "Radiochiavi.DB"),
    ("codes", "Codici.DB"),
    ("partitions", "Partizioni.DB"),
    ("outputs", "Uscite.DB"),
    ("events", "ImpEventi.DB"),
    ("expanders", "Expander.DB"),
    ("keypads", "ConfigTast.DB"),
    ("options", "opzioni.DB"),
    ("timers", "Tempi.DB"),
    ("phone", "Agenda.DB"),
    ("activators", "Attivatori.DB"),
    ("messages", "Messaggi.DB"),
    ("schedules", "ProgOrario.DB"),
])

# ---------------------------------------------------------------------------
# Paradox 7 field type codes
# ---------------------------------------------------------------------------

FIELD_TYPES = {
    0x01: "Alpha",
    0x02: "Date",
    0x03: "Short",
    0x04: "Long",
    0x06: "Number",
    0x09: "Logical",
    0x0C: "Memo",
    0x16: "AutoInc",
    0x18: "Bytes",
}


# ---------------------------------------------------------------------------
# Paradox 7 database parser
# ---------------------------------------------------------------------------

class ParadoxDB:
    """Generic Borland Paradox 7 .DB file parser.

    Reads the file header to extract field definitions (names, types, sizes),
    then iterates over data blocks to yield all records as ordered dicts.
    """

    def __init__(self, filepath):
        self.filepath = filepath
        self.record_size = 0
        self.header_size = 0
        self.file_type = 0
        self.block_size = 0
        self.num_records = 0
        self.num_fields = 0
        self.fields = []  # list of (name, type_code, type_name, size)
        self._data = b""

        self._parse()

    def _parse(self):
        """Read the file and parse the header."""
        with open(self.filepath, "rb") as fh:
            self._data = fh.read()

        if len(self._data) < 0x80:
            raise ValueError(f"File too small to be a Paradox DB: {self.filepath}")

        # Header fields
        self.record_size = struct.unpack_from("<H", self._data, 0x00)[0]
        self.header_size = struct.unpack_from("<H", self._data, 0x02)[0]
        self.file_type = self._data[0x04]
        block_mult = self._data[0x05]
        self.block_size = block_mult * 1024 if block_mult > 0 else 1024
        self.num_records = struct.unpack_from("<H", self._data, 0x06)[0]
        self.num_fields = self._data[0x21]

        if self.num_fields == 0:
            raise ValueError(f"No fields defined in header: {self.filepath}")

        # Field type/size pairs at offset 0x78
        field_pairs = []
        offset = 0x78
        for _ in range(self.num_fields):
            if offset + 2 > len(self._data):
                raise ValueError(f"Header truncated reading field pairs: {self.filepath}")
            ftype = self._data[offset]
            fsize = self._data[offset + 1]
            field_pairs.append((ftype, fsize))
            offset += 2

        # Skip num_fields * 4 bytes of pointer data
        offset += self.num_fields * 4

        # Find and skip the temp filename (null-terminated string like "resttemp.DB")
        temp_end = self._data.find(b"\x00", offset)
        if temp_end == -1:
            raise ValueError(f"Cannot find temp filename terminator: {self.filepath}")
        offset = temp_end + 1

        # Skip zero padding until the first field name
        while offset < self.header_size and self._data[offset] == 0x00:
            offset += 1

        # Read field names as consecutive null-terminated ASCII strings
        field_names = []
        for _ in range(self.num_fields):
            if offset >= self.header_size:
                field_names.append(f"field_{len(field_names)}")
                continue
            name_end = self._data.find(b"\x00", offset, self.header_size)
            if name_end == -1:
                # Take whatever remains
                name_bytes = self._data[offset:self.header_size]
                offset = self.header_size
            else:
                name_bytes = self._data[offset:name_end]
                offset = name_end + 1
            try:
                field_names.append(name_bytes.decode("ascii", errors="replace"))
            except Exception:
                field_names.append(f"field_{len(field_names)}")

        # Build field list
        self.fields = []
        for i, (ftype, fsize) in enumerate(field_pairs):
            name = field_names[i] if i < len(field_names) else f"field_{i}"
            type_name = FIELD_TYPES.get(ftype, f"Unknown(0x{ftype:02x})")
            self.fields.append((name, ftype, type_name, fsize))

    def records(self):
        """Yield all records as OrderedDicts.

        Iterates over the linked list of data blocks starting at header_size.
        Each block has a 6-byte header followed by record data.
        """
        if self.record_size == 0:
            return

        total_yielded = 0
        block_offset = self.header_size

        while block_offset > 0 and block_offset + 6 <= len(self._data):
            # Block header: next_block(2), prev_block(2), last_rec_offset(2)
            next_block = struct.unpack_from("<H", self._data, block_offset)[0]
            _prev_block = struct.unpack_from("<H", self._data, block_offset + 2)[0]
            last_rec_offset = struct.unpack_from("<H", self._data, block_offset + 4)[0]

            # last_rec_offset is the byte offset of the last record in the
            # block (relative to the data area). Record count is therefore
            # (last_rec_offset // record_size) + 1.
            num_recs = (last_rec_offset // self.record_size) + 1

            data_start = block_offset + 6

            for rec_idx in range(num_recs):
                if total_yielded >= self.num_records:
                    break
                rec_offset = data_start + rec_idx * self.record_size
                if rec_offset + self.record_size > len(self._data):
                    break

                rec_data = self._data[rec_offset:rec_offset + self.record_size]
                record = self._decode_record(rec_data)
                yield record
                total_yielded += 1

            # Follow the linked list
            if next_block == 0:
                break
            block_offset = self.header_size + (next_block - 1) * self.block_size

    def _decode_record(self, rec_data):
        """Decode a single record from raw bytes into an OrderedDict."""
        record = OrderedDict()
        offset = 0

        for name, ftype, type_name, fsize in self.fields:
            if offset + fsize > len(rec_data):
                record[name] = None
                offset += fsize
                continue

            raw = rec_data[offset:offset + fsize]

            if ftype == 0x01:  # Alpha
                record[name] = self._decode_alpha(raw)
            elif ftype == 0x02:  # Date
                record[name] = self._decode_date(raw)
            elif ftype == 0x03:  # Short (2 bytes)
                record[name] = self._decode_short(raw)
            elif ftype == 0x04:  # Long (4 bytes)
                record[name] = self._decode_long(raw)
            elif ftype == 0x06:  # Number (8-byte float)
                record[name] = self._decode_number(raw)
            elif ftype == 0x09:  # Logical (1 byte)
                record[name] = self._decode_logical(raw)
            elif ftype == 0x0C:  # Memo
                record[name] = self._decode_alpha(raw)
            elif ftype == 0x16:  # AutoInc (4 bytes)
                record[name] = self._decode_long(raw)
            elif ftype == 0x18:  # Bytes
                record[name] = raw.hex().upper()
            else:
                record[name] = raw.hex().upper()

            offset += fsize

        return record

    @staticmethod
    def _decode_alpha(raw):
        """Decode an Alpha (string) field, stripping trailing nulls/spaces."""
        try:
            text = raw.decode("ascii", errors="replace")
        except Exception:
            text = raw.hex()
        return text.rstrip("\x00").rstrip()

    @staticmethod
    def _decode_short(raw):
        """Decode a 2-byte Short integer (big-endian, XOR 0x8000, signed)."""
        if len(raw) < 2:
            return 0
        val = struct.unpack(">H", raw)[0]
        val ^= 0x8000
        if val >= 0x8000:
            val -= 0x10000
        return val

    @staticmethod
    def _decode_long(raw):
        """Decode a 4-byte Long integer (big-endian, XOR 0x80000000, signed)."""
        if len(raw) < 4:
            return 0
        val = struct.unpack(">I", raw)[0]
        val ^= 0x80000000
        if val >= 0x80000000:
            val -= 0x100000000
        return val

    @staticmethod
    def _decode_number(raw):
        """Decode an 8-byte Number (float) field.

        Encoding: big-endian IEEE 754 double with sign encoding.
        If the high bit of the first byte is set, XOR 0x80 on the first byte.
        Otherwise, bitwise complement all 8 bytes.
        """
        if len(raw) < 8:
            return 0.0
        buf = bytearray(raw)
        if buf[0] == 0x00 and all(b == 0x00 for b in buf):
            return 0.0
        if buf[0] & 0x80:
            buf[0] ^= 0x80
        else:
            buf = bytearray(b ^ 0xFF for b in buf)
        return struct.unpack(">d", bytes(buf))[0]

    @staticmethod
    def _decode_date(raw):
        """Decode a 4-byte Date field (big-endian long -> days since 1/1/0001)."""
        if len(raw) < 4:
            return ""
        val = struct.unpack(">I", raw)[0]
        val ^= 0x80000000
        if val == 0 or val < 0:
            return ""
        # Paradox dates: number of days since January 1, year 1
        # Python's datetime can't handle year 1 easily, so compute manually
        try:
            import datetime
            base = datetime.date(1, 1, 1)
            delta = datetime.timedelta(days=val - 1)
            d = base + delta
            return d.isoformat()
        except (ValueError, OverflowError):
            return f"day#{val}"

    @staticmethod
    def _decode_logical(raw):
        """Decode a 1-byte Logical (boolean) field.

        Paradox encoding: 0x00 = null, 0x80 = False, > 0x80 = True.
        """
        if len(raw) < 1:
            return None
        val = raw[0]
        if val == 0x00:
            return None  # null/unset
        return val > 0x80  # 0x80 = False, > 0x80 = True

    def __repr__(self):
        return (
            f"ParadoxDB({self.filepath!r}, "
            f"fields={self.num_fields}, records={self.num_records}, "
            f"record_size={self.record_size}, block_size={self.block_size})"
        )


# ---------------------------------------------------------------------------
# File discovery helpers
# ---------------------------------------------------------------------------

def find_case_insensitive(directory, filename):
    """Find a file in a directory with case-insensitive matching.

    Returns the full path if found, None otherwise.
    """
    if not os.path.isdir(directory):
        return None
    target = filename.lower()
    for entry in os.listdir(directory):
        if entry.lower() == target:
            return os.path.join(directory, entry)
    return None


def find_panel_dir(data_dir):
    """Find the panel family subdirectory within the data directory.

    Checks CLIENTI.DB to determine the panel type, then looks for the
    matching family directory (Spring/, Kyo300/, etc.).

    Returns:
        tuple: (panel_dir_path, panel_info_dict) or (None, None)
    """
    clienti_path = find_case_insensitive(data_dir, "CLIENTI.DB")
    panel_info = {}

    if clienti_path:
        try:
            db = ParadoxDB(clienti_path)
            for record in db.records():
                panel_info = dict(record)
                break  # Only need the first record
        except Exception as exc:
            sys.stderr.write(f"Warning: could not parse CLIENTI.DB: {exc}\n")

    # Try each known family directory
    for family_dir in PANEL_FAMILY_DIRS:
        candidate = find_case_insensitive(data_dir, family_dir)
        if candidate and os.path.isdir(candidate):
            return candidate, panel_info

    return None, panel_info


def load_table(panel_dir, table_name):
    """Load a table from the panel family directory.

    Args:
        panel_dir: Path to the panel family directory (e.g., .../Spring/)
        table_name: Logical table name (e.g., "zones", "keyfobs")

    Returns:
        tuple: (ParadoxDB instance, list of record dicts)
    """
    filename = TABLE_FILES.get(table_name)
    if not filename:
        raise ValueError(f"Unknown table: {table_name}")

    db_path = find_case_insensitive(panel_dir, filename)
    if not db_path:
        raise FileNotFoundError(
            f"Table file not found: {filename} in {panel_dir}"
        )

    db = ParadoxDB(db_path)
    records = list(db.records())
    return db, records


# ---------------------------------------------------------------------------
# Secret redaction
# ---------------------------------------------------------------------------

def redact_value(value, show_secrets=False):
    """Redact a sensitive value, showing only the last 4 characters."""
    if show_secrets:
        return str(value)
    s = str(value)
    if len(s) <= 4:
        return "**" + s
    return "**" + s[-4:]


def is_secret_field(field_name):
    """Check if a field name likely contains sensitive data (ESN, serial, code)."""
    indicators = [
        "esn", "serial", "seriale", "serialnumber",
        "codice", "code", "pin", "password", "chiave",
        "telefono", "numero", "phone",
    ]
    lower = field_name.lower()
    return any(ind in lower for ind in indicators)


# ---------------------------------------------------------------------------
# Terminal table formatting
# ---------------------------------------------------------------------------

def format_table(headers, rows, max_col_width=40):
    """Format a list of rows as a simple text table.

    Args:
        headers: List of column header strings.
        rows: List of lists, each inner list is one row of values.
        max_col_width: Maximum column width before truncation.

    Returns:
        str: Formatted table string.
    """
    if not rows:
        return "  (no data)\n"

    # Compute column widths
    widths = [len(str(h)) for h in headers]
    for row in rows:
        for i, val in enumerate(row):
            if i < len(widths):
                widths[i] = max(widths[i], min(len(str(val)), max_col_width))

    # Cap widths
    widths = [min(w, max_col_width) for w in widths]

    lines = []

    # Header
    hdr = "  ".join(str(h).ljust(w) for h, w in zip(headers, widths))
    lines.append(hdr)
    lines.append("  ".join("-" * w for w in widths))

    # Rows
    for row in rows:
        cells = []
        for i, val in enumerate(row):
            s = str(val)
            w = widths[i] if i < len(widths) else 20
            if len(s) > w:
                s = s[:w - 1] + "\u2026"
            cells.append(s.ljust(w))
        lines.append("  ".join(cells))

    return "\n".join(lines) + "\n"


# ---------------------------------------------------------------------------
# Table-specific formatters
# ---------------------------------------------------------------------------

def format_zones(records, show_secrets=False):
    """Format zone configuration records."""
    headers = ["#", "Name", "Type", "Area", "ESN"]
    rows = []
    for i, rec in enumerate(records, 1):
        name = _get_field(rec, ["nome", "name", "descrizione"], "")
        zone_type = _get_field(rec, ["tipo_comando_bilanciament", "tipo", "type", "tipozona"], "")
        area = _get_field(rec, ["area", "partizione", "partition"], "")
        esn = _get_field(rec, ["serialnumber", "esn", "serial", "seriale", "codiceradio"], "")

        if isinstance(zone_type, (int, float)):
            zone_type = str(int(zone_type))
        if isinstance(area, (int, float)):
            area = str(int(area))

        if esn and is_secret_field("esn"):
            esn = redact_value(esn, show_secrets)

        rows.append([i, name, zone_type, area, esn])

    return format_table(headers, rows)


def format_keyfobs(records, show_secrets=False):
    """Format wireless keyfob configuration records."""
    headers = ["#", "Name", "ESN", "Code"]
    rows = []
    for i, rec in enumerate(records, 1):
        name = _get_field(rec, ["nome", "name", "descrizione"], "")
        esn = _get_field(rec, ["seriale", "serialnumber", "esn", "serial", "codiceradio"], "")
        buttons = _get_field(rec, ["codice", "tasti", "buttons", "pulsanti"], "")

        if esn and is_secret_field("esn"):
            esn = redact_value(esn, show_secrets)

        rows.append([i, name, esn, buttons])

    return format_table(headers, rows)


def format_codes(records, show_secrets=False):
    """Format user code records."""
    headers = ["#", "Name", "Code", "Level", "Partitions"]
    rows = []
    for i, rec in enumerate(records, 1):
        name = _get_field(rec, ["nome", "name", "descrizione"], "")
        code = _get_field(rec, ["codice", "code", "pin"], "")
        level = _get_field(rec, ["livello", "level", "tipo"], "")
        parts = _get_field(rec, ["partizione", "partition", "area", "aree"], "")

        if code and is_secret_field("codice"):
            code = redact_value(code, show_secrets)
        if isinstance(level, (int, float)):
            level = str(int(level))

        rows.append([i, name, code, level, parts])

    return format_table(headers, rows)


def format_partitions(records, show_secrets=False):
    """Format partition configuration records."""
    headers = ["#", "Name", "Timer Entry", "Timer Exit"]
    rows = []
    for i, rec in enumerate(records, 1):
        name = _get_field(rec, ["nome", "name", "descrizione"], "")
        t_entry = _get_field(rec, ["tempo_ingresso", "timer_entry", "tingresso", "tentrata"], "")
        t_exit = _get_field(rec, ["tempo_uscita", "timer_exit", "tuscita"], "")

        if isinstance(t_entry, (int, float)):
            t_entry = f"{int(t_entry)}s"
        if isinstance(t_exit, (int, float)):
            t_exit = f"{int(t_exit)}s"

        rows.append([i, name, t_entry, t_exit])

    return format_table(headers, rows)


def format_outputs(records, show_secrets=False):
    """Format output configuration records."""
    headers = ["#", "Name", "Type", "Partitions"]
    rows = []
    for i, rec in enumerate(records, 1):
        name = _get_field(rec, ["nome", "name", "descrizione"], "")
        out_type = _get_field(rec, ["tipo", "type", "tipouscita"], "")
        parts = _get_field(rec, ["partizione", "partition", "area", "aree"], "")

        if isinstance(out_type, (int, float)):
            out_type = str(int(out_type))

        rows.append([i, name, out_type, parts])

    return format_table(headers, rows)


def format_events(records, show_secrets=False):
    """Format event routing matrix records."""
    headers = ["#", "Event", "Destinations"]
    rows = []
    for i, rec in enumerate(records, 1):
        # Collect all fields as a compact representation
        values = list(rec.values())
        event_desc = str(values[0]) if values else ""
        dests = ", ".join(str(v) for v in values[1:] if v) if len(values) > 1 else ""
        rows.append([i, event_desc, dests])

    return format_table(headers, rows)


def format_generic(records, show_secrets=False):
    """Format any table generically using all field names as columns."""
    if not records:
        return "  (no records)\n"

    # Use field names from the first record
    headers = list(records[0].keys())
    rows = []
    for rec in records:
        row = []
        for h in headers:
            val = rec.get(h, "")
            if is_secret_field(h) and val:
                val = redact_value(val, show_secrets)
            if isinstance(val, float) and val == int(val):
                val = int(val)
            row.append(val)
        rows.append(row)

    return format_table(headers, rows)


# Table name -> formatter function
TABLE_FORMATTERS = {
    "zones": format_zones,
    "keyfobs": format_keyfobs,
    "codes": format_codes,
    "partitions": format_partitions,
    "outputs": format_outputs,
    "events": format_events,
}


def _get_field(record, candidates, default=""):
    """Get a field value from a record, trying multiple candidate field names.

    Case-insensitive matching against the record keys.
    """
    lower_keys = {k.lower(): k for k in record}
    for candidate in candidates:
        if candidate.lower() in lower_keys:
            val = record[lower_keys[candidate.lower()]]
            return val if val is not None else default
    return default


# ---------------------------------------------------------------------------
# Summary mode
# ---------------------------------------------------------------------------

def format_summary(data_dir, show_secrets=False):
    """Produce a panel configuration summary from all available tables."""
    lines = []

    panel_dir, panel_info = find_panel_dir(data_dir)

    lines.append("=" * 60)
    lines.append("PANEL CONFIGURATION SUMMARY")
    lines.append("=" * 60)
    lines.append("")

    # Panel info from CLIENTI.DB
    if panel_info:
        tipo = None
        for key in ["tipocentrale", "TipoCentrale", "tipo"]:
            if key in panel_info:
                tipo = panel_info[key]
                break
            for k in panel_info:
                if k.lower() == key.lower():
                    tipo = panel_info[k]
                    break
            if tipo is not None:
                break

        model_name = "Unknown"
        if tipo is not None:
            tipo_int = int(tipo) if isinstance(tipo, (int, float)) else 0
            model_name = PANEL_MODELS.get(tipo_int, f"Unknown (code {tipo_int})")
            lines.append(f"Panel model:         {model_name} (type={tipo_int})")
        else:
            lines.append(f"Panel model:         Unknown")

        fw = None
        for key in ["rel_firmware", "firmware", "versione", "version", "fwversion", "sw"]:
            for k in panel_info:
                if k.lower() == key.lower():
                    fw = panel_info[k]
                    break
            if fw is not None:
                break
        if fw:
            lines.append(f"Firmware:            {fw}")

        # Show all CLIENTI.DB fields for transparency
        lines.append("")
        lines.append("CLIENTI.DB fields:")
        for key, val in panel_info.items():
            if is_secret_field(key) and val:
                val = redact_value(val, show_secrets)
            lines.append(f"  {key}: {val}")
    else:
        lines.append("Panel model:         (CLIENTI.DB not found or empty)")

    lines.append("")

    if not panel_dir:
        lines.append("Error: no panel family directory found.")
        lines.append(f"Searched in: {data_dir}")
        lines.append(f"Expected one of: {', '.join(PANEL_FAMILY_DIRS)}")
        lines.append("=" * 60)
        return "\n".join(lines)

    lines.append(f"Data directory:      {panel_dir}")
    lines.append("")

    # Load each table and report counts
    table_summaries = [
        ("zones", "Zones"),
        ("keyfobs", "Keyfobs"),
        ("codes", "User codes"),
        ("partitions", "Partitions"),
        ("outputs", "Outputs"),
        ("expanders", "Expanders"),
        ("keypads", "Keypads"),
        ("events", "Event routing"),
        ("timers", "Timers"),
        ("phone", "ARC phone numbers"),
        ("activators", "Activators"),
        ("messages", "Voice messages"),
        ("schedules", "Time schedules"),
    ]

    lines.append("-" * 60)
    lines.append("TABLE OVERVIEW")
    lines.append("-" * 60)
    lines.append("")

    for table_name, label in table_summaries:
        try:
            db, records = load_table(panel_dir, table_name)
            configured = _count_configured(records)
            lines.append(f"  {label + ':':<22s} {configured:>3d} configured / {len(records)} total")
        except FileNotFoundError:
            lines.append(f"  {label + ':':<22s} (file not found)")
        except Exception as exc:
            lines.append(f"  {label + ':':<22s} (error: {exc})")

    lines.append("")

    # Detailed sections for key tables
    _append_zone_summary(lines, panel_dir, show_secrets)
    _append_keyfob_summary(lines, panel_dir, show_secrets)
    _append_partition_summary(lines, panel_dir, show_secrets)
    _append_code_summary(lines, panel_dir, show_secrets)
    _append_phone_summary(lines, panel_dir, show_secrets)

    lines.append("=" * 60)
    return "\n".join(lines)


def _count_configured(records, table_name=None):
    """Count records that appear to have meaningful user configuration.

    Skips auto-increment IDs, foreign keys, sequence numbers, and default
    placeholder names (e.g. "Zone 12", "Output 3", "Keypad 05") to avoid
    counting pre-allocated but unconfigured rows.
    """
    # Fields that are structural/default, not indicative of user configuration
    skip_fields = {
        "serialcli", "serialecli", "id", "indice", "index", "recno",
        "progress", "numero", "num",
        # Zone defaults (all zones have these even when unconfigured)
        "area", "cicli", "glasstapp", "attributi", "doppioImp",
        "tipo_comando_bilanciament", "supervisione", "old_vector",
        "doub", "doubeol",
        # Output defaults
        "segnali_a", "segnali_b", "segnali_c", "segnali_d",
        "segnali_a_appo", "mono_on", "mono_off",
        # Event routing defaults
        "telefono", "codiceevento", "messaggio", "prioritario", "sia",
        "sendsia",
        # Keypad defaults
        "conf_a", "conf_b", "maskaree", "alison", "presente",
        # Partition defaults
        "timeingresso", "timeuscita", "timepreavviso", "timeandzone",
        "timecodeand",
        # Code defaults (keep tipo/aree — they indicate configured codes)
        "gialloaway", "giallostay", "gialloinst",
        "giallodisi", "verdeaway", "verdestay", "verdeinst", "verdedisi",
        # Schedule defaults
        "attivazioni",
        # Timer defaults
        "temporonda", "tempoallarme", "tempovivavoce",
        "tempoinattivita", "temponegligenza",
        "enableinattivita", "enablenegligenza",
        # Phone defaults
        "opzioni_a", "opzioni_b", "opzioni_tel", "tentativi",
        # Activator defaults
        "dati", "new_dati",
        # Options defaults
        "opzioni_tel", "opzioni_gen", "opzioni_gen1", "opzioni_gen2",
        "special_opt", "opzioni_new", "opzioni_new1", "opzioni_new2",
        "opzioni_new3",
    }

    # Default name patterns: "Zone 12", "Output 3", "Keypad 05", "Code 10",
    # "Partition 04", "Reader 01", "Digital Key 5", "Number 3", "Message1"
    import re
    default_name_re = re.compile(
        r"^(Zone|Output|Keypad|Code|Partition|Reader|Digital Key|Number|"
        r"Expander (In|Out)|Message|Attivatore)\s*\d+$",
        re.IGNORECASE,
    )

    # Normalize all skip fields to lowercase
    skip_lower = {f.lower() for f in skip_fields}

    count = 0
    for rec in records:
        has_real_data = False
        for key, val in rec.items():
            if key.lower() in skip_lower:
                continue
            if isinstance(val, str):
                stripped = val.strip()
                if stripped and not default_name_re.match(stripped):
                    has_real_data = True
                    break
            elif isinstance(val, bool):
                # Booleans: only count True as data for non-structural fields
                if val and key.lower() not in ("prioritario",):
                    has_real_data = True
                    break
            elif isinstance(val, (int, float)):
                if val != 0:
                    has_real_data = True
                    break
        if has_real_data:
            count += 1
    return count


def _append_zone_summary(lines, panel_dir, show_secrets):
    """Append a zone detail section to the summary."""
    import re
    default_re = re.compile(r"^Zone\s*\d+$", re.IGNORECASE)

    try:
        _, records = load_table(panel_dir, "zones")
    except (FileNotFoundError, ValueError):
        return

    configured = []
    for i, rec in enumerate(records, 1):
        name = _get_field(rec, ["nome", "name", "descrizione"], "").strip()
        esn = _get_field(rec, ["esn", "serial", "seriale", "serialnumber", "codiceradio"], "")
        esn_str = str(esn).strip() if esn else ""
        # Only show zones with a non-default name or an ESN
        if (name and not default_re.match(name)) or esn_str:
            area = _get_field(rec, ["area", "partizione", "partition"], "")
            if isinstance(area, (int, float)):
                area = str(int(area))
            esn_display = redact_value(esn_str, show_secrets) if esn_str else ""
            configured.append((i, name, area, esn_display))

    if configured:
        lines.append("-" * 60)
        lines.append("CONFIGURED ZONES")
        lines.append("-" * 60)
        lines.append("")
        headers = ["Zone", "Name", "Area", "ESN"]
        rows = [[z, n, a, e] for z, n, a, e in configured]
        lines.append(format_table(headers, rows))


def _append_keyfob_summary(lines, panel_dir, show_secrets):
    """Append a keyfob detail section to the summary."""
    try:
        _, records = load_table(panel_dir, "keyfobs")
    except (FileNotFoundError, ValueError):
        return

    configured = []
    for i, rec in enumerate(records, 1):
        name = _get_field(rec, ["nome", "name", "descrizione"], "").strip()
        esn = _get_field(rec, ["esn", "serial", "seriale", "codiceradio"], "")
        if name or (isinstance(esn, str) and esn.strip()) or (isinstance(esn, (int, float)) and esn != 0):
            esn_display = redact_value(esn, show_secrets) if esn else ""
            configured.append((i, name, esn_display))

    if configured:
        lines.append("-" * 60)
        lines.append("ENROLLED KEYFOBS")
        lines.append("-" * 60)
        lines.append("")
        headers = ["#", "Name", "ESN"]
        rows = list(configured)
        lines.append(format_table(headers, rows))


def _append_partition_summary(lines, panel_dir, show_secrets):
    """Append a partition detail section to the summary."""
    import re
    default_re = re.compile(r"^Partition\s*\d+$", re.IGNORECASE)

    try:
        _, records = load_table(panel_dir, "partitions")
    except (FileNotFoundError, ValueError):
        return

    configured = []
    for i, rec in enumerate(records, 1):
        name = _get_field(rec, ["nome", "name", "descrizione"], "").strip()
        if name and not default_re.match(name):
            t_entry = _get_field(rec, ["timeingresso"], "")
            t_exit = _get_field(rec, ["timeuscita"], "")
            entry_s = f"{int(t_entry)}s" if isinstance(t_entry, (int, float)) else str(t_entry)
            exit_s = f"{int(t_exit)}s" if isinstance(t_exit, (int, float)) else str(t_exit)
            configured.append((i, name, entry_s, exit_s))

    if configured:
        lines.append("-" * 60)
        lines.append("PARTITIONS")
        lines.append("-" * 60)
        lines.append("")
        headers = ["#", "Name", "Entry", "Exit"]
        rows = list(configured)
        lines.append(format_table(headers, rows))


def _append_code_summary(lines, panel_dir, show_secrets):
    """Append a user code summary section."""
    import re
    default_re = re.compile(r"^Code\s*\d+$", re.IGNORECASE)

    try:
        _, records = load_table(panel_dir, "codes")
    except (FileNotFoundError, ValueError):
        return

    configured = []
    for i, rec in enumerate(records, 1):
        name = _get_field(rec, ["nome", "name", "descrizione"], "").strip()
        tipo = _get_field(rec, ["tipo", "type", "livello", "level"], 0)
        aree = _get_field(rec, ["aree", "area", "partizione"], 0)
        # A code is configured if it has a non-default name, or Tipo > 0
        tipo_val = int(tipo) if isinstance(tipo, (int, float)) else 0
        if (name and not default_re.match(name)) or tipo_val > 0:
            aree_val = int(aree) if isinstance(aree, (int, float)) else 0
            tipo_labels = {0: "-", 1: "Master", 2: "Standard"}
            configured.append((
                i,
                name,
                tipo_labels.get(tipo_val, str(tipo_val)),
                aree_val if aree_val > 0 else "-",
            ))

    if configured:
        lines.append("-" * 60)
        lines.append("USER CODES")
        lines.append("-" * 60)
        lines.append("")
        headers = ["#", "Name", "Type", "Areas"]
        rows = list(configured)
        lines.append(format_table(headers, rows))


def _append_phone_summary(lines, panel_dir, show_secrets):
    """Append an ARC phone number summary section."""
    try:
        _, records = load_table(panel_dir, "phone")
    except (FileNotFoundError, ValueError):
        return

    configured = []
    for i, rec in enumerate(records, 1):
        # Try common field names for phone numbers
        phone = _get_field(rec, ["numero", "telefono", "phone", "number"], "")
        name = _get_field(rec, ["nome", "name", "descrizione"], "")
        if (isinstance(phone, str) and phone.strip()) or (isinstance(phone, (int, float)) and phone != 0):
            phone_display = redact_value(phone, show_secrets)
            configured.append((i, name, phone_display))

    if configured:
        lines.append("-" * 60)
        lines.append("ARC PHONE NUMBERS")
        lines.append("-" * 60)
        lines.append("")
        headers = ["#", "Name", "Number"]
        rows = list(configured)
        lines.append(format_table(headers, rows))


# ---------------------------------------------------------------------------
# Raw .DB file dump
# ---------------------------------------------------------------------------

def format_raw_db(filepath, show_secrets=False):
    """Parse a single .DB file and format all records."""
    db = ParadoxDB(filepath)
    records = list(db.records())

    lines = []
    lines.append(f"File:           {filepath}")
    lines.append(f"File type:      0x{db.file_type:02x}")
    lines.append(f"Record size:    {db.record_size} bytes")
    lines.append(f"Header size:    {db.header_size} bytes")
    lines.append(f"Block size:     {db.block_size} bytes")
    lines.append(f"Num records:    {db.num_records} (header), {len(records)} (parsed)")
    lines.append(f"Num fields:     {db.num_fields}")
    lines.append("")
    lines.append("Fields:")
    for i, (name, ftype, type_name, fsize) in enumerate(db.fields):
        lines.append(f"  [{i:2d}] {name:<30s}  {type_name:<10s} ({fsize} bytes)")
    lines.append("")

    if records:
        lines.append(f"Records ({len(records)}):")
        lines.append("")

        # Use generic table format
        headers = [f.split(".")[0] if "." in f else f for f, _, _, _ in db.fields]
        rows = []
        for rec in records:
            row = []
            for h_name, ftype, _, _ in db.fields:
                val = rec.get(h_name, "")
                if is_secret_field(h_name) and val and not show_secrets:
                    val = redact_value(val, show_secrets)
                if isinstance(val, float) and val == int(val):
                    val = int(val)
                row.append(val)
            rows.append(row)
        lines.append(format_table(headers, rows))
    else:
        lines.append("(no records)")

    return "\n".join(lines)


# ---------------------------------------------------------------------------
# JSON export
# ---------------------------------------------------------------------------

def export_json(data_dir, show_secrets=False):
    """Export all tables as a single JSON document."""
    panel_dir, panel_info = find_panel_dir(data_dir)

    result = OrderedDict()
    result["panel"] = panel_info if panel_info else {}

    if panel_dir:
        result["data_directory"] = panel_dir
        for table_name, filename in TABLE_FILES.items():
            try:
                db, records = load_table(panel_dir, table_name)
                # Redact secrets unless --show-secrets
                clean_records = []
                for rec in records:
                    clean = OrderedDict()
                    for k, v in rec.items():
                        if is_secret_field(k) and v and not show_secrets:
                            v = redact_value(v, show_secrets)
                        if isinstance(v, float) and v != v:  # NaN check
                            v = None
                        clean[k] = v
                    clean_records.append(clean)
                result[table_name] = {
                    "file": filename,
                    "fields": [
                        {"name": name, "type": type_name, "size": size}
                        for name, _, type_name, size in db.fields
                    ],
                    "records": clean_records,
                }
            except FileNotFoundError:
                result[table_name] = {"file": filename, "error": "file not found"}
            except Exception as exc:
                result[table_name] = {"file": filename, "error": str(exc)}

    return json.dumps(result, indent=2, default=str, ensure_ascii=False)


# ---------------------------------------------------------------------------
# CLI entry point
# ---------------------------------------------------------------------------

def build_parser():
    """Build the argument parser."""
    parser = argparse.ArgumentParser(
        prog="bentel-db-extract",
        description=(
            "Parse Borland Paradox 7 database files from KyoUnit alarm panel "
            "configuration backups. Extracts zone configuration, keyfob "
            "enrollment, user codes, partition settings, and other panel "
            "parameters."
        ),
        epilog=(
            "Table names: " + ", ".join(TABLE_FILES.keys()) + ". "
            "Supports all KYO panel families (Spring, Kyo300, NORMA, BGsm, "
            "EASYGATE, GTCOM, OMNIA)."
        ),
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument(
        "path",
        help=(
            "path to the KyoUnit data directory (containing CLIENTI.DB and "
            "panel family subdirectories), or a single .DB file with --raw"
        ),
    )
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument(
        "--table",
        metavar="NAME",
        choices=list(TABLE_FILES.keys()),
        help=(
            "show a specific configuration table: "
            + ", ".join(TABLE_FILES.keys())
        ),
    )
    mode.add_argument(
        "--json",
        action="store_true",
        default=False,
        help="export all tables as a single JSON document",
    )
    mode.add_argument(
        "--raw",
        action="store_true",
        default=False,
        help="parse a single .DB file and dump its structure and records",
    )
    parser.add_argument(
        "--show-secrets",
        action="store_true",
        default=False,
        help=(
            "show full ESN/serial numbers and user codes instead of redacted "
            "values (default: redacted, showing only last 4 characters)"
        ),
    )
    return parser


def main():
    """CLI entry point."""
    parser = build_parser()
    args = parser.parse_args()

    # Raw mode: parse a single .DB file
    if args.raw:
        if not os.path.isfile(args.path):
            sys.stderr.write(f"Error: file not found: {args.path}\n")
            sys.exit(1)
        try:
            print(format_raw_db(args.path, show_secrets=args.show_secrets))
        except Exception as exc:
            sys.stderr.write(f"Error parsing {args.path}: {exc}\n")
            sys.exit(1)
        return

    # Directory modes
    data_dir = args.path
    if not os.path.isdir(data_dir):
        sys.stderr.write(f"Error: not a directory: {data_dir}\n")
        sys.exit(1)

    if args.json:
        try:
            print(export_json(data_dir, show_secrets=args.show_secrets))
        except Exception as exc:
            sys.stderr.write(f"Error: {exc}\n")
            sys.exit(1)
        return

    if args.table:
        panel_dir, _ = find_panel_dir(data_dir)
        if not panel_dir:
            sys.stderr.write(
                f"Error: no panel family directory found in {data_dir}\n"
            )
            sys.exit(1)

        try:
            db, records = load_table(panel_dir, args.table)
        except FileNotFoundError as exc:
            sys.stderr.write(f"Error: {exc}\n")
            sys.exit(1)
        except Exception as exc:
            sys.stderr.write(f"Error parsing table: {exc}\n")
            sys.exit(1)

        sys.stderr.write(
            f"Table: {args.table} ({TABLE_FILES[args.table]})\n"
            f"  {len(records)} records, {db.num_fields} fields\n\n"
        )

        formatter = TABLE_FORMATTERS.get(args.table, format_generic)
        print(formatter(records, show_secrets=args.show_secrets))
        return

    # Default: summary mode
    try:
        print(format_summary(data_dir, show_secrets=args.show_secrets))
    except Exception as exc:
        sys.stderr.write(f"Error: {exc}\n")
        sys.exit(1)


if __name__ == "__main__":
    main()
