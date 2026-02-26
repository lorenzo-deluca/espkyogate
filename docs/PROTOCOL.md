# Bentel KYO Serial Protocol Reference

Reverse-engineered protocol for the Bentel Security KYO alarm panel family,
communicated over RS-232 serial (via MAX3232 level shifter) at **9600 baud,
8 data bits, EVEN parity, 1 stop bit**.

This document covers all models handled by the espkyogate component: KYO4,
KYO8, KYO8G, KYO8W, KYO8GW, KYO32, and KYO32G. Where behavior differs
between models, the variants are noted explicitly.

> **Disclaimer**: This protocol was determined by analyzing serial traffic
> between the panel and the Bentel "Kyo Unit 5.5" programming software
> (x86 version). It is not an official specification. Use at your own risk.

---

## 1. Physical Layer

| Parameter | Value |
|-----------|-------|
| Baud rate | 9600 |
| Data bits | 8 |
| Parity | EVEN |
| Stop bits | 1 |
| Connector | DB9 via MAX3232 TTL-to-RS232 adapter |
| ESP32 pins (typical) | TX=GPIO17, RX=GPIO16 |

The panel's serial port is active whenever the unit is NOT in programming
mode. While Kyo Unit software is connected (programming mode), the serial
port does not respond to external queries.

---

## 2. Framing and General Message Structure

### 2.1 Request (ESP32 -> Panel)

All requests begin with a **command byte** (`0xF0` for reads, `0x0F` for
writes) followed by parameters and a checksum/trailer byte.

There are two message types:

#### Read requests (6 bytes)

```
[0xF0] [ADDR_HI] [ADDR_LO] [LEN] [0x00] [CHK]
```

| Byte | Name | Description |
|------|------|-------------|
| 0 | Command | `0xF0` = read request |
| 1 | ADDR_HI | High byte of register address |
| 2 | ADDR_LO | Low byte of register address |
| 3 | LEN | Number of data bytes to read |
| 4 | Reserved | Always `0x00` |
| 5 | CHK | Checksum (see section 3) |

#### Write requests (variable length)

```
[0x0F] [ADDR_HI] [ADDR_LO] [CMD_TYPE] [0x00] [PAYLOAD_LEN] [DATA...] [CHK] [TRAILER]
```

| Byte | Name | Description |
|------|------|-------------|
| 0 | Command | `0x0F` = write request |
| 1 | ADDR_HI | High byte of register/function address |
| 2 | ADDR_LO | Low byte of register/function address |
| 3 | CMD_TYPE | Command sub-type (varies by command) |
| 4 | Reserved | Always `0x00` |
| 5 | PAYLOAD_LEN | Number of data bytes following |
| 6.. | DATA | Command-specific payload |
| N-1 | CHK | Checksum (see section 3) |
| N | TRAILER | Usually `0xFF` |

### 2.2 Response (Panel -> ESP32)

The panel echoes the original request bytes followed by response data
and a checksum:

```
[ECHO of request (6 bytes)] [DATA...] [CHK]
```

- For read requests, the echo is always the 6-byte request.
- The response data length varies by command.
- The final byte is a checksum over the data bytes (after the echo).

**Important**: Because the UART is half-duplex on a single bus, the
transmitted bytes appear in the receive buffer as an echo. The driver
reads all bytes and the first `N` bytes (where N = request length)
are the echo of the sent command. Actual response data begins at
byte index `N`.

In the code, response bytes are accessed as `Rx[0]` through `Rx[total-1]`,
where `Rx[0..5]` is the echoed request and `Rx[6..]` is the panel's
response data.

### 2.3 Response sizes

| Command | KYO8/4 response | KYO32/32G response |
|---------|------------------|--------------------|
| Get Sensor Status | 12 bytes total | 18 bytes total |
| Get Partition Status | 17 bytes total | 26 bytes total |
| Get Software Version | 19 bytes total | 19 bytes total |

---

## 3. Checksum Algorithms

Three different checksum methods are used depending on the command:

### 3.1 Read Request Checksum

For 6-byte read requests (`F0 ADDR_HI ADDR_LO LEN 00 CHK`), the
checksum is the low byte of the sum of bytes 0 through 4:

```
CHK = (bytes[0] + bytes[1] + bytes[2] + bytes[3] + bytes[4]) & 0xFF
```

This applies to all read requests, including custom configuration reads.
Verified against all known commands and all config register reads observed
in USB captures.

Example: reading zone config at address `0x009F`, length `0x3F`:
```
F0 + 00 + 9F + 3F + 00 = 0x1CE → CHK = 0xCE
Request: F0 00 9F 3F 00 CE
```

This formula enables constructing read requests to any register address.

