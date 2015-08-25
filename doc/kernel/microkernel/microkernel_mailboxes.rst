.. _microkernel_mailboxes:

Mailboxes
#########

Concepts
********

The microkernel's mailbox object type is an implementation of a traditional
message queue.

A mailbox allows tasks to exchange messages. A task that sends a message
is known as the *sending task*, while a task that receives the message
is known as the *receiving task*. Messages may not be sent or received
by fibers or ISRs, nor may a given message be received by more than one task
(i.e. point-to-multipoint messaging is not supported).

A mailbox has a queue of messages that have been sent, but not yet received.
The messages in the queue are sorted by priority, allowing a higher priority
message to be received before a lower priority message that was sent earlier.
Messages of equal priority are handled in a first in, first out manner.

Any number of mailboxes can be defined in a microkernel system. Each mailbox
has a name that uniquely identifies it. A mailbox does not limit the number
of messages it can queue, nor does it place limits on the size of the messages
it handles.

The content of a message is stored in an array of bytes, called the
*message data*. The size and format of the message data is application-defined,
and can vary from one message to the next. Message data may be stored
in a buffer provided by the task that sends or receives the message,
or in a memory pool block. The message data portion of a message is optional;
a message without any message data is called an *empty message*.

The lifecycle of a message is fairly simple. A message is created when it
is given to a mailbox by the sending task. The message is then owned
by the mailbox until it is given to a receiving task. The receiving task may
retrieve the message data when it receives the message from the mailbox,
or it may perform data retrieval during a second, subsequent mailbox operation.
Only when data retrieval has been performed is the message deleted
by the mailbox.

Messages can be exchanged non-anonymously or anonymously. A sending task
can specify the name of the task to which the message is being sent,
or it can specify that any task may receive the message. Likewise, a receiving
task can specify the name of the task it wishes to receive a message from,
or it can specify that it is willing to receive a message from any task.
A message is exchanged only if the requirements of both the sending task and
receiving task can both be satisfied; such tasks are said to be *compatible*.

For example, if task A sends a message to task B it will be received by task B
if the latter tries to receive a message from task A (or from any task), but
not if task B tries to receive a message from task C. The message can never
be received by task C, even if it is trying to receive a message from task A
(or from any task).

Messages can be exchanged sychronously or asynchronously. In a synchronous
exchange the sending task blocks until the message has been fully processed
by the receiving task. In an asynchronous exchange the sending task does not
wait until the message has been received by another task before continuing,
thereby allowing the task to do other work (such as gathering data that will be
used in the next message) before the message is given to a receiving task and
fully processed. The technique used for a given message exchange is determined
by the sending task.

The synchronous exchange technique provides an inherent form of flow control,
preventing a sending task from generating messages faster than they can be
consumed by receiving tasks. The asynchronous exchange technique provides an
optional form of flow control, which allows a sending task to determine
if a previously sent message still exists before sending a subsequent message.

Message Descriptor
==================

A message descriptor is a data structure that specifies where a message's data
is located and how the message is to be handled by the mailbox. Both the
sending task and the receiving task pass a message descriptor to the mailbox
when accessing a mailbox. The mailbox uses both message descriptors to perform
a message exchange between compatible sending and receiving tasks. The mailbox
also updates some fields of the descriptors during the exchange to allow both
tasks to know what occurred.

A message descriptor is a structure of type :c:type:`struct k_msg`. The fields
listed below are available for application use; all other fields are for
kernel use only.

   :c:option:`info`
      A 32 bit value that is exchanged by the message sender and receiver,
      and whose meaning is defined by the application. This exchange is
      bi-directional, allowing the sender to pass a value to the receiver
      during any message exchange, and allowing the receiver to pass a value
      to the sender during a synchronous message exchange.

   :c:option:`size`
      The message data size, in bytes. Set it to zero when sending an empty
      message, or when discarding the message data of a received message.
      The mailbox updates this field with the actual number of data bytes
      exchanged once the message is received.

   :c:option:`tx_data`
      A pointer to the sending task's message data buffer. Set it to
      :c:macro:`NULL` when sending a memory pool block, or when sending
      an empty message. (Not used when receiving a message.)

   :c:option:`tx_block`
      The descriptor for the memory pool block containing the sending task's
      message data. (Not used when sending a message data buffer,
      or when sending an empty message. Not used when receiving a message.)

   :c:option:`rx_data`
      A pointer to the receiving task's message data buffer. Set it to
      :c:macro:`NULL` when the message's data is not wanted, or when it will be
      retrieved by a subsequent mailbox operation. (Not used when sending
      a message.)

   :c:option:`tx_task`
      The name of the sending task. Set it to :c:macro:`ANYTASK` to receive
      a message sent by any task. The mailbox updates this field with the
      actual sender's name once the message is received. (Not used when
      sending a message.)

   :c:option:`rx_task`
      The name of the receiving task. Set it to :c:macro:`ANYTASK` to allow
      any task to receive the message. The mailbox updates this field with
      the actual receiver's name once the message is received, but only if
      the message is sent synchronously. (Not used when receiving a message.)

