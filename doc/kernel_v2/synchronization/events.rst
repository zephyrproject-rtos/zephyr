.. _events_v2:

Events
######

An :dfn:`event` is a kernel object that allows an application to perform
asynchronous signalling when a condition of interest occurs.

.. contents::
    :local:
    :depth: 2

Concepts
********

Any number of events can be defined. Each event is referenced by
its memory address.

An event has the following key properties:

* An **event handler**, which specifies the action to be taken
  when the event is signalled.

* An **event pending flag**, which is set if the event is signalled
  and an event handler function does not consume the signal.

An event must be initialized before it can be used. This establishes
its event handler and clears the event pending flag.

Event Lifecycle
===============

An ISR or a thread signals an event by **sending** the event
when a condition of interest occurs that cannot be handled by the detector.

Each time an event is sent, the kernel examines its event handler
to determine what action to take.

* :c:macro:`K_EVT_IGNORE` causes the event to be ignored.

* :c:macro:`K_EVT_DEFAULT` causes the event pending flag to be set.

* Any other value is assumed to be the address of an event handler function,
  and is invoked by the system workqueue thread. If the function returns
  zero, the signal is deemed to have been consumed; otherwise, the event
  pending flag is set.

  The kernel ensures that the event handler function is executed once
  for each time an event is sent, even if the event is sent multiple times
  in rapid succession.

An event whose event pending flag becomes set remains pending until
the event is accepted by a thread. This clears the event pending flag.

A thread accepts a pending event by **receiving** the event.
If the event's pending flag is currently clear, the thread may choose
to wait for the event to become pending.
Any number of threads may wait for a pending event simultaneously;
when the event is pended it is accepted by the highest priority thread
that has waited longest.

.. note::
    A thread that accepts an event cannot directly determine how many times
    the event pending flag was set since the event was last accepted.

    SHOULD WE ALLOW THE EVENT INITIALIZATION ROUTINE TO ACCEPT THE MAXIMUM
    NUMBER OF TIMES THE EVENT CAN PEND? IT'S A TRIVIAL CHANGE ...

Comparison to Unix-style Signals
================================

Zephyr events are somewhat akin to Unix-style signals, but have a number of
significant differences. The most notable of these are listed below:

* A Zephyr event cannot be blocked --- it is always delivered to its event
  handler immediately.

* A Zephyr event pends *after* it has been delivered to its event handler,
  and only if an event handler function does not consume the event.

* Zephyr has no pre-defined events or actions. All events are application
  defined, and all have a default action that pends the event.

Implementation
**************

Defining an Event
=================

An event is defined using a variable of type :c:type:`struct k_event`.
It must then be initialized by calling :cpp:func:`k_event_init()`.

The following code defines and initializes an event.

.. code-block:: c

    struct k_event my_event;

    extern int my_event_handler(struct k_event *event)

    k_event_init(&my_event, my_event_handler);

Alternatively, an event can be defined and initialized at compile time
by calling :c:macro:`K_EVENT_DEFINE()`.

The following code has the same effect as the code segment above.

.. code-block:: c

    K_EVENT_DEFINE(my_event, my_event_handler);

Signaling an Event
==================

An event is signalled by calling :cpp:func:`k_event_send()`.

The following code builds on the example above, and uses the event
in an ISR to signal that a key press has occurred.

.. code-block:: c

    void keypress_interrupt_handler(void *arg)
    {
        ...
        k_event_send(&my_event);
        ...
    }

Handling an Event
=================

An event handler function is used when a signalled event should not be ignored
or immediately pended. It has the following form:

.. code-block:: c

    int <function_name>(struct k_event *event)
    {
        /* catch the event signal; return zero if the signal is consumed, */
        /* or non-zero to let the event pend                              */
        ...
    }

The following code builds on the example above, and uses an event handler
function to do processing that is too complex to be performed by the ISR.

.. code-block:: c

    int my_event_handler(struct k_event *event_id_is_unused)
    {
        /* determine what key was pressed */
        char c = get_keypress();

        /* do complex processing of the keystroke */
	...

        /* signalled event has been consumed */
        return 0;
    }

Accepting an Event
==================

A pending event is accepted by a thread by calling :cpp:func:`k_event_recv()`.

The following code is an alternative to the example above,
and uses a dedicated thread to do very complex processing
of key presses that would otherwise monopolize the system workqueue.
The event handler function is used to filter out unwanted key press
notifications, allowing the dedicated thread to wake up and process
key presses only when needed.

.. code-block:: c

    int my_event_handler(struct k_event *event_id_is_unused)
    {
        /* determine what key was pressed */
        char c = get_keypress();

        /* signal thread only if key pressed was a digit */
        if ((c >= '0') && (c <= '9')) {
            /* save key press information */
            ...
            /* signalled event should be pended */
            return 1;
        } else {
            /* signalled event has been consumed */
            return 0;
        }
    }

    void keypress_thread(int unused1, int unused2, int unused3)
    {
        /* consume key presses */
        while (1) {

            /* wait for a key press event to pend */
            k_event_recv(&my_event, K_FOREVER);

            /* process saved key press, which must be a digit */
            ...
        }
    }

Suggested Uses
**************

Use an event to allow the kernel's system workqueue to handle an event,
rather than defining an application thread to handle it.

Use an event to allow the kernel's system workqueue to pre-process an event,
prior to letting an application thread handle it.

Configuration Options
*********************

Related configuration options:

* None.

APIs
****

The following APIs for an event are provided by :file:`kernel.h`:

:cpp:func:`k_event_handler_set()`
    Register an event handler function for an event.

:cpp:func:`k_event_send()`
    Signal an event.

:cpp:func:`k_event_recv()`
    Catch an event signal.
