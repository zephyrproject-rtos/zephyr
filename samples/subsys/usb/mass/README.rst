.. zephyr:code-sample:: usb-mass
   :name: USB Mass Storage
   :relevant-api: usbd_api _usb_device_core_api file_system_api

   Expose board's RAM or FLASH as a USB disk using USB Mass Storage driver.

Overview
********

This sample app demonstrates use of a USB Mass Storage driver by the Zephyr
project. This very simple driver enumerates a board with either RAM or FLASH
into an USB disk.  This sample can be found under
:zephyr_file:`samples/subsys/usb/mass` in the Zephyr project tree.

Requirements
************

This project requires a USB device driver, and either 96KiB of RAM or a FLASH device.

Building and Running
********************

This sample can be built for multiple boards, customized through overlays found
in :zephyr_file:`samples/subsys/usb/mass/boards` in the Zephyr project tree.
The selection between a RAM-based or a FLASH-based disk and file system
can be chosen passing Kconfig configuration via the -D command-line switch.

RAM-disk Example without any file system
========================================

The default configurations selects RAM-based disk without any file system.
This example only needs additional 96KiB RAM for the RAM-disk and is intended
for testing USB mass storage class implementation.

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/usb/mass
   :board: reel_board
   :gen-args: -DEXTRA_DTC_OVERLAY_FILE="ramdisk.overlay"
   :goals: build
   :compact:


FAT FS Example
==============

If more than 96KiB are available, FAT files system can be used
with a RAM-disk. Alternatively it is possible with the FLASH-based disk.
In this example we will build the sample with a RAM-based disk:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/usb/mass
   :board: reel_board
   :gen-args: -DEXTRA_DTC_OVERLAY_FILE="ramdisk.overlay" -DCONFIG_APP_MSC_STORAGE_RAM=y
   :goals: build
   :compact:


In this example we will build the sample with a FLASH-based disk and FAT
file system for Adafruit Feather nRF52840 Express.  This board configures
to use the external 16 MiBi QSPI flash chip with a 2 MiBy FAT partition.

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/usb/mass
   :board: adafruit_feather_nrf52840
   :gen-args: -DCONFIG_APP_MSC_STORAGE_FLASH_FATFS=y
   :goals: build
   :compact:

After you have built and flashed the sample app image to your board, plug the
board into a host device, for example, a PC running Linux.
The board will be detected as shown by the Linux journalctl command:

.. code-block:: console

    $ journalctl -k -n 17
    usb 2-2.4: new full-speed USB device number 29 using xhci_hcd
    usb 2-2.4: New USB device found, idVendor=2fe3, idProduct=0008, bcdDevice= 2.03
    usb 2-2.4: New USB device strings: Mfr=1, Product=2, SerialNumber=3
    usb 2-2.4: Product: Zephyr MSC sample
    usb 2-2.4: Manufacturer: ZEPHYR
    usb 2-2.4: SerialNumber: 86FE679A598AC47A
    usb-storage 2-2.4:1.0: USB Mass Storage device detected
    scsi host3: usb-storage 2-2.4:1.0
    scsi 3:0:0:0: Direct-Access     ZEPHYR   ZEPHYR USB DISK  0.01 PQ: 0 ANSI: 0 CCS
    sd 3:0:0:0: Attached scsi generic sg4 type 0
    sd 3:0:0:0: [sdb] 256 512-byte logical blocks: (131 kB/128 KiB)
    sd 3:0:0:0: [sdb] Write Protect is off
    sd 3:0:0:0: [sdb] Mode Sense: 03 00 00 00
    sd 3:0:0:0: [sdb] No Caching mode page found
    sd 3:0:0:0: [sdb] Assuming drive cache: write through
     sdb:
    sd 3:0:0:0: [sdb] Attached SCSI removable disk

The output to the console will look something like this
(file system contents will be different):

.. code-block:: none

    *** Booting Zephyr OS build zephyr-v2.3.0-1991-g4c8d1496eafb  ***
    Area 4 at 0x0 on GD25Q16 for 2097152 bytes
    Mount /NAND:: 0
    /NAND:: bsize = 512 ; frsize = 1024 ; blocks = 2028 ; bfree = 1901
    /NAND: opendir: 0
      F 0 SAMPLE.TXT
    End of files
    [00:00:00.077,423] <inf> main: The device is put in USB mass storage mode.

On most operating systems the drive will be automatically mounted.

SD Card Example
===============

This example requires SD card support, see :ref:`disk_access_api`, and
a SD card formatted with FAT filesystem.

If a board with SD card controller is available, the example can be built as
follows:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/usb/mass
   :board: mimxrt1050_evk
   :gen-args: -DCONFIG_APP_MSC_STORAGE_SDCARD=y
   :goals: build
   :compact:

In case the board has no support for SD card controller, but the card can
be connected to SPI using e.g. a shield, example can be built as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/usb/mass
   :board: nrf52840dk_nrf52840
   :shield: waveshare_epaper_gdeh0154a07
   :gen-args: -DCONFIG_APP_MSC_STORAGE_SDCARD=y
   :goals: build
   :compact:

Depending on the size of the media it can take time until the file system has
initialized the card and it is available via USB. It should also be noted that
the transfer speed over SPI is very slow.