Sending a Message
=================

A task sends a message by first creating the message data to be sent (if any).
The data may be placed in a message buffer---such as an array or structure
variable---whose contents are copied to an area supplied by the receiving task
during the message exchange. Alternatively, the data may be placed in a block
allocated from a memory pool, which is handed off to the receiving task
during the exchange. A message buffer is typically used when the amount of
data involved is small, and the cost of copying the data is less than the cost
of allocating and freeing a memory pool block. A memory pool block *must*
be used when a non-empty message is sent asynchronously.

Next, the task creates a message descriptor that characterizes the message
to be sent, as described in the previous section.

Finally, the task calls one of the mailbox send APIs to initiate the message
exchange. The message is immediately given to a compatible receiving task,
if one is currently waiting for a message. Otherwise, the message is added
to the mailbox's queue of messages, according to the priority specified by
the sending task. Typically, a sending task sets the message priority to
its own task priority level, allowing messages sent by higher priority tasks
to take precedence over those sent by lower priority tasks.

For a synchronous send operation the operation normally completes when a
receiving task has both received the message and retrieved the message data.
If the message is not received before the waiting period specified by the
sending task is reached, the message is removed from the mailbox's queue
and the sending task continues processing. When a send operation completes
successfully the sending task can examine the message descriptor to determine
which task received the message and how much data was exchanged, as well as
the application-defined info value supplied by the receiving task.

.. note::
   A synchronous send operation may block the sending task indefinitely---even
   when the task specifies a maximum waiting period---since the waiting period
   only limits how long the mailbox waits before the message is received
   by another task. Once a message is received there is no limit to the time
   the receiving task may take to retrieve the message data and unblock
   the sending task.

For an asynchronous send operation the operation always completes immediately.
This allows the sending task to continue processing regardless of whether the
message is immediately given to a receiving task or is queued by the mailbox.
The sending task may optionally specify a semaphore that the mailbox gives
when the message is deleted by the mailbox (i.e. when the message has been
received and its data retrieved by a receiving task). The use of a semaphore
allows the sending task to easily implement a flow control mechanism that
ensures that the mailbox holds no more than an application-specified number
of messages from a sending task (or set of sending tasks) at any point in time.

Receiving a Message
===================

A task receives a message by first creating a message descriptor that
characterizes the message it wants to receive. It then calls one of the
mailbox receive APIs. The mailbox searches its queue of messages
and takes the first one it finds that satisfies both the sending and
receiving tasks' message descriptor criteria. If no compatible message
exists, the receiving task may choose to wait for one to be sent. If no
compatible message appears before the waiting period specified
by the receiving task is reached, the receive operation fails and
the receiving task continues processing. Once a receive operation completes
successfully the receiving task can examine the message descriptor
to determine which task sent the message, how much data was exchanged,
and the application-defined info value supplied by the sending task.

The receiving task controls both the amount of data it retrieves from an
incoming message and where the data ends up. The task may choose to take
all of the data in the message, to take only the initial part of the data,
or to take no data at all. Similarly, the task may choose to have the data
copied into a buffer area of its choice or to have it placed in a memory
pool block. A message buffer is typically used when the amount of data
involved is small, and the cost of copying the data is less than the cost
of allocating and freeing a memory pool block.

The following sections outline various approaches a receiving task may use
when retrieving message data.

Retrieving Data Immediately into a Buffer
-----------------------------------------

The most straightforward way for a task to retrieve message data is to
specify a buffer when the message is received. The task indicates
both the location of the buffer (which must not be :c:macro:`NULL`)
and its size (which must be greater than zero).

The mailbox copies the message's data to the buffer as part of the
receive operation. If the buffer is not big enough to contain all of the
message's data, any uncopied data is lost. If the message is not big enough
to fill all of the buffer with data, the unused portion of the buffer is
left unchanged. In all cases the mailbox updates the receiving task's
message descriptor to indicate how many data bytes were copied (if any).

