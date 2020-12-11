.. _polling_v2:

Polling API
###########

The polling API is used to wait concurrently for any one of multiple conditions
to be fulfilled.

.. contents::
    :local:
    :depth: 2

Concepts
********

The polling API's main function is :c:func:`k_poll`, which is very similar
in concept to the POSIX :c:func:`poll` function, except that it operates on
kernel objects rather than on file descriptors.

The polling API allows a single thread to wait concurrently for one or more
conditions to be fulfilled without actively looking at each one individually.

There is a limited set of such conditions:

- a semaphore becomes available
- a kernel FIFO contains data ready to be retrieved
- a poll signal is raised

A thread that wants to wait on multiple conditions must define an array of
**poll events**, one for each condition.

All events in the array must be initialized before the array can be polled on.

Each event must specify which **type** of condition must be satisfied so that
its state is changed to signal the requested condition has been met.

Each event must specify what **kernel object** it wants the condition to be
satisfied.

Each event must specify which **mode** of operation is used when the condition
is satisfied.

Each event can optionally specify a **tag** to group multiple events together,
to the user's discretion.

Apart from the kernel objects, there is also a **poll signal** pseudo-object
type that be directly signaled.

The :c:func:`k_poll` function returns as soon as one of the conditions it
is waiting for is fulfilled. It is possible for more than one to be fulfilled
when :c:func:`k_poll` returns, if they were fulfilled before
:c:func:`k_poll` was called, or due to the preemptive multi-threading
nature of the kernel. The caller must look at the state of all the poll events
in the array to figured out which ones were fulfilled and what actions to take.

Currently, there is only one mode of operation available: the object is not
acquired. As an example, this means that when :c:func:`k_poll` returns and
the poll event states that the semaphore is available, the caller of
:c:func:`k_poll()` must then invoke :c:func:`k_sem_take` to take
ownership of the semaphore. If the semaphore is contested, there is no
guarantee that it will be still available when :c:func:`k_sem_give` is
called.

Implementation
**************

Using k_poll()
==============

The main API is :c:func:`k_poll`, which operates on an array of poll events
of type :c:struct:`k_poll_event`. Each entry in the array represents one
event a call to :c:func:`k_poll` will wait for its condition to be
fulfilled.

They can be initialized using either the runtime initializers
:c:macro:`K_POLL_EVENT_INITIALIZER()` or :c:func:`k_poll_event_init`, or
the static initializer :c:macro:`K_POLL_EVENT_STATIC_INITIALIZER()`. An object
that matches the **type** specified must be passed to the initializers. The
**mode** *must* be set to :c:macro:`K_POLL_MODE_NOTIFY_ONLY`. The state *must*
be set to :c:macro:`K_POLL_STATE_NOT_READY` (the initializers take care of
this). The user **tag** is optional and completely opaque to the API: it is
there to help a user to group similar events together. Being optional, it is
passed to the static initializer, but not the runtime ones for performance
reasons. If using runtime initializers, the user must set it separately in the
:c:struct:`k_poll_event` data structure. If an event in the array is to be
ignored, most likely temporarily, its type can be set to K_POLL_TYPE_IGNORE.

.. code-block:: c

    struct k_poll_event events[2] = {
        K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_SEM_AVAILABLE,
                                        K_POLL_MODE_NOTIFY_ONLY,
                                        &my_sem, 0),
        K_POLL_EVENT_STATIC_INITIALIZER(K_POLL_TYPE_FIFO_DATA_AVAILABLE,
                                        K_POLL_MODE_NOTIFY_ONLY,
                                        &my_fifo, 0),
    };

or at runtime

.. code-block:: c

    struct k_poll_event events[2];
    void some_init(void)
    {
        k_poll_event_init(&events[0],
                          K_POLL_TYPE_SEM_AVAILABLE,
                          K_POLL_MODE_NOTIFY_ONLY,
                          &my_sem);

        k_poll_event_init(&events[1],
                          K_POLL_TYPE_FIFO_DATA_AVAILABLE,
                          K_POLL_MODE_NOTIFY_ONLY,
                          &my_fifo);

        // tags are left uninitialized if unused
    }


After the events are initialized, the array can be passed to
:c:func:`k_poll`. A timeout can be specified to wait only for a specified
amount of time, or the special values :c:macro:`K_NO_WAIT` and
:c:macro:`K_FOREVER` to either not wait or wait until an event condition is
satisfied and not sooner.

