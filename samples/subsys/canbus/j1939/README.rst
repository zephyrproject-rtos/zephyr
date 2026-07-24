.. zephyr:code-sample:: j1939
   :name: J1939 sample
   :relevant-api: can_j1939

   Use J1939 library to exchange messages between two boards.

Overview
********
This sample demonstrates how to use the :ref:`J1939 library <can_j1939>`.

Address claim is done automatically by the library.

A periodic PGN is sent.

Building
********
.. zephyr-app-commands::
   :zephyr-app: samples/subsys/canbus/j1939
   :host-os: unix
   :board: native_sim
   :goals: build

Running
*******

Run build/zephyr/zephyr.exe
