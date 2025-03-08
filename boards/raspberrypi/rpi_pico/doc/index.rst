.. zephyr:board:: rpi_pico

Overview
********

The `Raspberry Pi Pico`_ and Pico W are small, low-cost, versatile boards from
Raspberry Pi. They are equipped with an `RP2040 <RP2040_Datasheet>`_ SoC, an on-board LED,
a USB connector, and an SWD interface.

The Pico W additionally contains an `Infineon CYW43439`_ 2.4 GHz Wi-Fi/Bluetooth module.

The USB bootloader allows the ability to flash without any adapter,
in a drag-and-drop manner.
It is also possible to flash and debug the boards with their SWD interface,
using an external adapter.

Hardware
********

- Dual core Arm Cortex-M0+ processor running up to 133MHz
- 264KB on-chip SRAM
- 2MB on-board QSPI flash with XIP capabilities
- 26 GPIO pins
- 3 Analog inputs
- 2 UART peripherals
- 2 SPI controllers
- 2 I2C controllers
- 16 PWM channels
- USB 1.1 controller (host/device)
- 8 Programmable I/O (PIO) for custom peripherals
- On-board LED
- 1 Watchdog timer peripheral
- Infineon CYW43439 2.4 GHz Wi-Fi chip (Pico W only)


.. figure:: img/rpi_pico.jpg
     :align: center
     :alt: Raspberry Pi Pico


.. figure:: img/rpi_pico_w.jpg
     :align: center
     :alt: Raspberry Pi Pico W

     Raspberry Pi Pico (above) and Pico W (below)
     (Images courtesy of Raspberry Pi)

Supported Features
==================

.. zephyr:board-supported-hw::

.. _rpi_pico_pin_mapping:

Pin Mapping
===========

The peripherals of the RP2040 SoC can be routed to various pins on the board.
The configuration of these routes can be modified through DTS. Please refer to
the datasheet to see the possible routings for each peripheral.

External pin mapping on the Pico W is identical to the Pico, but note that internal
RP2040 GPIO lines 23, 24, 25, and 29 are routed to the Infineon module on the W.
Since GPIO 25 is routed to the on-board LED on the Pico, but to the Infineon module
on the Pico W, the "blinky" sample program does not work on the W (use hello_world for
a simple test program instead).

Default Zephyr Peripheral Mapping:
----------------------------------

.. rst-class:: rst-columns

- UART0_TX : P0
- UART0_RX : P1
- I2C0_SDA : P4
- I2C0_SCL : P5
- I2C1_SDA : P6
- I2C1_SCL : P7
- SPI0_RX : P16
- SPI0_CSN : P17
- SPI0_SCK : P18
- SPI0_TX : P19
- ADC_CH0 : P26
- ADC_CH1 : P27
- ADC_CH2 : P28
- ADC_CH3 : P29

Programmable I/O (PIO)
**********************

The RP2040 SoC comes with two PIO peripherals. These are two simple
co-processors that are designed for I/O operations. The PIOs run
a custom instruction set, generated from a custom assembly language.
PIO programs are assembled using :command:`pioasm`, a tool provided by Raspberry Pi.

Zephyr does not (currently) assemble PIO programs. Rather, they should be
manually assembled and embedded in source code. An example of how this is done
can be found at :zephyr_file:`drivers/serial/uart_rpi_pico_pio.c`.

Sample:  SPI via PIO
====================

The :zephyr_file:`samples/sensor/bme280/README.rst` sample includes a
demonstration of using the PIO SPI driver to communicate with an
environmental sensor.  The PIO SPI driver supports using any
combination of GPIO pins for an SPI bus, as well as allowing up to
four independent SPI buses on a single board (using the two SPI
devices as well as both PIO devices).

.. _rpi_pico_pio_based_features:

PIO Based Features
==================

Raspberry Pi Pico's PIO is a programmable chip that can implement a variety of peripherals.

.. list-table::
   :header-rows: 1

   * - Peripheral
     - Kconfig option
     - Devicetree compatible
   * - UART (PIO)
     - :kconfig:option:`CONFIG_SERIAL`
     - :dtcompatible:`raspberrypi,pico-uart-pio`
   * - SPI (PIO)
     - :kconfig:option:`CONFIG_SPI`
     - :dtcompatible:`raspberrypi,pico-spi-pio`
   * - WS2812 (PIO)
     - :kconfig:option:`CONFIG_LED_STRIP`
     - :dtcompatible:`worldsemi,ws2812-rpi_pico-pio`

Programming and Debugging
*************************

Applications for the ``rpi_pico`` board configuration can be built and
flashed in the usual way (see :ref:`build_an_application` and
:ref:`application_run` for more details).

System requirements
===================

Prerequisites for the Pico W
----------------------------

Building for the Raspberry Pi Pico W requires the AIROC binary blobs
provided by Infineon. Run the command below to retrieve those files:

.. code-block:: console

   west blobs fetch hal_infineon

.. note::

   It is recommended running the command above after :file:`west update`.

Debug Probe and Host Tools
--------------------------

