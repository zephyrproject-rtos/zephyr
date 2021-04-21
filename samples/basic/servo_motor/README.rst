.. _servo-motor-sample:

Servomotor
##########

Overview
********

This is a sample app which drives a servomotor using PWM.

The sample rotates a servomotor back and forth in the 180 degree range with a
PWM control signal.

This app is targeted for servomotor ROB-09065. The corresponding PWM pulse
widths for a 0 to 180 degree range are 700 to 2300 microseconds, respectively.
Different servomotors may require different PWM pulse widths, and you may need
to modify the source code if you are using a different servomotor.

Requirements
************

You will see this error if you try to build this sample for an unsupported
board:

.. code-block:: none

   Unsupported board: pwm-servo devicetree alias is not defined

The sample requires a servomotor whose signal pin is connected to a PWM
device's channel 0. The PWM device must be configured using the ``pwm-servo``
:ref:`devicetree <dt-guide>` alias. Usually you will need to set this up via a
:ref:`devicetree overlay <set-devicetree-overlays>` like so:

.. code-block:: DTS

   / {
   	aliases {
   		pwm-servo = &some_pwm_node;
   	};
   };

Where ``some_pwm_node`` is the node label of a PWM device in your system.

See :zephyr_file:`samples/basic/servo_motor/boards/bbc_microbit.overlay` for an
example.

Wiring
******

BBC micro:bit
=============

You will need to connect the motor's red wire to external 5V, the black wire to
ground and the white wire to the SCL pin, i.e. pin 21 on the edge connector.

Building and Running
********************

The sample has a devicetree overlay for the :ref:`bbc_microbit`.

This sample can be built for multiple boards, in this example we will build it
for the bbc_microbit board:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/servo_motor
   :board: bbc_microbit
   :goals: build flash
   :compact:
