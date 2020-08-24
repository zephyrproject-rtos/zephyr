.. _logging_api:

Logging
#######

.. contents::
    :local:
    :depth: 2

The logger API provides a common interface to process messages issued by
developers. Messages are passed through a frontend and are then
processed by active backends.
Custom frontend and backends can be used if needed.
Default configuration uses built-in frontend and UART backend.

Summary of logger features:

- Deferred logging reduces the time needed to log a message by shifting time
  consuming operations to a known context instead of processing and sending
  the log message when called.
- Multiple backends supported (up to 9 backends).
- Custom frontend supported.
- Compile time filtering on module level.
- Run time filtering independent for each backend.
- Additional run time filtering on module instance level.
- Timestamping with user provided function.
- Dedicated API for dumping data.
- Dedicated API for handling transient strings.
- Panic support - in panic mode logger switches to blocking, synchronous
  processing.
- Printk support - printk message can be redirected to the logger.
- Design ready for multi-domain/multi-processor system.

Logger API is highly configurable at compile time as well as at run time. Using
Kconfig options (see :ref:`logger_kconfig`) logs can be gradually removed from
compilation to reduce image size and execution time when logs are not needed.
During compilation logs can be filtered out on module basis and severity level.

Logs can also be compiled in but filtered on run time using dedicate API. Run
time filtering is independent for each backend and each source of log messages.
Source of log messages can be a module or specific instance of the module.

There are four severity levels available in the system: error, warning, info
and debug. For each severity level the logger API (:zephyr_file:`include/logging/log.h`)
has set of dedicated macros. Logger API also has macros for logging data.

For each level following set of macros are available:

- ``LOG_X`` for standard printf-like messages, e.g. :c:macro:`LOG_ERR`.
- ``LOG_HEXDUMP_X`` for dumping data, e.g. :c:macro:`LOG_HEXDUMP_WRN`.
- ``LOG_INST_X`` for standard printf-like message associated with the
  particular instance, e.g. :c:macro:`LOG_INST_INF`.
- ``LOG_INST_HEXDUMP_X`` for dumping data associated with the particular
  instance, e.g. :c:macro:`LOG_HEXDUMP_INST_DBG`

There are two configuration categories: configurations per module and global
configuration. When logging is enabled globally, it works for modules. However,
modules can disable logging locally. Every module can specify its own logging
level. The module must define the :c:macro:`LOG_LEVEL` macro before using the
API. Unless a global override is set, the module logging level will be honored.
The global override can only increase the logging level. It cannot be used to
lower module logging levels that were previously set higher. It is also possible
to globally limit logs by providing maximal severity level present in the
system, where maximal means lowest severity (e.g. if maximal level in the system
is set to info, it means that errors, warnings and info levels are present but
debug messages are excluded).

Each module which is using the logger must specify its unique name and
register itself to the logger. If module consists of more than one file,
registration is performed in one file but each file must define a module name.

Logger's default frontend is designed to be thread safe and minimizes time needed
to log the message. Time consuming operations like string formatting or access to the
transport are not performed by default when logger API is called. When logger
API is called a message is created and added to the list. Dedicated,
configurable pool of log messages is used. There are 2 types of messages:
standard and hexdump. Each message contain source ID (module or instance ID and
domain ID which might be used for multiprocessor systems), timestamp and
severity level. Standard message contains pointer to the string and 32 bit
arguments. Hexdump message contains copied data.

.. _logger_kconfig:

Global Kconfig Options
**********************

These options can be found in the following path :zephyr_file:`subsys/logging/Kconfig`.

:option:`CONFIG_LOG`: Global switch, turns on/off the logger.

:option:`CONFIG_LOG_RUNTIME_FILTERING`: Enables runtime reconfiguration of the
logger.

:option:`CONFIG_LOG_MODE_OVERFLOW`: When logger cannot allocate new message
oldest one are discarded.

:option:`CONFIG_LOG_MODE_NO_OVERFLOW`: When logger cannot allocate new message
it is discarded.

