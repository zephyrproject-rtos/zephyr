.. _microkernelObjects:

Microkernel Objects
###################

Section Scope
*************

This section provides an overview of the most important microkernel
objects, and their operation.

Each object contains a definition, a function description, and a table
of Application Program Interfaces (API) including the context that may
call them. Please refer to the API documentation for further details
regarding each object’s functionality.

Microkernel FIFO Objects
************************

Definition
==========

The FIFO object is defined in :file:`include/microkernel/fifo.h` as a simple
first-in, first-out queue that handle small amounts of fixed size data.
FIFO objects have a buffer that stores a number of data transmits, and are the most
efficient way to pass small amounts of data between tasks. FIFO objects
are suitable for asynchronously transferring small amounts of data,
such as parameters, between tasks.

Function
========


FIFO objects store data in a statically allocated buffer defined within
the project’s MDEF file. The depth of the FIFO object buffer is only
limited by the available memory on the platform. Individual FIFO data
objects can be at most 40 bytes in size, and are stored in an ordered
first-come, first-serve basis, not by priority.

FIFO objects are asynchronous. When using a FIFO object, the sender can
add data even if the receiver is not ready yet. This only applies if
there is sufficient space on the buffer to store the sender's data.

FIFO objects are anonymous. The kernel object does not store the sender
or receiver identity. If the sender identification is required, it is
up to the caller to store that information in the data placed into the
FIFO. The receiving task can then check it. Alternatively, mailboxes
can be used to specify the sender and receiver identities.

FIFO objects read and write actions are always fixed-size block-based.
The width of each FIFO object block is specified in the project file.
If a task calls :c:func:`task_fifo_get()` and the call succeeds, then
the fixed number of bytes is copied from the FIFO object into the
addresses of the destination pointer.

Initialization
==============
FIFO objects are created by defining them in a project file, for example
:file:`projName.mdef`. Specify the name of the FIFO object, the width in
bytes of a single entry, the number of entries, and, if desired, the
location defined in the architecture file to be used for the FIFO. Use
the following syntax in the MDEF file to define a FIFO:

.. code-block:: console

   FIFO %name %depthNumEntries %widthBytes [ bufSegLocation ]

An example of a FIFO entry for use in the MDEF file:

.. code-block:: console

   % FIFO NAME       DEPTH WIDTH
   % ============================

     FIFO FIFOQ        2     4


Application Program Interfaces
==============================
The FIFO object APIs allow to putting data on the queue, receiving data
from the queue, finding the number of messages in the queue, and
emptying the queue.

+----------------------------------------+-------------------------------------------------+
| **Call**                               | **Description**                                 |
+----------------------------------------+-------------------------------------------------+
| :c:func:`task_fifo_put()`              | Put data on a FIFO, and fail                    |
|                                        | if the FIFO is full.                            |
+----------------------------------------+-------------------------------------------------+
| :c:func:`task_fifo_put_wait()`         | Put data on a FIFO, waiting                     |
|                                        | for room in the FIFO.                           |
+----------------------------------------+-------------------------------------------------+
| :c:func:`task_fifo_put_wait_timeout()` | Put data on a FIFO, waiting                     |
|                                        | for room in the FIFO, or a time out.            |
+----------------------------------------+-------------------------------------------------+
| :c:func:`task_fifo_get()`              | Get data off a FIFO,                            |
|                                        | returning immediately if no data is available.  |
+----------------------------------------+-------------------------------------------------+
| :c:func:`task_fifo_get_wait()`         | Get data off a FIFO,                            |
|                                        | waiting until data is available.                |
+----------------------------------------+-------------------------------------------------+
| :c:func:`task_fifo_get_wait_timeout()` | Get data off a FIFO,                            |
|                                        | waiting until data is available, or a time out. |
+----------------------------------------+-------------------------------------------------+
| :c:func:`task_fifo_purge()`            | Empty the FIFO buffer, and                      |
|                                        | signal any waiting receivers with an error.     |
+----------------------------------------+-------------------------------------------------+
| :c:func:`task_fifo_size_get()`         | Read the number of filled                       |
|                                        | entries in a FIFO.                              |
+----------------------------------------+-------------------------------------------------+

