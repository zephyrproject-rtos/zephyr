.. _async_notification:

Asynchronous Notification APIs
##############################

Zephyr APIs often include :ref:`api_term_async` functions where an
operation is initiated and the application needs to be informed when it
completes, and whether it succeeded.  Using :cpp:func:`k_poll()` is
often a good method, but some application architectures may be more
suited to a callback notification, and operations like enabling clocks
and power rails may need to be invoked before kernel functions are
available so a busy-wait for completion may be needed.

This API is intended to be embedded within specific subsystems such as
:ref:`resource_mgmt_onoff` and other APIs that support async
transactions.  The subsystem wrappers are responsible for extracting
operation-specific data from requests that include a notification
element, and for invoking callbacks with the parameters required by the
API.

API Reference
*************

.. doxygengroup:: sys_notify_apis
   :project: Zephyr
