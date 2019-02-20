.. _net_offload_interface:

Network Offloding
#################

Overview
********

The network offloading API provides hooks that a device vendor can use
to provide an alternate implementation for an IP stack. This means that the
actual network connection creation, data transfer, etc., is done in the vendor
HAL instead of the Zephyr network stack.

API Reference
*************

.. doxygengroup:: net_offload
   :project: Zephyr
