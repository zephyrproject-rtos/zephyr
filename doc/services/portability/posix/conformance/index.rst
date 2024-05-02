.. _posix_conformance:

POSIX Conformance
#################

As per `IEEE 1003.1-2017`, this section details Zephyr's POSIX conformance.

.. _posix_undefined_behaviour:

.. note::
   As per POSIX 1003.13, single process mode is supported directly by both PSE51 and PSE52
   profiles. While Zephyr includes support for many features found in PSE53, PSE53 itself requires
   supporting multiple processes. Since supporting multiple processes is beyond the scope of
   Zephyr's current design, some features requiring multi-process capabilities may exhibit
   undefined behaviour, which we denote with the † (obelus) symbol.

.. _posix_libc_provided:

.. note::
    Features listed in various POSIX Options or Option Groups may be provided in whole or in part
    by a conformant C library implementation. This includes (but is not limited to) POSIX
    Extensions to the ISO C Standard (`CX`_).

.. _posix_system_interfaces:

POSIX System Interfaces
=======================

.. The following have values greater than -1 in Zephyr, conformant with the POSIX specification.

.. csv-table:: POSIX System Interfaces
   :header: Symbol, Support, Remarks
   :widths: 50, 10, 50

    _POSIX_CHOWN_RESTRICTED, 0,
    _POSIX_NO_TRUNC, 0,
    _POSIX_VDISABLE, 0,

.. The following should be valued greater than zero in Zephyr, in order to be strictly conformant
    with the POSIX specification.

.. csv-table:: POSIX System Interfaces
   :header: Symbol, Support, Remarks
   :widths: 50, 10, 50

    _POSIX_JOB_CONTROL, -1, :ref:`†<posix_undefined_behaviour>`
    _POSIX_REGEXP, -1, :ref:`†<posix_undefined_behaviour>`
    _POSIX_SAVED_IDS, -1, :ref:`†<posix_undefined_behaviour>`
    _POSIX_SHELL, -1, :ref:`†<posix_undefined_behaviour>`

.. TODO: POSIX_ASYNCHRONOUS_IO, and other interfaces below, are mandatory. That means that a
   strictly conforming application need not be modified in order to compile against Zephyr.
   However, we may add implementations that simply fail with ENOSYS as long as the functional
   modification is clearly documented. The implementation is not required for PSE51 or PSE52
   and beyond that POSIX async I/O functions are rarely used in practice.

.. _posix_system_interfaces_required:

.. csv-table:: POSIX System Interfaces
   :header: Symbol, Support, Remarks
   :widths: 50, 10, 50

    _POSIX_VERSION, 200809L,
    :ref:`_POSIX_ASYNCHRONOUS_IO<posix_option_asynchronous_io>`, 200809L, :ref:`†<posix_undefined_behaviour>`
    :ref:`_POSIX_BARRIERS<posix_option_group_barriers>`, 200809L, :kconfig:option:`CONFIG_PTHREAD_BARRIER`
    :ref:`_POSIX_CLOCK_SELECTION<posix_option_group_clock_selection>`, 200809L, :kconfig:option:`CONFIG_POSIX_CLOCK`
    _POSIX_MAPPED_FILES, -1, :ref:`†<posix_undefined_behaviour>`
    _POSIX_MEMORY_PROTECTION, -1, :ref:`†<posix_undefined_behaviour>`
    :ref:`_POSIX_READER_WRITER_LOCKS<posix_option_reader_writer_locks>`, 200809L, :kconfig:option:`CONFIG_PTHREAD_IPC`
    :ref:`_POSIX_REALTIME_SIGNALS<posix_option_group_realtime_signals>`, -1,
    :ref:`_POSIX_SEMAPHORES<posix_option_group_semaphores>`, 200809L, :kconfig:option:`CONFIG_PTHREAD_IPC`
    :ref:`_POSIX_SPIN_LOCKS<posix_option_group_spin_locks>`, 200809L, :kconfig:option:`CONFIG_PTHREAD_SPINLOCK`
    :ref:`_POSIX_THREAD_SAFE_FUNCTIONS<posix_thread_safe_functions>`, -1,
    :ref:`_POSIX_THREADS<posix_option_group_threads_base>`, -1, :kconfig:option:`CONFIG_PTHREAD_IPC`
    :ref:`_POSIX_TIMEOUTS<posix_option_timeouts>`, 200809L, :kconfig:option:`CONFIG_PTHREAD_IPC`
    :ref:`_POSIX_TIMERS<posix_option_group_timers>`, 200809L, :kconfig:option:`CONFIG_POSIX_CLOCK`
    _POSIX2_C_BIND, 200809L,

.. csv-table:: POSIX System Interfaces (Optional)
   :header: Symbol, Support, Remarks
   :widths: 50, 10, 50

    _POSIX_ADVISORY_INFO, -1,
    _POSIX_CPUTIME, 200809L, :kconfig:option:`CONFIG_POSIX_CLOCK`
    _POSIX_FSYNC, 200809L, :kconfig:option:`CONFIG_POSIX_FS`
    _POSIX_IPV6, 200809L, :kconfig:option:`CONFIG_NET_IPV6`
    _POSIX_MEMLOCK, -1,
    _POSIX_MEMLOCK_RANGE, -1,
    :ref:`_POSIX_MESSAGE_PASSING<posix_option_message_passing>`, 200809L, :kconfig:option:`CONFIG_POSIX_MQUEUE`
    _POSIX_MONOTONIC_CLOCK, 200809L, :kconfig:option:`CONFIG_POSIX_CLOCK`
    _POSIX_PRIORITIZED_IO, -1,
    :ref:`_POSIX_PRIORITY_SCHEDULING<posix_option_priority_scheduling>`, -1, :kconfig:option:`CONFIG_POSIX_PRIORITY_SCHEDULING` (will fail with ``ENOSYS``:ref:`†<posix_undefined_behaviour>`)
    _POSIX_RAW_SOCKETS, 200809L, :kconfig:option:`CONFIG_NET_SOCKETS`
    _POSIX_SHARED_MEMORY_OBJECTS, -1,
    _POSIX_SPAWN, -1,
    _POSIX_SPORADIC_SERVER, -1,
    _POSIX_SYNCHRONIZED_IO, -1, :kconfig:option:`CONFIG_POSIX_FS`
    :ref:`_POSIX_THREAD_ATTR_STACKADDR<posix_option_thread_attr_stackaddr>`, 200809L, :kconfig:option:`CONFIG_PTHREAD`
    :ref:`_POSIX_THREAD_ATTR_STACKSIZE<posix_option_thread_attr_stacksize>`, 200809L, :kconfig:option:`CONFIG_PTHREAD`
    _POSIX_THREAD_CPUTIME, -1,
    _POSIX_THREAD_PRIO_INHERIT, 200809L, :kconfig:option:`CONFIG_PTHREAD_MUTEX`
    _POSIX_THREAD_PRIO_PROTECT, -1,
    :ref:`_POSIX_THREAD_PRIORITY_SCHEDULING<posix_option_thread_priority_scheduling>`, 200809L, :kconfig:option:`CONFIG_PTHREAD`
    _POSIX_THREAD_PROCESS_SHARED, -1,
    _POSIX_THREAD_SPORADIC_SERVER, -1,
    _POSIX_TRACE, -1,
    _POSIX_TRACE_EVENT_FILTER, -1,
    _POSIX_TRACE_INHERIT, -1,
    _POSIX_TRACE_LOG, -1,
    _POSIX_TYPED_MEMORY_OBJECTS, -1,
    _XOPEN_CRYPT, -1,
    _XOPEN_REALTIME, -1,
    _XOPEN_REALTIME_THREADS, -1,
    :ref:`_XOPEN_STREAMS<posix_option_xopen_streams>`, -1, :ref:`†<posix_undefined_behaviour>`
    _XOPEN_UNIX, -1,

POSIX Shell and Utilities
=========================

Zephyr does not support a POSIX shell or utilities at this time.

.. csv-table:: POSIX Shell and Utilities
   :header: Symbol, Support, Remarks
   :widths: 50, 10, 50

    _POSIX2_C_DEV, -1, :ref:`†<posix_undefined_behaviour>`
    _POSIX2_CHAR_TERM, -1, :ref:`†<posix_undefined_behaviour>`
    _POSIX2_FORT_DEV, -1, :ref:`†<posix_undefined_behaviour>`
    _POSIX2_FORT_RUN, -1, :ref:`†<posix_undefined_behaviour>`
    _POSIX2_LOCALEDEF, -1, :ref:`†<posix_undefined_behaviour>`
    _POSIX2_PBS, -1, :ref:`†<posix_undefined_behaviour>`
    _POSIX2_PBS_ACCOUNTING, -1, :ref:`†<posix_undefined_behaviour>`
    _POSIX2_PBS_LOCATE, -1, :ref:`†<posix_undefined_behaviour>`
    _POSIX2_PBS_MESSAGE, -1, :ref:`†<posix_undefined_behaviour>`
    _POSIX2_PBS_TRACK, -1, :ref:`†<posix_undefined_behaviour>`
    _POSIX2_SW_DEV, -1, :ref:`†<posix_undefined_behaviour>`
    _POSIX2_UPE, -1, :ref:`†<posix_undefined_behaviour>`
    _POSIX2_UNIX, -1, :ref:`†<posix_undefined_behaviour>`
    _POSIX2_UUCP, -1, :ref:`†<posix_undefined_behaviour>`

XSI Conformance
###############

XSI System Interfaces
=====================

.. csv-table:: XSI System Interfaces
   :header: Symbol, Support, Remarks
   :widths: 50, 10, 50

    _POSIX_FSYNC, 200809L, :kconfig:option:`CONFIG_POSIX_FS`
    :ref:`_POSIX_THREAD_ATTR_STACKADDR<posix_option_thread_attr_stackaddr>`, 200809L, :kconfig:option:`CONFIG_PTHREAD`
    :ref:`_POSIX_THREAD_ATTR_STACKSIZE<posix_option_thread_attr_stacksize>`, 200809L, :kconfig:option:`CONFIG_PTHREAD`
    _POSIX_THREAD_PROCESS_SHARED, -1,

.. _CX: https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap01.html
