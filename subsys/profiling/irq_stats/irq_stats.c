/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/profiling/irq_stats.h>
#include <string.h>

#ifdef CONFIG_IRQ_STATS_SHELL
#include <zephyr/shell/shell.h>
#include <zephyr/sw_isr_table.h>
#ifdef CONFIG_SYMTAB
#include <zephyr/debug/symtab.h>
#endif
#endif

/*
 * Statistics are kept per CPU so that each CPU only writes its own
 * slots: no atomics are needed in the dispatch path, and no updates are
 * lost when the same vector is serviced concurrently on several CPUs.
 * Aggregate figures are computed when read.
 */
static struct irq_stats_entry irq_stats[CONFIG_MP_MAX_NUM_CPUS][CONFIG_NUM_IRQS];

/*
 * Per-CPU stack of in-flight handlers: a nested interrupt pushes on
 * top of the one it preempted. Entries deeper than the stack are
 * dispatched normally but not measured.
 */
struct irq_stats_frame {
	uint32_t irq;
	uint32_t enter_cycles;
};

struct irq_stats_cpu {
	struct irq_stats_frame stack[CONFIG_IRQ_STATS_MAX_NESTING];
	uint8_t depth;
	uint8_t overflow;
};

static struct irq_stats_cpu irq_stats_cpus[CONFIG_MP_MAX_NUM_CPUS];

void z_irq_stats_enter(unsigned int irq)
{
	struct irq_stats_cpu *cpu = &irq_stats_cpus[arch_curr_cpu()->id];

	if (irq >= CONFIG_NUM_IRQS || cpu->depth >= CONFIG_IRQ_STATS_MAX_NESTING) {
		cpu->overflow++;
		return;
	}

	cpu->stack[cpu->depth].irq = irq;
	cpu->stack[cpu->depth].enter_cycles = k_cycle_get_32();
	cpu->depth++;
}

void z_irq_stats_exit(void)
{
	unsigned int cpu_id = arch_curr_cpu()->id;
	struct irq_stats_cpu *cpu = &irq_stats_cpus[cpu_id];
	struct irq_stats_frame *frame;
	struct irq_stats_entry *entry;
	uint32_t delta;

	if (cpu->overflow != 0U) {
		/* This exit belongs to an unmeasured enter */
		cpu->overflow--;
		return;
	}
	if (cpu->depth == 0U) {
		return;
	}

	cpu->depth--;
	frame = &cpu->stack[cpu->depth];
	delta = k_cycle_get_32() - frame->enter_cycles;

	/*
	 * A handler cannot migrate between enter and exit, so this is
	 * the CPU that pushed the frame and owns these counters.
	 */
	entry = &irq_stats[cpu_id][frame->irq];
	entry->count++;
	entry->total_cycles += delta;
	if (delta > entry->max_cycles) {
		entry->max_cycles = delta;
	}
}

int irq_stats_get_cpu(unsigned int irq, unsigned int cpu, struct irq_stats_entry *entry)
{
	if (irq >= CONFIG_NUM_IRQS || cpu >= CONFIG_MP_MAX_NUM_CPUS || entry == NULL) {
		return -EINVAL;
	}

	unsigned int key = irq_lock();

	*entry = irq_stats[cpu][irq];
	irq_unlock(key);

	return 0;
}

int irq_stats_get(unsigned int irq, struct irq_stats_entry *entry)
{
	if (irq >= CONFIG_NUM_IRQS || entry == NULL) {
		return -EINVAL;
	}

	memset(entry, 0, sizeof(*entry));

	unsigned int key = irq_lock();

	for (unsigned int cpu = 0; cpu < CONFIG_MP_MAX_NUM_CPUS; cpu++) {
		const struct irq_stats_entry *e = &irq_stats[cpu][irq];

		entry->count += e->count;
		entry->total_cycles += e->total_cycles;
		if (e->max_cycles > entry->max_cycles) {
			entry->max_cycles = e->max_cycles;
		}
	}
	irq_unlock(key);

	return 0;
}

