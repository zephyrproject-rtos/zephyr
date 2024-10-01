.. _posix_aep:

POSIX Application Environment Profiles (AEP)
############################################

Although inactive, `IEEE 1003.13-2003`_ defined a number of AEP that inspired the modern
subprofiling options of `IEEE 1003.1-2017`_. The single-purpose realtime system profiles
are listed below, for reference, in terms that agree with the current POSIX-1 standard. PSE54
is not considered at this time.

System Interfaces
=================

The required POSIX :ref:`System Interfaces<posix_system_interfaces_required>` are supported for
each Application Environment Profile.

..  figure:: si.svg
    :align: center
    :scale: 150%
    :alt: Required System Interfaces

    System Interfaces

.. _posix_aep_pse51:

Minimal Realtime System Profile (PSE51)
=======================================

The *Minimal Realtime System Profile* (PSE51) includes all of the
:ref:`System Interfaces<posix_system_interfaces_required>` along with several additional features.

..  figure:: aep-pse51.svg
    :align: center
    :scale: 150%
    :alt: Minimal Realtime System Profile (PSE51)

    Minimal Realtime System Profile (PSE51)

.. Conforming implementations shall define _POSIX_AEP_REALTIME_MINIMAL to the value 200312L

.. csv-table:: PSE51 System Interfaces
   :header: Symbol, Support, Remarks
   :widths: 50, 10, 50

    _POSIX_AEP_REALTIME_MINIMAL, -1, :kconfig:option:`CONFIG_POSIX_AEP_REALTIME_MINIMAL`

.. csv-table:: PSE51 Option Groups
   :header: Symbol, Support, Remarks
   :widths: 50, 10, 50

    :ref:`POSIX_C_LANG_JUMP <posix_option_group_c_lang_jump>`, yes,
    :ref:`POSIX_C_LANG_SUPPORT <posix_option_group_c_lang_support>`, yes,
    :ref:`POSIX_DEVICE_IO <posix_option_group_device_io>`,, :kconfig:option:`CONFIG_POSIX_DEVICE_IO`
    :ref:`POSIX_SIGNALS <posix_option_group_signals>`, yes, :kconfig:option:`CONFIG_POSIX_SIGNALS` :ref:`†<posix_undefined_behaviour>`
    :ref:`POSIX_SINGLE_PROCESS <posix_option_group_single_process>`, yes, :kconfig:option:`CONFIG_POSIX_SINGLE_PROCESS`
    :ref:`XSI_THREADS_EXT <posix_option_group_xsi_threads_ext>`, yes, :kconfig:option:`CONFIG_XSI_THREADS_EXT`

.. csv-table:: PSE51 Option Requirements
   :header: Symbol, Support, Remarks
   :widths: 50, 10, 50

    :ref:`_POSIX_FSYNC <posix_option_fsync>`, 200809L, :kconfig:option:`CONFIG_POSIX_FSYNC`
    :ref:`_POSIX_MEMLOCK <posix_option_memlock>`, 200809L, :kconfig:option:`CONFIG_POSIX_MEMLOCK` :ref:`†<posix_undefined_behaviour>`
    :ref:`_POSIX_MEMLOCK_RANGE <posix_option_memlock_range>`, 200809L, :kconfig:option:`CONFIG_POSIX_MEMLOCK_RANGE`
    :ref:`_POSIX_MONOTONIC_CLOCK <posix_option_monotonic_clock>`, 200809L, :kconfig:option:`CONFIG_POSIX_MONOTONIC_CLOCK`
    :ref:`_POSIX_SHARED_MEMORY_OBJECTS <posix_shared_memory_objects>`, 200809L, :kconfig:option:`CONFIG_POSIX_SHARED_MEMORY_OBJECTS`
    :ref:`_POSIX_SYNCHRONIZED_IO <posix_option_synchronized_io>`, 200809L, :kconfig:option:`CONFIG_POSIX_SYNCHRONIZED_IO`
    :ref:`_POSIX_THREAD_ATTR_STACKADDR<posix_option_thread_attr_stackaddr>`, 200809L, :kconfig:option:`CONFIG_POSIX_THREAD_ATTR_STACKADDR`
    :ref:`_POSIX_THREAD_ATTR_STACKSIZE<posix_option_thread_attr_stacksize>`, 200809L, :kconfig:option:`CONFIG_POSIX_THREAD_ATTR_STACKSIZE`
    :ref:`_POSIX_THREAD_CPUTIME <posix_option_thread_cputime>`, 200809L, :kconfig:option:`CONFIG_POSIX_CPUTIME`
    :ref:`_POSIX_THREAD_PRIO_INHERIT <posix_option_thread_prio_inherit>`, 200809L, :kconfig:option:`CONFIG_POSIX_THREAD_PRIO_INHERIT`
    :ref:`_POSIX_THREAD_PRIO_PROTECT <posix_option_thread_prio_protect>`, 200809L, :kconfig:option:`CONFIG_POSIX_THREAD_PRIO_PROTECT`
    :ref:`_POSIX_THREAD_PRIORITY_SCHEDULING <posix_option_thread_priority_scheduling>`, 200809L, :kconfig:option:`CONFIG_POSIX_THREAD_PRIORITY_SCHEDULING`
    _POSIX_THREAD_SPORADIC_SERVER, -1,

