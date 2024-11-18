.. _posix_conformance:

POSIX Conformance
#################

As per `IEEE 1003.1-2017`_, this section details Zephyr's POSIX conformance.

.. _IEEE 1003.1-2017: https://standards.ieee.org/ieee/1003.1/7101/

.. _posix_system_interfaces:

POSIX System Interfaces
=======================

.. The following have values greater than -1 in Zephyr, conformant with the POSIX specification.

.. csv-table:: POSIX System Interfaces
   :header: Symbol, Support, Remarks
   :widths: 50, 10, 50

    _POSIX_CHOWN_RESTRICTED, 0,
    _POSIX_NO_TRUNC, 0,
    _POSIX_VDISABLE, ``'\0'``,

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
    :ref:`_POSIX_ASYNCHRONOUS_IO<posix_option_asynchronous_io>`, 200809L, :kconfig:option:`CONFIG_POSIX_ASYNCHRONOUS_IO`:ref:`†<posix_undefined_behaviour>`
    :ref:`_POSIX_BARRIERS<posix_option_group_barriers>`, 200809L, :kconfig:option:`CONFIG_POSIX_BARRIERS`
    :ref:`_POSIX_CLOCK_SELECTION<posix_option_group_clock_selection>`, 200809L, :kconfig:option:`CONFIG_POSIX_CLOCK_SELECTION`
    :ref:`_POSIX_MAPPED_FILES<posix_option_group_mapped_files>`, 200809L, :kconfig:option:`CONFIG_POSIX_MAPPED_FILES`
    :ref:`_POSIX_MEMORY_PROTECTION<posix_option_group_memory_protection>`, 200809L, :kconfig:option:`CONFIG_POSIX_MEMORY_PROTECTION` :ref:`†<posix_undefined_behaviour>`
    :ref:`_POSIX_READER_WRITER_LOCKS<posix_option_reader_writer_locks>`, 200809L, :kconfig:option:`CONFIG_POSIX_READER_WRITER_LOCKS`
    :ref:`_POSIX_REALTIME_SIGNALS<posix_option_group_realtime_signals>`, -1, :kconfig:option:`CONFIG_POSIX_REALTIME_SIGNALS`
    :ref:`_POSIX_SEMAPHORES<posix_option_group_semaphores>`, 200809L, :kconfig:option:`CONFIG_POSIX_SEMAPHORES`
    :ref:`_POSIX_SPIN_LOCKS<posix_option_group_spin_locks>`, 200809L, :kconfig:option:`CONFIG_POSIX_SPIN_LOCKS`
    :ref:`_POSIX_THREAD_SAFE_FUNCTIONS<posix_option_thread_safe_functions>`, -1, :kconfig:option:`CONFIG_POSIX_THREAD_SAFE_FUNCTIONS`
    :ref:`_POSIX_THREADS<posix_option_group_threads_base>`, -1, :kconfig:option:`CONFIG_POSIX_THREADS`
    :ref:`_POSIX_TIMEOUTS<posix_option_timeouts>`, 200809L, :kconfig:option:`CONFIG_POSIX_TIMEOUTS`
    :ref:`_POSIX_TIMERS<posix_option_group_timers>`, 200809L, :kconfig:option:`CONFIG_POSIX_TIMERS`
    _POSIX2_C_BIND, 200809L,

.. The following should be valued greater than zero in Zephyr, in order to be strictly conformant
    with the POSIX specification.

.. csv-table:: POSIX System Interfaces (Unsupported)
   :header: Symbol, Support, Remarks
   :widths: 50, 10, 50

    _POSIX_JOB_CONTROL, -1, :ref:`†<posix_undefined_behaviour>`
    _POSIX_REGEXP, -1, :ref:`†<posix_undefined_behaviour>`
    _POSIX_SAVED_IDS, -1, :ref:`†<posix_undefined_behaviour>`
    _POSIX_SHELL, -1, :ref:`†<posix_undefined_behaviour>`

