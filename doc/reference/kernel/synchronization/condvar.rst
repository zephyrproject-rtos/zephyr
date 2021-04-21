.. _condvar:

Condition Variables
###################

A :dfn:`condition variable` is a synchronization primitive
that enables threads to wait until a particular condition occurs.

.. contents::
    :local:
    :depth: 2

Concepts
********

Any number of condition variables can be defined (limited only by available RAM). Each
condition variable is referenced by its memory address.

To wait for a condition to become true, a thread can make use of a condition
variable.

A condition variable is basically a queue of threads that threads can put
themselves on when some state of execution (i.e., some condition) is not as
desired (by waiting on the condition). The function
:c:func:`k_condvar_wait` performs atomically the following steps;

#. Releases the last acquired mutex.
#. Puts the current thread in the condition variable queue.

Some other thread, when it changes said state, can then wake one (or more)
of those waiting threads and thus allow them to continue by signaling on
the condition using :c:func:`k_condvar_signal` or
:c:func:`k_condvar_broadcast` then it:

#. Re-acquires the mutex previously released.
#. Returns from :c:func:`k_condvar_wait`.

A condition variable must be initialized before it can be used.


Implementation
**************

Defining a Condition Variable
=============================

A condition variable is defined using a variable of type :c:struct:`k_condvar`.
It must then be initialized by calling :c:func:`k_condvar_init`.

The following code defines a condition variable:

.. code-block:: c

    struct k_condvar my_condvar;

    k_condvar_init(&my_condvar);

Alternatively, a condition variable can be defined and initialized at compile time
by calling :c:macro:`K_CONDVAR_DEFINE`.

The following code has the same effect as the code segment above.

.. code-block:: c

    K_CONDVAR_DEFINE(my_condvar);

Waiting on a Condition Variable
===============================

A thread can wait on a condition by calling :c:func:`k_condvar_wait`.

The following code waits on the condition variable.


.. code-block:: c

    K_MUTEX_DEFINE(mutex);
    K_CONDVAR_DEFINE(condvar)

    void main(void)
    {
        k_mutex_lock(&mutex, K_FOREVER);

        /* block this thread until another thread signals cond. While
         * blocked, the mutex is released, then re-acquired before this
         * thread is woken up and the call returns.
         */
        k_condvar_wait(&condvar, &mutex, K_FOREVER);
        ...
        k_mutex_unlock(&mutex);
    }

Signaling a Condition Variable
===============================

A condition variable is signaled on by calling :c:func:`k_condvar_signal` for
one thread or by calling :c:func:`k_condvar_broadcast` for multiple threads.

The following code builds on the example above.

.. code-block:: c

    void worker_thread(void)
    {
        k_mutex_lock(&mutex, K_FOREVER);

        /*
         * Do some work and fullfill the condition
         */
        ...
        ...
        k_condvar_signal(&condvar);
        k_mutex_unlock(&mutex);
    }

Suggested Uses
**************

Use condition variables with a mutex to signal changing states (conditions) from
one thread to another thread.
Condition variables are not the condition itself and they are not events.
The condition is contained in the surrounding programming logic.

Mutexes alone are not designed for use as a notification/synchronization
mechanism. They are meant to provide mutually exclusive access to a shared
resource only.

Configuration Options
*********************

Related configuration options:

* None.

API Reference
**************

.. doxygengroup:: condvar_apis
   :project: Zephyr
