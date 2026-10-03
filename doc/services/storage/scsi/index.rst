.. _scsi_midlayer:

SCSI mid-layer
##############

Overview
********

The SCSI mid-layer in :file:`subsys/scsi/` implements transport-neutral command
execution, sense decoding, and device probe. A transport (USB Mass Storage,
UFS, or application code) implements :c:struct:`scsi_driver_api`, binds a
:c:struct:`scsi_device`, and uses the shared CDB helpers in
:zephyr_file:`include/zephyr/scsi/scsi_cmd.h`.

The mid-layer does not include USB, UFS, or other bus-specific headers.

Disk volumes
************

:c:func:`scsi_disk_register` exposes a probed SCSI LUN through the
:ref:`disk_access_api`. :c:func:`scsi_partition_discover_fat` locates a FAT
volume on partitioned media and supplies an LBA offset for the disk driver.

Device-side USB Mass Storage reuses the same CDB opcode definitions and layout
helpers via :kconfig:option:`CONFIG_USBD_MSC_CLASS`, which ``select``\s
:kconfig:option:`CONFIG_SCSI`.

Configuration
*************

.. list-table::
   :header-rows: 1

   * - Kconfig
     - Purpose
   * - :kconfig:option:`CONFIG_SCSI`
     - Enable the SCSI mid-layer
   * - :kconfig:option:`CONFIG_DISK_DRIVER_SCSI`
     - ``disk_access`` backend for host or application use
   * - :kconfig:option:`CONFIG_SCSI_DISK_BOUNCE_BUF`
     - Static bounce buffer for unaligned ``disk_access`` buffers (host-side)
   * - :kconfig:option:`CONFIG_SCSI_SYNC_CACHE_RETRY_COUNT`
     - Retries for SYNCHRONIZE CACHE on removable media

Host-side USB Mass Storage BOT and sample applications that mount FAT volumes
over ``scsi_disk`` are developed in separate changes. See the USB device
:zephyr:code-sample:`usb-mass` sample for device-side integration with this
mid-layer.

API reference
*************

Public headers:

* :zephyr_file:`include/zephyr/scsi/scsi.h`
* :zephyr_file:`include/zephyr/scsi/scsi_cmd.h`
* :zephyr_file:`include/zephyr/scsi/scsi_driver.h`
* :zephyr_file:`include/zephyr/scsi/scsi_sense.h`
* :zephyr_file:`include/zephyr/drivers/disk/scsi_disk.h`
* :zephyr_file:`include/zephyr/drivers/disk/scsi_partition.h`
