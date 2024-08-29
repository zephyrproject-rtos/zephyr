.. _spiram_test:

Espressif ESP32 SPIRAM test
###########################

Overview
********

This sample shows how to allocate memory from SPIRAM by using
:c:func:`shared_multi_heap_aligned_alloc` with `SMH_REG_ATTR_EXTERNAL` attribute. Checks if the
memory was correctly allocated then frees it by calling :c:func:`shared_multi_heap_free`.
It also allocates memory from internal memory and checks if the address range is correct.

Supported SoCs
**************

The following SoCs are supported by this sample code so far:

* ESP32
* ESP32-S2
* ESP32-S3

Building and Running
********************

Make sure you have your board connected over USB port.

.. code-block:: console

   west build -b esp32s3_devkitm/esp32s3/procpu samples/boards/esp32/spiram_test
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

   *** Booting Zephyr OS build v3.7.0-446-g93c9da66944c ***
   SPIRAM mem test pass
   Internal mem test pass
