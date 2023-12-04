.. _charger_api:

Chargers
########

The charger subsystem exposes an API to uniformly access battery charger devices.

Basic Operation
***************

Initiating a Charge Cycle
=========================

A charge cycle is initiated or terminated using :c:func:`charger_charge_enable`.

Properties
==========

Fundamentally, a property is a configurable setting, state, or quantity that a charger device can
measure.

Chargers typically support multiple properties, such as temperature readings of the battery-pack
or present-time current/voltage.

Properties are fetched by the client one at a time using :c:func:`charger_get_prop`.
Properties are set by the client one at a time using :c:func:`charger_set_prop`.

.. _charger_api_reference:

API Reference
*************

.. doxygengroup:: charger_interface
