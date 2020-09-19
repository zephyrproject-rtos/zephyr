.. _atomic_v2:

Atomic Services
###############

An :dfn:`atomic variable` is a 32-bit variable that can be read and modified
by threads and ISRs in an uninterruptible manner.

.. contents::
    :local:
    :depth: 2

Concepts
********

Any number of atomic variables can be defined (limited only by available RAM).

Using the kernel's atomic APIs to manipulate an atomic variable
guarantees that the desired operation occurs correctly,
even if higher priority contexts also manipulate the same variable.

The kernel also supports the atomic manipulation of a single bit
in an array of atomic variables.

Implementation
**************

Defining an Atomic Variable
===========================

An atomic variable is defined using a variable of type :c:type:`atomic_t`.

By default an atomic variable is initialized to zero. However, it can be given
a different value using :c:macro:`ATOMIC_INIT`:

.. code-block:: c

    atomic_t flags = ATOMIC_INIT(0xFF);

Manipulating an Atomic Variable
===============================

An atomic variable is manipulated using the APIs listed at the end of
this section.

The following code shows how an atomic variable can be used to keep track
of the number of times a function has been invoked. Since the count is
incremented atomically, there is no risk that it will become corrupted
in mid-increment if a thread calling the function is interrupted if
by a higher priority context that also calls the routine.

.. code-block:: c

    atomic_t call_count;

    int call_counting_routine(void)
    {
        /* increment invocation counter */
        atomic_inc(&call_count);

        /* do rest of routine's processing */
        ...
    }

Manipulating an Array of Atomic Variables
=========================================

An array of 32-bit atomic variables can be defined in the conventional manner.
However, you can also define an N-bit array of atomic variables using
:c:macro:`ATOMIC_DEFINE`.

A single bit in array of atomic variables can be manipulated using
the APIs listed at the end of this section that end with :c:func:`_bit`.

The following code shows how a set of 200 flag bits can be implemented
using an array of atomic variables.

.. code-block:: c

    #define NUM_FLAG_BITS 200

    ATOMIC_DEFINE(flag_bits, NUM_FLAG_BITS);

    /* set specified flag bit & return its previous value */
    int set_flag_bit(int bit_position)
    {
        return (int)atomic_set_bit(flag_bits, bit_position);
    }

Suggested Uses
**************

Use an atomic variable to implement critical section processing that only
requires the manipulation of a single 32-bit value.

Use multiple atomic variables to implement critical section processing
on a set of flag bits in a bit array longer than 32 bits.

.. note::
    Using atomic variables is typically far more efficient than using
    other techniques to implement critical sections such as using a mutex
    or locking interrupts.

Configuration Options
*********************

Related configuration options:

* :option:`CONFIG_ATOMIC_OPERATIONS_BUILTIN`
* :option:`CONFIG_ATOMIC_OPERATIONS_CUSTOM`
* :option:`CONFIG_ATOMIC_OPERATIONS_C`

API Reference
*************

.. important::
    All atomic services APIs can be used by both threads and ISRs.

.. doxygengroup:: atomic_apis
   :project: Zephyr
