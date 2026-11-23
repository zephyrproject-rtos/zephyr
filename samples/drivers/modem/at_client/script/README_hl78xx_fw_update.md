# HL78xx Modem Firmware Update Script

## Overview
Python script for updating Sierra Wireless HL78xx cellular modem firmware via XMODEM-1K protocol.

## Requirements
- Python 3.6 or later
- pyserial: `pip install pyserial`
- xmodem: `pip install xmodem`
- Direct serial connection to modem UART

## Supported Modems
- Sierra Wireless HL7800
- Sierra Wireless HL7802
- Sierra Wireless HL7810
- Other HL78xx variants

## Usage

### Direct Connection to Modem
When connected directly to the modem UART:

```bash
python hl78xx_firmware_update_direct.py -p COM5 -f firmware.ua
```

### With Zephyr AT Client Bridge
When using the Zephyr AT client sample as a bridge:

1. Flash the `at_client` sample to your board
2. Use the bridge version of the script (if available) or connect through the console port

### Command Line Options
```
-p, --port PORT       Serial port (e.g., /dev/ttyUSB0, COM5) [required]
-f, --firmware FILE   Firmware file path (.ua file) [required]
-b, --baudrate BAUD   Baud rate (default: 115200)
-t, --timeout SEC     Update timeout in seconds (default: 600)
-v, --verbose         Enable verbose logging
```

## Examples

### Linux
```bash
# Basic update
python hl78xx_firmware_update_direct.py -p /dev/ttyUSB0 -f HL7800.4.6.7.0.ua

# With verbose logging
python hl78xx_firmware_update_direct.py -p /dev/ttyUSB0 -f firmware.ua -v

# Custom timeout for slow connections
python hl78xx_firmware_update_direct.py -p /dev/ttyUSB0 -f firmware.ua -t 900
```

### Windows
```bash
# Basic update
python hl78xx_firmware_update_direct.py -p COM5 -f HL7800.4.6.7.0.ua

# With verbose logging
python hl78xx_firmware_update_direct.py -p COM5 -f firmware.ua -v
```

## Update Sequence
1. **Connect** - Opens serial port to modem
2. **Enable Indications** - Sends `AT+WDSI=?` to query supported ranges, then enables maximum value
3. **Initiate Download** - Sends `AT+WDSD=<filesize>`
4. **XMODEM Transfer** - Transfers firmware file using XMODEM-1K protocol with real-time progress bar
5. **Verify Download** - Waits for `OK` and `+WDSI: 3` confirmation
6. **Start Installation** - Sends `AT+WDSR=4`
7. **Monitor Installation** - Waits for `+WDSI: 14` (installation starting)
8. **Wait for Result** - Monitors for `+WDSI: 16` (success) or `+WDSI: 15` (failure)

## WDSI Indications
The script monitors these AT command indications:

- `+WDSI: 3` - Firmware package downloaded successfully
- `+WDSI: 14` - Installation starting (modem rebooting)
- `+WDSI: 15` - Installation failed
- `+WDSI: 16` - Installation successful

**Note:** The modem sends these indications with a space after the colon (e.g., `+WDSI: 16`)

## Firmware Files
- Extension: `.ua` (Update Archive)
- Type: Delta updates (upgrade from current version to target version)
- Source: Contact Sierra Wireless or your distributor
- Size: Typically 500KB - 5MB depending on update scope

## Troubleshooting

### "Port already in use"
- Close any terminal applications connected to the modem
- Check if ModemManager is running on Linux: `sudo systemctl stop ModemManager`

### "Timeout waiting for XMODEM ready signal"
- Verify modem is powered on and responding
- Check baud rate matches modem configuration
- Try sending `AT` command manually to verify connectivity
- Use `-v` flag for verbose output to see what modem is sending

### "XMODEM transfer failed"
- Verify firmware file is not corrupted
- Check serial connection stability
- Increase timeout with `-t 900`
- Some modems use checksum mode instead of CRC - script handles both

### "Installation failed (+WDSI:15)"
- Firmware file may be incompatible with current modem version
- Insufficient flash space on modem
- Verify you have the correct firmware file for your modem model

## Technical Details

### Progress Display
During firmware transfer, the script displays a real-time progress bar:
```
Progress: [████████████░░░░░░░░░░░░░░░░░░] 40% (229,376/570,988 bytes) | 5.2 KB/s | ETA: 65s
```
- Visual progress bar (30 characters wide)
- Percentage completed
- Bytes transferred / total bytes
- Current transfer rate (KB/s)
- Estimated time to completion (ETA)
- Updates every 1% on the same line

### XMODEM Protocol
- Uses XMODEM-1K with 1024-byte packets
- Supports both CRC and checksum modes
- Automatically detects modem's preferred mode based on ready signal

### Serial Configuration
- Baud rate: 115200 (default, configurable)
- Data bits: 8
- Parity: None
- Stop bits: 1
- Flow control: None

### Timing
- Download time: Varies by file size (~2-10 minutes typical)
  - Progress bar shows: percentage, bytes transferred, transfer rate, and ETA
  - Updates every 1% for real-time feedback
- Installation time: 5-15 minutes
- Total update time: 10-30 minutes typical

## Integration with Zephyr

This script is designed to work with the Zephyr RTOS `at_client` sample:

1. Build and flash the AT client sample to your board
2. The sample creates a transparent UART bridge
3. Run this script connecting to the console UART
4. AT commands and XMODEM data flow through the bridge to the modem

See `samples/drivers/modem/at_client/` for the bridge application.

## Error Codes
- Exit 0: Update successful
- Exit 1: Update failed or error occurred

## License
SPDX-License-Identifier: Apache-2.0
Copyright (c) 2025 Netfeasa Ltd.

## Support
For issues or questions:
- Modem-specific questions: Contact Sierra Wireless support
- Script issues: Open issue in Zephyr project
- Zephyr integration: See AT client sample documentation
