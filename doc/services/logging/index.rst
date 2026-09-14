.. _logging_api:

Logging
#######

.. contents::
    :local:
    :depth: 2

The logging API provides a common interface to process messages issued by
developers. Messages are passed through a frontend and are then
processed by active backends.
Custom frontend and backends can be used if needed.

Summary of the logging features:

- Deferred logging reduces the time needed to log a message by shifting time
  consuming operations to a known context instead of processing and sending
  the log message when called.
- Multiple backends supported (up to 9 backends).
- Custom frontend support. It can work together with backends.
- Compile time filtering on module level.
- Run time filtering independent for each backend.
- Additional run time filtering on module instance level.
- Timestamping with user provided function. Timestamp can have 32 or 64 bits.
- Dedicated API for dumping data.
- Dedicated API for handling transient strings.
- Panic support - in panic mode logging switches to blocking, synchronous
  processing.
- Printk support - printk message can be redirected to the logging.
- Design ready for multi-domain/multi-processor system.
- Support for logging floating point variables and long long arguments.
- Built-in copying of transient strings used as arguments.
- Support for multi-domain logging.
- Rate-limited logging macros to prevent log flooding when messages are generated frequently.

Logging API is highly configurable at compile time as well as at run time. Using
Kconfig options (see :ref:`logging_kconfig`) logs can be gradually removed from
compilation to reduce image size and execution time when logs are not needed.
During compilation logs can be filtered out on module basis and severity level.

Logs can also be compiled in but filtered on run time using dedicate API. Run
time filtering is independent for each backend and each source of log messages.
Source of log messages can be a module or specific instance of the module.

There are four severity levels available in the system: error, warning, info
and debug. For each severity level the logging API (:zephyr_file:`include/zephyr/logging/log.h`)
has set of dedicated macros. Logger API also has macros for logging data.

For each level the following set of macros are available:

- ``LOG_X`` for standard printf-like messages, e.g. :c:macro:`LOG_ERR`.
- ``LOG_HEXDUMP_X`` for dumping data, e.g. :c:macro:`LOG_HEXDUMP_WRN`.
- ``LOG_INST_X`` for standard printf-like message associated with the
  particular instance, e.g. :c:macro:`LOG_INST_INF`.
- ``LOG_INST_HEXDUMP_X`` for dumping data associated with the particular
  instance, e.g. :c:macro:`LOG_INST_HEXDUMP_DBG`

The warning level also exposes the following additional macro:

- :c:macro:`LOG_WRN_ONCE` for warnings where only the first occurrence is of interest.

Rate-limited logging macros are also available for all severity levels to prevent log flooding:

- ``LOG_X_RATELIMIT`` for rate-limited standard printf-like messages using default rate, e.g. :c:macro:`LOG_ERR_RATELIMIT`.
- ``LOG_X_RATELIMIT_RATE`` for rate-limited standard printf-like messages with custom rate, e.g. :c:macro:`LOG_ERR_RATELIMIT_RATE`.
- ``LOG_HEXDUMP_X_RATELIMIT`` for rate-limited data dumping using default rate, e.g. :c:macro:`LOG_HEXDUMP_WRN_RATELIMIT`.
- ``LOG_HEXDUMP_X_RATELIMIT_RATE`` for rate-limited data dumping with custom rate, e.g. :c:macro:`LOG_HEXDUMP_WRN_RATELIMIT_RATE`.

The convenience macros use the default rate specified by ``CONFIG_LOG_RATELIMIT_INTERVAL_MS``,
while the explicit rate macros take a rate parameter (in milliseconds) that specifies the minimum interval between log messages.

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

Each module which is using the logging must specify its unique name and
register itself to the logging. If module consists of more than one file,
registration is performed in one file but each file must define a module name.

Logger's default frontend is designed to be thread safe and minimizes time needed
to log the message. Time consuming operations like string formatting or access to the
transport are not performed by default when logging API is called. When logging
API is called a message is created and added to the list. Dedicated,
configurable buffer for pool of log messages is used. There are 2 types of messages:
standard and hexdump. Each message contain source ID (module or instance ID and
domain ID which might be used for multiprocessor systems), timestamp and
severity level. Standard message contains pointer to the string and arguments.
Hexdump message contains copied data and string.

.. _logging_kconfig:

Global Kconfig Options
**********************

These options can be found in the following path :zephyr_file:`subsys/logging/Kconfig`.

:kconfig:option:`CONFIG_LOG`: Global switch, turns on/off the logging.

Mode of operations:

:kconfig:option:`CONFIG_LOG_MODE_DEFERRED`: Deferred mode.

:kconfig:option:`CONFIG_LOG_MODE_IMMEDIATE`: Immediate (synchronous) mode.

:kconfig:option:`CONFIG_LOG_MODE_MINIMAL`: Minimal footprint mode.

Filtering options:

:kconfig:option:`CONFIG_LOG_RUNTIME_FILTERING`: Enables runtime reconfiguration of the
filtering.

:kconfig:option:`CONFIG_LOG_DEFAULT_LEVEL`: Default level, sets the logging level
used by modules that are not setting their own logging level.

:kconfig:option:`CONFIG_LOG_OVERRIDE_LEVEL`: It overrides module logging level when
it is not set or set lower than the override value.

:kconfig:option:`CONFIG_LOG_MAX_LEVEL`: Maximal (lowest severity) level which is
compiled in.

Processing options:

:kconfig:option:`CONFIG_LOG_MODE_OVERFLOW`: When new message cannot be allocated,
oldest one are discarded.

:kconfig:option:`CONFIG_LOG_BLOCK_IN_THREAD`: If enabled and new log message cannot
be allocated thread context will block for up to
:kconfig:option:`CONFIG_LOG_BLOCK_IN_THREAD_TIMEOUT_MS` or until log message is
allocated.

