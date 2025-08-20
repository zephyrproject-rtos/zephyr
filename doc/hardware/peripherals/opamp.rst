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

When SoC/MCU internal OPAMP enabled, an initial configuration must be provided using
the devicetree. The OPAMP can configure the gain of the non-inverting or inverting at
runtime using device driver API.

API Reference
*************

.. doxygengroup:: opamp_interface
