.. zephyr:board:: siwx917_rb4338a


Overview
********

The SiWx917-RB4338A (aka BRD4338A) radio board provides support for the Silicon
Labs SiWG917 SoC. This board cannot be used stand-alone and requires a a
`Wireless Pro Kit`_ Mainboard (Si-MB4002A aka BRD4002A), for power, debug
options etc.

This board might be sold as a part of the `SiWx917-PK6031A`_ bundle ("SiWx917
Wi-Fi 6 and Bluetooth LE 8 MB Flash Pro Kit"), which includes the BRD4002A
Mainboard in addition of the BRD4338A.

SiWG917 is an ultra-low power SoC that includes hardware support for Single-Band
Wi-Fi 6 + Bluetooth LE 5.4, Matter...

Hardware
********

For more information about the SiWG917 SoC and BRD4338A board, refer to these
documents:

- `SiWG917 Website`_
- `SiWG917 Datasheet`_
- `SiWG917 Reference Manual`_
- `BRD4338A Website`_
- `BRD4338A User Guide`_


Supported Features
==================

.. zephyr:board-supported-hw::

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Flashing
========

Applications for the ``siwx917_rb4338a`` board can be built in the usual
way. The flash method requires on `Simplicity Commander`_ installed on the host.

Then, connect the BRD4002A board with a mounted BRD4338A radio module to your
host computer using the USB port.

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: siwx917_rb4338a
   :goals: flash

Open a serial terminal (minicom, putty, etc.) with the following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Reset the board and you should see the following message in the terminal:

.. code-block:: console

   Hello World! siwx917_rb4338a


Debugging
=========

Debuggning relies on JLink tool. JLink is not able to flash the firmware. So
debug session has to be done in two steps. ``west flash`` will flahs the
firmware using Simplicity Commander. Then ``west attach`` will use JLink to
attach to the board. The Zephyr image may has already booted when user runs
``west attach``. User may execute ``monitor reset`` in the gdb prompt to reset
the board.



.. _SiWx917-PK6031A:
   https://www.silabs.com/development-tools/wireless/wi-fi/siwx917-pk6031a-wifi-6-bluetooth-le-soc-pro-kit

.. _Wireless Pro Kit:
   https://www.silabs.com/development-tools/wireless/wireless-pro-kit-mainboard

.. _BRD4338A Website:
   https://www.silabs.com/development-tools/wireless/wi-fi/siwx917-rb4338a-wifi-6-bluetooth-le-soc-radio-board

.. _BRD4338A User Guide:
   https://www.silabs.com/documents/public/user-guides/ug562-brd4338a-user-guide.pdf

.. _SiWG917 Website:
   https://www.silabs.com/wireless/wi-fi/siwx917-wireless-socs

.. _SiWG917 Datasheet:
   https://www.silabs.com/documents/public/data-sheets/siwg917-datasheet.pdf

.. _SiWG917 Reference Manual:
   https://www.silabs.com/documents/public/reference-manuals/siw917x-family-rm.pdf

.. _Simplicity Commander:
   https://www.silabs.com/developer-tools/simplicity-studio/simplicity-commander
