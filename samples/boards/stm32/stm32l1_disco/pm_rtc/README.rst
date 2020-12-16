# Still needs to be finished, when driver fully completed

.. _stm32-stm32l1_disco-pm_rtc-sample:

stm32l1_disco Power Management RTC system clock demo
#############################

Overview
********

This sample demonstrates the use of the STM32 RTC timer driver. The RTC is used 
as the kernel system clock. This is necessary to allow power 
management functionalities for STM32L1x SoCs.

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

*
*

Building, Flashing and Running
******************************

.. zephyr-app-commands::
   :zephyr-app: samples/boards/stm32l1_disco/pm_rtc
   :board: stm32l1_disco
   :goals: build flash
   :compact:

After flashing the device, run the code:

1. Unplug the USB cable from the discovery board and reconnect it to fully
   power-cycle it.
2. Open UART terminal.
3. Hit the Reset button on the discovery board.
***

Sample Output
=================

stm32l1_disco output
------------------------

.. code-block:: console

***