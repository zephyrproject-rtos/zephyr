.. zephyr:code-sample:: dac
   :name: Digital-to-Analog Converter (DAC)
   :relevant-api: dac_interface

   Generate an analog sawtooth signal using the DAC driver API.

Overview
********

This sample demonstrates how to use the :ref:`DAC driver API <dac_api>`.

Building and Running
********************

The DAC output is defined in the board's devicetree and pinmux file.

The board's :ref:`/zephyr,user <dt-zephyr-user>` node must have ``dac``,
``dac-channel-id``, and ``dac-resolution`` properties set. See the predefined
overlays in :zephyr_file:`samples/drivers/dac/boards` for examples.

Building and Running for ST Nucleo L073RZ
=========================================
The sample can be built and executed for the
:zephyr:board:`nucleo_l073rz` as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/dac
   :board: nucleo_l073rz
   :goals: build flash
   :compact:

Building and Running for ST Nucleo L152RE
=========================================
The sample can be built and executed for the
:zephyr:board:`nucleo_l152re` as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/dac
   :board: nucleo_l152re
   :goals: build flash
   :compact:

Building and Running for ST Nucleo F767ZI
=========================================
The sample can be built and executed for the
:zephyr:board:`nucleo_f767zi` as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/dac
   :board: nucleo_f767zi
   :goals: build flash
   :compact:

Building and Running for ST Disco F3
=========================================
The sample can be built and executed for the
:zephyr:board:`stm32f3_disco` as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/dac
   :board: stm32f3_disco
   :goals: build flash
   :compact:

Building and Running for ST Nucleo F429ZI
=========================================
The sample can be built and executed for the
:zephyr:board:`nucleo_f429zi` as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/dac
   :board: nucleo_f429zi
   :goals: build flash
   :compact:

Building and Running for STM32L562E DK
======================================
The sample can be built and executed for the
:zephyr:board:`stm32l562e_dk` as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/dac
   :board: stm32l562e_dk
   :goals: build flash
   :compact:

Building and Running for ST Nucleo L552ZE Q
===========================================
The sample can be built and executed for the
:zephyr:board:`nucleo_l552ze_q` as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/dac
   :board: nucleo_l552ze_q
   :goals: build flash
   :compact:

Building and Running for NXP TWR-KE18F
======================================
The sample can be built and executed for the :zephyr:board:`twr_ke18f` as
follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/dac
   :board: twr_ke18f
   :goals: build flash
   :compact:

DAC output is available on pin A32 of the primary TWR elevator
connector.

Building and Running for NXP FRDM-K64F
======================================
The sample can be built and executed for the :zephyr:board:`frdm_k64f` as
follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/dac
   :board: frdm_k64f
   :goals: build flash
   :compact:

DAC output is available on connector J4 pin 11.

Building and Running for BL652
==============================
The BL652 DVK PCB contains a footprint for a MCP4725 to use as an external
DAC. Note this is not populated by default. The sample can be built and
executed for the :zephyr:board:`bl652_dvk` as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/dac
   :board: bl652_dvk
   :goals: build flash
   :compact:

DAC output is available on pin 1 of the MCP4725.

Building and Running for BL653
==============================
The BL653 DVK PCB contains a footprint for a MCP4725 to use as an external
DAC. Note this is not populated by default. The sample can be built and
executed for the :zephyr:board:`bl653_dvk` as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/dac
   :board: bl653_dvk
   :goals: build flash
   :compact:

DAC output is available on pin 1 of the MCP4725.

Building and Running for BL654
==============================
The BL654 DVK PCB contains a footprint for a MCP4725 to use as an external
DAC. Note this is not populated by default. The sample can be built and
executed for the :zephyr:board:`bl654_dvk` as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/dac
   :board: bl654_dvk
   :goals: build flash
   :compact:

DAC output is available on pin 1 of the MCP4725.

Building and Running for BL5340
===============================
The BL5340 DVK PCB contains a MCP4725 to use as a DAC. The sample can be
built and executed for the :zephyr:board:`bl5340_dvk` as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/dac
   :board: bl5340_dvk/nrf5340/cpuapp
   :goals: build flash
   :compact:

DAC output is available on pin 1 of the MCP4725.

Building and Running for GD32450I-EVAL
======================================
The sample can be built and executed for the
:zephyr:board:`gd32f450i_eval` as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/dac
   :board: gd32f450i_eval
   :goals: build flash
   :compact:

Bridge the JP23 to DAC with the jumper cap, then DAC output will available on JP7.

Building and Running for Longan Nano and Longan Nano Lite
=========================================================
The sample can be built and executed for the
:zephyr:board:`longan_nano` as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/dac
   :board: longan_nano
   :goals: build flash
   :compact:

also can run for the Longan Nano Lite as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/dac
   :board: longan_nano/gd32vf103/lite
   :goals: build flash
   :compact:

Building and Running for NXP LPCXpresso55S36
============================================
The sample can be built and executed for the :zephyr:board:`lpcxpresso55s36` as
follows:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/dac
   :board: lpcxpresso55s36
   :goals: build flash
   :compact:

DAC output is available on connector J12 pin 4.

Sample output
=============

You should see a sawtooth signal with an amplitude of the DAC reference
voltage and a period of approx. 4 seconds at the DAC output pin specified
by the board.

The following output is printed:

.. code-block:: console

   Generating sawtooth signal at DAC channel 1.

.. note:: If the DAC is not supported, the output will be an error message.