:option:`CONFIG_LOG_BLOCK_IN_THREAD`: If enabled and new log message cannot
be allocated thread context will block for up to
:option:`CONFIG_LOG_BLOCK_IN_THREAD_TIMEOUT_MS` or until log message is
allocated.

:option:`CONFIG_LOG_DEFAULT_LEVEL`: Default level, sets the logging level
used by modules that are not setting their own logging level.

:option:`CONFIG_LOG_OVERRIDE_LEVEL`: It overrides module logging level when
it is not set or set lower than the override value.

:option:`CONFIG_LOG_MAX_LEVEL`: Maximal (lowest severity) level which is
compiled in.

:option:`CONFIG_LOG_FUNC_NAME_PREFIX_ERR`: Prepend standard ERROR log messages
with function name. Hexdump messages are not prepended.

:option:`CONFIG_LOG_FUNC_NAME_PREFIX_WRN`: Prepend standard WARNING log messages
with function name. Hexdump messages are not prepended.

:option:`CONFIG_LOG_FUNC_NAME_PREFIX_INF`: Prepend standard INFO log messages
with function name. Hexdump messages are not prepended.

:option:`CONFIG_LOG_FUNC_NAME_PREFIX_DBG`: Prepend standard DEBUG log messages
with function name. Hexdump messages are not prepended.

:option:`CONFIG_LOG_PRINTK`: Redirect printk calls to the logger.

:option:`CONFIG_LOG_PRINTK_MAX_STRING_LENGTH`: Maximal string length that can
be processed by printk. Longer strings are trimmed.

:option:`CONFIG_LOG_IMMEDIATE`: Messages are processed in the context
of the log macro call. Note that it can lead to errors when logger is used in
the interrupt context.

:option:`CONFIG_LOG_PROCESS_TRIGGER_THRESHOLD`: When number of buffered log
messages reaches the threshold dedicated thread (see :c:func:`log_thread_set`)
is waken up. If :option:`CONFIG_LOG_PROCESS_THREAD` is enabled then this
threshold is used by the internal thread.

:option:`CONFIG_LOG_PROCESS_THREAD`: When enabled, logger is creating own thread
which handles log processing.

:option:`CONFIG_LOG_BUFFER_SIZE`: Number of bytes dedicated for the logger
message pool. Single message capable of storing standard log with up to 3
arguments or hexdump message with 12 bytes of data take 32 bytes.

:option:`CONFIG_LOG_DETECT_MISSED_STRDUP`: Enable detection of missed transient
strings handling.

:option:`CONFIG_LOG_STRDUP_MAX_STRING`: Longest string that can be duplicated
using log_strdup().

:option:`CONFIG_LOG_STRDUP_BUF_COUNT`: Number of buffers in the pool used by
log_strdup().

:option:`CONFIG_LOG_DOMAIN_ID`: Domain ID. Valid in multi-domain systems.

:option:`CONFIG_LOG_FRONTEND`: Redirect logs to a custom frontend.

:option:`CONFIG_LOG_BACKEND_UART`: Enabled build-in UART backend.

:option:`CONFIG_LOG_BACKEND_SHOW_COLOR`: Enables coloring of errors (red)
and warnings (yellow).

:option:`CONFIG_LOG_BACKEND_FORMAT_TIMESTAMP`: If enabled timestamp is
formatted to *hh:mm:ss:mmm,uuu*. Otherwise is printed in raw format.

.. _log_usage:

Usage
*****

Logging in a module
===================

In order to use logger in the module, a unique name of a module must be
specified and module must be registered with the logger core using
:c:macro:`LOG_MODULE_REGISTER`. Optionally, a compile time log level for the
module can be specified as the second parameter. Default log level
(:option:`CONFIG_LOG_DEFAULT_LEVEL`) is used if custom log level is not
provided.

.. code-block:: c

   #include <logging/log.h>
   LOG_MODULE_REGISTER(foo, CONFIG_FOO_LOG_LEVEL);

