.. zephyr:code-sample:: syscall_perf
   :name: Syscall performance

   Measure performance overhead of a system calls compared to direct function calls.

Syscall performances
====================

The goal of this sample application is to measure the performance loss when a
user thread has to go through a system call compared to a supervisor thread that
calls the function directly.


Overview
********

This application creates a supervisor and a user thread.
Then both threads call :c:func:`k_current_get()` which returns a reference to the
current thread. The user thread has to go through a system call.

Both threads are showing the number of core clock cycles and the number of
instructions executed while calling :c:func:`k_current_get()`.


Sample Output
*************

.. code-block:: console

    User thread:		   18012 cycles	     748 instructions
    Supervisor thread:	       7 cycles	       4 instructions
    User thread:		   20136 cycles	     748 instructions
    Supervisor thread:	       7 cycles	       4 instructions
    User thread:		   18014 cycles	     748 instructions
    Supervisor thread:	       7 cycles	       4 instructions
