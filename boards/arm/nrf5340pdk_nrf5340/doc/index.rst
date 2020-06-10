.. _nrf5340pdk_nrf5340:

nRF5340 PDK
###########

Overview
********

The nRF5340 PDK (PCA10095) is a single-board development kit for evaluation
and development on the Nordic nRF5340 System-on-Chip (SoC).

The nRF5340 is a dual-core SoC based on the Arm® Cortex®-M33 architecture, with:

* a full-featured ARM Cortex-M33F core with DSP instructions, FPU, and
  ARMv8-M Security Extension, running at up to 128 MHz, referred to as
  the **Application MCU**
* a secondary ARM Cortex-M33 core, with a reduced feature set, running at
  a fixed 64 MHz, referred to as the **Network MCU**.

The nrf5340pdk_nrf5340_cpuapp provides support for the Application MCU on
nRF5340 SoC. The nrf5340pdk_nrf5340_cpunet provides support for the Network
MCU on nRF5340 SoC.

nRF5340 SoC provides support for the following devices:

* :abbr:`ADC (Analog to Digital Converter)`
* CLOCK
* FLASH
* :abbr:`GPIO (General Purpose Input Output)`
* :abbr:`IDAU (Implementation Defined Attribution Unit)`
* :abbr:`I2C (Inter-Integrated Circuit)`
* :abbr:`MPU (Memory Protection Unit)`
* :abbr:`NVIC (Nested Vectored Interrupt Controller)`
* :abbr:`PWM (Pulse Width Modulation)`
* RADIO (Bluetooth Low Energy and 802.15.4)
* :abbr:`RTC (nRF RTC System Clock)`
* Segger RTT (RTT Console)
* :abbr:`SPI (Serial Peripheral Interface)`
* :abbr:`UARTE (Universal asynchronous receiver-transmitter)`
* :abbr:`USB (Universal Serial Bus)`
* :abbr:`WDT (Watchdog Timer)`

.. figure:: img/nrf5340pdk.png
     :width: 711px
     :align: center
     :alt: nRF5340 PDK

     nRF5340 PDK (Credit: Nordic Semiconductor)

More information about the board can be found at the
`nRF5340 PDK website`_.
The `Nordic Semiconductor Infocenter`_
contains the processor's information and the datasheet.

.. note::

   In previous Zephyr releases this board was named *nrf5340_dk_nrf5340*.

Hardware
********

nRF5340 PDK has two external oscillators. The frequency of
the slow clock is 32.768 kHz. The frequency of the main clock
is 32 MHz.

Supported Features
==================

The nrf5340pdk_nrf5340_cpuapp board configuration supports the following
hardware features:

+-----------+------------+----------------------+
| Interface | Controller | Driver/Component     |
+===========+============+======================+
| ADC       | on-chip    | adc                  |
+-----------+------------+----------------------+
| CLOCK     | on-chip    | clock_control        |
+-----------+------------+----------------------+
| FLASH     | on-chip    | flash                |
+-----------+------------+----------------------+
| GPIO      | on-chip    | gpio                 |
+-----------+------------+----------------------+
| I2C(M)    | on-chip    | i2c                  |
+-----------+------------+----------------------+
| MPU       | on-chip    | arch/arm             |
+-----------+------------+----------------------+
| NVIC      | on-chip    | arch/arm             |
+-----------+------------+----------------------+
| PWM       | on-chip    | pwm                  |
+-----------+------------+----------------------+
| RTC       | on-chip    | system clock         |
+-----------+------------+----------------------+
| RTT       | Segger     | console              |
+-----------+------------+----------------------+
| SPI(M/S)  | on-chip    | spi                  |
+-----------+------------+----------------------+
| SPU       | on-chip    | system protection    |
+-----------+------------+----------------------+
| UARTE     | on-chip    | serial               |
+-----------+------------+----------------------+
| USB       | on-chip    | usb                  |
+-----------+------------+----------------------+
| WDT       | on-chip    | watchdog             |
+-----------+------------+----------------------+

The nrf5340pdk_nrf5340_cpunet board configuration supports the following
hardware features:

+-----------+------------+----------------------+
| Interface | Controller | Driver/Component     |
+===========+============+======================+
| ADC       | on-chip    | adc                  |
+-----------+------------+----------------------+
| CLOCK     | on-chip    | clock_control        |
+-----------+------------+----------------------+
| FLASH     | on-chip    | flash                |
+-----------+------------+----------------------+
| GPIO      | on-chip    | gpio                 |
+-----------+------------+----------------------+
| I2C(M)    | on-chip    | i2c                  |
+-----------+------------+----------------------+
| MPU       | on-chip    | arch/arm             |
+-----------+------------+----------------------+
| NVIC      | on-chip    | arch/arm             |
+-----------+------------+----------------------+
| PWM       | on-chip    | pwm                  |
+-----------+------------+----------------------+
| RADIO     | on-chip    | Bluetooth,           |
|           |            | ieee802154           |
+-----------+------------+----------------------+
| RTC       | on-chip    | system clock         |
+-----------+------------+----------------------+
| RTT       | Segger     | console              |
+-----------+------------+----------------------+
| SPI(M/S)  | on-chip    | spi                  |
+-----------+------------+----------------------+
| UARTE     | on-chip    | serial               |
+-----------+------------+----------------------+
| WDT       | on-chip    | watchdog             |
+-----------+------------+----------------------+

