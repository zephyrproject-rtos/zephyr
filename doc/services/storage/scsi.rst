.. _scsi_api:

SCSI mid-layer
##############

Overview
********

The SCSI mid-layer provides transport-neutral command construction, synchronous
command execution, sense parsing, and device state tracking. USB host Mass
Storage Class (MSC) is the initial transport consumer.

The SCSI mid-layer sits between file systems / disk_access and transport
drivers:

.. code-block:: none

   File system / disk_access
           |
   SCSI mid-layer (this subsystem)
           |
   USB MSC transport (subsys/usb/host/msc/)

The mid-layer owns SCSI CDB builders, status and sense handling, and high-level
commands such as READ(10) and WRITE(10). Transports implement
:c:type:`scsi_driver_api` and deliver commands over BOT (USB).

Future work (not in the initial series):

* Async command queuing and dedicated error-recovery handling

Transport driver API
********************

Transports register a :c:type:`scsi_driver_api` with the mid-layer.

Mandatory operation:

* ``exec()`` — execute one :c:type:`scsi_xfer` synchronously

Optional operations:

* ``reset()`` — reset transport / device SCSI path
* ``get_max_lun()`` — return maximum LUN index

USB host MSC implements the adapter in ``subsys/usb/host/msc/usb_msc_lun.c``.

Binding a SCSI device
*********************

.. code-block:: c

   struct scsi_device sdev;

   scsi_device_init(&sdev, transport_dev, &my_scsi_api, lun);
   ret = scsi_device_probe(&sdev);
   if (ret == 0) {
       /* sdev.block_size, sdev.block_count, sdev.removable are valid */
   }

USB host MSC binding example:

.. code-block:: c

   ret = usb_msc_scsi_bind(&sdev, uhc_dev, udev, msc, lun);
   if (ret == 0) {
       ret = scsi_device_probe(&sdev);
   }

   /* on disconnect */
   usb_msc_scsi_unbind(&sdev);

:c:func:`scsi_device_probe` runs INQUIRY, TEST UNIT READY (with configurable retries),
READ CAPACITY(10) and READ CAPACITY(16) when capacity exceeds 32-bit LBA space,
and MODE SENSE(6) for the write-protected flag. It is idempotent when the device
is already in :c:enumerator:`SCSI_DEV_READY` state.

Large disks (>2 TiB)
********************

When READ CAPACITY(10) returns ``last_lba == 0xffffffff``, the mid-layer
automatically issues READ CAPACITY(16) and sets :c:member:`scsi_device.use_16byte_cmds`.
Use :c:func:`scsi_io_read` / :c:func:`scsi_io_write` to select READ/WRITE(10) or (16).

Pass-through
************

:c:func:`scsi_sg_io` executes a raw CDB on a bound :c:type:`scsi_device`.

Command timeouts
****************

:c:member:`scsi_xfer.timeout_ms` is honored by the USB MSC BOT transport
(``usbh_msc_bot_set_command_timeout_ms``). When zero, transports use their
Kconfig default.

Generic disk driver
*******************

:c:func:`scsi_disk_register` in ``drivers/disk/scsi_disk.c`` is the
:c:func:`disk_access` backend for SCSI LUNs. USB MSC uses it for read, write,
and generic ioctls (including ``SG_IO`` with ``BSG_SUB_PROTOCOL_SCSI_CMD``).

Transport-specific glue is limited to **USB MSC** —
``subsys/usb/host/msc/usb_msc_lun.c`` maintains one ``usb_msc_lun`` object per
slot (transport + ``scsi_device`` [+ ``scsi_disk``]): binds transport, probes,
optionally discovers a FAT partition, registers ``scsi_disk`` during bringup,
and detaches on disconnect.

Enable :kconfig:option:`CONFIG_DISK_DRIVER_SCSI` for the generic SCSI disk
driver.

Command builders
****************

Use :file:`include/zephyr/scsi/scsi_cmd.h` helpers to populate
:c:type:`scsi_xfer`. Builders perform no I/O and encode multi-byte SCSI
fields in big-endian order. Shared opcodes are also used by the USB device-side
MSC class in ``subsys/usb/device_next/class/usbd_msc_scsi.c``.

High-level helpers in :file:`include/zephyr/scsi/scsi.h` (for example
``scsi_read_10()``) build the transfer, call ``scsi_exec()``, and on CHECK
CONDITION may issue REQUEST SENSE automatically.

CHECK CONDITION
***************

When a command completes with CHECK CONDITION:

* The transport ``exec()`` implementation should set :c:member:`scsi_xfer.status`
  to :c:macro:`SCSI_STATUS_CHECK_CONDITION`. For USB BOT this corresponds to a
  failed CSW with sense available.
* :c:func:`scsi_exec` preserves the SCSI status and returns a negative errno
  (typically ``-EIO``) even when the transport returns a non-zero errno, as long
  as the status byte indicates CHECK CONDITION.
* High-level helpers (``scsi_read_10``, ``scsi_test_unit_ready``, …) call
  REQUEST SENSE automatically and map sense key / ASC to errno via
  :c:func:`scsi_status_to_errno`.
* Callers using :c:func:`scsi_exec` directly may invoke
  :c:func:`scsi_request_sense` on the same :c:type:`scsi_device`.

Sense keys and a minimal ASC/ASCQ string mapping are provided by
:file:`include/zephyr/scsi/scsi_sense.h`.

Configuration
*************

Enable :kconfig:option:`CONFIG_SCSI`. USB MSC selects SCSI when
:kconfig:option:`CONFIG_USBH_MSC` is enabled. The generic SCSI disk driver is
selected with :kconfig:option:`CONFIG_DISK_DRIVER_SCSI`.
