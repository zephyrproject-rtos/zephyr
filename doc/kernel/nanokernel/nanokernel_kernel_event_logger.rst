.. _nanokernel_event_logger:

Kernel Event Logger
###################

Definition
**********

The kernel event logger is a standardized mechanism to record events within the Kernel while
providing a single interface for the user to collect the data. This mechanism is currently used
to log the following events:

* Sleep events (entering and exiting low power conditions).
* Context switch events.
* Interrupt events.

Kernel Event Logger Configuration
*********************************

Kconfig provides the ability to enable and disable the collection of events and to configure the
size of the buffer used by the event logger.

These options can be found in the following path :file:`kernel/Kconfig`.

General kernel event logger configuration:

* :option:`KERNEL_EVENT_LOGGER_BUFFER_SIZE`

  Default size: 128 words, 32-bit length.

Profiling points configuration:

* :option:`KERNEL_EVENT_INTERRUPT`

  Enables recording of interrupt-driven events by providing timestamp information.

* :option:`KERNEL_EVENT_SLEEP`

  Enables recording of sleep events:

    * Timestamp when the CPU went to sleep mode.
    * Timestamp when the CPU woke up.
    * The interrupt Id that woke the CPU up.

* :option:`KERNEL_EVENT_CONTEXT`

  Enables recording of context-switching events. Details include:

    * Which thread is leaving the CPU.
    * Timestamp when the event has occurred.

Adding a Kernel Event Logging Point
***********************************

Custom trace points can be added with the following API:

* :cpp:func:`sys_k_event_logger_put()`

  Adds the profile of a new event with custom data.

* :cpp:func:`sys_k_event_logger_put_timed()`

  Adds timestamped profile of a new event.

.. important::

   The data must be in 32-bit sized blocks.

Retrieving Kernel Event Data
****************************

Applications are required to implement a fiber for accessing the recorded event messages
in both the nanokernel and microkernel systems. Developers can use the provided API to
retrieve the data, or may write their own routines using the ring buffer provided by the
event logger.

The API functions provided are:

* :cpp:func:`sys_k_event_logger_get()`
* :cpp:func:`sys_k_event_logger_get_wait()`
* :cpp:func:`sys_k_event_logger_get_wait_timeout()`

The above functions specify various ways to retrieve a event message and to copy it to
the provided buffer. When the buffer size is smaller than the message, the function will
return an error. All three functions retrieve messages via a FIFO method. The :literal:`wait`
and :literal:`wait_timeout` functions allow the caller to pend until a new message is
logged, or until the timeout expires.

Message Formats
***************

Interrupt-driven Event Messaging
--------------------------------

The data of the interrupt-driven event message comes in two block of 32 bits:

* The first block contains the timestamp occurrence of the interrupt event.
* The second block contains the Id of the interrupt.

Example:

.. code-block:: c

   uint32_t data[2];
   data[0] = timestamp_event;
   data[1] = interrupt_id;

Context-switch Event Messaging
------------------------------

The data of the context-switch event message comes in two block of 32 bits:

* The first block contains the timestamp occurrence of the context-switch event.
* The second block contains the thread id of the context involved.

Example:

.. code-block:: c

   uint32_t data[2];
   data[0] = timestamp_event;
   data[1] = context_id;

Sleep Event Messaging
---------------------

The data of the sleep event message comes in three block of 32 bits:

* The first block contains the timestamp when the CPU went to sleep mode.
* The second block contains the timestamp when the CPU woke up.
* The third block contains the interrupt Id that woke the CPU up.

Example:

.. code-block:: c

   uint32_t data[3];
   data[0] = timestamp_went_sleep;
   data[1] = timestamp woke_up.
   data[2] = interrupt_id.


Task Monitor
------------

The task monitor tracks the activities of the task schedule server
in the microkernel and it is able to report three different types of
events related with the scheduler activities:


Task Monitor Task State Change Event
++++++++++++++++++++++++++++++++++++

The Task Monitor Task State Change Event tracks the task's status changes.
The event data is arranged as three 32 bit blocks:

* The first block contains the timestamp when the task server
  changed the task status.
* The second block contains the task ID of the affected task.
* The thid block contains a 32 bit number with the new status.

Example:

