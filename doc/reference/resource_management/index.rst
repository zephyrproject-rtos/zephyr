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
  a dependency). Supporting operations are reset (to clear an error
  state) and cancel (to reclaim client data from an in-progress
  transition).  The service manages the state based on calls to
  functions that initiate these operations.
* The service transitions from off to on when first client request is
  received.
* The service transitions from on to off when last client release is
  received.
* Each service configuration provides functions that implement the
  transition from off to on, from on to off, and optionally from an
  error state to off.  Transitions must be invokable from both thread
  and interrupt context.
* The request and reset operations are asynchronous using
  :ref:`async_notification`.  Both operations may be cancelled, but
  cancellation has no effect on the in-progress transition.
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

Transition operation completion notifications are provided through
:ref:`async_notification`.

Clients and other components interested in tracking all service state
changes, including when a service begins turning off or enters an error
state, can be informed of state transitions by registering a monitor
with onoff_monitor_register().  Notification of changes are provided
before issuing completion notifications associated with the new
state.

.. _resource_mgmt_sequence_manager:

Sequence Manager
****************

A sequence manager supports execution of a sequence of asynchronous actions.
User is notified when sequence is completed or when error occurs. Example
application are chain of I2C or SPI transfers.

A sequence is not modified by the manager thus if possible it can be constant.

The manager has the following properties:

* Manager can be in idle or active state. New sequence can be scheduled only if
  manager is in idle state. Error is returned otherwise.
* Manager assumes that actions are asynchronous and next action is triggered
  from the context in which previous action is completed.
* There are optional setup and teardown functions.
* Two modes are supported:
    * Fixed processing function associated with the manager. Each action
      contains specific data which is passed to the process function.
    * Custom process function where each action contains dedicated processing
      function followed by the data.
* Sequence is defined as an array of pointers thus each action data can have
  different size.
* Sequence can be aborted. Aborted sequence finalizes on next action completion.
* On action finalization user can influence sequence execution by providing
  offset which is taken into account when next item is picked. It can be used
  to skip or repeat action based on action result.

Sequence manager provides helper macros for creating sequences and actions.
:c:macro:`SYS_SEQ_DEFINE` can be used to create and initialize sequence with
:c:macro:`SYS_SEQ_ACTION` or :c:macro:`SYS_SEQ_CUSTOM_ACTION` as arguments to
instantiate and initialize actions.

Additionally, manager provides generic actions which can be used in custom mode:

* :c:macro:`SYS_SEQ_ACTION_PAUSE` - pause action. On pause action execution,
  provided handler is called. sys_seq_finalize must be called to resume sequence
  execution.
* :c:macro:`SYS_SEQ_ACTION_MS_DELAY` and :c:macro:`SYS_SEQ_ACTION_US_DELAY` -
  sequence execution is paused for requested amount of time. struct k_timer
  instance provided in sys_seq_mgr_init is used to apply the delay.

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
  interrupt handlers.  Note that operations may complete before the
  submit request has returned to its caller.

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
