.. zephyr:code-sample:: scsi-midlayer-test
   :name: SCSI mid-layer (mock transport)
   :relevant-api: scsi_api disk_access_interface

   Exercise :kconfig:option:`CONFIG_SCSI` CDB builders and FAT partition discovery
   with an in-application mock :c:struct:`scsi_driver_api`.

Overview
********

This sample runs on the host simulator only. It does not require USB or other
hardware. The application:

* Builds a TEST UNIT READY CDB via :c:func:`scsi_cmd_test_unit_ready`
* Binds a synthetic disk image to :c:struct:`scsi_device` through a minimal mock
  :c:struct:`scsi_driver_api`
* Locates a FAT slice with :c:func:`scsi_partition_find_fat_volume`

For the full mid-layer design, initiator vs USB Mass Storage target roles, and
Kconfig options, see :ref:`scsi_midlayer`.

Building and Running
********************

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/scsi/test
   :board: native_sim
   :goals: build run
   :compact:

Sample Output
=============

.. code-block:: console

   SCSI mid-layer sample
   CDB builders OK
   FAT volume LBA 4096 sectors 8192
