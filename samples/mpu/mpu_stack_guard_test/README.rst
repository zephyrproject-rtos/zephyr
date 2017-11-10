.. _mpu_stack_guard_test:

MPU Stack Guard Test
####################

Overview
********

This is a simple application that demonstrates basic thread stack guarding on
the supported platforms.
A thread spawned by the main task recursively calls a function that fills the
thread stack up to where it overwrites a preposed canary.
If the MPU is enabled and the Stack Guard feature is present the test succeeds
because an MEM Faults exception prevents the canary from being overwritten.
If the MPU is disabled the test fails because the canary is overwritten.

Building and Running
********************

This project outputs to the console.
To build the test with the MPU disabled:

.. zephyr-app-commands::
   :zephyr-app: samples/mpu/mpu_stack_guard_test
   :board: v2m_beetle
   :goals: build flash
   :compact:

To build the test with the MPU enabled and the stack guard feature present:

.. zephyr-app-commands::
   :zephyr-app: samples/mpu/mpu_stack_guard_test
   :board: v2m_beetle
   :conf: prj_stack_guard.conf
   :goals: build flash
   :compact:

Sample Output
=============

With the MPU enabled but the stack guard feature disabled:

.. code-block:: console

    ***** BOOTING ZEPHYR OS v1.8.99 - BUILD: Jun 15 2017 23:12:13 *****
    STACK_ALIGN 4
    MPU STACK GUARD Test
    Canary Initial Value = 0xf0cacc1a threads 0x20000b9c
    Canary = 0xfffffff5     Test not passed.
    ***** MPU FAULT *****
      Executing thread ID (thread): 0x20000b9c
      Faulting instruction address:  0x80025b0
      Data Access Violation
      Address: 0x8000edb
    Fatal fault in thread 0x20000b9c! Aborting.
    ***** HARD FAULT *****
      Fault escalation (see below)
    ***** MPU FAULT *****
      Executing thread ID (thread): 0x20000b9c
      Faulting instruction address:  0x8001db6
      Data Access Violation
      Address: 0x8000edb
    Fatal fault in ISR! Spinning...

With the MPU enabled and the stack guard feature enabled:

.. code-block:: console

    ***** BOOTING ZEPHYR OS v1.8.99 - BUILD: Jun 15 2017 23:05:56 *****
    STACK_ALIGN 20
    MPU STACK GUARD Test
    Canary Initial Value = 0xf0cacc1a threads 0x20000be0
    ***** MPU FAULT *****
      Executing thread ID (thread): 0x20000be0
      Faulting instruction address:  0x8001dc2
      Stacking error
    Fatal fault in thread 0x20000be0! Aborting.
