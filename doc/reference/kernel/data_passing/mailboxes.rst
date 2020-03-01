.. _mailboxes_v2:

Mailboxes
#########

A :dfn:`mailbox` is a kernel object that provides enhanced message queue
capabilities that go beyond the capabilities of a message queue object.
A mailbox allows threads to send and receive messages of any size
synchronously or asynchronously.

.. contents::
    :local:
    :depth: 2

Concepts
********

Any number of mailboxes can be defined. Each mailbox is referenced
by its memory address.

A mailbox has the following key properties:

* A **send queue** of messages that have been sent but not yet received.

* A **receive queue** of threads that are waiting to receive a message.

A mailbox must be initialized before it can be used. This sets both of its
queues to empty.

A mailbox allows threads, but not ISRs, to exchange messages.
A thread that sends a message is known as the **sending thread**,
while a thread that receives the message is known as the **receiving thread**.
Each message may be received by only one thread (i.e. point-to-multipoint and
broadcast messaging is not supported).

Messages exchanged using a mailbox are handled non-anonymously,
allowing both threads participating in an exchange to know
(and even specify) the identity of the other thread.

Message Format
==============

A **message descriptor** is a data structure that specifies where a message's
data is located, and how the message is to be handled by the mailbox.
Both the sending thread and the receiving thread supply a message descriptor
when accessing a mailbox. The mailbox uses the message descriptors to perform
a message exchange between compatible sending and receiving threads.
The mailbox also updates certain message descriptor fields during the exchange,
allowing both threads to know what has occurred.

A mailbox message contains zero or more bytes of **message data**.
The size and format of the message data is application-defined, and can vary
from one message to the next. There are two forms of message data:

* A **message buffer** is an area of memory provided by the thread
  that sends or receives the message. An array or structure variable
  can often be used for this purpose.

* A **message block** is an area of memory allocated from a memory pool.

A message may *not* have both a message buffer and a message block.
A message that has neither form of message data is called an **empty message**.

.. note::
    A message whose message buffer or memory block exists, but contains
    zero bytes of actual data, is *not* an empty message.

Message Lifecycle
=================

The life cycle of a message is straightforward. A message is created when
it is given to a mailbox by the sending thread. The message is then owned
by the mailbox until it is given to a receiving thread. The receiving thread
may retrieve the message data when it receives the message from the mailbox,
or it may perform data retrieval during a second, subsequent mailbox operation.
Only when data retrieval has occurred is the message deleted by the mailbox.

Thread Compatibility
====================

A sending thread can specify the address of the thread to which the message
is sent, or send it to any thread by specifying :c:macro:`K_ANY`.
Likewise, a receiving thread can specify the address of the thread from which
it wishes to receive a message, or it can receive a message from any thread
by specifying :c:macro:`K_ANY`.
A message is exchanged only when the requirements of both the sending thread
and receiving thread are satisfied; such threads are said to be **compatible**.

For example, if thread A sends a message to thread B (and only thread B)
it will be received by thread B if thread B tries to receive a message
from thread A or if thread B tries to receive from any thread.
The exchange will not occur if thread B tries to receive a message
from thread C. The message can never be received by thread C,
even if it tries to receive a message from thread A (or from any thread).

Message Flow Control
====================

Mailbox messages can be exchanged **synchronously** or **asynchronously**.
In a synchronous exchange, the sending thread blocks until the message
has been fully processed by the receiving thread. In an asynchronous exchange,
the sending thread does not wait until the message has been received
by another thread before continuing; this allows the sending thread to do
other work (such as gather data that will be used in the next message)
*before* the message is given to a receiving thread and fully processed.
The technique used for a given message exchange is determined
by the sending thread.

The synchronous exchange technique provides an implicit form of flow control,
preventing a sending thread from generating messages faster than they can be
consumed by receiving threads. The asynchronous exchange technique provides an
explicit form of flow control, which allows a sending thread to determine
if a previously sent message still exists before sending a subsequent message.

Implementation
**************

Defining a Mailbox
==================

A mailbox is defined using a variable of type :c:type:`struct k_mbox`.
It must then be initialized by calling :cpp:func:`k_mbox_init()`.

The following code defines and initializes an empty mailbox.

