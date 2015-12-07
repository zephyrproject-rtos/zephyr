.. _microkernel_events:

Events
######

Concepts
********

The microkernel's event objects are an implementation of traditional
binary semaphores.

Any number of events can be defined in a microkernel system. Each event
has a name that uniquely identifies it.

An event is typically *sent* by a task, fiber, or ISR and *received*
by a task, which then takes some action in response. Events are the easiest
and most efficient way to synchronize operations between two different
execution contexts.

Each event starts off in the *clear*  state. Once an event has been sent
it is placed into the *set* state, and remains in that state until it is
received; the event's state then reverts back to clear. Sending an event
that is already set is permitted, but does not affect the event's state
and does not allow the receiving task to recognize that the event has been sent
more than once.

The receiving task may test an event's state in either a non-blocking or
blocking manner. The kernel allows only a single receiving task to wait
for a given event; if a second task attempts to wait its receive operation
immediately returns a failure indication.

Each event has an optional *event handler* function, which is executed
by the microkernel server fiber when the event is sent. This function
allows an event to be processed more quickly, without requiring the kernel
to schedule a receiving task. If the event handler determines that the event
can be ignored, or it is able to process the event without the assistance
of a task, the event handler returns a value of zero and the event's state
is left unchanged. If the event handler determines that additional processing
is required it returns a non-zero value and the event's state is changed
to set (if it isn't already set).

An event handler function can be used to improve the efficiency of event
processing by the receiving task, and can even eliminate the need for the
receiving task entirely is some situations. Any event that does not require
an event handler can specify the :c:macro:`NULL` function. The event handler
function is passed the name of the event being sent each time it is invoked,
allowing the same function to be shared by multiple events. An event's event
handler function is specified at compile-time, but can be changed subsequently
at run-time.

Purpose
*******

Use an event to signal a task to take action in response to a condition
detected by another task, a fiber, or an ISR.

Use an event handler to allow the microkernel server fiber to handle an event,
prior to (or instead of) letting a task handle the event.

Usage
*****

Defining an Event
=================

The following parameters must be defined:

   *name*
          This specifies a unique name for the event.

   *handler*
          This specifies the name of the event handler function,
          which should have the following form:

          .. code-block:: c

             int <entry_point>(int event)
             {
                 /* start handling event; return zero if all done, */
                 /* or non-zero to let receiving task handle event */
                 ...
             }

          If no event handler is required specify :c:macro:`NULL`.

Public Event
------------

Define the event in the application's MDEF using the following syntax:

.. code-block:: console

   EVENT name handler

For example, the file :file:`projName.mdef` defines two events
as follows:

.. code-block:: console

    % EVENT NAME            ENTRY
    % ==========================================
      EVENT KEYPRESS        validate_keypress
      EVENT BUTTONPRESS     NULL

A public event can be referenced by name from any source file that includes
the file :file:`zephyr.h`.

Private Event
-------------

Define the event in a source file using the following syntax:

.. code-block:: c

   DEFINE_EVENT(name, handler);

For example, the following code defines a private event named ``PRIV_EVENT``,
which has no associated event handler function.

.. code-block:: c

   DEFINE_EVENT(PRIV_EVENT, NULL);

To utilize this event from a different source file use the following syntax:

.. code-block:: c

   extern const kevent_t PRIV_EVENT;

Example: Signaling an Event from an ISR
========================================

This code signals an event during the processing of an interrupt.

.. code-block:: c

   void keypress_interrupt_handler(void *arg)
   {
       ...
       isr_event_signal(KEYPRESS);
       ...
   }

Example: Consuming an Event using a Task
========================================

This code processes events of a single type using a task.

.. code-block:: c

   void keypress_task(void)
   {
       /* consume key presses */
       while (1) {

           /* wait for a key press to be signalled */
           task_event_recv(KEYPRESS, TICKS_NONE);

           /* determine what key was pressed */
           char c = get_keypress();

           /* process key press */
           ...
       }
   }

Example: Filtering Event Signals using an Event Handler
=======================================================

This code registers an event handler that filters out unwanted events
so that the receiving task only wakes up when needed.

.. code-block:: c

   int validate_keypress(int event_id_is_unused)
   {
       /* determine what key was pressed */
       char c = get_keypress();

       /* signal task only if key pressed was a digit */
       if ((c >= '0') && (c <= '9')) {
          /* save key press information */
          ...
          /* event is signalled to task */
          return 1;
       } else {
          /* event is not signalled to task */
          return 0;
       }
   }

   void keypress_task(void)
   {
       /* register the filtering routine */
       task_event_handler_set(KEYPRESS, validate_keypress);

       /* consume key presses */
       while (1) {

           /* wait for a key press to be signalled */
           task_event_recv(KEYPRESS, TICKS_NONE);

           /* process saved key press, which must be a digit */
           ...
       }
   }

APIs
****

The following Event APIs are provided by :file:`microkernel.h`:

:cpp:func:`isr_event_send()`
   Signal an event from an ISR.

:cpp:func:`fiber_event_send()`
   Signal an event from a fiber.

:cpp:func:`task_event_send()`
   Signal an event from a task.

:c:func:`task_event_recv()`
   Waits for an event signal for a specified time period.

:cpp:func:`task_event_handler_set()`
   Registers an event handler function for an event.
