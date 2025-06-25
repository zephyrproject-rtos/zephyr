.. zephyr:code-sample:: capture
   :name: PWM Capture
   :relevant-api: pwm_interface

   Implement an application using the PWM capture API.

Overview
********
This sample provides an example of PWM capture application using :ref:`PWM API <pwm_api>`.
It gets the PWM capture cycles in periodic, prints pulse width & period in loop.

Requirements
************

This sample requires the support of a timer IP compatible with pwm capture block.

Building and Running
********************

 .. zephyr-app-commands::
    :zephyr-app: samples/drivers/pwm/capture
    :host-os: unix
    :board: lp_mspm0g3507
    :goals: run
    :compact:

Sample Output
=============

 .. code-block:: console

    PWM capture lp_mspm0g3507/mspm0g350

    timclk 80000000 Hz
    {period:1000  pulse width: 499} in TIMCLK cycle
    {period: 80000.000000 Hz duty: 49.900002}

    <repeats endlessly>
