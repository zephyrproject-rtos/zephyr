.. _microkernel_pipes:

Pipes
#####

Concepts
********

The microkernel's pipe object type is an implementation of a traditional
anonymous pipe.

A pipe allows a task to send a byte stream to another task. The pipe can be
configured with a ring buffer which holds data that has been sent
but not yet received; alternatively, the pipe may have no ring buffer.
Pipes can be used to transfer chunks of data in whole or in part, and either
synchronously or asynchronously.

Any number of pipes can be defined in a microkernel system. Each pipe has a
name that uniquely identifies it. In addition, a pipe defines the size of
its ring buffer; a size of zero defines a pipe with no ring buffer.

Sending Data
============

A task sends data to a pipe by specifying a pointer to the data bytes
to be sent. It also specifies both the number of data bytes available
and a *pipe option* that indicates the minimum number of data bytes
the pipe must accept. The following pipe option values are supported:

   :option:`_ALL_N`
      Specifies that all available data bytes must be accepted by the pipe;
      otherwise the send request fails or performs a partial send.
   :option:`_1_TO_N`
      Specifies that at least one data byte must be accepted by the pipe;
      otherwise the send request fails.
   :option:`_0_TO_N`
      Specifies that any number of data bytes, including zero, may be accepted
      by the pipe; the send request never fails.

The pipe accepts data bytes from the sending task if they can be delivered
by copying them directly to the receiving task. If the sending task is unable
to wait, or has waited as long as it can, the pipe can also accept data bytes
by copying them to its ring buffer for later delivery. The ring buffer is used
only when necessary to minimize copying of data bytes.

Upon completion of a send operation a return code is provided to indicate
if the send request was satisfied. The sending task can also read the *bytes
written* argument to determine how many data bytes were accepted by the pipe,
allowing it to deal with any unsent data bytes.

Data sent to a pipe that does not have a ring buffer is sent synchronously;
that is, when the send operation is complete the sending task knows that the
receiving task has received the data that was sent. Data sent to a pipe
that has a ring buffer is sent asynchronously; that is, when the send operation
is complete some or all of the data that was sent may still be in the pipe
waiting for the receiving task to receive it.

Incomplete Send Requests
------------------------

Although a pipe endeavors to accept all available data bytes when the
:option:`_ALL_N` pipe option is specified, it does not guarantee that the
data bytes will be accepted in an "all or nothing" manner. If the pipe
is able to accept at least one data byte it returns :option:`RC_INCOMPLETE`
to notify the sending task that its request was not fully satisfied; if
the pipe is unable to accept any data bytes it returns :option:`RC_FAIL`.

An example of a situation that can result in an incomplete send is a time
limited send request through an unbuffered pipe. If the receiving task
chooses to receive only a subset of the sender's data bytes, and then the
send operation times out before the receiving task attempts to receive the
remainder, an incomplete send occurs.

Sending using a Memory Pool Block
---------------------------------

A task that sends large chunks of data through a pipe may be able to improve
its efficiency by placing the data into a memory pool block and sending
the block. The pipe treats the memory block as a temporary addition to
its ring buffer, allowing it to immediately accept the data bytes without
copying them. Once all of the data bytes in the block have been delivered
to the receiving task the pipe automatically frees the block back to the
memory pool.

Data sent using a memory pool block is always sent asynchronously, even for
a pipe with no ring buffer of its own. Likewise, the pipe always accepts all
of the available data in the block---a partial transfer never occurs.

Receiving Data
==============

A task receives from a pipe by specifying a pointer to an area to receive
the data bytes that were sent. It also specifies both the desired number
of data bytes and a *pipe option* that indicates the minimum number of data
bytes the pipe must deliver. The following pipe option values are supported:

   :option:`_ALL_N`
      Specifies that all desired number of data bytes must be received;
      otherwise the receive request fails or performs a partial receive.
   :option:`_1_TO_N`
      Specifies that at least one data byte must be received; otherwise
      the receive request fails.
   :option:`_0_TO_N`
      Specifies that any number of data bytes (including zero) may be
      received; the receive request never fails.

The pipe delivers data bytes by copying them directly from the sending task
or from the pipe's ring buffer. Data bytes taken from the ring buffer are
delivered in a first in, first out manner.

When a pipe is unable to deliver the specified minimum number of data bytes
the receiving task may choose to wait until they can be delivered.

Upon completion of a receive operation a return code is provided to indicate
if the receive request was satisfied. The receiving task can also read the
*bytes read* argument to determine how many data bytes were delivered
by the pipe.

Incomplete Receive Requests
---------------------------

Although a pipe endeavors to deliver all desired data bytes when the
:option:`_ALL_N` pipe option is specified, it does not guarantee that the
data bytes will be delivered in an "all or nothing" manner. If the pipe
is able to deliver at least one data byte it returns :option:`RC_INCOMPLETE`
to notify the receiving task that its request was not fully satisfied; if
the pipe is unable to deliver any data bytes it returns :option:`RC_FAIL`.

An example of a situation that can result in an incomplete receive is a time
limited recieve request through an unbuffered pipe. If the sending task
sends fewer than the desired number of data bytes, and then the receive
operation times out before the sending task attempts to send the remainder,
an incomplete receive occurs.

Receiving using a Memory Pool Block
-----------------------------------

A task can achieve the effect of receiving data from a pipe into a memory pool
block by pre-allocating a block and then receiving the data into it.

Sharing a Pipe
==============

A pipe is typically used by a single sending task and a single receiving
task; however, it is possible for a pipe to be shared by multiple sending
tasks or multiple receiving tasks.

Care must be taken when a pipe is shared by multiple sending tasks to
ensure the data bytes they send do not become interleaved unexpectedly;
using the :option:`_ALL_N` pipe option helps to ensure that each data chunk is
transferred in a single operation. The same is true when multiple receiving
tasks are reading from the same pipe.


Purpose
*******

Use a pipe to transfer data when the receiving task needs the ability
to split or merge the data items generated by the sending task.


Usage
*****

Defining a Pipe
===============

The following parameters must be defined:

   *name*
          This specifies a unique name for the pipe.

   *buffer_size*
          This specifies the size in bytes of the pipe's ring buffer.
          If no ring buffer is to be used specify zero.


Public Pipe
-----------

Define the pipe in the application's .MDEF file using the following syntax:

.. code-block:: console

   PIPE name buffer_size

For example, the file :file:`projName.mdef` defines a pipe with a 1 KB ring
buffer as follows:

.. code-block:: console

   % PIPE   NAME          BUFFERSIZE
   % ===============================
     PIPE   DATA_PIPE        1024

A public pipe can be referenced by name from any source file that includes
the file :file:`zephyr.h`.


Private Pipe
------------

Define the pipe in a source file using the following syntax:

.. code-block:: c

   DEFINE_PIPE(name, size);

For example, the following code defines a private pipe named ``PRIV_PIPE``.

.. code-block:: c

   DEFINE_PIPE(PRIV_PIPE, 1024);

To utilize this pipe from a different source file use the following syntax:

.. code-block:: c

   extern const kpipe_t PRIV_PIPE;


Example: Writing Fixed-Size Data Items to a Pipe
================================================

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
==================================================

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
===================================================

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
****

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
| :c:func:`task_pipe_block_put()`        | Writes data to a pipe from a       |
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
