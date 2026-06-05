.. _profiling-perf:

Perf
####

Perf is a profiler tool based on stack tracing. It can be used for lightweight profiling
with minimal code overhead.

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

* :kconfig:option:`CONFIG_PROFILING_PERF`: Enables the module. This option adds
  the ``perf`` command to the shell.

* :kconfig:option:`CONFIG_PROFILING_PERF_BUFFER_SIZE`: Sets the size of the perf buffer
  where samples are saved before printing.

Architecture backends may require additional stack-unwind support. The Cortex-M backend
requires SysTick, frame pointers, thread stack information, extra exception information,
and Arm stack walking support.

Usage
*****

Refer to the :zephyr:code-sample:`profiling-perf` sample for an example of how to use the perf tool.

 .. _FlameGraph: https://github.com/brendangregg/FlameGraph/
