.. _app_event_manager:

Application Event Manager
#########################

.. contents::
   :local:
   :depth: 2

The Application Event Manager is a piece of software that supports development of consistent, modular, event-based applications.
In an event-based application, parts of the application functionality are separated into isolated modules that communicate with each other using events.
Events are submitted by modules and other modules can subscribe and react to them.
The Application Event Manager acts as coordinator of the event-based communication.

.. figure:: em_overview.svg
   :alt: Architecture of an application based on Application Event Manager

See the :ref:`app_event_manager_sample` sample for a simple example of how to use the Application Event Manager.

Events
******

Events are structured data types that are defined by the application and can contain additional data.

The Application Event Manager handles the events by processing and propagating all of them to the modules (listeners) that subscribe to a specific event.
Multiple modules can subscribe to the same event.
As part of this communication, listeners can process events differently based on their type.

The Application Event Manager provides API for defining, creating, and subscribing events.
See `Implementing an event type`_ for details about how to create custom event types.

Modules
*******

Modules are separate source files that can subscribe to every defined event.
You can use events for communication between modules.

There is no limitation as to how many events each module can subscribe to.
An application can have as many modules as required.

The Application Event Manager provides an API for subscribing modules to specific events defined in the application.
When a module subscribes to a specific event, it is called a listener module.
Every listener is identified by a unique name.

.. _app_event_manager_configuration:

Configuration
*************

To use the Application Event Manager, enable it using the :kconfig:option:`CONFIG_APP_EVENT_MANAGER` Kconfig option and initialize it in your :file:`main.c` file.
Initializing the Application Event Manager allows it to handle submitted events and deliver them to modules that subscribe to the specified event type.

Complete the following steps:

1. Enable the :kconfig:option:`CONFIG_APP_EVENT_MANAGER` Kconfig option.
#. Include :file:`app_event_manager.h` in your :file:`main.c` file.
#. Call :c:func:`app_event_manager_init()`.

.. _app_event_manager_implementing_events:

Implementing events and modules
*******************************

If an application module is supposed to react to an event, your application must implement an event type, submit the event, and register the module as listener.
Read the following sections for details.

Implementing an event type
==========================

If you want to easily create and implement custom event types, the Application Event Manager provides macros to add a new event type in your application.
Complete the following steps:

* `Create a header file`_ for the event type you want to define
* `Create a source file`_ for the event type

Create a header file
--------------------

To create a header file for the event type you want to define:

1. Make sure the header file includes the Application Event Manager header file:

   .. code-block:: c

	   #include <app_event_manager/app_event_manager.h>

#. Define the new event type by creating a structure that contains an :c:struct:`app_event_header` named ``header`` as the first field.
#. Optionally, add additional custom data fields to the structure.
#. Declare the event type with the :c:macro:`APP_EVENT_TYPE_DECLARE` macro, passing the name of the created structure as an argument.

The following code example shows a header file for the event type :c:struct:`sample_event`:

.. code-block:: c

   #include <app_event_manager/app_event_manager.h>

   struct sample_event {
	   struct app_event_header header;

	   /* Custom data fields. */
	   int8_t value1;
	   int16_t value2;
	   int32_t value3;
   };

   APP_EVENT_TYPE_DECLARE(sample_event);

In some use cases, the length of the data associated with an event may vary.
You can use the :c:macro:`APP_EVENT_TYPE_DYNDATA_DECLARE` macro instead of :c:macro:`APP_EVENT_TYPE_DECLARE` to declare an event type with variable data size.
In such case, add the data with the variable size as the last member of the event structure.
For example, you can add variable sized data to the previously defined event by applying the following change to the code:

.. code-block:: c

   #include <app_event_manager/app_event_manager.h>

   struct sample_event {
	   struct app_event_header header;

	   /* Custom data fields. */
	   int8_t value1;
	   int16_t value2;
	   int32_t value3;
	   struct event_dyndata dyndata;
   };

   APP_EVENT_TYPE_DYNDATA_DECLARE(sample_event);

In this example, the :c:struct:`event_dyndata` structure contains the following information:

* A zero-length array that is used as a buffer with variable size (:c:member:`event_dyndata.data`).
* A number representing the size of the buffer (:c:member:`event_dyndata.size`).

Create a source file
--------------------

To create a source file for the event type you defined in the header file:

1. Include the header file for the new event type in your source file.
#. Define the event type with the :c:macro:`APP_EVENT_TYPE_DEFINE` macro.
   Pass the name of the event type as declared in the header along with additional parameters.
   For example, you can provide a function that fills a buffer with a string version of the event data (used for logging).
   The :c:macro:`APP_EVENT_TYPE_DEFINE` macro adds flags as a last parameter.
   These flags are constant and can only be set using :c:macro:`APP_EVENT_FLAGS_CREATE` on :c:macro:`APP_EVENT_TYPE_DEFINE` macro.
   To not set any flag, use :c:macro:`APP_EVENT_FLAGS_CREATE` without any argument as shown in the below example.
   To get value of specific flag, use :c:func:`app_event_get_type_flag` function.

