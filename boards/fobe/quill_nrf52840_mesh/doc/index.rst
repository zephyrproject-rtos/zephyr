.. zephyr:board:: quill_nrf52840_mesh

Overview
********

The FoBE Quill nRF52840 Mesh is a development kit featuring the nRF52840 SoC and an integrated
SX1262 LoRa® transceiver.

For more details see the `FoBE Quill nRF52840 Mesh`_ documentation page.

Hardware
********

The FoBE Quill nRF52840 Mesh is a compact and versatile development platform for IoT solutions.
It combines Nordic's high-end multiprotocol SoC, the nRF52840, with Semtech's ultra-low-power
sub-GHz radio transceiver, the SX1262 (packaged using SiP technology).
Designed for IoT applications, it offers a comprehensive wireless connectivity solution,
supporting protocols such as Bluetooth 5, Thread, Zigbee, IEEE 802.15.4, and LoRa®.

This development board is feature-rich, including a battery charger,
dedicated power path management, an ultra-low quiescent current DC-DC converter,
a 1.14-inch color IPS display, user-programmable LEDs and buttons, an MFP expansion connector,
a reversible USB-C connector, and header sockets for easy expansion.

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

The `FoBE Quill nRF52840 Mesh`_ Documentation has detailed information about board
connections.

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

The Quill nRF52840 Mesh ships with the `FoBE nRF52 Bootloader`_ which supports flashing
using `UF2`_. Doing so allows easy flashing of new images, but does not support debugging the
device. For debugging please use `External Debugger`_.

UF2 Flashing
============

To enter the bootloader, connect the USB port of the Quill nRF52840 Mesh to your host,
and double tap the reset button. A mass storage device named ``FOBEBOOT`` should appear
on the host. Using the command line, or your file manager copy the :file:`zephyr/zephyr.uf2`
file from your build to the base of the ``FOBEBOOT`` mass storage device. The board will
automatically reset and launch the newly flashed application.

External Debugger
=================

In order to support debugging the device, instead of using the bootloader, you can use an
:ref:`External Debug Probe <debug-probes>`. To flash and debug Zephyr applications you need to
connect an SWD debugger to the SWD pins on the board.

For Segger J-Link debug probes, follow the instructions in the :ref:`jlink-external-debug-probe`
page to install and configure all the necessary software.

Building & Flashing
*******************

Simple build and flash
======================

Build and flash applications as usual (see :ref:`build_an_application` and :ref:`application_run`
for more details).

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: quill_nrf52840_mesh
   :goals: build

The usual ``flash`` target will work with the ``quill_nrf52840_mesh`` board configuration.
Here is an example for the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: quill_nrf52840_mesh
   :goals: flash

Flashing with External Debugger
--------------------------------

Setup and connect a supported debug probe (JLink, instructions at
:ref:`jlink-external-debug-probe` or BlackMagic Probe). Then build and flash applications as usual.

Here is an example for the :zephyr:code-sample:`hello_world` application.

First, run your favorite terminal program to listen for output.

.. code-block:: console

   $ minicom -D <tty_device> -b 115200

Replace :code:`<tty_device>` with the port where the board can be found. For example,
under Linux, :code:`/dev/ttyACM0`.

Then build and flash the application. Just add ``CONFIG_BOOT_DELAY=5000`` to the configuration,
so that USB CDC ACM is initialized before any text is printed:

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: quill_nrf52840_mesh
   :goals: build flash
   :gen-args: -DCONFIG_BOOT_DELAY=5000

Debugging
*********

Refer to the :ref:`jlink-external-debug-probe` page to learn about debugging
boards with a Segger IC.

Debugging using a BlackMagic Probe is also supported.

Here is an example for building and debugging the :zephyr:code-sample:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: quill_nrf52840_mesh
   :goals: debug

Testing the LEDs
*****************

There is a sample that allows to test that LEDs on the board are working properly with Zephyr:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: quill_nrf52840_mesh
   :goals: build flash

You can build and flash the examples to make sure Zephyr is running correctly on your board.

Testing shell over USB
***********************

There is a sample that allows to test shell interface over USB CDC ACM interface with Zephyr:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/shell/shell_module
   :board: quill_nrf52840_mesh
   :goals: build flash

References
**********

.. target-notes::

.. _`FoBE Quill nRF52840 Mesh`: https://docs.fobestudio.com/product/f1101
.. _`FoBE nRF52 Bootloader`: https://github.com/fobe-projects/fobe-nrf52-bootloader
.. _`UF2`: https://github.com/microsoft/uf2
