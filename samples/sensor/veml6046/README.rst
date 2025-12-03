.. zephyr:code-sample:: veml6046
   :name: VEML6046 RGBIR Color Sensor
   :relevant-api: sensor_interface

   Get red, green, blue and IR light data from a VEML6046 sensor (polling
   mode).

Overview
********

 This sample measures the red, green, blue and IR light for all possible
 combinations of sensor attributes. They are:

 - integration time
 - effective photodiode size
 - gain

 These attributes can be used to put the sensor in an optimal working area.
 When the light value reaches the maximum raw value (0xFFFF), an error is
 returned to indicate the out of bounds situation to the user program.
 With this program the raw value is also printed out together with the
 attributes to be able to select good attribute values.
 Interrupt and trigger modes are not supported so far, but planned for future
 development.

Requirements
************

 This sample uses the VEML6046 sensor controlled using the I2C-2 interface of
 the Olimex-STM32-E407 board on Feather connector pins PF0 and PF1.

References
**********

 - VEML6046: https://www.vishay.com/docs/80173/veml6046x00.pdf
 - Application note: https://www.vishay.com/docs/80410/designingveml6046x00.pdf

Building and Running
********************

 This project outputs sensor data to the console. It requires a VEML6046
 sensor to be connected to the desired board.

 .. zephyr-app-commands::
    :zephyr-app: samples/sensor/veml6046/
    :goals: build flash
    :board: olimex_stm32_e407


Sample Output
=============

 .. code-block:: console

    Test all attributes for a good guess of attribute usage away of saturation.
    Red:     68 lx (    51) green:      68 lx (    84) blue:     68 lx (    51) IR:      68 lx (    27)   it: 0 pdd: 0 gain: 0  --
    Red:    121 lx (   181) green:     121 lx (   347) blue:    121 lx (   240) IR:     121 lx (    53)   it: 0 pdd: 0 gain: 1  --
    Red:    215 lx (   106) green:     215 lx (   226) blue:    215 lx (   160) IR:     215 lx (    19)   it: 0 pdd: 0 gain: 2  --
    Red:    201 lx (    75) green:     201 lx (   156) blue:    201 lx (   112) IR:     201 lx (    14)   it: 0 pdd: 0 gain: 3  --
    [...]
    Test finished.
