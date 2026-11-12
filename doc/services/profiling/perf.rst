.. _profiling-perf:

Perf
####

Perf provides stack sampling and standalone performance event counting sessions for lightweight
profiling with minimal code overhead.

Work Principle
**************

The ``perf record`` shell command starts a timer with the perf tracer function.
Timers are driven by interrupts, so the perf tracer function is called during an interruption.
The Zephyr core saves the return address and frame pointer in the interrupt stack,
``callee_saved`` structure, or architecture-specific exception frame before calling the
interrupt handler. Thus, the perf trace function makes stack traces by using the return
address and frame pointer.

On Cortex-M, perf wraps the SysTick handler so it can sample the interrupted
Thread-mode Process Stack Pointer (PSP) frame before the normal timer ISR uses the
handler stack. The backend validates that frame before passing it to the Arm stack
walker.

The Cortex-M backend is unavailable for Non-secure Trusted Execution images because
Secure exception frames are inaccessible to Non-secure firmware.

The :zephyr_file:`scripts/profiling/stackcollapse.py` script can be used to convert return addresses
in the stack trace to function names using symbols from the ELF file, and to prints them in the
format expected by `FlameGraph`_.

Configuration
*************

You can configure this module using the following options:

* :kconfig:option:`CONFIG_PROFILING_PERF`: Enables stack sampling and the ``perf record`` shell
  command.

* :kconfig:option:`CONFIG_PROFILING_PERF_BUFFER_SIZE`: Sets the size of the perf buffer
  where samples are saved before printing.

* :kconfig:option:`CONFIG_PROFILING_PERF_EVENTS`: Enables the performance event subsystem for
  provider-based event discovery and event counting session management. Requires
  :kconfig:option:`CONFIG_PROFILING` and :kconfig:option:`CONFIG_MULTITHREADING`.

* :kconfig:option:`CONFIG_PROFILING_PERF_EVENTS_SHELL`: Adds counter discovery and session
  commands to the ``perf`` shell command. Requires :kconfig:option:`CONFIG_SHELL` and
  :kconfig:option:`CONFIG_CBPRINTF_FULL_INTEGRAL` to print 64-bit event counts. It does not require
  stack-sampling support or :kconfig:option:`CONFIG_PROFILING_PERF`. The event C APIs do not require
  full integer formatting.

Architecture backends may require additional stack-unwind support. The Cortex-M backend
requires SysTick, thread stack information, extra exception information, Arm stack
walking support, and a uniprocessor configuration.

Usage
*****

Refer to the :zephyr:code-sample:`profiling-perf` sample for an example of how to use the perf tool.

Event counting sessions
***********************

.. note::

   The performance event, provider, and portable PMU APIs are
   :ref:`experimental <api_lifecycle_experimental>` and may change in future releases.

Counter providers expose canonical ``provider.event`` names. A stat session takes a baseline when
it starts and a final snapshot when it stops. Only one stack-sampling or stat session can run at a
time.

For example:

.. code-block:: shell

   perf list
   perf list cpu0
   perf stat start -e cpu0.CPU_CYCLES
   # Run the workload.
   perf stat stop
   perf printbuf

Repeat ``-e <provider.event>`` to select additional events, up to
:kconfig:option:`CONFIG_PROFILING_PERF_EVENTS_MAX_EVENTS`. With the event shell enabled,
:kconfig:option:`CONFIG_SHELL_ARGC_MAX` must be at least
``3 + 2 * CONFIG_PROFILING_PERF_EVENTS_MAX_EVENTS``: three command words and two arguments per
event. The default eight events need 19 arguments; 16 events need 35. Set
:kconfig:option:`CONFIG_SHELL_CMD_BUFF_SIZE` large enough for the complete command text.

``perf stat stop`` prints and retains the completed result. ``perf printbuf`` prints and clears the
retained result, while ``perf clear`` discards it. Successfully starting a new ``perf stat`` or
``perf record`` session also discards the retained stat result.
``perf info`` reports ``Perf buffer: stat`` when a stat result is retained; otherwise, it reports
stack buffer usage when stack sampling is enabled, or ``Perf buffer: empty`` when it is disabled.

With the event shell enabled, perf commands are serialized across shell instances and CPUs.
A command finishes its output and cleanup before another perf command runs. The lock is released
between commands, including between ``perf stat start`` and ``perf stat stop``; a long
``perf printbuf`` can therefore delay other perf commands. A failed measurement start prints an
error with the returned error code.

Applications can resolve known event names with :c:func:`perf_event_lookup` and control a session
with :c:func:`perf_stat_start` and :c:func:`perf_stat_stop`. These supervisor-only APIs must be
called from thread context and do not depend on the shell or stack-sampling support.

Event tokens are opaque, build-local identifiers. Applications must obtain them through
:c:func:`perf_event_lookup`, must not modify them, and must not reuse them across firmware builds.
The core supports up to 255 providers, each with provider-local event IDs from 0 to 65535.

Provider callbacks are serialized. All selected events are prepared before any event is started.
If startup fails, the core calls each successfully prepared or started event's ``stop()`` callback
once and discards its final count. Providers must therefore accept ``stop()`` after successful
preparation even when measurement never started. A failing ``prepare()`` cleans up its own partial
state; a failing ``start()`` undoes its partial activation. Providers without ``prepare()`` must
release all resources themselves when ``start()`` fails.

Event providers
***************

See the provider documentation for configuration, supported events, and measurement limitations.

.. toctree::
   :maxdepth: 1
   :glob:

   providers/*

API reference
*************

.. doxygengroup:: profiling_perf

.. _FlameGraph: https://github.com/brendangregg/FlameGraph/