.. code-block:: c

    struct k_mbox my_mailbox;

    k_mbox_init(&my_mailbox);

Alternatively, a mailbox can be defined and initialized at compile time
by calling :c:macro:`K_MBOX_DEFINE`.

The following code has the same effect as the code segment above.

.. code-block:: c

    K_MBOX_DEFINE(my_mailbox);

Message Descriptors
===================

A message descriptor is a structure of type :c:type:`struct k_mbox_msg`.
Only the fields listed below should be used; any other fields are for
internal mailbox use only.

*info*
    A 32-bit value that is exchanged by the message sender and receiver,
    and whose meaning is defined by the application. This exchange is
    bi-directional, allowing the sender to pass a value to the receiver
    during any message exchange, and allowing the receiver to pass a value
    to the sender during a synchronous message exchange.

*size*
    The message data size, in bytes. Set it to zero when sending an empty
    message, or when sending a message buffer or message block with no
    actual data. When receiving a message, set it to the maximum amount
    of data desired, or to zero if the message data is not wanted.
    The mailbox updates this field with the actual number of data bytes
    exchanged once the message is received.

*tx_data*
    A pointer to the sending thread's message buffer. Set it to :c:macro:`NULL`
    when sending a memory block, or when sending an empty message.
    Leave this field uninitialized when receiving a message.

*tx_block*
    The descriptor for the sending thread's memory block. Set tx_block.data
    to :c:macro:`NULL` when sending an empty message. Leave this field
    uninitialized when sending a message buffer, or when receiving a message.

*tx_target_thread*
    The address of the desired receiving thread. Set it to :c:macro:`K_ANY`
    to allow any thread to receive the message. Leave this field uninitialized
    when receiving a message. The mailbox updates this field with
    the actual receiver's address once the message is received.

*rx_source_thread*
    The address of the desired sending thread. Set it to :c:macro:`K_ANY`
    to receive a message sent by any thread. Leave this field uninitialized
    when sending a message. The mailbox updates this field
    with the actual sender's address when the message is put into
    the mailbox.

Sending a Message
=================

A thread sends a message by first creating its message data, if any.
A message buffer is typically used when the data volume is small,
and the cost of copying the data is less than the cost of allocating
and freeing a message block.

Next, the sending thread creates a message descriptor that characterizes
the message to be sent, as described in the previous section.

Finally, the sending thread calls a mailbox send API to initiate the
message exchange. The message is immediately given to a compatible receiving
thread, if one is currently waiting. Otherwise, the message is added
to the mailbox's send queue.

Any number of messages may exist simultaneously on a send queue.
The messages in the send queue are sorted according to the priority
of the sending thread. Messages of equal priority are sorted so that
the oldest message can be received first.

For a synchronous send operation, the operation normally completes when a
receiving thread has both received the message and retrieved the message data.
If the message is not received before the waiting period specified by the
sending thread is reached, the message is removed from the mailbox's send queue
and the send operation fails. When a send operation completes successfully
the sending thread can examine the message descriptor to determine
which thread received the message, how much data was exchanged,
and the application-defined info value supplied by the receiving thread.

.. note::
   A synchronous send operation may block the sending thread indefinitely,
   even when the thread specifies a maximum waiting period.
   The waiting period only limits how long the mailbox waits
   before the message is received by another thread. Once a message is received
   there is *no* limit to the time the receiving thread may take to retrieve
   the message data and unblock the sending thread.

For an asynchronous send operation, the operation always completes immediately.
This allows the sending thread to continue processing regardless of whether the
message is given to a receiving thread immediately or added to the send queue.
The sending thread may optionally specify a semaphore that the mailbox gives
when the message is deleted by the mailbox, for example, when the message
has been received and its data retrieved by a receiving thread.
The use of a semaphore allows the sending thread to easily implement
a flow control mechanism that ensures that the mailbox holds no more than
an application-specified number of messages from a sending thread
(or set of sending threads) at any point in time.

.. note::
   A thread that sends a message asynchronously has no way to determine
   which thread received the message, how much data was exchanged, or the
   application-defined info value supplied by the receiving thread.

Sending an Empty Message
------------------------