Only one thread can poll on a semaphore or a FIFO at a time. If a second thread
tries to poll on the same semaphore or FIFO, :c:func:`k_poll` immediately
returns with the return value -:c:macro:`EADDRINUSE`. In that case, if other
conditions passed to :c:func:`k_poll` were met, their state will be set in
the corresponding poll event.

In case of success, :c:func:`k_poll` returns 0. If it times out, it returns
-:c:macro:`EAGAIN`.

.. code-block:: c

    // assume there is no contention on this semaphore and FIFO
    // -EADDRINUSE will not occur; the semaphore and/or data will be available

    void do_stuff(void)
    {
        rc = k_poll(events, 2, 1000);
        if (rc == 0) {
            if (events[0].state == K_POLL_STATE_SEM_AVAILABLE) {
                k_sem_take(events[0].sem, 0);
            } else if (events[1].state == K_POLL_STATE_FIFO_DATA_AVAILABLE) {
                data = k_fifo_get(events[1].fifo, 0);
                // handle data
            }
        } else {
            // handle timeout
        }
    }

When :c:func:`k_poll` is called in a loop, the events state must be reset
to :c:macro:`K_POLL_STATE_NOT_READY` by the user.

.. code-block:: c

    void do_stuff(void)
    {
        for(;;) {
            rc = k_poll(events, 2, K_FOREVER);
            if (events[0].state == K_POLL_STATE_SEM_AVAILABLE) {
                k_sem_take(events[0].sem, 0);
            } else if (events[1].state == K_POLL_STATE_FIFO_DATA_AVAILABLE) {
                data = k_fifo_get(events[1].fifo, 0);
                // handle data
            }
            events[0].state = K_POLL_STATE_NOT_READY;
            events[1].state = K_POLL_STATE_NOT_READY;
        }
    }

Using k_poll_signal_raise()
===========================

One of the types of events is :c:macro:`K_POLL_TYPE_SIGNAL`: this is a "direct"
signal to a poll event. This can be seen as a lightweight binary semaphore only
one thread can wait for.

A poll signal is a separate object of type :c:struct:`k_poll_signal` that
must be attached to a k_poll_event, similar to a semaphore or FIFO. It must
first be initialized either via :c:macro:`K_POLL_SIGNAL_INITIALIZER()` or
:c:func:`k_poll_signal_init`.

.. code-block:: c

    struct k_poll_signal signal;
    void do_stuff(void)
    {
        k_poll_signal_init(&signal);
    }

It is signaled via the :c:func:`k_poll_signal_raise` function. This function
takes a user **result** parameter that is opaque to the API and can be used to
pass extra information to the thread waiting on the event.

.. code-block:: c

    struct k_poll_signal signal;

    // thread A
    void do_stuff(void)
    {
        k_poll_signal_init(&signal);

        struct k_poll_event events[1] = {
            K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL,
                                     K_POLL_MODE_NOTIFY_ONLY,
                                     &signal),
        };

        k_poll(events, 1, K_FOREVER);

        if (events.signal->result == 0x1337) {
            // A-OK!
        } else {
            // weird error
        }
    }

    // thread B
    void signal_do_stuff(void)
    {
        k_poll_signal_raise(&signal, 0x1337);
    }

If the signal is to be polled in a loop, *both* its event state and its
**signaled** field *must* be reset on each iteration if it has been signaled.

.. code-block:: c

    struct k_poll_signal signal;
    void do_stuff(void)
    {
        k_poll_signal_init(&signal);

        struct k_poll_event events[1] = {
            K_POLL_EVENT_INITIALIZER(K_POLL_TYPE_SIGNAL,
                                     K_POLL_MODE_NOTIFY_ONLY,
                                     &signal),
        };

        for (;;) {
            k_poll(events, 1, K_FOREVER);

            if (events[0].signal->result == 0x1337) {
                // A-OK!
            } else {
                // weird error
            }

            events[0].signal->signaled = 0;
            events[0].state = K_POLL_STATE_NOT_READY;
        }
    }

Suggested Uses
**************

Use :c:func:`k_poll` to consolidate multiple threads that would be pending
on one object each, saving possibly large amounts of stack space.

Use a poll signal as a lightweight binary semaphore if only one thread pends on
it.

.. note::
    Because objects are only signaled if no other thread is waiting for them to
    become available and only one thread can poll on a specific object, polling
    is best used when objects are not subject of contention between multiple
    threads, basically when a single thread operates as a main "server" or
    "dispatcher" for multiple objects and is the only one trying to acquire
    these objects.

Configuration Options
*********************

Related configuration options:

* :option:`CONFIG_POLL`

API Reference
*************

.. doxygengroup:: poll_apis
   :project: Zephyr
