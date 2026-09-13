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

Shell commands
**************

.. zephyr:shell-module:: haptics
   :kconfig: CONFIG_HAPTICS_SHELL
   :depends: CONFIG_HAPTICS

   The ``haptics`` command allows starting and stopping the output of a haptic driver device,
   selecting its source, adjusting its output level and running its calibration through an
   interactive interface.

.. rubric:: Command reference

.. zephyr:shell-command-reference::

API Reference
*************

.. doxygengroup:: haptics_interface