This code uses a mailbox to synchronously pass 4 byte random values
to any consuming thread that wants one. The message "info" field is
large enough to carry the information being exchanged, so the data
portion of the message isn't used.

.. code-block:: c

    void producer_thread(void)
    {
        struct k_mbox_msg send_msg;

        while (1) {

            /* generate random value to send */
            u32_t random_value = sys_rand32_get();

            /* prepare to send empty message */
            send_msg.info = random_value;
            send_msg.size = 0;
            send_msg.tx_data = NULL;
            send_msg.tx_block.data = NULL;
            send_msg.tx_target_thread = K_ANY;

            /* send message and wait until a consumer receives it */
            k_mbox_put(&my_mailbox, &send_msg, K_FOREVER);
        }
    }

Sending Data Using a Message Buffer
-----------------------------------

This code uses a mailbox to synchronously pass variable-sized requests
from a producing thread to any consuming thread that wants it.
The message "info" field is used to exchange information about
the maximum size message buffer that each thread can handle.

.. code-block:: c

    void producer_thread(void)
    {
        char buffer[100];
        int buffer_bytes_used;

        struct k_mbox_msg send_msg;

        while (1) {

            /* generate data to send */
            ...
            buffer_bytes_used = ... ;
            memcpy(buffer, source, buffer_bytes_used);

            /* prepare to send message */
            send_msg.info = buffer_bytes_used;
            send_msg.size = buffer_bytes_used;
            send_msg.tx_data = buffer;
            send_msg.tx_block.data = NULL;
            send_msg.tx_target_thread = K_ANY;

            /* send message and wait until a consumer receives it */
            k_mbox_put(&my_mailbox, &send_msg, K_FOREVER);

            /* info, size, and tx_target_thread fields have been updated */

            /* verify that message data was fully received */
            if (send_msg.size < buffer_bytes_used) {
                printf("some message data dropped during transfer!");
                printf("receiver only had room for %d bytes", send_msg.info);
            }
        }
    }

Sending Data Using a Message Block
----------------------------------

This code uses a mailbox to send asynchronous messages. A semaphore is used
to hold off the sending of a new message until the previous message
has been consumed, so that a backlog of messages doesn't build up
when the consuming thread is unable to keep up.

The message data is stored in a memory block obtained from a memory pool,
thereby eliminating unneeded data copying when exchanging large messages.
The memory pool contains only two blocks: one block gets filled with
data while the previously sent block is being processed

.. code-block:: c

    /* define a semaphore, indicating that no message has been sent */
    K_SEM_DEFINE(my_sem, 1, 1);

    /* define a memory pool containing 2 blocks of 4096 bytes */
    K_MEM_POOL_DEFINE(my_pool, 4096, 4096, 2, 4);

    void producer_thread(void)
    {
        struct k_mbox_msg send_msg;

        volatile char *hw_buffer;

        while (1) {
            /* allocate a memory block to hold the message data */
            k_mem_pool_alloc(&my_pool, &send_msg.tx_block, 4096, K_FOREVER);

            /* keep overwriting the hardware-generated data in the block    */
            /* until the previous message has been received by the consumer */
            do {
                memcpy(send_msg.tx_block.data, hw_buffer, 4096);
            } while (k_sem_take(&my_sem, K_NO_WAIT) != 0);

            /* finish preparing to send message */
            send_msg.size = 4096;
            send_msg.tx_target_thread = K_ANY;

            /* send message containing most current data and loop around */
            k_mbox_async_put(&my_mailbox, &send_msg, &my_sem);
        }
    }

Receiving a Message
===================

A thread receives a message by first creating a message descriptor that
characterizes the message it wants to receive. It then calls one of the
mailbox receive APIs. The mailbox searches its send queue and takes the message
from the first compatible thread it finds. If no compatible thread exists,
the receiving thread may choose to wait for one. If no compatible thread
appears before the waiting period specified by the receiving thread is reached,
the receive operation fails.
Once a receive operation completes successfully the receiving thread
can examine the message descriptor to determine which thread sent the message,
how much data was exchanged,
and the application-defined info value supplied by the sending thread.

Any number of receiving threads may wait simultaneously on a mailboxes'
receive queue. The threads are sorted according to their priority;
threads of equal priority are sorted so that the one that started waiting
first can receive a message first.

