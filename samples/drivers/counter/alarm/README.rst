.. _alarm_sample:

Counter Alarm Sample
#####################

Overview
********
This sample provides an example of alarm application using counter API.
It sets an alarm with an initial delay of 2 seconds. At each alarm
expiry, a new alarm is configured with a delay multiplied by 2.

.. note::
   In case of 1Hz frequency (RTC for example), precision is 1 second.
   Therefore, the sample output may differ in 1 second

Requirements
************

This sample requires the support of a timer IP compatible with alarm setting.

References
**********

- :ref:`disco_l475_iot1_board`

Building and Running
********************

 .. zephyr-app-commands::
    :zephyr-app: samples/drivers/counter/alarm
    :host-os: unix
    :board: disco_l475_iot1
    :goals: run
    :compact:

Sample Output
=============

 .. code-block:: console

    Counter alarm sample

    Set alarm in 2 sec
    !!! Alarm !!!
    Now: 2
    Set alarm in 4 sec
    !!! Alarm !!!
    Now: 6
    Set alarm in 8 sec
    !!! Alarm !!!
    Now: 14
    Set alarm in 16 sec
    !!! Alarm !!!
    Now: 30

    <repeats endlessly>