:kconfig:option:`CONFIG_LOG_PRINTK`: Redirect printk calls to the logging.

:kconfig:option:`CONFIG_LOG_PROCESS_TRIGGER_THRESHOLD`: When the number of buffered log
messages reaches the threshold, the dedicated thread (see :c:func:`log_thread_set`)
is woken up. If :kconfig:option:`CONFIG_LOG_PROCESS_THREAD` is enabled then this
threshold is used by the internal thread.

:kconfig:option:`CONFIG_LOG_PROCESS_THREAD`: When enabled, logging thread is created
which handles log processing.

:kconfig:option:`CONFIG_LOG_PROCESS_THREAD_STARTUP_DELAY_MS`: Delay in milliseconds
after which logging thread is started.

:kconfig:option:`CONFIG_LOG_BUFFER_SIZE`: Number of bytes dedicated for the circular
packet buffer.

:kconfig:option:`CONFIG_LOG_FRONTEND`: Direct logs to a custom frontend.

:kconfig:option:`CONFIG_LOG_FRONTEND_ONLY`: No backends are used when messages goes to frontend.

:kconfig:option:`CONFIG_LOG_FRONTEND_OPT_API`: Optional API optimized for the most common
simple messages.

:kconfig:option:`CONFIG_LOG_CUSTOM_HEADER`: Injects an application provided header into log.h

:kconfig:option:`CONFIG_LOG_TIMESTAMP_64BIT`: 64 bit timestamp.

:kconfig:option:`CONFIG_LOG_SIMPLE_MSG_OPTIMIZE`: Optimizes simple log messages for size
and performance. Option available only for 32 bit architectures.

Formatting options:

:kconfig:option:`CONFIG_LOG_FUNC_NAME_PREFIX_ERR`: Prepend standard ERROR log messages
with function name. Hexdump messages are not prepended.

:kconfig:option:`CONFIG_LOG_FUNC_NAME_PREFIX_WRN`: Prepend standard WARNING log messages
with function name. Hexdump messages are not prepended.

:kconfig:option:`CONFIG_LOG_FUNC_NAME_PREFIX_INF`: Prepend standard INFO log messages
with function name. Hexdump messages are not prepended.

:kconfig:option:`CONFIG_LOG_FUNC_NAME_PREFIX_DBG`: Prepend standard DEBUG log messages
with function name. Hexdump messages are not prepended.

:kconfig:option:`CONFIG_LOG_BACKEND_SHOW_TIMESTAMP`: Enables backend to print timestamps
with log.

:kconfig:option:`CONFIG_LOG_BACKEND_SHOW_LEVEL`: Enables backend to print levels with log.

:kconfig:option:`CONFIG_LOG_BACKEND_SHOW_COLOR`: Enables coloring of errors (red)
and warnings (yellow).

:kconfig:option:`CONFIG_LOG_BACKEND_FORMAT_TIMESTAMP`: If enabled timestamp is
formatted to *hh:mm:ss:mmm,uuu*. Otherwise is printed in raw format.

Backend options:

:kconfig:option:`CONFIG_LOG_BACKEND_UART`: Enable built-in UART backend.

:kconfig:option:`CONFIG_LOG_BACKEND_NET`: Enable built-in Networking backend to send syslog messages
to a network server.


.. _log_usage:

Usage
*****

Logging in a module
===================

In order to use logging in the module, a unique name of a module must be
specified and module must be registered using :c:macro:`LOG_MODULE_REGISTER`.
Optionally, a compile time log level for the module can be specified as the
second parameter. Default log level (:kconfig:option:`CONFIG_LOG_DEFAULT_LEVEL`) is used
if custom log level is not provided.

.. code-block:: c

   #include <zephyr/logging/log.h>
   LOG_MODULE_REGISTER(foo, CONFIG_FOO_LOG_LEVEL);

If the module consists of multiple files, then ``LOG_MODULE_REGISTER()`` should
appear in exactly one of them. Each other file should use
:c:macro:`LOG_MODULE_DECLARE` to declare its membership in the module.
Optionally, a compile time log level for the module can be specified as
the second parameter. Default log level (:kconfig:option:`CONFIG_LOG_DEFAULT_LEVEL`)
is used if custom log level is not provided.

.. code-block:: c

   #include <zephyr/logging/log.h>
   /* In all files comprising the module but one */
   LOG_MODULE_DECLARE(foo, CONFIG_FOO_LOG_LEVEL);

In order to use logging API in a function implemented in a header file
:c:macro:`LOG_MODULE_DECLARE` macro must be used in the function body
before logging API is called. Optionally, a compile time log level for the module
can be specified as the second parameter. Default log level
(:kconfig:option:`CONFIG_LOG_DEFAULT_LEVEL`) is used if custom log level is not
provided.

.. code-block:: c

   #include <zephyr/logging/log.h>

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
across the system enabling logs will lead to flooding. The logger provides the tools
which can be used to provide filtering on instance level rather than module
level. In that case logging can be enabled for particular instance.

In order to use instance level filtering following steps must be performed:

- a pointer to specific logging structure is declared in instance structure.
  :c:macro:`LOG_INSTANCE_PTR_DECLARE` is used for that.

.. code-block:: c

   #include <zephyr/logging/log_instance.h>

   struct foo_object {
   	LOG_INSTANCE_PTR_DECLARE(log);
   	uint32_t id;
   }

- module must provide macro for instantiation. In that macro, logging instance
  is registered and log instance pointer is initialized in the object structure.

