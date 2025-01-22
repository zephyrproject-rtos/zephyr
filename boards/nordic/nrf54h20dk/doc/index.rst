.. _nrf54h20dk_nrf54h20:

nRF54H20 DK
###########

Overview
********

.. note::

   All software for the nRF54H20 SoC is experimental and hardware availability
   is restricted to the participants in the limited sampling program.

The nRF54H20 DK is a single-board development kit for evaluation and development
on the Nordic nRF54H20 System-on-Chip (SoC).

The nRF54H20 is a multicore SoC with:

* an Arm Cortex-M33 core with DSP instructions, FPU, and Armv8-M Security
  Extensions, running at up to 320 MHz, referred to as the **application core**
* an Arm Cortex-M33 core with DSP instructions, FPU, and Armv8-M Security
  Extensions, running at up to 256 MHz, referred to as the **radio core**.
* a Nordic VPR RISC-V core, referred to as the **ppr core** (Peripheral
  Processor).

The ``nrf54h20dk/nrf54h20/cpuapp`` build target provides support for
the application core on the nRF54H20 SoC.
The ``nrf54h20dk/nrf54h20/cpurad`` build target provides support for
the radio core on the nRF54H20 SoC.
The ``nrf54h20dk/nrf54h20/cpuppr`` build target provides support for
the PPR core on the nRF54H20 SoC executing from RAM.
The ``nrf54h20dk/nrf54h20/cpuppr/xip`` build target provides support for
the PPR core on the nRF54H20 SoC executing from MRAM.

nRF54H20 SoC provides support for the following devices:

* :abbr:`ADC (Analog to Digital Converter)`
* CLOCK
* :abbr:`GPIO (General Purpose Input Output)`
* :abbr:`GRTC (Global real-time counter)`
* :abbr:`I2C (Inter-Integrated Circuit)`
* MEMCONF
* MRAM
* :abbr:`PWM (Pulse Width Modulation)`
* RADIO (Bluetooth Low Energy and 802.15.4)
* :abbr:`SPI (Serial Peripheral Interface)`
* :abbr:`UART (Universal asynchronous receiver-transmitter)`
* :abbr:`USB (Universal Serial Bus)`
* :abbr:`WDT (Watchdog Timer)`

Hardware
********

nRF54H20 DK has two crystal oscillators:

* High-frequency 32 MHz crystal oscillator (HFXO)
* Low-frequency 32.768 kHz crystal oscillator (LFXO)

Supported Features
==================

The ``nrf54h20dk/nrf54h20/cpuapp`` board configuration supports the following
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
| MEMCONF   | on-chip    | retained_mem         |
+-----------+------------+----------------------+
| SPI(M/S)  | on-chip    | spi                  |
+-----------+------------+----------------------+
| UART      | on-chip    | serial               |
+-----------+------------+----------------------+
| WDT       | on-chip    | watchdog             |
+-----------+------------+----------------------+

The ``nrf54h20dk/nrf54h20/cpurad`` board configuration supports the following
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
| MEMCONF   | on-chip    | retained_mem         |
+-----------+------------+----------------------+
| SPI(M/S)  | on-chip    | spi                  |
+-----------+------------+----------------------+
| UART      | on-chip    | serial               |
+-----------+------------+----------------------+
| WDT       | on-chip    | watchdog             |
+-----------+------------+----------------------+

Other hardware features have not been enabled yet for this board.

Connections and IOs
===================

LEDs
----

* LED1 (green) = P9.0
* LED2 (green) = P9.1
* LED3 (green) = P9.2
* LED4 (green) = P9.3

Push buttons
------------

* BUTTON1 = P0.8
* BUTTON2 = P0.9
* BUTTON3 = P0.10
* BUTTON4 = P0.11
* RESET (SW1)

Programming and Debugging
*************************

.. note::
   When first using the nRF54H20 DK, you must program the `nRF54H20 SoC binaries`_ on the development kit.
   To do so, follow the bring up steps instructions on the `Getting started with the nRF54H20 DK`_ documentation.

Applications for all targets can be built and flashed the usual way.
See :ref:`build_an_application` and :ref:`application_run` for more details on
building and running. Debugging is for now limited to the application and radio
cores only, using :ref:`nordic_segger`.

Flashing
========

As an example, this section shows how to build and flash the :zephyr:code-sample:`hello_world`
application.

To build and program the sample to the nRF54H20 DK, complete the following steps:

1. Connect the nRF54H20 DK to your computer using the IMCU USB port on the DK.
2. Install `nRF Util`_
#. Build the sample by running the following command:

   .. zephyr-app-commands::
      :zephyr-app: samples/hello_world
      :board: nrf54h20dk/nrf54h20/cpuapp
      :goals: build flash

Testing the LEDs and buttons in the nRF54H20 DK
***********************************************

There are 2 samples that allow you to test that the buttons (switches) and LEDs
on the board are working properly with Zephyr:

* :zephyr:code-sample:`blinky`
* :zephyr:code-sample:`button`

You can build and flash the examples to make sure Zephyr is running correctly on
your board. The button and LED definitions can be found in
:zephyr_file:`boards/nordic/nrf54h20dk/nrf54h20dk_nrf54h20_cpuapp.dts`.

.. _nRF Util:
   https://www.nordicsemi.com/Products/Development-tools/nrf-util

.. _Getting started with the nRF54H20 DK:
   https://docs.nordicsemi.com/bundle/ncs-latest/page/nrf/app_dev/device_guides/nrf54h/ug_nrf54h20_gs.html

.. _nRF54H20 SoC binaries:
   https://docs.nordicsemi.com/bundle/ncs-latest/page/nrf/releases_and_maturity/abi_compatibility.html
