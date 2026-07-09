.. _sdio_api:

SDIO (role-neutral subsystem)
#############################

Overview
********

The SDIO subsystem models SDIO as a general-purpose transport layer.
It is usable both when an SoC drives an SDIO companion device (host/master)
and when an SoC *is* the companion device.

Typical users are Wi-Fi, Bluetooth and combo companion chips, vendor
mailbox/FIFO/register transports and custom co-processor links.

The subsystem is intentionally layered to keep these concepts role-neutral:

* a role-neutral **core** (:c:struct:`sdio_dev`, :c:struct:`sdio_function`)
  exposes the function I/O API and dispatches to a transport backend through
  :c:struct:`sdio_transport_api`;
* the **host role** binds the core to an :ref:`SD host controller <sdhc_api>`
  and provides a compatibility bridge from the SD-card stack
  (:c:func:`sdio_host_bind_card`);
* the **device/slave role** layers on an :ref:`SDIO device controller
  <sdio_dc_api>` to expose functions, register windows and FIFOs to a remote
  host.

The SD-card centered SDIO API (``include/zephyr/sd/sdio.h``,
:kconfig:option:`CONFIG_SDIO_STACK`) is unchanged and continues to work; the
new subsystem is additive.

.. note::

   The role-neutral subsystem (:kconfig:option:`CONFIG_SDIO`) and the legacy
   SD-card SDIO protocol support (:kconfig:option:`CONFIG_SDIO_STACK`) are
   distinct options. A host endpoint can be created from an already-enumerated
   card with :c:func:`sdio_host_bind_card`.

Core function I/O
*****************

Once an endpoint (:c:struct:`sdio_dev`) is initialized for a role, functions
are bound with :c:func:`sdio_func_bind` and accessed through a single set of
helpers regardless of role or backend:

* register access (CMD52): :c:func:`sdio_func_read_reg`,
  :c:func:`sdio_func_write_reg`, :c:func:`sdio_func_rw_reg`;
* fixed-address FIFO/data-port access (CMD53):
  :c:func:`sdio_func_read_fifo`, :c:func:`sdio_func_write_fifo`,
  :c:func:`sdio_func_read_blocks_fifo`, :c:func:`sdio_func_write_blocks_fifo`;
* incrementing-address windows (CMD53): :c:func:`sdio_func_read_addr`,
  :c:func:`sdio_func_write_addr`.

Host role
*********

:c:func:`sdio_host_init` binds an endpoint to an SDHC controller;
:c:func:`sdio_host_bind_card` wraps a card already enumerated by the SD stack.
After binding, the core function I/O helpers drive the bus.

Related configuration options:

* :kconfig:option:`CONFIG_SDIO`
* :kconfig:option:`CONFIG_SDIO_HOST`

Device/slave role
*****************

A device endpoint (:c:struct:`sdio_device`) is bound to an SDIO device
controller and exposes :c:struct:`sdio_device_function` instances. Each
function may back an incrementing-address register window with a buffer and/or
service fixed-address FIFO accesses through a callback. Incoming host accesses
are demultiplexed to the matching function;
:c:func:`sdio_device_raise_interrupt` signals the host.

Related configuration options:

* :kconfig:option:`CONFIG_SDIO`
* :kconfig:option:`CONFIG_SDIO_DEVICE`
* :kconfig:option:`CONFIG_SDIO_DC`

.. _sdio_dc_api:

SDIO device controller
**********************

An SDIO device controller (DC) is the slave-side counterpart of an SD host
controller. Host-only controllers implement the :ref:`SDHC API <sdhc_api>`,
device-only controllers implement the SDIO device controller API, and
dual-role hardware registers an instance of each. A software-only virtual
controller (:kconfig:option:`CONFIG_SDIO_DC_VIRTUAL`) provides an in-process
loopback transport for tests and samples on platforms without SDIO hardware.

API Reference
*************

.. doxygengroup:: sdio_core

.. doxygengroup:: sdio_host

.. doxygengroup:: sdio_device

.. doxygengroup:: sdio_dc_interface