.. code-block:: none

   *** Booting Zephyr OS build v2.5.0-rc3-73-gd85067f0a759  ***
   Mount /SD:: 0
   [00:00:00.281,585] <inf> sdhc_spi: Found a ~3751 MiB SDHC card.
   [00:00:00.282,867] <inf> sdhc_spi: Manufacturer ID=27 OEM='SM' Name='00000' Revision=0x10 Serial=0x16fdd47b
   [00:00:00.308,654] <inf> sdhc_spi: Found a ~3751 MiB SDHC card.
   [00:00:00.309,906] <inf> sdhc_spi: Manufacturer ID=27 OEM='SM' Name='00000' Revision=0x10 Serial=0x16fdd47b
   /SD:: bsize = 512 ; frsize = 32768 ; blocks = 119776 ; bfree = 119773
   /SD: opendir: 0
     D 0 42
     F 1111 TEST.TXT
   End of files
   [00:00:18.588,043] <inf> main: The device is put in USB mass storage mode.

LittleFS Example
================

This board configures to use the external 64 MiBi QSPI flash chip with a
128 KiBy `littlefs`_ partition compatible with the one produced by the
:zephyr:code-sample:`littlefs` sample.

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/usb/mass
   :board: nrf52840dk_nrf52840
   :gen-args: -DCONFIG_APP_MSC_STORAGE_FLASH_LITTLEFS=y
   :goals: build
   :compact:

After you have built and flashed the sample app image to your board,
connect the board's two USB connectors (debug and nRF USB) to a host
running a littlefs-FUSE-capable operating system.  The output to the
console will look something like this (file system contents will be
different):

.. code-block:: none

    *** Booting Zephyr OS build zephyr-v2.2.0-1966-g7815942d5fc5  ***
    Area 4 at 0x0 on MX25R64 for 65536 bytes
    [00:00:00.005,310] <inf> main: The device is put in USB mass storage mode.

    [00:00:00.009,002] <inf> littlefs: LittleFS version 2.2, disk version 2.0
    [00:00:00.009,063] <inf> littlefs: FS at MX25R64:0x0 is 16 0x1000-byte blocks with 512 cye
    [00:00:00.009,063] <inf> littlefs: sizes: rd 16 ; pr 16 ; ca 64 ; la 32
    [00:00:00.011,718] <inf> littlefs: /lfs mounted
    Mount /lfs: 0
    /lfs: bsize = 16 ; frsize = 4096 ; blocks = 16 ; bfree = 13
    /lfs opendir: 0
      F 8 hi
      F 128 linux
      F 5 newfile
    End of files

For information on mounting littlefs file system on Linux or FreeBSD
systems refer to the "littlefs Usage" section below.

littlefs Usage
==============

While a FAT-based file system can be mounted by many systems automatically,
mounting the littlefs file system on a Linux or FreeBSD system can be
accomplished using the `littlefs-FUSE`_ utility.

First determine the local device name from the system log, e.g.:

.. code-block:: none

    Apr 25 08:10:25 tirzah kernel: [570310.921039] scsi 17:0:0:0: Direct-Access     ZEPHYR   ZEPHYR USB DISK  0.01 PQ: 0 ANSI: 0 CCS
    Apr 25 08:10:25 tirzah kernel: [570310.921550] sd 17:0:0:0: Attached scsi generic sg4 type 0
    Apr 25 08:10:25 tirzah kernel: [570310.922277] sd 17:0:0:0: [sdd] 256 512-byte logical blocks: (131 kB/128 KiB)
    Apr 25 08:10:25 tirzah kernel: [570310.922696] sd 17:0:0:0: [sdd] Write Protect is off

This shows that the block device associated with the USB drive is
``/dev/sdd``:

.. code-block:: shell

    tirzah[447]$ ll /dev/sdd
    brw-rw---- 1 root disk 8, 48 Apr 25 08:10 /dev/sdd

This can be mounted as a file system with the following commands:

.. code-block:: shell

   sudo chmod a+rw /dev/sdd   # required to allow user access
   mkdir /tmp/lfs
   lfs \
          --read_size=16 \
          --prog_size=16 \
          --block_size=4096 \
          --block_count=32 \
          --cache_size=64 \
          --lookahead_size=32 \
          /dev/sdd /tmp/lfs

which produces output like this (disk contents will vary):

.. code-block:: none

    tirzah[467]$ ls -l /tmp/lfs
    total 0
    -rwxrwxrwx 0 root root   8 Dec 31  1969 hi
    -rwxrwxrwx 0 root root 128 Dec 31  1969 linux
    -rwxrwxrwx 0 root root   5 Dec 31  1969 newfile

``lfs`` is a mount command and you should take care to unmount the
device before removing the USB drive:

.. code-block:: shell

   umount /tmp/lfs

littlefs parameter selection
----------------------------

Be aware that the parameters passed to :command:`lfs` in the example
above **must** exactly match the corresponding parameters used to
initialize the file system.  The required parameters can be observed
from the Zephyr mount log messages:

.. code-block:: none

    [00:00:00.009,002] <inf> littlefs: LittleFS version 2.2, disk version 2.0
    [00:00:00.009,063] <inf> littlefs: FS at MX25R64:0x0 is 16 0x1000-byte blocks with 512 cye
    [00:00:00.009,063] <inf> littlefs: sizes: rd 16 ; pr 16 ; ca 64 ; la 32

* ``--read_size`` corresponds to the ``rd`` size and is 16;
* ``--prog_size`` corresponds to the ``pr`` size and is 16;
* ``--block_size`` comes from ``0x1000-byte blocks`` and is 4096 (0x1000);
* ``--block_count`` comes from ``16 0x1000-byte blocks`` and is 16;
* ``--cache_size`` comes from the ``ca`` size and is 64;
* ``--lookahead_size`` comes from the ``la`` size and is 32

If any of the parameters are inconsistent between the Zephyr and Linux
specification the file system will not mount correctly.

.. _littlefs: https://github.com/littlefs-project/littlefs
.. _littlefs-FUSE: https://github.com/littlefs-project/littlefs-fuse