.. code-block:: c

   #define FOO_OBJECT_DEFINE(_name)                             \
   	LOG_INSTANCE_REGISTER(foo, _name, CONFIG_FOO_LOG_LEVEL) \
   	struct foo_object _name = {                             \
   		LOG_INSTANCE_PTR_INIT(log, foo, _name)          \
   	}

Note that when logging is disabled logging instance and pointer to that instance
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

Controlling the logging
=======================

By default, logging processing in deferred mode is handled internally by the
dedicated task which starts automatically. However, it might not be available
if multithreading is disabled. It can also be disabled by unsetting
:kconfig:option:`CONFIG_LOG_PROCESS_TRIGGER_THRESHOLD`. In that case, logging can
be controlled using the API defined in :zephyr_file:`include/zephyr/logging/log_ctrl.h`.
Logging must be initialized before it can be used. Optionally, the user can provide
a function which returns the timestamp value. If not provided, :c:macro:`k_cycle_get`
or :c:macro:`k_cycle_get_32` is used for timestamping.
The :c:func:`log_process` function is used to trigger processing of one log
message (if pending), and returns true if there are more messages pending.
However, it is recommended to use macro wrappers (:c:macro:`LOG_INIT` and
:c:macro:`LOG_PROCESS`) which handle the case where logging is disabled.

The following snippet shows how logging can be processed in simple forever loop.

.. code-block:: c

   #include <zephyr/logging/log_ctrl.h>

   int main(void)
   {
   	LOG_INIT();
   	/* If multithreading is enabled provide thread id to the logging. */
   	log_thread_set(k_current_get());

   	while (1) {
   		if (LOG_PROCESS() == false) {
   			/* sleep */
   		}
   	}
   }

If logs are processed from a thread (user or internal) then it is possible to enable
a feature which will wake up processing thread when certain amount of log messages are
buffered (see :kconfig:option:`CONFIG_LOG_PROCESS_TRIGGER_THRESHOLD`).

.. _logging_ratelimited:

Rate-limited logging
********************

Rate-limited logging macros provide a way to prevent log flooding when messages are
generated frequently. These macros ensure that log messages are not output more
frequently than a specified interval, similar to Linux's ``printk_ratelimited``
functionality.

The rate-limited logging system provides two types of macros:

**Convenience macros (using default rate):**
- :c:macro:`LOG_ERR_RATELIMIT` - Rate-limited error messages
- :c:macro:`LOG_WRN_RATELIMIT` - Rate-limited warning messages
- :c:macro:`LOG_INF_RATELIMIT` - Rate-limited info messages
- :c:macro:`LOG_DBG_RATELIMIT` - Rate-limited debug messages
- :c:macro:`LOG_HEXDUMP_ERR_RATELIMIT` - Rate-limited error hexdump
- :c:macro:`LOG_HEXDUMP_WRN_RATELIMIT` - Rate-limited warning hexdump
- :c:macro:`LOG_HEXDUMP_INF_RATELIMIT` - Rate-limited info hexdump
- :c:macro:`LOG_HEXDUMP_DBG_RATELIMIT` - Rate-limited debug hexdump

**Explicit rate macros (with custom rate):**
- :c:macro:`LOG_ERR_RATELIMIT_RATE` - Rate-limited error messages with custom rate
- :c:macro:`LOG_WRN_RATELIMIT_RATE` - Rate-limited warning messages with custom rate
- :c:macro:`LOG_INF_RATELIMIT_RATE` - Rate-limited info messages with custom rate
- :c:macro:`LOG_DBG_RATELIMIT_RATE` - Rate-limited debug messages with custom rate
- :c:macro:`LOG_HEXDUMP_ERR_RATELIMIT_RATE` - Rate-limited error hexdump with custom rate
- :c:macro:`LOG_HEXDUMP_WRN_RATELIMIT_RATE` - Rate-limited warning hexdump with custom rate
- :c:macro:`LOG_HEXDUMP_INF_RATELIMIT_RATE` - Rate-limited info hexdump with custom rate
- :c:macro:`LOG_HEXDUMP_DBG_RATELIMIT_RATE` - Rate-limited debug hexdump with custom rate

The convenience macros use the default rate specified by :kconfig:option:`CONFIG_LOG_RATELIMIT_INTERVAL_MS`
(5000ms by default). The explicit rate macros take a rate parameter (in milliseconds) that specifies
the minimum interval between log messages. The rate limiting is per-macro-call-site, meaning
that each unique call to a rate-limited macro has its own independent rate limit.

Example usage:

.. code-block:: c

    #include <zephyr/logging/log.h>
    #include <zephyr/kernel.h>

    LOG_MODULE_REGISTER(my_module, CONFIG_LOG_DEFAULT_LEVEL);

    void process_data(void)
    {
        /* Convenience macros using default rate (CONFIG_LOG_RATELIMIT_INTERVAL_MS) */
        LOG_WRN_RATELIMIT("Data processing warning: %d", error_code);
        LOG_ERR_RATELIMIT("Critical error occurred: %s", error_msg);
        LOG_INF_RATELIMIT("Processing status: %d items", item_count);
        LOG_HEXDUMP_WRN_RATELIMIT(data_buffer, data_len, "Data buffer:");

        /* Explicit rate macros with custom intervals */
        LOG_WRN_RATELIMIT_RATE(1000, "Fast rate warning: %d", error_code);
        LOG_ERR_RATELIMIT_RATE(30000, "Slow rate error: %s", error_msg);
        LOG_INF_RATELIMIT_RATE(2000, "Custom rate status: %d items", item_count);
        LOG_HEXDUMP_ERR_RATELIMIT_RATE(5000, data_buffer, data_len, "Error data:");
    }

Rate-limited logging is particularly useful for:

