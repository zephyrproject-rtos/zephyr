.. _pwm_test:

Test for PWM
###########

Overview
********
This is a sample PWM application.
This test verifies the APIs for PWM IP

To validate the output in PWM mode, you need to connect the oscilloscope
to pin1 on pwm0. 

Below parameters need to be changed as per platform -
a. PWM_CHANNEL
b. PWM_LABEL

The granularity of duty cycles depends on the input clock speed of the PWM IP
on the platform; higher the input clock, better accuracy of duty cycles.

Building and Running
********************
Standard build procedure.

Sample Output
=============

[PWM] Bind Success
[PWM] Running rate: 32768
[PWM] Generating pulses-Period: 100000000 nsec, Pulse: 20000000 nsec
[PWM] Generating pulses-Period: 8192 cycles, Pulse: 4096 cycles
