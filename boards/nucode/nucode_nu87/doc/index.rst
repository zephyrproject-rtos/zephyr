.. zephyr:board:: nucode_nu87

Overview
********

The NUCODE NU87-TinyDK is a compact development board carrying the NUCODE NU-87
module, which is built around the Realtek RTL8720DF, a part in the RTL872xD
(AmebaD) family. The SoC pairs a Real-M300 (Arm Cortex-M33) application core
with a Real-M200 core, and supports dual-band Wi-Fi 4 and Bluetooth LE 5.0.

For more information, see the `NU87-TinyDK product page`_.

The board breaks the module's GPIOs out on two 0.1" headers, and carries an RGB
LED, a USB-C connector wired to a CP2102N USB-to-UART bridge, and three buttons.

Hardware
********

The ``nucode_nu87`` board is built around the RTL8720DF with 4 MiB of external
NOR flash.

- Realtek RTL8720DF, Real-M300 (Cortex-M33) + Real-M200
- 4 MiB NOR flash
- RGB LED
- USB-C connector, connected to an on-board CP2102N USB-to-UART bridge
- Three buttons: ``RESET``, ``BOOT`` and one silkscreened ``PA27``

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

LOGUART
-------

The console is the SoC's LOGUART on PA7 (TX) and PA8 (RX), running at
1500000 baud. Both pins are routed to the on-board CP2102N, so the console
appears on the host as a USB serial device.

LEDs
----

* RGB LED red: PA13
* RGB LED green: PA12
* RGB LED blue: PA14

All three are active high. The two small LEDs next to the USB connector are
UART activity indicators driven by the CP2102N and cannot be controlled from
firmware.

Buttons
-------

* ``RESET`` resets the SoC.
* ``BOOT`` is sampled at reset only. Holding it while pressing ``RESET`` and
  then releasing it enters the ROM UART download mode used for programming.
* A third button is silkscreened ``PA27``.

Neither ``RESET`` nor ``BOOT`` is readable as a GPIO from the application. The
``PA27`` button was not either: with every pin of both GPIO banks configured as
an input and monitored, pressing it produced no level change on any of them,
while a ``RESET`` press during the same run was picked up as expected. It is
therefore left out of the devicetree until its wiring is confirmed, rather than
declared as a ``gpio-keys`` entry that would not work.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Building
========

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: nucode_nu87
   :goals: build

That builds and links without any binary blobs. Producing an image that can
actually be programmed additionally needs the prebuilt image for the NP core,
which is off by default and does require the Realtek HAL blobs:

.. code-block:: console

   west blobs fetch hal_realtek
   west build -b nucode_nu87 samples/hello_world -- -DCONFIG_SOC_AMEBA_NP_IMAGE=y

That build writes ``bootloader_all.bin`` and ``km0_km4_app.bin`` into the
``images`` directory alongside the usual build output.

Flashing
========

The board has no dedicated debug connector. Images are programmed over the same
UART that carries the console, reached through the on-board CP2102N bridge, by
putting the SoC in its ROM download mode.

`AmebaImageTool`_ drives that protocol from a host. See the Image Tool chapter
of the application note for details.

#. Connect the board to the host with a USB-C cable.
#. Put the SoC in download mode: hold ``BOOT``, press and release ``RESET``,
   then release ``BOOT``.
#. Select the serial port and a baud rate of 1500000.
#. Program ``bootloader_all.bin`` at 0x000000 and ``km0_km4_app.bin`` at
   0x014000, both from the ``images`` directory of the build.
#. Press ``RESET`` to run the new image.

.. note::

   On a blank device both the bootloader and the application image must be
   programmed.

References
**********

.. _`NU87-TinyDK product page`: https://nucode.store/product/nu-87-tiny-dk-nucode-rtl8720df-ble-50-dual-band-wi-fi-mcu-arm-cortex-m/31/
.. _`AmebaImageTool`: https://github.com/Ameba-AIoT/ameba-rtos/blob/master/tools/ameba/ImageTool/AmebaImageTool.exe
