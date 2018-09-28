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
   :board: frdm_k64f
   :goals: build flash
   :compact:

To build the test with the MPU enabled and the stack guard feature present:

.. zephyr-app-commands::
   :zephyr-app: samples/mpu/mpu_stack_guard_test
   :board: frdm_k64f
   :conf: prj_stack_guard.conf
   :goals: build flash
   :compact:

Sample Output
=============

With the MPU enabled but the stack guard feature disabled:

.. code-block:: console

    ***** Booting Zephyr OS v1.13.0-rc1-14-gd47fada *****
    STACK_ALIGN 0x8
    MPU STACK GUARD Test
    Canary Initial Value = 0xf0cacc1a threads 0x20000ff8
    Canary = 0x20000128     Test not passed.
    ***** BUS FAULT *****
      Instruction bus error
      NXP MPU error, port 3
        Mode: Supervisor, Instruction Address: 0x20001030
        Type: Read, Master: 0, Regions: 0x8800
    ***** Hardware exception *****
    Current thread ID = 0x20000ff8
    Faulting instruction address = 0x20001030
    Fatal fault in essential thread! Spinning...

With the MPU enabled and the stack guard feature enabled:

.. code-block:: console

    ***** Booting Zephyr OS v1.13.0-rc1-14-gd47fada *****
    STACK_ALIGN 0x20
    MPU STACK GUARD Test
    Canary Initial Value = 0xf0cacc1a threads 0x20001100
    ***** BUS FAULT *****
      Stacking error
      NXP MPU error, port 3
        Mode: Supervisor, Data Address: 0x200011b0
        Type: Write, Master: 0, Regions: 0x8400
    ***** Hardware exception *****
    Current thread ID = 0x20001100
    Faulting instruction address = 0x0
    Fatal fault in thread 0x20001100! Aborting.
