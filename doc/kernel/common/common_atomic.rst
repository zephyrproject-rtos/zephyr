.. _atomic_services:

Atomic Services
###############

Concepts
********

The kernel supports an atomic 32-bit data type, called :c:type:`atomic_t`.
A variable of this type can be read and modified by any task, fiber, or ISR
in an uninterruptible manner. This guarantees that the desired operation
will not be interfered with due to the scheduling of a higher priority context,
even if the higher priority context manipulates the same variable.


Purpose
*******

Use the atomic services to implement critical section processing that only
requires the manipulation of a single 32-bit data item.

.. note::
   Using an atomic variable is typically far more efficient than using
   other techniques to implement critical sections, such as using
   a microkernel mutex, offloading the processing to a fiber, or
   locking interrupts.


Usage
*****

Example: Implementing an Uninterruptible Counter
================================================
This code shows how a function can keep track of the number of times
it has been invoked. Since the count is incremented atomically, there is
no risk that it will become corrupted in mid-increment if the routine is
interrupted by the scheduling of a higher priority context that also
calls the routine.

.. code-block:: c

   atomic_t call_count;

   int call_counting_routine(void)
   {
       /* increment invocation counter */
       atomic_inc(&call_count);

       /* do rest of routine's processing */
       ...
   }


APIs
****

The following atomic operation APIs are provided by :file:`atomic.h`.

+---------------------------------------+-------------------------------------+
| Call                                  | Description                         |
+=======================================+=====================================+
| :c:func:`atomic_get()`                | Reads an atomic variable.           |
+---------------------------------------+-------------------------------------+
| :c:func:`atomic_set()`                | Writes an atomic variable.          |
+---------------------------------------+-------------------------------------+
| :c:func:`atomic_clear()`              | Zeroes an atomic variable.          |
+---------------------------------------+-------------------------------------+
| | :c:func:`atomic_add()`              | Performs an arithmetic operation    |
| | :c:func:`atomic_sub()`              | on an atomic variable.              |
| | :c:func:`atomic_inc()`              |                                     |
| | :c:func:`atomic_dec()`              |                                     |
+---------------------------------------+-------------------------------------+
| | :c:func:`atomic_and()`              | Performs a logical operation        |
| | :c:func:`atomic_or()`               | on an atomic variable.              |
| | :c:func:`atomic_xor()`              |                                     |
| | :c:func:`atomic_nand()`             |                                     |
+---------------------------------------+-------------------------------------+
| :c:func:`atomic_cas()`                | Performs compare-and-set operation  |
|                                       | on an atomic variable.              |
+---------------------------------------+-------------------------------------+
| :c:func:`atomic_set_bit()`            | Sets specified bit of an atomic     |
|                                       | variable to 1.                      |
+---------------------------------------+-------------------------------------+
| :c:func:`atomic_clear_bit()`          | Sets specified bit of an atomic     |
|                                       | variable to 0.                      |
+---------------------------------------+-------------------------------------+
| :c:func:`atomic_test_bit()`           | Reads specified bit of an atomic    |
|                                       | variable.                           |
+---------------------------------------+-------------------------------------+
| :c:func:`atomic_test_and_set_bit()`   | Reads specified bit of an atomic    |
|                                       | variable and sets it to 1.          |
+---------------------------------------+-------------------------------------+
| :c:func:`atomic_test_and_clear_bit()` | Reads specified bit of an atomic    |
|                                       | variable and sets it to 0.          |
+---------------------------------------+-------------------------------------+
