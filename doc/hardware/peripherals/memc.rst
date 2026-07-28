.. _memc_api:

Memory Controller (MEMC)
########################

Overview
********

The MEMC API provides a generic interface for accessing external memory
devices such as PSRAM and NOR flash. It supports two access modes:

- **Memory-mapped:** The controller exposes the device through a direct
  CPU-accessible address window. The driver may implement
  :c:member:`memc_driver_api.get_mem_base` to advertise the mapped base
  address, allowing :c:func:`memc_read` and :c:func:`memc_write` to use
  ``memcpy`` automatically. Alternatively, the driver may provide no runtime
  API at all if the hardware maps memory transparently - in this case upper
  layers access the device directly using a platform-specific base address.

- **Bus transaction:** The controller issues explicit bus commands (e.g.,
  MSPI) for each transfer. The driver implements
  :c:member:`memc_driver_api.read` and :c:member:`memc_driver_api.write`.
  Used when no memory-mapped aperture exists on the controller.

:c:func:`memc_read` and :c:func:`memc_write` automatically select the
memory-mapped path when available, falling back to bus transactions otherwise.

Additional optional APIs are provided for device introspection:
:c:func:`memc_get_size` returns the device capacity and
:c:func:`memc_read_id` returns device identification bytes.

Legacy (Init-Only) Drivers
**************************

Existing memc drivers that pass ``NULL`` as the API pointer to
``DEVICE_DT_INST_DEFINE`` are not affected by this API. Callers should
use :c:macro:`DEVICE_API_IS` to test whether a device implements the
memc interface before calling any memc function:

.. code-block:: c

   if (DEVICE_API_IS(memc, dev)) {
      memc_read(dev, offset, buf, sizeof(buf));
   } else {
      /* Legacy init-only driver - device is memory-mapped.
       * Access via platform-specific base address.
       */
      memcpy(buf, (const uint8_t *)MEMC_BASE + offset, sizeof(buf));
   }

Calling :c:func:`memc_read` or :c:func:`memc_write` on a device that
does not implement the memc API returns ``-ENOTSUP``.

Configuration Options
*********************

Related configuration options:

* :kconfig:option:`CONFIG_MEMC`
* :kconfig:option:`CONFIG_MEMC_INIT_PRIORITY`

API Reference
*************

.. doxygengroup:: memc_interface