Pipe Objects
************

Definition
==========

Microkernel pipes are defined in :file:`kernel/microkernel/k_pipe.c`.
Pipes allow any task to put any amount of data in or out. Pipes are
conceptually similar to FIFO objects in that they communicate
anonymously in a time-ordered, first-in, first-out manner, to exchange
data between tasks. Like FIFO objects, pipes can have a buffer, but
un-buffered operation is also possible. The main difference between
FIFO objects and pipes is that pipes handle variable-sized data.

Function
========

Pipes accept and send variable-sized data, and can be configured to work
with or without a buffer. Buffered pipes are time-ordered. The incoming
data is stored on a first-come, first-serve basis in the buffer; it is
not sorted by priority.

Pipes have no size limit. The size of the data transfer and the size of
the buffer have no limit except for the available memory. Pipes allow
senders or receivers to perform partial read and partial write
operations.

Pipes support both synchronous and asynchronous operations. If a pipe is
unbuffered, the sender can asynchronously put data into the pipe, or
wait for the data to be received, and the receiver can attempt to
remove data from the pipe, or wait on the data to be available.
Buffered pipes are synchronous by design.

Pipes are anonymous. The pipe transfer does not identify the sender or
receiver. Alternatively, mailboxes can be used to specify the sender
and receiver identities.

Initialization
==============


A target pipe has to be defined in the project file, for example
:file:`projName.mdef`. Specify the name of the pipe, the size of the
buffer in bytes, and the memory location for the pipe buffer as defined
in the linker script. The buffer’s memory is allocated on the processor
that manages the pipe. Use the following syntax in the MDEF file to
define a pipe:

.. code-block:: console

   PIPE %name %buffersize [%bufferSegment]

An example of a pipe entry for use in the MDEF file:

.. code-block:: console

   % PIPE    NAME           BUFFERSIZE [BUFFER_SEGMENT]

   % ===================================================

     PIPE    PIPE_ID           256


Application Program Interfaces
==============================

The pipes APIs allow to sending and receiving data to and from a pipe.

+----------------------------------------+----------------------------------------+
| **Call**                               | **Description**                        |
+----------------------------------------+----------------------------------------+
| :c:func:`task_pipe_put()`              | Put data on a pipe                     |
+----------------------------------------+----------------------------------------+
| :c:func:`task_pipe_put_wait()`         | Put data on a pipe with a delay.       |
+----------------------------------------+----------------------------------------+
| :c:func:`task_pipe_put_wait_timeout()` | Put data on a pipe with a timed delay. |
+----------------------------------------+----------------------------------------+
| :c:func:`task_pipe_get()`              | Put data on a pipe.                    |
+----------------------------------------+----------------------------------------+
| :c:func:`task_pipe_get_wait()`         | Put data on a pipe with a delay.       |
+----------------------------------------+----------------------------------------+
| :c:func:`task_pipe_get_wait_timeout()` | Put data on a pipe with a timed delay. |
+----------------------------------------+----------------------------------------+
| :c:func:`task_pipe_put_async()`        | Put data on a pipe asynchronously.     |
+----------------------------------------+----------------------------------------+

Mailbox Objects
***************

Definition
==========

A mailbox object is defined in include :file:`/microkernel/mailbox.h`.
Mailboxes are a flexible way to pass data and for tasks to exchange messages.

Function
========

Each transfer within a mailbox can vary in size. The size of a data
transfer is only limited by the available memory on the platform.
Transmitted data is not buffered in the mailbox itself. Instead, the
buffer is either allocated from a memory pool block, or in block of
memory defined by the user.

Mailboxes can work synchronously and asynchronously. Asynchronous
mailboxes require the sender to allocate a buffer from a memory pool
block, while synchronous mailboxes will copy the sender data to the
receiver buffer.

