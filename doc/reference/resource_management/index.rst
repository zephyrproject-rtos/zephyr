.. _resource_mgmt:

Resource Management
###################

There are various situations where it's necessary to coordinate resource
use at runtime among multiple clients.  These include power rails,
clocks, other peripherals, and binary device power management. The
complexity of properly managing multiple consumers of a device in a
multithreaded system, especially when transitions may be asynchronous,
suggests that a shared implementation is desirable.

Zephyr provides managers for several coordination policies.  These
managers are embedded into services that use them for specific
functions.

.. contents::
    :local:
    :depth: 2

.. _resource_mgmt_onoff:

On-Off Manager
**************

An on-off manager supports an arbitrary number of clients of a service
which has a binary state.  Example applications are power rails, clocks,
and binary device power management.

The manager has the following properties:

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
shut out, and the manager does not maintain a record of specific clients
that have been granted a request.

Failures in executing a transition are recorded and inhibit further
requests or releases until the manager is reset. Pending requests are
notified (and cancelled) when errors are discovered.

Transition operation completion notifications are provided through the
standard :ref:`async_notification`, supporting these methods:

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

.. doxygengroup:: resource_mgmt_onoff_apis
   :project: Zephyr

.. _resource_mgmt_queued_operation:

Queued Operation Manager
************************

While :ref:`resource_mgmt_onoff` supports a shared resource that must be
available as long as any user still depends on it, the queued operation
manager provides serialized exclusive access to a resource that executes
operations asynchronously.  This can be used to support (for example)
ADC sampling for different sensors, or groups of bus transactions.
Clients submit a operation request that is processed when the device
becomes available, with clients being notified of the completion of the
operation though the standard :ref:`async_notification`.

As with the on-off manager, the queued resource manager is a generic
infrastructure tool that should be used by a extending service, such as
an I2C bus controller or an ADC.  The manager has the following
characteristics:

* The stable states are idle and processing.  The manager always begins
  in the idle state.
* The core client operations are submit (add an operation) and cancel
  (remove an operation before it starts).
* Ownership of the operation object transitions from the client to the
  manager when a queue request is accepted, and is returned to the
  client when the manager notifies the client of operation completion.
* The core client event is completion.  Manager state changes only as a
  side effect from submitting or completing an operation.
* The service transitions from idle to processing when an operation is
  submitted.
* The service transitions from processing to idle when notification of
  the last operation has completed and there are no queued operations.
* The manager selects the next operation to process when notification of
  completion has itself completed.  In particular, changes to the set of
  pending operations that are made during a completion callback affect
  the next operation to execute.
* Each submitted operation includes a priority that orders execution by
  first-come-first-served within priority.
* Operations are asynchronous, with completion notification through the
  :ref:`async_notification`.  The operations and notifications are run
  in a context that is service-specific.  This may be one or more
  dedicated threads, or work queues.  Notifications may come from
  interrupt handlers.  Note that for some services certain operations
  may complete before the submit request has returned to its caller.

The generic infrastructure holds the active operation and a queue of
pending operations.  A service extension shall provide functions that:

* check that a request is well-formed, i.e. can be added to the queue;
* receive notification that a new operation is to be processed, or that
  no operations are available (allowing the service to enter a
  power-down mode);
* translate a generic completion callback into a service-specific
  callback.

.. doxygengroup:: resource_mgmt_queued_operation_apis
   :project: Zephyr
