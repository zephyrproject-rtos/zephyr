.. _usb_host_msd_sample:

USB Host Mass Storage Class Sample
###################################

Overview
********

This sample demonstrates the USB Host Mass Storage Class (MSC) driver
using the Bulk-Only Transport (BOT) protocol. It:

* Waits for a USB flash drive to be connected
* Prints device information (sector size and total capacity)
* Performs a raw sector read/write round-trip test
* Mounts the FAT file system and lists the root directory
* Writes a test file ``ZEPHYR.TXT`` and reads it back to verify

Requirements
************

* A board with a USB host controller supported by the Zephyr UHC driver
  (e.g., ``rd_rw612_bga`` or ``mimxrt700_evk``)
* A FAT-formatted USB flash drive

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/usb/host_msd
   :board: rd_rw612_bga
   :goals: build flash
   :compact:

After flashing, connect a USB flash drive to the USB host port. The
sample will print output similar to::

   USB Host Mass Storage Class Sample Application
   USB host enabled, please connect a USB flash drive
   Waiting for USB MSD device to connect...
   USB MSD device connected!
   === USB Mass Storage Device Information ===
     Disk Name:    USBDISK0
     Sector Size:  512 bytes
     Sector Count: 15523840
     Capacity:     7580 MB
   === Raw Sector Read/Write Test (LBA 15523832) ===
     Read/Write round-trip OK (512 bytes)
   FAT file system mounted at /USB
   === Directory Listing: /USB ===
     [FILE] EXISTING.TXT      1024 bytes
   === Writing Test File: /USB/ZEPHYR.TXT ===
     Wrote 33 bytes
   === Verifying Test File: /USB/ZEPHYR.TXT ===
     Content verified OK
   MSD connected: 10 seconds elapsed

Configuration
*************

The following Kconfig options are available:

* :kconfig:option:`CONFIG_USBH_MSD_CLASS` -- Enable USB host MSD driver
* :kconfig:option:`CONFIG_USBH_MSD_INSTANCES_COUNT` -- Max simultaneous devices
* :kconfig:option:`CONFIG_USBH_MSD_MAX_SECTORS_PER_XFER` -- Sectors per BOT transfer
