.. zephyr:code-sample:: zbus-confirmed-channel
   :name: Confirmed channel
   :relevant-api: zbus_apis

   Use confirmed zbus channels to ensure all subscribers consume a message.

Overview
********
This sample implements a simple way of using confirmed channels in zbus.
The confirmed channel can only be published when all the subscribers consume the message.

Building and Running
********************

This project outputs to the console.  It can be built and executed
on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/zbus/confirmed_channel
   :host-os: unix
   :board: qemu_x86
   :goals: run

Sample Output
=============

.. code-block:: console

   I: From listener -> Confirmed message payload = 0
   I: From bar_sub2 subscriber -> Confirmed message payload = 0
   I: From bar_sub1 subscriber -> Confirmed message payload = 0
   I: From bar_sub3 subscriber -> Confirmed message payload = 0
   I: From listener -> Confirmed message payload = 1
   I: From bar_sub2 subscriber -> Confirmed message payload = 1
   I: From bar_sub1 subscriber -> Confirmed message payload = 1
   I: From bar_sub3 subscriber -> Confirmed message payload = 1
   I: From listener -> Confirmed message payload = 2
   I: From bar_sub2 subscriber -> Confirmed message payload = 2
   I: From bar_sub1 subscriber -> Confirmed message payload = 2
   I: From bar_sub3 subscriber -> Confirmed message payload = 2
   I: From listener -> Confirmed message payload = 3
   I: From bar_sub2 subscriber -> Confirmed message payload = 3
   I: From bar_sub1 subscriber -> Confirmed message payload = 3
   I: From bar_sub3 subscriber -> Confirmed message payload = 3
   <continues>

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.