int irq_stats_reset_irq(unsigned int irq)
{
	if (irq >= CONFIG_NUM_IRQS) {
		return -EINVAL;
	}

	unsigned int key = irq_lock();

	for (unsigned int cpu = 0; cpu < CONFIG_MP_MAX_NUM_CPUS; cpu++) {
		memset(&irq_stats[cpu][irq], 0, sizeof(irq_stats[cpu][irq]));
	}
	irq_unlock(key);

	return 0;
}

void irq_stats_reset(void)
{
	unsigned int key = irq_lock();

	memset(irq_stats, 0, sizeof(irq_stats));
	irq_unlock(key);
}

#ifdef CONFIG_IRQ_STATS_SHELL

static int cmd_irq_stats_show(const struct shell *sh, size_t argc, char **argv)
{
	unsigned int min_count = 0;
	bool any = false;
	int ret = 0;

	if (argc > 1) {
		min_count = shell_strtoul(argv[1], 10, &ret);
		if (ret != 0) {
			shell_error(sh, "Failed to parse %s: %d", argv[1], ret);
			return ret;
		}
	}

	shell_fprintf(sh, SHELL_NORMAL, "%4s", "IRQ");
	if (CONFIG_MP_MAX_NUM_CPUS > 1) {
		for (unsigned int cpu = 0; cpu < arch_num_cpus(); cpu++) {
			shell_fprintf(sh, SHELL_NORMAL, " %6s%2u", "CPU", cpu);
		}
	}
	shell_fprintf(sh, SHELL_NORMAL, " %10s %12s %10s  %s\n", "COUNT", "TOTAL(us)", "MAX(us)",
		      "HANDLER");

	for (unsigned int irq = 0; irq < CONFIG_NUM_IRQS; irq++) {
		struct irq_stats_entry entry;
		const char *name = "";

		(void)irq_stats_get(irq, &entry);
		if (entry.count == 0U || entry.count <= min_count) {
			continue;
		}
		any = true;

		shell_fprintf(sh, SHELL_NORMAL, "%4u", irq);
		if (CONFIG_MP_MAX_NUM_CPUS > 1) {
			for (unsigned int cpu = 0; cpu < arch_num_cpus(); cpu++) {
				struct irq_stats_entry per_cpu;

				(void)irq_stats_get_cpu(irq, cpu, &per_cpu);
				shell_fprintf(sh, SHELL_NORMAL, " %8u", per_cpu.count);
			}
		}

#ifdef CONFIG_SYMTAB
		name = symtab_find_symbol_name((uintptr_t)_sw_isr_table[irq].isr, NULL);
#endif
		shell_fprintf(sh, SHELL_NORMAL, " %10u %12llu %10u  %s\n", entry.count,
			      (unsigned long long)k_cyc_to_us_floor64(entry.total_cycles),
			      (uint32_t)k_cyc_to_us_floor32(entry.max_cycles), name);
	}

	if (!any) {
		shell_print(sh, "(no interrupts recorded)");
	}

	return 0;
}

static int cmd_irq_stats_reset(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	irq_stats_reset();
	shell_print(sh, "Interrupt statistics reset");

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_irq_stats,
			       SHELL_CMD_ARG(show, NULL,
					     "Show per-interrupt statistics\n"
					     "Usage: show [min_count]",
					     cmd_irq_stats_show, 1, 1),
			       SHELL_CMD_ARG(reset, NULL, "Reset per-interrupt statistics",
					     cmd_irq_stats_reset, 1, 0),
			       SHELL_SUBCMD_SET_END);
SHELL_CMD_ARG_REGISTER(irqstats, &sub_irq_stats, "Per-interrupt statistics", cmd_irq_stats_show, 1,
		       1);

#endif /* CONFIG_IRQ_STATS_SHELL */
