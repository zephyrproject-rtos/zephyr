.. zephyr:board:: shrike_lite

Overview
********

`Shrike-lite`_ is a low-cost, open-source microcontroller + FPGA development board
combining an `RP2040`_ MCU and a 1120 LUT FPGA. It features a PMOD connector,
breadboard-compatible layout, integrated FPGA–MCU IO interface, QSPI flash,
dual user LEDs, and USB Type-C for power/programming.
Ideal for learners, hobbyists, and hardware/software co-design prototyping.


Hardware
********

- Microcontroller `RP2040`_, with a max frequency of 133 MHz
- Dual ARM Cortex M0+ cores
- 264 kByte SRAM
- 4 Mbyte QSPI flash
- GPIO
- ADC
- I2C
- SPI
- UART
- USB Type-C connector
- Dual user LEDs
- Debug Connector
- Reset and boot buttons
- 1120 LUT SLG47910 Low-Power FPGA
- Blue LED


Supported Features
==================

.. zephyr:board-supported-hw::


Programming
***********

The ``shrike_lite`` board is supported by the following runners:

.. zephyr:board-supported-runners::

There are multiple ways to program and debug the `Shrike-lite`_ board.
All methods are standard to the way of programming and debugging for the `Raspberry Pi Pico`_.

The supported format for pico series (UF2) is located at :file:`build/zephyr/zephyr.uf2` file.

To build the application using USB Type-C connector, you can use the following command:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: shrike_lite
   :goals: build

To build the application using `Raspberry Pi Debug Probe`_, you can use the following command:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: shrike_lite
   :goals: build
   :gen-args: -DRPI_PICO_DEBUG_ADAPTER=cmsis-dap


Flashing the board
==================

To flash the board, you can use the following methods:

- USB Type-C connector (default)

Press and hold the boot button while plugging in the USB Type-C connector
to enter the bootloader mode and show the mass storage volume named "RP1-RP2" in the file manager.

You can either:

Drag and drop the :file:`build/zephyr/zephyr.uf2` file to the mass storage volume to program the board.
You can also use the following command to program the board:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: shrike_lite
   :goals: flash
   :flash-args: --runner uf2

Or use the following command to program the board to try another runner:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: shrike_lite
   :goals: flash
   :flash-args: --openocd /usr/local/bin/openocd


Debugging the board
===================

To debug the board,

Press and hold the boot button while the `Raspberry Pi Debug Probe`_ is connected
to enter the debug mode and show the mass storage volume named "RP1-RP2" in the file manager.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: shrike_lite
   :goals: debug
   :debug-args: --openocd /usr/local/bin/openocd

.. target-notes::

.. _Shrike-lite:
   https://vicharak-in.github.io/shrike/introduction.html

.. _Shrike-lite Source Repository:
   https://github.com/vicharak-in/shrike

.. _RP2040:
   https://datasheets.raspberrypi.com/rp2040/rp2040-datasheet.pdf

.. _Raspberry Pi Debug Probe:
   https://www.raspberrypi.com/documentation/microcontrollers/debug-probe.html

.. _Raspberry Pi Pico:
   https://www.raspberrypi.com/products/raspberry-pi-pico/
