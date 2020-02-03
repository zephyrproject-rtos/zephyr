.. _Adc-battery-sample:

Measure battery voltage application
###################################

Overview
********

This application show how to configure and read ADC driver with NRF52 chip
to measure battery voltage.

Requirements
************

NRF52840 compatible board.

Building and Running
********************

This application was developed and tested with NRF52840 Dev board. But probably
will work with others as well.

.. zephyr-app-commands::
   :zephyr-app: samples/boards/nrf/adc_battery
   :board: nrf52840_pca10056
   :goals: build flash
   :compact:


Sample Output
=============

.. code-block:: console

        SEGGER J-Link V6.34b - Real time terminal output
        J-Link OB-SAM3U128-V2-NordicSemi compiled Jan  7 2019 14:07:15 V1.0, SN=683400448
        Process: JLinkExe
        *** Booting Zephyr OS build zephyr-v2.1.0-1628-gb45b1e393c95  ***
        Adc batterty sample
        ADC value: 3412 mV
        ADC value: 3422 mV
        ADC value: 3424 mV
        ADC value: 3414 mV
        ADC value: 3414 mV
        ADC value: 3413 mV