The following code example shows a source file for the event type ``sample_event``:

.. code-block:: c

   #include "sample_event.h"

   static void log_sample_event(const struct app_event_header *aeh)
   {
	   struct sample_event *event = cast_sample_event(aeh);

	   APP_EVENT_MANAGER_LOG(aeh, "val1=%d val2=%d val3=%d", event->value1,
			   event->value2, event->value3);
   }

   APP_EVENT_TYPE_DEFINE(sample_event,			/* Unique event name. */
		     log_sample_event,			/* Function logging event data. */
		     NULL,				/* No event info provided. */
		     APP_EVENT_FLAGS_CREATE());		/* Flags managing event type. */

.. note::
	There is a deprecated way of logging Application Event Manager events by writing a string to the provided buffer.
	To use the deprecated way, you need to set the :kconfig:option:`CONFIG_APP_EVENT_MANAGER_USE_DEPRECATED_LOG_FUN` option.
	You can then use both ways of logging events.
	Application Event Manager figures out which way to be used based on the type of the logging function passed.

Submitting an event
===================

To submit an event of a given type, for example ``sample_event``:

1. Allocate the event by calling the function with the name *new_event_type_name*.
   For example, ``new_sample_event()``.
#. Write values to the data fields.
#. Use :c:macro:`APP_EVENT_SUBMIT` to submit the event.

The following code example shows how to create and submit an event of type ``sample_event`` that has three data fields:

.. code-block:: c

	/* Allocate event. */
	struct sample_event *event = new_sample_event();

	/* Write data to datafields. */
	event->value1 = value1;
	event->value2 = value2;
	event->value3 = value3;

	/* Submit event. */
	APP_EVENT_SUBMIT(event);

If an event type also defines data with variable size, you must pass also the size of the data as an argument to the function that allocates the event.
For example, if the ``sample_event`` also contains data with variable size, you must apply the following changes to the code:

.. code-block:: c

	/* Allocate event. */
	struct sample_event *event = new_sample_event(my_data_size);

	/* Write data to datafields. */
	event->value1 = value1;
	event->value2 = value2;
	event->value3 = value3;

	/* Write data with variable size. */
	memcpy(event->dyndata.data, my_buf, my_data_size);

	/* Submit event. */
	APP_EVENT_SUBMIT(event);

After the event is submitted, the Application Event Manager adds it to the processing queue.
When the event is processed, the Application Event Manager notifies all modules that subscribe to this event type.

.. note::
	Events are dynamically allocated and must be submitted.
	If an event is not submitted, it will not be handled and the memory will not be freed.

.. _app_event_manager_register_module_as_listener:

Registering a module as listener
================================

If you want a module to receive events managed by the Application Event Manager, you must register it as a listener and you must subscribe it to a given event type.

To turn a module into a listener for specific event types, complete the following steps:

1. Include the header files for the respective event types, for example, ``#include "sample_event.h"``.
#. :ref:`Implement an Event handler function <app_event_manager_register_module_as_listener_handler>` and define the module as a listener with the :c:macro:`APP_EVENT_LISTENER` macro, passing both the name of the module and the event handler function as arguments.
#. Subscribe the listener to specific event types.

For subscribing to an event type, the Application Event Manager provides three types of subscriptions, differing in priority.
They can be registered with the following macros:

* :c:macro:`APP_EVENT_SUBSCRIBE_FIRST` - notification as the first subscriber
* :c:macro:`APP_EVENT_SUBSCRIBE_EARLY` - notification before other listeners
* :c:macro:`APP_EVENT_SUBSCRIBE` - standard notification
* :c:macro:`APP_EVENT_SUBSCRIBE_FINAL` - notification as the last, final subscriber

There is no defined order in which subscribers of the same priority are notified.

The module will receive events for the subscribed event types only.
The listener name passed to the subscribe macro must be the same one used in the macro :c:macro:`APP_EVENT_LISTENER`.

.. _app_event_manager_register_module_as_listener_handler:

Implementing an event handler function
--------------------------------------

The event handler function is called when any of the subscribed event types are being processed.
Only one event handler function can be registered per listener.
Therefore, if a listener subscribes to multiple event types, the function must handle all of them.

The event handler gets a pointer to the :c:struct:`app_event_header` structure as the function argument.
The function should return ``true`` to consume the event, which means that the event is not propagated to further listeners, or ``false``, otherwise.

