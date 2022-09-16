.. _events:

Events
######

An :dfn:`event object` is a kernel object that implements traditional events.

.. contents::
    :local:
    :depth: 2

Concepts
********

Any number of event objects can be defined (limited only by available RAM). Each
event object is referenced by its memory address. One or more threads may wait
on an event object until the desired set of events has been delivered to the
event object. When new events are delivered to the event object, all threads
whose wait conditions have been satisfied become ready simultaneously.

An event object has the following key properties:

* A 32-bit value that tracks which events have been delivered to it.

An event object must be initialized before it can be used.

Events may be **delivered** by a thread or an ISR. When delivering events, the
events may either overwrite the existing set of events or add to them in
a bitwise fashion. When overwriting the existing set of events, this is referred
to as setting. When adding to them in a bitwise fashion, this is referred to as
posting. Both posting and setting events have the potential to fulfill match
conditions of multiple threads waiting on the event object. All threads whose
match conditions have been met are made active at the same time.

Threads may wait on one or more events. They may either wait for all of the
the requested events, or for any of them. Furthermore, threads making a wait
request have the option of resetting the current set of events tracked by the
event object prior to waiting. Care must be taken with this option when
multiple threads wait on the same event object.

.. note::
    The kernel does allow an ISR to query an event object, however the ISR must
    not attempt to wait for the events.

Implementation
**************

Defining an Event Object
========================

An event object is defined using a variable of type :c:struct:`k_event`.
It must then be initialized by calling :c:func:`k_event_init`.

The following code defines an event object.

.. code-block:: c

    struct k_event my_event;

    k_event_init(&my_event);

Alternatively, an event object can be defined and initialized at compile time
by calling :c:macro:`K_EVENT_DEFINE`.

The following code has the same effect as the code segment above.

.. code-block:: c

    K_EVENT_DEFINE(my_event);

Setting Events
==============

Events in an event object are set by calling :c:func:`k_event_set`.

The following code builds on the example above, and sets the events tracked by
the event object to 0x001.

.. code-block:: c

    void input_available_interrupt_handler(void *arg)
    {
        /* notify threads that data is available */

        k_event_set(&my_event, 0x001);

        ...
    }

Posting Events
==============

Events are posted to an event object by calling :c:func:`k_event_post`.

The following code builds on the example above, and posts a set of events to
the event object.

.. code-block:: c

    void input_available_interrupt_handler(void *arg)
    {
        ...

        /* notify threads that more data is available */

        k_event_post(&my_event, 0x120);

        ...
    }

Waiting for Events
==================

Threads wait for events by calling :c:func:`k_event_wait`.

The following code builds on the example above, and waits up to 50 milliseconds
for any of the specified events to be posted.  A warning is issued if none
of the events are posted in time.

.. code-block:: c

    void consumer_thread(void)
    {
        uint32_t  events;

        events = k_event_wait(&my_event, 0xFFF, false, K_MSEC(50));
        if (events == 0) {
            printk("No input devices are available!");
        } else {
            /* Access the desired input device(s) */
            ...
        }
        ...
    }

Alternatively, the consumer thread may desire to wait for all the events
before continuing.

.. code-block:: c

    void consumer_thread(void)
    {
        uint32_t  events;

        events = k_event_wait_all(&my_event, 0x121, false, K_MSEC(50));
        if (events == 0) {
            printk("At least one input device is not available!");
        } else {
            /* Access the desired input devices */
            ...
        }
        ...
    }

Suggested Uses
**************

Use events to indicate that a set of conditions have occurred.

Use events to pass small amounts of data to multiple threads at once.

Configuration Options
*********************

Related configuration options:

* :kconfig:option:`CONFIG_EVENTS`

API Reference
**************

.. doxygengroup:: event_apis
