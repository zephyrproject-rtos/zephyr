TODOOOOOOOO (if you read this I definitely forgot it)

.. _stm32-stm32l1_disco-rtc_systemclock-sample:

stm32l1_disco RTC system clock demo
#############################

Overview
********

This sample demonstrates the use of the STM32 RTC timer driver. The RTC is used 
as the kernel system clock. This is useful especially when combined with power 
management functionalities, but for this a separate sample will be made available.

By using the RTC as the kernel system clock, the kernel's system clock is continuous 
(since the RTC doesn't shut off during sleep), and monotonic (if configuration isn't changed, 
e.g. no timezone change is implemented). 

The driver configures the RTC's prescalers depending on the source chosen (LSI/LSE, both 
available on stm32l1_disco board), to have 1Hz frequency. This is required for correct 
'time-passing'. 
The prescaler configuration has a large influence on the correct workings of the timer driver, 
so please take caution if you change it. See 'AN4759 Rev 5' and 'RM0038 Rev 16' for more 
information. 
CONFIG_SYS_CLOCK_TICKS_PER_SEC and CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC are set according to the 
highest granularity possible (depending on the RTC source and prescaler configuration). This 
is heavily dependent on the sub-second functionality, since without it the highest granularity 
would be 1 (1 tick per second). By including the sub-second alarm functionality, this is 
increased to the highest sub-second alarm frequency (dependent on the synchronous prescaler 
value) and sub-second alarm bit mask chosen ([1] in our case). 


#something about ticks per sec default (prescaler+1)/2, but can be any /2 of that?
#something about all the default configs (ticks_per_sec, prescalers, rtc source, ...?), 
#    for board, soc, driver, sample?
#something about combined used with counter driver? (maybe works, not tested, but shouldn't 
#  change the epoch/time of the rtc

The sample will run multiple threads to test whether scheduling is managed properly.


The functional behavior is:
* ...
* ...


Requirements
************

This application uses the CC13x2/CC26x2 LaunchPad for the demo.

Building, Flashing and Running
******************************

.. zephyr-app-commands::
   :zephyr-app: samples/boards/cc13x2_cc26x2/system_off
   :board: cc1352r1_launchxl
   :goals: build flash
   :compact:

After flashing the device, run the code:

1. Unplug the USB cable from the LaunchPad and reconnect it to fully
   power-cycle it.
2. Open UART terminal.
3. Hit the Reset button on the LaunchPad.
4. Device will turn off the external flash, which is on by default, to
   reduce power consumption.
5. Device will demonstrate active, idle and standby modes.
6. Device will explicitly turn itself off by going into shutdown mode.
   Press Button 1 to wake the device and restart the application as if
   it had been powered back on.

Sample Output
=================

cc1352rl_launchxl output
------------------------

.. code-block:: console

        *** Booting Zephyr OS build zephyr-v2.2.0-664-gd774727cc66e  ***

        cc1352r1_launchxl system off demo
        Busy-wait 5 s
        Sleep 2000 us (IDLE)
        Sleep 3 s (STANDBY)
        Entering system off (SHUTDOWN); press BUTTON1 to restart
