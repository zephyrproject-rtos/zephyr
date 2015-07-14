.. _microkernel_fifos:

Microkernel FIFOs
*****************

Definition
==========

The FIFO is defined in :file:`include/microkernel/fifo.h` as a simple
first-in, first-out queue that handle small amounts of fixed size data.
FIFO objects have a buffer that stores a number of data transmits, and are the
most
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

+---------------------------------------+-----------------------------------+
| Call                                  | Description                       |
+=======================================+===================================+
| :c:func:`task_fifo_put()`             | Put data on a FIFO, and fail      |
|                                       | if the FIFO is full.              |
+---------------------------------------+-----------------------------------+
| :c:func:`task_fifo_put_wait()`        | Put data on a FIFO, waiting       |
|                                       | for room in the FIFO.             |
+---------------------------------------+-----------------------------------+
| :c:func:`task_fifo_put_wait_timeout()` | Put data on a FIFO, waiting      |
|                                        | for room in the FIFO, or a time  |
|                                        | out.                             |
+---------------------------------------+-----------------------------------+
| :c:func:`task_fifo_get()`             | Get data off a FIFO,              |
|                                       | returning immediately if no data  |
|                                       | is available.                     |
+---------------------------------------+-----------------------------------+
| :c:func:`task_fifo_get_wait()`        | Get data off a FIFO,              |
|                                       | waiting until data is available.  |
+---------------------------------------+-----------------------------------+
| :c:func:`task_fifo_get_wait_timeout()` | Get data off a FIFO, waiting     |
|                                        | until data is available, or a    |
|                                        | time out.                        |
+---------------------------------------+-----------------------------------+
| :c:func:`task_fifo_purge()`           | Empty the FIFO buffer, and signal |
|                                       | any waiting receivers with an     |
|                                       | error.                            |
+---------------------------------------+-----------------------------------+
| :c:func:`task_fifo_size_get()`        | Read the number of filled entries |
|                                       | in a FIFO.                        |
+---------------------------------------+-----------------------------------+
