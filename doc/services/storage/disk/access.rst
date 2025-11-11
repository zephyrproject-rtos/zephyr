.. _disk_access_api:

Disk Access
###########

Overview
********

The disk access API provides access to storage devices.

Initializing Disks
******************

Since many disk devices (such as SD cards) are hotpluggable, the disk access
API provides IOCTLs to initialize and de-initialize the disk. They are
as follows:

* :c:macro:`DISK_IOCTL_CTRL_INIT`: Initialize the disk. Must be called before
  additional I/O operations can be run on the disk device. Equivalent to
  calling the legacy function :c:func:`disk_access_init`.

* :c:macro:`DISK_IOCTL_CTRL_DEINIT`: De-initialize the disk. Once this IOCTL
  is issued, the :c:macro:`DISK_IOCTL_CTRL_INIT` must be issued before
  the disk can be used for addition I/O operations.

Init/deinit IOCTL calls are balanced, so a disk will not de-initialize until
an equal number of deinit IOCTLs have been issued as init IOCTLs.

It is also possible to force a disk de-initialization by passing a
pointer to a boolean set to ``true`` as a parameter to the
:c:macro:`DISK_IOCTL_CTRL_DEINIT` IOCTL. This is an unsafe operation which
each disk driver may handle differently, but it will always return
a value indicating success.

Note that de-initializing a disk is a low level operation- typically the
de-initialization and initialization calls should be left to the filesystem
implementation, and the user application should not need to manually
de-initialize the disk and can instead call :c:func:`fs_unmount`

SD Card support
***************

Zephyr has support for some SD card controllers and support for interfacing
SD cards via SPI. These drivers use disk driver interface and a file system
can access the SD cards via disk access API.
Both standard and high-capacity SD cards are supported.

.. note:: FAT filesystems are not power safe so the filesystem may become
   corrupted if power is lost or if the card is removed without unmounting
   the filesystem

SD Memory Card subsystem
========================

Zephyr supports SD memory cards via the disk driver API, or via the SDMMC
subsystem. This subsystem can be used transparently via the disk driver API,
but also supports direct block level access to cards. The SDMMC subsystem
interacts with the :ref:`sd host controller api <sdhc_api>` to communicate
with attached SD cards.


SD Card support via SPI
=======================

Example devicetree fragment below shows how to add SD card node to ``spi1``
interface. Example uses pin ``PA27`` for chip select, and runs the SPI bus
at 24 MHz once the SD card has been initialized:

.. code-block:: devicetree

    &spi1 {
            status = "okay";
            cs-gpios = <&porta 27 GPIO_ACTIVE_LOW>;

            sdhc0: sdhc@0 {
		    compatible = "zephyr,sdhc-spi-slot";
                    reg = <0>;
                    status = "okay";
		    mmc {
			compatible = "zephyr,sdmmc-disk";
                        disk-name = "SD";
			status = "okay";
		    };
                    spi-max-frequency = <24000000>;
            };
    };

The SD card will be automatically detected and initialized by the
filesystem driver when the board boots.

To read and write files and directories, see the :ref:`file_system_api` in
:zephyr_file:`include/zephyr/fs/fs.h` such as :c:func:`fs_open()`,
:c:func:`fs_read()`, and :c:func:`fs_write()`.

eMMC Device Support
*******************

Zephyr also has support for eMMC devices using the Disk Access API.
MMC in zephyr is implemented using the SD subsystem because the MMC bus
shares a lot of similarity with the SD bus. MMC controllers also use the
SDHC device driver API.

Emulated block device on flash partition support
************************************************

Zephyr flashdisk driver makes it possible to use flash memory partition as
a block device. The flashdisk instances are defined in devicetree:

.. code-block:: devicetree

    / {
        msc_disk0 {
            compatible = "zephyr,flash-disk";
            partition = <&storage_partition>;
            disk-name = "NAND";
            cache-size = <4096>;
        };
    };

The cache size specified in :dtcompatible:`zephyr,flash-disk` node should be
equal to backing partition minimum erasable block size.

NVMe disk support
=================

NVMe disks are also supported

.. toctree::
    :maxdepth: 1

    nvme.rst


Disk Access API Configuration Options
*************************************

Related configuration options:

* :kconfig:option:`CONFIG_DISK_ACCESS`

API Reference
*************

.. doxygengroup:: disk_access_interface

Disk Driver Configuration Options
*********************************

Related driver configuration options:

* :kconfig:option:`CONFIG_DISK_DRIVERS`

Disk Driver Interface
*********************

.. doxygengroup:: disk_driver_interface
