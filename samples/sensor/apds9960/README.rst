RGB and Gesture Sensor
######################

Overview
********

This sample utilizes APDS-9960 Sensor and reads RGB values from the sensor
then displays the color through the APA102C LED.


.. note::
   This sample does not use the Zephyr sensor APIs

Wiring
******

The SparkFun RGB and Gesture Sensor was being used:

- https://www.sparkfun.com/products/12787
- https://www.adafruit.com/product/2343

For APA102C, on the sensor subsystem (ARC) side of Arduino 101:

1. GPIO_SS_2 is on AD0 (for APA102C data)
2. GPIO_SS_3 is on AD1 (for APA102C clock)

The GPIO driver is being used for bit-banging to control the APA102C LED.

The APA102/C requires 5V data and clock signals, so logic level shifter
(preferred) or pull-up resistors are needed.  Make sure the pins are 5V
tolerant if using pull-up resistors.

.. important::

   The APA102C are very bright even at low settings.  Protect your eyes
   and do not look directly into those LEDs.

Building and Running
********************

This sample can be built for multiple boards, in this example we will build it
for the Arduino 101 board:

.. code-block:: console

   $ cd samples/sensors/apds9960
   $ make BOARD=arduino_101_sss
   $ make BOARD=arduino_101_sss flash # with JTAG
   .. or
   $ dfu-util -a sensor_core -D outdir/arduino_101_sss/zephyr.bin # with DFU
