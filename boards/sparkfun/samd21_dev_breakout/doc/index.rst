.. zephyr:board:: sparkfun_samd21_breakout

Overview
********

The SparkFun SAMD21 dev breakout board is an arduino form factor development board.
Designed to be a more powerful arduino. It shares the arduino form factor and gpio
footprint. Based on the ATSAMD21G18 MCU, it runs a 32-Bit Arm Cortex-M0+ at up to 48MHz.
More information can be found on the `SAMD21 dev breakout specification website`_.

Hardware
********

- ATSAMD21G18 32-bit/48MHz ARM Cortex-M0+
- 256KB Flash Memory
- 32KB SRAM
- 32KB of EEPROM (emulated in Flash)
- 30 GPIO Count with Arduino R3 footprint

- 6x Configurable SERCOM ports

   - USART with full-duplex and single-wire half-duplex configuration
   - I2C up to 3.4 MHz
   - SPI
   - LIN client

- One 12-bit, 350ksps Analog-to-Digital Converter (ADC) with up to 20 channels

   - Differential and single-ended input
   - 1/2x to 16x programmable gain stage
   - Automatic offset and gain error compensation
   - Oversampling and decimation in hardware to support 13-bit, 14-bit, 15-bit, or 16-bit resolution

- One two-channel Inter-IC Sound (I2S) interface
- 32-bit Real Time Counter (RTC) with clock/calendar function
- Watchdog Timer (WDT)
- CRC-32 generator

- One full-speed (12 Mbps) Universal Serial Bus (USB) 2.0 interface

   - Embedded host and device function
   - Eight endpoint

Supported Features
==================

.. zephyr::board-supported-hw::

Connections and IOs
===================

LED
---

* Led0 (blue) = D13

Serial IO
---------

* sparkfun_spi = sercom4
* sparkfun_i2c = sercom3
* sparkfun_serial = sercom0
* sparkfun_dac = dac0
* sparkfun_adc = adc

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications for the ``sparkfun_samd21_breakout/samd21g18a`` board target can be
built, flashed, and debugged in the usual way. See
:ref:`build_an_application` and :ref:`application_run` for more details on
building and running.

Flashing
========

To enter flash mode, connect the board via the micro USB to the host, press the
reset button twice. The onboard blue led should slowly fade, indicating flash mode.

Flashing Application
********************
This board utilizes the UF2 bootloader, which flashes firmware via uploaded a file
to board via mass-storage device. See `uf2-samdx1 bootloader info`_.


Flashing Bootloader
*******************
If flashing the bootloader you can use a Jeff Probe/Black Magic Debugger to connect
via SWD. Note this can also be used to recover a bricked board. Bootloader source
code repo is available at: `uf2-samdx1 bootloader repo`_.

Build bootloader for sparkfun samd21 dev board:

.. code-block:: console

   git clone https://github.com/adafruit/uf2-samdx1
   cd uf2-samdx1
   make BOARD=sparkfun-samd21-dev

Place commands to flash bootloader into a ``gdb_init`` file:

.. code-block:: console

   target extended-remote /dev/ttyACM<num> # check in dmesg
   monitor swdp_scan # make sure this works too, cable could be upside down
   attach 1
   load

Connect the debug probe and run:

.. code-block:: console

   # Run gdb with your gdb_init script:
   arm-none-eabi-gdb bootloader-sparkfun-samd21-dev-<bootloader-version>.elf -x gdb_init

Debugging
=========
Using the Jeff Probe/Black Magic Debugger, you can connect SWD and run gdb
through the serial connection. See above for example instructions for flashing the bootloader.
Otherwise, use west to debug.

Debug with west:

.. code-block:: console

   west debug -r blackmagicprobe


References
**********

.. target-notes::


.. _SAMD21 dev breakout specification website: https://www.sparkfun.com/sparkfun-samd21-dev-breakout.html
.. _uf2-samdx1 bootloader repo: https://github.com/adafruit/uf2-samdx1
.. _uf2-samdx1 bootloader info: https://learn.sparkfun.com/tutorials/arm-programming/bootloaders
