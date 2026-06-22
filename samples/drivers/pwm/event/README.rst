.. zephyr:code-sample:: event
   :name: PWM Event
   :relevant-api: pwm_interface

   Events on a PWM signal.

Overview
********
This sample provides an example application using the :ref:`PWM API <pwm_api>` event
API to get events on a PWM signal. The sample demonstrates how events can be used to
precisely time x periods of a PWM signal. A usecase for this would be the transmitting
part of an IR remote, where precise pulse timing is needed.

Requirements
************

This sample requires the support of the :ref:`PWM API <pwm_api>` event API.

Building and Running
********************

 .. zephyr-app-commands::
    :zephyr-app: samples/drivers/pwm/event
    :host-os: unix
    :board: sam4s_xplained
    :goals: run
    :compact:

Sample Output
=============

When capturing the signal using an oscilloscope or logic analyzer, it should look like
the following repeating sequence (after a startup transient)

 .. code-block::

     ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐                   ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐
    ─┘ └─┘ └─┘ └─┘ └─┘ └───────────────────┘ └─┘ └─┘ └─┘ └─┘ └─────
