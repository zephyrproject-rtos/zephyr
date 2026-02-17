.. _fuel_gauge_api:

Fuel Gauge
##########

The fuel gauge subsystem exposes an API to uniformly access battery fuel gauge devices.

Basic Operation
***************

Properties
==========

Fundamentally, a property is a quantity that a fuel gauge device can measure.

Fuel gauges typically support multiple properties, such as temperature readings of the battery-pack
or present-time current/voltage.

Properties are fetched by the client one at a time using :c:func:`fuel_gauge_get_prop`, or fetched
in a batch using :c:func:`fuel_gauge_get_props`. Buffer properties, e.g. device name, are fetched by
using :c:func:`fuel_gauge_get_buffer_prop`.

Properties are set by the client one at a time using :c:func:`fuel_gauge_set_prop`, or set in a
batch using :c:func:`fuel_gauge_set_props`.

Property Units
==============

Property enum names and :c:struct:`fuel_gauge_prop_val` union fields carry an explicit unit suffix
so that the physical quantity and scale are unambiguous at the call site.

The following suffixes are used throughout the API:

.. list-table::
   :header-rows: 1
   :widths: 25 25

   * - Suffix
     - Unit
   * - ``_UA`` / ``_ua``
     - Microampere (µA)
   * - ``_UV`` / ``_uv``
     - Microvolt (µV)
   * - ``_UAH`` / ``_uah``
     - Microampere-hour (µAh)
   * - ``_MV`` / ``_mv``
     - Millivolt (mV)
   * - ``_DK`` / ``_dk``
     - Deci-Kelvin (0.1 K)
   * - ``_PCT`` / ``_pct``
     - Percent (%)
   * - ``_MINS`` / ``_mins``
     - Minutes

Mode-Dependent Properties
=========================

The following properties do **not** carry a unit suffix because their unit depends on
the runtime value of :c:enumerator:`FUEL_GAUGE_SBS_MODE` (CAPACITY_MODE):

.. list-table::
   :header-rows: 1
   :widths: 40 30 30

   * - Property / struct field
     - Unit when capacity mode
     - Unit when energy mode
   * - ``FUEL_GAUGE_DESIGN_CAPACITY`` / ``val.design_cap``
     - mAh
     - 10 mWh
   * - ``FUEL_GAUGE_SBS_ATRATE`` / ``val.sbs_at_rate``
     - mA
     - 10 mW
   * - ``FUEL_GAUGE_SBS_REMAINING_CAPACITY_ALARM`` / ``val.sbs_remaining_capacity_alarm``
     - mAh
     - 10 mWh

.. _fuel_gauge_deprecated_properties:

Deprecated Property Names
=========================

Prior to the introduction of unit suffixes, properties and union fields had no unit annotation
(e.g. :c:enumerator:`FUEL_GAUGE_CURRENT`, ``val.current``). These names are now **deprecated**
and will be removed in a future release.

Each deprecated name has a direct, value-identical replacement that carries the appropriate unit
suffix (e.g. :c:enumerator:`FUEL_GAUGE_CURRENT_UA`, ``val.current_ua``). The deprecated names are
aliased to their replacements so that existing code continues to compile without changes; however,
the compiler will emit deprecation warnings. Applications and drivers should migrate to the
unit-suffixed names.

