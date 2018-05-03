.. _stacks_v2:

Stacks
######

A :dfn:`stack` is a kernel object that implements a traditional
last in, first out (LIFO) queue, allowing threads and ISRs
to add and remove a limited number of 32-bit data values.

.. contents::
    :local:
    :depth: 2

Concepts
********

Any number of stacks can be defined. Each stack is referenced
by its memory address.

A stack has the following key properties:

* A **queue** of 32-bit data values that have been added but not yet removed.
  The queue is implemented using an array of 32-bit integers,
  and must be aligned on a 4-byte boundary.

* A **maximum quantity** of data values that can be queued in the array.

A stack must be initialized before it can be used. This sets its queue to empty.

A data value can be **added** to a stack by a thread or an ISR.
The value is given directly to a waiting thread, if one exists;
otherwise the value is added to the lifo's queue.
The kernel does *not* detect attempts to add a data value to a stack
that has already reached its maximum quantity of queued values.

.. note::
    Adding a data value to a stack that is already full will result in
    array overflow, and lead to unpredictable behavior.

A data value may be **removed** from a stack by a thread.
If the stack's queue is empty a thread may choose to wait for it to be given.
Any number of threads may wait on an empty stack simultaneously.
When a data item is added, it is given to the highest priority thread
that has waited longest.

.. note::
    The kernel does allow an ISR to remove an item from a stack, however
    the ISR must not attempt to wait if the stack is empty.

Implementation
**************

Defining a Stack
================

A stack is defined using a variable of type :c:type:`struct k_stack`.
It must then be initialized by calling :cpp:func:`k_stack_init()` or
:cpp:func:`k_stack_alloc_init()`. In the latter case, a buffer is not
provided and it is instead allocated from the calling thread's resource
pool.

The following code defines and initializes an empty stack capable of holding
up to ten 32-bit data values.

.. code-block:: c

    #define MAX_ITEMS 10

    u32_t my_stack_array[MAX_ITEMS];
    struct k_stack my_stack;

    k_stack_init(&my_stack, my_stack_array, MAX_ITEMS);

Alternatively, a stack can be defined and initialized at compile time
by calling :c:macro:`K_STACK_DEFINE`.

The following code has the same effect as the code segment above. Observe
that the macro defines both the stack and its array of data values.

.. code-block:: c

    K_STACK_DEFINE(my_stack, MAX_ITEMS);

Pushing to a Stack
==================

A data item is added to a stack by calling :cpp:func:`k_stack_push()`.

The following code builds on the example above, and shows how a thread
can create a pool of data structures by saving their memory addresses
in a stack.

.. code-block:: c

    /* define array of data structures */
    struct my_buffer_type {
        int field1;
        ...
	};
    struct my_buffer_type my_buffers[MAX_ITEMS];

    /* save address of each data structure in a stack */
    for (int i = 0; i < MAX_ITEMS; i++) {
        k_stack_push(&my_stack, (u32_t)&my_buffers[i]);
    }

Popping from a Stack
====================

A data item is taken from a stack by calling :cpp:func:`k_stack_pop()`.

The following code builds on the example above, and shows how a thread
can dynamically allocate an unused data structure.
When the data structure is no longer required, the thread must push
its address back on the stack to allow the data structure to be reused.

.. code-block:: c

    struct my_buffer_type *new_buffer;

    k_stack_pop(&buffer_stack, (u32_t *)&new_buffer, K_FOREVER);
    new_buffer->field1 = ...

Suggested Uses
**************

Use a stack to store and retrieve 32-bit data values in a "last in,
first out" manner, when the maximum number of stored items is known.

Configuration Options
*********************

Related configuration options:

* None.

APIs
****

The following stack APIs are provided by :file:`kernel.h`:

* :c:macro:`K_STACK_DEFINE`
* :cpp:func:`k_stack_init()`
* :cpp:func:`k_stack_push()`
* :cpp:func:`k_stack_pop()`
