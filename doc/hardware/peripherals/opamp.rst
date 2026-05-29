.. _opamp_api:

Operational Amplifier (OPAMP)
#############################

Overview
********

The operational amplifier is an analog device that amplifies differential
input signals (difference between inverting and non-inverting input) to
give a resulting output voltage.


Configuration
*************

When OPAMP is enabled, an initial configuration shall be provided using
the devicetree. The OPAMP gain can be adjusted at runtime.

Related configuration options:

* :kconfig:option:`CONFIG_OPAMP`

API Reference
*************

.. doxygengroup:: opamp_interface
