.. camera:

Camera Sample Application
###################################

Overview
********
This project demos camera samples.
It should work with any platform featuring a I2C peripheral interface and a CMOS sensor interface.
It does not work on QEMU.
If there is a display device on the board, the demo displays camera stream on the display device.

Requirements
************
This project requires a CMOS sensor connected to board.

Building and Running
********************
This sample can be built for a ``mimxrt1050_evk`` board with mt9m114 CMOS sensor connected.

.. zephyr-app-commands::
   :zephyr-app: samples/camera
   :board: mimxrt1050_evk
   :goals: build flash

To run this sample, a CMOS sensor should be connected to the board.
The sample outputs the FPS(frames per second) number on the console in capture
mode and preview mode periodically.
If there is a display device present on the board, the sample outputs the camera
stream on the display device.
