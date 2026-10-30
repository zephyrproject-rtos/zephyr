.. _scsi_midlayer:

SCSI mid-layer
##############

Overview
********

The SCSI mid-layer in :file:`subsys/scsi/` implements transport-neutral command
execution, sense decoding, and device probe. An **initiator** transport implements
:c:struct:`scsi_driver_api`, binds a :c:struct:`scsi_device`, and uses the
shared CDB helpers in :zephyr_file:`include/zephyr/scsi/scsi_cmd.h`.

The mid-layer does not include bus-specific headers (USB, UFS, and so on).

Public headers are marked **experimental** (API version 0.1.0 in Doxygen); see
:ref:`api_lifecycle_experimental`.

Initiator vs target
*******************

.. list-table::
   :header-rows: 1

   * - Role
     - Typical use
     - ``CONFIG_SCSI`` / ``scsi_device``
     - ``CONFIG_DISK_DRIVER_SCSI`` / ``scsi_disk_register``
   * - Initiator (host)
     - Probe and I/O on a remote LUN
     - Yes
     - Yes, for :ref:`disk_access_api` on block media
   * - Target (device)
     - Parse CDBs locally on the gadget
     - No
     - No; LUNs use :c:macro:`USBD_DEFINE_MSC_LUN` and ``disk_access``

Device-side Mass Storage uses :kconfig:option:`CONFIG_USBD_MSC_CLASS` only.
It shares opcodes via :zephyr_file:`include/zephyr/scsi/scsi_opcode.h` and does
**not** enable :kconfig:option:`CONFIG_SCSI`, :c:func:`scsi_exec`, or
:c:func:`scsi_disk_register`. See :zephyr:code-sample:`usb-mass`.

Runtime registration
********************

:c:func:`scsi_disk_register` is for **initiator** use when a LUN appears at
runtime (removable media). There is no devicetree node for an arbitrary plug-in
volume, so the transport binds :c:struct:`scsi_device` after discovery and
registers a ``disk_access`` name.

When layout is known at build time, preset :c:member:`scsi_disk.lba_offset` and
:c:member:`scsi_disk.sector_count` on :c:struct:`scsi_disk` and skip partition
walk. Fixed on-SoC LUNs may later use devicetree phandles; that does not replace
runtime registration for hot-plug media.

Partition tables and filesystem probes
**************************************

:c:func:`scsi_disk_register` exposes a probed SCSI LUN through the
:ref:`disk_access_api`.

**Table parsing** (MBR/GPT) is separate from **filesystem identification**:

* :c:func:`disk_partition_walk` fills :c:struct:`disk_partition_table` from DOS
  MBR (including extended/logical chains) or GPT. If no table is found,
  :c:func:`disk_partition_set_whole_disk` describes one span over the whole
  device.
* :c:func:`disk_partition_find_fat_volume` and
  :c:func:`disk_partition_find_ext2_volume` test each slice (or the whole-disk
  span) for FAT/exFAT BPB or ext2 superblock magic.

SCSI adapters :c:func:`scsi_partition_walk`,
:c:func:`scsi_partition_find_fat_volume`, and
:c:func:`scsi_partition_find_ext2_volume` call :c:func:`scsi_io_read` on the raw
LUN before a ``disk_access`` volume exists.

Partition helpers keep one sector buffer (512 bytes when the logical block size
is 512) and a partition table on the **calling thread's stack**. Reserve several
KiB of stack above the transport's usual depth when calling
:c:func:`disk_partition_walk` or the SCSI partition adapters.

Validation
**********

**Initiator:** :zephyr:code-sample:`scsi-midlayer-test` walks CDB builders and
:c:func:`scsi_partition_find_fat_volume` with a mock transport on
``native_sim``. Additional coverage lives under :zephyr_file:`tests/subsys/scsi/`
and :zephyr_file:`tests/subsys/disk/partition`. **Target:** USB Mass Storage
remains :zephyr:code-sample:`usb-mass` (shared opcodes via
:zephyr_file:`include/zephyr/scsi/scsi_opcode.h` only).

.. list-table::
   :header-rows: 1

   * - Twister scenario
     - Source tree
     - Coverage
   * - ``subsys.scsi.cmd``
     - :zephyr_file:`tests/subsys/scsi/cmd`
     - CDB builders in :zephyr_file:`include/zephyr/scsi/scsi_cmd.h`
   * - ``subsys.scsi.disk``
     - :zephyr_file:`tests/subsys/scsi/scsi_disk`
     - Mock :c:struct:`scsi_driver_api`, :c:func:`scsi_partition_find_fat_volume`
   * - ``subsys.disk.partition``
     - :zephyr_file:`tests/subsys/disk/partition`
     - MBR/GPT walk and FAT/ext2 probes via :c:func:`disk_partition_walk`

Run the suites on the host simulator from :envvar:`ZEPHYR_BASE`:

.. code-block:: console

   west twister -p native_sim -T tests/subsys/scsi -T tests/subsys/disk/partition
   west twister -p native_sim/native/64 -T tests/subsys/scsi \
       -T tests/subsys/disk/partition

Configuration
*************

.. list-table::
   :header-rows: 1

   * - Kconfig
     - Purpose
   * - :kconfig:option:`CONFIG_SCSI`
     - Enable the SCSI mid-layer
   * - :kconfig:option:`CONFIG_DISK_DRIVER_SCSI`
     - ``disk_access`` backend for initiator use
   * - :kconfig:option:`CONFIG_DISK_PARTITION`
     - MBR/GPT walk (selected by ``DISK_DRIVER_SCSI``)
   * - :kconfig:option:`CONFIG_SCSI_DISK_BOUNCE_BUF`
     - Static bounce buffer for unaligned ``disk_access`` buffers
   * - :kconfig:option:`CONFIG_SCSI_SYNC_CACHE_RETRY_COUNT`
     - Retries for SYNCHRONIZE CACHE on removable media

API reference
*************

Public headers:

* :zephyr_file:`include/zephyr/scsi/scsi.h`
* :zephyr_file:`include/zephyr/scsi/scsi_opcode.h`
* :zephyr_file:`include/zephyr/scsi/scsi_cmd.h`
* :zephyr_file:`include/zephyr/scsi/scsi_driver.h`
* :zephyr_file:`include/zephyr/scsi/scsi_sense.h`
* :zephyr_file:`include/zephyr/drivers/disk/scsi_disk.h`
* :zephyr_file:`include/zephyr/drivers/disk/scsi_partition.h`
* :zephyr_file:`include/zephyr/storage/disk_partition.h`
