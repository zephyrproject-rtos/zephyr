.. zephyr:board:: siwx917_dk2605a


Overview
********
The SiWx917-DK2605A (BRD2605A) is a development kit for the SiWx917 Wi-Fi 6 and
Bluetooth LE SoC. This board port provides support for Zephyr RTOS.

Hardware
********

For more information about the SiWG917 SoC and BRD2605A board, refer to these
documents:

- `SiWG917 Website`_
- `SiWG917 Datasheet`_
- `SiWG917 Reference Manual`_
- `BRD2605A Website`_
- `BRD2605A User Guide`_

Supported Features
******************

.. zephyr:board-supported-hw::

Refer to the :ref:`siwx917_wifi_features` page for a list of supported Wi-Fi features.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Building and Flashing
*********************

Applications for the ``siwx917_dk2605a`` board can be built in the usual
way. The flash method requires on `Simplicity Commander`_ installed on the host.

Then, connect the board to your host computer using the USB port.

Here's an example of building and flashing the :zephyr:code-sample:`hello_world` sample:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: siwx917_dk2605a
   :goals: build flash

Open a serial terminal (minicom, putty, etc.) with the following settings:

- Speed: 115200
- Data: 8 bits
- Parity: None
- Stop bits: 1

Reset the board and you should see the following message in the terminal:

.. code-block:: console

   Hello World! siwx917_dk2605a

Debugging
*********

Debugging relies on JLink tool. JLink is not able to flash the firmware. So
debug session has to be done in two steps. ``west flash`` will flash the
firmware using Simplicity Commander. Then ``west attach`` will use JLink to
attach to the board. The Zephyr image may has already booted when user runs
``west attach``. User may execute ``monitor reset`` in the gdb prompt to reset
the board.

.. _BRD2605A Website:
   https://www.silabs.com/development-tools/wireless/wi-fi/siwx917-dk2605a-wifi-6-bluetooth-le-soc-dev-kit?tab=overview

.. _BRD2605A User Guide:
   https://www.silabs.com/documents/public/user-guides/ug581-brd2605a-user-guide.pdf

.. _SiWG917 Website:
   https://www.silabs.com/wireless/wi-fi/siwx917-wireless-socs

.. _SiWG917 Datasheet:
   https://www.silabs.com/documents/public/data-sheets/siwg917-datasheet.pdf

.. _SiWG917 Reference Manual:
   https://www.silabs.com/documents/public/reference-manuals/siw917x-family-rm.pdf

.. _Simplicity Commander:
   https://www.silabs.com/developer-tools/simplicity-studio/simplicity-commander
