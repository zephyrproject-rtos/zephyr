.. _microbit_line_follower_robot:

BBC micro:bit line-follower robot
#################################

Overview
********
This sample show makers and robotics engineers how to use a Zephyr
OS application to control a stand-alone line-following DFRobot Maqueen
robot chassis containing a BBC micro:bit board.

Building and running
********************

To build and run this sample you'll need a `DFRobot Maqueen robot
chassis (ROB0148) <https://www.dfrobot.com/product-1783.html>`_
with a BBC micro:bit board. Use black tape to create a line track
for the robot to follow. Build and flash the program to the BBC
micro:bit board (described below), turn on the robot,
and put it on the black line track.

Build this sample project using these commands:

.. zephyr-app-commands::
  :zephyr-app: samples/boards/bbc_microbit/line_follower_robot
  :board: bbc_microbit
  :goals: build
  :compact:

Then flash the program to the micro:bit board.

Sample Output
*************

The sample program controls the robot to follow a line track and does
not write to the console. You can watch this `robot video`_
to see it in action.

.. _robot video:
   https://youtu.be/tIvoHQjo8a4
