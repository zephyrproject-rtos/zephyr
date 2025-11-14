.. _instrumentation:

Instrumentation
###############

Overview
********

The instrumentation subsystem provides compiler-managed runtime system instrumentation capabilities
for Zephyr applications. It enables developers to trace function calls, observe context switches,
and profile application performance with minimal manual instrumentation effort.

Unlike the :ref:`tracing <tracing>` subsystem, which provides RTOS-aware tracing with structured
event APIs, the instrumentation subsystem works at a lower level by leveraging compiler
instrumentation hooks. This approach makes it possible to capture virtually any function entry and
exit events without requiring manual tracing calls in the code.

.. admonition:: Tracing vs. Instrumentation
   :class: hint

   **When to use Tracing**: Choose the tracing subsystem when you need RTOS-aware event tracing
   (e.g. thread switches, semaphore operations, etc.) and want to minimize overhead.

   **When to use Instrumentation**: Choose instrumentation when you need a detailed view of
   function-level execution to better understand code flow, or to identify performance bottlenecks
   without adding manual trace points.

The instrumentation subsystem relies on compiler support for automatic function instrumentation.
When enabled, the compiler automatically inserts calls to special instrumentation handler functions
at the entry and exit of every function in your application (excluding those explicitly marked with
``__no_instrumentation__``). Currently, only GCC is supported with the ``-finstrument-functions``
compiler flag.

The subsystem initializes automatically after RAM initialization and uses trigger/stopper functions
to control when recording is active. The default trigger and stopper functions are both set to
``main()`` (configurable via Kconfig), meaning instrumentation captures the entire execution from
when ``main()`` starts until it returns.

The recorded data is stored in RAM and can be accessed from a host computer thanks to a UART backend
that exposes a set of simple commands. :zephyr_file:`scripts/instrumentation/zaru.py` script allows
to execute these commands through a high-level command-line interface and makes it easy to obtain
data in a format suitable for further analysis (e.g. using `Perfetto`_).

Operational Modes
*****************

The instrumentation subsystem supports two modes that can be enabled independently or together:

Callgraph Mode (Tracing)
========================

In callgraph mode (enabled with :kconfig:option:`CONFIG_INSTRUMENTATION_MODE_CALLGRAPH`), the
subsystem records function entry and exit events along with timestamps and context information in a
memory buffer. This enables:

- Reconstruction of the complete function call graph
- Observation of thread context switches
- Analysis of execution flow and timing relationships

The trace buffer can operate in ring buffer mode (default, overwrites old entries) or fixed buffer
mode (stops when full). Buffer size is configurable via
:kconfig:option:`CONFIG_INSTRUMENTATION_MODE_CALLGRAPH_TRACE_BUFFER_SIZE`.

.. code-block:: console
   :caption: Example of callgraph mode output. See :ref:`zaru_usage` for more details.

   $ ./scripts/instrumentation/zaru.py trace

      Thread Name      Thread ID  CPU  Mode     Timestamp          Function(s)
   ------------------------------------------------------------------------------------------------
               ... (truncated) ...

               main    0x20001a38   0)    0 |    187837720 ns |               sys_dlist_append();
               main    0x20001a38   0)    0 |    188802680 ns |             };   /* z_priq_simple_add */
               main    0x20001a38   0)    0 |    189282840 ns |           };   /* add_to_waitq_locked */
               main    0x20001a38   0)    0 |    189770000 ns |           add_thread_timeout();
               main    0x20001a38   0)    0 |    190732920 ns |         };   /* pend_locked */
               main    0x20001a38   0)    0 |    191198480 ns |         k_spin_release();
               main    0x20001a38   0)    0 |    192125560 ns |         z_swap() {
               main    0x20001a38   0)    0 |    192590080 ns |           k_spin_release();
               main    0x20001a38   0)    0 |    193520000 ns |           z_swap_irqlock() {
               main    0x20001a38   0)    0 |    193987840 ns |             __set_BASEPRI() {
               main    0x20001a38   0)    0 |    194474640 ns | /* --> Scheduler switched OUT from thread 'main' */
        thread-none   none-thread   0)    0 |    195178000 ns | /* <-- Scheduler switched IN thread 'thread-none' */
        thread-none   none-thread   0)    0 |    195851520 ns | z_thread_entry() {
        thread-none   none-thread   0)    0 |    196312600 ns |   k_sched_current_thread_query() {
        thread-none   none-thread   0)    0 |    196774680 ns |     z_impl_k_sched_current_thread_query();
        thread-none   none-thread   0)    0 |    197694480 ns |   };   /* k_sched_current_thread_query */
           thread_A    0x200000d8   0)    7 |    198160000 ns | thread_A() {
           thread_A    0x200000d8   0)    7 |    198443400 ns |   get_sem_and_exec_function() {
           thread_A    0x200000d8   0)    7 |    198727440 ns |     k_sem_take() {
           thread_A    0x200000d8   0)    7 |    199011840 ns |       z_impl_k_sem_take() {
           thread_A    0x200000d8   0)    7 |    199397520 ns |         k_spin_lock() {
           thread_A    0x200000d8   0)    7 |    199784200 ns |           __get_BASEPRI();
           thread_A    0x200000d8   0)    7 |    200557840 ns |           __set_BASEPRI_MAX();
           thread_A    0x200000d8   0)    7 |    201333640 ns |           __ISB();
           thread_A    0x200000d8   0)    7 |    202111360 ns |           z_spinlock_validate_pre();
           thread_A    0x200000d8   0)    7 |    202891000 ns |           z_spinlock_validate_post();
           thread_A    0x200000d8   0)    7 |    203664760 ns |         };   /* k_spin_lock */
           thread_A    0x200000d8   0)    7 |    204058000 ns |         k_spin_unlock() {
           thread_A    0x200000d8   0)    7 |    204450840 ns |           __set_BASEPRI();
           thread_A    0x200000d8   0)    7 |    205231640 ns |           __ISB();
           thread_A    0x200000d8   0)    7 |    206009600 ns |         };   /* k_spin_unlock */
           thread_A    0x200000d8   0)    7 |    206291600 ns |       };   /* z_impl_k_sem_take */
           thread_A    0x200000d8   0)    7 |    206572920 ns |     };   /* k_sem_take */

           ... (truncated) ...

