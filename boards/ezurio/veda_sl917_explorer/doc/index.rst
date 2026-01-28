.. zephyr:board:: veda_sl917_explorer


Overview
********

The Veda SL917 SoC Explorer board features the Ezurio Veda SL917 radio module
based on the Silicon Labs SiWG917 chipset. This board includes everything
necessary for getting started developing with Zephyr.

SiWG917 is an ultra-low power SoC that includes hardware support for Single-Band
Wi-Fi 6 + Bluetooth LE 5.4.

Hardware
********

For more information about the Veda SL917 module and SoC Explorer board, refer to these
documents:

- `Ezurio Website`_
- `Veda SL917 Datasheet`_

Supported Features
==================

.. zephyr:board-supported-hw::

Refer to the `SiWx917 Wi-Fi Features`_ page for a list of supported Wi-Fi features.


Programming and Debugging
*************************

.. zephyr:board-supported-runners::


Flashing
========

Applications for the ``veda_sl917_explorer`` board can be built in the usual
way. The flash method requires `Simplicity Commander`_ installed on the host.

Connect the Veda SL917 SoC Explorer board to your host computer using the USB port.

Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: veda_sl917_explorer
   :goals: flash

Open a serial terminal (minicom, putty, etc.) with the following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Reset the board and you should see the following message in the terminal:

.. code-block:: console

   Hello World! veda_sl917_explorer

Debugging
=========

Debugging relies on the JLink tool, however JLink is not able to flash the
firmware. Therefore, starting a debug session requires two steps. Use
``west flash`` to flash the firmware using Simplicity Commander. Then use
``west attach`` to use the JLink tool to attach to the board. The Zephyr
image may already be booted when running the ``west attach`` command.
Execute ``monitor reset`` in the gdb prompt to reset the board as needed.

.. _Ezurio Website:
   https://www.ezurio.com/product/veda-sl917-wi-fi-6-bluetooth-le-5-4-module

.. _Veda SL917 Datasheet:
   https://www.ezurio.com/documentation/datasheet-veda-sl917-soc-module

.. _SiWx917 Wi-Fi Features:
   https://docs.zephyrproject.org/latest/boards/silabs/radio_boards/common/wifi.html

.. _SiWG917 Website:
   https://www.silabs.com/wireless/wi-fi/siwx917-wireless-socs

.. _SiWG917 Datasheet:
   https://www.silabs.com/documents/public/data-sheets/siwg917-datasheet.pdf

.. _SiWG917 Reference Manual:
   https://www.silabs.com/documents/public/reference-manuals/siw917x-family-rm.pdf

.. _Simplicity Commander:
   https://www.silabs.com/developer-tools/simplicity-studio/simplicity-commander
