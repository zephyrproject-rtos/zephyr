.. _charger_api:

Chargers
########

The charger subsystem exposes an API to uniformly access battery charger devices.

A charger device, or charger peripheral, is a device used to take external power provided to the
system as an input and provide power as an output downstream to the battery pack(s) and system.
The charger device can exist as a module, an integrated circuit, or as a functional block in a power
management integrated circuit (PMIC).

The action of charging a battery pack is referred to as a charge cycle. When the charge cycle is
executed the battery pack is charged according to the charge profile configured on the charger
device. The charge profile is defined in the battery pack's specification that is provided by the
manufacturer. On charger devices with a control port, the charge profile can be configured by the
host controller by setting the relevant properties, and can be adjusted at runtime to respond to
environmental changes.

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
