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

    I: Sensor sample started raw reading, version 0.1-2!
    I: Channel list:
    I: 0 - Channel acc_data_chan:
    I:       Message size: 12
    I:       Observers:
    I:       - foo_lis
    I:       - bar_sub
    I:       - baz_async_lis
    I: 1 - Channel simple_chan:
    I:       Message size: 4
    I:       Observers:
    I: 2 - Channel version_chan:
    I:       Message size: 4
    I:       Observers:
    I: Observers list:
    I: 0 - Subscriber bar_sub
    I: 1 - Subscriber baz_async_lis
    I: 2 - Listener foo_lis
    I: From listener -> Acc x=1, y=1, z=1
    I: From async listener -> Acc x=1, y=1, z=1
    I: From subscriber -> Acc x=1, y=1, z=1
    I: From listener -> Acc x=2, y=2, z=2
    I: From async listener -> Acc x=2, y=2, z=2
    I: From subscriber -> Acc x=2, y=2, z=2
    I: Pub a valid value to a channel with validator successfully.
    I: Pub an invalid value to a channel with validator successfully.

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.
