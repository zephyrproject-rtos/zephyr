X-NUCLEO-PGEEZ1 M95P32 extended operations
############################################

Overview
********

This sample demonstrates the M95P32-specific flash operations exposed through
the Flash API ``flash_ex_op()`` extension:

* Read the 1024-byte identification area with ``RDID`` (0x83).
* Write and read back a pattern in the customer identification page with
  ``WRID`` (0x82), and verify that a write crossing its boundary is rejected.
* Read across the factory/customer identification-page boundary.
* Compare standard page programming through ``flash_write()`` with ``PGPR``
  (0x0A) page program with buffer load over two independently erased 64-KiB
  main-array regions. The sample times only each write call, then reads back
  both regions to verify their contents.

The page-program-with-buffer-load operation enables and disables the M95P32
buffer mode internally using its volatile register. Direct volatile-register
access is not part of the public API.

.. warning::

   The sample modifies the customer identification page at offset 0x200 and
   erases the main-array regions from 0x3e0000 to 0x3fffff. Do not run it when
   this region contains data that must be preserved.

Building and Running
********************

Build the sample for a NUCLEO-U575ZI-Q with the X-NUCLEO-PGEEZ1 shield:

.. zephyr-app-commands::
   :zephyr-app: samples/shields/x_nucleo_pgeez1
   :board: nucleo_u575zi_q
   :shield: x_nucleo_pgeez1
   :goals: build flash
   :compact:

Sample Output
*************

.. code-block:: console

   [00:00:00.000,000] <inf> ospi_stm32: MSPI config result: success
   *** Booting Zephyr OS build v4.4.0-11617-g2189ec6da640 ***
   M95P32 extended operation test

   ==================== READ: Factory identification page ====================
   Identification page [0x000..0x00f]: 20 00 16 00 ff ff ff ff ff ff ff ff ff ff ff ff

   ==================== READ: Customer identification page before clear ====================
   Customer identification page before WRID [0x200..0x20f]: 4d 39 35 50 33 32 2d 65 78 2d 6f 70 2d 74 65 73

   ==================== WRITE: Clear customer identification page ====================
   Clearing customer identification page [0x200..0x20f]: ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff

   ==================== READ: Customer identification page after clear ====================
   Customer identification page after clear [0x200..0x20f]: ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff

   ==================== WRITE: Customer identification page ====================
   Writing customer identification page [0x200..0x20f]: 4d 39 35 50 33 32 2d 65 78 2d 6f 70 2d 74 65 73

   ==================== READ: Customer identification page after WRID ====================
   Customer identification page after WRID [0x200..0x20f]: 4d 39 35 50 33 32 2d 65 78 2d 6f 70 2d 74 65 73

   ==================== READ: Identification page boundary ====================
   Identification page boundary [0x1f8..0x207]: ff ff ff ff ff ff ff ff 4d 39 35 50 33 32 2d 65

   ==================== WRITE: Cross-page protection ====================
   Cross-page WRID protection passed

   Identification page read/write passed

   ==================== PREPARE: Erase write benchmark regions ====================

   ==================== WRITE: Standard page program ====================
   Standard page program data verification passed

   ==================== WRITE: Page program with buffer load ====================
   Page program with buffer load data verification passed

   ==================== RESULT: Write performance ====================
   Standard page program (65536 bytes): 21752449 cycles, 0.135952 s
   Page program with buffer load (65536 bytes): 17151523 cycles, 0.107197 s
   Page-program-with-buffer-load speedup: 1.26x (21.1% less write time)
   M95P32 extended operation test passed
