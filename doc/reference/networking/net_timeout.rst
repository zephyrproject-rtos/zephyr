.. _net_timeout_interface:

Network Timeout
###############

.. contents::
    :local:
    :depth: 2

Overview
********

Zephyr's network infrastructure mostly uses the millisecond-resolution uptime
clock to track timeouts, with both deadlines and durations measured with
32-bit unsigned values.  The 32-bit value rolls over at 49 days 17 hours 2 minutes
47.296 seconds.

Timeout processing is often affected by latency, so that the time at which the
timeout is checked may be some time after it should have expired.  Handling
this correctly without arbitrary expectations of maximum latency requires that
the maximum delay that can be directly represented be a 31-bit non-negative
number (``INT32_MAX``), which overflows at 24 days 20 hours 31 minutes 23.648
seconds.

Most network timeouts are shorter than the delay rollover, but a few protocols
allow for delays that are represented as unsigned 32-bit values counting
seconds, which corresponds to a 42-bit millisecond count.

The net_timeout API provides a generic timeout mechanism to correctly track
the remaining time for these extended-duration timeouts.

Use
***

The simplest use of this API is:

#. Configure a network timeout using :c:func:`net_timeout_set()`.
#. Use :c:func:`net_timeout_evaluate()` to determine how long it is until the
   timeout occurs.  Schedule a timeout to occur after this delay.
#. When the timeout callback is invoked, use :c:func:`net_timeout_evaluate()`
   again to determine whether the timeout has completed, or whether there is
   additional time remaining.  If the latter, reschedule the callback.
#. While the timeout is running, use :c:func:`net_timeout_remaining` to get
   the number of seconds until the timeout expires.  This may be used to
   explicitly update the timeout, which should be done by canceling any
   pending callback and restarting from step 1 with the new timeout.

The :c:struct:`net_timeout` contains a ``sys_snode_t`` that allows multiple
timeout instances to be aggregated to share a single kernel timer element.
The application must use :c:func:`net_timeout_evaluate()` on all instances to
determine the next timeout event to occur.

:c:func:`net_timeout_deadline()` may be used to reconstruct the full-precision
deadline of the timeout.  This exists primarily for testing but may have use
in some applications, as it does allow a millisecond-resolution calculation of
remaining time.

API Reference
*************

.. doxygengroup:: net_timeout
