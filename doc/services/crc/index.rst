.. _crc:

CRC
###

Overview
********

The CRC subsystem provides software implementations of various Cyclic Redundancy Check algorithms
for data integrity verification.
CRCs are commonly used to detect accidental changes to data in storage and communication systems.

The subsystem offers a comprehensive set of CRC algorithms including CRC-4, CRC-7, CRC-8, CRC-16,
CRC-24, and CRC-32 variants, and provides optional hardware acceleration support when available.

.. note::
   This library is distinct from the :ref:`CRC hardware driver API <crc_api>`, which provides an
   interface to hardware CRC acceleration peripherals.

   When a hardware CRC unit is available and the ``zephyr,crc`` property has been set in the
   ``/chosen`` node in Devicetree, the library functions will default to using hardware
   acceleration for improved performance (can be disabled by setting
   :kconfig:option:`CONFIG_CRC_HW_HANDLER` to ``n``).

Usage
=====

To compute a CRC, include the appropriate header and call the desired function:

.. code-block:: c

   #include <zephyr/sys/crc.h>

   uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
   uint32_t checksum = crc32_ieee(data, sizeof(data));

For streaming data processed in chunks, use the "update" variants:

.. code-block:: c

   uint32_t crc = 0;
   crc = crc32_ieee_update(crc, chunk1, len1);
   crc = crc32_ieee_update(crc, chunk2, len2);
   /* Final CRC value is in 'crc' */

The generic :c:func:`crc_by_type` function provides a unified interface to select the CRC algorithm
at runtime.

Configuration
*************

Related configuration options:

* :kconfig:option:`CONFIG_CRC` - Enable CRC support
* :kconfig:option:`CONFIG_CRC_HW_HANDLER` - Enable hardware CRC acceleration
* :kconfig:option:`CONFIG_CRC_SHELL` - Enable CRC shell commands
* :kconfig:option-regex:`CONFIG_CRC[0-9].*` - Enable software implementations of specific CRC
  algorithms

API Reference
*************

.. doxygengroup:: crc
