.. _smp_pi:

SMP Pi
###########

Overview
********
This sample application calculates Pi independently in many threads, and
demonstrates the benefit of multiple execution units (CPU cores)
when compute-intensive tasks can be run in parallel, with
no cross-dependencies or shared resources.

By changing the value of CONFIG_MP_MAX_NUM_CPUS on SMP systems, you
can see that using more cores takes almost linearly less time
to complete the computational task.

You can also edit the sample source code to change the
number of digits calculated (``DIGITS_NUM``), and the
number of threads to use (``THREADS_NUM``).

Building and Running
********************

This project outputs Pi values calculated by each thread and in the end total time
required for all the calculation to be done. It can be built and executed
on Synopsys ARC HSDK board as follows:

.. zephyr-app-commands::
   :zephyr-app: samples/smp/pi
   :host-os: unix
   :board: qemu_x86_64
   :goals: run
   :compact:

Sample Output
=============

.. code-block:: console

    Calculate first 240 digits of Pi independently by 16 threads.
    Pi value calculated by thread #0: 3141592653589793238462643383279502884197...
    Pi value calculated by thread #1: 3141592653589793238462643383279502884197...
    ...
    Pi value calculated by thread #14: 314159265358979323846264338327950288419...
    Pi value calculated by thread #15: 314159265358979323846264338327950288419...
    All 16 threads executed by 4 cores in 28 msec
