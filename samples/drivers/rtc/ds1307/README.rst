DS1307: Tiny RTC Module
#######################################################

Overview
********
This sample application periodically reads date and time data from the DS1307 RTC module, and displays on the console.

Requirements
************

The DS1307 is a real-time clock module that keeps track of the current date and time using a 32.768 kHz crystal oscillator. It communicates via I2C and includes a battery backup to maintain time even when the main power is off.

References
**********

For more information about the DS1307 RTC module, see 
https://www.analog.com/en/products/ds1307.html

Building and Running
********************

This project outputs rtc date and time to the console. It requires a DS1307
system-in-package.

.. zephyr-app-commands::
   :zephyr-app: samples/rtc/ds1307
   :board: esp32_devkitc_wroom
   :goals: build
   :compact:

Sample Output
=============
 Current RTC time: 2024-07-28 02:46:00

 Current RTC time: 2024-07-28 02:46:01

 Current RTC time: 2024-07-28 02:46:02

 Current RTC time: 2024-07-28 02:46:03

 Current RTC time: 2024-07-28 02:46:04

 Current RTC time: 2024-07-28 02:46:05

.. code-block:: console

   <repeats endlessly every 1 seconds>