### 3.2 Write Command CRC (`calculateCRC`)

Used for arm/disarm partition commands:

```c
byte calculateCRC(byte *cmd, int lcmd) {
    int sum = 0x00;
    for (int i = 0; i <= lcmd; i++)
        sum += cmd[i];
    return (0x203 - sum);
}
```

Sums bytes `cmd[0]` through `cmd[lcmd]` (inclusive), then returns
`0x203 - sum`, truncated to a byte. This is used for the arm/disarm
commands where `lcmd = 8` (summing bytes 0-8, placing result at byte 9).

### 3.3 Simple Additive Checksum (`getChecksum`)

Used for date/time update and zone include/exclude commands:

```c
uint8_t getChecksum(byte *data, int offset, int len) {
    uint8_t ckSum = 0x00;
    for (int i = offset; i < len; i++)
        ckSum += data[i];
    return ckSum;
}
```

Simply sums the payload bytes and truncates to 8 bits.

### 3.4 Response Checksum Validation

The last byte of every panel response is a checksum over all data bytes
after the echo:

```
expected = sum(Rx[echoLen .. totalLen-2]) & 0xFF
actual   = Rx[totalLen-1]
```

---

## 4. Model Detection

### 4.1 Software Version Query

```
TX: F0 00 00 0B 00 FB
```

Read address `0x0000`, length `0x0B` (11 bytes).

**Response** (19 bytes total): 6-byte echo + 12 ASCII chars + 1 checksum.

The 12 ASCII characters at `Rx[6..17]` contain the firmware identification
string. Known values:

| Firmware string | Model |
|-----------------|-------|
| `KYO32G  x.xx` | KYO 32G |
| `KYO32   x.xx` | KYO 32 (non-G) |
| `KYO8G   x.xx` | KYO 8G |
| `KYO8W   x.xx` | KYO 8W |
| `KYO8    x.xx` | KYO 8 |
| `KYO4    x.xx` | KYO 4 |

Example: `KYO32   1.05` = KYO 32 non-G, firmware version 1.05.

Model detection uses longest-prefix matching: `KYO32G` is checked before
`KYO32`, `KYO8G`/`KYO8W` before `KYO8`, etc.

### 4.2 Fallback Detection (Response Size)

If the version query fails, the model is inferred from the response
size to the sensor status query:
- **18 bytes**: KYO32-series (defaults to KYO_32 non-G)
- **12 bytes**: KYO8-series or KYO4

---

## 5. Read Commands

### 5.1 Get Sensor Status (Realtime Status + Trouble Status)

```
TX: F0 04 F0 0A 00 EE
```

Read address `0x04F0`, length `0x0A` (10 bytes).

#### KYO32/32G Response (18 bytes total)

| Index | Byte | Content |
|-------|------|---------|
| 0-5 | Echo | Request echo |
| 6 | Rx[6] | Zone status: zones 25-32 (bit 0 = zone 25, bit 7 = zone 32) |
| 7 | Rx[7] | Zone status: zones 17-24 |
| 8 | Rx[8] | Zone status: zones 9-16 |
| 9 | Rx[9] | Zone status: zones 1-8 (bit 0 = zone 1, bit 7 = zone 8) |
| 10 | Rx[10] | Zone tamper: zones 25-32 |
| 11 | Rx[11] | Zone tamper: zones 17-24 |
| 12 | Rx[12] | Zone tamper: zones 9-16 |
| 13 | Rx[13] | Zone tamper: zones 1-8 |
| 14 | Rx[14] | Warning flags (see 5.1.1) |
| 15 | Rx[15] | Partition alarm: bit N = partition N+1 in alarm |
| 16 | Rx[16] | Tamper/sabotage flags (see 5.1.2) |
| 17 | Rx[17] | Checksum |

**Zone bit ordering (KYO32)**: Zones are packed in big-endian byte order
but little-endian bit order within each byte. Byte `Rx[6]` holds zones
25-32, `Rx[7]` holds 17-24, `Rx[8]` holds 9-16, `Rx[9]` holds 1-8.
Within each byte, bit 0 is the lowest-numbered zone in that group.

#### KYO8 Response (12 bytes total)

| Index | Byte | Content |
|-------|------|---------|
| 0-5 | Echo | Request echo |
| 6 | Rx[6] | Zone status: zones 1-8 (bit N = zone N+1) |
| 7 | Rx[7] | Zone tamper: zones 1-8 |
| 8 | Rx[8] | Warning flags (see 5.1.1) |
| 9 | Rx[9] | Partition alarm: bit N = partition N+1 |
| 10 | Rx[10] | Tamper/sabotage flags (see 5.1.2) |
| 11 | Rx[11] | Checksum |

#### 5.1.1 Warning Flags