.. _posix_aep_pse52:

Realtime Controller System Profile (PSE52)
==========================================

The *Realtime Controller System Profile* (PSE52) includes all features from PSE51 and the
:ref:`System Interfaces<posix_system_interfaces_required>`.

..  figure:: aep-pse52.svg
    :align: center
    :scale: 150%
    :alt: Realtime Controller System Profile (PSE52)

    Realtime Controller System Profile (PSE52)

.. Conforming implementations shall define _POSIX_AEP_REALTIME_CONTROLLER to the value 200312L

.. csv-table:: PSE52 System Interfaces
   :header: Symbol, Support, Remarks
   :widths: 50, 10, 50

    _POSIX_AEP_REALTIME_CONTROLLER, -1, :kconfig:option:`CONFIG_POSIX_AEP_REALTIME_CONTROLLER`

.. csv-table:: PSE52 Option Groups
   :header: Symbol, Support, Remarks
   :widths: 50, 10, 50

    :ref:`POSIX_C_LANG_MATH <posix_option_group_c_lang_math>`, yes,
    :ref:`POSIX_FD_MGMT <posix_option_group_fd_mgmt>`,, :kconfig:option:`CONFIG_POSIX_FD_MGMT`
    :ref:`POSIX_FILE_SYSTEM <posix_option_group_file_system>`,, :kconfig:option:`CONFIG_POSIX_FILE_SYSTEM`

.. csv-table:: PSE52 Option Requirements
   :header: Symbol, Support, Remarks
   :widths: 50, 10, 50

    :ref:`_POSIX_MESSAGE_PASSING <posix_option_message_passing>`, 200809L, :kconfig:option:`CONFIG_POSIX_MESSAGE_PASSING`
    _POSIX_TRACE, -1,
    _POSIX_TRACE_EVENT_FILTER, -1,
    _POSIX_TRACE_LOG, -1,

.. _posix_aep_pse53:

Dedicated Realtime System Profile (PSE53)
=========================================

The *Dedicated Realtime System Profile* (PSE53) includes all features from PSE52, PSE51, and the
:ref:`System Interfaces<posix_system_interfaces_required>`.

..  figure:: aep-pse53.svg
    :align: center
    :scale: 150%
    :alt: Dedicated Realtime System Profile (PSE53)

    Dedicated Realtime System Profile (PSE53)

.. Conforming implementations shall define _POSIX_AEP_REALTIME_DEDICATED to the value 200312L

.. csv-table:: PSE53 System Interfaces
   :header: Symbol, Support, Remarks
   :widths: 50, 10, 50

    _POSIX_AEP_REALTIME_DEDICATED, -1, :kconfig:option:`CONFIG_POSIX_AEP_REALTIME_DEDICATED`

.. csv-table:: PSE53 Option Groups
   :header: Symbol, Support, Remarks
   :widths: 50, 10, 50

    :ref:`POSIX_MULTI_PROCESS<posix_option_group_multi_process>`,, :kconfig:option:`CONFIG_POSIX_MULTI_PROCESS`:ref:`†<posix_undefined_behaviour>`
    :ref:`POSIX_NETWORKING <posix_option_group_networking>`, yes, :kconfig:option:`CONFIG_POSIX_NETWORKING`
    :ref:`POSIX_PIPE <posix_option_group_pipe>`,,
    :ref:`POSIX_SIGNAL_JUMP <posix_option_group_signal_jump>`,,

.. csv-table:: PSE53 Option Requirements
   :header: Symbol, Support, Remarks
   :widths: 50, 10, 50

    :ref:`_POSIX_CPUTIME <posix_option_cputime>`, 200809L, :kconfig:option:`CONFIG_POSIX_CPUTIME`
    _POSIX_PRIORITIZED_IO, -1,
    :ref:`_POSIX_PRIORITY_SCHEDULING <posix_option_priority_scheduling>`, -1,
    :ref:`_POSIX_RAW_SOCKETS <posix_option_raw_sockets>`, 200809L, :kconfig:option:`CONFIG_POSIX_RAW_SOCKETS`
    _POSIX_SPAWN, -1, :ref:`†<posix_undefined_behaviour>`
    _POSIX_SPORADIC_SERVER, -1, :ref:`†<posix_undefined_behaviour>`

.. _IEEE 1003.1-2017: https://standards.ieee.org/ieee/1003.1/7101/
.. _IEEE 1003.13-2003: https://standards.ieee.org/ieee/1003.13/3322/
