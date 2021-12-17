.. _rtc:

Real Time Clock driver sample
#############################

Overview
********

This sample demonstrates how to use the RTC driver API.

Sample output
=============

In the main.c, select TEST_WUT to test Wake Up Timer or select TEST_ALARM to test AlarmA and AlarmB

If TEST_WUT is selected, 3 wake up timer callback messages are generated every 10s then every 5s.
You should get a similar output as below with TEST_WUT:

.. code-block:: console
    
    ...
    epoch:1634826602 wd:2 2021/9/21 14:30:2
    epoch:1634826604 wd:2 2021/9/21 14:30:4
    epoch:1634826606 wd:2 2021/9/21 14:30:6
    epoch:1634826608 wd:2 2021/9/21 14:30:8
    wakeup_timer_callback: 1        epoch:1634826610 wd:2 2021/9/21 14:30:10
    epoch:1634826612 wd:2 2021/9/21 14:30:12
    epoch:1634826614 wd:2 2021/9/21 14:30:14
    epoch:1634826616 wd:2 2021/9/21 14:30:16
    epoch:1634826618 wd:2 2021/9/21 14:30:18
    wakeup_timer_callback: 2        epoch:1634826620 wd:2 2021/9/21 14:30:20
    epoch:1634826622 wd:2 2021/9/21 14:30:22
    epoch:1634826624 wd:2 2021/9/21 14:30:24
    epoch:1634826626 wd:2 2021/9/21 14:30:26
    epoch:1634826628 wd:2 2021/9/21 14:30:28
    wakeup_timer_callback: 3        epoch:1634826630 wd:2 2021/9/21 14:30:30
    ...


If TEST_ALARM is selected, only one AlarmB message is generated after 10s and AlarmA messsages are generated every 60s.
You should get a similar output as below with TEST_ALARM:

.. code-block:: console

    ...
    epoch:1634826600 wd:2 2021/9/21 14:30:0
    epoch:1634826602 wd:2 2021/9/21 14:30:2
    epoch:1634826604 wd:2 2021/9/21 14:30:4
    epoch:1634826606 wd:2 2021/9/21 14:30:6
    epoch:1634826608 wd:2 2021/9/21 14:30:8
    alarmB_callback: 1      epoch:1634826610 wd:2 2021/9/21 14:30:10
    epoch:1634826612 wd:2 2021/9/21 14:30:12
    ...
    epoch:1634826628 wd:2 2021/9/21 14:30:28
    alarmA_callback: 1      epoch:1634826630 wd:2 2021/9/21 14:30:30
    epoch:1634826632 wd:2 2021/9/21 14:30:32
    ...
    epoch:1634826688 wd:2 2021/9/21 14:31:28
    alarmA_callback: 2      epoch:1634826690 wd:2 2021/9/21 14:31:30
    epoch:1634826692 wd:2 2021/9/21 14:31:32
    ...