.. code-block:: c

   uint32_t data[3];

   data[0] = timestamp;
   data[1] = task_id.
   data[2] = status_data.

Task Monitor Kevent Event
+++++++++++++++++++++++++

The Task Monitor Kevent Event tracks the commands requested to the
task server by the kernel. The event data is arranged as two blocks
of 32 bits each:

* The first block contains the timestamp when the task server
  attended the kernel command.
* The second block contains the code of the command.

.. code-block:: c

   uint32_t data[3];

   data[0] = timestamp;
   data[1] = event_code.

Task Monitor Command Packet Event
+++++++++++++++++++++++++++++++++

The Task Monitor Command Packet Event track the command packets sent
to the task server. The event data is arranged as three blocks of
32 bits each:

* The first block contains the timestamp when the task server
  attended the kernel command.
* The second block contains the task identifier of the task
  affected by the packet.
* The thid block contains the memory vector of the routine
  executed by the task server.

Example:

.. code-block:: c

   uint32_t data[3];

   data[0] = timestamp;
   data[1] = task_id.
   data[2] = comm_handler.

Example: Retrieving Profiling Messages
======================================

.. code-block:: c

   uint32_t data[3];
   uint8_t data_length = SIZE32_OF(data);
   uint8_t dropped_count;

   while(1) {
      /* collect the data */
      res = sys_k_event_logger_get_wait(&event_id, &dropped_count, data,
         &data_length);

      if (dropped_count > 0) {
         /* process the message dropped count */
      }

      if (res > 0) {
         /* process the data */
         switch (event_id) {
         case KERNEL_EVENT_CONTEXT_SWITCH_EVENT_ID:
            /* ... Process the context switch event data ... */
            break;
         case KERNEL_EVENT_INTERRUPT_EVENT_ID:
            /* ... Process the interrupt event data ... */
            break;
         case KERNEL_EVENT_SLEEP_EVENT_ID:
            /* ... Process the data for a sleep event ... */
            break;
         case KERNEL_EVENT_LOGGER_TASK_MON_TASK_STATE_CHANGE_EVENT_ID:
            /* ... Process the data for a task monitor event ... */
            break;
         case KERNEL_EVENT_LOGGER_TASK_MON_KEVENT_EVENT_ID:
            /* ... Process the data for a task monitor command event ... */
            break;
         case KERNEL_EVENT_LOGGER_TASK_MON_CMD_PACKET_EVENT_ID:
            /* ... Process the data for a task monitor packet event ... */
            break;
         default:
            printf("unrecognized event id %d\n", event_id);
         }
      } else {
         if (res == -EMSGSIZE) {
            /* ERROR - The buffer provided to collect the
             * profiling events is too small.
             */
         } else if (ret == -EAGAIN) {
            /* There is no message available in the buffer */
         }
      }
   }

.. note::

   To see an example that shows how to collect the kernel event data, check the sample
   project: :file:`samples/microkernel/apps/kernel_event_logger_sample`.

Example: Adding a Kernel Event Logging Point
============================================

.. code-block:: c

   uint32_t data[2];

   data[0] = custom_data_1;
   data[1] = custom_data_2;

   sys_k_event_logger_put(KERNEL_EVENT_LOGGER_CUSTOM_ID, data, ARRAY_SIZE(data));

Use the following function to register only the time of an event.

.. code-block:: c

   sys_k_event_logger_put_timed(KERNEL_EVENT_LOGGER_CUSTOM_ID);

APIs
****

The following APIs are provided by the :file:`k_event_logger.h` file:

:cpp:func:`sys_k_event_logger_register_as_collector()`
   Register the current fiber as the collector fiber.

:cpp:func:`sys_k_event_logger_put()`
   Enqueue a kernel event logger message with custom data.

:cpp:func:`sys_k_event_logger_put_timed()`
   Enqueue a kernel event logger message with the current time.

:cpp:func:`sys_k_event_logger_get()`
   De-queue a kernel event logger message.

:cpp:func:`sys_k_event_logger_get_wait()`
   De-queue a kernel event logger message. Wait if the buffer is empty.

:cpp:func:`sys_k_event_logger_get_wait_timeout()`
   De-queue a kernel event logger message. Wait if the buffer is empty until the timeout expires.
