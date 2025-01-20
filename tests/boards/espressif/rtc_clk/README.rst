.. _rtc_clk_test:

Espressif's RTC Clock test
##########################

Overview
********

This test iterates through all the clock sources and checks if the expected frequency is being set.
Espressif SOCs have 3 main clock subsystems: CPU, RTC_FAST and RTC_SLOW. Each of these subsystems have their own clock sources and possible rates.


Supported Boards
****************
- esp32_devkitc_wrover/esp32/procpu
- esp32c3_devkitm
- esp32c6_devkitc/esp32c6/hpcore
- esp32s2_saola
- esp32s3_devkitm/esp32s3/procpu

Building and Running
********************

Make sure you have the target connected over USB port.

.. code-block:: console

   west build -b <board> tests/boards/espressif/rtc_clk
   west flash && west espressif monitor

To run the test with twister, use the following command:

.. code-block:: console

   west twister -p <board> --device-testing --device-serial=/dev/ttyUSB0 -vv --flash-before -T tests/boards/espressif/rtc_clk

If the external 32K crystal is connect to pins 32K_XP and 32K_XN, the test can be run with ``external_xtal`` fixture enabled:

.. code-block:: console

	west twister -p esp32c3_devkitm --device-testing --device-serial=/dev/ttyUSB0 -vv --flash-before -T tests/boards/espressif/rtc_clk -X external_xtal

Sample Output
=============

.. code-block:: console

	*** Booting Zephyr OS build v3.6.0-3162-g529005998ea6 ***
   Running TESTSUITE rtc_clk
   ===================================================================
   CPU frequency: 240000000
   RTC_FAST frequency: 17500000
   RTC_SLOW frequency: 136000
   START - test_cpu_pll_src
   Testing CPU frequency: 80 MHz
   Testing CPU frequency: 160 MHz
   Testing CPU frequency: 240 MHz
   PASS - test_cpu_pll_src in 0.020 seconds
   ===================================================================
   START - test_cpu_xtal_src
   Testing CPU frequency: 40 MHz
   Testing CPU frequency: 20 MHz
   Testing CPU frequency: 10 MHz
   Testing CPU frequency: 5 MHz
   PASS - test_cpu_xtal_src in 17.645 seconds
   ===================================================================
   START - test_rtc_fast_src
   Testing RTC FAST CLK freq: 20000000 MHz
   Testing RTC FAST CLK freq: 17500000 MHz
   PASS - test_rtc_fast_src in 0.001 seconds
   ===================================================================
   START - test_rtc_slow_src
   Testing RTC SLOW CLK freq: 136000 MHz
   Testing RTC SLOW CLK freq: 68359 MHz
   PASS - test_rtc_slow_src in 0.002 seconds
   ===================================================================
   TESTSUITE rtc_clk succeeded
   ------ TESTSUITE SUMMARY START ------
   SUITE PASS - 100.00% [rtc_clk]: pass = 4, fail = 0, skip = 0, total = 4 duration = 17.668 seconds
   - PASS - [rtc_clk.test_cpu_pll_src] duration = 0.020 seconds
   - PASS - [rtc_clk.test_cpu_xtal_src] duration = 17.645 seconds
   - PASS - [rtc_clk.test_rtc_fast_src] duration = 0.001 seconds
   - PASS - [rtc_clk.test_rtc_slow_src] duration = 0.002 seconds
   ------ TESTSUITE SUMMARY END ------
   ===================================================================
   PROJECT EXECUTION SUCCESSFUL
