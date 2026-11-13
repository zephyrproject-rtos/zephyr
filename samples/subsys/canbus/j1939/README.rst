.. zephyr:code-sample:: j1939
   :name: J1939 sample
   :relevant-api: can_j1939

   Use J1939 library to exchange messages between two boards.

Overview
********
This sample demonstrates how to use the :ref:`J1939 library <can_j1939>`.

Address claim is done automatically by the library.
The application declares two fixed-address nodes with different NAMEs and
preferred addresses using :c:macro:`J1939_NODE_DEFINE`. It starts the CAN
controller before starting J1939 and checks both initialization results.

Product-specific identity can be supplied before startup with
:c:func:`j1939_configure_node`; applications do not need to modify address-claim
sources in the subsystem. See :ref:`can_j1939` for provisioning and
alternate-address policy.

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
