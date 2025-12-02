:orphan:

.. _mcuboot_guide:

MCUboot Guide
#############

1. Overview
===========

MCUboot began its life as the bootloader for Mynewt. It has since acquired the ability to be used as a bootloader for Zephyr as well.

This document provides a detailed introduction to the subsystems, drivers related to MCUboot in Zephyr, as well as examples and test cases on how to test these components.

The following are the subsystems and drivers related to MCUboot in Zephyr:

- **Modules**: crypto/mbedtls
- **Subsystems**: MCUmgr, DFU, Retention System
- **Drivers**: FLASH, FLASH_MAP, STREAM_FLASH, NVS

2. Related Modules, Subsystems, and Drivers
===========================================

2.1 Modules
-----------

* **crypto/mbedtls**
  Provides cryptographic algorithms (such as SHA-256, ECDSA) and signature verification functions, which are used to verify the firmware images signed by MCUboot (to prevent tampering).

2.2 Subsystems
--------------

* **MCUmgr**
  Subsystem in Zephyr for remote management of devices. Based on the Simple Management Protocol (SMP), supports UART, Bluetooth, and network.

* **DFU (Device Firmware Update)**
  Provides the necessary frameworks to upgrade the image of a Zephyr-based application at run time.

* **Retention System**
  Provides APIs to read/write from memory or devices that retain data across reboots.

2.3 Drivers
-----------

* **NVS (Non-Volatile Storage)**: Simple Flash-based filesystem for non-volatile data.
* **Flash map**: Defines layout and mapping of Flash partitions.


3. Samples
==========

3.1 with_mcuboot
----------------

**Path**: ``samples/sysbuild/with_mcuboot``

.. code-block:: console

   # Ubuntu build
   west build -b apollo510_eb -p=always --sysbuild ./samples/sysbuild/with_mcuboot -d ../build/sysbuild/with_mcuboot

   # flash
   west flash -d ../build/sysbuild/with_mcuboot

3.2 smp_svr
-----------

**Path**: ``samples/subsys/mgmt/mcumgr/smp_svr``

See the Practical Guide.

3.3 dfu-next
------------

**Path**: ``samples/subsys/usb/dfu-next``

#TODO


4. Tests
========

4.1 test_mcuboot
----------------

**Path**: ``tests/boot/test_mcuboot``

.. code-block:: console

   # Ubuntu build
   west build -b apollo510_evb -p=auto -d ../build/tests/boot/test_mcuboot ./tests/boot/test_mcuboot/ -T bootloader.mcuboot

   # flash
   west flash -d ../build/tests/boot/test_mcuboot

   # test with twister
   ./scripts/twister -vv --west-flash --enable-slow -T ./tests/boot/test_mcuboot -p apollo510_evb --device-testing --device-serial /dev/ttyACM0 --outdir ../twister-out

Note: Sometimes the twister test fails due to incomplete serial log when MCU resets.

4.2 with_mcumgr
---------------

**Path**: ``tests/boot/with_mcumgr``

.. code-block:: console

   # Ubuntu build
   west build -b apollo510_evb -p=auto ./tests/boot/with_mcumgr/ -d ../build/tests/boot/with_mcumgr/test_upgrade -T boot.with_mcumgr.test_upgrade
   west build -b apollo510_evb -p=auto ./tests/boot/with_mcumgr -d ../build/tests/boot/with_mcumgr/test_upgrade -T boot.with_mcumgr.test_upgrade.swap_using_offset

   # flash
   west flash -d ../build/tests/boot/mcuboot_data_sharing

   # Test with the twister
   ./scripts/twister -vv --west-flash --enable-slow -T ./tests/boot/with_mcumgr -p apollo510_evb --device-testing --device-serial /dev/ttyACM0 --outdir ../twister-out

The test requires installing the ``mcumgr`` CLI: ``apache/mynewt-mcumgr-cli``.

4.3 mcuboot_recovery_retention
------------------------------

**Path**: ``tests/boot/mcuboot_recovery_retention``

#TODO

4.4 mcuboot_data_sharing
------------------------

**Path**: ``tests/boot/mcuboot_data_sharing``

.. code-block:: console

   # Ubuntu build
   west build -b apollo510_evb -p=auto ./tests/boot/mcuboot_data_sharing -d ../build/tests/boot/mcuboot_data_sharing -T bootloader.mcuboot.data.sharing

   # flash
   west flash -d ../build/tests/boot/mcuboot_data_sharing

   # Test with the twister
   west twister -vv -p apollo510_evb -T ./tests/boot/mcuboot_data_sharing --outdir ../twister-out

4.5 retention
-------------

**Path**: ``tests/subsys/settings/retention``

.. code-block:: console

   west build -b apollo510_evb -p=always ./tests/subsys/settings/retention/ -d ../build/tests/settings-retention
   west flash -d ../build/tests/settings-retention

4.6 retained_mem
----------------

**Path**: ``tests/drivers/retained_mem/api``

.. code-block:: console

   west build -b apollo510_evb -p=always ./tests/drivers/retained_mem/api/ -d ../build/tests/drivers/retained_mem/api
   west flash -d ../build/tests/drivers/retained_mem/api

   # twister test
   python ./scripts/twister -vv --platform apollo510_evb -T ./tests/drivers/retained_mem/api/


4.7 flash/erase_blocks
----------------------

**Path**: ``tests/drivers/flash/erase_blocks``

.. code-block:: console

   west build -b apollo510_evb -p=always ./tests/drivers/flash/erase_blocks/ -d ../build/tests/drivers/flash/erase-blocks
   west flash -d ../build/tests/drivers/flash/erase-blocks

4.8 dfu/img_util
----------------

**Path**: ``tests/subsys/dfu/img_util``

.. code-block:: console

   west build -b apollo510_evb -p=always ./tests/subsys/dfu/img_util -d ../build/tests/dfu/img_util -T dfu.image_util
   west flash -d ../build/tests/dfu/img_util


5. Practical Guide
==================

5.1 Prerequisites
-----------------

* **Tools**:
  To send firmware updates via UART, we will use **AuTerm**. For alternatives, check the related documentation.

* **Create a test image using sysbuild**:

.. code-block:: console

   # Path: samples/sysbuild/hello_world
   west build -b apollo510_evb -p auto --sysbuild ./samples/hello_world -d ../build/sysbuild/helloworld


5.2 DFU over UART
-----------------

**Path**: ``samples/subsys/mgmt/mcumgr/smp_svr``

.. code-block:: console

   # Ubuntu build
   west build -b apollo510_evb -p=always ./samples/subsys/mgmt/mcumgr/smp_svr/ --sysbuild -d ../build/samples/susys/mgmt-mcumgr-smp-svr/serial -T sample.mcumgr.smp_svr.serial

   # flash
   west flash -d ../build/samples/susys/mgmt-mcumgr-smp-svr/serial

Connect to the AuTerm via serial.

Click **Connect** to connect to the board.

Reset the board.

List images on the device.

Upload and mark the uploaded new image for testing.

List images again.

Reset remotely using mcumgr client.

The device will boot into the test image. After reset, the new image runs.

Note: Since the image is marked as *test*, pressing the reset button will revert boot to slot0.

5.3 DFU over BLE
----------------

**Path**: ``samples/subsys/mgmt/mcumgr/smp_svr``

#TODO

5.4 DFU over USB
----------------

#TODO
