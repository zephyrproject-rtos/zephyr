# Ethernet Sample

This sample demonstrates Ethernet networking on the LPC54S018 using the built-in
ENET controller with LAN8720A PHY.

## Overview

The sample configures the Ethernet interface and runs a TCP echo server on port 7.
It demonstrates:
- Ethernet controller initialization
- PHY power control
- Static IP configuration
- TCP server implementation
- Network status monitoring

## Hardware Requirements

- LPCXpresso54S018M board
- Ethernet cable
- LAN8720A PHY (on board)

## Pin Configuration

The sample uses RMII (Reduced Media Independent Interface) with these pins:
- P1.16: ENET_MDC (Management Data Clock)
- P1.23: ENET_MDIO (Management Data I/O)
- P4.8: ENET_TXD0 (Transmit Data 0)
- P4.9: ENET_TXD1 (Transmit Data 1)
- P4.10: ENET_RX_DV (Receive Data Valid)
- P4.11: ENET_RXD0 (Receive Data 0)
- P4.12: ENET_RXD1 (Receive Data 1)
- P4.13: ENET_TX_EN (Transmit Enable)
- P4.14: ENET_RX_CLK (Receive Clock)
- P3.3: PHY_POWER (PHY Power Control)

## Building and Running

### Build the sample:
```bash
# From project root
west build -b lpcxpresso54s018 samples/drivers/ethernet -p auto
```

### Flash to device:
```bash
west flash
```

## Configuration

### Static IP (default):
- Device IP: 192.168.1.100
- Subnet Mask: 255.255.255.0
- Gateway: 192.168.1.1

### To enable DHCP:
Edit `prj.conf` and change:
```
CONFIG_NET_DHCPV4=y
```

## Testing

1. Connect Ethernet cable to the board
2. Wait for LED to turn on (indicates network is ready)
3. Ping the device:
   ```bash
   ping 192.168.1.100
   ```
4. Test echo server:
   ```bash
   telnet 192.168.1.100 7
   # Type any text - it will be echoed back
   ```

## Network Shell Commands

The sample includes network shell support. Connect to UART console and use:
- `net iface` - Show network interfaces
- `net ping <ip>` - Ping a host
- `net stats` - Show network statistics
- `net arp` - Show ARP table

## LED Status

- Off: Network not configured
- On: Network interface is up and has IP address

## Troubleshooting

1. **No link**: Check Ethernet cable and PHY power
2. **Cannot ping**: Verify IP configuration matches your network
3. **PHY not detected**: Check PHY power control (GPIO3_3)
4. **No console output**: Connect to UART at 115200 baud

## Notes

- The LAN8720A PHY operates at 10/100 Mbps
- RMII interface requires 50MHz reference clock
- PHY address is 0 (configured by hardware)
- The Ethernet driver uses Zephyr's built-in NXP ENET support