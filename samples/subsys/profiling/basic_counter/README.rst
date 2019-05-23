.. _profiling_counter_sample:

Profiling Counter
#################

Overview
********
This is a simple application that demonstrates use of the profiling subsytem
to track the execution time of an arbitrary section of code.

Building and Running
********************

This project outputs multiple log message to the console.  It can be built and
executed on the nrf52840_pca10056 as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/subsys/profiling/basic_counter
   :host-os: unix
   :board: nrf52840_pca10056
   :goals: run
   :compact:


Sample Output
=============

.. code-block:: console

        [00:00:00.000,000] <inf> profiling: started basic_counter
        [00:00:00.000,000] <inf> profiling: basic_counter: 337 ticks
        [00:00:00.000,000] <inf> profiling: basic_counter: 14616 ticks
        [00:00:00.000,000] <inf> profiling: basic_counter: 29906 ticks
        [00:00:00.000,000] <inf> profiling: basic_counter: 45194 ticks
        [00:00:00.000,000] <inf> profiling: basic_counter: 60482 ticks
        [00:00:00.000,152] <inf> profiling: basic_counter: 75770 ticks
        [00:00:00.000,396] <inf> profiling: basic_counter: 91058 ticks
        [00:00:00.000,640] <inf> profiling: basic_counter: 106346 ticks
        [00:00:00.000,885] <inf> profiling: basic_counter: 121634 ticks
        [00:00:00.001,129] <inf> profiling: basic_counter: 137731 ticks
        [00:00:00.001,373] <inf> profiling: stopped basic_counter: 154016 ticks
