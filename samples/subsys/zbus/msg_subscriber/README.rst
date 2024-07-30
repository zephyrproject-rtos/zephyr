.. zephyr:code-sample:: zbus-msg-subscriber
   :name: Message subscriber
   :relevant-api: zbus_apis

   Use zbus message subscribers to listen to messages published to channels.

Overview
********
This sample illustrates how to use a message subscriber in different ways with other types of
observers. It is possible to explore the pool isolation feature by setting the pool size and if it
is static or dynamic by setting the proper :kconfig:option:`CONFIG_ZBUS_MSG_SUBSCRIBER_BUF_ALLOC`.

Building and Running
********************

This project outputs to the console.  It can be built and executed
on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/zbus/msg_subscriber
   :host-os: unix
   :board: qemu_x86
   :goals: run

Sample Output
=============

.. code-block:: console

   -- west build: running target run
   [0/1] To exit from QEMU enter: 'CTRL+a, x'[QEMU] CPU: qemu32,+nx,+pae
   Booting from ROM..
   I: ----> Publishing to acc_data_chan channel
   I:  AL Memory allocated 28 bytes. Total allocated 28 bytes
   I: From listener foo_lis -> Acc x=1, y=10, z=100
   I: From msg subscriber bar_msg_sub1 -> Acc x=1, y=10, z=100
   I: From msg subscriber bar_msg_sub2 -> Acc x=1, y=10, z=100
   I: From msg subscriber bar_msg_sub3 -> Acc x=1, y=10, z=100
   I: From msg subscriber bar_msg_sub4 -> Acc x=1, y=10, z=100
   I: From msg subscriber bar_msg_sub5 -> Acc x=1, y=10, z=100
   I: From msg subscriber bar_msg_sub6 -> Acc x=1, y=10, z=100
   I: From msg subscriber bar_msg_sub7 -> Acc x=1, y=10, z=100
   I: From msg subscriber bar_msg_sub8 -> Acc x=1, y=10, z=100
   I: From msg subscriber bar_msg_sub9 -> Acc x=1, y=10, z=100
   I: From msg subscriber bar_msg_sub10 -> Acc x=1, y=10, z=100
   I: From msg subscriber bar_msg_sub11 -> Acc x=1, y=10, z=100
   I: From msg subscriber bar_msg_sub12 -> Acc x=1, y=10, z=100
   I: From msg subscriber bar_msg_sub13 -> Acc x=1, y=10, z=100
   I: From msg subscriber bar_msg_sub14 -> Acc x=1, y=10, z=100
   I: From msg subscriber bar_msg_sub15 -> Acc x=1, y=10, z=100
   I:  FR Memory freed 28 bytes. Total allocated 0 bytes
   I: From msg subscriber bar_msg_sub16 -> Acc x=1, y=10, z=100
   I: From subscriber bar_sub1 -> Acc x=1, y=10, z=100
   I: From subscriber bar_sub2 -> Acc x=1, y=10, z=100
   I: ----> Publishing to acc_data_chan channel
   I:  AL Memory allocated 28 bytes. Total allocated 28 bytes
   I: From listener foo_lis -> Acc x=2, y=20, z=200
   I: From msg subscriber bar_msg_sub1 -> Acc x=2, y=20, z=200
   I: From msg subscriber bar_msg_sub2 -> Acc x=2, y=20, z=200
   I: From msg subscriber bar_msg_sub3 -> Acc x=2, y=20, z=200
   I: From msg subscriber bar_msg_sub4 -> Acc x=2, y=20, z=200
   I: From msg subscriber bar_msg_sub5 -> Acc x=2, y=20, z=200
   I: From msg subscriber bar_msg_sub6 -> Acc x=2, y=20, z=200
   I: From msg subscriber bar_msg_sub7 -> Acc x=2, y=20, z=200
   I: From msg subscriber bar_msg_sub8 -> Acc x=2, y=20, z=200
   I: From msg subscriber bar_msg_sub9 -> Acc x=2, y=20, z=200
   I: From msg subscriber bar_msg_sub10 -> Acc x=2, y=20, z=200
   I: From msg subscriber bar_msg_sub11 -> Acc x=2, y=20, z=200
   I: From msg subscriber bar_msg_sub12 -> Acc x=2, y=20, z=200
   I: From msg subscriber bar_msg_sub13 -> Acc x=2, y=20, z=200
   I: From msg subscriber bar_msg_sub14 -> Acc x=2, y=20, z=200
   I: From msg subscriber bar_msg_sub15 -> Acc x=2, y=20, z=200
   I:  FR Memory freed 28 bytes. Total allocated 0 bytes
   I: From msg subscriber bar_msg_sub16 -> Acc x=2, y=20, z=200
   I: From subscriber bar_sub1 -> Acc x=2, y=20, z=200
   I: From subscriber bar_sub2 -> Acc x=2, y=20, z=200
   <continues>

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.
