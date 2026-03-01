# Bentel KYO Tools

Utilities for capturing, analyzing, and extracting configuration data from Bentel KYO alarm panels.

## USB Capture Guide

This guide explains how to record USB sessions between KyoUnit and the alarm panel, export them as text logs, and parse them with the included extraction script.

### Requirements

- **Windows PC** (or a Windows VM — works with UTM, Parallels, or VMware on macOS)
- **HHD Device Monitoring Studio** — the **Premium version** is required for data recording and text export (14-day free trial available)
- **USB Monitor driver** (installed automatically with HHD Device Monitoring Studio)
- **USB-to-serial UART cable** — FTDI FT232R recommended
- **Null modem adapter** — for crossing TX/RX between the cable and the panel's serial port
- **Bentel KyoUnit software** — for communicating with the panel

> **Tested on:** Apple Silicon Mac with UTM running Windows 11 (Arm64).

### Setup

1. Connect the USB UART cable with a null modem adapter to the alarm panel's serial port.
2. If using a VM, attach the USB device to the Windows guest (UTM: USB icon in toolbar, Parallels: Devices menu, VMware: Removable Devices menu).
3. Install FTDI drivers — download the **CDM ARM64 WHQL Certified** package from [FTDI's website](https://ftdichip.com/drivers/). For macOS ARM64 VMs (UTM/Parallels), the ARM64 driver is required.
4. The device should appear as **FT232R USB UART** (FT232 Serial (UART) IC) in Device Manager.

### Capturing

1. Open **HHD Device Monitoring Studio**.
2. Select the **FT232R USB UART** device.
3. Click **Start Monitoring**.
4. Enable **Data Recording** to save the raw session.
5. Click **Add to Session**, then configure start options as needed.
6. Open KyoUnit and perform the operations you want to capture (upload config, arm/disarm, read status, etc.).
7. Click **Stop Monitoring** when the session is complete.

### Exporting

1. Open the recorded session file.
2. Select the time range for playback — click **Ok** for the full range.
3. Go to **Exporters > Text Exporter**.
4. Under **Export Settings**, select the destination file path.
5. Click to start the conversion — this may take a while for large sessions.
6. Sometimes you need to click **Next** to advance through the session during export.

> **Important:** You must use the **Text Exporter**, not the binary format. The binary recording format (`.dmslog`) is a proprietary HHD format that cannot be parsed externally. The Text Exporter produces a UTF-16LE encoded text file that the extraction script can read.

### Analyzing

Once exported, use the `bentel-usb-extract.py` script to parse the capture:

```bash
# Show only state changes (default)
python3 bentel-usb-extract.py capture.log

# Show all protocol messages
python3 bentel-usb-extract.py capture.log --raw

# Show capture summary
python3 bentel-usb-extract.py capture.log --summary

# Export as JSON
python3 bentel-usb-extract.py capture.log --json
```

The script reassembles fragmented USB bulk transfers, validates protocol checksums, and decodes known Bentel KYO commands including sensor status, partition status, software version queries, and configuration read/write operations.

### Reading the output

- Start with `--summary` to identify dominant commands and capture phases.
- Use `--raw` when you need full frame-level detail.
- Raw configuration downloads (`0F` writes) are reassembled as a single command line containing:
  - the 6-byte header (`0F ADDR_LO ADDR_HI LEN 00 CHK`)
  - followed by the raw data block (`LEN+1` bytes) and trailing data checksum.
- In summary/counts, `0F`/`F0` commands are grouped by their 6-byte header so repeated config writes are easier to compare.
- Some HHD exports split one `UsbPayload` across multiple hex-dump lines; the parser now joins those automatically.

### Troubleshooting

- If Python cannot write `.pyc` files (restricted directory), run with:

```bash
PYTHONDONTWRITEBYTECODE=1 python3 bentel-usb-extract.py capture.log --summary
```

### Alternative: Host-side capture with socat (macOS)

If using UTM on macOS, you may consider capturing on the host instead of inside the VM. In theory, `socat` can bridge the USB serial device to a TCP port that the Windows VM connects to, while logging the raw traffic:

```bash
socat -x -v FILE:/dev/cu.usbserial-XXXXX,rawer,ispeed=9600,ospeed=9600,cs8,parenb,clocal \
  TCP-LISTEN:9600,reuseaddr,fork 2> ~/Desktop/kyo_capture.txt
```

> **Note:** This approach was tested but **did not work reliably with UTM**. While the TCP serial server was functional (Windows boot logs were visible through it), KyoUnit could not communicate with the panel through this bridge. The issue may be timing-sensitive — the KYO protocol uses tight request/response windows that TCP latency disrupts.
>
> **Warning:** The serial device must be configured while the panel is disconnected or powered off. Attaching a serial monitor to a live panel's BPI bus can cause unexpected behavior (including triggering the siren). Always connect the serial cable before powering on the capture setup.

It is likely possible to make host-side recording work with a different VM solution (e.g., Parallels, which has native USB passthrough with lower latency), but this has not been tested. For now, the recommended approach remains using HHD Device Monitoring Studio inside the Windows VM.

### Notes

- Large captures (30+ minutes of monitoring) can produce files with thousands of USB entries — the script handles these efficiently.
- The script works with all KYO panel variants (KYO4, KYO8, KYO8G, KYO32, KYO32G).
- The script is a single self-contained Python file with no external dependencies (stdlib only).
- **Wireshark/USBPcap** was also evaluated as an alternative capture method, but USBPcap does not currently support ARM64 Windows, so it cannot be used in ARM64 VMs on Apple Silicon.

## KyoUnit Database Extractor

Parses Borland Paradox 7 `.DB` database files from KyoUnit panel configuration backups. Extracts zone configuration, keyfob enrollment, user codes, partition settings, and other panel parameters.

### Background

KyoUnit (Bentel's panel configuration software for Windows) stores panel configuration in a directory of Borland Paradox 7 `.DB` files. When you save or back up a configuration in KyoUnit, it writes to a data directory with the following structure:

```
Data/
├── CLIENTI.DB              # Customer index (panel type, firmware version)
└── Spring/                 # Panel family directory (Spring = KYO4/8/32)
    ├── Zone.DB             # Zone configuration (names, ESNs, area assignments)
    ├── Radiochiavi.DB      # Wireless keyfob configuration (names, ESNs)
    ├── Codici.DB           # User codes
    ├── Partizioni.DB       # Partition configuration
    ├── Uscite.DB           # Output configuration
    ├── Expander.DB         # BPI expander configuration
    ├── ConfigTast.DB       # Keypad configuration
    ├── ImpEventi.DB        # Event routing matrix
    ├── opzioni.DB          # Panel options
    ├── Tempi.DB            # System timers
    ├── Agenda.DB           # ARC phone numbers
    ├── Attivatori.DB       # Digital key/activator configuration
    ├── Messaggi.DB         # Voice messages
    └── ProgOrario.DB       # Time schedules
```

Other panel families use different subdirectory names (Kyo300/, NORMA/, NORMA2/, BGsm/, EASYGATE/, GTCOM/, OMNIA/) but follow the same file format and table structure.

### Usage

```bash
# Show panel summary (auto-detects panel family from CLIENTI.DB)
python3 bentel-db-extract.py /path/to/Data/

# Show a specific configuration table
python3 bentel-db-extract.py /path/to/Data/ --table zones
python3 bentel-db-extract.py /path/to/Data/ --table keyfobs
python3 bentel-db-extract.py /path/to/Data/ --table codes
python3 bentel-db-extract.py /path/to/Data/ --table partitions
python3 bentel-db-extract.py /path/to/Data/ --table outputs
python3 bentel-db-extract.py /path/to/Data/ --table events
python3 bentel-db-extract.py /path/to/Data/ --table expanders
python3 bentel-db-extract.py /path/to/Data/ --table keypads
python3 bentel-db-extract.py /path/to/Data/ --table options
python3 bentel-db-extract.py /path/to/Data/ --table timers
python3 bentel-db-extract.py /path/to/Data/ --table phone

# Export all tables as JSON
python3 bentel-db-extract.py /path/to/Data/ --json

# Parse a single .DB file directly (raw mode)
python3 bentel-db-extract.py /path/to/Zone.DB --raw

# Show full ESN/serial numbers and user codes (redacted by default)
python3 bentel-db-extract.py /path/to/Data/ --show-secrets
```

### Paradox 7 Database Format

The Paradox 7 `.DB` format is a flat-file database format originally created by Borland. Each file is self-contained with a header describing the schema and data blocks containing the records.

**Header layout:**

| Offset | Size | Field |
|--------|------|-------|
| 0x00 | 2B LE | Record size |
| 0x02 | 2B LE | Header size (data blocks start here) |
| 0x04 | 1B | File type |
| 0x05 | 1B | Block size multiplier (block_size = value * 1024) |
| 0x06 | 2B LE | Number of records |
| 0x21 | 1B | Number of fields |
| 0x78 | 2B * N | Field type/size pairs (N = num_fields) |

**Field types:**

| Code | Type | Size | Encoding |
|------|------|------|----------|
| 0x01 | Alpha | variable | ASCII string, null-padded |
| 0x02 | Date | 4B | Big-endian long, days since 1/1/0001 |
| 0x03 | Short | 2B | Big-endian, XOR 0x8000, signed two's complement |
| 0x04 | Long | 4B | Big-endian, XOR 0x80000000, signed two's complement |
| 0x06 | Number | 8B | Big-endian IEEE 754 double with sign encoding |
| 0x09 | Logical | 1B | 0x00=null, 0x80=false, else true |
| 0x0C | Memo | variable | ASCII string |
| 0x16 | AutoInc | 4B | Same as Long |
| 0x18 | Bytes | variable | Raw binary data |

**Field name discovery:** After the field type/size pairs at 0x78, skip `num_fields * 4` bytes of pointer data, find a null-terminated temp filename string (e.g., `resttemp.DB`), skip past it and any zero padding, then read `num_fields` consecutive null-terminated ASCII strings.

**Data blocks:** Start at `header_size`. Each block is `block_size` bytes with a 6-byte header: `next_block` (2B LE, 1-based linked list, 0=end), `prev_block` (2B LE), `add_data_size` (2B LE). The number of records in a block is `add_data_size / record_size`.

**Integer encoding:** Big-endian with high-bit XOR. For Short values: XOR 0x8000; values >= 0x8000 after XOR are negative (subtract 0x10000). For Long values: XOR 0x80000000; values >= 0x80000000 after XOR are negative (subtract 0x100000000).

**Number (float) encoding:** Big-endian 8-byte IEEE 754 double. If the high bit of the first byte is set, XOR 0x80 on the first byte. Otherwise, bitwise complement all 8 bytes.

### Notes

- ESN/serial numbers and user codes are redacted by default (showing only the last 4 characters like `**ACFE`). Use `--show-secrets` to display full values.
- The `tipocentrale` field in CLIENTI.DB identifies the panel model. Known value: 26 = KYO32M (non-G).
- The Paradox parser is a generic reusable class (`ParadoxDB`) that can parse any Paradox 7 `.DB` file — the table-specific formatting is separate logic on top.
- The script is a single self-contained Python file with no external dependencies (stdlib only).
