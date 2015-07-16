.. _pipes:

Pipes
*****

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

+----------------------------------------+------------------------------------+
| Call                                   | Description                        |
+========================================+====================================+
| :c:func:`task_pipe_put()`              | Put data on a pipe                 |
+----------------------------------------+------------------------------------+
| :c:func:`task_pipe_put_wait()`         | Put data on a pipe with a delay.   |
+----------------------------------------+------------------------------------+
| :c:func:`task_pipe_put_wait_timeout()` | Put data on a pipe with a timed    |
|                                        | delay.                             |
+----------------------------------------+------------------------------------+
| :c:func:`task_pipe_get()`              | Get data off a pipe.               |
+----------------------------------------+------------------------------------+
| :c:func:`task_pipe_get_wait()`         | Get data off a pipe with a delay.  |
+----------------------------------------+------------------------------------+
| :c:func:`task_pipe_get_wait_timeout()` | Get data off a pipe with a timed   |
|                                        | delay.                             |
+----------------------------------------+------------------------------------+
| :c:func:`task_pipe_put_async()`        | Put data on a pipe asynchronously. |
+----------------------------------------+------------------------------------+
