.. _events:

Events
******

Definition
==========

Event objects are microkernel synchronization objects that tasks can
signal and test. Fibers and interrupt service routines may signal
events but they cannot test or wait on them. Use event objects for
situations in which multiple signals come in but only one test is
needed to reset the event. Events do not count signals like a semaphore
does due to their binary behavior. An event needs only one signal to be
available and only needs to be tested once to become clear and
unavailable.

Function
========

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

Initialization
==============


An event has to be defined in the project file, :file:`projName.mdef`.
Specify the name of the event, the name of the processor node that
manages it, and its event-handler function. Use the following syntax:

.. code-block:: console

   EVENT %name %handler

.. note::

   In the project file, you can specify the name of the event and the
   event handler, but not the event's number.

Define application events in the project’s MDEF file. Define the driver’s
events in either the project’s MDEF file or a BSP-specific MDEF file.

Application Program Interfaces
==============================

Event APIs allow signaling or testing an event (blocking or
non-blocking), and setting the event handler.

If the event is in a signaled state, the test function returns
successfully and resets the event to the non-signaled state. If the
event is not signaled at the time of the call, the test either reports
failure immediately in case of a non-blocking call, or blocks the
calling task into a until the event signal becomes available.

+---------------------------------------+-----------------------------------+
| Call                                  | Description                       |
+=======================================+===================================+
| :c:func:`fiber_event_send()`          | Signal an event from a fiber.     |
+---------------------------------------+-----------------------------------+
| :c:func:`task_event_set_handler()`    | Installs or removes an event      |
|                                       | handler function from a task.     |
+---------------------------------------+-----------------------------------+
| :c:func:`task_event_send()`           | Signal an event from a task.      |
+---------------------------------------+-----------------------------------+
| :c:func:`task_event_recv()`           | Waits for an event signal.        |
+---------------------------------------+-----------------------------------+
| :c:func:`task_event_recv_wait()`      | Waits for an event signal with a  |
|                                       | delay.                            |
+---------------------------------------+-----------------------------------+
| :c:func:`task_event_recv_wait_timeout()` | Waits for an event signal with |
|                                          | a delay and a timeout.         |
+---------------------------------------+-----------------------------------+
| :c:func:`isr_event_send()`            | Signal an event from an ISR       |
+---------------------------------------+-----------------------------------+