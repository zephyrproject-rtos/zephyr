.. _mailboxes:

Mailboxes
*********

Definition
==========

A mailbox is defined in include :file:`/microkernel/mailbox.h`.
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

Usage
=====

Defining a mailbox
------------------

The following parameters must be defined:

   *name*
          This specifies a unique name for the mailbox.

Add an entry for a mailbox in the project .MDEF file using the
following syntax:

.. code-block:: console

   MAILBOX %name

For example, the file :file:`projName.mdef` defines a mailbox as follows:

.. code-block:: console

   % MAILBOX   NAME
   % =================
     MAILBOX   REQUEST_BOX


Example: Sending Variable-Sized Mailbox Messages
------------------------------------------------

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

Example: Receiving Variable-Sized Mailbox Messages
--------------------------------------------------

This code uses a mailbox to process variable-sized requests from any
producing task. The message "info" field is used to exchange information
about the maximum size buffer that each task can handle.

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
-----------------------------------------

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

Example: Receiving a Mailbox Message in 2 Stages
------------------------------------------------

This code uses a mailbox to receive data from a producing task only if
it meets certain criteria, thereby eliminating unneeded data copying.
The message "info" field supplied by the sender is used to classify the message.

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

           /* get message data for only some certain messages */
           if (is_message_type_ok(recv_msg.info)) {
               /* retrieve message data and discard message */
               recv_msg.rx_data = buffer;
               task_mbox_data_get(&recv_msg);

               /* process data in "buffer" */
               ...
           } else {
               /* ignore message data and discard message */
               recv_msg.size = 0;
               task_mbox_data_get(&recv_msg);
           }
       }
   }

Example: Sending an Asynchronous Mailbox Message
------------------------------------------------

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
           task_mbox_put_async(REQUEST_BOX, send_priority, &send_msg, MY_SEMA);
       }
   }

Example: Receiving an Asynchronous Mailbox Message
--------------------------------------------------

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

           /* get message data as a memory block and discard message */
           task_mbox_data_get_async_block_wait(&recv_msg, &recv_block, RXPOOL);

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
====

The following APIs for synchronous mailbox operations are provided
by microkernel.h.

+-----------------------------------------+-----------------------------------+
| Call                                    | Description                       |
+=========================================+===================================+
| :c:func:`task_mbox_put()`               | Puts message in a mailbox, or     |
|                                         | fails if a receiver isn't waiting.|
+-----------------------------------------+-----------------------------------+
| :c:func:`task_mbox_put_wait()`          | Puts message in a mailbox and     |
|                                         | waits until it is received.       |
+-----------------------------------------+-----------------------------------+
| :c:func:`task_mbox_put_wait_timeout()`  | Puts message in a mailbox and     |
|                                         | waits for a specified time period |
|                                         | for it to be received.            |
+-----------------------------------------+-----------------------------------+
| :c:func:`task_mbox_get()`               | Gets message from a mailbox, or   |
|                                         | fails if no message is available. |
+-----------------------------------------+-----------------------------------+
| :c:func:`task_mbox_get_wait()`          | Gets message from a mailbox, or   |
|                                         | waits until one is available.     |
+-----------------------------------------+-----------------------------------+
| :c:func:`task_mbox_get_wait_timeout()`  | Gets message from a mailbox, or   |
|                                         | waits for a specified time period |
|                                         | for one to become available.      |
+-----------------------------------------+-----------------------------------+
| :c:func:`task_mbox_data_get()`          | Finishes receiving message that   |
|                                         | was received without its data.    |
+-----------------------------------------+-----------------------------------+

The following APIs for asynchronous mailbox operations using memory pool blocks
are provided by microkernel.h.

+---------------------------------------------------------+-----------------------------------+
| Call                                                    | Description                       |
+=========================================================+===================================+
| :c:func:`task_mbox_put_async()`                         | Puts message in a mailbox, even   |
|                                                         | if a receiver isn't waiting.      |
+---------------------------------------------------------+-----------------------------------+
| :c:func:`task_mbox_data_get_async_block()`              | Finishes receiving message that   |
|                                                         | was received without its data, or |
|                                                         | fails if no block is available.   |
+---------------------------------------------------------+-----------------------------------+
| :c:func:`task_mbox_data_get_async_block_wait()`         | Finishes receiving message that   |
|                                                         | was received without its data, or |
|                                                         | waits until a block is available. |
+---------------------------------------------------------+-----------------------------------+
| :c:func:`task_mbox_data_get_async_block_wait_timeout()` | Finishes receiving message that   |
|                                                         | was received without its data, or |
|                                                         | waits for a specified time period |
|                                                         | for a block to become available.  |
+---------------------------------------------------------+-----------------------------------+