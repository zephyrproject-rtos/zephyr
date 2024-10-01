.. zephyr:code-sample:: esp32-flash-memory-mapped
   :name: Memory-Mapped Flash
   :relevant-api: flash_interface

   Write data into scratch area and read it using flash API and memory-mapped pointer.

Overview
********

ESP32 features memory hardware which allows regions of flash memory to be mapped into instruction
and data address spaces. This mapping works only for read operations. It is not possible to modify
contents of flash memory by writing to a mapped memory region.

Mapping happens in 64 KB pages. Memory mapping hardware can map flash into the data address space
and the instruction address space. See the technical reference manual for more details and
limitations about memory mapping hardware. For more information, check `ESP32 Flash Memory-Mapping`_.

Supported SoCs
**************

All ESP32 SoCs support flash memory-mapped feature.

Building and Running
********************

Make sure you have your board connected over USB port.

.. code-block:: console

   west build -b esp32s3_devkitm samples/boards/espressif/flash_memory_mapped
   west flash

Sample Output
=============

To check the output of this sample, run ``west espressif monitor`` or any other serial
console program (e.g., minicom, putty, screen, etc).
This example uses ``west espressif monitor``, which automatically detects the serial
port at ``/dev/ttyUSB0``:

.. code-block:: console

   $ west espressif monitor

The sample code erases the scratch area defined in DTS file and writes a 32-bytes data buffer in it.
Next, it prints that region content using flash API read and also using memory-mapped pointer.
Both readings should return the same value. Important to notice that writing using memory-mapped pointer
is not allowed.


.. code-block:: console

  *** Booting Zephyr OS build v3.5.0-rc3-10-g3118724fa268 ***
  [00:00:00.255,000] <inf> flash_memory_mapped: memory-mapped pointer address: 0x3c040000
  [00:00:01.122,000] <inf> flash_memory_mapped: flash read using memory-mapped pointer
                                                ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff |........ ........
                                                ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff |........ ........
  [00:00:01.122,000] <inf> flash_memory_mapped: writing 32-bytes data using flash API
  [00:00:01.122,000] <inf> flash_memory_mapped: flash read using flash API
                                                00 01 02 03 04 05 06 07  08 09 0a 0b 0c 0d 0e 0f |........ ........
                                                10 11 12 13 14 15 16 17  18 19 1a 1b 1c 1d 1e 1f |........ ........
  [00:00:01.122,000] <inf> flash_memory_mapped: flash read using memory-mapped pointer
                                                00 01 02 03 04 05 06 07  08 09 0a 0b 0c 0d 0e 0f |........ ........
                                                10 11 12 13 14 15 16 17  18 19 1a 1b 1c 1d 1e 1f |........ ........

.. _ESP32 Flash Memory-Mapping:
   https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/spi_flash/index.html#memory-mapping-api
