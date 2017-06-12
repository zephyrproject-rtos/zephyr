.. _96b_carbon_nrf51_board:

96Boards Carbon nRF51
#####################

.. note::

   If you're looking to reprogram the main STMicro part, see
   :ref:`96b_carbon_board`. Users should not use this configuration
   unless they want to reprogram the secondary chip which provides
   Bluetooth connectivity.

Overview
********

Zephyr applications use the 96b_carbon_nrf51 configuration to run on
the secondary nRF51822 chip on the 96Boards Carbon. This chip provides
Bluetooth functionality to the main STM32F401RET chip via SPI.

Hardware
********

The 96Boards Carbon nRF51 has two external oscillators. The frequency
of the slow clock is 32.768 kHz. The frequency of the main clock is 16
MHz.

See :ref:`96b_carbon_board` for other general information about the
board; that configuration is for the same physical board, just a
different chip.

Supported Features
==================

+-----------+------------+-------------------------------------+
| Interface | Controller | Driver/Component                    |
+===========+============+=====================================+
| NVIC      | on-chip    | nested vector interrupt controller  |
+-----------+------------+-------------------------------------+
| RTC       | on-chip    | system clock                        |
+-----------+------------+-------------------------------------+
| UART      | on-chip    | serial port                         |
+-----------+------------+-------------------------------------+
| GPIO      | on-chip    | gpio                                |
+-----------+------------+-------------------------------------+
| FLASH     | on-chip    | flash                               |
+-----------+------------+-------------------------------------+
| SPIS      | on-chip    | SPI slave                           |
+-----------+------------+-------------------------------------+
| RADIO     | on-chip    | Bluetooth                           |
+-----------+------------+-------------------------------------+

The default configuration can be found in the defconfig file:

        ``boards/arm/96b_carbon_nrf51/96b_carbon_nrf51_defconfig``

Connections and IOs
===================

The SWD debug pins are broken out to an external header; all other
connected pins are to the main STM32F401RET chip.

Programming and Debugging
*************************

Flashing
========

The 96Boards Carbon nRF51 can be flashed using an external SWD
debugger, via the debug header labeled "BLE" on the board's
silkscreen. The header is not populated; 0.1" male header must be
soldered on first.

.. figure:: img/96b-carbon-nrf51-debug.png
     :align: center
     :alt: 96Boards Carbon nRF51 Debug

     96Boards Carbon nRF51 Debug

The following example assumes a Zephyr binary ``zephyr.elf`` will be
flashed to the board.

It uses the `Black Magic Debug Probe`_ as an SWD programmer, which can
be connected to the BLE debug header using flying leads and its 20 Pin
JTAG Adapter Board Kit. When plugged into your host PC, the Black
Magic Debug Probe enumerates as a USB serial device as documented on
its `Getting started page`_.

It also uses the GDB binary provided with the Zephyr SDK,
``arm-zephyr-eabi-gdb``. Other GDB binaries, such as the GDB from GCC
ARM Embedded, can be used as well.

.. code-block:: console

   $ arm-zephyr-eabi-gdb -q zephyr.elf
   (gdb) target extended-remote /dev/ttyACM0
   Remote debugging using /dev/ttyACM0
   (gdb) monitor swdp_scan
   Target voltage: 3.3V
   Available Targets:
   No. Att Driver
    1      nRF51
   (gdb) attach 1
   Attaching to Remote target
   0xabcdef12 in ?? ()
   (gdb) load

Debugging
=========

After you've flashed the chip, you can keep debugging using the same
GDB instance. To reattach, just follow the same steps above, but don't
run "load". You can then debug as usual with GDB. In particular, type
"run" at the GDB prompt to restart the program you've flashed.

As an aid to debugging, this board configuration directs a console
output to a currently unused pin connected to the STM32F401RET. Users
who are experienced in electronics rework can remove a resistor (R22)
on the board and attach a wire to the nRF51822's UART output.

References
**********

- `Board documentation from 96Boards`_
- `nRF51822 information from Nordic Semiconductor`_

.. _Black Magic Debug Probe:
   https://github.com/blacksphere/blackmagic/wiki

.. _Getting started page:
   https://github.com/blacksphere/blackmagic/wiki/Getting-Started

.. _Board documentation from 96Boards:
   http://www.96boards.org/product/carbon/

.. _nRF51822 information from Nordic Semiconductor:
   https://www.nordicsemi.com/eng/Products/Bluetooth-low-energy/nRF51822
