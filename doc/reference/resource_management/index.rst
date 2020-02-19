.. _resource_mgmt:

Resource Management
###################

There are various situations where it's necessary to coordinate resource
use at runtime among multiple clients.  These include power rails,
clocks, other peripherals, and binary device power management. The
complexity of properly managing multiple consumers of a device in a
multithreaded system, especially when transitions may be asynchronous,
suggests that a shared implementation is desirable.

.. contents::
    :local:
    :depth: 2


.. _resource_mgmt_onoff:

On-Off Services
***************

An on-off service supports an arbitrary number of clients of a service
which has a binary state.  Example applications are power rails, clocks,
and binary device power management.

The service has the following properties:

* The stable states are off, on, and error.  The service always begins
  in the off state.  The service may also be in a transition to a given
  state.
* The core operations are request (add a dependency) and release (remove
  a dependency). The service manages the state based on calls to
  functions that initiate these operations.
* The service transitions from off to on when first client request is
  received.
* The service transitions from on to off when last client release is
  received.
* Each service configuration provides functions that implement the
  transition from off to on, from on to off, and optionally from an
  error state to off.  Transitions that may put a calling thread to
  sleep must be flagged in the configuration to support safe invocation
  from non-thread context.
* All operations are asynchronous, and are initiated by a function call
  that references a specific service and is given client notification
  data. The function call will succeed or fail. On success, the
  operation is guaranteed to be initiated, but whether the operation
  itself succeeds or fails is indicated through client notification.
  The initiation functions can be invoked from pre-kernel, thread, or
  ISR context.  In contexts and states where the operation cannot
  be started the function will result in an error.
* Requests to turn on may be queued while a transition to off is in
  progress: when the service has turned off successfully it will be
  immediately turned on again (where context allows) and waiting clients
  notified when the start completes.

Requests are reference counted, but not tracked. That means clients are
responsible for recording whether their requests were accepted, and for
initiating a release only if they have previously successfully completed
a request.  Improper use of the API can cause an active client to be
shut out, and the service does not maintain a record of specific clients
that have been granted a request.

Failures in executing a transition are recorded and inhibit further
requests or releases until the service is reset. Pending requests are
notified (and cancelled) when errors are discovered.

Transition operation completion notifications are provided through any
of the following mechanisms:

* Signal: A pointer to a :c:type:`struct k_poll_signal` is provided, and
  the signal is raised when the transition completes. The operation
  completion code is stored as the signal value.
* Callback: a function pointer is provided by the client along with an
  opaque pointer, and on completion of the operation the function is
  invoked with the pointer and the operation completion code.
* Spin-wait: the client is required to check for operation completion
  using the :cpp:func:`onoff_client_fetch_result()` function.

Synchronous transition may be implemented by a caller based on its
context, for example by using :cpp:func:`k_poll()` to wait until the
completion is signalled.

.. doxygengroup:: resource_mgmt_apis
   :project: Zephyr
