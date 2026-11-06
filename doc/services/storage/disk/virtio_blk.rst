.. _disk_virtio_blk:

VirtIO Block
############

VirtIO is a standardized interface for exposing virtual devices to a guest,
typically under a hypervisor such as QEMU. The ``virtio,blk`` driver presents a
VirtIO block device as a Zephyr disk: it is reachable through the
:ref:`Disk Access API <disk_access_api>` and, through it, the
:ref:`File System API <file_system_api>`.

The driver is transport-agnostic and runs unchanged over both VirtIO transports
Zephyr supports, PCI and MMIO (see :ref:`VirtIO <virtio>`). An instance is
created for every enabled ``virtio,blk`` devicetree node.

Driver design
*************

The driver lives in :zephyr_file:`drivers/disk/virtio_blk.c` and is layered on
top of the generic VirtIO API. A ``virtio,blk`` node is a child of a VirtIO
transport device (``DT_INST_PARENT``), so the same code serves a PCI or an MMIO
device without change.

Request pool
============

The ``disk_access`` read/write API is synchronous and blocking, but the driver
keeps up to :kconfig:option:`CONFIG_DISK_VIRTIO_BLK_MAX_INFLIGHT` requests in
flight. Each request owns its header, status byte, completion semaphore and
slot in the free-index pool, and the completion callback signals only that
request. The caller blocks on its own semaphore and returns the slot after
reading the status.

Chain construction is serialized by a mutex. The virtqueue hands out
descriptors one at a time and may block partway through a chain, so two
builders could otherwise each hold descriptors the other one needs.
Completions are not serialized, so submitted requests still overlap.

The request queue is sized for ``(MAX_SEGMENTS + 2) * MAX_INFLIGHT``
descriptors, rounded up to a power of two and capped at 256 entries; the two
extra descriptors per request hold the request header and the status byte. When
the configured combination would need more than 256 entries, the driver lowers
the effective number of in-flight requests at build time. If the device
advertises a smaller ``seg_max`` or queue size, that limit applies instead.

Block size
==========

The VirtIO block protocol always addresses the device in fixed 512-byte sectors
and reports device capacity in those units. The logical block size exposed to
``disk_access`` may differ:

* When the device offers ``VIRTIO_BLK_F_BLK_SIZE``, the driver negotiates it and
  exposes the device-advertised ``blk_size``.
* Otherwise it falls back to
  :kconfig:option:`CONFIG_DISK_VIRTIO_BLK_SECTOR_SIZE`.

All addressing is converted to 512-byte VirtIO sectors internally. The block
size must be a multiple of 512 and, under :kconfig:option:`CONFIG_MMU`, must not
exceed the MMU page size; an unsupported value is rejected at initialization.

Zero-copy transfers
===================

Caller buffers are handed to the device directly, without a bounce buffer. Under
:kconfig:option:`CONFIG_MMU` a buffer may lie outside the kernel's linear RAM
mapping (for example the thread-stack work area ``f_mkfs()`` passes down) and
need not be physically contiguous. The driver walks such a buffer page by page
with ``arch_page_phys_get()``, coalesces physically adjacent runs into
scatter-gather segments, and, when a transfer is more fragmented than the
segment budget allows, splits it across several device requests.

Feature negotiation
===================

Besides ``VIRTIO_BLK_F_BLK_SIZE`` the driver negotiates:

* ``VIRTIO_BLK_F_RO`` — a read-only device is reported as
  ``DISK_STATUS_WR_PROTECT`` and write requests are rejected with ``-EROFS``.
* ``VIRTIO_BLK_F_FLUSH`` — ``DISK_IOCTL_CTRL_SYNC`` issues a flush to the device.
  When the device did not offer ``FLUSH`` there is no writeback cache to flush,
  so the ioctl is a successful no-op rather than an error.

A completed request whose status byte is not ``OK`` is logged and mapped to an
errno: an unsupported request becomes ``-ENOTSUP`` and an I/O error ``-EIO``.

VirtIO block configuration
**************************

DTS
===

A ``virtio,blk`` node is placed under its VirtIO transport parent and requires a
``disk-name`` used by the ``disk_access`` APIs. Over the PCI transport:

.. code-block:: devicetree

    #include <zephyr/dt-bindings/pcie/pcie.h>

    / {
        pcie0 {
            virtio-blk-pci {
                compatible = "virtio,pci";
                vendor-id = <0x1af4>;
                device-id = <0x1001>;
                interrupts = <0xb 0x0 0x0>;
                interrupt-parent = <&intc>;
                status = "okay";

                virtio_blk: virtio-blk {
                    compatible = "virtio,blk";
                    disk-name = "VIRTIOBLK0";
                    status = "okay";
                };
            };
        };
    };

Over the MMIO transport the node is a child of an existing ``virtio,mmio`` bus:

.. code-block:: devicetree

    &virtio_mmio4 {
        status = "okay";

        virtio_blk: virtio-blk {
            compatible = "virtio,blk";
            disk-name = "VIRTIOBLK0";
            status = "okay";
        };
    };

Options
-------

* :kconfig:option:`CONFIG_DISK_DRIVER_VIRTIO_BLK`
* :kconfig:option:`CONFIG_DISK_VIRTIO_BLK_MAX_SEGMENTS`
* :kconfig:option:`CONFIG_DISK_VIRTIO_BLK_MAX_INFLIGHT`
* :kconfig:option:`CONFIG_DISK_VIRTIO_BLK_SECTOR_SIZE`

QEMU options
------------

When :kconfig:option:`CONFIG_DISK_DRIVER_VIRTIO_BLK` is enabled on a QEMU board,
the emulation layer creates a raw backing image and attaches it to the emulated
virtio-blk device. PCI transports are attached from the common QEMU emulation
code; the ``qemu_cortex_a53`` board attaches an MMIO ``virtio-blk-device`` to
``virtio-mmio-bus.4``.

* :kconfig:option:`CONFIG_QEMU_VIRTIO_BLK_LOGICAL_BLOCK_SIZE`
* :kconfig:option:`CONFIG_QEMU_VIRTIO_BLK_DISK_SIZE`

Limitations
***********

* Each ``disk_access`` call blocks until its request completes. The driver
  overlaps requests from different threads, but a single caller cannot
  pipeline transfers.
* The effective number of in-flight requests can be lower than
  :kconfig:option:`CONFIG_DISK_VIRTIO_BLK_MAX_INFLIGHT` when the request queue
  would otherwise exceed 256 descriptors. A device that advertises a smaller
  queue or ``seg_max`` also lowers the achievable concurrency.
* Like the rest of Zephyr's VirtIO subsystem, the driver assumes coherent DMA
  and performs no cache maintenance on descriptors, rings, or data buffers.
* Under an MMU the logical block size cannot exceed the MMU page size.
