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

.. note::

   A generic API may be implemented by multiple drivers where the common
   case is asynchronous.  The on-off client structure may be an
   appropriate solution for the generic API.  Where drivers that can
   guarantee synchronous context-independent transitions a driver may
   use :c:struct:`onoff_sync_service` and its supporting API rather than
   :c:struct:`onoff_manager`, with only a small reduction in functionality
   (primarily no support for the monitor API).

.. doxygengroup:: resource_mgmt_onoff_apis
