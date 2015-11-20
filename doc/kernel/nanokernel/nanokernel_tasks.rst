.. _nanokernel_tasks:

Task Services
#############

Concepts
********

A task is a preemptible thread of execution that implements a portion of
an application's processing. It is is normally used to perform processing that
is too lengthy or complex to be performed by a fiber or an ISR.

A nanokernel application can define only a single application task, known
as the *background task*, which executes when there is no fiber or ISR
that needs to execute. The entry point function for the background task
is :c:func:`main()`, and must be supplied by the application.

.. note::
   The background task is very different from the tasks used by a microkernel
   application; for more information see
   :ref:`Microkernel Task Services <microkernel_tasks>`.

Task Lifecycle
==============

The kernel automatically starts the background task during system
initialization.

Once the background task is started it executes forever. If the task attempts
to terminate by returning from :c:func:`main()`, the kernel puts the task into
a permanant idling state since the background task must always be available
to execute.

Task Scheduling
===============

The nanokernel's scheduler executes the background task only when there is no
fiber or ISR that needs to execute, since fiber and ISR execution takes
precedence.

The kernel automatically takes care of saving the background task's CPU register
values when it performs a context switch to a fiber or an ISR. It restores
these values when the background task later resumes execution.


Usage
*****

Defining the Background Task
============================

The application must supply a function of the following form:

.. code-block:: c

   void main(void)
   {
       /* background task processing */
       ...
       /* (optional) enter permanent idling state */
       return;
   }

This function is used as the background task's entry point function. If a
nanokernel application does not need to perform any task-level processing
:c:func:`main()` can simply do an immediate return.

The :option:`MAIN_STACK_SIZE` configuration option specifies the size,
in bytes, of the memory region used for the background task's stack
and for other execution context information.

APIs
****

The nanokernel provides the following API for manipulating the background task.

:cpp:func:`task_sleep()`
   Put the background task to sleep for a specified time period.
