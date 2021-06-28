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
Default configuration uses built-in frontend and UART backend.

Summary of the logging features:

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
- Panic support - in panic mode logging switches to blocking, synchronous
  processing.
- Printk support - printk message can be redirected to the logging.
- Design ready for multi-domain/multi-processor system.

Logging v2 introduces following changes:

- Option to use 64 bit timestamp
- Support for logging floating point variables
- Support for logging variables extending size of a machine word (64 bit values
  on 32 bit architectures)
- Remove the need for special treatment of ``%s`` format specifier
- Extend API for dumping data to accept formatted string
- Improve memory utilization. More log messages fit in the logging buffer in
  deferred mode.
- Log message is no longer fragmented. It is self-contained block of memory which
  simplifies out of domain handling (e.g. offline processing)
- Improved performance when logging from user space
- Improved performance when logging to full buffer and message are dropped.
- Slightly degrade performance in normal circumstances due to the fact that
  allocation from ring buffer is more complex than from memslab.
- No change in logging API
- Logging backend API exteded with function for processing v2 messages.

Logging API is highly configurable at compile time as well as at run time. Using
Kconfig options (see :ref:`logging_kconfig`) logs can be gradually removed from
compilation to reduce image size and execution time when logs are not needed.
During compilation logs can be filtered out on module basis and severity level.

Logs can also be compiled in but filtered on run time using dedicate API. Run
time filtering is independent for each backend and each source of log messages.
Source of log messages can be a module or specific instance of the module.

There are four severity levels available in the system: error, warning, info
and debug. For each severity level the logging API (:zephyr_file:`include/logging/log.h`)
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

:kconfig:`CONFIG_LOG`: Global switch, turns on/off the logging.

Mode of operations:

:kconfig:`CONFIG_LOG_MODE_DEFERRED`: Deferred mode.

:kconfig:`CONFIG_LOG2_MODE_DEFERRED`: Deferred mode v2.

:kconfig:`CONFIG_LOG_MODE_IMMEDIATE`: Immediate (synchronous) mode.

:kconfig:`CONFIG_LOG2_MODE_IMMEDIATE`: Immediate (synchronous) mode v2.

:kconfig:`CONFIG_LOG_MODE_MINIMAL`: Minimal footprint mode.

Filtering options:

:kconfig:`CONFIG_LOG_RUNTIME_FILTERING`: Enables runtime reconfiguration of the
filtering.

:kconfig:`CONFIG_LOG_DEFAULT_LEVEL`: Default level, sets the logging level
used by modules that are not setting their own logging level.

:kconfig:`CONFIG_LOG_OVERRIDE_LEVEL`: It overrides module logging level when
it is not set or set lower than the override value.

:kconfig:`CONFIG_LOG_MAX_LEVEL`: Maximal (lowest severity) level which is
compiled in.

Processing options:

:kconfig:`CONFIG_LOG_MODE_OVERFLOW`: When new message cannot be allocated,
oldest one are discarded.

:kconfig:`CONFIG_LOG_BLOCK_IN_THREAD`: If enabled and new log message cannot
be allocated thread context will block for up to
:kconfig:`CONFIG_LOG_BLOCK_IN_THREAD_TIMEOUT_MS` or until log message is
allocated.

:kconfig:`CONFIG_LOG_PRINTK`: Redirect printk calls to the logging.

:kconfig:`CONFIG_LOG_PRINTK_MAX_STRING_LENGTH`: Maximal string length that can
be processed by printk. Longer strings are trimmed.

:kconfig:`CONFIG_LOG_PROCESS_TRIGGER_THRESHOLD`: When number of buffered log
messages reaches the threshold dedicated thread (see :c:func:`log_thread_set`)
is waken up. If :kconfig:`CONFIG_LOG_PROCESS_THREAD` is enabled then this
threshold is used by the internal thread.

:kconfig:`CONFIG_LOG_PROCESS_THREAD`: When enabled, logging thread is created
which handles log processing.

:kconfig:`CONFIG_LOG_BUFFER_SIZE`: Number of bytes dedicated for the message pool.
Single message capable of storing standard log with up to 3 arguments or hexdump
message with 12 bytes of data take 32 bytes. In v2 it indicates buffer size
dedicated for circular packet buffer.

:kconfig:`CONFIG_LOG_DETECT_MISSED_STRDUP`: Enable detection of missed transient
strings handling.

:kconfig:`CONFIG_LOG_STRDUP_MAX_STRING`: Longest string that can be duplicated
using log_strdup().

:kconfig:`CONFIG_LOG_STRDUP_BUF_COUNT`: Number of buffers in the pool used by
log_strdup().

:kconfig:`CONFIG_LOG_DOMAIN_ID`: Domain ID. Valid in multi-domain systems.

:kconfig:`CONFIG_LOG_FRONTEND`: Redirect logs to a custom frontend.

:kconfig:`CONFIG_LOG_TIMESTAMP_64BIT`: 64 bit timestamp.

Formatting options:

:kconfig:`CONFIG_LOG_FUNC_NAME_PREFIX_ERR`: Prepend standard ERROR log messages
with function name. Hexdump messages are not prepended.

:kconfig:`CONFIG_LOG_FUNC_NAME_PREFIX_WRN`: Prepend standard WARNING log messages
with function name. Hexdump messages are not prepended.

:kconfig:`CONFIG_LOG_FUNC_NAME_PREFIX_INF`: Prepend standard INFO log messages
with function name. Hexdump messages are not prepended.

:kconfig:`CONFIG_LOG_FUNC_NAME_PREFIX_DBG`: Prepend standard DEBUG log messages
with function name. Hexdump messages are not prepended.

:kconfig:`CONFIG_LOG_BACKEND_SHOW_COLOR`: Enables coloring of errors (red)
and warnings (yellow).

:kconfig:`CONFIG_LOG_BACKEND_FORMAT_TIMESTAMP`: If enabled timestamp is
formatted to *hh:mm:ss:mmm,uuu*. Otherwise is printed in raw format.

Backend options:

:kconfig:`CONFIG_LOG_BACKEND_UART`: Enabled build-in UART backend.

.. _log_usage:

Usage
*****

Logging in a module
===================

In order to use logging in the module, a unique name of a module must be
specified and module must be registered using :c:macro:`LOG_MODULE_REGISTER`.
Optionally, a compile time log level for the module can be specified as the
second parameter. Default log level (:kconfig:`CONFIG_LOG_DEFAULT_LEVEL`) is used
if custom log level is not provided.

.. code-block:: c

   #include <logging/log.h>
   LOG_MODULE_REGISTER(foo, CONFIG_FOO_LOG_LEVEL);

If the module consists of multiple files, then ``LOG_MODULE_REGISTER()`` should
appear in exactly one of them. Each other file should use
:c:macro:`LOG_MODULE_DECLARE` to declare its membership in the module.
Optionally, a compile time log level for the module can be specified as
the second parameter. Default log level (:kconfig:`CONFIG_LOG_DEFAULT_LEVEL`)
is used if custom log level is not provided.

.. code-block:: c

   #include <logging/log.h>
   /* In all files comprising the module but one */
   LOG_MODULE_DECLARE(foo, CONFIG_FOO_LOG_LEVEL);

In order to use logging API in a function implemented in a header file
:c:macro:`LOG_MODULE_DECLARE` macro must be used in the function body
before logging API is called. Optionally, a compile time log level for the module
can be specified as the second parameter. Default log level
(:kconfig:`CONFIG_LOG_DEFAULT_LEVEL`) is used if custom log level is not
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

- a pointer to specific logging structure is declared in instance structure.
  :c:macro:`LOG_INSTANCE_PTR_DECLARE` is used for that.

.. code-block:: c

   #include <logging/log_instance.h>

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

Logging can be controlled using API defined in
:zephyr_file:`include/logging/log_ctrl.h`. Logger must be initialized before it can be
used. Optionally, user can provide function which returns timestamp value. If
not provided, :c:macro:`k_cycle_get_32` is used for timestamping.
:c:func:`log_process` function is used to trigger processing of one log
message (if pending). Function returns true if there is more messages pending.

Following snippet shows how logging can be processed in simple forever loop.

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

If logs are processed from a thread then it is possible to enable a feature
which will wake up processing thread when certain amount of log messages are
buffered (see :kconfig:`CONFIG_LOG_PROCESS_TRIGGER_THRESHOLD`). It is also
possible to enable internal logging thread (see :kconfig:`CONFIG_LOG_PROCESS_THREAD`).
In that case, logging thread is initialized and log messages are processed implicitly.

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

Default frontend is engaged when logging API is called in a source of logging (e.g.
:c:macro:`LOG_INF`) and is responsible for filtering a message (compile and run
time), allocating buffer for the message, creating the message and committing that
message. Since logging API can be called in an interrupt, frontend is optimized
to log the message as fast as possible.

Log message v1
--------------

Each log message consists of one or more fixed size chunks allocated from the
pool of fixed size buffers (:ref:`memory_slabs_v2`). Message head chunk
contains log entry details like: source ID, timestamp, severity level and the
data (string pointer and arguments or raw data). Message contains also a
reference counter which indicates how many users still uses this message. It is
used to return message to the pool once last user indicates that it can be
freed. If more than 3 arguments or 12 bytes of raw data is used in the log then
log message is formed from multiple chunks which are linked together. When
message body is filled it is put into the list.
When log processing is triggered, a message is removed from the list of pending
messages. If runtime filtering is disabled, the message is passed to all
active backends, otherwise the message is passed to only those backends that
have requested messages from that particular source (based on the source ID in
the message), and severity level. Once all backends are iterated, the message
is considered processed, but the message may still be in use by a backend.
Because message is allocated from a pool, it is not mandatory to sequentially
free messages. Processing by the backends is asynchronous and memory is freed
when last user indicates that message can be freed. It also means that improper
backend implementation may lead to pool drought.

Log message v2
--------------

Log message v2 contains message descriptor (source, domain and level), timestamp,
formatted string details (see :ref:`cbprintf_packaging`) and optional data.
Log messages v2 are stored in a continuous block of memory (contrary to v1).
Memory is allocated from a circular packet buffer (:ref:`mpsc_pbuf`). It has
few consequences:

 * Each message is self-contained, continuous block of memory thus it is suited
   for copying the message (e.g. for offline processing).
 * Memory is better utilized because fixed size chunks are not used.
 * Messages must be sequentially freed. Backend processing is synchronous. Backend
   can make a copy for deferred processing.

Log message has following format:

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

It may happen that frontend cannot allocate a message. It happens if system is
generating more log messages than it can process in certain time frame. There
are two strategies to handle that case:

- No overflow - new log is dropped if space for a message cannot be allocated.
- Overflow - oldest pending messages are freed, until new message can be
  allocated. Enabled by :kconfig:`CONFIG_LOG_MODE_OVERFLOW`. Note that it degrades
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

Custom Frontend
===============

Custom frontend is enabled using :kconfig:`CONFIG_LOG_FRONTEND`. Logs are redirected
to functions declared in :zephyr_file:`include/logging/log_frontend.h`.
This may be required in very time-sensitive cases, but most of the logging
features cannot be used then, which includes default frontend, core and all
backends features.

.. _logging_strings:

Logging strings
===============

Logging v1
----------

Since log message contains only the value of the argument, when ``%s`` is used
only the address of a string is stored. Because a string variable argument could
be transient, allocated on the stack, or modifiable, logger provides a mechanism
and a dedicated buffer pool to hold copies of strings. The buffer size and count
is configurable (see :kconfig:`CONFIG_LOG_STRDUP_MAX_STRING` and
:kconfig:`CONFIG_LOG_STRDUP_BUF_COUNT`).

If a string argument is transient, the user must call :c:func:`log_strdup` to
duplicate the passed string into a buffer from the pool. See the examples below.
If a strdup buffer cannot be allocated, a warning message is logged and an error
code returned indicating :kconfig:`CONFIG_LOG_STRDUP_BUF_COUNT` should be
increased. Buffers are freed together with the log message.

.. code-block:: c

   char local_str[] = "abc";

   LOG_INF("logging transient string: %s", log_strdup(local_str));
   local_str[0] = '\0'; /* String can be modified, logger will use duplicate."

When :kconfig:`CONFIG_LOG_DETECT_MISSED_STRDUP` is enabled logger will scan
each log message and report if string format specifier is found and string
address is not in read only memory section or does not belong to memory pool
dedicated to string duplicates. It indictes that :c:func:`log_strdup` is
missing in a call to log a message, such as ``LOG_INF``.

Logging v2
----------

String arguments are handled by :ref:`cbprintf_packaging` thus no special action
is required.

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
each backend in processing phase. Additionally, backend is notfied when logging
enter panic mode with :c:func:`log_backend_panic`. On that call backend should
switch to synchronous, interrupt-less operation or shut down itself if that is
not supported.  Occasionally, logging may inform backend about number of dropped
messages with :c:func:`log_backend_dropped`. Message processing API is version
specific.

Logging v1
----------

Logging backend interface contains following functions for processing:

- :c:func:`log_backend_put` - backend gets log message in deferred mode.
- :c:func:`log_backend_put_sync_string` - backend gets log message with formatted
  string message in the immediate mode.
- :c:func:`log_backend_put_sync_hexdump` - backend gets log message with hexdump
  message in the immediate mode.

The log message contains a reference counter tracking how many backends are
processing the message. On receiving a message backend must claim it by calling
:c:func:`log_msg_get` on that message which increments a reference counter.
Once message is processed, backend puts back the message
(:c:func:`log_msg_put`) decrementing a reference counter. On last
:c:func:`log_msg_put`, when reference counter reaches 0, message is returned
to the pool. It is up to the backend how message is processed.

.. note::

   The message pool can be starved if a backend does not call
   :c:func:`log_msg_put` when it is done processing a message. The logging
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

Logging v2
----------

:c:func:`log_backend_msg2_process` is used for processing message. It is common for
standard and hexdump messages because log message v2 hold string with arguments
and data. It is also common for deferred and immediate logging.

Message formatting
------------------

Logging provides set of function that can be used by the backend to format a
message. Helper functions are available in :zephyr_file:`include/logging/log_output.h`.

Example message formatted using :c:func:`log_output_msg_process` or
:c:func:`log_output_msg2_process`.

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

- :kconfig:`CONFIG_LOG_DICTIONARY_SUPPORT` enables dictionary-based logging
  support. This should be selected by the backends which require it.

- The UART backend can be used for dictionary-based logging. These are
  additional config for the UART backend:

  - :kconfig:`CONFIG_LOG_BACKEND_UART_OUTPUT_DICTIONARY_HEX` tells
    the UART backend to output hexadecimal characters for dictionary based
    logging. This is useful when the log data needs to be captured manually
    via terminals and consoles.

  - :kconfig:`CONFIG_LOG_BACKEND_UART_OUTPUT_DICTIONARY_BIN` tells
    the UART backend to output binary data.


Usage
-----

When dictionary-based logging is enabled via enabling related logging backends,
a JSON database file, named :file:`log_dictionary.json`, will be created
in the build directory. This database file contains information for the parser
to correctly parse the log data. Note that this database file only works
with the same build, and cannot be used for any other builds.

To use the log parser:

.. code-block:: console

  ./scripts/logging/dictionary/log_parser.py <build dir>/log_dictionary.json <log data file>

The parser takes two required arguments, where the first one is the full path
to the JSON database file, and the second part is the file containing log data.
Add an optional argument ``--hex`` to the end if the log data file contains
hexadecimal characters
(e.g. when ``CONFIG_LOG_BACKEND_UART_OUTPUT_DICTIONARY_HEX=y``). This tells
the parser to convert the hexadecimal characters to binary before parsing.

Please refer to :ref:`logging_dictionary_sample` on how to use the log parser.



Limitations and recommendations
*******************************

Logging v1
==========

The are following limitations:

* Strings as arguments (*%s*) require special treatment (see
  :ref:`logging_strings`).
* Logging double and float variables is not possible because arguments are
  word size.
* Variables larger than word size cannot be logged.
* Number of arguments in the string is limited to 15.

Logging v2
==========

Solves major limitations of v1. However, in order to get most of the logging
capabilities following recommendations shall be followed:

* Enable :kconfig:`CONFIG_LOG_SPEED` to slightly speed up deferred logging at the
  cost of slight increase in memory footprint.
* Compiler with C11 ``_Generic`` keyword support is recommended. Logging
  performance is significantly degraded without it. See :ref:`cbprintf_packaging`.
* When ``_Generic`` is supported, during compilation it is determined which
  packaging method shall be used: static or runtime. It is done by searching for
  any string pointers in the argument list. If string pointer is used with
  format specifier other than string, e.g. ``%p``, it is recommended to cast it
  to ``void *``.

.. code-block:: c

   LOG_WRN("%s", str);
   LOG_WRN("%p", (void *)str);

Benchmark
*********

Benchmark numbers from :zephyr_file:`tests/subsys/logging/log_benchmark` performed
on ``qemu_x86``. It is a rough comparison to give general overview. Overall,
logging v2 improves in most a the areas with the biggest improvement in logging
from userspace. It is at the cost of larger memory footprint for a log message.

+--------------------------------------------+----------------+------------------+----------------+
| Feature                                    | v1             | v2               | Summary        |
+============================================+================+==================+================+
| Kernel logging                             | 7us            | 7us [#f0]_/11us  | No significant |
|                                            |                |                  | change         |
+--------------------------------------------+----------------+------------------+----------------+
| User logging                               | 86us           | 13us             | **Strongly     |
|                                            |                |                  | improved**     |
+--------------------------------------------+----------------+------------------+----------------+
| kernel logging with overwrite              | 23us           | 10us [#f0]_/15us | **Improved**   |
+--------------------------------------------+----------------+------------------+----------------+
| Logging transient string                   | 16us           | 42us             | **Degraded**   |
+--------------------------------------------+----------------+------------------+----------------+
| Logging transient string from user         | 111us          | 50us             | **Improved**   |
+--------------------------------------------+----------------+------------------+----------------+
| Memory utilization [#f1]_                  | 416            | 518              | Slightly       |
|                                            |                |                  | improved       |
+--------------------------------------------+----------------+------------------+----------------+
| Memory footprint (test) [#f2]_             | 3.2k           | 2k               | **Improved**   |
+--------------------------------------------+----------------+------------------+----------------+
| Memory footprint (application) [#f3]_      | 6.2k           | 3.5k             | **Improved**   |
+--------------------------------------------+----------------+------------------+----------------+
| Message footprint [#f4]_                   | 15 bytes       | 47 [#f0]_/32     | **Degraded**   |
|                                            |                | bytes            |                |
+--------------------------------------------+----------------+------------------+----------------+

.. rubric:: Benchmark details

.. [#f0] :kconfig:`CONFIG_LOG_SPEED` enabled.

.. [#f1] Number of log messages with various number of arguments that fits in 2048
  bytes dedicated for logging.

.. [#f2] Logging subsystem memory footprint in :zephyr_file:`tests/subsys/logging/log_benchmark`
  where filtering and formatting features are not used.

.. [#f3] Logging subsystem memory footprint in :zephyr_file:`samples/subsys/logging/logger`.

.. [#f4] Avarage size of a log message (excluding string) with 2 arguments on ``Cortex M3``

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

Logger output formatting
========================

.. doxygengroup:: log_output
