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

.. code-block:: console

   $ cd samples/mpu_stack_guard_test
   $ make

To build the test with the MPU enabled and the stack guard feature present:

.. code-block:: console

   $ cd samples/mpu_stack_guard_test
   $ make CONF_FILE=prj_stack_guard.conf

Sample Output
=============

With the MPU disabled:

.. code-block:: console

   ***** BOOTING ZEPHYR OS v1.7.99 - BUILD: Mar 29 2017 11:07:09 *****
   MPU STACK GUARD Test
   Canary Initial Value = 0xf0cacc1a
   Canary = 0x200003a8     Test not passed.
   ...

With the MPU enabled and the stack guard feature present:

.. code-block:: console

   ***** BOOTING ZEPHYR OS v1.7.99 - BUILD: Mar 29 2017 11:20:10 *****
   MPU STACK GUARD Test
   Canary Initial Value = 0xf0cacc1a
   [general] [DBG] arm_core_mpu_configure: Region info: 0x200003ac 0x400
   ***** MPU FAULT *****
     Executing thread ID (thread): 0x200003ac
     Faulting instruction address:  0x0
     Stacking error
   Fatal fault in thread 0x200003ac! Aborting.
   ***** HARD FAULT *****
     Fault escalation (see below)
   ***** MPU FAULT *****
     Executing thread ID (thread): 0x200003ac
     Faulting instruction address:  0x8000466
     Stacking error
   Fatal fault in ISR! Spinning...
