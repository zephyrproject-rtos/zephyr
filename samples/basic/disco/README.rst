.. _disco-sample:

Disco demo
##########

Overview
********

A simple 'disco' demo. The demo assumes that 2 LEDs are connected to
GPIO outputs of the MCU/board.

Requirements
************

This demo assumes there are two LEDs connected to two GPIO lines. These two
LEDs should be given aliases as 'led0' and 'led1' in the board dts file so the
configure process will automatically create these four define macros in generated
board head file
:file:`samples/basic/disco/build/zephyr/include/generated/generated_dts_board.h`.

 - LED0_GPIO_CONTROLLER
 - LED0_GPIO_PIN
 - LED1_GPIO_CONTROLLER
 - LED1_GPIO_PIN

Building and Running
*********************

After startup, the program looks up two predefined GPIO line defined in
devicetree, and configures them in output mode. During each  iteration of the
main loop, the state of GPIO lines will be changed so that one of the lines is
in high state, while the other is in low, thus switching the LEDs on and off in
an alternating pattern.

This project does not output to the serial console, but instead causes two LEDs
connected to the GPIO device to blink in an alternating pattern.

The sample can be found here: :file:`samples/basic/disco`.

Nucleo F103RB
=============

.. zephyr-app-commands::
   :zephyr-app: samples/basic/disco
   :board: nucleo_f103rb
   :goals: build
   :compact:

Nucleo F401RE
=============

.. zephyr-app-commands::
   :zephyr-app: samples/basic/disco
   :board: nucleo_f401re
   :goals: build
   :compact:

Arduino 101
============

.. zephyr-app-commands::
   :zephyr-app: samples/basic/disco
   :board: arduino_101
   :goals: build
   :compact:
