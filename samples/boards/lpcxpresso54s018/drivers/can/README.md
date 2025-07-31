# LPC54S018 CAN Sample

This sample demonstrates the CAN (Controller Area Network) functionality on the LPC54S018.

## Overview

The LPC54S018 has two dedicated MCAN (CAN with Flexible Data-rate) controllers:
- CAN0: Base address 0x4009D000
- CAN1: Base address 0x4009E000

Both controllers support:
- CAN 2.0B (Classic CAN)
- CAN FD (CAN with Flexible Data-rate)
- Up to 1 Mbps in classic mode
- Up to 8 Mbps in FD mode

## Hardware

### Pin Configuration

**CAN0:**
- TX: P1.2 (pin L14)
- RX: P1.3 (pin J13)

**CAN1:**
- TX: P1.17 (pin N12)
- RX: P1.18 (pin D1)

### CAN Transceiver

You need an external CAN transceiver (e.g., TJA1050, MCP2551) connected to the CAN pins.

## Building and Running

```bash
west build -b lpcxpresso54s018 samples/drivers/can
west flash
```

## Sample Output

```
=== LPC54S018 CAN Test ===
CAN device ready
CAN configured for 500 kbps
CAN State: 0, TX errors: 0, RX errors: 0
Starting CAN test loop...
Sent CAN frame ID: 0x123, DLC: 8
```

## Configuration

The sample configures CAN for:
- 500 kbps bitrate
- Normal mode (not loopback)
- Standard 11-bit identifiers

To enable CAN FD mode, uncomment `CONFIG_CAN_FD_MODE=y` in prj.conf.

## Testing

1. Connect a CAN transceiver to the CAN0 pins
2. Connect the CAN bus to another CAN node
3. The sample will send a test frame every 2 seconds
4. Any received frames matching ID 0x123 will be displayed

Private Porting , by David Hor - Xtooltech 2025, david.hor@xtooltech.com