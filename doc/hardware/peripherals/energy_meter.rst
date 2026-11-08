.. _energy_meter_api:

Energy Meter
############

The energy meter API gives uniform access to electrical energy metering ICs, which measure the
voltages and currents of a single-phase or poly-phase installation and derive power, power
factor, frequency and accumulated energy.

Basic Operation
***************

Quantities and lines
====================

A value is addressed by an :c:type:`energy_meter_channel`: a quantity, such as
:c:enumerator:`ENERGY_METER_VOLTAGE` or :c:enumerator:`ENERGY_METER_ACTIVE_ENERGY_IMPORT`, on a
line. Lines A, B and C are the phases of a three-phase installation; a single-phase meter reports
its line on :c:enumerator:`ENERGY_METER_LINE_A`. :c:enumerator:`ENERGY_METER_LINE_N` is the
neutral, and :c:enumerator:`ENERGY_METER_LINE_TOTAL` holds sums over the phases and values that
belong to the installation as a whole, such as the line frequency.

Values are signed 64-bit integers in millionths of the SI unit of the quantity: microvolts,
microamperes, microwatts, microwatt-hours and so on. Active power is positive when energy flows
from the supply to the load.

Drivers can define device-specific quantities, such as harmonic power or status registers,
starting at :c:enumerator:`ENERGY_METER_QUANTITY_PRIV_START`.

Reading values
==============

:c:func:`energy_meter_get_info` returns the number of phases and, for each line, the quantities
the device supports. :c:func:`energy_meter_get` reads a batch of channels, which the driver reads
from the device in one go where the hardware allows it, and :c:func:`energy_meter_get_one` reads a
single value.

:c:func:`energy_meter_get_snapshot` reads every supported quantity into an
:c:struct:`energy_meter_snapshot`, a structure with one named field per quantity for each line,
and marks which fields hold a value.

Energy
======

Energy quantities return the energy accumulated since the device was initialized. Metering ICs
usually keep energy in short registers that are cleared when read, so drivers accumulate them in
software; applications must read energy often enough for the device registers not to overflow, as
documented by each driver. :c:func:`energy_meter_reset_energy` sets the accumulated energy to zero.

Events
======

Energy pulse outputs, interrupt outputs and fault signals are reported through a callback set with
:c:func:`energy_meter_set_event_callback`.

Shell
*****

With :kconfig:option:`CONFIG_ENERGY_METER_SHELL`, the ``energy_meter`` shell command lists what a
meter measures (``info``), prints all values once (``get``) or periodically (``watch``), reads a
single common or device-specific quantity (``read``) and resets the accumulated energy
(``reset_energy``).

.. code-block:: console

   uart:~$ energy_meter get meter@0
                                                 a              b              c          total
   voltage [V]                             230.125        229.870        231.004              -
   current [A]                               5.123          4.998          5.051              -
   active_power [W]                       1150.320       1120.110       1149.870       3420.300
   power_factor                              0.985          0.982          0.987          0.985
   frequency [Hz]                                -              -              -         50.020

.. _energy_meter_api_reference:

API Reference
*************

.. doxygengroup:: energy_meter_interface
