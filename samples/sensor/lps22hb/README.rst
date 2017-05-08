LPS22HB: Pressure and Temperature Monitor
#########################################

Overview
********
This sample periodically reads temperature and pressure values from LPS22HB
Sensor and displays them on console


Requirements
************

This sample uses the LPS22HB sensor, controlled using the I2C interface.

References
**********

 - LPS22HB: http://www.st.com/en/mems-and-sensors/lps22hb.html

 Building and Running
 ********************

 This project outputs sensor data to the console. It requires a LPS22HB
 sensor, which is present on the disco_l475_iot1 board.

 .. code-block:: console

    $ cd samples/sensors/lps22hb
    $ make BOARD=disco_l475_iot1


 Sample Output
 =============

 .. code-block:: console

   Temperature: 28.33 C; Pressure: 1013.93 mb
   Temperature: 28.33 C; Pressure: 1013.96 mb
   Temperature: 28.35 C; Pressure: 1013.97 mb
   Temperature: 28.35 C; Pressure: 1013.97 mb

   <repeats endlessly every second>