If the module consists of multiple files, then ``LOG_MODULE_REGISTER()`` should
appear in exactly one of them. Each other file should use
:c:macro:`LOG_MODULE_DECLARE` to declare its membership in the module.
Optionally, a compile time log level for the module can be specified as
the second parameter. Default log level (:option:`CONFIG_LOG_DEFAULT_LEVEL`)
is used if custom log level is not provided.

.. code-block:: c

   #include <logging/log.h>
   /* In all files comprising the module but one */
   LOG_MODULE_DECLARE(foo, CONFIG_FOO_LOG_LEVEL);

In order to use logger API in a function implemented in a header file
:c:macro:`LOG_MODULE_DECLARE` macro must be used in the function body
before logger API is called. Optionally, a compile time log level for the module
can be specified as the second parameter. Default log level
(:option:`CONFIG_LOG_DEFAULT_LEVEL`) is used if custom log level is not
provided.

.. code-block:: c

   #include <logging/log.h>

   static inline void foo(void)
   {
   	LOG_MODULE_DECLARE(foo, CONFIG_FOO_LOG_LEVEL);

   	LOG_INF("foo");
   }

Dedicated Kconfig template (:zephyr_file:`subsys/logging/Kconfig.template.log_config`)
can be used to create local log level configuration.

Example below presents usage of the template. As a result CONFIG_FOO_LOG_LEVEL
will be generated:

.. code-block:: none

   module = FOO
   module-str = foo
   source "subsys/logging/Kconfig.template.log_config"

Logging in a module instance
============================

In case of modules which are multi-instance and instances are widely used
across the system enabling logs will lead to flooding. Logger provide the tools
which can be used to provide filtering on instance level rather than module
level. In that case logging can be enabled for particular instance.

In order to use instance level filtering following steps must be performed:

- a pointer to specific logger structure is declared in instance structure.
  :c:macro:`LOG_INSTANCE_PTR_DECLARE` is used for that.

.. code-block:: c

   #include <logging/log_instance.h>

   struct foo_object {
   	LOG_INSTANCE_PTR_DECLARE(log);
   	uint32_t id;
   }

- module must provide macro for instantiation. In that macro, logger instance
  is registered and log instance pointer is initialized in the object structure.

.. code-block:: c

   #define FOO_OBJECT_DEFINE(_name)                             \
   	LOG_INSTANCE_REGISTER(foo, _name, CONFIG_FOO_LOG_LEVEL) \
   	struct foo_object _name = {                             \
   		LOG_INSTANCE_PTR_INIT(log, foo, _name)          \
   	}

Note that when logger is disabled logger instance and pointer to that instance
are not created.

In order to use the instance logging API in a source file, a compile-time log
level must be set using :c:macro:`LOG_LEVEL_SET`.

.. code-block:: c

   LOG_LEVEL_SET(CONFIG_FOO_LOG_LEVEL);

   void foo_init(foo_object *f)
   {
   	LOG_INST_INF(f->log, "Initialized.");
   }

In order to use the instance logging API in a header file, a compile-time log
level must be set using :c:macro:`LOG_LEVEL_SET`.

.. code-block:: c

   static inline void foo_init(foo_object *f)
   {
   	LOG_LEVEL_SET(CONFIG_FOO_LOG_LEVEL);

   	LOG_INST_INF(f->log, "Initialized.");
   }

Controlling the logger
======================

Logger can be controlled using API defined in
:zephyr_file:`include/logging/log_ctrl.h`. Logger must be initialized before it can be
used. Optionally, user can provide function which returns timestamp value. If
not provided, :c:macro:`k_cycle_get_32` is used for timestamping.
:c:func:`log_process` function is used to trigger processing of one log
message (if pending). Function returns true if there is more messages pending.

Following snippet shows how logger can be processed in simple forever loop.

.. code-block:: c

   #include <log_ctrl.h>

   void main(void)
   {
   	log_init();

   	while (1) {
   		if (log_process() == false) {
   			/* sleep */
   		}
   	}
   }

