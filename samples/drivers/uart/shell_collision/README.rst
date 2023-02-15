.. _uart_shell_collision:

NPCX UART Shell Collision Sample
##################

Overview
********

This sample designs a collision test caused by NPCX UART driver accesses bits of
UFTCTL register simultaneously.

If the collision symptom occurred, the task 1 went into deadlock and only see
task 2 messages on the console.

Building and Running
********************

Build and flash the sample as follows, changing ``nrf52840dk_nrf52840`` for
your board:

.. zephyr-app-commands::
   :zephyr-app: samples/drivers/uart/shell_collision
   :board: npcx9m5f_evb
   :goals: build flash
   :compact:

Sample Output
=============

.. code-block:: console

    >Iter1 shell_fprintf 11111111111111111111111111111111111111<n
    >Iter1 printk 22222222222222222222----deadlock---------<
