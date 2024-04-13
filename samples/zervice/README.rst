.. _zervice:

Zervice
#######

Overview
********

A simple sample to demonstrater some zervice capabilities. It simply
run a series of zervice and print a result confirming that each one has been
run.

Building and Running
********************

This application can be built and executed on FRDM-K64F as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/zervice
   :board: frdm_k64f
   :goals: flash

Sample Output
=============

.. code-block:: console

    after_console from zervice!
    *** Booting Zephyr OS build v3.6.0-2517-g895f2b751fd1 ***
    third from zervice!
    fourth from zervice!
    first: 1
    second: 1
    third: 1
    fourth: 1
    console: 1
