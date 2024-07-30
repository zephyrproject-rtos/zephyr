.. _xt_wdt_sample:

Espressif XT WDT Sample
#######################

Overview
********

This sample shows how to work with XTAL32K Watchdog Timer (XT WDT) peripheral.
To properly run this sample, you need to have a board with an external 32.728 Hz
crystal oscillator connected to the XTAL_32K_P and XTAL_32K_N pins.

The app will ask for the crystal removal to simulate a crystal failure and trigger
the XT WDT interrupt. Internally the hardware switch the RTC SLOW clock source from
ESP32_RTC_SLOW_CLK_SRC_XTAL32K to ESP32_RTC_SLOW_CLK_SRC_RC_SLOW (136 KHz for C3 and S3 /
9 KHz for S2).

Supported SoCs
**************

The following SoCs supports the XT WDT peripheral:

* ESP32-C3
* ESP32-S2
* ESP32-S3

Building and Running
********************

Make sure you have your board connected over USB port and the external crystal is connected
to pins XTAL_32K_P and XTAL_32K_N.

.. code-block:: console

   west build -p -b esp32s3_devkitm/esp32s3/procpu samples/boards/esp32/xt_wdt
   west flash

If using another supported Espressif board, replace the argument in the above
command with a proper board name (e.g., `esp32s2_saola`).

Sample Output
=============

To check output of this sample, run ``west espressif monitor`` or any other serial
console program (e.g., minicom, putty, screen, etc).
This example uses ``west espressif monitor``, which automatically detects the serial
port at ``/dev/ttyUSB0``:

.. code-block:: console

   $ west espressif monitor

.. code-block:: console

   *** Booting Zephyr OS build v3.6.0-3896-gb4a7f061524f ***
   [00:00:01.287,000] <inf> xt_wdt_sample: XT WDT Sample on esp32s3_devkitm/esp32s3/procpu
   [00:00:01.287,000] <inf> xt_wdt_sample: Current RTC SLOW clock rate: 32768 Hz
   [00:00:01.287,000] <inf> xt_wdt_sample: Remove the external 32K crystal to trigger the watchdog
   XT WDT callback
   [00:00:03.554,000] <inf> xt_wdt_sample: Current RTC SLOW clock rate: 136000 Hz