Logger controlling API contains also functions for run time reconfiguration of
the logger. If run time filtering is enabled the :c:func:`log_filter_set` can
be used to change maximal severity level for given module. Module is identified
by source ID and domain ID. Source ID can be retrieved if source name is known
by iterating through all registered sources.

If logger is processed from a thread then it is possible to enable a feature
which will wake up processing thread when certain amount of log messages are
buffered (see :option:`CONFIG_LOG_PROCESS_TRIGGER_THRESHOLD`). It is also
possible to enable internal logger thread (see
:option:`CONFIG_LOG_PROCESS_THREAD`). In that case logger thread is initialized
and log messages are processed implicitly.

.. _logger_panic:

Logger panic
************

In case of error condition system usually can no longer rely on scheduler or
interrupts. In that situation deferred log message processing is not an option.
Logger controlling API provides a function for entering into panic mode
(:c:func:`log_panic`) which should be called in such situation.

When :c:func:`log_panic` is called, logger sends _panic_ notification to
all active backends. It is backend responsibility to react. Backend should
switch to blocking, synchronous mode (stop using interrupts) or disable itself.
Once all backends are notified, logger flushes all buffered messages. Since
that moment all logs are processed in a blocking way.

.. _log_architecture:

Architecture
************

Logger consists of 3 main parts:

- Frontend
- Core
- Backends

Log message is generated by a source of logging which can be a module or
instance of a module.

Default Frontend
================

Default frontend is engaged when logger API is called in a source of logging (e.g.
:c:macro:`LOG_INF`) and is responsible for filtering a message (compile and run
time), allocating buffer for the message, creating the message and putting that
message into the list of pending messages. Since logger API can be called in an
interrupt, frontend is optimized to log the message as fast as possible. Each
log message consists of one or more fixed size chunks. Message head chunk
contains log entry details like: source ID, timestamp, severity level and the
data (string pointer and arguments or raw data). Message contains also a
reference counter which indicates how many users still uses this message. It is
used to return message to the pool once last user indicates that it can be
freed. If more than 3 arguments or 12 bytes of raw data is used in the log then
log message is formed from multiple chunks which are linked together.

It may happen that frontend cannot allocate message. It happens if system is
generating more log messages than it can process in certain time frame. There
are two strategies to handle that case:

- Overflow - oldest pending messages are freed, before backends process them,
  until new message can be allocated.
- No overflow - new log is dropped if message cannot be allocated.

Second option is simpler however in many case less welcomed. On the other hand,
handling overflows degrades performance of the logger since allocating a
message requires freeing other messages which degrades logger performance. It
is thus recommended to avoid such cases by increasing logger buffer or
filtering out logs.

If run-time filtering is enabled, then for each source of logging a filter
structure in RAM is declared. Such filter is using 32 bits divided into ten 3
bit slots. Except *slot 0*, each slot stores current filter for one backend in
the system. *Slot 0* (bits 0-2) is used to aggregate maximal filter setting for
given source of logging. Aggregate slot determines if log message is created
for given entry since it indicates if there is at least one backend expecting
that log entry. Backend slots are examined when message is process by the
logger core to determine if message is accepted by given backend.

In the example below backend 1 is set to receive errors (*slot 1*) and backend
2 up to info level (*slot 2*). Slots 3-9 are not used. Aggregated filter
(*slot 0*) is set to info level and up to this level message from that
particular source will be buffered.

+------+------+------+------+-----+------+
|slot 0|slot 1|slot 2|slot 3| ... |slot 9|
+------+------+------+------+-----+------+
| INF  | ERR  | INF  | OFF  | ... | OFF  |
+------+------+------+------+-----+------+

Custom Frontend
===============

Custom frontend is enabled using :option:`CONFIG_LOG_FRONTEND`. Logs are redirected
to functions declared in :zephyr_file:`include/logging/log_frontend.h`.
This may be required in very time-sensitive cases, but this hurts
logger functionality. All features from default frontend, core and all backends
are not used.

Core
====

