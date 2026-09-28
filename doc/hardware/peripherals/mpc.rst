.. _mpc_api:

Memory Protection Controller (MPC)
##################################

Overview
********

A Memory Protection Controller (MPC) is a bus-level filter that splits a
memory into Secure and Non-Secure regions, independently of the CPU's own
security attribution unit. On Arm TrustZone-M systems the Secure image
programs the MPCs to hand specific memory ranges to the Non-Secure world
before it leaves the Secure state.

The class exposes a single entry point, :c:func:`mpc_configure_all`, which
walks every ``status = "okay"`` MPC instance in the devicetree and applies the
regions described by its child nodes. The concrete driver is selected
automatically from the ``compatible`` of each MPC node.

API Reference
*************

.. doxygengroup:: mpc_interface
