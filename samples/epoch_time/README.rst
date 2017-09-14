.. _Epoch Time Sample App:

Epoch Time Sample App
######################

Overview
********
This sample demostrates that gettimeofday() and settimeofday() works on frdm
k64f.

Building and Running
********************

This sample sets the system time and output system time every 5 seconds to the
console. It can be built and executed on frdm k64f as follows:

.. code-block:: console

   $ cd samples/epoch_time
   $ make flash

Sample Output
=============

.. code-block:: console

    settimeofday: 0
    gettimeofday: 0, sec: 150529005
    gettimeofday: 0, sec: 150529010
    gettimeofday: 0, sec: 150529015

