.. _mpu_test:

Memory Protection Unit (MPU) TEST
#################################

Overview
********
This test provides a set options to check the correct MPU configuration
against the following security issues:

* Read at an address that is reserved in the memory map.
* Write into the boot Flash/ROM.
* Run code located in SRAM.

If the MPU configuration is correct each option selected ends up in an MPU
fault.

Building and Running
********************

This project can be built and executed as follows:

.. code-block:: console

   $ cd samples/mpu_test
   $ make BOARD=v2m_beetle

Connect the board to your host computer using the USB port.
Flash the generated zephyr.bin on the board.
Reset the board and you should be able to see on the corresponding
Serial Port the following message:

.. code-block:: console

   ***** BOOTING ZEPHYR OS v1.7.99 - BUILD: Mar  9 2017 13:01:59 *****
   *** MPU test options ***
   1 - Read a reserved address in the memory map
   2 - Write in to boot FLASH/ROM
   3 - Run code located in RAM
   Select an option:

To build the single thread version, use the supplied configuration file for
single thread: :file:`prj_single.conf`:

.. code-block:: console

   $ make BOARD=v2m_beetle CONF_FILE=prj_single.conf run

Sample Output
=============

.. code-block:: console

   ***** BOOTING ZEPHYR OS v1.7.99 - BUILD: May 12 2017 09:47:02 *****
   shell> select mpu_test
   mpu_test> read
   ***** BUS FAULT *****
     Executing thread ID (thread): 0x200003b8
     Faulting instruction address:  0x290
     Precise data bus error
     Address: 0x24000000
   Fatal fault in thread 0x200003b8! Aborting.
