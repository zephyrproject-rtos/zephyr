.. _cache_coex_test:

Espressif ESP32 PSRAM/SPI Flash co-existence test
#################################################

Overview
********

This code tests SPI Flash and PSRAM content integrity after multithreaded and concurrent accesses to
a common cache. It does so by allocating a big PSRAM memory chunk and repeatedly filling that region
with a random generated pattern. At the same time, a whole SPI Flash page is updated with another pattern
value. By the end of the thread iterations, both PSRAM and SPI Flash have its contents compared against
expected values to check for integrity.

Supported Boards
****************
- esp32_devkitc_wrover
- esp32s2_saola
- esp32s3_devkitm

Building and Running
********************

Make sure you have the target connected over USB port.

.. code-block:: console

   west build -b <board> tests/boards/espressif_esp32/cache_coex
   west flash && west espressif monitor

Sample Output
=============

.. code-block:: console

	Running TESTSUITE cache_coex
	===================================================================
	START - test_flash_integrity
	PASS - test_flash_integrity in 0.001 seconds
	===================================================================
	START - test_ram_integrity
	PASS - test_ram_integrity in 0.001 seconds
	===================================================================
	START - test_using_spiram
	PASS - test_using_spiram in 0.001 seconds
	===================================================================
	TESTSUITE cache_coex succeeded
	------ TESTSUITE SUMMARY START ------
	SUITE PASS - 100.00% [cache_coex]: pass = 3, fail = 0, skip = 0, total = 3 duration = 0.003 seconds
	- PASS - [cache_coex.test_flash_integrity] duration = 0.001 seconds
	- PASS - [cache_coex.test_ram_integrity] duration = 0.001 seconds
	- PASS - [cache_coex.test_using_spiram] duration = 0.001 seconds
	------ TESTSUITE SUMMARY END ------
	===================================================================
	PROJECT EXECUTION SUCCESSFUL