The immediate data retrieval technique is best suited for applications involving
small messages where the maximum size of a message is known in advance.

.. note::
   This technique can be used when the message data is actually located
   in a memory pool block supplied by the sending task. The mailbox copies
   the data into the buffer specified by the receiving task, then automatically
   frees the block back to its memory pool. This allows a receiving task
   to retrieve message data without having to know whether the data
   was sent using a buffer or a block.

Retrieving Data Subsequently into a Buffer
------------------------------------------

A receiving task may choose to retrieve no message data at the time the message
is received, so that it can retrieve the data into a buffer at a later time.
The task does this by specifying a buffer location of :c:macro:`NULL`
and a size indicating the maximum amount of data it is willing to retrieve
later (which must be greater than or equal to zero).

The mailbox does not copy any message data as part of the receive operation.
However, the mailbox still updates the receiving task's message descriptor
to indicate how many data bytes are available for retrieval.

The receiving task must then respond as follows:

* If the message descriptor size is zero, then either the received message is
  an empty message or the receiving task did not want to receive any
  message data. The receiving task does not need to take any further action
  since the mailbox has already completed data retrieval and deleted the
  message.

* If the message descriptor size is non-zero and the receiving task still
  wants to retrieve the data, the task must supply a buffer large enough
  to hold the data. The task first sets the message descriptor's
  :c:option:`rx_data` field to the address of the buffer, then calls
  :c:func:`task_mbox_data_get()`. This instructs the mailbox to copy the data
  and delete the message.

* If the message descriptor size is non-zero and the receiving task does *not*
  want to retrieve the data, the task sets the message descriptor's
  :c:option:`size` field to zero and calls :c:func:`task_mbox_data_get()`.
  This instructs the mailbox to delete the message without copying the data.

The subsequent data retrieval technique is suitable for applications where
immediate retrieval of message data is undesirable. For example, it can be
used when memory limitations make it impractical for the receiving task to
always supply a buffer capable of holding the largest possible incoming message.

.. note::
   This technique can be used when the message data is actually located
   in a memory pool block supplied by the sending task. The mailbox copies
   the data into the buffer specified by the receiving task, then automatically
   frees the block back to its memory pool. This allows a receiving task
   to retrieve message data without having to know whether the data
   was sent using a buffer or a block.

Retrieving Data Subsequently into a Block
-----------------------------------------

A receiving task may choose to retrieve message data into a memory pool block,
rather than a buffer area of its choice. This is done in much the same way
as retrieving data subsequently into a buffer---the receiving task first
receives the message without its data, then retrieves the data by calling
:c:func:`task_mbox_data_block_get()`. The latter call fills in the block
descriptor supplied by the receiving task, allowing the task to access the data.
This call also causes the mailbox to delete the received message, since
data retrieval has been completed. The receiving task is then responsible
for freeing the block back to the memory pool when the data is no longer needed.

This technique is best suited for applications where the message data has
been sent using a memory pool block, either because a large amount of data
is involved or because the message was sent asynchronously.

.. note::
   This technique can be used when the message data is located in a buffer
   supplied by the sending task. The mailbox automatically allocates a memory
   pool block and copies the message data into it. However, this is much less
   efficient than simply retrieving the data into a buffer supplied by the
   receiving task. In addition, the receiving task must be designed to handle
   cases where the data retrieval operation fails because the mailbox cannot
   allocate a suitable block from the memory pool. If such cases are possible,
   the receiving task can call :c:func:`task_mbox_data_block_get_wait()` or
   :c:func:`task_mbox_data_block_get_wait_timeout()` to permit the task to wait
   until a suitable block can be allocated. Alternatively, the task can use
   :c:func:`task_mbox_data_get()` to inform the mailbox that it no longer wishes
   to receive the data at all, allowing the mailbox to release the message.

Purpose
*******

Use a mailbox to transfer data items between tasks whenever the capabilities
of a FIFO are insufficient.


Usage
*****

Defining a Mailbox
==================

The following parameters must be defined:

   *name*
          This specifies a unique name for the mailbox.

Public Mailbox
--------------

Define the mailbox in the application's MDEF using the following syntax:

.. code-block:: console

   MAILBOX name

For example, the file :file:`projName.mdef` defines a mailbox as follows:

.. code-block:: console

   % MAILBOX   NAME
   % ==========================
     MAILBOX   REQUEST_BOX

