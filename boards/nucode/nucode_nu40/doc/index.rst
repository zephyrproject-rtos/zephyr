.. zephyr:board:: nucode_nu40

Overview
********

The NUCODE NU40 is a development kit based on the Nordic Semiconductor
nRF52840 ARM Cortex-M4F SoC. It features an onboard UF2 bootloader
(Adafruit nRF52 fork) enabling drag-and-drop firmware flashing via USB,
as well as support for BLE 5.4, IEEE 802.15.4 (Thread/Zigbee), and
USB 2.0 Full Speed.

More information about the board can be found at the
`NUCODE NU40 website`_.

Hardware
********

The ``nucode_nu40/nrf52840`` board target has two external oscillators.
The frequency of the slow clock is 32.768 kHz. The frequency of the
main clock is 32 MHz.

Supported Features
==================

.. zephyr:board-supported-hw::

Connections and IOs
===================

LEDs
----

* LED0 = P0.13 (active high)
* LED1 = P0.14 (active high)
* LED2 = P0.15 (active high)
* LED3 = P0.16 (active high)

Push Buttons
------------

* BUTTON0 = SW0 = P0.11 (active low, pull-up)
* BUTTON1 = SW1 = P0.12 (active low, pull-up)
* BUTTON2 = SW2 = P0.24 (active low, pull-up)
* BUTTON3 = SW3 = P0.25 (active low, pull-up)

UART
----

* UART0: TX = P0.6, RX = P0.8, RTS = P0.5, CTS = P0.7
* UART1: TX = P1.2, RX = P1.1

I2C
---

* I2C0 (Arduino): SDA = P0.26, SCL = P0.27
* I2C1: SDA = P0.30, SCL = P0.31

SPI
---

* SPI1: SCK = P0.31, MOSI = P0.30, MISO = P1.8
* SPI3 (Arduino): SCK = P1.15, MOSI = P1.13, MISO = P1.14

Programming and Debugging
*************************

.. zephyr:board-supported-runners::

Applications for the ``nucode_nu40/nrf52840`` board configuration can be
built in the usual way (see :ref:`build_an_application` for more details).

Flashing
========

The board supports the following programming options:

1. Using the built-in UF2 bootloader
2. Using an external :ref:`debug probe <debug-probes>`

Option 1: Using the Built-In UF2 Bootloader
--------------------------------------------

The board is factory-programmed with a UF2 bootloader (Adafruit nRF52
fork, maintained by chcbaram). With this option, the board enumerates as
a USB mass-storage device when in bootloader mode.

#. Enter bootloader mode by double-tapping the RESET button (or holding
   BUTTON0 while connecting USB). The board mounts as ``BARAM-NU40``.

#. Build the sample application:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: nucode_nu40/nrf52840
      :goals: build

#. Copy the generated ``zephyr.uf2`` file to the ``BARAM-NU40`` drive:

   .. code-block:: console

      cp build/zephyr/zephyr.uf2 /run/media/$USER/BARAM-NU40/

   The board resets automatically and runs the new firmware.

Option 2: Using an External Debug Probe
---------------------------------------

The board can also be flashed and debugged using a J-Link or other
compatible debug probe connected to the SWD pads.

.. code-block:: console

   west flash

Debugging
=========

Refer to the :ref:`application_run` page for more details on debugging.

References
**********

.. target-notes::

.. _NUCODE NU40 website: https://nuworks.io
