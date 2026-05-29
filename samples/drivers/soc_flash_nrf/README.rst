.. zephyr:code-sample:: soc-flash-nrf
   :name: nRF SoC Internal Storage
   :relevant-api: flash_interface flash_area_api

   Use the flash API to interact with the SoC flash.

Overview
********

This sample demonstrates using the :ref:`Flash API <flash_api>` on an SoC internal storage.
The sample uses :ref:`Flash map API <flash_map_api>` to obtain a device that has one
partition defined with the label ``storage_partition``, then uses :ref:`Flash API <flash_api>`
to directly access and modify the contents of a device within the area defined for said
partition.

Within the sample, user may observe how read/write/erase operations
are performed on a device, and how to first check whether device is
ready for operation.

Building and Running
********************

The sample will be built for any SoC with internal storage, as long as
there is a fixed-partition named ``storage_partition`` defined
on that internal storage.

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/soc_flash_nrf
   :board: nrf52840dk/nrf52840
   :goals: build flash
   :compact:

Sample Output
=============

.. code-block:: console

   *** Booting Zephyr OS build v2.7.99-17621-g54832687bcbb ***

   Nordic nRF5 Internal Storage Sample
   ===================================

   Test 1: Flash erase page at 0x82000
      Flash erase succeeded!

   Test 2: Flash write (word array 1)
      Attempted to write 1122 at 0x82000
      Attempted to read 0x82000
      Data read: 1122
      Data read matches data written. Good!
      Attempted to write aabb at 0x82004
      Attempted to read 0x82004
      Data read: aabb
      Data read matches data written. Good!
      Attempted to write abcd at 0x82008
      Attempted to read 0x82008
      Data read: abcd
      Data read matches data written. Good!
      Attempted to write 1234 at 0x8200c
      Attempted to read 0x8200c
      Data read: 1234
      Data read matches data written. Good!

   Test 3: Flash erase (2 pages at 0x80000)
      Flash erase succeeded!

   Test 4: Flash write (word array 2)
      Attempted to write 1234 at 0x82000
      Attempted to read 0x82000
      Data read: 1234
      Data read matches data written. Good!
      Attempted to write aabb at 0x82004
      Attempted to read 0x82004
      Data read: aabb
      Data read matches data written. Good!
      Attempted to write abcd at 0x82008
      Attempted to read 0x82008
      Data read: abcd
      Data read matches data written. Good!
      Attempted to write 1122 at 0x8200c
      Attempted to read 0x8200c
      Data read: 1122
      Data read matches data written. Good!

   Test 5: Flash erase page at 0x82000
      Flash erase succeeded!

   Test 6: Non-word aligned write (word array 3)
      Attempted to write 1122 at 0x82001
      Attempted to read 0x82001
      Data read: 1122
      Data read matches data written. Good!
      Attempted to write aabb at 0x82005
      Attempted to read 0x82005
      Data read: aabb
      Data read matches data written. Good!
      Attempted to write abcd at 0x82009
      Attempted to read 0x82009
      Data read: abcd
      Data read matches data written. Good!
      Attempted to write 1234 at 0x8200d
      Attempted to read 0x8200d
      Data read: 1234
      Data read matches data written. Good!
      Attempted to write 1122 at 0x82011
      Attempted to read 0x82011
      Data read: 1122
      Data read matches data written. Good!
      Attempted to write aabb at 0x82015
      Attempted to read 0x82015
      Data read: aabb
      Data read matches data written. Good!
      Attempted to write abcd at 0x82019
      Attempted to read 0x82019
      Data read: abcd
      Data read matches data written. Good!
      Attempted to write 1234 at 0x8201d
      Attempted to read 0x8201d
      Data read: 1234
      Data read matches data written. Good!

   Test 7: Page layout API
      Offset  0x00041234:
        belongs to the page 65 of start offset 0x00041000
        and the size of 0x00001000 B.
      Page of number 37 has start offset 0x00025000
        and size of 0x00001000 B.
        Page index resolved properly
      SoC flash consists of 256 pages.

   Test 8: Write block size API
      write-block-size = 1

   Finished!
