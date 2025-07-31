.. _pwm_sample:

PWM Sample
##########

Overview
********

This sample demonstrates PWM (Pulse Width Modulation) control using the
LPC54S018's SCTimer peripheral. The sample shows three common PWM use cases:

1. LED brightness control (dimming)
2. Servo motor position control
3. DC motor speed control

The SCTimer provides 6 independent PWM channels with configurable frequency
and duty cycle.

Wiring
******

The sample uses the following pins:

- P3.0 (SCT0_OUT0): PWM Channel 0 - LED dimming
- P3.1 (SCT0_OUT1): PWM Channel 1 - Servo control
- P3.2 (SCT0_OUT2): PWM Channel 2 - Motor speed
- P3.4 (SCT0_OUT3): PWM Channel 3 - Available
- P3.5 (SCT0_OUT4): PWM Channel 4 - Available
- P3.6 (SCT0_OUT5): PWM Channel 5 - Available

Connect:
- LED with current limiting resistor to Channel 0
- Servo motor signal wire to Channel 1
- Motor driver PWM input to Channel 2

Building and Running
********************

.. code-block:: console

   west build -b lpcxpresso54s018 samples/drivers/pwm
   west flash

Sample Output
*************

.. code-block:: console

   *** Booting Zephyr OS build v4.2.0 ***
   [00:00:00.000,000] <inf> pwm_sample: PWM sample application for LPC54S018
   [00:00:00.001,000] <inf> pwm_sample: PWM device ready, starting demonstration
   [00:00:01.000,000] <inf> pwm_sample: Channel 0: duty cycle = 0%, pulse = 0 us
   [00:00:01.100,000] <inf> pwm_sample: Channel 0: duty cycle = 5%, pulse = 1000 us
   [00:00:01.100,000] <inf> pwm_sample: Servo channel 1: angle = 9 degrees, pulse = 1050 us
   [00:00:01.200,000] <inf> pwm_sample: Channel 0: duty cycle = 10%, pulse = 2000 us

The sample will:
- Fade the LED from 0% to 100% brightness and back
- Sweep the servo from 0 to 180 degrees and back
- Ramp the motor speed up and down

API Usage
*********

Basic PWM control:

.. code-block:: c

   /* Set 50% duty cycle on channel 0 */
   uint32_t period = PWM_PERIOD_US;
   uint32_t pulse = period / 2;
   pwm_set(pwm_dev, 0, period, pulse, 0);

Servo control (1-2ms pulse):

.. code-block:: c

   /* Set servo to 90 degrees */
   uint32_t pulse = 1500; /* 1.5ms */
   pwm_set(pwm_dev, 1, 20000, pulse, 0);

Advanced Features
*****************

The SCTimer supports:
- Up to 16 independent outputs
- Counter synchronization
- Dead-time insertion for motor control
- Pattern generation
- Event-triggered operation

Configuration Options
********************

Adjust prescaler in device tree for different frequencies:

.. code-block:: dts

   &sctimer0 {
       prescaler = <1>;   /* No prescaling - highest frequency */
       prescaler = <128>; /* Divide by 128 - lowest frequency */
   };