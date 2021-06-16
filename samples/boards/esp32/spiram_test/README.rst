.. _spiram_test:

Espressif ESP32 SPIRAM test
###########################

Overview
********

This sample allocates memory from internal DRAM and SPIRAM by calling `k_malloc`, frees
allocated memory by calling `k_free` and checks if memory can be allocated again.
Capability of allocated memory is decided by ESP_HEAP_MIN_EXTRAM_THRESHOLD. If size is less than
ESP_HEAP_MIN_EXTRAM_THRESHOLD, memory is allocated from internal DRAM. If size is greater than
ESP_HEAP_MIN_EXTRAM_THRESHOLD, memory is allocated from SPIRAM.

Building and Running
********************

Make sure you have the ESP32_WROVER_KIT connected over USB port.

.. code-block:: console

   west build -b esp32 samples/boards/esp32/spiram_test
   west flash --esp-device /dev/ttyUSB0

Sample Output
=============

To check output of this sample, any serial console program can be used (i.e. on Linux minicom, putty, screen, etc)
This example uses ``picocom`` on the serial port ``/dev/ttyUS0``:

.. code-block:: console

    mem test ok! 209
    SPIRAM mem test pass
    mem test ok! 194
    Internal mem test pass
