.. _fuel_gauge_api:

Fuel Gauge
##########

The fuel gauge subsystem exposes an API to uniformly access battery fuel gauge devices. Currently,
only reading data is supported.

Note: This API is currently experimental and this doc will be significantly changed as new features
are added to the API.

Basic Operation
***************

Properties
==========

Fundamentally, a property is a quantity that a fuel gauge device can measure.

Fuel gauges typically support multiple properties, such as temperature readings of the battery-pack
or present-time current/voltage.

Properties are fetched by the client one at a time using :c:func:`fuel_gauge_get_prop`, or fetched
in a batch using :c:func:`fuel_gauge_get_props`.

Properties are set by the client one at a time using :c:func:`fuel_gauge_set_prop`, or set in a
batch using :c:func:`fuel_gauge_set_props`.

Battery Cutoff
==============

Many fuel gauges embedded within battery packs expose a register address that when written to with a
specific payload will do a battery cutoff. This battery cutoff is often referred to as ship, shelf,
or sleep mode due to its utility in reducing battery drain while devices are stored or shipped.

The fuel gauge API exposes battery cutoff with the :c:func:`fuel_gauge_battery_cutoff` function.

Caching
=======

The Fuel Gauge API explicitly provides no caching for its clients.


.. _fuel_gauge_api_reference:

API Reference
*************

.. doxygengroup:: fuel_gauge_interface
.. doxygengroup:: fuel_gauge_emulator_backend