- Error conditions that might occur frequently but don't need to flood the logs
- Status updates in tight loops or high-frequency callbacks
- Debug information that could overwhelm the logging system
- Network or I/O operations that might fail repeatedly

Configuration
==============

Rate-limited logging can be configured using the following Kconfig options:

- :kconfig:option:`CONFIG_LOG_RATELIMIT` - Master switch to enable/disable rate-limited logging
- :kconfig:option:`CONFIG_LOG_RATELIMIT_INTERVAL_MS` - Default interval for convenience macros (5000ms)

When :kconfig:option:`CONFIG_LOG_RATELIMIT` is disabled, the behavior of rate-limited macros is controlled
by the :kconfig:option:`CONFIG_LOG_RATELIMIT_FALLBACK` choice:

- :kconfig:option:`CONFIG_LOG_RATELIMIT_FALLBACK_LOG` - All rate-limited macros behave as regular logging macros
- :kconfig:option:`CONFIG_LOG_RATELIMIT_FALLBACK_DROP` - All rate-limited macros expand to no-ops (default)

This allows you to control whether rate-limited log macros should always print or be completely
suppressed when rate limiting is not available.

The rate limiting is implemented using static variables and :c:func:`k_uptime_get_32`
to track the last log time for each call site.

.. _logging_panic:

Logging panic
*************

In case of error condition system usually can no longer rely on scheduler or
interrupts. In that situation deferred log message processing is not an option.
Logger controlling API provides a function for entering into panic mode
(:c:func:`log_panic`) which should be called in such situation.

When :c:func:`log_panic` is called, _panic_ notification is sent to all active
backends. Once all backends are notified, all buffered messages are flushed. Since
that moment all logs are processed in a blocking way.

.. _logging_printk:

Printk
******

Typically, logging and :c:func:`printk` use the same output, which they compete
for. This can lead to issues if the output does not support preemption but it may
also result in corrupted output because logging data is interleaved with printk
data. However, it is possible to redirect printk messages to the
logging subsystem by enabling :kconfig:option:`CONFIG_LOG_PRINTK`. In that case,
printk entries are treated as log messages with level 0 (they cannot be disabled).
When enabled, logging manages the output so there is no interleaving. However,
in deferred mode the printk behaviour is changed since the output is delayed
until the logging thread processes the data. :kconfig:option:`CONFIG_LOG_PRINTK`
is enabled by default.


.. _log_architecture:

Architecture
************

Logging consists of 3 main parts:

- Frontend
- Core
- Backends

Log message is generated by a source of logging which can be a module or
instance of a module.

Default Frontend
================

Default frontend is engaged when the logging API is called in a source of logging (e.g.
:c:macro:`LOG_INF`) and is responsible for filtering a message (compile and run
time), allocating a buffer for the message, creating the message and committing that
message. Since the logging API can be called in an interrupt, the frontend is optimized
to log the message as fast as possible.

Log message
-----------

A log message contains a message descriptor (source, domain and level), timestamp,
formatted string details (see :ref:`cbprintf_packaging`) and optional data.
Log messages are stored in a continuous block of memory.
Memory is allocated from a circular packet buffer (:ref:`mpsc_pbuf`), which has
a few consequences:

 * Each message is a self-contained, continuous block of memory thus it is suited
   for copying the message (e.g. for offline processing).
 * Messages must be sequentially freed. Backend processing is synchronous. Backend
   can make a copy for deferred processing.

A log message has following format:

+------------------+----------------------------------------------------+
| Message Header   | 2 bits: MPSC packet buffer header                  |
|                  +----------------------------------------------------+
|                  | 1 bit: Trace/Log message flag                      |
|                  +----------------------------------------------------+
|                  | 3 bits: Domain ID                                  |
|                  +----------------------------------------------------+
|                  | 3 bits: Level                                      |
|                  +----------------------------------------------------+
|                  | 10 bits: Cbprintf Package Length                   |
|                  +----------------------------------------------------+
|                  | 12 bits: Data length                               |
|                  +----------------------------------------------------+
|                  | 1 bit: Reserved                                    |
|                  +----------------------------------------------------+
|                  | pointer: Pointer to the source descriptor [#l0]_   |
|                  +----------------------------------------------------+
|                  | 32 or 64 bits: Timestamp [#l0]_                    |
|                  +----------------------------------------------------+
|                  | Optional padding [#l1]_                            |
+------------------+----------------------------------------------------+
| Cbprintf         | Header                                             |
|                  +----------------------------------------------------+
| | package        | Arguments                                          |
| | (optional)     +----------------------------------------------------+
|                  | Appended strings                                   |
+------------------+----------------------------------------------------+
| Hexdump data (optional)                                               |
+------------------+----------------------------------------------------+
| Alignment padding (optional)                                          |
+------------------+----------------------------------------------------+

.. rubric:: Footnotes

.. [#l0] Depending on the platform and the timestamp size fields may be swapped.
.. [#l1] It may be required for cbprintf package alignment

Log message allocation
----------------------

It may happen that the frontend cannot allocate a message. This happens if the
system is generating more log messages than it can process in certain time
frame. There are two strategies to handle that case:

- No overflow - the new log is dropped if space for a message cannot be allocated.
- Overflow - the oldest pending messages are freed, until the new message can be
  allocated. Enabled by :kconfig:option:`CONFIG_LOG_MODE_OVERFLOW`. Note that it degrades
  performance thus it is recommended to adjust buffer size and amount of enabled
  logs to limit dropping.

.. _logging_runtime_filtering:

Run-time filtering
------------------

If run-time filtering is enabled, then for each source of logging a filter
structure in RAM is declared. Such filter is using 32 bits divided into ten 3
bit slots. Except *slot 0*, each slot stores current filter for one backend in
the system. *Slot 0* (bits 0-2) is used to aggregate maximal filter setting for
given source of logging. Aggregate slot determines if log message is created
for given entry since it indicates if there is at least one backend expecting
that log entry. Backend slots are examined when message is processed by the core
to determine if message is accepted by the given backend. Contrary to compile
time filtering, binary footprint is increased because logs are compiled in.

In the example below backend 1 is set to receive errors (*slot 1*) and backend
2 up to info level (*slot 2*). Slots 3-9 are not used. Aggregated filter
(*slot 0*) is set to info level and up to this level message from that
particular source will be buffered.

+------+------+------+------+-----+------+
|slot 0|slot 1|slot 2|slot 3| ... |slot 9|
+------+------+------+------+-----+------+
| INF  | ERR  | INF  | OFF  | ... | OFF  |
+------+------+------+------+-----+------+

.. _log_frontend:

Custom Frontend
===============

Custom frontend is enabled using :kconfig:option:`CONFIG_LOG_FRONTEND`. Logs are directed
to functions declared in :zephyr_file:`include/zephyr/logging/log_frontend.h`.
If option :kconfig:option:`CONFIG_LOG_FRONTEND_ONLY` is enabled then log message is not
created and no backend is handled. Otherwise, custom frontend can coexist with
backends.

In some cases, logs need to be redirected at the macro level. For these cases,
:kconfig:option:`CONFIG_LOG_CUSTOM_HEADER` can be used to inject an application provided
header named :file:`zephyr_custom_log.h` at the end of :zephyr_file:`include/zephyr/logging/log.h`.

Frontend using ARM Coresight STM (System Trace Macrocell)
---------------------------------------------------------

For more details about logging using ARM Coresight STM see :ref:`logging_cs_stm`.

Frontend using IPC service
--------------------------

Frontend which sends log messages to another domain over the IPC service. For more details
see :ref:`logging_multidomain_ipc`.

.. _logging_strings:

Logging strings
===============

String arguments are handled by :ref:`cbprintf_packaging`. See
:ref:`cbprintf_packaging_limitations` for limitations and recommendations.

Multi-domain support
====================

More complex systems can consist of multiple domains where each domain is an
independent binary. Examples of domains are a core in a multicore SoC or one
of the binaries (Secure or Nonsecure) on an ARM TrustZone core.

Tracing and debugging on a multi-domain system is more complex and requires an efficient logging
system. Two approaches can be used to structure this logging system:

* Log inside each domain independently.
  This option is not always possible as it requires that each domain has an available backend
  (for example, UART). This approach can also be troublesome to use and not scalable,
  as logs are presented on independent outputs.
* Use a multi-domain logging system where log messages from each domain end up in one root domain,
  where they are processed exactly as in a single domain case.
  In this approach, log messages are passed between domains over a connection created from
  a backend (or a frontend) on the remote side and a link on the root side.

  The Log link is the interface used on the root side of such a connection. A link is
  responsible for receiving log messages from a remote domain, providing them to the logging
  core (:c:func:`log_link_get_msg`, :c:func:`log_link_put_msg`) and handling logger control
  requests like configuring filtering or getting log source names. How received messages are
  stored is up to the link implementation. A link implementation matches the complementary
  backend or frontend implementation on the remote side.

Multi-domain support in the root domain is enabled with
:kconfig:option:`CONFIG_LOG_MULTIDOMAIN`.

There are two types of domains in a multi-domain system:

* The *remote domain* has the logging core implementation and a cross-domain backend or
  frontend. When a cross-domain backend is used, the domain can also have other backends
  in parallel, for example a local UART backend.
* The *root domain* has one link per remote domain and backends that output logs to the user.
  Messages received over a link are processed like locally created messages.

Each link handles exactly one remote domain and messages are not relayed through intermediate
domains, so every remote domain is connected directly to the root domain.

See the following image for an example of a multi-domain setup:

.. figure:: images/multidomain.svg

    Multi-domain example

Domain ID
---------

The source of each log message can be identified by the following fields in the header:
``source_id`` and ``domain_id``.

The value assigned to the ``domain_id`` is relative. Whenever a domain creates a log message,
it sets its ``domain_id`` to ``0``. The message keeps that value while it is transferred to the
root domain. When the root domain takes the message from a link, it adds the link domain offset
to the ``domain_id``.

Domain offsets are assigned when links are activated, in the order in which links are registered.
The first link has the offset set to ``1`` and each following link gets the next value. Since a
link handles a single domain, the ``domain_id`` of a remote domain equals the index of its link
incremented by one.

The name reported for a domain is :kconfig:option:`CONFIG_LOG_DOMAIN_NAME` for the local domain
and the link name for a remote domain. Domain names are added to the message prefix when
:kconfig:option:`CONFIG_LOG_DOMAIN_NAME_PREFIX` is enabled, which is implied by
:kconfig:option:`CONFIG_LOG_MULTIDOMAIN`.

Cross-domain log message
------------------------

In most cases, the address space of each domain is unique, and one domain
cannot access directly the data in another domain. For this reason, the backend can
partially process the message before it is passed to another domain. Partial
processing can include converting a string package to a *fully self-contained*
version (copying read-only strings to the package body).

If the root domain can access the memory of the remote domain, that conversion is not needed.
:kconfig:option:`CONFIG_LOG_BACKEND_IPC_SERVICE_STRING_ACCESS` and
:kconfig:option:`CONFIG_LOG_FRONTEND_IPC_STRING_ACCESS` are used in the remote domain to
indicate that read-only strings do not need to be included in the log message, which reduces
the processing time and the message size. The remote domain then also shares the addresses of
its log source table and its read-only log strings, so the root domain reads source names
directly instead of requesting them over the cross-domain connection.

Each domain can have a different timestamp source in terms of frequency and
offset. Logging does not perform any timestamp conversion.

Runtime filtering
-----------------

In the single-domain case, each log source has a dedicated variable with runtime
filtering for each backend in the system. In the multi-domain case, the originator of
the log message is not aware of the number of backends in the root domain.

As such, to filter logs in multiple domains, each source requires a runtime
filtering setting in the remote domain and in the root domain. As the number of
sources in other domains is not known during the compilation, the runtime filtering
of remote sources must use dynamically allocated memory (one word per
source). When a backend in the root domain changes the filtering of the module from a
remote domain, the local filter is updated. After the update, the aggregated
filter (the maximum from all the local backends) is checked and, if changed, the remote domain is
informed about this change. With this approach, the runtime filtering works identically
in both multi-domain and single-domain scenarios.

Message ordering
----------------

Logging does not provide any mechanism for synchronizing timestamps across multiple
domains. When picking the next message to process, the root domain compares the oldest message
in the local buffer with the oldest message provided by each link and processes the one with
the earliest timestamp. Ordering is therefore maintained only if all domains share the same
timestamp source or if there is an out-of-bound mechanism that recalculates timestamps.

Since a remote message arrives with a delay, processing is deferred by
:kconfig:option:`CONFIG_LOG_PROCESSING_LATENCY_US` to give remote messages a chance to arrive
before an already available local message is processed. A higher value increases the chances
of maintaining correct ordering at the cost of increased processing latency.

If a message with a timestamp older than the previously processed one is still found, the
message is marked as unordered. Backends which use :ref:`log_output` for formatting prefix such
message with ``<!>``.

.. _logging_multidomain_ipc:

Logging over IPC service
------------------------

Zephyr provides a cross-domain link, backend and frontend implemented on top of the
:ref:`ipc_service`. They exchange log messages and control requests over an endpoint named
``logging``. The IPC instance used for logging is taken from the ``zephyr,log-ipc`` chosen node.
If that node is not present, the only IPC instance found in the devicetree is used. Instances of
the link, the backend and the frontend are created by the logging subsystem, so no application
code is needed.

In the root domain, :kconfig:option:`CONFIG_LOG_LINK_IPC_SERVICE` adds a link named ``cpu1``.
Messages received from the remote domain are stored in one of two ways:

* If :kconfig:option:`CONFIG_LOG_LINK_IPC_SERVICE_RX_HOLD` is enabled (default when the IPC
  service backend supports holding of RX buffers), messages are processed in place in the IPC
  RX buffers, which are released once all the messages they contain are processed. Pointers to
  the held buffers are kept in a queue of
  :kconfig:option:`CONFIG_LOG_LINK_IPC_SERVICE_RX_HOLD_QUEUE_SIZE` entries. The queue should not
  be too long because holding RX buffers may block other endpoints of the IPC instance.
* Otherwise, messages are copied into a dedicated packet buffer of
  :kconfig:option:`CONFIG_LOG_LINK_IPC_SERVICE_BUFFER_SIZE` bytes.

In the remote domain, messages can be sent to the root domain either by a backend or by
a frontend:

* :kconfig:option:`CONFIG_LOG_BACKEND_IPC_SERVICE` sends messages when they are processed by
  the logging core, which in deferred mode is the log processing thread. If the IPC service
  backend supports the zero copy API, the outgoing message is created directly in the IPC TX
  buffer (:kconfig:option:`CONFIG_LOG_BACKEND_IPC_SERVICE_ZERO_COPY`).
* :kconfig:option:`CONFIG_LOG_FRONTEND_IPC` sends messages in the context of the logging API
  call, which means that messages are not buffered locally and the log processing thread is not
  used. Multiple messages are packed into a single IPC packet of
  :kconfig:option:`CONFIG_LOG_FRONTEND_IPC_PACKET_SIZE` bytes, which is sent when it is full or
  when :kconfig:option:`CONFIG_LOG_FRONTEND_IPC_MSG_TIMEOUT` expires. Messages created before
  the endpoint is bound are kept in a buffer of
  :kconfig:option:`CONFIG_LOG_FRONTEND_IPC_EARLY_BUFFER_SIZE` bytes. The frontend can only be
  used with an IPC service backend which supports the zero copy API and sending from any
  context with interrupts locked. If that is not the case, use the logging backend.

Both of them can skip copying read-only strings into the transferred message when the root
domain can access the memory of the remote domain
(:kconfig:option:`CONFIG_LOG_BACKEND_IPC_SERVICE_STRING_ACCESS` and
:kconfig:option:`CONFIG_LOG_FRONTEND_IPC_STRING_ACCESS`, see `Cross-domain log message`_).
Both of them also count messages that could not be sent and report the number to the root
domain, where it is printed as a warning.

See the :zephyr:code-sample:`logging_multidomain` sample for a complete setup.

Logging backends
================

Logging backends are registered using :c:macro:`LOG_BACKEND_DEFINE`. The macro
creates an instance in the dedicated memory section. Backends can be dynamically
enabled (:c:func:`log_backend_enable`) and disabled. When
:ref:`logging_runtime_filtering` is enabled, :c:func:`log_filter_set` can be used
to dynamically change filtering of a module logs for given backend. Module is
identified by source ID and domain ID. Source ID can be retrieved if source name
is known by iterating through all registered sources.

Logging supports up to 9 concurrent backends. Log message is passed to the
each backend in processing phase. Additionally, backend is notified when logging
enter panic mode with :c:func:`log_backend_panic`. On that call backend should
switch to synchronous, interrupt-less operation or shut down itself if that is
not supported.  Occasionally, logging may inform backend about number of dropped
messages with :c:func:`log_backend_dropped`. Message processing API is version
specific.

:c:func:`log_backend_msg_process` is used for processing message. It is common for
standard and hexdump messages because log message hold string with arguments
and data. It is also common for deferred and immediate logging.

.. _log_output:

Message formatting
------------------

Logging provides set of function that can be used by the backend to format a
message. Helper functions are available in :zephyr_file:`include/zephyr/logging/log_output.h`.

Example message formatted using :c:func:`log_output_msg_process`.

.. code-block:: console

   [00:00:00.000,274] <info> sample_instance.inst1: logging message


.. _logging_guide_dictionary:

Dictionary-based Logging
========================

Dictionary-based logging, instead of human readable texts, outputs the log
messages in binary format. This binary format encodes arguments to formatted
strings in their native storage formats which can be more compact than their
text equivalents. For statically defined strings (including the format
strings and any string arguments), references to the ELF file are encoded
instead of the whole strings. A dictionary created at build time contains
the mappings between these references and the actual strings. This allows
the offline parser to obtain the strings from the dictionary to parse
the log messages. This binary format allows a more compact representation
of log messages in certain scenarios. However, this requires the use of
an offline parser and is not as intuitive to use as text-based log messages.

Note that ``long double`` is not supported by Python's ``struct`` module.
Therefore, log messages with ``long double`` will not display the correct
values.


Configuration
-------------

Here are kconfig options related to dictionary-based logging:

- :kconfig:option:`CONFIG_LOG_DICTIONARY_SUPPORT` enables dictionary-based logging
  support. This should be selected by the backends which require it.

- The UART backend can be used for dictionary-based logging. These are
  additional config for the UART backend:

  - :kconfig:option:`CONFIG_LOG_BACKEND_UART_OUTPUT_DICTIONARY_HEX` tells
    the UART backend to output hexadecimal characters for dictionary based
    logging. This is useful when the log data needs to be captured manually
    via terminals and consoles.

  - :kconfig:option:`CONFIG_LOG_BACKEND_UART_OUTPUT_DICTIONARY_BIN` tells
    the UART backend to output binary data.

- The RTT backend can also be used for dictionary-based logging:

  - :kconfig:option:`CONFIG_LOG_BACKEND_RTT` enables the RTT backend.

  - :kconfig:option:`CONFIG_LOG_BACKEND_RTT_OUTPUT_DICTIONARY` enables
    dictionary-based output for the RTT backend. Use together with
    :kconfig:option:`CONFIG_USE_SEGGER_RTT`.

  - :kconfig:option:`CONFIG_LOG_BACKEND_RTT_OUTPUT_DICTIONARY_HEX` tells
    the RTT backend to output hexadecimal characters for dictionary based
    logging.


Usage
-----

When dictionary-based logging is enabled via enabling related logging backends,
a JSON database file, named :file:`log_dictionary.json`, will be created
in the build directory. This database file contains information for the parser
to correctly parse the log data. Note that this database file only works
with the same build, and cannot be used for any other builds.

Offline Parsing
^^^^^^^^^^^^^^^

To parse a previously captured log file:

.. code-block:: console

  ./scripts/logging/dictionary/log_parser.py <build dir>/log_dictionary.json <log data file>

The parser takes two required arguments, where the first one is the full path
to the JSON database file, and the second part is the file containing log data.
Add an optional argument ``--hex`` to the end if the log data file contains
hexadecimal characters
(e.g. when ``CONFIG_LOG_BACKEND_UART_OUTPUT_DICTIONARY_HEX=y``). This tells
the parser to convert the hexadecimal characters to binary before parsing.

Live Parsing
^^^^^^^^^^^^

For real-time decoding of dictionary-based log output, use the live log parser.
It connects to a running device and continuously decodes binary log data as it
arrives. Note that the live parser only supports binary dictionary output
(not hex-encoded). The live parser supports three input modes:

**Serial (UART):**

.. code-block:: console

  ./scripts/logging/dictionary/live_log_parser.py <build dir>/log_dictionary.json serial <port> <baudrate>

For example, to read from ``/dev/ttyACM0`` at 115200 baud:

.. code-block:: console

  ./scripts/logging/dictionary/live_log_parser.py build/zephyr/log_dictionary.json serial /dev/ttyACM0 115200

**JLink RTT:**

.. code-block:: console

  ./scripts/logging/dictionary/live_log_parser.py <build dir>/log_dictionary.json jlink-rtt <device_name>

For example, to read RTT output from an nRF5340:

.. code-block:: console

  ./scripts/logging/dictionary/live_log_parser.py build/zephyr/log_dictionary.json jlink-rtt nrf5340_xxaa_app

The JLink RTT mode requires the ``pylink-square`` Python package
(``pip install pylink-square``). Optional arguments include ``--channel`` to
select the RTT channel (default: 0), ``--speed`` to set the connection speed,
and ``--block-address`` to specify the RTT control block address in hex.

**File / stdin:**

.. code-block:: console

  ./scripts/logging/dictionary/live_log_parser.py <build dir>/log_dictionary.json file <filepath>

When ``<filepath>`` is omitted, the parser reads from stdin, which allows piping
binary data directly into it.

Please refer to the :zephyr:code-sample:`logging-dictionary` sample for more
examples on using the log parsers.


Recommendations and limitations
*******************************

The are following recommendations:

* Enable :kconfig:option:`CONFIG_LOG_SPEED` to slightly speed up deferred logging at the
  cost of slight increase in memory footprint.
* Compiler with C11 ``_Generic`` keyword support is recommended. Logging
  performance is significantly degraded without it. See :ref:`cbprintf_packaging`.
* It is recommended to cast pointer to ``const char *`` when it is used with ``%s``
  format specifier and it points to a constant string.
* It is recommended to cast pointer to ``char *`` when it is used with ``%s``
  format specifier and it points to a transient string.
* It is required to cast a character pointer to non character pointer
  (e.g., ``void *``) when it is used with ``%p`` format specifier.

.. code-block:: c

   LOG_WRN("%s", str);
   LOG_WRN("%p", (void *)str);

There are following limitations:

* Logging does not support string format specifier with width (e.g., ``%.*s`` or ``%8s``). That
  is because format string content is not used to build a log message, only argument types.
* If deferred logging is used and log messages are prefixed with the thread name
  (Kconfig option ``CONFIG_LOG_THREAD_ID_PREFIX=y`` and ``CONFIG_THREAD_NAME=y``), it is assumed that
  the corresponding :c:struct:`k_thread` structure is still valid when the log message is
  formatted. This can be an issue when that structure is allocated dynamically, using :c:func:`k_malloc` or
  :c:func:`malloc` for instance. In this case, if the thread logs some messages and then gets
  stopped and its ``struct k_thread`` is freed, the log system will still try to access that
  structure when handling the message later. This creates a use-after-free scenario.
  To avoid this, a solution consists in calling :c:func:`log_flush` before freeing the structure.

.. code-block:: c

   struct k_thread *thread = k_malloc(sizeof(*thread)); /* struct allocated dynamically */
   k_thread_create(thread, ...);
   k_thread_name_set(thread, "foobar");

   /* Thread calls LOG_*(...) */

   k_thread_join(thread, K_FOREVER);
   log_flush();  /* flush log buffer before freeing the struct k_thread */
   k_free(thread); /* avoid a potential use-after-free scenario if deferred logging is used */

Benchmark
*********

Benchmark numbers from :zephyr_file:`tests/subsys/logging/log_benchmark` performed
on ``qemu_x86``. It is a rough comparison to give a general overview.

+--------------------------------------------+------------------+
| Feature                                    |                  |
+============================================+==================+
| Kernel logging                             | 7us [#f0]_/11us  |
|                                            |                  |
+--------------------------------------------+------------------+
| User logging                               | 13us             |
|                                            |                  |
+--------------------------------------------+------------------+
| kernel logging with overwrite              | 10us [#f0]_/15us |
+--------------------------------------------+------------------+
| Logging transient string                   | 42us             |
+--------------------------------------------+------------------+
| Logging transient string from user         | 50us             |
+--------------------------------------------+------------------+
| Memory utilization [#f1]_                  | 518              |
|                                            |                  |
+--------------------------------------------+------------------+
| Memory footprint (test) [#f2]_             | 2k               |
+--------------------------------------------+------------------+
| Memory footprint (application) [#f3]_      | 3.5k             |
+--------------------------------------------+------------------+
| Message footprint [#f4]_                   | 47 [#f0]_/32     |
|                                            | bytes            |
+--------------------------------------------+------------------+

.. rubric:: Benchmark details

.. [#f0] :kconfig:option:`CONFIG_LOG_SPEED` enabled.

.. [#f1] Number of log messages with various number of arguments that fits in 2048
  bytes dedicated for logging.

.. [#f2] Logging subsystem memory footprint in :zephyr_file:`tests/subsys/logging/log_benchmark`
  where filtering and formatting features are not used.

.. [#f3] Logging subsystem memory footprint in :zephyr_file:`samples/subsys/logging/logger`.

.. [#f4] Average size of a log message (excluding string) with 2 arguments on ``Cortex M3``

Stack usage
***********

When logging is enabled it impacts stack usage of the context that uses logging API. If stack
is optimized it may lead to stack overflow. Stack usage depends on mode and optimization. It
also significantly varies between platforms. In general, when :kconfig:option:`CONFIG_LOG_MODE_DEFERRED`
is used stack usage is smaller since logging is limited to creating and storing log message.
When :kconfig:option:`CONFIG_LOG_MODE_IMMEDIATE` is used then log message is processed by the backend
which includes string formatting. In case of that mode, stack usage will depend on which backends
are used.

Some of the platforms characterization for log message with two ``integer`` arguments listed below:

+---------------+----------+----------------------------+-----------+-----------------------------+
| Platform      | Deferred | Deferred (no optimization) | Immediate | Immediate (no optimization) |
+===============+==========+============================+===========+=============================+
| ARM Cortex-M3 | 40       | 152                        | 412       | 783                         |
+---------------+----------+----------------------------+-----------+-----------------------------+
| x86           | 12       | 224                        | 388       | 796                         |
+---------------+----------+----------------------------+-----------+-----------------------------+
| riscv32       | 24       | 208                        | 456       | 844                         |
+---------------+----------+----------------------------+-----------+-----------------------------+
| xtensa        | 72       | 336                        | 504       | 944                         |
+---------------+----------+----------------------------+-----------+-----------------------------+
| x86_64        | 32       | 528                        | 1088      | 1440                        |
+---------------+----------+----------------------------+-----------+-----------------------------+

Logging using ARM Coresight STM
*******************************

For logging on NRF54H20 using ARM Coresight STM see :ref:`logging_cs_stm`.

API Reference
*************

Logger API
==========

.. doxygengroup:: log_api

Logger control
==============

.. doxygengroup:: log_ctrl

Log message
===========

.. doxygengroup:: log_msg

Logger backend interface
========================

.. doxygengroup:: log_backend

Log link interface
==================

.. doxygengroup:: log_link

Logger output formatting
========================

.. doxygengroup:: log_output

.. toctree::
   :maxdepth: 1

   cs_stm.rst