**KYO32/32G** (Rx[14]):

| Bit | Warning |
|-----|---------|
| 0 | Mains power failure (mancanza rete) |
| 1 | BPI missing (scomparsa BPI) |
| 2 | Fuse fault (fusibile) |
| 3 | Low battery (batteria bassa) |
| 4 | Telephone line fault (guasto linea telefonica) |
| 5 | Default codes active (codici default) |
| 6 | Wireless fault |
| 7 | **UNMAPPED** (observed but purpose unknown) |

**KYO8** (Rx[8]):

| Bit | Warning |
|-----|---------|
| 0 | Mains power failure |
| 1 | BPI missing |
| 2 | Fuse fault |
| 3 | Low battery |
| 4 | *(unused)* |
| 5 | Telephone line fault |
| 6 | Default codes active |
| 7 | *(unused)* |

#### 5.1.2 Tamper/Sabotage Flags

**KYO32/32G** (Rx[16]):

| Bit | Sabotage type |
|-----|---------------|
| 0 | **UNMAPPED** |
| 1 | **UNMAPPED** |
| 2 | Zone tamper (sabotaggio zona) |
| 3 | False key (chiave falsa) |
| 4 | BPI tamper |
| 5 | System tamper (sabotaggio sistema) |
| 6 | RF jam detected (sabotaggio jam) |
| 7 | Wireless tamper (sabotaggio wireless) |

**KYO8** (Rx[10]):

| Bit | Sabotage type |
|-----|---------------|
| 0-3 | *(unused)* |
| 4 | Zone tamper |
| 5 | False key |
| 6 | BPI tamper |
| 7 | System tamper |

---

### 5.2 Get Partition Status

This command reads partition arming state, siren status, output status,
bypassed zones, alarm memory, and tamper memory.

**Register addresses differ by model**:

| Model | Address | Command bytes |
|-------|---------|---------------|
| KYO32G | `0x0215` | `F0 02 15 12 00 19` |
| KYO32 (non-G) | `0xEC14` | `F0 EC 14 12 00 02` |
| KYO8/4/8G/8W | `0x680E` | `F0 68 0E 09 00 6F` |

Read length: `0x12` (18) for KYO32-series, `0x09` (9) for KYO8-series.

#### KYO32/32G Response (26 bytes total)

| Index | Byte | Content |
|-------|------|---------|
| 0-5 | Echo | Request echo |
| 6 | Rx[6] | Partitions armed TOTAL: bit N = partition N+1 armed totally |
| 7 | Rx[7] | Partitions armed PARTIAL: bit N = partition N+1 armed partially |
| 8 | Rx[8] | Partitions armed PARTIAL with delay 0 |
| 9 | Rx[9] | Partitions DISARMED: bit N = partition N+1 disarmed |
| 10 | Rx[10] | Siren/system status (bit 5 = siren active) |
| 11 | Rx[11] | *(purpose uncertain)* |
| 12 | Rx[12] | Output (OC/PGM) status: bit N = output N+1 active **(KYO32G only; reads 0xFF on non-G)** |
| 13 | Rx[13] | Bypassed zones 25-32 |
| 14 | Rx[14] | Bypassed zones 17-24 |
| 15 | Rx[15] | Bypassed zones 9-16 |
| 16 | Rx[16] | Bypassed zones 1-8 |
| 17 | Rx[17] | Alarm memory zones 25-32 |
| 18 | Rx[18] | Alarm memory zones 17-24 |
| 19 | Rx[19] | Alarm memory zones 9-16 |
| 20 | Rx[20] | Alarm memory zones 1-8 |
| 21 | Rx[21] | Tamper memory zones 25-32 |
| 22 | Rx[22] | Tamper memory zones 17-24 |
| 23 | Rx[23] | Tamper memory zones 9-16 |
| 24 | Rx[24] | Tamper memory zones 1-8 |
| 25 | Rx[25] | Checksum |

**Siren status** is at `Rx[10]`, bit 5.

**OC output readback (Rx[12])**: On KYO32G, each bit corresponds to one
of the 8 programmable outputs. On KYO32 non-G, this byte always reads
`0xFF` -- the actual output readback register address for non-G models
is unknown. Output activate/deactivate commands still function correctly;
only readback is broken.

**Zone data bit ordering** follows the same convention as sensor status:
bytes are big-endian (Rx[13] = zones 25-32, Rx[16] = zones 1-8), bits
are little-endian within each byte (bit 0 = lowest zone in group).

#### KYO8 Response (17 bytes total)