Statistical Mode (Profiling)
============================

In statistical mode (enabled with :kconfig:option:`CONFIG_INSTRUMENTATION_MODE_STATISTICAL`), the
subsystem accumulates timing statistics for each unique function executed between the trigger and
stopper points. This provides total execution time per function and helps identify performance
bottlenecks. The subsystem tracks up to
:kconfig:option:`CONFIG_INSTRUMENTATION_MODE_STATISTICAL_MAX_NUM_FUNC` unique functions.

.. code-block:: console
   :caption: Example of statistical mode output (top 10 most expensive functions). See
             :ref:`zaru_usage` for more details.

   $ ./scripts/instrumentation/zaru.py profile -n 10

   9.45% 0000061d main
   6.00% 0000049d k_msleep
   5.98% 00000469 k_sleep
   5.95% 0000aea1 z_impl_k_sleep
   5.93% 0000ad6d z_tick_sleep
   5.66% 00000431 k_sem_take
   5.65% 00007e65 z_impl_k_sem_take
   5.51% 0000ac29 z_pend_curr
   2.83% 000063ed sys_clock_isr
   2.67% 0000d361 sys_clock_announce

Configuration
*************

Enable instrumentation with:

.. code-block:: cfg

   CONFIG_INSTRUMENTATION=y
   CONFIG_INSTRUMENTATION_MODE_CALLGRAPH=y    # For tracing
   CONFIG_INSTRUMENTATION_MODE_STATISTICAL=y  # For profiling

The instrumentation subsystem uses :ref:`retained memory <retention_api>` to persist trigger/stopper
function addresses across reboots. This must be configured in the devicetree:

.. code-block:: devicetree

   / {
       sram@2003FC00 {
           compatible = "zephyr,memory-region", "mmio-sram";
           reg = <0x2003FC00 DT_SIZE_K(1)>;
           zephyr,memory-region = "RetainedMem";

           retainedmem {
               compatible = "zephyr,retained-ram";
               status = "okay";

               instrumentation_triggers: retention@0 {
                   compatible = "zephyr,retention";
                   status = "okay";
                   reg = <0x0 0x10>;
               };
           };
       };
   };

   /* Adjust main SRAM to exclude retained region */
   &sram0 {
       reg = <0x20000000 DT_SIZE_K(255)>;
   };

See the :zephyr:code-sample:`instrumentation` sample for complete configuration examples.
Additional options include buffer sizes, trigger functions, and function/file exclusion lists (see
Kconfig options starting with :kconfig:option-regex:`CONFIG_INSTRUMENTATION_*`).

.. _zaru_usage:

``zaru.py`` Usage
*****************

The ``zaru.py`` command-line tool (located in :zephyr_file:`scripts/instrumentation/zaru.py`)
provides an interface for controlling instrumentation and extracting data from the target over UART.

The tool offers several commands:

- ``status``: Check if the target device supports callgraph (tracing) and statistical (profiling)
  modes.
- ``trace``: Capture and display function call traces.
- ``profile``: Capture and display function profiling data.
- ``reboot``: Reboot the target device.

You can get help for each command by running ``zaru.py <command> --help``.

By default, ``zaru.py`` attempts to connect to the target device using ``/dev/ttyACM0``. You can
specify a different serial port using the ``--serial`` option:

.. code-block:: console

   $ ./scripts/instrumentation/zaru.py --serial /dev/ttyACM1 status

The ``--build-dir`` option can be used to specify the Zephyr build directory, which is needed to
locate the ELF file for symbol resolution. If not provided, ``zaru.py`` will attempt to find it
automatically.

See the :zephyr:code-sample:`instrumentation` sample documentation for detailed usage instructions.

Limitations and Considerations
******************************

Compiler support
  The instrumentation subsystem requires GCC with ``-finstrument-functions`` support. Other
  compilers are not supported.

Stack size requirements
  Instrumentation adds overhead to every function call, which increases stack usage. You will likely
  need to increase thread stack sizes to accommodate the additional space required by instrumentation
  handlers and nested function calls.

Execution overhead
  All function calls incur instrumentation overhead. Code size will increase due to added
  instrumentation calls, and performance will be impacted.

Initialization constraints
  Code that runs before RAM initialization (e.g., early boot functions) is not captured as it runs
  before the instrumentation subsystem is initialized.

To reduce overhead, use trigger/stopper functions to instrument only code regions of interest, and
exclude performance-critical functions via
:kconfig:option:`CONFIG_INSTRUMENTATION_EXCLUDE_FUNCTION_LIST` and
:kconfig:option:`CONFIG_INSTRUMENTATION_EXCLUDE_FILE_LIST`.

API Reference
*************

.. doxygengroup:: instrumentation_api

.. _Perfetto: https://perfetto.dev/