Several debugging tools support the Raspberry Pi Pico.
The `Raspberry Pi Debug Probe`_ is an easy-to-obtain CMSIS-DAP adapter
officially provided by the Raspberry Pi Foundation,
making it a convenient choice for debugging ``rpi_pico``.

It can be used with

- :ref:`openocd-debug-host-tools`
- :ref:`pyocd-debug-host-tools`

OpenOCD is the default for ``rpi_pico``.

- `SEGGER J-Link`_
- `Black Magic Debug Probe <Black Magic Debug>`_

can also be used.
These are used with dedicated probes.

Flashing
========

The ``rpi_pico`` can flash with Zephyr's standard method.
See also :ref:`Building, Flashing and Debugging<west-flashing>`.

Here is an example of building and flashing the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: rpi_pico
   :goals: build

.. code-block:: console

  west flash --runner jlink


.. _rpi_pico_flashing_using_openocd:

Using OpenOCD
-------------

To use a debugging adapter such as the Raspberry Pi Debug Probe,
You must configure **udev**. Refer to :ref:`setting-udev-rules` for details.

The Raspberry Pi Pico has an SWD interface that can be used to program
and debug the onboard SoC. This interface can be used with OpenOCD.
To use it, OpenOCD version 0.12.0 or later is needed.

If you are using a Debian based system (including RaspberryPi OS, Ubuntu. and more),
using the `pico_setup.sh`_ script is a convenient way to set up the forked version of OpenOCD.

Here is an example of building and flashing the :zephyr:code-sample:`blinky` application.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: rpi_pico
   :goals: build flash
   :gen-args: -DOPENOCD=/usr/local/bin/openocd -DRPI_PICO_DEBUG_ADAPTER=cmsis-dap

Set the CMake option **OPENOCD** to :file:`/usr/local/bin/openocd`. This should work
with the OpenOCD that was installed with the default configuration.
This configuration also works with an environment that is set up by the `pico_setup.sh`_ script.

**RPI_PICO_DEBUG_ADAPTER** specifies what debug adapter is used for debugging.

If **RPI_PICO_DEBUG_ADAPTER** was not set, ``cmsis-dap`` is used by default.
The ``raspberrypi-swd`` and ``jlink`` are verified to work.
How to connect ``cmsis-dap`` and ``raspberrypi-swd`` is described in `Getting Started with Raspberry Pi Pico`_.
Any other SWD debug adapter maybe also work with this configuration.

The value of **RPI_PICO_DEBUG_ADAPTER** is cached, so it can be omitted from
``west flash`` and ``west debug`` if it was previously set while running
``west build``.

**RPI_PICO_DEBUG_ADAPTER** is used in an argument to OpenOCD as ``"source [find interface/${RPI_PICO_DEBUG_ADAPTER}.cfg]"``.
Thus, **RPI_PICO_DEBUG_ADAPTER** needs to be assigned the file name of the debug adapter.

.. _rpi_pico_flashing_using_uf2:

Using UF2
---------

If you don't have an SWD adapter, you can flash the Raspberry Pi Pico with
a UF2 file. By default, building an app for this board will generate a
:file:`build/zephyr/zephyr.uf2` file. If the Pico is powered on with the ``BOOTSEL``
button pressed, it will appear on the host as a mass storage device. The
UF2 file should be drag-and-dropped to the device, which will flash the Pico.

Debugging
=========

Like flashing, debugging can also be performed using Zephyr's standard method
(see :ref:`application_run`).
The following sample demonstrates how to debug using OpenOCD and
the `Raspberry Pi Debug Probe`_.

.. zephyr-app-commands::
   :zephyr-app: samples/basic/blinky
   :board: rpi_pico
   :maybe-skip-config:
   :goals: debug
   :gen-args: -DOPENOCD=/usr/local/bin/openocd -DRPI_PICO_DEBUG_ADAPTER=cmsis-dap

The default debugging tool is ``openocd``.
If you use a different tool, specify it with the ``--runner``,
such as ``jlink``.

If you use OpenOCD, see also the description about flashing :ref:`rpi_pico_flashing_using_uf2`
for more information.


.. target-notes::

.. _Raspberry Pi Pico:
   https://www.raspberrypi.com/products/raspberry-pi-pico/

.. _RP2040 Datasheet:
   https://datasheets.raspberrypi.com/rp2040/rp2040-datasheet.pdf

.. _Infineon CYW43439:
   https://www.infineon.com/cms/en/product/wireless-connectivity/airoc-wi-fi-plus-bluetooth-combos/wi-fi-4-802.11n/cyw43439/

.. _pico_setup.sh:
   https://raw.githubusercontent.com/raspberrypi/pico-setup/master/pico_setup.sh

.. _Getting Started with Raspberry Pi Pico:
   https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf

.. _Raspberry Pi Debug Probe:
   https://www.raspberrypi.com/documentation/microcontrollers/debug-probe.html

.. _SEGGER J-Link:
   https://www.segger.com/products/debug-probes/j-link/

.. _Black Magic Debug:
   https://black-magic.org/