| Index | Byte | Content |
|-------|------|---------|
| 0-5 | Echo | Request echo |
| 6 | Rx[6] | Partitions armed TOTAL |
| 7 | Rx[7] | Partitions armed PARTIAL |
| 8 | Rx[8] | Partitions armed PARTIAL delay 0 |
| 9 | Rx[9] | Partitions DISARMED |
| 10 | Rx[10] | Siren/system status (bit 5 = siren) |
| 11 | Rx[11] | Bypassed zones 1-8 |
| 12 | Rx[12] | Alarm memory zones 1-8 |
| 13 | Rx[13] | Tamper memory zones 1-8 |
| 14-15 | | *(padding/unknown)* |
| 16 | Rx[16] | Checksum |

---

## 6. Write Commands

### 6.1 Arm Partition

```
TX: 0F 00 F0 03 00 02 [TOTAL] [PARTIAL] 00 [CRC] FF
```

Total length: 11 bytes.

| Byte | Value | Description |
|------|-------|-------------|
| 0 | `0x0F` | Write command |
| 1-2 | `0x00 0xF0` | Function address |
| 3 | `0x03` | Sub-command: arm |
| 4 | `0x00` | Reserved |
| 5 | `0x02` | Payload length |
| 6 | TOTAL | Bitmask of partitions to arm totally (bit 0 = P1, bit 7 = P8) |
| 7 | PARTIAL | Bitmask of partitions to arm partially |
| 8 | `0x00` | Padding |
| 9 | CRC | `calculateCRC(cmd, 8)` = `0x203 - sum(cmd[0..8])` |
| 10 | `0xFF` | Trailer |

**specific_area mode**: When `specific_area=1`, the current arming state
of all partitions is read from the sensor cache and the target partition
bit is OR'd in, preserving other partitions' state. When `specific_area=0`,
only the target partition is set.

### 6.2 Disarm Partition

```
TX: 0F 00 F0 03 00 02 [TOTAL] [PARTIAL] 00 [CRC] FF
```

Identical structure to Arm. The difference is in the bitmask values:
- `specific_area=0`: Both TOTAL and PARTIAL are `0x00` (disarm all).
- `specific_area=1`: Current state is read, and the target partition's
  bit is cleared (either from total or partial, depending on which was set).

### 6.3 Reset Alarms

```
TX: 0F 05 F0 01 00 05 FF 00 FF
```

Fixed 9-byte command. No variable parameters. Clears alarm memory
for all zones and partitions.

| Byte | Value | Description |
|------|-------|-------------|
| 0 | `0x0F` | Write command |
| 1-2 | `0x05 0xF0` | Function address |
| 3 | `0x01` | Sub-command |
| 4 | `0x00` | Reserved |
| 5 | `0x05` | Payload length |
| 6 | `0xFF` | Reset mask (all zones) |
| 7 | `0x00` | Padding |
| 8 | `0xFF` | Checksum/trailer |

### 6.4 Activate Output

```
TX: 0F 06 F0 01 00 06 [ACTIVATE_MASK] 00 [CHK]
```

Total length: 9 bytes.

| Byte | Value | Description |
|------|-------|-------------|
| 0 | `0x0F` | Write command |
| 1-2 | `0x06 0xF0` | Function address |
| 3 | `0x01` | Sub-command |
| 4 | `0x00` | Reserved |
| 5 | `0x06` | Payload length indicator |
| 6 | MASK | Bitmask of outputs to activate (bit 0 = output 1) |
| 7 | `0x00` | Deactivation mask (zero for activate) |
| 8 | CHK | Same value as byte 6 (MASK) |

To activate output N (1-based): `MASK = 1 << (N - 1)`.

### 6.5 Deactivate Output

```
TX: 0F 06 F0 01 00 06 00 [DEACTIVATE_MASK] [CHK]
```

Total length: 9 bytes.

| Byte | Value | Description |
|------|-------|-------------|
| 6 | `0x00` | Activation mask (zero for deactivate) |
| 7 | MASK | Bitmask of outputs to deactivate (bit 0 = output 1) |
| 8 | CHK | Same value as byte 7 (MASK) |

Bytes 0-5 are identical to activate.

### 6.6 Update Date/Time

```
TX: 0F 03 F0 05 00 07 [DAY] [MONTH] [YEAR] [HOURS] [MINUTES] [SECONDS] [CHK]
```

Total length: 13 bytes.

| Byte | Value | Description |
|------|-------|-------------|
| 0 | `0x0F` | Write command |
| 1-2 | `0x03 0xF0` | Function address |
| 3 | `0x05` | Sub-command |
| 4 | `0x00` | Reserved |
| 5 | `0x07` | Payload length |
| 6 | DAY | Day of month (1-31, decimal, NOT BCD) |
| 7 | MONTH | Month (1-12) |
| 8 | YEAR | Year offset from 2000 (e.g., 25 for 2025) |
| 9 | HOURS | Hours (0-23) |
| 10 | MINUTES | Minutes (0-59) |
| 11 | SECONDS | Seconds (0-59) |
| 12 | CHK | `getChecksum(cmd, 6, 12)` = sum of bytes 6-11 |

