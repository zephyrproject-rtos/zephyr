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

.. zephyr-app-commands::
   :zephyr-app: samples/mpu/mpu_test
   :board: v2m_beetle
   :goals: build flash
   :compact:

To build the single thread version, use the supplied configuration file for
single thread: :file:`prj_single.conf`:

.. zephyr-app-commands::
   :zephyr-app: samples/mpu/mpu_test
   :board: v2m_beetle
   :conf: prj_single.conf
   :goals: run
   :compact:

To build a version that allows writes to the flash device, edit
``prj.conf``, and follow the directions in the comments to enable the
proper configs.

Sample Output
=============

.. code-block:: console

  uart:~$ mpu read
  ***** BUS FAULT *****
    Precise data bus error
    BFAR Address: 0x24000000
  ***** Hardware exception *****
  Current thread ID = 0x20000400
  Faulting instruction address = 0x80004a8
  Fatal fault in thread 0x20000400! Aborting.