When log processing is triggered, a message is removed from the list of pending
messages.  If runtime filtering is disabled, the message is passed to all
active backends, otherwise the message is passed to only those backends that
have requested messages from that particular source (based on the source ID in
the message), and severity level. Once all backends are iterated, the message
is considered processed by the logger, but the message may still be in use by a
backend.

.. _logger_strings:

Logging strings
===============
Logger stores the address of a log message string argument passed to it. Because
a string variable argument could be transient, allocated on the stack, or
modifiable, logger provides a mechanism and a dedicated buffer pool to hold
copies of strings.  The buffer size and count is configurable
(see :option:`CONFIG_LOG_STRDUP_MAX_STRING` and
:option:`CONFIG_LOG_STRDUP_BUF_COUNT`).


If a string argument is transient, the user must call :c:func:`log_strdup` to
duplicate the passed string into a buffer from the pool. See the examples below.
If a strdup buffer cannot be allocated, a warning message is logged and an error
code returned indicating :option:`CONFIG_LOG_STRDUP_BUF_COUNT` should be
increased. Buffers are freed together with the log message.

.. code-block:: c

   char local_str[] = "abc";

   LOG_INF("logging transient string: %s", log_strdup(local_str));
   local_str[0] = '\0'; /* String can be modified, logger will use duplicate."

When :option:`CONFIG_LOG_DETECT_MISSED_STRDUP` is enabled logger will scan
each log message and report if string format specifier is found and string
address is not in read only memory section or does not belong to memory pool
dedicated to string duplicates. It indictes that :c:func:`log_strdup` is
missing in a call to log a message, such as ``LOG_INF``.

Logger backends
===============

Logger supports up to 9 concurrent backends. Logger backend interface consists
of two functions:

- :c:func:`log_backend_put` - backend gets log message.
- :c:func:`log_backend_panic` - on that call backend is notified that it must
  switch to panic (synchronous) mode. If backend cannot support synchronous,
  interrupt-less operation (e.g. network) it should stop any processing.

The log message contains a reference counter tracking how many backends are
processing the message. On receiving a message backend must claim it by calling
:c:func:`log_msg_get` on that message which increments a reference counter.
Once message is processed, backend puts back the message
(:c:func:`log_msg_put`) decrementing a reference counter. On last
:c:func:`log_msg_put`, when reference counter reaches 0, message is returned
to the pool. It is up to the backend how message is processed. If backend
intends to format message into the string, helper function for that are
available in :zephyr_file:`include/logging/log_output.h`.

Example message formatted using :c:func:`log_output_msg_process`.

.. code-block:: console

   [00:00:00.000,274] <info> sample_instance.inst1: logging message

.. note::

   The message pool can be starved if a backend does not call
   :c:func:`log_msg_put` when it is done processing a message. The logger
   core has no means to force messages back to the pool if they're still marked
   as in use (with a non-zero reference counter).

.. code-block:: c

   #include <log_backend.h>

   void put(const struct log_backend *const backend,
   	    struct log_msg *msg)
   {
   	log_msg_get(msg);

	/* message processing */

   	log_msg_put(msg);
   }

Logger backends are registered to the logger using
:c:macro:`LOG_BACKEND_DEFINE` macro. The macro creates an instance in the
dedicated memory section. Backends can be dynamically enabled
(:c:func:`log_backend_enable`) and disabled.

Limitations
***********

The Logger architecture has the following limitations:

- Strings as arguments (*%s*) require special treatment (see
  :ref:`logger_strings`).
- Logging double floating point variables is not possible because arguments are
  32 bit values.
- Number of arguments in the string is limited to 9.


API Reference
*************

Logger API
==========

.. doxygengroup:: log_api
   :project: Zephyr

Logger control
==============

.. doxygengroup:: log_ctrl
   :project: Zephyr

Log message
===========

.. doxygengroup:: log_msg
   :project: Zephyr

Logger backend interface
========================

.. doxygengroup:: log_backend
   :project: Zephyr

Logger output formatting
========================

.. doxygengroup:: log_output
   :project: Zephyr