To check if an event has a given type, call the function with the name *is*\_\ *event_type_name* (for example, ``is_sample_event()``), passing the pointer to the application event header as the argument.
This function returns ``true`` if the event matches the given type, or ``false`` otherwise.

To access the event data, cast the :c:struct:`app_event_header` structure to a proper event type, using the function with the name *cast*\_\ *event_type_name* (for example, ``cast_sample_event()``), passing the pointer to the application event header as the argument.

Code example
------------

The following code example shows how to register an event listener with an event handler function and subscribe to the event type ``sample_event``:

.. code-block:: c

	#include "sample_event.h"

	static bool app_event_handler(const struct app_event_header *aeh)
	{
		if (is_sample_event(aeh)) {

			/* Accessing event data. */
			struct sample_event *event = cast_sample_event(aeh);

			int8_t v1 = event->value1;
			int16_t v2 = event->value2;
			int32_t v3 = event->value3;

			/* Actions when received given event type. */
			foo(v1, v2, v3);

			return false;
		}

		return false;
	}

	APP_EVENT_LISTENER(sample_module, app_event_handler);
	APP_EVENT_SUBSCRIBE(sample_module, sample_event);

The variable size data is accessed in the same way as the other members of the structure defining an event.

Application Event Manager extensions
************************************

The Application Event Manager provides additional features that could be helpful when debugging event-based applications.

.. _app_event_manager_profiling_init_hooks:

Initialization hook
===================

.. em_initialization_hook_start

The Application Event Manager provides an initialization hook for any module that relies on the Application Event Manager initialization before the first event is processed.
The hook function should be declared in the ``int hook(void)`` format.
If the hook function returns a non-zero value, the initialization process is interrupted and a related error is returned.

To register the initialization hook, use the macro :c:macro:`APP_EVENT_MANAGER_HOOK_POSTINIT_REGISTER`.
For details, refer to :ref:`app_event_manager_api`.

.. em_initialization_hook_end

.. _app_event_manager_profiling_tracing_hooks:

Tracing hooks
=============

.. em_tracing_hooks_start

The Application Event Manager uses flexible mechanism to implement hooks when an event is submitted, before it is processed, and after its processing.
Oryginally designed to implement event tracing, the tracing hooks can be used for other purposes as well.
The registered hook function should be declared in the ``void hook(const struct app_event_header *aeh)`` format.

The following macros are implemented to register event tracing hooks:

* :c:macro:`APP_EVENT_HOOK_ON_SUBMIT_REGISTER_FIRST`, :c:macro:`APP_EVENT_HOOK_ON_SUBMIT_REGISTER`, :c:macro:`APP_EVENT_HOOK_ON_SUBMIT_REGISTER_LAST`
* :c:macro:`APP_EVENT_HOOK_PREPROCESS_REGISTER_FIRST`, :c:macro:`APP_EVENT_HOOK_PREPROCESS_REGISTER`, :c:macro:`APP_EVENT_HOOK_PREPROCESS_REGISTER_LAST`
* :c:macro:`APP_EVENT_HOOK_POSTPROCESS_REGISTER_FIRST`, :c:macro:`APP_EVENT_HOOK_POSTPROCESS_REGISTER`, :c:macro:`APP_EVENT_HOOK_POSTPROCESS_REGISTER_LAST`

For details, refer to :ref:`app_event_manager_api`.

.. em_tracing_hooks_end

.. _app_event_manager_profiling_mem_hooks:

Memory management hooks
=======================

The Application Event Manager implements default memory management functions using weak implementation.
You can override this implementation to implement other types of memory allocation.

The following weak functions are provided by the Application Event Manager as the memory management hooks:

* :c:func:`app_event_manager_alloc`
* :c:func:`app_event_manager_free`

For details, refer to :ref:`app_event_manager_api`.

Shell integration
=================

Shell integration is available to display additional information and to dynamically enable or disable logging for given event types.

The Event Manager is integrated with Zephyr's :ref:`shell_api` module.
When the shell is turned on, an additional subcommand set (:command:`app_event_manager`) is added.

This subcommand set contains the following commands:

:command:`show_listeners`
  Show all registered listeners.

:command:`show_subscribers`
  Show all registered subscribers.

:command:`show_events`
  Show all registered event types.
  The letters "E" or "D" indicate if logging is currently enabled or disabled for a given event type.

:command:`enable` or :command:`disable`
  Enable or disable logging.
  If called without additional arguments, the command applies to all event types.
  To enable or disable logging for specific event types, pass the event type indexes, as displayed by :command:`show_events`, as arguments.

.. _app_event_manager_api:

API documentation
*****************

| Header file: :file:`include/app_event_manager.h`
| Source files: :file:`subsys/app_event_manager/`

.. doxygengroup:: app_event_manager
   :project: nrf
   :members:
