# USB Printer Sample

This sample implements a USB printer class device compliant with the IEEE 1284 specification for bi-directional parallel printer protocol. The implementation supports PCL and PJL printer languages and provides a job queue management system.

## Overview

The USB printer sample demonstrates:
- USB Printer Class implementation
- PCL and PJL command processing
- Print job queue management
- Error handling and status reporting
- Bi-directional communication

## Requirements

- Supported development board (e.g., nRF52840 DK)
- USB cable
- Host computer (Windows, Linux, or macOS)
- Zephyr SDK

## Building and Running

1. Build the sample:
   ```
   west build -b nrf52840dk_nrf52840 samples/subsys/usb/printer
   ```

2. Flash to your board:
   ```
   west flash
   ```

3. Connect the board via USB to your host computer

## Features

### USB Printer Class
- Standard printer class descriptors
- GET_DEVICE_ID implementation
- GET_PORT_STATUS support
- SOFT_RESET command handling

### Print Job Management
- Job queue with FIFO processing
- Job status tracking
- Error recovery mechanisms
- Job cancellation support

### Printer Languages
- PCL (Printer Command Language)
  - Basic text printing
  - Font selection
  - Page orientation
  - Line spacing
  
- PJL (Printer Job Language)
  - Job control
  - Printer status
  - Device configuration

### Error Handling
- Paper out detection
- Error status reporting
- Automatic recovery
- Job state preservation

## Configuration

The sample can be configured through Kconfig options:

- `CONFIG_USB_PRINTER_JOBQUEUE_SIZE`: Maximum number of jobs in queue
- `CONFIG_USB_PRINTER_BUFFER_SIZE`: Size of print data buffer
- `CONFIG_USB_PRINTER_PCL_SUPPORT`: Enable PCL support
- `CONFIG_USB_PRINTER_PJL_SUPPORT`: Enable PJL support

## Testing

Refer to [TESTING.md](TESTING.md) for detailed testing procedures and automated test script usage.

### Quick Test

1. Install Python dependencies:
   ```
   pip install -r requirements.txt
   ```

2. Run automated tests:
   ```
   python test_printer.py --all
   ```

## Debugging

Debug messages can be enabled with:
- `CONFIG_USB_PRINTER_LOG_LEVEL_DBG`: Enable debug logging
- `CONFIG_USB_DEVICE_LOG_LEVEL_DBG`: Enable USB stack debug logging

## Known Limitations

1. Limited PCL command set support
2. Basic PJL implementation
3. Fixed buffer sizes for print data
4. Single interface configuration

## Contributing

Please read the [Zephyr Contribution Guide](../../../CONTRIBUTING.rst) for details on the contribution process.

## License

This sample is licensed under the same terms as the Zephyr RTOS. See [LICENSE](../../../LICENSE) for details.