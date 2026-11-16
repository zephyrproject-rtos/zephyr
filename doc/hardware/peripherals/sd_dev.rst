.. _sd_dev_api:

SD Device (Peripheral/Slave) Subsystem
#######################################

Overview
********

The SD Device subsystem provides a framework for implementing the
**device (slave/peripheral) side** of SD, SDIO, and MMC protocols in
Zephyr. This is the counterpart to the SD Host Controller (SDHC) subsystem,
which implements the host side.

A typical use case is a microcontroller or SoC that exposes an SDIO slave
interface to a host processor, allowing the host to communicate with the
device over the SDIO bus.

.. note::
   This subsystem is distinct from the :ref:`sdhc_api` (SD Host Controller)
   subsystem. The SDHC subsystem is for devices that *host* SD cards, while
   this subsystem is for devices that *act as* SD peripherals.

Architecture
************

The SD Device subsystem is organized into three layers:

.. code-block:: none

   ┌─────────────────────────────────────────┐
   │         Application / Network Stack      │
   ├─────────────────────────────────────────┤
   │       SD Device Subsystem (subsys/)      │
   │  sd_dev.c  sdio_dev.c  sd_dev_io.c      │
   │            sd_dev_pkt.c                  │
   ├─────────────────────────────────────────┤
   │    SD Device Driver API (drivers/sd_dev) │
   │         include/zephyr/drivers/sd_dev.h  │
   ├─────────────────────────────────────────┤
   │         Vendor HAL Driver                │
   └─────────────────────────────────────────┘

**Subsystem layer** (``subsys/sd_dev/``)
  Implements the SD/SDIO/MMC device-side protocol logic: CCCR/FBR
  configuration, function management, RX FIFO buffering, and packet
  dispatch. Vendor drivers only need to provide hardware-specific
  register access and DMA operations.

**Driver API layer** (``include/zephyr/drivers/sd_dev.h``)
  Defines the ``sd_dev_driver_api`` interface that all vendor drivers
  must implement. Provides syscall wrappers for register read/write,
  state management, and data transfer.

**Vendor HAL driver** (``drivers/sd_dev/<vendor>/``)
  Hardware-specific implementation. Handles low-level register access,
  interrupt handling, DMA configuration, and power management.

Supported Card Types
********************

The subsystem supports the following SD device types, selected via Kconfig:

* **SDIO device** (``CONFIG_SDIO_DEV``) — SDIO slave with up to 7 functions,
  CCCR/FBR register management, per-function RX FIFO, and block transfers.
* **MMC device** (``CONFIG_SDMMC_DEV``) — MMC slave (basic support,
  placeholder for future expansion).

Data Path
*********

The SD Device subsystem uses a packet abstraction (:c:struct:`sd_dev_pkt_t`)
for all data transfers. Packets are allocated from pre-allocated memory slabs
to avoid dynamic allocation in the data path.

**Receive path (host → device)**:

1. Vendor driver receives data via DMA/interrupt and calls
   :c:func:`sd_dev_rx_dispatch`.
2. The subsystem routes the packet to the appropriate SDIO function RX FIFO.
3. The application reads packets using :c:func:`sdio_read_pkt`.

**Transmit path (device → host)**:

1. Application allocates a packet with :c:func:`sd_dev_pkt_alloc`.
2. Application fills the packet buffer and calls :c:func:`sdio_write_pkt`
   or :c:func:`sdio_send_data`.
3. The vendor driver transmits the data over the SDIO bus.

.. note::
   The subsystem owns the RX packet buffer until the application calls
   :c:func:`sd_dev_pkt_free`. For TX packets, ownership transfers to the
   driver upon calling the send API; the driver is responsible for freeing
   the packet after transmission completes.

Initialization
**************

A typical initialization sequence:

.. code-block:: c

   #include <zephyr/sd_dev/sd_dev.h>

   const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(sdio_dev0));
   struct sd_dev_card *card;

   /* Initialize the SD device and allocate card context */
   ret = sd_dev_init(dev, &card);
   if (ret < 0) {
       LOG_ERR("SD device init failed: %d", ret);
       return ret;
   }

   /* Register RX callback */
   sd_dev_register_rx_cb(dev, my_rx_handler);

   /* Signal host that device is ready */
   sd_dev_set_dev_ready(dev);

Configuration
*************

The SD Device subsystem is configured via Kconfig:

* :kconfig:option:`CONFIG_SD_DEV` — Enable the SD Device framework.
* :kconfig:option:`CONFIG_SDIO_DEV` — Enable SDIO device protocol support.
* :kconfig:option:`CONFIG_SDIO_DEV_MAX_FUNCS` — Maximum number of SDIO
  functions (1–7, default 3).
* :kconfig:option:`CONFIG_SDEV_PKT_POOL_SIZE` — Number of packets in the
  TX/RX packet pool (default 16).
* :kconfig:option:`CONFIG_SDEV_PKT_BUF_SIZE` — Size of each packet buffer
  in bytes (default 4096).
* :kconfig:option:`CONFIG_SDEV_DMA_ALIGN` — DMA alignment in bytes
  (default 32).
* :kconfig:option:`CONFIG_SDEV_HEAP_SIZE` — Heap size for SD device
  internal allocations (default 4096).

Device Tree
***********

SD Device drivers are instantiated from Device Tree. The binding for SDIO
devices is defined in ``dts/bindings/sd_dev/``. Example overlay:

.. code-block:: devicetree

   &sdio_dev {
       status = "okay";
       num-funcs = <2>;
       vendor-id = <0x0000>;
       device-id = <0x0000>;
   };

Samples
*******

API Reference
*************

SD Device Driver APIs
=====================

.. doxygengroup:: sd_dev_interface

SD Device Packet APIs
=====================

.. doxygengroup:: sd_dev_pkt

SDIO Device APIs
================

.. doxygengroup:: sdio_dev
