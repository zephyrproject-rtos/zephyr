.. zephyr:code-sample:: zbus-work-queue
   :name: Work queue
   :relevant-api: zbus_apis

   Use a work queue to process zbus messages in various ways.

Overview
********
This sample implements an application using zbus to illustrate three different reaction options. First, the observer can react "instantaneously" by using a listener callback. It can schedule a job, pushing that to the system work queue. Last, it can wait and execute that in a subscriber thread.

Building and Running
********************

This project outputs to the console.  It can be built and executed
on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/zbus/work_queue
   :host-os: unix
   :board: qemu_x86
   :goals: run

Sample Output
=============

.. code-block:: console

    I: Sensor msg processed by CALLBACK fh1: temp = 10, press = 1, humidity = 100
    I: Sensor msg processed by CALLBACK fh2: temp = 10, press = 1, humidity = 100
    I: Sensor msg processed by CALLBACK fh3: temp = 10, press = 1, humidity = 100
    I: Sensor msg processed by WORK QUEUE handler dh1: temp = 10, press = 1, humidity = 100
    I: Sensor msg processed by WORK QUEUE handler dh2: temp = 10, press = 1, humidity = 100
    I: Sensor msg processed by WORK QUEUE handler dh3: temp = 10, press = 1, humidity = 100
    I: Sensor msg processed by THREAD handler 1: temp = 10, press = 1, humidity = 100
    I: Sensor msg processed by THREAD handler 2: temp = 10, press = 1, humidity = 100
    I: Sensor msg processed by THREAD handler 3: temp = 10, press = 1, humidity = 100
    I: Sensor msg processed by CALLBACK fh1: temp = 20, press = 2, humidity = 200
    I: Sensor msg processed by CALLBACK fh2: temp = 20, press = 2, humidity = 200
    I: Sensor msg processed by CALLBACK fh3: temp = 20, press = 2, humidity = 200
    I: Sensor msg processed by WORK QUEUE handler dh1: temp = 20, press = 2, humidity = 200
    I: Sensor msg processed by WORK QUEUE handler dh2: temp = 20, press = 2, humidity = 200
    I: Sensor msg processed by WORK QUEUE handler dh3: temp = 20, press = 2, humidity = 200
    I: Sensor msg processed by THREAD handler 1: temp = 20, press = 2, humidity = 200
    I: Sensor msg processed by THREAD handler 2: temp = 20, press = 2, humidity = 200
    I: Sensor msg processed by THREAD handler 3: temp = 20, press = 2, humidity = 200
    I: Sensor msg processed by CALLBACK fh1: temp = 30, press = 3, humidity = 300
    I: Sensor msg processed by CALLBACK fh2: temp = 30, press = 3, humidity = 300
    I: Sensor msg processed by CALLBACK fh3: temp = 30, press = 3, humidity = 300
    I: Sensor msg processed by WORK QUEUE handler dh1: temp = 30, press = 3, humidity = 300
    I: Sensor msg processed by WORK QUEUE handler dh2: temp = 30, press = 3, humidity = 300
    I: Sensor msg processed by WORK QUEUE handler dh3: temp = 30, press = 3, humidity = 300
    I: Sensor msg processed by THREAD handler 1: temp = 30, press = 3, humidity = 300
    I: Sensor msg processed by THREAD handler 2: temp = 30, press = 3, humidity = 300
    I: Sensor msg processed by THREAD handler 3: temp = 30, press = 3, humidity = 300
    <continues>

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.
