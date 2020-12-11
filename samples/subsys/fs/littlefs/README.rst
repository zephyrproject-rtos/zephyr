.. _littlefs-sample:

littlefs File System Sample Application
#######################################

Overview
********

This sample app demonstrates use of Zephyr's :ref:`file system API
<file_system_api>` over `littlefs`_, using a file system with a file that
counts the number of times the system has booted.  Other information
about the file system is also displayed.

.. _littlefs:
   https://github.com/ARMmbed/littlefs

Requirements
************

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

Building and Running
********************

This example should work on any board that provides a "storage"
partition.  Two tested board targets are described below.

You can set ``CONFIG_APP_WIPE_STORAGE`` to force the file system to be
recreated.

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