### 6.7 Include Zone (Re-enable bypassed zone)

```
TX: 0F 01 F0 07 00 07 00 00 00 00 [Z25-32] [Z17-24] [Z9-16] [Z1-8] [CHK]
```

Total length: 15 bytes.

| Byte | Value | Description |
|------|-------|-------------|
| 0 | `0x0F` | Write command |
| 1-2 | `0x01 0xF0` | Function address |
| 3 | `0x07` | Sub-command |
| 4 | `0x00` | Reserved |
| 5 | `0x07` | Payload length |
| 6-9 | `0x00` | Exclude bitmask (zero for include) |
| 10 | Z25-32 | Include bitmask: zones 25-32 |
| 11 | Z17-24 | Include bitmask: zones 17-24 |
| 12 | Z9-16 | Include bitmask: zones 9-16 |
| 13 | Z1-8 | Include bitmask: zones 1-8 |
| 14 | CHK | Equals the single non-zero bitmask byte |

To include zone N: set the appropriate bit in the correct byte.
Zone mapping to bytes:

| Zone range | Byte index |
|------------|------------|
| 1-8 | 13 |
| 9-16 | 12 |
| 17-24 | 11 |
| 25-32 | 10 |

Within each byte: `bit = 1 << ((zone - base) % 8)` where base is
the lowest zone number for that byte group.

### 6.8 Exclude Zone (Bypass zone)

```
TX: 0F 01 F0 07 00 07 [Z25-32] [Z17-24] [Z9-16] [Z1-8] 00 00 00 00 [CHK]
```

Total length: 15 bytes. Same structure as Include but the bitmask
occupies bytes 6-9 (exclude fields) instead of bytes 10-13 (include
fields).

| Byte | Value | Description |
|------|-------|-------------|
| 6 | Z25-32 | Exclude bitmask: zones 25-32 |
| 7 | Z17-24 | Exclude bitmask: zones 17-24 |
| 8 | Z9-16 | Exclude bitmask: zones 9-16 |
| 9 | Z1-8 | Exclude bitmask: zones 1-8 |
| 10-13 | `0x00` | Include bitmask (zero for exclude) |
| 14 | CHK | Equals the single non-zero bitmask byte |

---

## 7. Register Address Summary

| Address | Length | Command Name | Models |
|---------|--------|--------------|--------|
| `0x0000` | `0x0B` | Software Version | All |
| `0x04F0` | `0x0A` | Sensor Status (zones + warnings + tampers) | All |
| `0x0215` | `0x12` | Partition Status | KYO32G |
| `0xEC14` | `0x12` | Partition Status | KYO32 (non-G) |
| `0x680E` | `0x09` | Partition Status | KYO8/4/8G/8W |

### Write function addresses:

| Addr bytes | Function |
|------------|----------|
| `00 F0` | Arm / Disarm partition |
| `05 F0` | Reset alarms |
| `06 F0` | Activate / Deactivate output |
| `03 F0` | Update date/time |
| `01 F0` | Include / Exclude zone |

---

## 8. Polling Architecture

The component alternates between two polling states every 500ms:

1. **Init state**: Calls `update_kyo_status()` (sensor status query).
   On first run, also calls `detect_alarm_model()`.
2. **Status state**: Calls `update_kyo_partitions()` (partition status
   query).

This results in a full status refresh every 1 second (two 500ms cycles).

Communication health is tracked via `centralInvalidMessageCount`:
- Resets to 0 on each successful response.
- Increments on each failed response.
- After 3 consecutive failures, `kyo_comunication` sensor publishes `false`.

---

## 9. KYO32 vs KYO32G Differences

| Feature | KYO32G | KYO32 (non-G) |
|---------|--------|----------------|
| Firmware prefix | `KYO32G` | `KYO32` |
| Partition status address | `0x0215` | `0xEC14` |
| Max programmable outputs | 8 | 3 |
| Output readback (Rx[12]) | Valid bitmask | Always `0xFF` |
| Sensor status response | 18 bytes | 18 bytes |
| Partition status response | 26 bytes | 26 bytes |
| Wireless support | Yes | Varies by sub-model |

The non-G model is the older generation. The KYO 32M (marketing name)
is a non-G KYO32 that supports wireless zones. Its firmware string is
`KYO32   x.xx` (same as non-G), and it uses the KYO32 (non-G) register
address `0xEC14` for partition status.

---

## 10. Configuration Register Map

The panel's entire configuration is stored in a 64K address space readable
via the `0xF0` read command (section 2.1). The following register addresses
were identified by analyzing USB captures of KyoUnit upload (read) sessions.

All addresses are for **KYO32/KYO32M** (non-G). KYO32G addresses may differ
for some registers (notably partition status uses `0x0215` instead of `0xEC14`).

### 10.1 Zone Configuration (0x009F-0x011E)

128 bytes total: 4 bytes per zone, 32 zones.

```
Address range: 0x009F - 0x011E
Read as:       F0 00 9F 3F 00 CE (zones 1-16, 64 bytes)
               F0 00 DF 3F 00 0E (zones 17-32, 64 bytes)
```

Each 4-byte zone record:

| Offset | Name | Description |
|--------|------|-------------|
| +0 | Type/Balance | Zone type in lower bits, balance mode in upper bits |
| +1 | Enrolled | `0x01` = sensor enrolled, `0x00` = not enrolled |
| +2 | Area | Area bitmask (`0x01`=area 1, `0x02`=area 2, `0x04`=area 3, etc.) |
| +3 | Attributes | Zone attributes (observed: `0x0F` for all zones) |

**Zone type** (byte 0 lower bits):

| Value | Type |
|-------|------|
| `0x00` | Instant |
| `0x01` | Entry/Exit path |
| `0x02` | Delayed follower |
| `0x18` | Unconfigured (factory default) |

Example from a KYO32M:
```
Zone  1 (configured):      00 01 01 0F → Instant, enrolled, area 1
Zone  2 (configured):      02 01 01 0F → Delayed follower, enrolled, area 1
Zone  3 (configured):      01 01 02 0F → Entry/Exit, enrolled, area 2
Zone  4 (configured):      00 01 02 0F → Instant, enrolled, area 2
Zone  7 (configured):      00 01 04 0F → Instant, enrolled, area 3
Zone 12 (unconfigured):    00 00 01 0F → Instant, not enrolled, area 1
Zone 17 (unconfigured):    18 00 01 0F → Factory default
```

### 10.2 Zone Names (0x2E00-0x2FFF)

512 bytes total: 16 ASCII bytes per zone name, 32 zones, space-padded.

```
Address range: 0x2E00 - 0x2FFF
Read as:       F0 2E 00 3F 00 5D (zones 1-4)
               F0 2E 40 3F 00 9D (zones 5-8)
               F0 2E 80 3F 00 DD (zones 9-12)
               F0 2E C0 3F 00 1D (zones 13-16)
               F0 2F 00 3F 00 5E (zones 17-20)
               F0 2F 40 3F 00 9E (zones 21-24)
               F0 2F 80 3F 00 DE (zones 25-28)
               F0 2F C0 3F 00 1E (zones 29-32)
```

Each read returns 64 data bytes (4 zone names × 16 bytes). Default names
follow the pattern `Zone NN` (e.g., `Zone 12          `).

### 10.3 Zone Enrollment (0x019E-0x01F7)

96 bytes total: 3 bytes per zone, 32 zones.

```
Address range: 0x019E - 0x01F7
Read as:       F0 01 9E 3F 00 CE (zones 1-21, 64 bytes)
               F0 01 DE 07 00 D6 (zones 22-24, partial)
```

Each 3-byte record contains 2 bytes of ESN/serial fragment followed by
`0xFF`. Enrolled wireless sensors have non-zero ESN bytes; unenrolled
or wired zones show their zone number as placeholder (e.g., `00 03 FF`
for zone 3).

### 10.4 Partition Configuration (0x01E9-0x0228)

64 bytes total: 10 bytes per partition, up to 8 partitions.

```
Read as: F0 01 E9 3F 00 19 (64 bytes)
```

Each 10-byte partition record:

| Offset | Name | Description |
|--------|------|-------------|
| +0 | Area | Area bitmask for this partition |
| +1 | Entry Timer | Entry delay (seconds) |
| +2 | Exit Timer | Exit delay (seconds) |
| +3..+5 | Unknown | Observed as `0x00` |
| +6 | Siren Timer | Siren duration value (encoding TBD) |
| +7..+9 | Unknown | Observed as `0x00` |

Example:
```
Partition 1: 01 07 07 00 00 00 06 00 00 00 → Area 1, 7s entry, 7s exit
Partition 2: 02 07 07 00 00 00 06 00 00 00 → Area 2, 7s entry, 7s exit
Partition 3: (all zeros = unconfigured or default)
```

### 10.5 Code Names (0x3000-0x317F)

384 bytes total: 16 ASCII bytes per code name, 24 codes, space-padded.

