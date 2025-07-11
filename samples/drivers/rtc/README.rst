.. zephyr:code-sample:: rtc
   :name: Real-Time Clock (RTC)
   :relevant-api: rtc_interface

   Set and read the date/time from a Real-Time Clock.

Overview
********

This sample shows how to use the :ref:`rtc driver API <rtc_api>`
to set and read the date/time from RTC and display on the console
and can be built and executed on boards supporting RTC.

Building and Running
********************

Build and flash as follows, replacing ``stm32f3_disco`` with your board:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/rtc
   :board: stm32f3_disco
   :goals: build flash
   :compact:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/rtc
   :board: mimxrt700_evk/mimxrt798s/cm33_cpu0
   :goals: build flash

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/rtc
   :board: mimxrt700_evk/mimxrt798s/cm33_cpu1
   :goals: build ram

Sample Output
=============

.. code-block:: console

   RTC date and time: 2024-11-17 04:19:00
   RTC date and time: 2024-11-17 04:19:01

   <repeats endlessly>
