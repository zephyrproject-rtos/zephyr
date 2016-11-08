.. _custom_data_v2:

Custom Data
###########

A thread's :dfn:`custom data` is a 32-bit, thread-specific value
that may be used by an application for any purpose.

.. contents::
    :local:
    :depth: 2

Concepts
********

Every thread has a 32-bit custom data area.
The custom data is accessible only by the thread itself,
and may be used by the application for any purpose it chooses.
The default custom data for a thread is zero.

.. note::
   Custom data support is not available to ISRs because they operate
   within a single shared kernel interrupt handling context.

Implementation
**************

Using Custom Data
=================

By default, thread custom data support is disabled. The configuration option
:option:`CONFIG_THREAD_CUSTOM_DATA` can be used to enable support.

The :cpp:func:`k_thread_custom_data_set()` and
:cpp:func:`k_thread_custom_data_get()` functions are used to write and read
a thread's custom data, respectively. A thread can only access its own
custom data, and not that of another thread.

The following code uses the custom data feature to record the number of times
each thread calls a specific routine.

.. note::
    Obviously, only a single routine can use this technique,
    since it monopolizes the use of the custom data feature.

.. code-block:: c

    int call_tracking_routine(void)
    {
        uint32_t call_count;

        if (k_is_in_isr()) {
	    /* ignore any call made by an ISR */
        } else {
            call_count = (uint32_t)k_thread_custom_data_get();
            call_count++;
            k_thread_custom_data_set((void *)call_count);
	}

        /* do rest of routine's processing */
        ...
    }

Suggested Uses
**************

Use thread custom data to allow a routine to access thread-specific information,
by using the custom data as a pointer to a data structure owned by the thread.

Configuration Options
*********************

Related configuration options:

* :option:`CONFIG_THREAD_CUSTOM_DATA`

APIs
****

The following thread custom data APIs are provided by :file:`kernel.h`:

* :cpp:func:`k_thread_custom_data_set()`
* :cpp:func:`k_thread_custom_data_get()`
