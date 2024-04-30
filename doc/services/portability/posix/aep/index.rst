.. _posix_aep:

POSIX Application Environment Profiles (AEP)
############################################

Although inactive, `IEEE 1003.13-2003`_ defined a number of AEP that inspired the modern
subprofiling options of `IEEE 1003.1-2017`_. The single-purpose realtime system profiles
are listed below, for reference, in terms that agree with the current POSIX-1 standard. PSE54
is not considered at this time.

.. _posix_aep_pse51:

Minimal Realtime System Profile (PSE51)
=======================================

.. Conforming implementations shall define _POSIX_AEP_REALTIME_MINIMAL to the value 200312L

.. csv-table:: PSE51 System Interfaces
   :header: Symbol, Support, Remarks
   :widths: 50, 10, 50

    _POSIX_AEP_REALTIME_MINIMAL, -1,

.. csv-table:: PSE51 Option Groups
   :header: Symbol, Support, Remarks
   :widths: 50, 10, 50

    POSIX_C_LANG_JUMP, yes, :ref:`POSIX_C_LANG_JUMP <posix_option_group_c_lang_jump>`
    POSIX_C_LANG_SUPPORT, yes, :ref:`POSIX_C_LANG_SUPPORT <posix_option_group_c_lang_support>`
    POSIX_DEVICE_IO,, :ref:`†<posix_undefined_behaviour>`
    POSIX_FILE_LOCKING,,
    POSIX_SIGNALS,, :ref:`†<posix_undefined_behaviour>`
    POSIX_SINGLE_PROCESS, yes,
    POSIX_THREADS_BASE, yes, :ref:`†<posix_undefined_behaviour>`
    XSI_THREADS_EXT, yes,

.. csv-table:: PSE51 Option Requirements
   :header: Symbol, Support, Remarks
   :widths: 50, 10, 50

    _POSIX_CLOCK_SELECTION, 200809L, :kconfig:option:`CONFIG_POSIX_CLOCK`
    _POSIX_FSYNC, -1,
    _POSIX_MEMLOCK, -1,
    _POSIX_MEMLOCK_RANGE, -1,
    _POSIX_MONOTONIC_CLOCK, 200809L, :kconfig:option:`CONFIG_POSIX_CLOCK`
    _POSIX_REALTIME_SIGNALS, -1,
    _POSIX_SEMAPHORES, 200809L, :kconfig:option:`CONFIG_PTHREAD_IPC`
    _POSIX_SHARED_MEMORY_OBJECTS, -1,
    _POSIX_SYNCHRONIZED_IO, -1,
    _POSIX_THREAD_ATTR_STACKADDR, 200809L, :kconfig:option:`CONFIG_PTHREAD`
    _POSIX_THREAD_ATTR_STACKSIZE, 200809L, :kconfig:option:`CONFIG_PTHREAD`
    _POSIX_THREAD_CPUTIME, -1,
    _POSIX_THREAD_PRIO_INHERIT, 200809L, :kconfig:option:`CONFIG_PTHREAD_MUTEX`
    _POSIX_THREAD_PRIO_PROTECT, -1,
    _POSIX_THREAD_PRIORITY_SCHEDULING, -1, :kconfig:option:`CONFIG_POSIX_PRIORITY_SCHEDULING` (will fail with ``ENOSYS``:ref:`†<posix_undefined_behaviour>`)
    _POSIX_THREAD_SPORADIC_SERVER, -1,
    _POSIX_TIMEOUTS, 200809L, :kconfig:option:`CONFIG_PTHREAD_IPC`
    _POSIX_TIMERS, 200809L, :kconfig:option:`CONFIG_POSIX_CLOCK`

.. _posix_aep_pse52:

Realtime Controller System Profile (PSE52)
==========================================

.. Conforming implementations shall define _POSIX_AEP_REALTIME_CONTROLLER to the value 200312L

.. csv-table:: PSE52 System Interfaces
   :header: Symbol, Support, Remarks
   :widths: 50, 10, 50

    _POSIX_AEP_REALTIME_CONTROLLER, -1,

.. csv-table:: PSE52 Option Groups
   :header: Symbol, Support, Remarks
   :widths: 50, 10, 50

    POSIX_C_LANG_JUMP, yes, :ref:`POSIX_C_LANG_JUMP <posix_option_group_c_lang_jump>`
    POSIX_C_LANG_MATH, yes, :ref:`POSIX_C_LANG_MATH <posix_option_group_c_lang_math>`
    POSIX_C_LANG_SUPPORT, yes, :ref:`POSIX_C_LANG_SUPPORT <posix_option_group_c_lang_support>`
    POSIX_DEVICE_IO,, :ref:`†<posix_undefined_behaviour>`
    POSIX_FD_MGMT,,
    POSIX_FILE_LOCKING,,
    POSIX_FILE_SYSTEM,,
    POSIX_SIGNALS,, :ref:`†<posix_undefined_behaviour>`
    POSIX_SINGLE_PROCESS, yes,
    POSIX_THREADS_BASE, yes, :ref:`†<posix_undefined_behaviour>`
    XSI_THREADS_EXT, yes,

.. csv-table:: PSE52 Option Requirements
   :header: Symbol, Support, Remarks
   :widths: 50, 10, 50

    _POSIX_CLOCK_SELECTION, 200809L, :kconfig:option:`CONFIG_POSIX_CLOCK`
    _POSIX_FSYNC, -1,
    _POSIX_MAPPED_FILES, -1,
    _POSIX_MEMLOCK, -1,
    _POSIX_MEMLOCK_RANGE, -1,
    _POSIX_MESSAGE_PASSING, 200809L, :kconfig:option:`CONFIG_POSIX_MQUEUE`
    _POSIX_MONOTONIC_CLOCK, 200809L, :kconfig:option:`CONFIG_POSIX_CLOCK`
    _POSIX_REALTIME_SIGNALS, -1,
    _POSIX_SEMAPHORES, 200809L, :kconfig:option:`CONFIG_PTHREAD_IPC`
    _POSIX_SHARED_MEMORY_OBJECTS, -1,
    _POSIX_SYNCHRONIZED_IO, -1,
    _POSIX_THREAD_ATTR_STACKADDR, 200809L, :kconfig:option:`CONFIG_PTHREAD`
    _POSIX_THREAD_ATTR_STACKSIZE, 200809L, :kconfig:option:`CONFIG_PTHREAD`
    _POSIX_THREAD_CPUTIME, -1,
    _POSIX_THREAD_PRIO_INHERIT, 200809L, :kconfig:option:`CONFIG_PTHREAD_MUTEX`
    _POSIX_THREAD_PRIO_PROTECT, -1,
    _POSIX_THREAD_PRIORITY_SCHEDULING, -1,
    _POSIX_THREAD_SPORADIC_SERVER, -1,
    _POSIX_TIMEOUTS, 200809L, :kconfig:option:`CONFIG_PTHREAD_IPC`
    _POSIX_TIMERS, 200809L, :kconfig:option:`CONFIG_POSIX_CLOCK`
    _POSIX_TRACE, -1,
    _POSIX_TRACE_EVENT_FILTER, -1,
    _POSIX_TRACE_LOG, -1,

.. _posix_aep_pse53:

Dedicated Realtime System Profile (PSE53)
=========================================

.. Conforming implementations shall define _POSIX_AEP_REALTIME_DEDICATED to the value 200312L

.. csv-table:: PSE53 System Interfaces
   :header: Symbol, Support, Remarks
   :widths: 50, 10, 50

    _POSIX_AEP_REALTIME_DEDICATED, -1,

