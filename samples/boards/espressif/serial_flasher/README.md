# ESP32 Zephyr Example

## Overview

This sample code demonstrates how to flash an Espressif SoC (target) from another MCU (host) using
`esp_serial_flasher`.
Binaries to be flashed from the host MCU to another Espressif SoC can be found in [binaries](../binaries/) folder
and are converted into C-array during build process.

## How the Example Works

The example performs the following steps to flash the target device:

1. Initialization: Sets up the required peripherals (GPIO for BOOT and EN pins, and UART)
2. Target Reset: Puts the target into bootloader mode using the BOOT and EN pins
3. Connection: Establishes communication with the target bootloader using esp_loader_connect()
4. Baud Rate Change: Increases the transmission speed for faster flashing (except for ESP8266)
5. Flashing Process:
   - Calls esp_loader_flash_start() to begin flashing and erase the required memory
   - Repeatedly calls esp_loader_flash_write() to transfer the binary
   - Verifies the flash integrity if MD5 checking is enabled

> [!NOTE]
> In addition, to steps mentioned above, `esp_loader_change_transmission_rate()` is called after connection is established in order to increase the flashing speed. This does not apply for ESP8266, as its bootloader does not support this command. However, ESP8266 is capable of detecting the baud rate during connection phase and can be changed before calling `esp_loader_connect()`, if necessary.

## Hardware Required

- ESP32-DevKitC board or another compatible Espressif development board as a host
- Any supported Espressif SoC development board as a target
- One or two USB cables for power supply and programming
- Jumper cables to connect the host to the target according to the table below.

## Hardware Connection

This example uses the **UART interface**. For detailed interface information and general hardware considerations, see the [Hardware Connections Guide](../../docs/hardware-connections.md#uartserial-interface).

**ESP32-to-Espressif SoC Pin Assignment:**

| ESP32 (host) | Espressif SoC (target) |
| :----------: | :--------------------: |
|     IO26     |          BOOT          |
|     IO25     |         RESET          |
|     IO4      |          RX0           |
|     IO5      |          TX0           |

> [!NOTE]
> Pin assignments can be modified in the device tree overlay.

## Prerequisites

Before building the example, you need to set up the Zephyr development environment as stated in [Zephyr's Getting Started Guide](https://docs.zephyrproject.org/latest/develop/getting_started/index.html). After this step, you can proceed to building and flashing the example.

## Prepare Target Firmware

Place the required target firmware binaries in the `target-firmware/` directory. You can use your own binaries, build them from the esp-idf examples, or build them from the source in the `test/target-example-src` directory.

**Required binaries:**

- `bootloader.bin` - ESP bootloader binary
- `partition-table.bin` - Partition table configuration
- `app.bin` - Main application binary

To build from the example source:

```bash
cd test/target-example-src/hello-world-ESP32-src
idf.py set-target esp32s3  # Set target chip (esp32, esp32s3, esp32c3, etc.)
idf.py fullclean
idf.py -D SDKCONFIG_DEFAULTS=sdkconfig.defaults.flash reconfigure build

# Copy binaries to example
cp build/bootloader/bootloader.bin ../../zephyr_example/target-firmware/
cp build/partition_table/partition-table.bin ../../zephyr_example/target-firmware/
cp build/hello_world.bin ../../zephyr_example/target-firmware/app.bin
```

## Build and Flash

To run the example, type the following commands:

```bash
# Initialize west workspace if not done already
west init

# Update Zephyr modules
west update

# Build the example with ESP-Serial-Flasher module
west build -p -b <supported board from the boards folder> path/to/esp-serial-flasher/examples/zephyr_example -D ZEPHYR_EXTRA_MODULES=/path/to/esp-serial-flasher

west flash
west espressif monitor
```

> [!NOTE]
> Instead of using the `-D ZEPHYR_EXTRA_MODULES` parameter, you can add the esp-serial-flasher module to your `west.yml` file.

## Configuration

For details about available configuration option, please refer to top level [README.md](../../README.md).
Compile definitions can be specified in `prj.conf` file.

Binaries to be flashed are placed in a separate folder (binaries.c) for each possible target and converted to C-array. Without explicitly enabling MD5 check, flash integrity verification is disabled by default.

## Example Output

Here is the example's console output:

```text
Running ESP Flasher from Zephyr
Connected to target
Transmission rate changed.
Erasing flash (this may take a while)...
Start programming
Progress: 100 %
Finished programming
Flash verified
Erasing flash (this may take a while)...
Start programming
Progress: 100 %
Finished programming
Flash verified
Erasing flash (this may take a while)...
Start programming
Progress: 100 %
Finished programming
Flash verified
********************************************
*** Logs below are print from slave .... ***
********************************************
Hello world!
```
