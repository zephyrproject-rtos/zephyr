.. zephyr:code-sample:: zbus-hello-world
   :name: zbus Hello World
   :relevant-api: zbus_apis

   Make three threads talk to each other using zbus.

Overview
********
This sample implements a simple hello world application using zbus to make the threads talk to each other.

Building and Running
********************

This project outputs to the console.  It can be built and executed
on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/zbus/hello_world
   :host-os: unix
   :board: qemu_x86
   :goals: run

Sample Output
=============

.. code-block:: console

    D: Sensor sample started raw reading, version 0.1-2!
    D: Channel list:
    D: 0 - Channel acc_data:
    D:       Message size: 12
    D:       Observers:
    D:       - my_listener
    D:       - my_subscriber
    D: 1 - Channel version:
    D:       Message size: 4
    D:       Observers:
    D: Observers list:
    D: 0 - Listener my_listener
    D: 1 - Subscriber my_subscriber
    D: START processing channel acc_data change
    D: From listener -> Acc x=1, y=1, z=1
    D: FINISH processing channel acc_data change
    D: From subscriber -> Acc x=1, y=1, z=1
    D: START processing channel acc_data change
    D: From listener -> Acc x=2, y=2, z=2
    D: FINISH processing channel acc_data change
    D: From subscriber -> Acc x=2, y=2, z=2

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.