A public mailbox can be referenced by name from any source file that
includes the file :file:`zephyr.h`.

Private Mailbox
---------------

Define the mailbox in a source file using the following syntax:

.. code-block:: c

   DEFINE_MAILBOX(name);

For example, the following code defines a private mailbox named ``PRIV_MBX``.

.. code-block:: c

   DEFINE_MAILBOX(PRIV_MBX);

The mailbox ``PRIV_MBX`` can be used in the same style as those
defined in the MDEF.

To utilize this mailbox from a different source file use the following syntax:

.. code-block:: c

   extern const kmbox_t PRIV_MBX;

Example: Sending a Variable-Sized Mailbox Message
=================================================

This code uses a mailbox to synchronously pass variable-sized requests
from a producing task to any consuming task that wants it. The message
"info" field is used to exchange information about the maximum size buffer
that each task can handle.

.. code-block:: c

   void producer_task(void)
   {
       char buffer[100];
       int buffer_bytes_used;

       struct k_msg send_msg;
       k_priority_t send_priority = task_priority_get();

       while (1) {

           /* generate data to send */
           ...
           buffer_bytes_used = ... ;
           memcpy(buffer, source, buffer_bytes_used);

           /* prepare to send message */
           send_msg.info = buffer_bytes_used;
           send_msg.size = buffer_bytes_used;
           send_msg.tx_data = buffer;
           send_msg.rx_task = ANYTASK;

           /* send message and wait until a consumer receives it */
           task_mbox_put_wait(REQUEST_BOX, send_priority, &send_msg);

           /* info, size, and rx_task fields have been updated */

           /* verify that message data was fully received */
           if (send_msg.size < buffer_bytes_used) {
               printf("some message data dropped during transfer!");
               printf("receiver only had room for %d bytes", send_msg.info);
           }
       }
   }

Example: Receiving a Variable-Sized Mailbox Message
===================================================

This code uses a mailbox to process variable-sized requests from any
producing task, using the immediate data retrieval technique. The message
"info" field is used to exchange information about the maximum size buffer
that each task can handle.

.. code-block:: c

   void consumer_task(void)
   {
       struct k_msg recv_msg;
       char buffer[100];

       int i;
       int total;

       while (1) {
           /* prepare to receive message */
           recv_msg.info = 100;
           recv_msg.size = 100;
           recv_msg.rx_data = buffer;
           recv_msg.rx_task = ANYTASK;

           /* get a data item, waiting as long as needed */
           task_mbox_get_wait(REQUEST_BOX, &recv_msg);

           /* info, size, and tx_task fields have been updated */

           /* verify that message data was fully received */
           if (recv_msg.info != recv_msg.size) {
               printf("some message data dropped during transfer!");
               printf("sender tried to send %d bytes", recv_msg.info);
           }

           /* compute sum of all message bytes (from 0 to 100 of them) */
           total = 0;
           for (i = 0; i < recv_msg.size; i++) {
               total += buffer[i];
           }
       }
   }

Example: Sending an Empty Mailbox Message
=========================================

This code uses a mailbox to synchronously pass 4 byte random values
to any consuming task that wants one. The message "info" field is
large enough to carry the information being exchanged, so the data buffer
portion of the message isn't used.

.. code-block:: c

   void producer_task(void)
   {
       struct k_msg send_msg;
       k_priority_t send_priority = task_priority_get();

       while (1) {

           /* generate random value to send */
           uint32_t random_value = sys_rand32_get();

           /* prepare to send empty message */
           send_msg.info = random_value;
           send_msg.size = 0;
           send_msg.tx_data = NULL;
           send_msg.rx_task = ANYTASK;

           /* send message and wait until a consumer receives it */
           task_mbox_put_wait(REQUEST_BOX, send_priority, &send_msg);

           /* no need to examine the receiver's "info" value */
       }
   }

Example: Deferring the Retrieval of Message Data
================================================

This code uses a mailbox's subsequent data retrieval mechanism to get message
data from a producing task only if the message meets certain criteria,
thereby eliminating unneeded data copying. The message "info" field supplied
by the sender is used to classify the message.

