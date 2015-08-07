.. _microkernel_semaphores:

Semaphores
##########

Definition
**********

The microkernel semaphore is defined in
:file:`kernel/microkernel/k_semaphore.c` and are an implementation of
traditional counting semaphores. Semaphores are used to synchronize
application task activities.

Function
********

Semaphores are initialized by the system. At start the semaphore is
un-signaled and no task is waiting for it. Any task in the system can
signal a semaphore. Every signal increments the count value associated
with the semaphore. When several tasks wait for the same semaphore at
the same time, they are held in a prioritized list. If the semaphore is
signaled, the task with the highest priority is released. If more tasks
of that priority are waiting, the first one that requested the
semaphore wakes up. Other tasks can test the semaphore to see if it is
signaled. If not signaled, tasks can either wait, with or without a
timeout, until signaled or return immediately with a failed status.

Usage
*****

Defining a Semaphore in MDEF file
=================================

The following parameters must be defined:

   *name*
          This specifies a unique name for the semaphore.


Add an entry for a semaphore in the project .MDEF file using the
following syntax:

.. code-block:: console

   SEMA %name

For example, the file :file:`projName.mdef` defines two semaphores
as follows:

.. code-block:: console

    % SEMA NAME
    % ================
      SEMA INPUT_DATA
      SEMA WORK_DONE


Defining Semaphore Inside Code
==============================

In addition to defining semaphores in MDEF file, it is also possible to
define semaphores inside code. The macro ``DEFINE_SEMAPHORE(sem_name)``
can be used for this purpose.

For example, the following code can be used to define a global semaphore
``PRIV_SEM``.

.. code-block:: c

   DEFINE_SEMAPHORE(PRIV_SEM);

The semaphore ``PRIV_SEM`` can be used in the same style as those
semaphores defined in MDEF file.

It is possible to utilize this semaphore in another source file, simply
add:

.. code-block:: c

   extern const ksem_t PRIV_SEM;

to that file. The semaphore ``PRIV_SEM`` can be then used there.


Example: Giving a Semaphore from a Task
=======================================

This code uses a semaphore to indicate that a unit of data
is available for processing by a consumer task.

.. code-block:: c

   void producer_task(void)
   {
       /* save data item in a buffer */
       ...

        /* notify task that an additional data item is available */
       task_sem_give(INPUT_DATA);

       ...
   }

Example: Giving a Semaphore from an ISR
=======================================

This code uses a semaphore to indicate that a unit of data
is available for processing by a consumer task.

.. code-block:: c

   /*
    * reserve 2 command packets for semaphore updates
    *
    * note: this assumes that input data arrives at a rate that allows
    * the microkernel server fiber to finish the semaphore give operation
    * for data item "N" before the ISR begins working on data item "N+2"
    * (i.e. data arrives in bursts of at most one unit)
    */
   static CMD_PKT_SET_INSTANCE(cmd_packets, 2);

   void input_data_interrupt_handler(void *arg)
   {
       /* save data item in a buffer */
       ...

        /* notify task that an additional data item is available */
       isr_sem_give(INPUT_DATA, &CMD_PKT_SET(cmd_packets));

       ...
   }

Example: Taking a Semaphore with a Conditional Time-out
=======================================================

This code waits up to 500 ticks for a semaphore to be given,
and gives a warning if it is not obtained in that time.

.. code-block:: c

   void consumer_task(void)
   {
       ...

       if (task_sem_take_wait_timeout(INPUT_DATA, 500) == RC_TIME) {
           printf("Input data not available!");
       } else {
           /* extract saved data item from buffer and process it */
           ...
       }
       ...
   }

Example: Monitoring Multiple Semaphores at Once
===============================================

This code waits on two semaphores simultaneously, and then takes
action depending on which one was given.

.. code-block:: c

   ksem_t my_sem_group[3] = { INPUT_DATA, WORK_DONE, ENDLIST };

   void consumer_task(void)
   {
       ksem_t sem_id;
       ...

       sem_id = task_sem_group_take_wait(my_sem_group);
       if (sem_id == WORK_DONE) {
           printf("Shutting down!");
           return;
       } else {
           /* process input data */
           ...
       }
       ...
   }

Example: Giving Multiple Semaphores at Once
===========================================

This code uses a semaphore group to allow a controlling task to signal
the semaphores used by four other tasks in a single operation.

.. code-block:: c

   ksem_t my_sem_group[5] = { SEM1, SEM2, SEM3, SEM4, ENDLIST };

   void control_task(void)
   {
       ...
       task_semaphore_group_give(my_sem_group);
       ...
   }


APIs
****

The following APIs for an individual semaphore are provided by microkernel.h.

+----------------------------------------+------------------------------------+
| Call                                   | Description                        |
+========================================+====================================+
| :c:func:`isr_sem_give()`               | Signal a semaphore from an ISR.    |
+----------------------------------------+------------------------------------+
| :c:func:`fiber_sem_give()`             | Signal a semaphore from a fiber.   |
+----------------------------------------+------------------------------------+
| :c:func:`task_sem_give()`              | Signal a semaphore from a task.    |
+----------------------------------------+------------------------------------+
| :c:func:`task_sem_take()`              | Test a semaphore without waiting.  |
+----------------------------------------+------------------------------------+
| :c:func:`task_sem_take_wait()`         | Wait on a semaphore.               |
+----------------------------------------+------------------------------------+
| :c:func:`task_sem_take_wait_timeout()` | Wait on a semaphore for a          |
|                                        | specified time period.             |
+----------------------------------------+------------------------------------+
| :c:func:`task_sem_reset()`             | Sets the semaphore count to zero.  |
+----------------------------------------+------------------------------------+
| :c:func:`task_sem_count_get()`         | Read signal count for a semaphore. |
+----------------------------------------+------------------------------------+


The following APIs for semaphore groups are provided by microkernel.h.

+----------------------------------------------+------------------------------+
| Call                                         | Description                  |
+==============================================+==============================+
| :c:func:`task_sem_group_give()`              | Signal a set of semaphores.  |
+----------------------------------------------+------------------------------+
| :c:func:`task_sem_group_take()`              | Test a set of semaphores     |
|                                              | without waiting.             |
+----------------------------------------------+------------------------------+
| :c:func:`task_sem_group_take_wait()`         | Wait on a set of semaphores. |
+----------------------------------------------+------------------------------+
| :c:func:`task_sem_group_take_wait_timeout()` | Wait on a set of semaphores  |
|                                              | for a specified time period. |
+----------------------------------------------+------------------------------+
| :c:func:`task_sem_group_reset()`             | Sets the semaphore count to  |
|                                              | to zero for a set of         |
|                                              | semaphores.                  |
+----------------------------------------------+------------------------------+
