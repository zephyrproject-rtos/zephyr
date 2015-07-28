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

Usage
=====

Defining a pipe in MDEF file
----------------------------

The following parameters must be defined:

   *name*
          This specifies a unique name for the pipe.

   *buffer_size*
          This specifies the size (in bytes) of the pipe's internal buffer.
          If no internal buffer is to be used specify zero.

Add an entry for a pipe in the project .MDEF file using the
following syntax:

.. code-block:: console

   PIPE %name %buffer_size

For example, the file :file:`projName.mdef` defines a pipe as follows:

.. code-block:: console

   % PIPE   NAME          BUFFERSIZE
   % ===============================
     PIPE   DATA_PIPE        1024


Defining pipes in source code
-----------------------------

In addition to defining pipes in MDEF file, it is also possible to
define pipes inside code. The macro ``DEFINE_PIPE(...)`` can be
used for this purpose.

For example, the following code can be used to define a global pipe
``PRIV_PIPE``.

.. code-block:: c

   DEFINE_PIPE(PRIV_PIPE, size);

where the parameters are the same as pipes defined in MDEF file.
The task ``PRIV_PIPE`` can be used in the same style as those
defined in MDEF file.

It is possible to utilize this pipe in another source file, simply
add:

.. code-block:: c

   extern const kpipe_t PRIV_PIPE;

to that file. The pipe ``PRIV_PIPE`` can be then used there.


Example: Writing Fixed-Size Data Items to a Pipe
------------------------------------------------

This code uses a pipe to send a series of fixed-size data items
to a consuming task.

.. code-block:: c

   void producer_task(void)
   {
       struct item_type data_item;
       int amount_written;

       while (1) {
           /* generate a data item to send */
           data_item = ... ;

           /* write the entire data item to the pipe */
           task_pipe_put_wait(DATA_PIPE, &data_item, sizeof(data_item),
                              &amount_written, _ALL_N);

       }
   }

Example: Reading Fixed-Size Data Items from a Pipe
--------------------------------------------------

This code uses a pipe to receive a series of fixed-size data items
from a producing task. To improve performance, the consuming task
waits until 20 data items are available then reads them as a group,
rather than reading them individually.

.. code-block:: c

   void consumer_task(void)
   {
       struct item_type data_items[20];
       int amount_read;
       int i;

       while (1) {
           /* read 20 complete data items at once */
           task_pipe_get_wait(DATA_PIPE, &data_items, sizeof(data_items),
                              &amount_read, _ALL_N);

           /* process the data items one at a time */
           for (i = 0; i < 20; i++) {
               ... = data_items[i];
               ...
           }
       }
   }

Example: Reading a Stream of Data Bytes from a Pipe
---------------------------------------------------

This code uses a pipe to process a stream of data bytes from a
producing task. The pipe is read in a non-blocking manner to allow
the consuming task to perform other work when there are no
unprocessed data bytes in the pipe.

.. code-block:: c

   void consumer_task(void)
   {
       char data_area[20];
       int amount_read;
       int i;

       while (1) {
           /* consume any data bytes currently in the pipe */
           while (task_pipe_get(DATA_PIPE, &data_area, sizeof(data_area),
                                &amount_read, _1_TO_N) == RC_OK) {
               /* now have from 1 to 20 data bytes */
               for (i = 0; i < amount_read; i++) {
                   ... = data_area[i];
                   ...
               }
           }

           /* do other processing */
           ...
       }
   }


APIs
====

The following Pipe APIs are provided by :file:`microkernel.h`.

+----------------------------------------+------------------------------------+
| Call                                   | Description                        |
+========================================+====================================+
| :c:func:`task_pipe_put()`              | Writes data to a pipe, or fails &  |
|                                        | continues if unable to write data. |
+----------------------------------------+------------------------------------+
| :c:func:`task_pipe_put_wait()`         | Writes data to a pipe, or waits    |
|                                        | if unable to write data.           |
+----------------------------------------+------------------------------------+
| :c:func:`task_pipe_put_wait_timeout()` | Writes data to a pipe, or waits    |
|                                        | for a specified time period if     |
|                                        | unable to write data.              |
+----------------------------------------+------------------------------------+
| :c:func:`task_pipe_put_async()`        | Writes data to a pipe from a       |
|                                        | memory pool block.                 |
+----------------------------------------+------------------------------------+
| :c:func:`task_pipe_get()`              | Reads data from a pipe, or fails   |
|                                        | and continues if data isn't there. |
+----------------------------------------+------------------------------------+
| :c:func:`task_pipe_get_wait()`         | Reads data from a pipe, or waits   |
|                                        | for data if data isn't there.      |
+----------------------------------------+------------------------------------+
| :c:func:`task_pipe_get_wait_timeout()` | Reads data from a pipe, or waits   |
|                                        | for data for a specified time      |
|                                        | period if data isn't there.        |
+----------------------------------------+------------------------------------+
