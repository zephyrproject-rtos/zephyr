.. _charger_api:

Chargers
########

The charger subsystem exposes an API to uniformly access battery charger devices. Currently,
only reading data is supported.

Basic Operation
***************

Properties
==========

Fundamentally, a property is a configurable setting, state, or quantity that a charger device can
measure.

Chargers typically support multiple properties, such as temperature readings of the battery-pack
or present-time current/voltage.

Properties are fetched using a client allocated array of :c:struct:`charger_get_property`.  This
array is then populated by values as according to its `property_type` field.

.. _charger_api_reference:

API Reference
*************

.. doxygengroup:: charger_interface
