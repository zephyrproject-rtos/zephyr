.. _regulator_api:

Regulators
##########

This subsystem provides control of voltage and current regulators. A common
example is a GPIO that controls a transistor that supplies current to a device
that is not always needed. Another example is a PMIC, typically a much more
complex device.

The ``*-supply`` devicetree properties are used to identify the regulator(s)
that a devicetree node directly depends on. Within the driver for the node the
regulator API is used to issue requests for power when the device is to be
active, and release the power request when the device shuts down.

The simplest case where a regulator is needed is one where there is only one
client. For those situations the cost of using the regulator device
infrastructure is not justified, and ``*-gpios`` devicetree properties should be
used. There is no device interface to these regulators as they are entirely
controlled within the driver for the corresponding node, e.g. a sensor.

.. _regulator_api_reference:

API Reference
**************

.. doxygengroup:: regulator_interface
