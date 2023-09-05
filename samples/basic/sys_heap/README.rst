.. zephyr:code-sample:: sys-heap
   :name: System heap

   Print system heap usage to the console.

Overview
********

A simple sample that can be used with any :ref:`supported board <boards>` and
prints system heap usage to the console.

Building
********************

This application can be built on native_posix as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/basic/sys_heap
   :host-os: unix
   :board: native_posix
   :goals: build
   :compact:

To build for another board, change "native_posix" above to that board's name.

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
