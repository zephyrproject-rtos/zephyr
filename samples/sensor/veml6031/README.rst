.. zephyr:code-sample:: veml6031
   :name: VEML6031 High Accuracy Ambient Light Sensor
   :relevant-api: sensor_interface

   Get ambient light data from a VEML4040 sensor (polling mode).

Overview
********

 This sample measures the ambient light for all possible combinations of sensor
 attributes. There are:
 - integration time
 - effective photodiode size
 - gain
 These attributes can be used to put the sensor in an optimal working area.
 When the light value reaches the maximum raw value (0xFFFF) an error is
 returned to indicate the out of bounds situation to the user program.
 With this program the raw value is also printed out together with the
 attributes to be able to select good attribute values.
 Interrupt and trigger mode is not supported so far, but planned for future
 development.

Requirements
************

 This sample uses the VEML6031 sensor controlled using the I2C-2 interface at
 the Olimex-STM32-E407 board on feather PF0 and PF1.

References
**********

 - VEML6031: https://www.vishay.com/docs/80007/veml6031x00.pdf

Building and Running
********************

 This project outputs sensor data to the console. It requires a VEML6031
 sensor to be connected to the desired board.

 .. zephyr-app-commands::
    :zephyr-app: samples/sensor/veml6031/
    :goals: build flash


Sample Output
=============

 .. code-block:: console

    Test all attributes for a good guess of attribute usage away of saturation.
    Light (lux):   6179 ALS (raw):   7100 IR (raw):     27   it: 0 div4: 0 gain: 0  --
    Light (lux):   1500 ALS (raw):   3447 IR (raw):     34   it: 0 div4: 0 gain: 1  --
    Light (lux):   4664 ALS (raw):   3537 IR (raw):     17   it: 0 div4: 0 gain: 2  --
    Light (lux):   5601 ALS (raw):   3218 IR (raw):     13   it: 0 div4: 0 gain: 3  --
    Light (lux):   1302 ALS (raw):    374 IR (raw):      5   it: 0 div4: 1 gain: 0  --
    Light (lux):   5584 ALS (raw):   3208 IR (raw):     11   it: 0 div4: 1 gain: 1  --
    Light (lux):   5285 ALS (raw):   1002 IR (raw):      3   it: 0 div4: 1 gain: 2  --
    Light (lux):   1455 ALS (raw):    209 IR (raw):      2   it: 0 div4: 1 gain: 3  --
    Light (lux):   4925 ALS (raw):  11317 IR (raw):     50   it: 1 div4: 0 gain: 0  --
    Light (lux):   3916 ALS (raw):  17999 IR (raw):     90   it: 1 div4: 0 gain: 1  --
    Light (lux):   2796 ALS (raw):   4299 IR (raw):     28   it: 1 div4: 0 gain: 2  --
    Light (lux):   5178 ALS (raw):   5950 IR (raw):     26   it: 1 div4: 0 gain: 3  --
    Light (lux):   4339 ALS (raw):   2493 IR (raw):     12   it: 1 div4: 1 gain: 0  --
    Light (lux):   2186 ALS (raw):   2512 IR (raw):     19   it: 1 div4: 1 gain: 1  --
    Light (lux):   5578 ALS (raw):   2115 IR (raw):      8   it: 1 div4: 1 gain: 2  --
    Light (lux):   4494 ALS (raw):   1291 IR (raw):      6   it: 1 div4: 1 gain: 3  --
    Light (lux):   3675 ALS (raw):  16892 IR (raw):     93   it: 2 div4: 0 gain: 0  --
    Light (lux):   3209 ALS (raw):  29495 IR (raw):    172   it: 2 div4: 0 gain: 1  --
    Light (lux):   4520 ALS (raw):  13710 IR (raw):     66   it: 2 div4: 0 gain: 2  --
    Light (lux):   4215 ALS (raw):   9687 IR (raw):     49   it: 2 div4: 0 gain: 3  --
    Light (lux):   3492 ALS (raw):   4013 IR (raw):     22   it: 2 div4: 1 gain: 0  --
    Light (lux):   3786 ALS (raw):   8700 IR (raw):     43   it: 2 div4: 1 gain: 1  --
    Light (lux):   4735 ALS (raw):   3591 IR (raw):     15   it: 2 div4: 1 gain: 2  --
    Light (lux):   3779 ALS (raw):   2171 IR (raw):     11   it: 2 div4: 1 gain: 3  --
    Light (lux):   3848 ALS (raw):  35376 IR (raw):    188   it: 3 div4: 0 gain: 0  --
    Light (lux):   3565 ALS (raw):  65535 IR (raw):    359   it: 3 div4: 0 gain: 1  --  OVERFLOW
    Light (lux):   4333 ALS (raw):  26297 IR (raw):    130   it: 3 div4: 0 gain: 2  --
    Light (lux):   3576 ALS (raw):  16435 IR (raw):     93   it: 3 div4: 0 gain: 3  --
    Light (lux):   4409 ALS (raw):  10133 IR (raw):     47   it: 3 div4: 1 gain: 0  --
    Light (lux):   3395 ALS (raw):  15606 IR (raw):     85   it: 3 div4: 1 gain: 1  --
    Light (lux):   4519 ALS (raw):   6854 IR (raw):     31   it: 3 div4: 1 gain: 2  --
    Light (lux):   3694 ALS (raw):   4245 IR (raw):     23   it: 3 div4: 1 gain: 3  --
    Light (lux):   3565 ALS (raw):  65535 IR (raw):    376   it: 4 div4: 0 gain: 0  --  OVERFLOW
    Light (lux):   1782 ALS (raw):  65535 IR (raw):    723   it: 4 div4: 0 gain: 1  --  OVERFLOW
    Light (lux):   3992 ALS (raw):  48450 IR (raw):    253   it: 4 div4: 0 gain: 2  --
    Light (lux):   3943 ALS (raw):  36247 IR (raw):    191   it: 4 div4: 0 gain: 3  --
    Light (lux):   3970 ALS (raw):  18248 IR (raw):     92   it: 4 div4: 1 gain: 0  --
    Light (lux):   3814 ALS (raw):  35064 IR (raw):    176   it: 4 div4: 1 gain: 1  --
    Light (lux):   4082 ALS (raw):  12381 IR (raw):     62   it: 4 div4: 1 gain: 2  --
    Light (lux):   4052 ALS (raw):   9311 IR (raw):     47   it: 4 div4: 1 gain: 3  --
    [...]
    Test finished.
