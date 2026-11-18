.. _scsi_midlayer:

SCSI mid-layer
##############

Overview
********

The SCSI mid-layer in :file:`subsys/scsi/` implements transport-neutral command
execution, sense decoding, and device probe. An initiator transport implements
:c:struct:`scsi_driver_api`, binds a :c:struct:`scsi_device`, and uses the
shared CDB helpers in :zephyr_file:`include/zephyr/scsi/scsi_cmd.h`.

The mid-layer does not include bus-specific headers.

Public headers are marked **experimental** (API version 0.1.0 in Doxygen); see
:ref:`api_lifecycle_experimental`.

Initiator and ``disk_access``
*****************************

Enable :kconfig:option:`CONFIG_SCSI` for the mid-layer and
:kconfig:option:`CONFIG_DISK_DRIVER_SCSI` to expose a probed LUN through the
:ref:`disk_access_api` with :c:func:`scsi_disk_register`. When a LUN appears at
runtime (removable media), the transport binds :c:struct:`scsi_device` and
registers a disk name; preset :c:member:`scsi_disk.lba_offset` and
:c:member:`scsi_disk.sector_count` when geometry is fixed at build time.

Optional :kconfig:option:`CONFIG_SCSI_DISK_PARTITION` adds
:c:func:`scsi_partition_walk` and selects :kconfig:option:`CONFIG_DISK_PARTITION`
(see :ref:`disk_partition_layout`). After :c:func:`scsi_device_probe`, walk
partitions if needed, select a slice in application code, set offset and sector
count on :c:struct:`scsi_disk`, then register. If no partition table is found,
use ``lba_offset = 0`` and ``sector_count = 0`` to expose the full LUN.

USB Mass Storage **gadget** (SCSI target) uses :kconfig:option:`CONFIG_USBD_MSC_CLASS`
and opcode constants from :zephyr_file:`include/zephyr/scsi/scsi_opcode.h` only;
it does not enable :kconfig:option:`CONFIG_SCSI` or :c:func:`scsi_disk_register`.

Device lifecycle
****************

Each :c:struct:`scsi_device` is bound to a transport-specific :c:type:`device`
(one LUN per binding). :c:func:`scsi_exec` passes that binding to
:c:member:`scsi_driver_api.exec`.

:c:func:`scsi_device_mark_removed` is for detach: further I/O returns
``-ENODEV``. :c:func:`scsi_device_media_changed` and
:c:func:`scsi_device_invalidate` refresh cached geometry after media change.
Transports should call :c:func:`scsi_device_mark_removed` before
:c:func:`scsi_disk_unregister`.

:c:func:`disk_partition_walk` and :c:func:`scsi_partition_walk` use a sector
buffer and partition table on the **calling thread's stack**; reserve extra
stack headroom in the transport thread.

Configuration
*************

.. list-table::
   :header-rows: 1

   * - Kconfig
     - Purpose
   * - :kconfig:option:`CONFIG_SCSI`
     - Enable the SCSI mid-layer
   * - :kconfig:option:`CONFIG_DISK_DRIVER_SCSI`
     - ``disk_access`` slice on a SCSI LUN
   * - :kconfig:option:`CONFIG_SCSI_DISK_PARTITION`
     - :c:func:`scsi_partition_walk` adapter (selects ``DISK_PARTITION``)
   * - :kconfig:option:`CONFIG_DISK_PARTITION`
     - Transport-neutral :c:func:`disk_partition_walk` (see :ref:`disk_partition_layout`)
   * - :kconfig:option:`CONFIG_SCSI_DISK_BOUNCE_BUF`
     - Static bounce buffer for unaligned ``disk_access`` buffers
   * - :kconfig:option:`CONFIG_SCSI_SYNC_CACHE_RETRY_COUNT`
     - Retries for SYNCHRONIZE CACHE when failure is not a non-fatal
       synchronization sense (see below)

SYNCHRONIZE CACHE and filesystem flush
**************************************

:c:func:`scsi_synchronize_cache_10` is used when the disk layer handles
``DISK_IOCTL_CTRL_SYNC`` (for example FatFs ``f_sync``). Some block devices
reject **SYNCHRONIZE CACHE (10)** with **ILLEGAL REQUEST** and the **INVALID
COMMAND OPERATION CODE** additional sense code (``0x20``) even though normal
READ/WRITE operations succeed. Because the device does not provide a usable
explicit cache-flush command, that response is treated as non-fatal for
filesystem synchronization.

**ASC 0x3a** (medium not present) is also non-fatal for cache synchronization
because there is no medium to flush.

A return value of ``0`` in these cases does **not** prove that data held in a
volatile device cache has reached persistent media. Sudden power loss or unsafe
removal may still lose recently written data.

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

See also :zephyr_file:`tests/subsys/scsi/`.
