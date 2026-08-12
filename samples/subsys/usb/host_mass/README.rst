.. zephyr:code-sample:: usb-host-mass
   :name: USB Host Mass Storage
   :relevant-api: disk_driver_interface usb_host_core_api

   Access a USB mass storage device from a Zephyr USB host.

Overview
********

This sample demonstrates how to use the USB Host MSC (Mass Storage Class)
driver to access a USB flash drive or other mass storage device connected
to a Zephyr device acting as a USB host.

Upon connection, the storage device is detected and enumerated automatically.
The sample performs raw sector-level read/write tests using the disk_access
API, analyzes the boot sector to identify the partition scheme, and
optionally mounts a FAT file system and runs basic file operations.

Requirements
************

This sample uses the USB host stack and requires a USB host controller driver.

A USB mass storage device (USB flash drive) formatted with FAT32 is required.

Building and Running
********************

The sample can be built and flashed as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/usb/host_mass
   :board: frdm_mcxn947/mcxn947/cpu0
   :goals: build flash
   :compact:

The device is expected to detect the USB mass storage device automatically
when connected.

Sample Output
=============

When a USB mass storage device is connected, you should see:

.. code-block:: console

   *** Booting Zephyr OS build v4.4.0 ***
   <inf> main: USB Host Mass Storage Sample - Raw Disk Access
   <inf> main: USB host MSC device usbh_msc_0 is ready
   <inf> main: Waiting for USB Mass Storage device...
   <inf> usbh_dev: New device with address 1 state 2
   <inf> usbh_dev: Configuration 1 bNumInterfaces 1
   <inf> usbh_dev: Set Interfaces 0, alternate 0 -> 0
   <inf> usbh_class: Class 'usbh_msc_c_data_0' matches interface 0
   <inf> main: USB Mass Storage device detected!
   <inf> main: Setting up disk access...

MSC Command Tests
=================

The sample runs a set of MSC command tests:

.. code-block:: console

   <inf> main: === MSC Command Test Start ===
   <inf> main: Test 1: Read Capacity - Get sector count...
   <inf> main:   Success, last logical block: 31260671
   <inf> main: Test 2: Read Capacity - Get block length...
   <inf> main:   Success, block length: 512 bytes
   <inf> main:   Total capacity: 16005464064 bytes (15264.00 MB)
   <inf> main: Test 3: Read(10) - Read sector 0...
   <inf> main:   Success
   <inf> main: Test 4: Write(10) - Write test pattern to sector 100...
   <inf> main:   Success
   <inf> main: Test 5: Read(10) - Read back sector 100 for verification...
   <inf> main:   Success
   <inf> main:   Data verification PASSED
   <inf> main: Test 6: Multi-sector Write/Read test (4 sectors)...
   <inf> main:   Write success
   <inf> main:   Read success
   <inf> main:   Multi-sector verification PASSED
   <inf> main: === MSC Command Test Complete ===

Boot Sector Analysis
====================

The sample analyzes the boot sector to identify the partition scheme:

.. code-block:: console

   <inf> main: === Analyzing Boot Sector (MBR/GPT) ===
   <inf> main: Valid boot sector signature (0x55AA)
   <inf> main: Partition scheme: MBR (Master Boot Record)
   <inf> main: Partition 1:
   <inf> main:   Status: 0x00
   <inf> main:   Type: 0x0C (FAT32)
   <inf> main:   Start LBA: 32
   <inf> main:   Sectors: 31260640 (2975.98 MB)

File System Operations
======================

When :kconfig:option:`CONFIG_APP_HOST_MASS_USE_FILESYSTEM` is enabled,
the sample mounts the FAT volume and performs basic file operations:

.. code-block:: console

   <inf> main: Mounting FAT file system at /usbh_msc_0:
   <inf> main: File system mounted successfully
   <inf> main: === File System Test Start ===
   <inf> main: Directory listing: /usbh_msc_0:
   <inf> main:   [DIR]  SYSTEM~1
   <inf> main:   [DIR]  ZEPHYR
   <inf> main: Directory: /usbh_msc_0:/zephyr
   <inf> main: Wrote 41 bytes to /usbh_msc_0:/zephyr/test.txt
   <inf> main: Read back: "Hello from Zephyr USB host mass storage!"
   <inf> main: File system test passed
   <inf> main: === File System Test Complete ===
   <inf> main: === Disk Access Setup Complete ===

Configuration Options
*********************

The sample can be configured through ``prj.conf``:

File System Support
===================

- :kconfig:option:`CONFIG_APP_HOST_MASS_USE_FILESYSTEM` - Enable or disable
  FAT file system access on the connected USB mass storage device.

Memory Configuration
====================

- :kconfig:option:`CONFIG_HEAP_MEM_POOL_SIZE` - General heap size
- :kconfig:option:`CONFIG_MAIN_STACK_SIZE` - Main thread stack size

USB Transfer Configuration
===========================

- :kconfig:option:`CONFIG_USBH_USB_DEVICE_HEAP` - USB device heap size
- :kconfig:option:`CONFIG_UHC_XFER_COUNT` - Max transfer requests
- :kconfig:option:`CONFIG_UHC_BUF_COUNT` - Transfer buffer count
- :kconfig:option:`CONFIG_UHC_BUF_POOL_SIZE` - Transfer buffer pool size

Logging Configuration
=====================

- :kconfig:option:`CONFIG_LOG` - Enable logging
- :kconfig:option:`CONFIG_LOG_BUFFER_SIZE` - Log buffer size
- :kconfig:option:`CONFIG_USBH_MSC_CLASS_LOG_LEVEL_WRN` - MSC driver log level

Troubleshooting
***************

Device Not Detected
===================

If the USB mass storage device is not detected, check the USB host controller
driver and ensure the device is properly connected.

File System Mount Failure
=========================

If the file system fails to mount, ensure the device is formatted with FAT32.
The sample will continue without file system access in this case.

Transfer Errors
===============

If transfer errors occur, they are handled automatically with a
Bulk-Only Transport reset recovery. Check USB host controller
buffer settings:

- :kconfig:option:`CONFIG_UHC_BUF_POOL_SIZE`
- :kconfig:option:`CONFIG_UHC_BUF_COUNT`
- :kconfig:option:`CONFIG_UHC_XFER_COUNT`
