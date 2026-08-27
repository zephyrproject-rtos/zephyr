.. zephyr:code-sample:: zbus-async-listeners
   :name: zbus Async Listeners
   :relevant-api: zbus_apis

   Demonstrate the usage of zbus async listeners.

Overview
********
This sample demonstrates how to use zbus asynchronous listeners by implementing a simple
application. The main thread submits an event to a channel that has three listeners: (i) a
conventional listener, (ii) an asynchronous listener that utilizes the system's work queue, and
(iii) an asynchronous listener that employs an isolated work queue.


Building and Running
********************

This project outputs to the console.  It can be built and executed
on QEMU as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/zbus/async_listeners/
   :host-os: unix
   :board: qemu_x86
   :goals: run

Sample Output
=============

.. code-block:: console

   I: Output                       | Thread Context
   I: =============================================
   I: From listener -> Evt=0       | main
   I: From async listener -> Evt=0 | sysworkq
   I: From async listener -> Evt=0 | My work queue
   I: From listener -> Evt=1       | main
   I: From async listener -> Evt=1 | sysworkq
   I: From async listener -> Evt=1 | My work queue
   I: From listener -> Evt=2       | main
   I: From async listener -> Evt=2 | sysworkq
   I: From async listener -> Evt=2 | My work queue
   I: From listener -> Evt=3       | main
   I: From async listener -> Evt=3 | sysworkq
   I: From async listener -> Evt=3 | My work queue
   I: =============================================
   I: From listener -> Evt=0       | main
   I: From async listener -> Evt=0 | sysworkq
   I: From async listener -> Evt=0 | My work queue
   I: From listener -> Evt=1       | main
   I: From async listener -> Evt=1 | sysworkq
   I: From async listener -> Evt=1 | My work queue
   I: From listener -> Evt=2       | main
   I: From async listener -> Evt=2 | sysworkq
   I: From async listener -> Evt=2 | My work queue
   I: From listener -> Evt=3       | main
   I: From async listener -> Evt=3 | sysworkq
   I: From async listener -> Evt=3 | My work queue

Exit QEMU by pressing :kbd:`CTRL+A` :kbd:`x`.
