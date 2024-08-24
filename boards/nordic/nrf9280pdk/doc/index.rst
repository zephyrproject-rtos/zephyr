.. _nrf9280pdk_nrf9280:

nRF9280 PDK
###########

Overview
********

.. note::

   All software for the nRF9280 SiP is experimental and hardware availability
   is restricted to the participants in the limited sampling program.

The nRF9280 DK is a single-board development kit for evaluation and development
on the Nordic nRF9280 System-in-Package (SiP).

The nRF9280 is a multicore SiP with:

* an Arm Cortex-M33 core with DSP instructions, FPU, and Armv8-M Security
  Extensions, running at up to 320 MHz, referred to as the **application core**
* an Arm Cortex-M33 core with DSP instructions, FPU, and Armv8-M Security
  Extensions, running at up to 256 MHz, referred to as the **radio core**.

The ``nrf9280pdk/nrf9280/cpuapp`` board target provides support for
the application core on the nRF9280 SiP.
The ``nrf9280pdk/nrf9280/cpurad`` board target provides support for
the radio core on the nRF9280 SiP.
The ``nrf9280pdk/nrf9280/cpuppr`` board target provides support for
the PPR core on the nRF9280 SiP.

nRF9280 SiP provides support for the following devices:

* :abbr:`ADC (Analog to Digital Converter)`
* CLOCK
* :abbr:`GPIO (General Purpose Input Output)`
* :abbr:`GRTC (Global real-time counter)`
* :abbr:`I2C (Inter-Integrated Circuit)`
* MRAM
* :abbr:`PWM (Pulse Width Modulation)`
* RADIO (Bluetooth Low Energy and 802.15.4)
* :abbr:`SPI (Serial Peripheral Interface)`
* :abbr:`UART (Universal asynchronous receiver-transmitter)`
* :abbr:`USB (Universal Serial Bus)`
* :abbr:`WDT (Watchdog Timer)`

Hardware
********

nRF9280 DK has two crystal oscillators:

* High-frequency 32 MHz crystal oscillator (HFXO)
* Low-frequency 32.768 kHz crystal oscillator (LFXO)

Supported Features
==================

The ``nrf9280pdk/nrf9280/cpuapp`` board target supports the following
hardware features:

+-----------+------------+----------------------+
| Interface | Controller | Driver/Component     |
+===========+============+======================+
| GPIO      | on-chip    | gpio                 |
+-----------+------------+----------------------+
| GRTC      | on-chip    | system clock         |
+-----------+------------+----------------------+
| I2C(M)    | on-chip    | i2c                  |
+-----------+------------+----------------------+
| PWM       | on-chip    | pwm                  |
+-----------+------------+----------------------+
| SPI(M/S)  | on-chip    | spi                  |
+-----------+------------+----------------------+
| UART      | on-chip    | serial               |
+-----------+------------+----------------------+
| WDT       | on-chip    | watchdog             |
+-----------+------------+----------------------+

The ``nrf9280pdk/nrf9280/cpurad`` board target supports the following
hardware features:

+-----------+------------+----------------------+
| Interface | Controller | Driver/Component     |
+===========+============+======================+
| GPIO      | on-chip    | gpio                 |
+-----------+------------+----------------------+
| GRTC      | on-chip    | system clock         |
+-----------+------------+----------------------+
| I2C(M)    | on-chip    | i2c                  |
+-----------+------------+----------------------+
| SPI(M/S)  | on-chip    | spi                  |
+-----------+------------+----------------------+
| UART      | on-chip    | serial               |
+-----------+------------+----------------------+
| WDT       | on-chip    | watchdog             |
+-----------+------------+----------------------+

The ``nrf9280pdk/nrf9280/cpuppr`` board target supports the following
hardware features:

+-----------+------------+----------------------+
| Interface | Controller | Driver/Component     |
+===========+============+======================+
| GPIO      | on-chip    | gpio                 |
+-----------+------------+----------------------+
| GRTC      | on-chip    | system clock         |
+-----------+------------+----------------------+
| I2C(M)    | on-chip    | i2c                  |
+-----------+------------+----------------------+
| SPI(M/S)  | on-chip    | spi                  |
+-----------+------------+----------------------+
| PWM       | on-chip    | pwm                  |
+-----------+------------+----------------------+

Other hardware features have not been enabled yet for this board.

Connections and IOs
===================

LEDs
----

* LED1 (green) = P9.02
* LED2 (green) = P9.03
* LED3 (green) = P9.04
* LED4 (green) = P9.05

Push buttons
------------

* BUTTON1 = P0.8
* BUTTON2 = P0.9
* BUTTON3 = P0.10
* BUTTON4 = P0.11
* RESET (SW1)

Programming and Debugging
*************************

Applications for both the ``nrf9280pdk/nrf9280/cpuapp`` and
``nrf9280pdk/nrf9280/cpurad`` board targets can be built, flashed,
and debugged in the usual way. See :ref:`build_an_application`
and :ref:`application_run` for more details on building and running.

Flashing
========

As an example, this section shows how to build and flash the :ref:`hello_world`
application.

Follow the instructions in the :ref:`nordic_segger` page to install
and configure all the necessary software. Further information can be
found in :ref:`nordic_segger_flashing`.

To build and program the sample to the nRF9280 DK, complete the following steps:

1. Connect the nRF9280 DK to your computer using the IMCU USB port on the DK.
#. Build the sample by running the following command:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: nrf9280pdk/nrf9280/cpuapp
      :goals: build flash

Testing the LEDs and buttons in the nRF9280 DK
***********************************************

There are 2 samples that allow you to test that the buttons (switches) and LEDs
on the board are working properly with Zephyr:

* :zephyr:code-sample:`blinky`
* :zephyr:code-sample:`button`

You can build and flash the examples to make sure Zephyr is running correctly on
your board. The button and LED definitions can be found in
:zephyr_file:`boards/nordic/nrf9280pdk/nrf9280pdk_nrf9280_cpuapp.dts`.
