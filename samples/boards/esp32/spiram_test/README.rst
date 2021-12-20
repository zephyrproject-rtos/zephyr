.. _spiram_test:

Espressif ESP32 SPIRAM test
###########################

Overview
********

This sample allocates memory from internal DRAM and SPIRAM by calling
:c:func:`k_malloc`, frees allocated memory by calling :c:func:`k_free` and
checks if memory can be allocated again. Capability of allocated memory is
decided by ESP_HEAP_MIN_EXTRAM_THRESHOLD. If size is less than
ESP_HEAP_MIN_EXTRAM_THRESHOLD, memory is allocated from internal DRAM. If
size is greater than ESP_HEAP_MIN_EXTRAM_THRESHOLD, memory is allocated from
SPIRAM.

Supported SoCs
**************

The following SoCs are supported by this sample code so far:

* ESP32
* ESP32-S2

Building and Running
********************

Make sure you have your board connected over USB port.

.. code-block:: console

   west build -b esp32 samples/boards/esp32/spiram_test
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

    mem test ok! 209
    SPIRAM mem test pass
    mem test ok! 194
    Internal mem test pass
