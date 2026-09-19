.. _ppc_api:

Peripheral Protection Controller (PPC)
######################################

Overview
********

A Peripheral Protection Controller (PPC) is a bus-level filter that assigns
peripherals (or peripheral register groups) to the Secure or Non-Secure world,
independently of the CPU's own security attribution unit. On Arm TrustZone-M
systems the Secure image programs the PPCs to hand the peripherals the
Non-Secure world needs to it before leaving the Secure state.

The class exposes a single entry point, :c:func:`ppc_configure_ns_all`, which
walks every ``status = "okay"`` PPC instance in the devicetree and opens the
peripheral regions it is configured to expose. A driver indicates the API is
available by selecting :kconfig:option:`CONFIG_PPC`.

API Reference
*************

.. doxygengroup:: ppc_interface
