.. zephyr:board:: nrf9161dk

Overview
********

The nRF9161 DK (PCA10153) is a single-board development kit for evaluation and
development on the nRF9161 SiP for DECT NR+ and LTE-M/NB-IoT with GNSS. The ``nrf9161dk/nrf9161``
board configuration provides support for the Nordic Semiconductor nRF9161 ARM
Cortex-M33F CPU with ARMv8-M Security Extension and the following devices:

* :abbr:`ADC (Analog to Digital Converter)`
* CLOCK
* FLASH
* :abbr:`GPIO (General Purpose Input Output)`
* :abbr:`I2C (Inter-Integrated Circuit)`
* :abbr:`MPU (Memory Protection Unit)`
* :abbr:`NVIC (Nested Vectored Interrupt Controller)`
* :abbr:`PWM (Pulse Width Modulation)`
* :abbr:`RTC (nRF RTC System Clock)`
* Segger RTT (RTT Console)
* :abbr:`SPI (Serial Peripheral Interface)`
* :abbr:`UARTE (Universal asynchronous receiver-transmitter with EasyDMA)`
* :abbr:`WDT (Watchdog Timer)`
* :abbr:`IDAU (Implementation Defined Attribution Unit)`

More information about the board can be found at the
`nRF9161 DK website`_. `nRF9161 Product Specification`_
contains the processor's information and the datasheet.


Hardware
********

nRF9161 DK has two external oscillators. The frequency of
the slow clock is 32.768 kHz. The frequency of the main clock
is 32 MHz.

Supported Features
==================

.. zephyr:board-supported-hw::

.. _nrf9161dk_additional_hardware:

See `nRF9161 DK website`_ and `nRF9161 Product Specification`_
for a complete list of nRF9161 DK board hardware features.

Connections and IOs
===================

LED
---

* LED1 (green) = P0.0
* LED2 (green) = P0.1
* LED3 (green) = P0.4
* LED4 (green) = P0.5

Push buttons and Switches
-------------------------

* BUTTON1 = P0.8
* BUTTON2 = P0.9
* SWITCH1 = P0.18
* SWITCH2 = P0.19
* BOOT = SW5 = boot/reset

Security components
===================

- Implementation Defined Attribution Unit (`IDAU`_).  The IDAU is implemented
  with the System Protection Unit and is used to define secure and non-secure
  memory maps.  By default, all of the memory space  (Flash, SRAM, and
  peripheral address space) is defined to be secure accessible only.
- Secure boot.


Programming and Debugging
*************************

.. zephyr:board-supported-runners::

``nrf9161dk/nrf9161`` supports the Armv8m Security Extension, and by default boots
in the Secure state.

Building Secure/Non-Secure Zephyr applications with Arm |reg| TrustZone |reg|
=============================================================================

The process requires the following steps:

1. Build the Secure Zephyr application using ``-DBOARD=nrf9161dk/nrf9161`` and
   ``CONFIG_TRUSTED_EXECUTION_SECURE=y`` in the application project configuration file.
2. Build the Non-Secure Zephyr application using ``-DBOARD=nrf9161dk/nrf9161/ns``.
3. Merge the two binaries together.

When building a Secure/Non-Secure application, the Secure application will
have to set the IDAU (SPU) configuration to allow Non-Secure access to all
CPU resources utilized by the Non-Secure application firmware. SPU
configuration shall take place before jumping to the Non-Secure application.

Building a Secure only application
==================================

Build the Zephyr app in the usual way (see :ref:`build_an_application`
and :ref:`application_run`), using ``-DBOARD=nrf9161dk/nrf9161``.

Flashing
========

Follow the instructions in the :ref:`nordic_segger` page to install
and configure all the necessary software. Further information can be
found in :ref:`nordic_segger_flashing`. Then build and flash
applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Here is an example for the :zephyr:code-sample:`hello_world` application.

First, run your favorite terminal program to listen for output.

.. code-block:: console

   $ minicom -D <tty_device> -b 115200

Replace :code:`<tty_device>` with the port where the nRF9161 DK
can be found. For example, under Linux, :code:`/dev/ttyACM0`.

Then build and flash the application in the usual way.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: nrf9161dk/nrf9161
   :goals: build flash

Debugging
=========

Refer to the :ref:`nordic_segger` page to learn about debugging Nordic boards with a
Segger IC.


Testing the LEDs and buttons in the nRF9161 DK
**********************************************

There are 2 samples that allow you to test that the buttons (switches) and LEDs on
the board are working properly with Zephyr:

* :zephyr:code-sample:`blinky`
* :zephyr:code-sample:`button`

You can build and flash the examples to make sure Zephyr is running correctly on
your board. The button and LED definitions can be found in
:zephyr_file:`boards/nordic/nrf9161dk/nrf9161dk_nrf9161_common.dtsi`.

References
**********

.. target-notes::

.. _IDAU:
   https://developer.arm.com/docs/100690/latest/attribution-units-sau-and-idau
.. _nRF9161 DK website: https://www.nordicsemi.com/Software-and-Tools/Development-Kits/nRF9161-DK
.. _nRF9161 Product Specification: https://docs.nordicsemi.com/bundle/ps_nrf9161/page/nRF9161_html5_keyfeatures.html
.. _Trusted Firmware M: https://www.trustedfirmware.org/projects/tf-m/
