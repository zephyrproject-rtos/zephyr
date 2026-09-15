.. _profiling_irq_stats:

Per-interrupt statistics
########################

:kconfig:option:`CONFIG_IRQ_STATS` collects statistics from the interrupt
dispatch path, per software ISR table index and per CPU: how often each
interrupt fired, the total CPU cycles spent in its handler, and the
longest single invocation — Zephyr's equivalent of ``/proc/interrupts``.

The ``irqstats`` shell command (enabled by default with
:kconfig:option:`CONFIG_SHELL`) prints the collected statistics, resolving
handler names when :kconfig:option:`CONFIG_SYMTAB` is enabled, and
``irqstats reset`` clears them:

.. code-block:: none

    uart:~$ irqstats
     IRQ      COUNT    TOTAL(us)    MAX(us)  HANDLER
       7     294810      4830212         39  timer_isr
      11        898        26878         69  plic_irq_handler
      22        921        17724         58  uart_ns16550_isr

``irqstats show <min_count>`` restricts the output to interrupts that
fired more than ``min_count`` times. The :c:func:`irq_stats_get`,
:c:func:`irq_stats_get_cpu`, :c:func:`irq_stats_reset_irq` and
:c:func:`irq_stats_reset` API allows programmatic access, e.g. asserting
interrupt-load budgets from tests.

Counters are kept per CPU, so each CPU only writes its own slots and no
updates are lost when the same vector is serviced concurrently; the
aggregate figures are summed on read. On SMP the shell adds a per-CPU
count column.

Statistics are recorded where the architecture dispatches through the
software ISR table (currently Cortex-M and RISC-V, including per-line
attribution below the RISC-V PLIC). Interrupts that bypass the table —
direct ISRs, Cortex-M system exceptions such as SysTick and PendSV — are
not counted. Durations are gross: time spent in a nested interrupt is
attributed to both the nested handler and the one it preempted.

Relation to the PLIC shell
**************************

The RISC-V PLIC driver's ``plic stats get`` command
(:kconfig:option:`CONFIG_PLIC_SHELL_IRQ_COUNT`) reports the same
counters, presented per PLIC line rather than per software ISR table
index, and now depends on :kconfig:option:`CONFIG_IRQ_STATS` rather than
keeping its own. That removes a duplicate set of counters from the
interrupt hot path, lifts the previous 16-bit saturation of the PLIC hit
counts, and adds handler timing to that view.

API Reference
*************

.. doxygengroup:: irq_stats
