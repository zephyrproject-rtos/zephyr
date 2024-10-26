.. zephyr:code-sample:: servo-motor
   :name: Servomotor
   :relevant-api: pwm_interface

   Drive a servomotor using the PWM API.

Overview
********

This is a sample app which drives a servomotor using the :ref:`PWM API <pwm_api>`.

The sample rotates a servomotor back and forth in the 180 degree range with a
PWM control signal.

This app is targeted for servomotor ROB-09065. The corresponding PWM pulse
widths for a 0 to 180 degree range are 700 to 2300 microseconds, respectively.
Different servomotors may require different PWM pulse widths, and you may need
to modify the source code if you are using a different servomotor.

Requirements
************

The sample requires a servomotor whose signal pin is connected to a pin driven
by PWM. The servo must be defined in Devicetree using the ``pwm-servo``
compatible (part of the sample) and setting its node label to ``servo``. You
will need to do something like this:

.. code-block:: devicetree

   / {
       servo: servo {
           compatible = "pwm-servo";
           pwms = <&pwm0 1 PWM_MSEC(20) PWM_POLARITY_NORMAL>;
           min-pulse = <PWM_USEC(700)>;
           max-pulse = <PWM_USEC(2500)>;
       };
   };

Note that a commonly used period value is 20 ms. See
:zephyr_file:`samples/basic/servo_motor/boards/bbc_microbit.overlay` for an
example.

Wiring
******

BBC micro:bit
=============

You will need to connect the motor's red wire to external 5V, the black wire to
ground and the white wire to the SCL pin, i.e. pin P19 on the edge connector.

Building and Running
********************

The sample has a devicetree overlay for the :zephyr:board:`bbc_microbit`.

This sample can be built for multiple boards, in this example we will build it
for the bbc_microbit board:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/servo_motor
   :board: bbc_microbit
   :goals: build flash
   :compact:
