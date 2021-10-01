.. _semaphores_v2:

Semaphores
##########

A :dfn:`semaphore` is a kernel object that implements a traditional
counting semaphore.

.. contents::
    :local:
    :depth: 2

Concepts
********

Any number of semaphores can be defined (limited only by available RAM). Each
semaphore is referenced by its memory address.

A semaphore has the following key properties:

* A **count** that indicates the number of times the semaphore can be taken.
  A count of zero indicates that the semaphore is unavailable.

* A **limit** that indicates the maximum value the semaphore's count
  can reach.

A semaphore must be initialized before it can be used. Its count must be set
to a non-negative value that is less than or equal to its limit.

A semaphore may be **given** by a thread or an ISR. Giving the semaphore
increments its count, unless the count is already equal to the limit.

A semaphore may be **taken** by a thread. Taking the semaphore
decrements its count, unless the semaphore is unavailable (i.e. at zero).
When a semaphore is unavailable a thread may choose to wait for it to be given.
Any number of threads may wait on an unavailable semaphore simultaneously.
When the semaphore is given, it is taken by the highest priority thread
that has waited longest.

.. note::
    You may initialize a "full" semaphore (count equal to limit) to limit the number
    of threads able to execute the critical section at the same time. You may also
    initialize an empty semaphore (count equal to 0, with a limit greater than 0)
    to create a gate through which no waiting thread may pass until the semaphore
    is incremented. All standard use cases of the common semaphore are supported.

.. note::
    The kernel does allow an ISR to take a semaphore, however the ISR must
    not attempt to wait if the semaphore is unavailable.

Implementation
**************

Defining a Semaphore
====================

A semaphore is defined using a variable of type :c:struct:`k_sem`.
It must then be initialized by calling :c:func:`k_sem_init`.

The following code defines a semaphore, then configures it as a binary
semaphore by setting its count to 0 and its limit to 1.

.. code-block:: c

    struct k_sem my_sem;

    k_sem_init(&my_sem, 0, 1);

Alternatively, a semaphore can be defined and initialized at compile time
by calling :c:macro:`K_SEM_DEFINE`.

The following code has the same effect as the code segment above.

.. code-block:: c

    K_SEM_DEFINE(my_sem, 0, 1);

Giving a Semaphore
==================

A semaphore is given by calling :c:func:`k_sem_give`.

The following code builds on the example above, and gives the semaphore to
indicate that a unit of data is available for processing by a consumer thread.

.. code-block:: c

    void input_data_interrupt_handler(void *arg)
    {
        /* notify thread that data is available */
        k_sem_give(&my_sem);

        ...
    }

Taking a Semaphore
==================

A semaphore is taken by calling :c:func:`k_sem_take`.

The following code builds on the example above, and waits up to 50 milliseconds
for the semaphore to be given.
A warning is issued if the semaphore is not obtained in time.

.. code-block:: c

    void consumer_thread(void)
    {
        ...

        if (k_sem_take(&my_sem, K_MSEC(50)) != 0) {
            printk("Input data not available!");
        } else {
            /* fetch available data */
            ...
        }
        ...
    }

Suggested Uses
**************

Use a semaphore to control access to a set of resources by multiple threads.

Use a semaphore to synchronize processing between a producing and consuming
threads or ISRs.

Configuration Options
*********************

Related configuration options:

* None.

API Reference
**************

.. doxygengroup:: semaphore_apis

User Mode Semaphore API Reference
*********************************

The sys_sem exists in user memory working as counter semaphore for user mode
thread when user mode enabled. When user mode isn't enabled, sys_sem behaves
like k_sem.

.. doxygengroup:: user_semaphore_apis