.. csv-table:: PSE53 Option Groups
   :header: Symbol, Support, Remarks
   :widths: 50, 10, 50

    POSIX_C_LANG_JUMP, yes, :ref:`POSIX_C_LANG_JUMP <posix_option_group_c_lang_jump>`
    POSIX_C_LANG_MATH, yes, :ref:`POSIX_C_LANG_MATH <posix_option_group_c_lang_math>`
    POSIX_C_LANG_SUPPORT, yes, :ref:`POSIX_C_LANG_SUPPORT <posix_option_group_c_lang_support>`
    POSIX_DEVICE_IO,, :ref:`†<posix_undefined_behaviour>`
    POSIX_FD_MGMT,,
    POSIX_FILE_LOCKING,,
    POSIX_FILE_SYSTEM,,
    POSIX_MULTI_PROCESS,, :ref:`†<posix_undefined_behaviour>`
    POSIX_NETWORKING, yes, :ref:`POSIX_NETWORKING <posix_option_group_networking>`
    POSIX_PIPE,, :ref:`†<posix_undefined_behaviour>`
    POSIX_SIGNALS,, :ref:`†<posix_undefined_behaviour>`
    POSIX_SIGNAL_JUMP,, :ref:`†<posix_undefined_behaviour>`
    POSIX_SINGLE_PROCESS, yes,
    POSIX_THREADS_BASE, yes, :ref:`†<posix_undefined_behaviour>`
    XSI_THREADS_EXT, yes,

.. csv-table:: PSE53 Option Requirements
   :header: Symbol, Support, Remarks
   :widths: 50, 10, 50

    _POSIX_ASYNCHRONOUS_IO, -1,
    _POSIX_CLOCK_SELECTION, 200809L, :kconfig:option:`CONFIG_POSIX_CLOCK`
    _POSIX_CPUTIME, -1,
    _POSIX_FSYNC, -1,
    _POSIX_MAPPED_FILES, -1,
    _POSIX_MEMLOCK, -1,
    _POSIX_MEMLOCK_RANGE, -1,
    _POSIX_MEMORY_PROTECTION, -1,
    _POSIX_MESSAGE_PASSING, 200809L, :kconfig:option:`CONFIG_POSIX_MQUEUE`
    _POSIX_MONOTONIC_CLOCK, 200809L, :kconfig:option:`CONFIG_POSIX_CLOCK`
    _POSIX_PRIORITIZED_IO, -1,
    _POSIX_PRIORITY_SCHEDULING, -1,
    _POSIX_RAW_SOCKETS, 200809L, :kconfig:option:`CONFIG_NET_SOCKETS`
    _POSIX_REALTIME_SIGNALS, -1,
    _POSIX_SEMAPHORES, 200809L, :kconfig:option:`CONFIG_PTHREAD_IPC`
    _POSIX_SHARED_MEMORY_OBJECTS, -1,
    _POSIX_SPAWN, -1,
    _POSIX_SPORADIC_SERVER, -1,
    _POSIX_SYNCHRONIZED_IO, -1,
    _POSIX_THREAD_ATTR_STACKADDR, 200809L, :kconfig:option:`CONFIG_PTHREAD`
    _POSIX_THREAD_ATTR_STACKSIZE, 200809L, :kconfig:option:`CONFIG_PTHREAD`
    _POSIX_THREAD_CPUTIME, -1,
    _POSIX_THREAD_PRIO_INHERIT, 200809L, :kconfig:option:`CONFIG_PTHREAD_MUTEX`
    _POSIX_THREAD_PRIO_PROTECT, -1,
    _POSIX_THREAD_PRIORITY_SCHEDULING, -1,
    _POSIX_THREAD_PROCESS_SHARED, -1,
    _POSIX_THREAD_SPORADIC_SERVER, -1,
    _POSIX_TIMEOUTS, 200809L, :kconfig:option:`CONFIG_PTHREAD_IPC`
    _POSIX_TIMERS, 200809L, :kconfig:option:`CONFIG_POSIX_CLOCK`
    _POSIX_TRACE, -1,
    _POSIX_TRACE_EVENT_FILTER, -1,
    _POSIX_TRACE_LOG, -1,

.. _IEEE 1003.1-2017: https://standards.ieee.org/ieee/1003.1/7101/
.. _IEEE 1003.13-2003: https://standards.ieee.org/ieee/1003.13/3322/
