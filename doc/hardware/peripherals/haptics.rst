.. _haptics_api:

Haptics
#######

Overview
********

The haptics API allows for the control of haptic driver devices for the
purposes of performing haptic feedback events.

During a haptic feedback event the haptic device drives a signal to an
actuator. The source of the haptic event signal varies depending on the
capabilities of the haptic device.

Some examples of haptic signal sources are analog signals, preprogrammed
(ROM) wavetables, synthesized (RAM) wavetables, and digital audio streams.

Additionally, haptic driver devices often offer controls for adjusting and
tuning the drive signal to meet the electrical requirements of their respective
actuators.

API Reference
*************

.. doxygengroup:: haptics_interface
