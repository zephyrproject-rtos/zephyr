PWM: Servo motor
################

Overview
********

This is a sample app which drives a servo motor using
PWM.

This app is targeted for servo motor ROB-09065. With the
PWM control signal, the servo motor can rotate to any
angle between 0 and 180 degrees. The corresponding PWM
pulse width is between 700 micro seconds and 2300 micro
seconds. The motor is programmed to rotate back and forth
in the 180 degree range.

Since different servo motors may require different PWM
pulse width, you may need to modify the pulse width in
the app if you are using a different servo motor.

Wiring
******

Arduino 101 and Quark D2000 CRB
===============================

You will need to connect the motor's red wire to 5v,
the black wire to ground and the white wire to PWM 0 via
the shield.

Building and Running
********************

This sample can be built for multiple boards, in this example we will build it
for the arduino_101 board:

.. code-block:: console

   $ cd samples/basic/servo_motor
   $ make BOARD=arduino_101
   $ make BOARD=arduino_101 flash
