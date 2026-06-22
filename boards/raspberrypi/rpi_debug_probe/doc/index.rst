.. zephyr:board:: rpi_debug_probe

Overview
********

The `Raspberry Pi Debug Probe`_ is a board based on the RP2040
microcontroller from Raspberry Pi Ltd. The board exposes four GPIO pins
via two 3-pin connectors, and the board has a USB micro B connector.

.. note::

    This is for using the Raspberry Pi Debug Probe board as a general-purpose
    microcontroller board, not using it as a tool for debugging other boards.
    See below for how to load the board with the official firmware, which is
    needed if you would use this board for its original debugging purpose.


Hardware
********

- Microcontroller Raspberry Pi RP2040, with a max frequency of 133 MHz
- Dual ARM Cortex M0+ cores
- 264 kByte SRAM
- 2 Mbyte QSPI flash
- 6 GPIO pins, of which 4 are exposed via 3-pin connectors
- UART
- USB micro B connector
- Boot button
- 5 user LEDs


Default Zephyr Peripheral Mapping
=================================

.. rst-class:: rst-columns

    - LED D1 red : GPIO2
    - LED D2 green (close to UART connector) : GPIO7
    - LED D3 yellow (close to UART connector) : GPIO8
    - LED D4 green (close to DBUG connector) : GPIO15
    - LED D5 yellow (close to DBUG connector) : GPIO16
    - Connector J2 (UART) pin 1 (TX) : GPIO4 UART1
    - Connector J2 (UART) pin 3 (RX) : GPIO6
    - Connector J2 (UART) pin 3 (RX) via input buffer : GPIO5 UART1
    - Connector J3 (DBUG) pin 1 (CLK) : GPIO12
    - Connector J3 (DBUG) pin 3 (DIO) : GPIO14
    - Connector J3 (DBUG) pin 3 (DIO) via input buffer : GPIO13
    - Connector J4 pin 1 : GPIO0
    - Connector J4 pin 3 : GPIO1

The pins in the "UART" and "DBUG" connectors are using 100 Ohm
series resistors.

The connector J4 is not populated by default, so you need to solder
a 3-pin header to the board in order to use that connector.

See also `schematic`_.


Supported Features
==================

.. zephyr:board-supported-hw::


Programming and Debugging
*************************

.. zephyr:board-supported-runners::

By default programming is done via the USB connector.
Press and hold the BOOTSEL button when connecting the board to your
computer. It will appear as a USB mass-storage device named "RPI-RP2".
Building your application will result in a :file:`build/zephyr/zephyr.uf2` file.
Drag and drop the file to the USB mass storage unit, and the board
will be reprogrammed.

It is also possible to program and debug the board via the SWDIO and SWCLK pads,
exposed as testpads on the back of the board. You need to solder connectors to the pads.
Then a separate programming hardware tool is required, and
for example the :command:`openocd` software is used. Typically the
``OPENOCD`` and ``OPENOCD_DEFAULT_PATH``
values should be set when building, and the ``--runner openocd``
argument should be used when flashing.
For more details on programming RP2040-based boards, see
:ref:`rpi_pico_programming_and_debugging`.

If you would like to restore the official firmware on the Debug Probe,
download the `latest firmware`_.


Flashing
========

To run the :zephyr:code-sample:`blinky` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky/
   :board: rpi_debug_probe
   :goals: build flash

Try also the :zephyr:code-sample:`hello_world` and
:zephyr:code-sample:`usb-cdc-acm-console` samples.


References
**********

.. target-notes::

.. _Raspberry Pi Debug Probe:
    https://www.raspberrypi.com/documentation/microcontrollers/debug-probe.html

.. _latest firmware:
    https://www.raspberrypi.com/documentation/microcontrollers/debug-probe.html#updating-the-firmware-on-the-debug-probe

.. _schematic:
    https://datasheets.raspberrypi.com/debug/raspberry-pi-debug-probe-schematics.pdf
