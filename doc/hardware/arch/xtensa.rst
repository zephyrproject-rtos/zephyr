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
they are saved and from where they are restored.

In the eager sharing model, the HiFi registers are saved and restored during
every thread context switch, regardless of whether the thread used them or not.
Additional stack space may be required for each thread to account for the extra
registers that must be saved. This is default of the two models.

In the lazy sharing model, the kernel tracks the thread that 'owns' the
coprocessor. If the 'owning' thread is switched out, the HiFi registers will
not be saved until a new thread attempts to use the HiFi, after which that
new thread becomes the new owner and its HiFi registers are loaded.

.. note::
    If an SMP system detects that the owner-to-be is still an owner on another
    CPU, an IPI will be sent to that CPU to initiate the saving of its HiFi
    registers to memory. The current processor will then spin until the HiFi
    registers are saved. This spinning may result in sporadically longer
    delays. For the best performance, it is recommended that a thread
    using HiFi be pinned to a single CPU.

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

ESP32-S3 PIE
************

The Processor Instruction Extensions (PIE) of the ESP32-S3 add 128-bit SIMD instructions to its
Xtensa LX7 cores, with eight 128-bit ``Q`` registers, the ``QACC`` and ``ACCX`` accumulators and a
few state registers. PIE is coprocessor 3 of the core configuration, named ``cop_ai``.

When :kconfig:option:`CONFIG_ESP32S3_PIE_SHARING` is enabled, the kernel preserves the PIE
registers across context switches, allowing multiple threads to use PIE instructions. It uses the
same mechanism as the shared HiFi registers mode described above, and the same restriction
applies: only threads may use the PIE registers, not ISRs. Without this option, the behavior is
undefined if two or more threads use PIE instructions.

Two sharing models are available:

* :kconfig:option:`CONFIG_ESP32S3_LAZY_PIE_SHARING`, the default, keeps PIE disabled for all
  threads but the last one that used it. The kernel saves and restores the PIE registers only when
  another thread executes a PIE instruction, using a 208-byte area in each thread structure. That
  instruction traps, and the trap takes about 320 instructions. Other context switches take 4
  more instructions.

* :kconfig:option:`CONFIG_ESP32S3_EAGER_PIE_SHARING` saves and restores the PIE registers on every
  context switch and interrupt, which adds about 100 instructions to each of them. Every saved
  context grows by 224 bytes, which thread and interrupt stacks must have room for.

The lazy model costs less when PIE rarely changes hands between threads, the eager model when
threads using PIE alternate on most context switches.
