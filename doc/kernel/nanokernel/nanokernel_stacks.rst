.. _nanokernel_stacks:

Nanokernel Stacks
#################

Concepts
********

The nanokernel's stack object type is an implementation of a traditional
last in, first out queue for a limited number of 32-bit data values.
It is mainly intended for use by fibers.

Each stack uses an array of 32-bit words to hold its data values. The array
may be of any size, but must be aligned on a 4-byte boundary.

Any number of nanokernel stacks can be defined. Each stack is a distinct
variable of type :c:type:`struct nano_stack`, and is referenced using a pointer
to that variable. A stack must be initialized to use its array before it
can be used to send or receive data values.

Data values can be added to a stack in a non-blocking manner by any context type
(i.e. ISR, fiber, or task).

.. note::
   A context must not attempt to add a data value to a stack whose array
   is already full, as the resulting array overflow will lead to
   unpredictable behavior.

Data values can be removed from a stack in a non-blocking manner by any context
type; if the stack is empty a special return code indicates that no data value
was removed. Data values can also be removed from a stack in a blocking manner
by a fiber or task; if the stack is empty the fiber or task waits for a data
value to be added.

Only a single fiber, but any number of tasks, may wait on an empty nanokernel
stack simultaneously. When a data value becomes available it is given to the
waiting fiber, or to a waiting task if no fiber is waiting.

.. note::
   The nanokernel does not allow more than one fiber to wait on a nanokernel
   stack. If a second fiber starts waiting the first waiting fiber is
   superseded and ends up waiting forever.

   A task that waits on an empty nanokernel stack does a busy wait. This is
   not an issue for a nanokernel application's background task; however, in
   a microkernel application a task that waits on a nanokernel stack remains
   the current task. In contrast, a microkernel task that waits on a
   microkernel data passing object ceases to be the current task, allowing
   other tasks of equal or lower priority to do useful work.

   If multiple tasks in a microkernel application wait on the same nanokernel
   stack, higher priority tasks are given data values in preference to lower
   priority tasks. However, the order in which equal priority tasks are given
   data values is unpredictible.


Purpose
*******

Use a nanokernel stack to store and retrieve 32-bit data values in a "last in,
first out" manner, when the maximum number of stored items is known.


Usage
*****

Example: Initializing a Nanokernel Stack
========================================

This code establishes an empty nanokernel stack capable of holding
up to 10 items.

.. code-block:: c

   #define MAX_ALARMS 10

   struct nano_stack alarm_stack;

   uint32_t stack_area[MAX_ALARMS];

   ...

   nano_stack_init(&alarm_stack, stack_area);


Example: Writing to a Nanokernel Stack
======================================

This code shows how an ISR can use a nanokernel stack to pass a 32-bit alarm
indication to a processing fiber.

.. code-block:: c

   #define OVERHEAT_ALARM   17

   void overheat_interrupt_handler(void *arg)
   {
       ...
       /* report alarm */
       nano_isr_stack_push(&alarm_stack, OVERHEAT_ALARM);
       ...
   }


Example: Reading from a Nanokernel Stack
========================================

This code shows how a fiber can use a nanokernel stack to retrieve 32-bit alarm
indications signalled by other parts of the application,
such as ISRs and other fibers. It is assumed that the fiber can handle
bursts of alarms before the stack overflows, and that the order
in which alarms are processed isn't significant.

.. code-block:: c

   void alarm_handler_fiber(int arg1, int arg2)
   {
       uint32_t alarm_number;

       while (1) {
           /* wait for an alarm to be reported */
           alarm_number = nano_fiber_stack_pop_wait(&alarm_stack);
           /* process alarm indication */
           ...
       }
   }


APIs
****

The following APIs for a nanokernel stack are provided by :file:`nanokernel.h.`

+-----------------------------------------+-----------------------------------+
| Call                                    | Description                       |
+=========================================+===================================+
| :c:func:`nano_stack_init()`             | Initializes a stack.              |
+-----------------------------------------+-----------------------------------+
| | :c:func:`nano_task_stack_push()`      | Adds item to a stack.             |
| | :c:func:`nano_fiber_stack_push()`     |                                   |
| | :c:func:`nano_isr_stack_push()`       |                                   |
+-----------------------------------------+-----------------------------------+
| | :c:func:`nano_task_stack_pop()`       | Removes item from a stack,        |
| | :c:func:`nano_fiber_stack_pop()`      | or fails and continues            |
| | :c:func:`nano_isr_stack_pop()`        | if it is empty.                   |
+-----------------------------------------+-----------------------------------+
| | :c:func:`nano_task_stack_pop_wait()`  | Removes item from a stack, or     |
| | :c:func:`nano_fiber_stack_pop_wait()` | waits for an item if it is empty. |
+-----------------------------------------+-----------------------------------+

