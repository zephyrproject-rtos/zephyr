.. _opamp_api:

Operational Amplifier (OPAMP)
#############################

Overview
********

The operational amplifier is an analog device that amplifies differential input
signals (difference between inverting and non-inverting input) to give a resulting
output voltage.


Configuration
*************

When internal OPAMP is enabled, an initial configuration must be provided using
the devicetree. The OPAMP can configure non-inverting or inverting gain at runtime
using device driver API.

Related configuration options:

* :kconfig:option:`CONFIG_OPAMP`

API Reference
*************

.. doxygengroup:: opamp_interface
