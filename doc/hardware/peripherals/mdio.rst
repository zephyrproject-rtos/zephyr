.. _mdio_api:

Management Data Input/Output (MDIO)
###################################

Overview
********

MDIO is a bus that is commonly used to communicate with ethernet PHY devices.
Many ethernet MAC controllers also provide hardware to communicate over MDIO
bus with a peripheral device.

This API is intended to be used primarily by PHY drivers but can also be
used by user firmware.

API Reference
*************

.. doxygengroup:: mdio_interface
   :project: Zephyr
