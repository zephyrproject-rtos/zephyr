
EADOGS164: Display I2C
#################################################

Description
***********

EADOGS164 display is used to show the progress.
The basic functions are implemented. For example;
printing on x,y coordinate upto single line. As
the display supports 3,4 line options but only
4 line is considered in this implementation.
Besides, curse on/off, reading and writing
to memory is implemented.
More complex implementation is left for future
and is left for users of the display.

Building and Running
********************

This project send data to the EADOGS164 display.
It should work with any platform featuring a I2C peripheral
interface. The display has been tested with disco_l475_iot1
board.

.. zephyr-app-commands::
   :zephyr-app: samples/display/eadogs164
   :board: disco_l475_iot1
   :goals: build flash

Sample Output
=============
Sends Hello_World and reads back from DDRAM of LCD
