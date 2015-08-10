.. _nanokernel_stacks:

Nanokernel Stacks
#################

Concepts
********

The nanokernel's stack object provides "last in, first out" queuing
of a limited number of 32-bit data values.

Any number of nanokernel stacks can be defined. Each stack must have an
associated array of integers that holds its data values. The array may
be of any size, and is bound to the stack when the stack is initialized.

Items can be added to a stack in a non-blocking manner by any context type
(i.e. ISR, fiber, or task).

.. note::
   A context must not attempt to add an item to a stack whose array
   is already full, as the resulting array overflow will lead to
   unpredictable behavior.

Items can be removed from a stack in a non-blocking manner by any context type;
if the stack is empty a special return code indicates that no item was removed.
Items can also be removed from a stack in a blocking manner by a single fiber
or task; if the stack is empty the fiber or task waits for an item to be added.

.. note::
   The nanokernel does not allow more than one context to wait on a stack
   at any given time. If a second context starts waiting the first waiting
   context is superseded and ends up waiting forever.


Purpose
*******

Use a stack to store and retrieve 32-bit data values in a "last in, first out"
manner, when the maximum number of stored items is known.


Usage
*****

Example: Initializing a Nanokernel Stack
========================================

This code establishes a stack capable of holding up to 10 items.

.. code-block:: c

   #define MAX_ALARMS 10

   struct nano_stack alarm_stack;

   uint32_t stack_area[MAX_ALARMS];

   ...

   nano_stack_init(&alarm_stack, stack_area);


Example: Writing to a Nanokernel Stack
======================================

This code shows how an ISR can use a stack to pass a 32-bit alarm indication
to a processing fiber.

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

This code shows how a fiber can use a stack to retrieve 32-bit alarm
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

