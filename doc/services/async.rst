.. _async_helpers:

Common Async Callback
#####################

Zephyr APIs often include :ref:`api_term_async` functions where an
operation is initiated and the application needs to be informed when it
completes, and whether it succeeded.  Using :c:func:`k_poll` with k_poll_signal
is often a good method, but some application architectures may be more
suited to a callback notification. This common callback type and an implementation
of it to raise a signal are enable flexibility in choosing.

When used with syscalls the callback address is meant to be compared with
known good async callbacks for things such as k_poll_signal and
used to interpret the type of data passed along. By doing so the syscall
parameter verification and copying may take place appropriately.

Direct callbacks are invalid from userspace.

API Reference
*************

.. doxygengroup:: async_utility_apis