.. csv-table:: POSIX System Interfaces (Optional)
   :header: Symbol, Support, Remarks
   :widths: 50, 10, 50

    _POSIX_ADVISORY_INFO, -1,
    :ref:`_POSIX_CPUTIME<posix_option_cputime>`, 200809L, :kconfig:option:`CONFIG_POSIX_CPUTIME`
    :ref:`_POSIX_FSYNC<posix_option_fsync>`, 200809L, :kconfig:option:`CONFIG_POSIX_FSYNC`
    :ref:`_POSIX_IPV6<posix_option_ipv6>`, 200809L, :kconfig:option:`CONFIG_POSIX_IPV6`
    :ref:`_POSIX_MEMLOCK <posix_option_memlock>`, 200809L, :kconfig:option:`CONFIG_POSIX_MEMLOCK` :ref:`†<posix_undefined_behaviour>`
    :ref:`_POSIX_MEMLOCK_RANGE <posix_option_memlock_range>`, 200809L, :kconfig:option:`CONFIG_POSIX_MEMLOCK_RANGE`
    :ref:`_POSIX_MESSAGE_PASSING<posix_option_message_passing>`, 200809L, :kconfig:option:`CONFIG_POSIX_MESSAGE_PASSING`
    :ref:`_POSIX_MONOTONIC_CLOCK<posix_option_monotonic_clock>`, 200809L, :kconfig:option:`CONFIG_POSIX_MONOTONIC_CLOCK`
    _POSIX_PRIORITIZED_IO, -1,
    :ref:`_POSIX_PRIORITY_SCHEDULING<posix_option_priority_scheduling>`, 200809L, :kconfig:option:`CONFIG_POSIX_PRIORITY_SCHEDULING`
    :ref:`_POSIX_RAW_SOCKETS<posix_option_raw_sockets>`, 200809L, :kconfig:option:`CONFIG_POSIX_RAW_SOCKETS`
    :ref:`_POSIX_SHARED_MEMORY_OBJECTS <posix_shared_memory_objects>`, 200809L, :kconfig:option:`CONFIG_POSIX_SHARED_MEMORY_OBJECTS`
    _POSIX_SPAWN, -1, :ref:`†<posix_undefined_behaviour>`
    _POSIX_SPORADIC_SERVER, -1, :ref:`†<posix_undefined_behaviour>`
    :ref:`_POSIX_SYNCHRONIZED_IO <posix_option_synchronized_io>`, 200809L, :kconfig:option:`CONFIG_POSIX_SYNCHRONIZED_IO`
    :ref:`_POSIX_THREAD_ATTR_STACKADDR<posix_option_thread_attr_stackaddr>`, 200809L, :kconfig:option:`CONFIG_POSIX_THREAD_ATTR_STACKADDR`
    :ref:`_POSIX_THREAD_ATTR_STACKSIZE<posix_option_thread_attr_stacksize>`, 200809L, :kconfig:option:`CONFIG_POSIX_THREAD_ATTR_STACKSIZE`
    :ref:`_POSIX_THREAD_CPUTIME <posix_option_thread_cputime>`, 200809L, :kconfig:option:`CONFIG_POSIX_CPUTIME`
    :ref:`_POSIX_THREAD_PRIO_INHERIT <posix_option_thread_prio_inherit>`, 200809L, :kconfig:option:`CONFIG_POSIX_THREAD_PRIO_INHERIT`
    :ref:`_POSIX_THREAD_PRIO_PROTECT <posix_option_thread_prio_protect>`, 200809L, :kconfig:option:`CONFIG_POSIX_THREAD_PRIO_PROTECT`
    :ref:`_POSIX_THREAD_PRIORITY_SCHEDULING <posix_option_thread_priority_scheduling>`, 200809L, :kconfig:option:`CONFIG_POSIX_THREAD_PRIORITY_SCHEDULING`
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
    :ref:`_XOPEN_STREAMS<posix_option_xopen_streams>`, 200809L, :kconfig:option:`CONFIG_XOPEN_STREAMS`
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

X/Open System Interfaces
========================

.. csv-table:: X/Open System Interfaces
   :header: Symbol, Support, Remarks
   :widths: 50, 10, 50

    :ref:`_POSIX_FSYNC<posix_option_fsync>`, 200809L, :kconfig:option:`CONFIG_POSIX_FSYNC`
    :ref:`_POSIX_THREAD_ATTR_STACKADDR<posix_option_thread_attr_stackaddr>`, 200809L, :kconfig:option:`CONFIG_POSIX_THREAD_ATTR_STACKADDR`
    :ref:`_POSIX_THREAD_ATTR_STACKSIZE<posix_option_thread_attr_stacksize>`, 200809L, :kconfig:option:`CONFIG_POSIX_THREAD_ATTR_STACKSIZE`
    _POSIX_THREAD_PROCESS_SHARED, -1,

.. _posix_undefined_behaviour:

.. note::
   Some features may exhibit undefined behaviour as they fall beyond the scope of Zephyr's current
   design and capabilities. For example, multi-processing, ad-hoc memory-mapping, multiple users,
   or regular expressions are features that are uncommon in low-footprint embedded systems.
   Such undefined behaviour is denoted with the † (obelus) symbol. Additional details
   :ref:`here <posix_option_groups>`.

.. _posix_libc_provided:

.. note::
    Features listed in various POSIX Options or Option Groups may be provided in whole or in part
    by a conformant C library implementation. This includes (but is not limited to) POSIX
    Extensions to the ISO C Standard (`CX`_).

.. _CX: https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap01.html
