.. _microkernel_events:

Events
######

Definition
**********

Event objects are microkernel synchronization objects that tasks can
signal and test. Fibers and interrupt service routines may signal
events but they cannot test or wait on them. Use event objects for
situations in which multiple signals come in but only one test is
needed to reset the event. Events do not count signals like a semaphore
does due to their binary behavior. An event needs only one signal to be
available and only needs to be tested once to become clear and
unavailable.

Function
********

Events were designed for interrupt service routines and nanokernel
fibers that need to wake up a waiting task. The event signal can be
passed to a task to trigger an event test to RC_OK. Events are the
easiest and most efficient way to wake up a task to synchronize
operations between the two levels.

A feature of events are the event handlers. Event handlers are attached
to events. They perform simple processing in the nanokernel before a
context switch is made to a blocked task. This way, signals can be
interpreted before the system requires to reschedule a fiber or task.

Only one task may wait for an event. If a second task tests the same
event the call returns a fail. Use semaphores for multiple tasks to
wait on a signal from them.

Usage
*****

Defining an Event
=================

The following parameters must be defined:

   *name*
          This specifies a unique name for the event.

   *handler*
          This specifies the name of the event handler function to be executed
          each time the event is signalled. If no event handler is required
          specify NULL.


Public Event
------------

Define the event in the application's .MDEF file using the following syntax:

.. code-block:: console

   EVENT name handler

For example, the file :file:`projName.mdef` defines two events
as follows:

.. code-block:: console

    % EVENT NAME            ENTRY
    % ==========================================
      EVENT KEYPRESS        validate_keypress
      EVENT BUTTONPRESS     NULL

A public event can be referenced from any source file that includes
the file :file:`zephyr.h`.

.. note::
   Private events are not supported by the Zephyr kernel.


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
           task_event_recv(KEYPRESS);

           /* determine what key was pressed */
           char c = get_keypress();

           /* process key press */
           ...
       }
   }

Example: Filtering Event Signals using an Event Handler
=======================================================

This code registers an event handler that filters out unwanted events
so that the consuming task only wakes up when needed.

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
           task_event_recv(KEYPRESS);

           /* process saved key press, which must be a digit */
           ...
       }
   }


APIs
****

The following Event APIs are provided by microkernel.h.

+------------------------------------------+----------------------------------+
| Call                                     | Description                      |
+==========================================+==================================+
| :c:func:`isr_event_send()`               | Signal an event from an ISR      |
+------------------------------------------+----------------------------------+
| :c:func:`fiber_event_send()`             | Signal an event from a fiber.    |
+------------------------------------------+----------------------------------+
| :c:func:`task_event_send()`              | Signal an event from a task.     |
+------------------------------------------+----------------------------------+
| :c:func:`task_event_recv()`              | Tests for an event signal        |
|                                          | without waiting.                 |
+------------------------------------------+----------------------------------+
| :c:func:`task_event_recv_wait()`         | Waits for an event signal.       |
+------------------------------------------+----------------------------------+
| :c:func:`task_event_recv_wait_timeout()` | Waits for an event signal        |
|                                          | for a specified time period.     |
+------------------------------------------+----------------------------------+
| :c:func:`task_event_handler_set()`       | Registers an event handler       |
|                                          | function for an event.           |
+------------------------------------------+----------------------------------+
