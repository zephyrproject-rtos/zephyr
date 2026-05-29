.. _sensor-trigger:

Sensor Triggers
###############

:dfn:`Triggers`, enumerated in :c:enum:`sensor_trigger_type`, are sensor
generated events. Typically sensors allow setting up these events to cause
digital line signaling for easy capture by a microcontroller. The events can
then commonly be inspected by reading registers to determine which event caused
the digital line signaling to occur.

There are many kinds of triggers sensors provide, from informative ones such as
data ready to physical events such as taps or steps.
