.. _cpu_idle:

CPU Idling
##########

Although normally reserved for the idle thread, in certain special
applications, a thread might want to make the CPU idle.

.. contents::
    :local:
    :depth: 2

Concepts
********

Making the CPU idle causes the kernel to pause all operations until an event,
normally an interrupt, wakes up the CPU. In a regular system, the idle thread
is responsible for this. However, in some constrained systems, it is possible
that another thread takes this duty.

Implementation
**************

Making the CPU idle
===================

Making the CPU idle is simple: call the k_cpu_idle() API. The CPU will stop
executing instructions until an event occurs. Make sure interrupts are not
locked before invoking it. Most likely, it will be called within a loop.

.. code-block:: c

    static k_sem my_sem;

    void my_isr(void *unused)
    {
        k_sem_give(&my_sem);
    }

    void main(void)
    {
        k_sem_init(&my_sem, 0, 1);

        /* wait for semaphore from ISR, then do related work */

        for (;;) {

            /* wait for ISR to trigger work to perform */
            if (k_sem_take(&my_sem, K_NO_WAIT) == 0) {

                /* ... do processing */

            }

            /* put CPU to sleep to save power */
            k_cpu_idle();
        }
    }

Making the CPU idle in an atomic fashion
========================================

It is possible that there is a need to do some work atomically before making
the CPU idle. In such a case, k_cpu_atomic_idle() should be used instead.

In fact, there is a race condition in the previous example: the interrupt could
occur between the time the semaphore is taken, finding out it is not available
and making the CPU idle again. In some systems, this can cause the CPU to idle
until *another* interrupt occurs, which might be *never*, thus hanging the
system completely. To prevent this, k_cpu_atomic_idle() should have been used,
like in this example.

.. code-block:: c

    static k_sem my_sem;

    void my_isr(void *unused)
    {
        k_sem_give(&my_sem);
    }

    void main(void)
    {
        k_sem_init(&my_sem, 0, 1);

        for (;;) {

            unsigned int key = irq_lock();

            /*
             * Wait for semaphore from ISR; if acquired, do related work, then
             * go to next loop iteration (the semaphore might have been given
             * again); else, make the CPU idle.
             */

            if (k_sem_take(&my_sem, K_NO_WAIT) == 0) {

                irq_unlock(key);

                /* ... do processing */


            } else {
                /* put CPU to sleep to save power */
                k_cpu_atomic_idle(key);
            }
        }
    }


Suggested Uses
**************

Use k_cpu_atomic_idle() when a thread has to do some real work in addition to
idling the CPU to wait for an event. See example above.

Use k_cpu_idle() only when a thread is only responsible for idling the CPU,
i.e. not doing any real work, like in this example below.

.. code-block:: c

    void main(void)
    {
        /* ... do some system/application initialization */


        /* thread is only used for CPU idling from this point on */
        for (;;) {
            k_cpu_idle();
        }
    }

.. note::
     **Do not use these APIs unless absolutely necessary.** In a normal system,
     the idle thread takes care of power management, including CPU idling.

API Reference
*************

.. doxygengroup:: cpu_idle_apis
   :project: Zephyr