.. note::
    Receiving threads do not always receive messages in a first in, first out
    (FIFO) order, due to the thread compatibility constraints specified by the
    message descriptors. For example, if thread A waits to receive a message
    only from thread X and then thread B waits to receive a message from
    thread Y, an incoming message from thread Y to any thread will be given
    to thread B and thread A will continue to wait.

The receiving thread controls both the quantity of data it retrieves from an
incoming message and where the data ends up. The thread may choose to take
all of the data in the message, to take only the initial part of the data,
or to take no data at all. Similarly, the thread may choose to have the data
copied into a message buffer of its choice or to have it placed in a message
block. A message buffer is typically used when the volume of data
involved is small, and the cost of copying the data is less than the cost
of allocating and freeing a memory pool block.

The following sections outline various approaches a receiving thread may use
when retrieving message data.

Retrieving Data at Receive Time
-------------------------------

The most straightforward way for a thread to retrieve message data is to
specify a message buffer when the message is received. The thread indicates
both the location of the message buffer (which must not be :c:macro:`NULL`)
and its size.

The mailbox copies the message's data to the message buffer as part of the
receive operation. If the message buffer is not big enough to contain all of the
message's data, any uncopied data is lost. If the message is not big enough
to fill all of the buffer with data, the unused portion of the message buffer is
left unchanged. In all cases the mailbox updates the receiving thread's
message descriptor to indicate how many data bytes were copied (if any).

The immediate data retrieval technique is best suited for small messages
where the maximum size of a message is known in advance.

.. note::
   This technique can be used when the message data is actually located
   in a memory block supplied by the sending thread. The mailbox copies
   the data into the message buffer specified by the receiving thread, then
   frees the message block back to its memory pool. This allows
   a receiving thread to retrieve message data without having to know
   whether the data was sent using a message buffer or a message block.

The following code uses a mailbox to process variable-sized requests from any
producing thread, using the immediate data retrieval technique. The message
"info" field is used to exchange information about the maximum size
message buffer that each thread can handle.