The transfer contains one word of information that identifies either the
sender, or the receiver, or both. The sender task specifies the task it
wants to send to. The receiver task specifies the task it wants to
receive from. Then the mailbox checks the identity of the sender and
receiver tasks before passing the data.

Initialization
==============

A mailbox has to be defined in the project file, for example
:file:`projName.mdef`, which will specify the object type, and the name
of the mailbox. Use the following syntax in the MDEF file to define a
Mailbox:

.. code-block:: console

   MAILBOX %name

An example of a mailbox entry for use in the MDEF file:

.. code-block:: console

   % MAILBOX   NAME

   % =================

     MAILBOX   MYMBOX



Application Program Interfaces
==============================


Mailbox APIs provide flexibility and control for transferring data
between tasks.

+--------------------------------------------+---------------------------------------------------------------------+
| **Call**                                   | **Description**                                                     |
+--------------------------------------------+---------------------------------------------------------------------+
| :c:func:`task_mbox_put()`                  | Attempt to put data in a                                            |
|                                            | mailbox, and fail if the receiver isn’t waiting.                    |
+--------------------------------------------+---------------------------------------------------------------------+
| :c:func:`task_mbox_put_wait()`             | Puts data in a mailbox,                                             |
|                                            | and waits for it to be received.                                    |
+--------------------------------------------+---------------------------------------------------------------------+
| :c:func:`task_mbox_put_wait_timeout()`     | Puts data in a mailbox,                                             |
|                                            | and waits for it to be received, with a timeout.                    |
+--------------------------------------------+---------------------------------------------------------------------+
| :c:func:`task_mbox_put_async()`            | Puts data in a mailbox                                              |
|                                            | asynchronously.                                                     |
|                                            |                                                                     |
+--------------------------------------------+---------------------------------------------------------------------+
| :c:func:`task_mbox_get()`                  | Gets k_msg message                                                  |
|                                            | header information from a mailbox and gets mailbox data, or returns |
|                                            | immediately if the sender isn’t ready.                              |
+--------------------------------------------+---------------------------------------------------------------------+
| :c:func:`task_mbox_get_wait()`             | Gets k_msg message                                                  |
|                                            | header information from a mailbox and gets mailbox data, and waits  |
|                                            | until the sender is ready with data.                                |
+--------------------------------------------+---------------------------------------------------------------------+
| :c:func:`task_mbox_get_wait_timeout()`     | Gets k_msg message                                                  |
|                                            | header information from a mailbox and gets mailbox data, and waits  |
|                                            | until the sender is ready with a timeout.                           |
+--------------------------------------------+---------------------------------------------------------------------+
| :c:func:`task_mbox_data_get()`             | Gets mailbox data and                                               |
|                                            | puts it in a buffer specified by a pointer.                         |
|                                            |                                                                     |
+--------------------------------------------+---------------------------------------------------------------------+
| :c:func:`task_mbox_data_get_async_block()` | Gets the mailbox data                                               |
|                                            | and puts it in a memory pool block.                                 |
|                                            |                                                                     |
+--------------------------------------------+---------------------------------------------------------------------+

Semaphore Objects
*****************

Definition
==========

The microkernel semaphore is defined in
:file:`kernel/microkernel/k_sema.c` and are an implementation of
traditional counting semaphores. Semaphores are used to synchronize
application task activities.

Function
========

Semaphores are initialized by the system. At start the semaphore is
un-signaled and no task is waiting for it. Any task in the system can
signal a semaphore. Every signal increments the count value associated
with the semaphore. When several tasks wait for the same semaphore at
the same time, they are held in a prioritized list. If the semaphore is
signaled, the task with the highest priority is released. If more tasks
of that priority are waiting, the first one that requested the
semaphore wakes up. Other tasks can test the semaphore to see if it is
signaled. If not signaled, tasks can either wait, with or without a
timeout, until signaled or return immediately with a failed status.

Initialization
==============

