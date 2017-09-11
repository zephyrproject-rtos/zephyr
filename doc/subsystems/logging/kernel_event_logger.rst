.. _kernel_event_logger_v2:

Kernel Event Logger
###################

The kernel event logger records the occurrence of certain types of kernel
events, allowing them to be subsequently extracted and reviewed.
This capability can be helpful in profiling the operation of an application,
either for debugging purposes or for optimizing the performance the application.

.. contents::
    :local:
    :depth: 2

Concepts
********

The kernel event logger does not exist unless it is configured for an
application. The capacity of the kernel event logger is also configurable.
By default, it has a ring buffer that can hold up to 128 32-bit words
of event information.

The kernel event logger is capable of recording the following predefined
event types:

* Interrupts.
* Context switching of threads.
* Kernel sleep events (i.e. entering and exiting a low power state).

The kernel event logger only records the predefined event types it has been
configured to record. Each event type can be enabled independently.

An application can also define and record custom event types.
The information recorded for a custom event, and the times
at which it is recorded, must be implemented by the application.

All events recorded by the kernel event logger remain in its ring buffer
until they are retrieved by the application for review and analysis. The
retrieval and analysis logic must be implemented by the application.

.. important::
    An application must retrieve the events recorded by the kernel event logger
    in a timely manner, otherwise new events will be dropped once the event
    logger's ring buffer becomes full. A recommended approach is to use
    a cooperative thread to retrieve the events, either on a periodic basis
    or as its sole responsibility.

By default, the kernel event logger records all occurrences of all event types
that have been enabled. However, it can also be configured to allow an
application to dynamically start or stop the recording of events at any time,
and to control which event types are being recorded. This permits
the application to capture only the events that occur during times
of particular interest, thereby reducing the work needed to analyze them.

.. note::
    The kernel event logger can also be instructed to ignore context switches
    involving a single specified thread. This can be used to avoid recording
    context switch events involving the thread that retrieves the events
    from the kernel event logger.

Event Formats
=============

Each event recorded by the kernel event logger consists of one or more
32-bit words of data that describe the event.

An **interrupt event** has the following format:

.. code-block:: c

    struct {
        u32_t timestamp;        /* time of interrupt */
        u32_t interrupt_id;     /* ID of interrupt */
    };

A **context-switch event** has the following format:

.. code-block:: c

    struct {
        u32_t timestamp;        /* time of context switch */
        u32_t context_id;       /* ID of thread that was switched out */
    };

A **sleep event** has the following format:

.. code-block:: c

    struct {
        u32_t sleep_timestamp;  /* time when CPU entered sleep mode */
        u32_t wake_timestamp;   /* time when CPU exited sleep mode */
        u32_t interrupt_id;     /* ID of interrupt that woke CPU */
    };

A **custom event** must have a type ID that does not conflict with
any existing predefined event type ID. The format of a custom event
is application-defined, but must contain at least one 32-bit data word.
A custom event may utilize a variable size, to allow different events
of a single type to record differing amounts of information.

Timestamps
==========

By default, the timestamp recorded with each predefined event is obtained from
the kernel's :ref:`hardware clock <clocks_v2>`. This 32-bit clock counts up
extremely rapidly, which means the timestamp value wraps around frequently.
(For example, the Lakemont APIC timer for Quark SE wraps every 134 seconds.)
This wraparound must be accounted for when analyzing kernel event logger data.
In addition, care must be taken when tickless idle is enabled, in case a sleep
duration exceeds 2^32 clock cycles.

If desired, the kernel event logger can be configured to record
a custom timestamp, rather than the default timestamp.
The application registers the callback function that generates the custom 32-bit
timestamp at run-time by calling :cpp:func:`sys_k_event_logger_set_timer()`.

Implementation
**************

Retrieving An Event
===================

An event can be retrieved from the kernel event logger in a blocking or
non-blocking manner using the following APIs:

* :cpp:func:`sys_k_event_logger_get()`
* :cpp:func:`sys_k_event_logger_get_wait()`
* :cpp:func:`sys_k_event_logger_get_wait_timeout()`

In each case, the API also returns the type and size of the event, as well
as the event information itself. The API also indicates how many events
were dropped between the occurrence of the previous event and the retrieved
event.

The following code illustrates how a thread can retrieve the events
recorded by the kernel event logger.

