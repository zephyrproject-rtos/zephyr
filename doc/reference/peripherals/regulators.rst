.. _regulator_api:

Regulators
##########

This subsystem provides control of voltage and current regulators.  A
common example is a GPIO that controls a transistor that supplies
current to a device that is not always needed.

Conceptually regulators have two modes: off and on.  A transition
between modes may involve a time delay, so operations on regulators are
inherently asynchronous.  To maximize flexibility the
:ref:`resource_mgmt_onoff` infrastructure is used in the generic API for
the regulator subsystem.  Nodes with a devicetree compatible of
``regulator-fixed`` are the most common flexible regulators.

In some cases the transitions are close enough to instantaneous that the
the asynchronous driver implementation is not needed, and the resource
cost in RAM is not justified.  Such a regulator still uses the
asynchronous API, but may be implemented internally in a way that
ensures the result of the operation is presented before the transition
completes.  Zephyr recognizes devicetree nodes with a compatible of
``regulator-fixed-sync`` as devices with synchronous transitions.

The ``vin-supply`` devicetree property is used to identify the
regulator(s) that a devicetree node directly depends on.  Within the
driver for the node the regulator API is used to issue requests for
power when the device is to be active, and release the power request
when the device shuts down.

The simplest case where a regulator is needed is one where there is only
one client.  For those situations the cost of using even the optimized
synchronous regulator device infrastructure is not justified, and the
``supply-gpios`` devicetree property should be used.  There is no device
interface to these regulators as they are entirely controlled within the
driver for the corresponding node, e.g. a sensor.

.. _regulator_api_reference:

API Reference
**************

.. doxygengroup:: regulator_interface
   :project: Zephyr
