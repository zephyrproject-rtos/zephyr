.. _input:

Input
#####

The input subsystem provides an API for dispatching input events from input
devices to the application.

Input Events
************

The subsystem is built around the :c:struct:`input_event` structure. An input
event represents a change in an individual event entity, for example the state
of a single button, or a movement in a single axis.

The :c:struct:`input_event` structure describes the specific event, and
includes a synchronization bit to indicate that the device reached a stable
state, for example when the events corresponding to multiple axes of a
multi-axis device have been reported.

Input Devices
*************

An input device can report input events directly using :c:func:`input_report`
or any related function; for example buttons or other on-off input entities
would use :c:func:`input_report_key`.

Complex devices may use a combination of multiple events, and set the ``sync``
bit once the output is stable.

The ``input_report*`` functions take a :c:struct:`device` pointer, which is
used to indicate which device reported the event and can be used by subscribers
to only receive events from a specific device. If there's no actual device
associated with the event, it can be set to ``NULL``, in which case only
subscribers with no device filter will receive the event.

Application API
***************

An application can register a callback using the
:c:macro:`INPUT_LISTENER_CB_DEFINE` macro. If a device node is specified, the
callback is only invoked for events from the specific device, otherwise the
callback will receive all the events in the system. This is the only type of
filtering supported, any more complex filtering logic has to be implemented in
the callback itself.

The subsystem can operate synchronously or by using an event queue, depending
on the :kconfig:option:`CONFIG_INPUT_MODE` option. If the input thread is used,
all the events are added to a queue and executed in a common ``input`` thread.
If the thread is not used, the callback are invoked directly in the input
driver context.

The synchronous mode can be used in a simple application to keep a minimal
footprint, or in a complex application with an existing event model, where the
callback is just a wrapper to pipe back the event in a more complex application
specific event system.

Kscan Compatibility
*******************

Input devices generating X/Y/Touch events can be used in existing applications
based on the :ref:`kscan_api` API by defining a
:dtcompatible:`zephyr,kscan-input` node as a childnode of the corresponding
input device.

API Reference
*************

.. doxygengroup:: input_interface

Input Event Definitions
***********************

.. doxygengroup:: input_events
