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

Building and Running
********************

Make sure you have the ESP32 DevKitC connected over USB port.

.. code-block:: console

   west build -b esp32 tests/boards/espressif_esp32/cache_coex
   west flash --esp-device /dev/ttyUSB0

Sample Output
=============

To check output of this test, any serial console program can be used (i.e. on Linux picocom, putty, screen, etc).
This test uses ``minicom`` on the serial port ``/dev/ttyUS0``. The following lines indicate a successful test:

.. code-block:: console

    Running test suite cache_coex_test
    ===================================================================
    START - flash_integrity_test
     PASS - flash_integrity_test in 0.1 seconds
    ===================================================================
    START - ram_integrity_test
     PASS - ram_integrity_test in 0.1 seconds
    ===================================================================
    Test suite cache_coex_test succeeded
    ===================================================================
    PROJECT EXECUTION SUCCESSFUL