A semaphore has to be defined in the project file, for example
:file:`projName.mdef`, which will specify the object type, and the name
of the semaphore. Use the following syntax in the MDEF file to define a
semaphore::

.. code-block:: console

   SEMA %name %node

An example of a semaphore entry for use in the MDEF file:

.. code-block:: console

   % SEMA   NAME

   % =================

     SEMA   SEM_TASKDONE



Application Program Interfaces
==============================

Semaphore APIs allow signaling a semaphore. They also provide means to
reset the signal count.

+----------------------------------------+---------------------------------------------------+
| **Call**                               | **Description**                                   |
+----------------------------------------+---------------------------------------------------+
| :c:func:`isr_sem_give()`               | Signal a semaphore from an ISR.                   |
+----------------------------------------+---------------------------------------------------+
| :c:func:`task_sem_give()`              | Signal a semaphore from a task.                   |
+----------------------------------------+---------------------------------------------------+
| :c:func:`task_sem_take()`              | Test a semaphore from a task.                     |
+----------------------------------------+---------------------------------------------------+
| :c:func:`task_sem_take_wait()`         | Wait on a semaphore from a task.                  |
+----------------------------------------+---------------------------------------------------+
| :c:func:`task_sem_take_wait_timeout()` | Wait on a semaphore, with a timeout, from a task. |
+----------------------------------------+---------------------------------------------------+
| :c:func:`task_sem_group_reset()`       | Sets a list of semaphores to zero.                |
+----------------------------------------+---------------------------------------------------+
| :c:func:`task_sem_group_give()`        | Signals a list of semaphores from a task.         |
+----------------------------------------+---------------------------------------------------+
| :c:func:`task_sem_reset()`             | Sets a semaphore to zero.                         |
+----------------------------------------+---------------------------------------------------+

Event Objects
*************

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

   EVENT name handler

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

+------------------------------------------+------------------------------------------------------------+
| **Call**                                 | **Description**                                            |
+------------------------------------------+------------------------------------------------------------+
| :c:func:`fiber_event_send()`             | Signal an event from a fiber.                              |
+------------------------------------------+------------------------------------------------------------+
| :c:func:`task_event_set_handler()`       | Installs or removes an event handler function from a task. |
+------------------------------------------+------------------------------------------------------------+
| :c:func:`task_event_send()`              | Signal an event from a task.                               |
+------------------------------------------+------------------------------------------------------------+
| :c:func:`task_event_recv()`              | Waits for an event signal.                                 |
+------------------------------------------+------------------------------------------------------------+
| :c:func:`task_event_recv_wait()`         | Waits for an event signal with a delay.                    |
+------------------------------------------+------------------------------------------------------------+
| :c:func:`task_event_recv_wait_timeout()` | Waits for an event signal with a delay and a timeout.      |
+------------------------------------------+------------------------------------------------------------+
| :c:func:`isr_event_send()`               | Signal an event from an ISR                                |
+------------------------------------------+------------------------------------------------------------+


MUTEXES
*******


This microkernel object is organized with the following sections:

* Concepts: what the object is and its characteristics

* Purpose:  why you would use the object

* Usage:    how (when and where) you use the object and examples

* APIs:     list of object-related APIs


Concepts
========

The microkernel's mutex objects provide reentrant mutex
capabilities with priority inheritance.

Each mutex allows multiple tasks to safely share an associated
resource by ensuring mutual exclusivity while the resource is
being accessed by a task.

Any number of mutexes can be defined in a microkernel system.
Each mutex has a name that uniquely identifies it. Typically,
the name should relate to the resource being shared, but this is
not a requirement.

A task that needs to use a shared resource must first gain
exclusive access by locking the associated mutex. When the mutex
is already locked by another task, the requesting task may
choose to wait for the mutex to be unlocked. After obtaining the lock,
a task can safely use the shared resource for as long as needed.
When the task no longer needs the resource, it must unlock the
associated mutex to allow other tasks to use the resource.