.. code-block:: c

    void consumer_thread(void)
    {
        struct k_mbox_msg recv_msg;
        char buffer[100];

        int i;
        int total;

        while (1) {
            /* prepare to receive message */
            recv_msg.info = 100;
            recv_msg.size = 100;
            recv_msg.rx_source_thread = K_ANY;

            /* get a data item, waiting as long as needed */
            k_mbox_get(&my_mailbox, &recv_msg, buffer, K_FOREVER);

            /* info, size, and rx_source_thread fields have been updated */

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

Retrieving Data Later Using a Message Buffer
--------------------------------------------

A receiving thread may choose to defer message data retrieval at the time
the message is received, so that it can retrieve the data into a message buffer
at a later time.
The thread does this by specifying a message buffer location of :c:macro:`NULL`
and a size indicating the maximum amount of data it is willing to retrieve
later.

The mailbox does not copy any message data as part of the receive operation.
However, the mailbox still updates the receiving thread's message descriptor
to indicate how many data bytes are available for retrieval.

The receiving thread must then respond as follows:

* If the message descriptor size is zero, then either the sender's message
  contained no data or the receiving thread did not want to receive any data.
  The receiving thread does not need to take any further action, since
  the mailbox has already completed data retrieval and deleted the message.

* If the message descriptor size is non-zero and the receiving thread still
  wants to retrieve the data, the thread must call :cpp:func:`k_mbox_data_get()`
  and supply a message buffer large enough to hold the data. The mailbox copies
  the data into the message buffer and deletes the message.

* If the message descriptor size is non-zero and the receiving thread does *not*
  want to retrieve the data, the thread must call :cpp:func:`k_mbox_data_get()`.
  and specify a message buffer of :c:macro:`NULL`. The mailbox deletes
  the message without copying the data.

The subsequent data retrieval technique is suitable for applications where
immediate retrieval of message data is undesirable. For example, it can be
used when memory limitations make it impractical for the receiving thread to
always supply a message buffer capable of holding the largest possible
incoming message.

.. note::
   This technique can be used when the message data is actually located
   in a memory block supplied by the sending thread. The mailbox copies
   the data into the message buffer specified by the receiving thread, then
   frees the message block back to its memory pool. This allows
   a receiving thread to retrieve message data without having to know
   whether the data was sent using a message buffer or a message block.

The following code uses a mailbox's deferred data retrieval mechanism
to get message data from a producing thread only if the message meets
certain criteria, thereby eliminating unneeded data copying. The message
"info" field supplied by the sender is used to classify the message.

.. code-block:: c

    void consumer_thread(void)
    {
        struct k_mbox_msg recv_msg;
        char buffer[10000];

        while (1) {
            /* prepare to receive message */
            recv_msg.size = 10000;
            recv_msg.rx_source_thread = K_ANY;

            /* get message, but not its data */
            k_mbox_get(&my_mailbox, &recv_msg, NULL, K_FOREVER);

            /* get message data for only certain types of messages */
            if (is_message_type_ok(recv_msg.info)) {
                /* retrieve message data and delete the message */
                k_mbox_data_get(&recv_msg, buffer);

                /* process data in "buffer" */
                ...
            } else {
                /* ignore message data and delete the message */
                k_mbox_data_get(&recv_msg, NULL);
            }
        }
    }

Retrieving Data Later Using a Message Block
-------------------------------------------

A receiving thread may choose to retrieve message data into a memory block,
rather than a message buffer. This is done in much the same way as retrieving
data subsequently into a message buffer --- the receiving thread first
receives the message without its data, then retrieves the data by calling
:cpp:func:`k_mbox_data_block_get()`. The mailbox fills in the block descriptor
supplied by the receiving thread, allowing the thread to access the data.
The mailbox also deletes the received message, since data retrieval
has been completed. The receiving thread is then responsible for freeing
the message block back to the memory pool when the data is no longer needed.

This technique is best suited for applications where the message data has
been sent using a memory block.

.. note::
   This technique can be used when the message data is located in a message
   buffer supplied by the sending thread. The mailbox automatically allocates
   a memory block and copies the message data into it. However, this is much
   less efficient than simply retrieving the data into a message buffer
   supplied by the receiving thread. In addition, the receiving thread
   must be designed to handle cases where the data retrieval operation fails
   because the mailbox cannot allocate a suitable message block from the memory
   pool. If such cases are possible, the receiving thread must either try
   retrieving the data at a later time or instruct the mailbox to delete
   the message without retrieving the data.

The following code uses a mailbox to receive messages sent using a memory block,
thereby eliminating unneeded data copying when processing a large message.
(The messages may be sent synchronously or asynchronously.)

.. code-block:: c

    /* define a memory pool containing 1 block of 10000 bytes */
    K_MEM_POOL_DEFINE(my_pool, 10000, 10000, 1, 4);

    void consumer_thread(void)
    {
        struct k_mbox_msg recv_msg;
        struct k_mem_block recv_block;

        int total;
        char *data_ptr;
        int i;

        while (1) {
            /* prepare to receive message */
            recv_msg.size = 10000;
            recv_msg.rx_source_thread = K_ANY;

            /* get message, but not its data */
            k_mbox_get(&my_mailbox, &recv_msg, NULL, K_FOREVER);

            /* get message data as a memory block and discard message */
            k_mbox_data_block_get(&recv_msg, &my_pool, &recv_block, K_FOREVER);

            /* compute sum of all message bytes in memory block */
            total = 0;
            data_ptr = (char *)(recv_block.data);
            for (i = 0; i < recv_msg.size; i++) {
                total += data_ptr++;
            }

            /* release memory block containing data */
            k_mem_pool_free(&recv_block);
        }
    }

.. note::
    An incoming message that was sent using a message buffer is also processed
    correctly by this algorithm, since the mailbox automatically allocates
    a memory block from the memory pool and fills it with the message data.
    However, the performance benefit of using the memory block approach is lost.

Suggested Uses
**************

Use a mailbox to transfer data items between threads whenever the capabilities
of a message queue are insufficient.

Configuration Options
*********************

Related configuration options:

* :option:`CONFIG_NUM_MBOX_ASYNC_MSGS`

API Reference
*************

.. doxygengroup:: mailbox_apis
   :project: Zephyr
