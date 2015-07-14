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

+---------------------------------------+-----------------------------------+
| Call                                  | Description                       |
+=======================================+===================================+
| :c:func:`task_mbox_put()`             | Attempt to put data in a          |
|                                       | mailbox, and fail if the receiver |
|                                       | isn’t waiting.                    |
+---------------------------------------+-----------------------------------+
| :c:func:`task_mbox_put_wait()`        | Puts data in a mailbox,           |
|                                       | and waits for it to be received.  |
+---------------------------------------+-----------------------------------+
| :c:func:`task_mbox_put_wait_timeout()` | Puts data in a mailbox,          |
|                                        | and waits for it to be received, |
|                                        | with a timeout.                  |
+---------------------------------------+-----------------------------------+
| :c:func:`task_mbox_put_async()`       | Puts data in a mailbox            |
|                                       | asynchronously.                   |
+---------------------------------------+-----------------------------------+
| :c:func:`task_mbox_get()`             | Gets k_msg message                |
|                                       | header information from a mailbox |
|                                       | and gets mailbox data, or returns |
|                                       | immediately if the sender isn’t   |
|                                       | ready.                            |
+---------------------------------------+-----------------------------------+
| :c:func:`task_mbox_get_wait()`        | Gets k_msg                        |
|                                       | header information from a mailbox |
|                                       | and gets mailbox data, and waits  |
|                                       | until the sender is ready with    |
|                                       | data.                             |
+---------------------------------------+-----------------------------------+
| :c:func:`task_mbox_get_wait_timeout()` | Gets k_msg message               |
|                                        | header information from a        |
|                                        | mailbox and gets mailbox data,   |
|                                        | and waits until the sender is    |
|                                        | ready with a timeout.            |
+---------------------------------------+-----------------------------------+
| :c:func:`task_mbox_data_get()`        | Gets mailbox data and             |
|                                       | puts it in a buffer specified by  |
|                                       | a pointer.                        |
+---------------------------------------+-----------------------------------+
| :c:func:`task_mbox_data_get_async_block()` | Gets the mailbox data and    |
|                                       | puts it in a memory pool block.   |
+---------------------------------------+-----------------------------------+