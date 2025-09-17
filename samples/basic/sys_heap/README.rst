.. zephyr:code-sample:: sys-heap
   :name: System heap

   Print system heap usage to the console.

Overview
********

A simple sample that can be used with any :ref:`supported board <boards>` and
prints system heap usage to the console.

Building
********

This application can be built on :zephyr:board:`native_sim <native_sim>` as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/sys_heap
   :host-os: unix
   :board: native_sim
   :goals: build
   :compact:

To build for another board, change "native_sim" above to that board's name.

Running
*******

Run build/zephyr/zephyr.exe

Sample Output
*************

.. code-block:: console

   System heap sample

   allocated 0, free 196, max allocated 0, heap size 256
   allocated 156, free 36, max allocated 156, heap size 256
   allocated 100, free 92, max allocated 156, heap size 256
   allocated 0, free 196, max allocated 156, heap size 256
   1 static heap(s) allocated:
         0 - address 0x80552a8 allocated 12, free 180, max allocated 12, heap size 256
   2 heap(s) allocated (including static):
         0 - address 0x80552a8 allocated 12, free 180, max allocated 12, heap size 256
         1 - address 0x805530c allocated 0, free 196, max allocated 156, heap size 256