Any number of tasks may wait on a locked mutex simultaneously.
When there is more than one waiting task, the mutex locks the
resource for the highest priority task that has waited the longest
first.

Priority inheritance occurs whenever a high priority task waits
on a mutex that is locked by a lower priority task. The priority
of the lower priority task increases temporarily to the priority
of the highest priority task that is waiting on a mutex held by
the lower priority task. This allows the lower priority
task to complete its work and unlock the mutex as quickly as
possible. Once the mutex has been unlocked, the lower priority task
sets its task priority to that of the highest priority task
that is waiting on a mutex it holds. When there is no higher
priority task waiting, the lower priority task sets its task priority
to the task’s original priority.

.. note::

   The priority of an executing task, locked by a mutex can
   be elevated repeatedly as more higher priority tasks wait on the
   mutex(es). Conversely, the priority of the task repeatedly lowers
   as mutex(es) release.

The microkernel also allows a task to repeatedly lock a mutex it
already locked. This ensures that the task can access the resource
at a point in its execution when the resource may or may not
already be locked. A mutex that is repeatedly locked must be unlocked
an equal number of times before the mutex releases the resource
completely.


Purpose
=======
Use mutexes to provide exclusive access to a resource,
such as a physical device.


Usage
=====

Defining a Mutex
----------------

Add an entry for the mutex in the project file using the
following syntax:

.. code-block:: console

   MUTEX %name

For example, the file :file:`projName.mdef` defines a single mutex as follows:

.. code-block:: console

   % MUTEX  NAME
   % ===============
     MUTEX  DEVICE_X



Example: Locking a Mutex with No Conditions
-------------------------------------------
This code waits indefinitely for the mutex to become available if the
mutex is in use.

.. code-block:: console

	task_mutex_lock_wait(XYZ);
	moveto(100,100);
	lineto(200,100);
	task_mutex_unlock(XYZ);


Example: Locking a Mutex with a Conditional Timeout
---------------------------------------------------
This code waits for a mutex to become available for a specified
time, and gives a warning if the mutex does not become available
in the specified amount of time.

.. code-block:: console

	if (task_mutex_lock_wait_timeout(XYZ, 100) == RC_OK)
		{
		moveto(100,100);
		lineto(200,100);
		task_mutex_unlock(XYZ);
		}
	else
		{
		printf("Cannot lock XYZ display\n");
		}



Example: Locking a Mutex with a No Blocking Condition
-----------------------------------------------------
This code gives an immediate warning when a mutex is in use.

.. code-block:: console

	if (task_mutex_lock(XYZ) == RC_OK);
		{
		do_something();
		task_mutex_unlock(XYZ); /* and unlock mutex*/
		}
	else
		{
		display_warning(); /* and do not unlock mutex*/
		}


APIs
====

The following Mutex APIs are provided by microkernel.h.

+------------------------------------------+-----------------------------------+
| Call                                     | Description                       |
+==========================================+===================================+
| :c:func:`task_mutex_lock()`              | Locks a mutex, and increments     |
|                                          | the lock count.                   |
+------------------------------------------+-----------------------------------+
| :c:func:`task_mutex_lock_wait()`         | Waits on a locked mutex until it  |
|                                          | is unlocked, then locks the mutex |
|                                          | and increments the lock count.    |
+------------------------------------------+-----------------------------------+
| :c:func:`task_mutex_lock_wait_timeout()` | Waits on a locked mutex for the   |
|                                          | period of time defined by the     |
|                                          | timeout parameter. If the mutex   |
|                                          | becomes available during that     |
|                                          | period, the function locks the    |
|                                          | mutex, and increments the lock    |
|                                          | count.  If the timeout expires,   |
|                                          | it returns RC_TIME.               |
+------------------------------------------+-----------------------------------+
| :c:func:`task_mutex_unlock()`            | Decrements a mutex lock count,    |
|                                          | and unlocks the mutex when the    |
|                                          | count reaches zero.               |
+------------------------------------------+-----------------------------------+
