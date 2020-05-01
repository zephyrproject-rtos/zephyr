# SimpleLink BLE for Zephyr

This directory contains a semi-functional implementation of the Zephyr BLE Split-Stack API for the [CC1352R1 LaunchXL](http://www.ti.com/tool/LAUNCHXL-CC1352R1). 

## Build Instructions

### Prerequisites

* Ubuntu / Bionic (Linux)
* x86_64 (supported by Zephyr) / arm64 (works with a small effort)

### Getting Started with Zephyr 

Follow the instructions [here](https://docs.zephyrproject.org/latest/getting_started/index.html)
to get your build environment set up for Zephyr.

### Persistent Environment Variables

For a convenient and persistent way to store and modify environment variables
for each build, add the following to your `${HOME}/.zephyrrc` file.

```bash
export ZEPHYR_TOOLCHAIN_VARIANT=zephyr
export ZEPHYR_SDK_INSTALL_DIR=~/zephyr-sdk-0.11.2
export BOARD=cc1352r1_launchxl
export ZEPHYR_PROJECT=samples/bluetooth/peripheral_hr
```

### Check Out the simplelink-ble-for-zephyr Branch

Add the `cfriedt` remote and pull the sources

```bash
cd zephyrproject/zephyr
git remote add cfriedt https://github.com/cfriedt/zephyr.git
git fetch cfriedt && git checkout -b simplelink-ble-for-zephyr \
	cfriedt/simplelink-ble-for-zephyr
```

### Build the `peripheral_hr` demo

```bash
source zephyr-env.sh
west build $ZEPHYR_PROJECT
```

If you ever encounter errors in the build phase, you can always delete the
`build` directory or use `west build -p always`.

You may also browse project configuration using `ninja -C build menuconfig`.

### Open a terminal

```bash
minicom ttyACM0
```

### Flash the `peripheral_hr` demo to your CC1352R1 LaunchXL  

```bash
$ west flash
-- west flash: rebuilding
ninja: no work to do.
-- west flash: using runner openocd
-- runners.openocd: Flashing file: /home/cfriedt/workspace/zephyrproject/zephyr/build/zephyr/zephyr.hex
Open On-Chip Debugger 0.10.0+dev-00992-g3333261df-dirty (2019-08-16-00:14)
Licensed under GNU GPL v2
For bug reports, read
	http://openocd.org/doc/doxygen/bugs.html
adapter speed: 2500 kHz
srst_only separate srst_gates_jtag srst_open_drain connect_deassert_srst
adapter_nsrst_delay: 100
adapter speed: 1500 kHz
Info : XDS110: connected
Info : XDS110: firmware version = 2.3.0.18
Info : XDS110: hardware version = 0x0023
Info : XDS110: connected to target via JTAG
Info : XDS110: TCK set to 2500 kHz
Info : clock speed 1500 kHz
Info : JTAG tap: cc13x2.jrc tap/device found: 0x3bb4102f (mfg: 0x017 (Texas Instruments), part: 0xbb41, ver: 0x3)
Info : JTAG tap: cc13x2.cpu enabled
Info : cc13x2.cpu: hardware has 6 breakpoints, 4 watchpoints
Info : cc13x2.cpu: external reset detected
Info : Listening on port 3333 for gdb connections
    TargetName         Type       Endian TapName            State       
--  ------------------ ---------- ------ ------------------ ------------
 0* cc13x2.cpu         cortex_m   little cc13x2.cpu         running
Info : JTAG tap: cc13x2.jrc tap/device found: 0x3bb4102f (mfg: 0x017 (Texas Instruments), part: 0xbb41, ver: 0x3)
Info : JTAG tap: cc13x2.cpu enabled
Error: Debug regions are unpowered, an unexpected reset might have happened
Error: JTAG-DP STICKY ERROR
Error: Could not find MEM-AP to control the core
target halted due to debug-request, current mode: Thread 
xPSR: 0x61000000 pc: 0x1000118e msp: 0x11001ff0
auto erase enabled
wrote 360448 bytes from file /home/cfriedt/workspace/zephyrproject/zephyr/build/zephyr/zephyr.hex in 8.469719s (41.560 KiB/s)
Info : JTAG tap: cc13x2.jrc tap/device found: 0x3bb4102f (mfg: 0x017 (Texas Instruments), part: 0xbb41, ver: 0x3)
Info : JTAG tap: cc13x2.cpu enabled
Error: Debug regions are unpowered, an unexpected reset might have happened
Error: JTAG-DP STICKY ERROR
Error: Could not find MEM-AP to control the core
target halted due to debug-request, current mode: Thread 
xPSR: 0x61000000 pc: 0x1000118e msp: 0x11001ff0
verified 360448 bytes in 6.180702s (56.951 KiB/s)
Info : JTAG tap: cc13x2.jrc tap/device found: 0x3bb4102f (mfg: 0x017 (Texas Instruments), part: 0xbb41, ver: 0x3)
Info : JTAG tap: cc13x2.cpu enabled
Error: Debug regions are unpowered, an unexpected reset might have happened
Error: JTAG-DP STICKY ERROR
Error: Could not find MEM-AP to control the core
shutdown command invoked
Info : XDS110: disconnected
```

After flashing is complete, you should see the following terminal output in minicom:

```
*** Booting Zephyr OS build zephyr-v2.2.0-2072-g4672b8807750  ***
Bluetooth initialized
Advertising successfully started
[00:00:00.006,591] <dbg> bt_ctlr_hal_ti_radio.ble_cc13xx_cc26xx_data_init: device address: cd:de:1e:b0:6f:c0
[00:00:00.008,300] <dbg> bt_ctlr_llsw_ti_lll.swi_lll_cc13xx_cc26xx_isr: IRQ 1
[00:00:00.008,392] <dbg> bt_ctlr_llsw_ti_lll.swi_lll_cc13xx_cc26xx_isr: IRQ 1
[00:00:00.011,627] <inf> bt_hci_core: HW Platform: Texas Instruments (0x0004)
[00:00:00.011,657] <inf> bt_hci_core: HW Variant: cc13xx_cc26xx (0x0001)
[00:00:00.011,688] <inf> bt_hci_core: Firmware: Standard Bluetooth controller (0x00) Version 2.2 Build 99
[00:00:00.014,007] <inf> bt_hci_core: Identity: c0:6f:b0:1e:de:cd (random)
[00:00:00.014,068] <inf> bt_hci_core: HCI: version 5.1 (0x0a) revision 0x0000, manufacturer 0x05f1
[00:00:00.014,068] <inf> bt_hci_core: LMP: version 5.1 (0x0a) subver 0xffff
```

Open up a BLE-compatible device and attempt to pair to the CC1352R1 LaunchXL.
You should see `Zephyr Heartrate Sensor`.

<img src="https://raw.githubusercontent.com/cfriedt/zephyr/simplelink-ble-for-zephyr/subsys/bluetooth/controller/ll_sw/ti/pairing.png" width="200">

Once you attempt to pair with the device, there will be [a lot of diagnostic
logs](https://github.com/cfriedt/zephyr/raw/simplelink-ble-for-zephyr/subsys/bluetooth/controller/ll_sw/ti/ti.txt) output to the terminal.

## Debugging with an Oscilloscope

If you have an oscilloscope handy, you can use pins 30, 5, 23, and 24 to
observe TX, RX, ISR, and BLE TX Window events, respectively.

The screenshot below captured some BLE advertisement packets being exchanged.

<img src="https://raw.githubusercontent.com/cfriedt/zephyr/simplelink-ble-for-zephyr/subsys/bluetooth/controller/ll_sw/ti/rx-tx-gpio.png" width="600">

## Current State of the Port

The PR has been open for a long time and is updated periodically.

https://github.com/zephyrproject-rtos/zephyr/pull/21631

Simple advertisement is working. CONNECT_IND is received but there seems to be a timeout for setting up the initial TX Window.

There is also another branch with a brief attempt to use several independent
radio event handlers instead of a single ISR. However, that branch is not currently
rebased on Zephyr's master. 

https://github.com/cfriedt/zephyr/tree/and-now-for-something-completely-different
 