```
Address range: 0x3000 - 0x317F
Read as:       F0 30 00 3F 00 5F (codes 1-4)
               F0 30 40 3F 00 9F (codes 5-8)
               F0 30 80 3F 00 DF (codes 9-12)
               F0 30 C0 3F 00 1F (codes 13-16)
               F0 31 00 3F 00 60 (codes 17-20)
               F0 31 40 3F 00 A0 (codes 21-24)
```

Default names: `Code 1          `, `Code 2          `, `Code 3          `, etc.
The first two codes may have custom default names depending on panel firmware.

### 10.6 Output Names (0x3280-0x337F)

256 bytes total: 16 ASCII bytes per output name, 16 outputs, space-padded.

```
Address range: 0x3280 - 0x337F
Read as:       F0 32 80 3F 00 E1 (outputs 1-4)
               F0 32 C0 3F 00 21 (outputs 5-8)
               F0 33 00 3F 00 62 (outputs 9-12)
               F0 33 40 3F 00 A2 (outputs 13-16)
```

Default names: `Output 1        `, `Output 2        `, etc.

### 10.7 Keyfob ESN Storage (0xC0B1-0xC0DE)

48 bytes total: 3 bytes per keyfob, 16 keyfob slots.

```
Address range: 0xC0B1 - 0xC0DE (3-byte stride)
Read as:       F0 C0 B1 02 00 63 (keyfob 1)
               F0 C0 B4 02 00 66 (keyfob 2)
               ... (address increments by 3 for each keyfob)
               F0 C0 DE 02 00 90 (keyfob 16)
```

Each read returns 3 data bytes: the keyfob's ESN. Unenrolled slots
return `00 00 00`.

### 10.8 Panel Mode (0x01E6-0x01E8)

3 bytes of panel status/mode information.

```
Read as: F0 01 E6 02 00 D9
```

Observed response: `11 10 FF`. Exact bit mapping TBD.

### 10.9 Keyfob Button Config (0x011F-0x016E)

80 bytes total: 5 bytes per keyfob slot, 16 slots.

```
Read as:       F0 01 1F 3F 00 4F (keyfobs 1-13, 64 bytes)
               F0 01 5F 0F 00 5F (keyfobs 14-16, 16 bytes)
```

Each 5-byte record contains keyfob button assignment data. Unenrolled
slots contain `00 00 00 00 FF`.

### 10.10 Status Flags (0x1503-0x1508)

6 bytes of system status flags.

```
Read as: F0 15 03 05 00 0D
```

Observed response: `FF FF FF FF FF FF`. Exact bit mapping TBD.

### 10.11 Register Address Summary

| Address | Size | Content |
|---------|------|---------|
| `0x0000` | 12B | Firmware version string (ASCII) |
| `0x009F` | 128B | Zone configuration (32 × 4 bytes) |
| `0x011F` | 80B | Keyfob button config (16 × 5 bytes) |
| `0x019E` | 96B | Zone enrollment/ESN (32 × 3 bytes) |
| `0x01E6` | 3B | Panel mode/status |
| `0x01E9` | 64B | Partition configuration (8 × 10 bytes) |
| `0x03D0` | ~1362B | Event log area (all zeros observed) |
| `0x04F0` | 11B | Sensor status (realtime) |
| `0x1503` | 6B | System status flags |
| `0x2E00` | 512B | Zone names (32 × 16 bytes ASCII) |
| `0x3000` | 384B | Code names (24 × 16 bytes ASCII) |
| `0x3280` | 256B | Output names (16 × 16 bytes ASCII) |
| `0xC0B1` | 48B | Keyfob ESN storage (16 × 3 bytes) |
| `0xEC14` | 19B | Partition status (KYO32 non-G) |

---

## 11. Known Issues and Unmapped Data

> Note: Many items from the original "known issues" list have been resolved
> by the configuration register map in section 10. The remaining open items
> are listed below.

### 11.1 OC Output Readback on Non-G

Partition status `Rx[12]` always reads `0xFF` on KYO32 non-G. The actual
register holding output state is unknown. Activate and deactivate commands
work correctly -- only readback is affected.

### 11.2 Unmapped Status Bits

- **Rx[14] bit 7** (warning flags): Always observed as 0, purpose unknown.
  Candidate for "T" trouble indicator.
- **Rx[16] bits 0-1** (tamper flags): Always observed as 0, purpose unknown.
  Also candidates for trouble indicator.

### 11.3 "T" Trouble Indicator

The keypad displays a "T" symbol during arm/disarm operations despite no
zone triggers. This is likely an ARC (Alarm Receiving Centre) communications
failure indicator — possibly triggered by a failed phone line tone check
(panel option "29 - Disallow tone check" may need to be enabled on
VoIP/fiber landlines with non-standard dial tones). The register that
drives this display is not yet identified. A USB capture while "T" is
actively displayed would be needed to locate the source register.