Other hardware features are not supported by the Zephyr kernel.
See `Nordic Semiconductor Infocenter`_
for a complete list of nRF5340 Development Kit board hardware features.

Connections and IOs
===================

LED
---

* LED1 (green) = P0.28
* LED2 (green) = P0.29
* LED3 (green) = P0.30
* LED4 (green) = P0.31

Push buttons
------------

* BUTTON1 = SW1 = P0.23
* BUTTON2 = SW2 = P0.24
* BUTTON3 = SW3 = P0.8
* BUTTON4 = SW4 = P0.9
* BOOT = SW5 = boot/reset

Security components
===================

- Implementation Defined Attribution Unit (`IDAU`_) on the Application MCU.
  The IDAU is implemented with the System Protection Unit and is used to
  define secure and non-secure memory maps.  By default, all of the memory
  space  (Flash, SRAM, and peripheral address space) is defined to be secure
  accessible only.
- Secure boot.

Programming and Debugging
*************************

nRF5340 Application MCU supports the Armv8m Security Extension.
Applications build for the nrf5340pdk_nrf5340_cpuapp board by default
boot in the Secure state.

nRF5340 Network MCU does not support the Armv8m Security Extension.
nRF5340 IDAU may configure bus accesses by the nRF5340 Network MCU
to have Secure attribute set; the latter allows to build and run
Secure only applications on the nRF5340 SoC.

Building Secure/Non-Secure Zephyr applications
==============================================

The process requires the following steps:

1. Build the Secure Zephyr application for the Application MCU
   using ``-DBOARD=nrf5340pdk_nrf5340_cpuapp`` and
   ``CONFIG_TRUSTED_EXECUTION_SECURE=y`` in the application
   project configuration file.
2. Build the Non-Secure Zephyr application for the Application MCU
   using ``-DBOARD=nrf5340pdk_nrf5340_cpuappns``.
3. Merge the two binaries together.
4. Build the application firmware for the Network MCU using
   ``-DBOARD=nrf5340pdk_nrf5340_cpunet``.

When building a Secure/Non-Secure application for the nRF5340 Application MCU,
the Secure application will have to set the IDAU (SPU) configuration to allow
Non-Secure access to all CPU resources utilized by the Non-Secure application
firmware. SPU configuration shall take place before jumping to the Non-Secure
application.

Building a Secure only application
==================================

Build the Zephyr app in the usual way (see :ref:`build_an_application`
and :ref:`application_run`), using ``-DBOARD=nrf5340pdk_nrf5340_cpuapp`` for
the firmware running on the nRF5340 Application MCU, and using
``-DBOARD=nrf5340pdk_nrf5340_cpunet`` for the firmware running
on the nRF5340 Network MCU.

Flashing
========

Follow the instructions in the :ref:`nordic_segger` page to install
and configure all the necessary software. Further information can be
found in :ref:`nordic_segger_flashing`. Then build and flash
applications as usual (see :ref:`build_an_application` and
:ref:`application_run` for more details).

Here is an example for the :ref:`hello_world` application running on the
nRF5340 Application MCU.

First, run your favorite terminal program to listen for output.

.. code-block:: console

   $ minicom -D <tty_device> -b 115200

Replace :code:`<tty_device>` with the port where the board nRF5340 PDK
can be found. For example, under Linux, :code:`/dev/ttyACM0`.

Then build and flash the application in the usual way.

.. zephyr-app-commands::
   :zephyr-app: samples/hello_world
   :board: nrf5340pdk_nrf5340_cpuapp
   :goals: build flash

Debugging
=========

Refer to the :ref:`nordic_segger` page to learn about debugging Nordic
boards with a Segger IC.


Testing the LEDs and buttons in the nRF5340 PDK
***********************************************

There are 2 samples that allow you to test that the buttons (switches) and
LEDs on the board are working properly with Zephyr:

* :ref:`blinky-sample`
* :ref:`button-sample`

You can build and flash the examples to make sure Zephyr is running correctly on
your board. The button and LED definitions can be found in
:zephyr_file:`boards/arm/nrf5340pdk_nrf5340/nrf5340_cpuapp_common.dts`.

References
**********

.. target-notes::

.. _IDAU:
   https://developer.arm.com/docs/100690/latest/attribution-units-sau-and-idau
.. _nRF5340 PDK website:
   https://www.nordicsemi.com/Software-and-tools/Development-Kits/nRF5340-PDK
.. _Nordic Semiconductor Infocenter: https://infocenter.nordicsemi.com
