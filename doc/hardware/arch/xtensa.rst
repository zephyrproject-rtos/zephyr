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
HiFi registers. During each thread context switch, the kernel saves the outgoing
thread's HiFi registers and loads the incoming thread's HiFi registers,
regardless of whether the thread utilizes them or not.

Additional stack space may be required for each thread to account for the extra
registers that must be saved.

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