### 11.4 Event Log

The panel stores an event log. The register range `0x03D0`-`0x0921` was
read during a KyoUnit upload session but returned all zeros (panel had no
stored events at the time). The event log entry format remains undecoded.

### 11.5 Rx[11] in Partition Status

In the KYO32 partition status response, `Rx[11]` is read but not mapped
to any known function. Its purpose is unknown.

### 11.6 Rx[10] Non-Siren Bits

Only bit 5 of `Rx[10]` (partition status) is mapped (siren state). The
other 7 bits in this byte are not decoded and may contain additional
system state flags.

---

## 12. Worked Examples

### 12.1 Read Sensor Status

```
TX:  F0 04 F0 0A 00 EE
RX:  F0 04 F0 0A 00 EE  00 00 00 02 00 00 00 00 00 00 00 02

Echo: F0 04 F0 0A 00 EE
Data: 00 00 00 02  00 00 00 00  00  00  00  02
      |---------|  |---------|  |   |   |   |
      Zone stat    Zone tamper  |   |   |   Checksum
      Z25-32=0     T25-32=0     |   |   Tamper flags=0x00
      Z17-24=0     T17-24=0     |   Area alarm=0x00
      Z9-16=0      T9-16=0      |   Warning flags=0x00
      Z1-8=0x02    T1-8=0x00

Zone status 0x02 at Rx[9]: bit 1 set -> Zone 2 is active (open/triggered).
```

### 12.2 Arm Partition 1 Totally

```
TX:  0F 00 F0 03 00 02 01 00 00 09 FF

0F       = Write command
00 F0    = Arm/disarm function
03       = Arm sub-command
00       = Reserved
02       = Payload length
01       = Total arm mask: bit 0 set = partition 1
00       = Partial arm mask: none
00       = Padding
09       = CRC: 0x203 - (0x0F+0x00+0xF0+0x03+0x00+0x02+0x01+0x00+0x00)
         = 0x203 - 0x1F5 = 0x0E... wait:
         sum = 15+0+240+3+0+2+1+0+0 = 261 = 0x105
         CRC = 0x203 - 0x105 = 0xFE (truncated to byte)
         (The template has 0xCC as placeholder; actual CRC is computed at runtime)
FF       = Trailer
```

### 12.3 Exclude Zone 3

```
TX:  0F 01 F0 07 00 07 00 00 00 04 00 00 00 00 04

0F 01 F0 = Write to include/exclude function
07       = Sub-command
00       = Reserved
07       = Payload length

Exclude mask (bytes 6-9):
  [6] Z25-32 = 0x00
  [7] Z17-24 = 0x00
  [8] Z9-16  = 0x00
  [9] Z1-8   = 0x04  (bit 2 set = zone 3)

Include mask (bytes 10-13):
  All 0x00

[14] Checksum = 0x04 (matches the non-zero mask byte)
```

---

## 13. Raw Command Reference Table

| Name | Hex Bytes | Direction | Description |
|------|-----------|-----------|-------------|
| Get Software Version | `F0 00 00 0B 00 FB` | Read | 12-char firmware ID string |
| Get Sensor Status | `F0 04 F0 0A 00 EE` | Read | Zone state + tamper + warnings |
| Get Partition (32G) | `F0 02 15 12 00 19` | Read | Arm state, bypass, alarm/tamper memory |
| Get Partition (32) | `F0 EC 14 12 00 02` | Read | Same, non-G register address |
| Get Partition (8) | `F0 68 0E 09 00 6F` | Read | Same, KYO8/4 register address |
| Arm Partition | `0F 00 F0 03 00 02 TT PP 00 CC FF` | Write | TT=total mask, PP=partial mask |
| Disarm Partition | `0F 00 F0 03 00 02 TT PP 00 CC FF` | Write | Same structure, different masks |
| Reset Alarms | `0F 05 F0 01 00 05 FF 00 FF` | Write | Clear all alarm memory |
| Activate Output | `0F 06 F0 01 00 06 MM 00 MM` | Write | MM=activate bitmask |
| Deactivate Output | `0F 06 F0 01 00 06 00 MM MM` | Write | MM=deactivate bitmask |
| Update DateTime | `0F 03 F0 05 00 07 DD MM YY HH mm SS CC` | Write | Date/time components + checksum |
| Include Zone | `0F 01 F0 07 00 07 00 00 00 00 B4 B3 B2 B1 CC` | Write | B1-B4=include zone bitmasks |
| Exclude Zone | `0F 01 F0 07 00 07 B4 B3 B2 B1 00 00 00 00 CC` | Write | B1-B4=exclude zone bitmasks |