.. code-block:: c

   void consumer_task(void)
   {
       struct k_msg recv_msg;
       char buffer[10000];

       while (1) {
           /* prepare to receive message */
           recv_msg.size = 10000;
           recv_msg.rx_data = NULL;
           recv_msg.rx_task = ANYTASK;

           /* get message, but not its data */
           task_mbox_get_wait(REQUEST_BOX, &recv_msg);

           /* get message data for only certain types of messages */
           if (is_message_type_ok(recv_msg.info)) {
               /* retrieve message data and delete the message */
               recv_msg.rx_data = buffer;
               task_mbox_data_get(&recv_msg);

               /* process data in "buffer" */
               ...
           } else {
               /* ignore message data and delete the message */
               recv_msg.size = 0;
               task_mbox_data_get(&recv_msg);
           }
       }
   }

Example: Sending an Asynchronous Mailbox Message
================================================

This code uses a mailbox to send asynchronous messages using memory blocks
obtained from TXPOOL, thereby eliminating unneeded data copying when exchanging
large messages. The optional semaphore capability is used to hold off
the sending of a new message until the previous message has been consumed,
so that a backlog of messages doesn't build up if the consuming task is unable
to keep up.

.. code-block:: c

   void producer_task(void)
   {
       struct k_msg send_msg;
       kpriority_t send_priority = task_priority_get();

       volatile char *hw_buffer;

       /* indicate that all previous messages have been processed */
       task_sem_give(MY_SEMA);

       while (1) {
           /* allocate memory block that will hold message data */
           task_mem_pool_alloc_wait(&send_msg.tx_block, TXPOOL, 4096);

           /* keep saving hardware-generated data in the memory block      */
           /* until the previous message has been received by the consumer */
           do {
               memcpy(send_msg.tx_block.pointer_to_data, hw_buffer, 4096);
           } while (task_sem_take(MY_SEMA) != RC_OK);

           /* finish preparing to send message */
           send_msg.size = 4096;
           send_msg.rx_task = ANYTASK;

           /* send message containing most current data and loop around */
           task_mbox_block_put(REQUEST_BOX, send_priority, &send_msg, MY_SEMA);
       }
   }

Example: Receiving an Asynchronous Mailbox Message
==================================================

This code uses a mailbox to receive messages sent asynchronously using a
memory block, thereby eliminating unneeded data copying when processing
a large message.

.. code-block:: c

   void consumer_task(void)
   {
       struct k_msg recv_msg;
       struct k_block recv_block;

       int total;
       char *data_ptr;
       int i;

       while (1) {
           /* prepare to receive message */
           recv_msg.size = 10000;
           recv_msg.rx_data = NULL;
           recv_msg.rx_task = ANYTASK;

           /* get message, but not its data */
           task_mbox_get_wait(REQUEST_BOX, &recv_msg);

           /* get memory block holding data and delete the message */
           task_mbox_data_block_get_wait(&recv_msg, &recv_block, RXPOOL);

           /* compute sum of all message bytes in memory block */
           total = 0;
           data_ptr = (char *)(recv_block.pointer_to_data);
           for (i = 0; i < recv_msg.size; i++) {
               total += data_ptr++;
           }

           /* release memory block containing data */
           task_mem_pool_free(&recv_block);
       }
   }

.. note::
   An incoming message that was sent synchronously is also processed correctly
   by this algorithm, since the mailbox automatically creates a memory block
   containing the message data using RXPOOL. However, the performance benefit
   of using the asynchronous approach is lost.

APIs
****

The following APIs for mailbox operations are provided by the kernel:

:c:func:`task_mbox_put()`
   Sends synchrnonous message to a receiving task, with no waiting.

:c:func:`task_mbox_put_wait()`
   Sends synchrnonous message to a receiving task, with unlimited waiting.

:c:func:`task_mbox_put_wait_timeout()`
   Sends synchrnonous message to a receiving task, with time limited waiting.

:c:func:`task_mbox_block_put()`
   Sends asynchrnonous message to a receiving task, or to a mailbox queue.

:c:func:`task_mbox_get()`
   Gets message from a mailbox, with no waiting.

:c:func:`task_mbox_get_wait()`
   Gets message from a mailbox, with unlimited waiting.

:c:func:`task_mbox_get_wait_timeout()`
   Gets message from a mailbox, with time limited waiting.

:c:func:`task_mbox_data_get()`
   Retrieves message data into a buffer.

:c:func:`task_mbox_data_block_get()`
   Retrieves message data into a block, with no waiting.

:c:func:`task_mbox_data_block_get_wait()`
   Retrieves message data into a block, with unlimited waiting.

:c:func:`task_mbox_data_block_get_wait_timeout()`
   Retrieves message data into a block, with time limited waiting.
