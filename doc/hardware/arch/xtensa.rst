.. _xtensa_developer_guide:

Xtensa Developer Guide
######################

Overview
********

This page contains information on certain aspects when developing for
Xtensa-based platforms.

HiFi Audio Engine DSP
*********************

The kernel allows threads to use the HiFi Audio Engine DSP registers on boards
that support these registers. The kernel only supports the use of the HiFi
registers by threads and not ISRs.

.. note::
    Presently, only the Intel ADSP ACE hardware platforms are configured for
    HiFi support by default.

Concepts
========

The kernel can be configured for an application to leverage the services
provided by the Xtensa HiFi Audio Engine DSP. Three modes of operation are
supported, which are described below.

No HiFi registers mode
----------------------

This mode is used when the application has no threads that use the HiFi
registers. It is the kernel's default HiFi services mode.

Unshared HiFi registers mode
----------------------------

This mode is used when the application has only a single thread that uses the
HiFi registers. The HiFi registers are left unchanged whenever a context
switch occurs.

.. note::
    The behavior is undefined, if two or more threads attempt to use
    the HiFi registers, as the kernel does not attempt to detect
    (nor prevent) multiple threads from using these registers.

Shared HiFi registers mode
--------------------------

This mode is used when the application has two or more threads that use HiFi
registers. When enabled, the kernel automatically allows all threads to use the
HiFi registers. Conceptually, it can be sub-divided into two sub-modes--eager
mode and lazy mode. They will both save and restore the HiFi registers, but
they differ in when the registers are saved and restored, as well as to where
the registers they are saved and from where they are restored.

In the eager sharing model, the HiFi registers are saved and restored during
every thread context switch, regardless of whether the thread used them or not.
Additional stack space may be required for each thread to account for the extra
registers that must be saved. This is default of the two models.

In the lazy sharing model, the kernel tracks the thread that 'owns' the
coprocessor. If the 'owning' thread is switched out, the HiFi registers will
not be saved until a new thread attempts to use the HiFi, after which that
that new thread becomes the new owner and its HiFi registers are loaded.

.. note::
    Lazy HiFi sharing requires that a thread that 'owns' the HiFi coprocessor
    to not change its CPU affinity while it is the owner. Developers are thus
    strongly discouraged from changing the CPU affinity of threads that have
    been started. To aid in this, lazy HiFi sharing requires that the
    configuration option :kconfig:option:`CONFIG_SCHED_CPU_MASK_PIN_ONLY` be
    enabled.

Configuration Options
=====================

The unshared HiFi registers mode is selected when configuration option
:kconfig:option:`CONFIG_XTENSA_HIFI_SHARING` is disabled but configuration
options :kconfig:option:`CONFIG_XTENSA_HIFI3` and/or
:kconfig:option:`CONFIG_XTENSA_HIFI4` are enabled.

The shared HiFi registers mode is selected when the configuration option
:kconfig:option:`CONFIG_XTENSA_HIFI_SHARING` is enabled in addition to
configuration options :kconfig:option:`CONFIG_XTENSA_HIFI3` and/or
:kconfig:option:`CONFIG_XTENSA_HIFI4`. Threads must have sufficient
stack space for saving the HiFi register values during context switches
as described above.

Both eager and lazy HiFi sharing modes require the configuration option
:kconfig:option:`CONFIG_XTENSA_HIFI_SHARING` to be enabled. Although eager
HiFi sharing is the default, it can be explicitly selected by enabling the
configuration option :kconfig:option:`CONFIG_XTENSA_EAGER_HIFI_SHARING`. To
select lazy HiFi sharing instead, enable the configuration option
:kconfig:option:`CONFIG_XTENSA_LAZY_HIFI_SHARING`.