.. code-block:: c

    u16_t event_id;
    u8_t  dropped_count;
    u32_t data[3];
    u8_t  data_size;

    while(1) {
        /* retrieve an event */
        data_size = SIZE32_OF(data);
        res = sys_k_event_logger_get_wait(&event_id, &dropped_count, data,
                                          &data_size);

        if (dropped_count > 0) {
            /* ... Process the dropped events count ... */
        }

        if (res > 0) {
            /* process the event */
            switch (event_id) {
            case KERNEL_EVENT_LOGGER_CONTEXT_SWITCH_EVENT_ID:
                /* ... Process the context switch event ... */
                break;
            case KERNEL_EVENT_LOGGER_INTERRUPT_EVENT_ID:
                /* ... Process the interrupt event ... */
                break;
            case KERNEL_EVENT_LOGGER_SLEEP_EVENT_ID:
                /* ... Process the sleep event ... */
                break;
            default:
                printf("unrecognized event id %d\n", event_id);
            }
        } else if (res == -EMSGSIZE) {
            /* ... Data array is too small to hold the event! ... */
        }
    }

Adding a Custom Event Type
==========================

A custom event type must use an integer type ID that does not duplicate
an existing type ID. The type IDs for the predefined events can be found
in :file:`include/logging/kernel_event_logger.h`. If dynamic recording of
events is enabled, the event type ID must not exceed 32.

Custom events can be written to the kernel event logger using the following
APIs:

* :cpp:func:`sys_k_event_logger_put()`
* :cpp:func:`sys_k_event_logger_put_timed()`

Both of these APIs record an event as long as there is room in the kernel
event logger's ring buffer. To enable dynamic recording of a custom event type,
the application must first call :cpp:func:`sys_k_must_log_event()` to determine
if event recording is currently active for that event type.

The following code illustrates how an application can write a custom
event consisting of two 32-bit words to the kernel event logger.

.. code-block:: c

    #define MY_CUSTOM_EVENT_ID 8

    /* record custom event only if recording is currently wanted */
    if (sys_k_must_log_event(MY_CUSTOM_EVENT_ID)) {
        u32_t data[2];

        data[0] = custom_data_1;
        data[1] = custom_data_2;

        sys_k_event_logger_put(MY_CUSTOM_EVENT_ID, data, ARRAY_SIZE(data));
    }

The following code illustrates how an application can write a custom event
that records just a timestamp using a single 32-bit word.

.. code-block:: c

    #define MY_CUSTOM_TIME_ONLY_EVENT_ID 9

    if (sys_k_must_log_event(MY_CUSTOM_TIME_ONLY_EVENT_ID)) {
        sys_k_event_logger_put_timed(MY_CUSTOM_TIME_ONLY_EVENT_ID);
    }

Configuration Options
*********************

Related configuration options:

* :option:`CONFIG_KERNEL_EVENT_LOGGER`
* :option:`CONFIG_KERNEL_EVENT_LOGGER_CONTEXT_SWITCH`
* :option:`CONFIG_KERNEL_EVENT_LOGGER_INTERRUPT`
* :option:`CONFIG_KERNEL_EVENT_LOGGER_SLEEP`
* :option:`CONFIG_KERNEL_EVENT_LOGGER_BUFFER_SIZE`
* :option:`CONFIG_KERNEL_EVENT_LOGGER_DYNAMIC`
* :option:`CONFIG_KERNEL_EVENT_LOGGER_CUSTOM_TIMESTAMP`

Related Functions
*******************

The following kernel event logger APIs are provided by
:file:`kernel_event_logger.h`:

* :cpp:func:`sys_k_event_logger_register_as_collector()`
* :cpp:func:`sys_k_event_logger_get()`
* :cpp:func:`sys_k_event_logger_get_wait()`
* :cpp:func:`sys_k_event_logger_get_wait_timeout()`
* :cpp:func:`sys_k_must_log_event()`
* :cpp:func:`sys_k_event_logger_put()`
* :cpp:func:`sys_k_event_logger_put_timed()`
* :cpp:func:`sys_k_event_logger_get_mask()`
* :cpp:func:`sys_k_event_logger_set_mask()`
* :cpp:func:`sys_k_event_logger_set_timer()`

APIs
****

Event Logger
============

An event logger is an object that can record the occurrence of significant
events, which can be subsequently extracted and reviewed.

.. doxygengroup:: event_logger
   :project: Zephyr

Kernel Event Logger
===================

The kernel event logger records the occurrence of significant kernel events,
which can be subsequently extracted and reviewed.
(See :ref:`kernel_event_logger_v2`.)

.. doxygengroup:: kernel_event_logger
   :project: Zephyr
