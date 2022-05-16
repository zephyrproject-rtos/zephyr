.. _littlefs-sample:

littlefs File System Sample Application
#######################################

Overview
********

This sample app demonstrates use of Zephyr's :ref:`file system API
<file_system_api>` over `littlefs`_, using file system with files that:
* count the number of times the system has booted
* holds binary pattern with properly incremented values in it

Other information about the file system is also displayed.

.. _littlefs:
   https://github.com/ARMmbed/littlefs

Requirements
************

Flash memory device
-------------------

The partition labeled "storage" will be used for the file system; see
:ref:`flash_map_api`.  If that area does not already have a
compatible littlefs file system its contents will be replaced by an
empty file system.  You will see diagnostics like this::

   [00:00:00.010,192] <inf> littlefs: LittleFS version 2.0, disk version 2.0
   [00:00:00.010,559] <err> littlefs: Corrupted dir pair at 0 1
   [00:00:00.010,559] <wrn> littlefs: can't mount (LFS -84); formatting

The error and warning are normal for a new file system.

After the file system is mounted you'll also see::

   [00:00:00.182,434] <inf> littlefs: filesystem mounted!
   [00:00:00.867,034] <err> fs: failed get file or dir stat (-2)

This error is also normal for Zephyr not finding a file (the boot count,
in this case).

Block device (e.g. SD card)
---------------------------

One needs to prepare the SD card with littlefs file system on
the host machine with the `lfs`_ program.

.. _lfs:
   https://www.thevtool.com/mounting-littlefs-on-linux-machine/

.. code-block:: console

   sudo chmod a+rw /dev/sda
   lfs -d -s -f --read_size=512 --prog_size=512 --block_size=512 --cache_size=512 --lookahead_size=8192 --format /dev/sda
   lfs -d -s -f --read_size=512 --prog_size=512 --block_size=512 --cache_size=512 --lookahead_size=8192 /dev/sda ./mnt_littlefs
   cd ./mnt_littlefs
   echo -en '\x01' > foo.txt
   cd -
   fusermount -u ./mnt_littlefs


Building and Running
********************

Flash memory device
-------------------

This example should work on any board that provides a "storage"
partition.  Two tested board targets are described below.

You can set ``CONFIG_APP_WIPE_STORAGE`` to force the file system to be
recreated.

Block device (e.g. SD card)
---------------------------

This example has been devised and initially tested on :ref:`Nucleo H743ZI <nucleo_h743zi_board>`
board. It can be also run on any other board with SD card connected to it.

To build the test:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/fs/littlefs
   :board: nucleo_h743zi
   :goals: build flash
   :gen-args: -DCONF_FILE=prj_blk.conf
   :compact:

One can also set ``CONFIG_SDMMC_VOLUME_NAME`` to provide the mount point name
for `littlefs` file system block device.


NRF52840 Development Kit
========================

On this device the file system will be placed in the SOC flash.

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/fs/littlefs
   :board: nrf52840dk_nrf52840
   :goals: build
   :compact:

Particle Xenon
==============

On this device the file system will be placed on the external SPI NOR
flash memory.

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/fs/littlefs
   :board: particle_xenon
   :goals: build
   :compact:
