.. _fuel_gauge_api:

Fuel Gauges (Experimental API Stub Doc)
#######################################

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

Properties are fetched using a client allocated array of :c:struct:`fuel_gauge_get_property`.  This
array is then populated by values as according to its `property_type` field.

Caching
=======

The Fuel Gauge API explicitly provides no caching for its clients.


.. _fuel_gauge_api_reference:

API Reference
*************

.. doxygengroup:: fuel_gauge_interface