.. list-table::
   :header-rows: 1
   :widths: 45 45

   * - Deprecated name
     - Replacement
   * - ``FUEL_GAUGE_AVG_CURRENT`` / ``val.avg_current``
     - ``FUEL_GAUGE_AVG_CURRENT_UA`` / ``val.avg_current_ua``
   * - ``FUEL_GAUGE_CURRENT`` / ``val.current``
     - ``FUEL_GAUGE_CURRENT_UA`` / ``val.current_ua``
   * - ``FUEL_GAUGE_VOLTAGE`` / ``val.voltage``
     - ``FUEL_GAUGE_VOLTAGE_UV`` / ``val.voltage_uv``
   * - ``FUEL_GAUGE_FULL_CHARGE_CAPACITY`` / ``val.full_charge_capacity``
     - ``FUEL_GAUGE_FULL_CHARGE_CAPACITY_UAH`` / ``val.full_charge_capacity_uah``
   * - ``FUEL_GAUGE_REMAINING_CAPACITY`` / ``val.remaining_capacity``
     - ``FUEL_GAUGE_REMAINING_CAPACITY_UAH`` / ``val.remaining_capacity_uah``
   * - ``FUEL_GAUGE_DESIGN_VOLTAGE`` / ``val.design_volt``
     - ``FUEL_GAUGE_DESIGN_VOLTAGE_MV`` / ``val.design_volt_mv``
   * - ``FUEL_GAUGE_TEMPERATURE`` / ``val.temperature``
     - ``FUEL_GAUGE_TEMPERATURE_DK`` / ``val.temperature_dk``
   * - ``FUEL_GAUGE_CHARGE_CURRENT`` / ``val.chg_current``
     - ``FUEL_GAUGE_CHARGE_CURRENT_UA`` / ``val.chg_current_ua``
   * - ``FUEL_GAUGE_CHARGE_VOLTAGE`` / ``val.chg_voltage``
     - ``FUEL_GAUGE_CHARGE_VOLTAGE_UV`` / ``val.chg_voltage_uv``
   * - ``FUEL_GAUGE_ABSOLUTE_STATE_OF_CHARGE`` / ``val.absolute_state_of_charge``
     - ``FUEL_GAUGE_ABSOLUTE_STATE_OF_CHARGE_PCT`` / ``val.absolute_state_of_charge_pct``
   * - ``FUEL_GAUGE_RELATIVE_STATE_OF_CHARGE`` / ``val.relative_state_of_charge``
     - ``FUEL_GAUGE_RELATIVE_STATE_OF_CHARGE_PCT`` / ``val.relative_state_of_charge_pct``
   * - ``FUEL_GAUGE_RUNTIME_TO_EMPTY`` / ``val.runtime_to_empty``
     - ``FUEL_GAUGE_RUNTIME_TO_EMPTY_MINS`` / ``val.runtime_to_empty_mins``
   * - ``FUEL_GAUGE_RUNTIME_TO_FULL`` / ``val.runtime_to_full``
     - ``FUEL_GAUGE_RUNTIME_TO_FULL_MINS`` / ``val.runtime_to_full_mins``
   * - ``FUEL_GAUGE_SBS_ATRATE_TIME_TO_FULL`` / ``val.sbs_at_rate_time_to_full``
     - ``FUEL_GAUGE_SBS_ATRATE_TIME_TO_FULL_MINS`` / ``val.sbs_at_rate_time_to_full_mins``
   * - ``FUEL_GAUGE_SBS_ATRATE_TIME_TO_EMPTY`` / ``val.sbs_at_rate_time_to_empty``
     - ``FUEL_GAUGE_SBS_ATRATE_TIME_TO_EMPTY_MINS`` / ``val.sbs_at_rate_time_to_empty_mins``
   * - ``FUEL_GAUGE_SBS_REMAINING_TIME_ALARM`` / ``val.sbs_remaining_time_alarm``
     - ``FUEL_GAUGE_SBS_REMAINING_TIME_ALARM_MINS`` / ``val.sbs_remaining_time_alarm_mins``
   * - ``FUEL_GAUGE_STATE_OF_CHARGE_ALARM`` / ``val.state_of_charge_alarm``
     - ``FUEL_GAUGE_STATE_OF_CHARGE_ALARM_PCT`` / ``val.state_of_charge_alarm_pct``
   * - ``FUEL_GAUGE_LOW_VOLTAGE_ALARM`` / ``val.low_voltage_alarm``
     - ``FUEL_GAUGE_LOW_VOLTAGE_ALARM_UV`` / ``val.low_voltage_alarm_uv``
   * - ``FUEL_GAUGE_HIGH_VOLTAGE_ALARM`` / ``val.high_voltage_alarm``
     - ``FUEL_GAUGE_HIGH_VOLTAGE_ALARM_UV`` / ``val.high_voltage_alarm_uv``
   * - ``FUEL_GAUGE_LOW_CURRENT_ALARM`` / ``val.low_current_alarm``
     - ``FUEL_GAUGE_LOW_CURRENT_ALARM_UA`` / ``val.low_current_alarm_ua``
   * - ``FUEL_GAUGE_HIGH_CURRENT_ALARM`` / ``val.high_current_alarm``
     - ``FUEL_GAUGE_HIGH_CURRENT_ALARM_UA`` / ``val.high_current_alarm_ua``
   * - ``FUEL_GAUGE_LOW_TEMPERATURE_ALARM`` / ``val.low_temperature_alarm``
     - ``FUEL_GAUGE_LOW_TEMPERATURE_ALARM_DK`` / ``val.low_temperature_alarm_dk``
   * - ``FUEL_GAUGE_HIGH_TEMPERATURE_ALARM`` / ``val.high_temperature_alarm``
     - ``FUEL_GAUGE_HIGH_TEMPERATURE_ALARM_DK`` / ``val.high_temperature_alarm_dk``
   * - ``FUEL_GAUGE_LOW_GPIO_ALARM`` / ``val.low_gpio_alarm``
     - ``FUEL_GAUGE_LOW_GPIO_ALARM_UV`` / ``val.low_gpio_alarm_uv``
   * - ``FUEL_GAUGE_HIGH_GPIO_ALARM`` / ``val.high_gpio_alarm``
     - ``FUEL_GAUGE_HIGH_GPIO_ALARM_UV`` / ``val.high_gpio_alarm_uv``
   * - ``FUEL_GAUGE_GPIO_VOLTAGE`` / ``val.gpio_voltage``
     - ``FUEL_GAUGE_GPIO_VOLTAGE_UV`` / ``val.gpio_voltage_uv``

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
