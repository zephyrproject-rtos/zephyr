# USB CDC ECM Network Sample

This sample demonstrates USB networking on the LPC54S018 using Zephyr's built-in
CDC ECM (Ethernet Control Model) support.

## Overview

The sample configures the LPC54S018 as a USB Ethernet adapter. When connected to
a PC, it appears as a network interface. The device runs a TCP echo server on
port 13400 (VCI port).

## Features

- USB CDC ECM device (standard USB Ethernet)
- Static IP configuration (192.168.7.1)
- TCP echo server on port 13400
- LED status indication
- Automatic USB enumeration by Zephyr

## Building and Running

### Build the sample:
```bash
# From project root
west build -b lpcxpresso54s018 samples/drivers/usb_net -p auto
```

### Flash to device:
```bash
west flash
```

## Host Configuration

### Linux
The device will appear as a USB Ethernet interface (typically `usb0` or `enx*`):
```bash
# Configure host IP
sudo ip addr add 192.168.7.2/24 dev usb0
sudo ip link set usb0 up

# Test connection
ping 192.168.7.1

# Connect to echo server
telnet 192.168.7.1 13400
```

### Windows
1. The device will appear as "USB Ethernet/RNDIS Gadget" in Device Manager
2. Configure the network adapter:
   - IP Address: 192.168.7.2
   - Subnet Mask: 255.255.255.0
   - Default Gateway: (leave empty)

### macOS
Similar to Linux, the device appears as a network interface:
```bash
# Configure IP
sudo ifconfig en<X> 192.168.7.2 netmask 255.255.255.0

# Test connection
ping 192.168.7.1
```

## Testing

1. Connect USB cable to PC
2. Wait for LED to turn on (indicates network is ready)
3. Ping the device: `ping 192.168.7.1`
4. Connect to echo server: `telnet 192.168.7.1 13400`
5. Type any text - it will be echoed back

## LED Status

- Blinking slowly: USB not configured
- Solid on: Network interface is up and ready

## Configuration Options

Key configuration options in `prj.conf`:
- `CONFIG_USB_DEVICE_NETWORK_ECM=y` - Enable CDC ECM
- `CONFIG_NET_CONFIG_MY_IPV4_ADDR` - Device IP address
- `VCI_PORT` - TCP server port (defined in main.c)

## Troubleshooting

1. **Device not recognized**: Check USB cable and drivers
2. **Cannot ping**: Verify host IP configuration
3. **LED not turning on**: Check if network interface is up
4. **No serial output**: Connect to UART console at 115200 baud

## Notes

- Zephyr handles all USB enumeration and descriptor management automatically
- The USB stack creates a virtual Ethernet interface that integrates with Zephyr's network stack
- No manual USB configuration is required - just enable USB and select the network type