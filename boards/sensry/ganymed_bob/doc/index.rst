.. zephyr:board:: ganymed_bob

.. _ganymed_bob:

Overview
********

.. note::

   All software for the Ganymed Break-Out-Board (BOB) is experimental and hardware availability
   is restricted to the participants in the limited sampling program.

The Ganymed board hardware provides support for the Ganymed sy1xx series IoT multicore
RISC-V SoC with optional sensor level.

Hardware
********

* 32-Bit RISC-V 1+8-core processor, up to 500MHz

  * 1x Data Acquisition Unit
  * 8x Data Processing Unit
  * Event Bus
  * MicroDMA

* 4096 KB Global SRAM
* 64 KB Secure SRAM
* 512 KB Global MRAM
* 512 KB Secure MRAM
* CLOCK
* Peripherals

    * :abbr:`32x GPIO (General Purpose Input Output)`
    * :abbr:`4x TWIM (I2C-compatible two-wire interface with MicroDMA)`
    * 4x I2S
    * :abbr:`7x SPI (Serial Peripheral Interface with MicroDMA)`
    * :abbr:`3x UART (Universal receiver-transmitter with MicroDMA)`
    * :abbr:`1x TSN (Time sensitive networking ethernet MAC with MicroDMA)`
    * 1x CAN-FD
    * 3x ADC
* Power section for on-board power generation and power measurement (selectable)

    * USB type-C
    * external 5V power source
* 40-pin JTAG connector (compatible to Olimex ARM-JTAG-OCD-H)
* USB over FTDI (connected to UART0)
* Header for all I/Os and configuration

* Assembly options for the SoC

  * SY120-GBM - Generic Base Module without top level sensors
  * SY120-GEN1 - Generic Module type 1 with top level sensors (see below)

The ``ganymed-bob/sy120-gen1`` comes with additional on-board sensors.

Supported Features
==================

.. zephyr:board-supported-hw::

For more detailed description please refer to `Ganymed BreakOut Board Documentation`_


Programming and Testing
***********************

.. zephyr:board-supported-runners::

Building
========

Applications for the ``ganymed_bob/sy120_gbm`` board can be
built and flashed in the usual way. See
:ref:`build_an_application` and :ref:`application_run` for more details on
building and running.

Building the :zephyr:code-sample:`hello_world` sample:

.. zephyr-app-commands::
   :tool: west
   :zephyr-app: samples/hello_world
   :board: ganymed_bob/sy120_gbm
   :goals: build
   :compact:


Flashing
========

Test the Ganymed with a :zephyr:code-sample:`hello_world` sample.

Flash the zephyr image:

.. zephyr-app-commands::
   :tool: west
   :zephyr-app: samples/hello_world
   :goals: flash
   :west-args: --serial /dev/ttyUSB0
   :compact:


Testing
=======

Then attach a serial console, ex. minicom / picocom / putty; Reset the target.
The sample output should be:

.. code-block:: console

    Hello World! ganymed_bob/sy120_gbm


References
**********

.. target-notes::

.. _`Ganymed BreakOut Board Documentation`: https://docs.sensry.net/datasheets/sy120-bob/
