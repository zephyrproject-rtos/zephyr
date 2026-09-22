.. zephyr:code-sample:: hwspinlock
   :name: HWSPINLOCK
   :relevant-api: hwspinlock_interface mbox_interface

   Demonstrate cross-core synchronization using the HWSPINLOCK API together
   with the MBOX API.

Overview
********

This sample runs two Zephyr images (sysbuild):

- **HOST** periodically locks a hardware spinlock gate, sends a mailbox *ping*
  while holding the lock, then releases the lock and waits for a *pong*.
- **REMOTE** receives the ping, shows that a non-blocking trylock fails while
  HOST holds the gate, then blocks until it can lock the same gate and replies.

The purpose is to show that the :ref:`HWSPINLOCK API <hwspinlock_api>` provides
real inter-core mutual exclusion (via a hardware hwspinlock controller), while
:ref:`MBOX <mbox_api>` is used for signaling.

Building and Running
********************

Build for FRDM-MCXN947 (CPU0 + CPU1)
************************************

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/hwspinlock
   :board: frdm_mcxn947/mcxn947/cpu0
   :goals: build
   :west-args: --sysbuild

Expected Output
===============

HOST output example:

.. code-block:: console

   Hello from HOST - frdm_mcxn947/mcxn947/cpu0
   ...
   HOST: locked gate id=0, sending ping on channel 1
   HOST: unlocked gate, waiting for pong...
   HOST: received pong

REMOTE output example:

.. code-block:: console

   Hello from REMOTE - frdm_mcxn947/mcxn947/cpu1
   ...
   REMOTE: got ping (rx_sem), checking gate contention...
   REMOTE: trylock returned -16 (expected if HOST holds the lock)
   REMOTE: waiting to acquire gate...
   REMOTE: acquired gate id=0
   REMOTE: locked gate id=0, sending pong on channel 0
