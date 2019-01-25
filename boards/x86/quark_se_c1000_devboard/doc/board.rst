.. _quark_se_c1000_devboard:

Quark SE C1000 Development Board
################################

Overview
********

The Intel |reg| Quark |trade| SE microcontroller C1000 is an ultra-low power
Intel Architecture (IA) system-on-chip (SoC) that integrates an Intel |reg|
Quark SE processor core, Sensor subsystem, Memory subsystem (volatile and
non-volatile), pattern matching engine, and industry standard I/O interfaces
into a single silicon-die packaged solution.

Hardware
********

Intel |reg| Quark |trade| SE microcontroller C1000 development platform main expansion
options:

- "Arduino Uno" like SIL sockets (1.8V and 3.3V IO)
- On-board components:

  - Certified Bluetooth low energy (BLE) module with integrated antenna
  - 802.15.4 transceiver with on-board antenna
  - 6-axis Accelerometer / Gyroscope (connected to Sensor subsystem)
  - Temperature sensor (connected to Intel® Quark™ SE processor core)
  - UART/JTAG to USB converter (USB debug port)
- Other connectors include:

  - 2 x USB Device Port – micro Type B
  - 5V input a screw terminal/header
  - Dual row connectors for all I/O signals from the SoC

- Power sources for this platform:

  - External (2.5V - 5V) DC input
  - USB power (5V) – via debug / SoC device port


Consult with the `Platform User Guide`_ for more details.

Supported Features
===================


+-----------+------------+-----+-----+-----------------------+
| Interface | Controller | ARC | x86 | Driver/Component      |
+===========+============+=====+=====+=======================+
| APIC      | on-chip    | N   | Y   | interrupt_controller  |
+-----------+------------+-----+-----+-----------------------+
| UART      | on-chip    | N   | Y   | serial port-polling;  |
|           |            |     |     | serial port-interrupt |
+-----------+------------+-----+-----+-----------------------+
| SPI       | on-chip    | Y   | Y   | spi                   |
+-----------+------------+-----+-----+-----------------------+
| ADC       | on-chip    | Y   | N   | adc                   |
+-----------+------------+-----+-----+-----------------------+
| I2C       | on-chip    | Y   | Y   | i2c                   |
+-----------+------------+-----+-----+-----------------------+
| GPIO      | on-chip    | Y   | Y   | gpio                  |
+-----------+------------+-----+-----+-----------------------+
| PWM       | on-chip    | Y   | Y   | pwm                   |
+-----------+------------+-----+-----+-----------------------+
| mailbox   | on-chip    | Y   | Y   | ipm                   |
+-----------+------------+-----+-----+-----------------------+

Programming and Debugging
*************************

The board configuration details are found in the project's tree at
:file:`boards/x86/quark_se_c1000_devboard`.

Applications for the ``quark_se_c1000_devboard`` board configuration can be built and
flashed in the usual way (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Flashing
========

#. Since the board has a built-in JTAG; it is possible to flash the device
   through the USB only.

#. Connect the board via USB to the host computer.

#. Build and flash a Zephyr application. Here is an example for the
   :ref:`hello_world` application.

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: quark_se_c1000_devboard
      :goals: build flash

Debugging
=========

You can debug an application in the usual way.  Here is an example for the
:ref:`hello_world` application.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: quark_se_c1000_devboard
   :maybe-skip-config:
   :goals: debug

.. _Platform User Guide:
   https://www.intel.com/content/dam/www/public/us/en/documents/guides/quark-c1000-development-platform-user-guide.pdf
