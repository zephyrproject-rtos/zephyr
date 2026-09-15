/*
 * Copyright (c) 2023 Intel Corporation
 * Copyright (c) 2026 The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/toolchain.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/arch/xtensa/smp.h>
#include <zephyr/zsr.h>
#include <ipi.h>
#include <ksched.h>
#include <kswap.h>

/* Only built when CONFIG_SMP=y (see arch/xtensa/core/CMakeLists.txt), so
 * no #ifdef CONFIG_SMP needed in this file.
 */

#ifdef CONFIG_XTENSA_MORE_SPIN_RELAX_NOPS
/* Some compilers might "optimize out" (i.e. remove) continuous NOPs.
 * So force no optimization to avoid that.
 */
__no_optimization
void arch_spin_relax(void)
{
#define NOP1(_, __) __asm__ volatile("nop.n;");
	LISTIFY(CONFIG_XTENSA_NUM_SPIN_RELAX_NOPS, NOP1, (;))
#undef NOP1
}
#endif /* CONFIG_XTENSA_MORE_SPIN_RELAX_NOPS */

struct xtensa_cpustart_rec {
	arch_cpustart_t fn;
	void *arg;
	char *stack_top;
};

static struct xtensa_cpustart_rec cpustart_rec[CONFIG_MP_MAX_NUM_CPUS];

/* True once a core has reached xtensa_smp_secondary_start() (or, for
 * core 0, unconditionally from arch_smp_init()). Doubles as the
 * "active" bitmap arch_sched_directed_ipi()/arch_cpu_active() consult,
 * and as the handshake arch_cpu_start() spins on -- every prior
 * per-SoC implementation (ESP32's alive_flag, Intel's
 * soc_cpus_active[]) had its own copy of exactly this.
 */
static bool cpu_active[CONFIG_MP_MAX_NUM_CPUS];

/**
 * @brief Stack top for a core that has been told to start but hasn't
 *        reached xtensa_smp_secondary_start() yet.
 *
 * SoC-specific boot trampolines run in assembly before any C stack
 * exists, so they need this value to set up SP themselves prior to
 * calling into xtensa_smp_secondary_start(). Exposed as an accessor
 * rather than a bare global so multi-secondary-core SoCs don't need
 * one global per core the way the current single-APPCPU ESP32 code
 * does (static void *appcpu_top).
 */
void *xtensa_smp_cpu_stack_top(int cpu_num)
{
	return cpustart_rec[cpu_num].stack_top;
}

void arch_cpu_start(int cpu_num, k_thread_stack_t *stack, int sz, arch_cpustart_t fn, void *arg)
{
	__ASSERT(cpu_num > 0 && cpu_num < CONFIG_MP_MAX_NUM_CPUS, "invalid secondary core %d",
		 cpu_num);
	__ASSERT_NO_MSG(!cpu_active[cpu_num]);

	cpustart_rec[cpu_num].fn = fn;
	cpustart_rec[cpu_num].arg = arg;
	cpustart_rec[cpu_num].stack_top = K_KERNEL_STACK_BUFFER(stack) + sz;

	/* soc_mp_start_core() only releases/resets the target core; it
	 * does not wait for it to come up. The completion handshake is
	 * cpu_active[cpu_num], set from *inside* xtensa_smp_secondary_start()
	 * -- i.e. by code actually running on cpu_num, not guessed at by
	 * the initiating core.
	 */
	soc_mp_start_core(cpu_num);

	while (!cpu_active[cpu_num]) {
		arch_spin_relax();
	}

	/* Not fully explained, but confirmed to help on ESP32: without a settle
	 * delay here, right after both cores are confirmed up and before either
	 * touches anything flash-heavy (e.g. NVS/settings), APPCPU intermittently
	 * hangs or the board hits a spurious watchdog reset later in boot. Both
	 * symptoms went away, repeatedly, once this delay was added. Same
	 * category of unexplained-but-load-bearing fix as esp32-mp.c's
	 * smp_log() comment, different mechanism -- leave in place until
	 * understood.
	 */
	k_busy_wait(100000);
}

FUNC_NORETURN void xtensa_smp_secondary_start(int cpu_num)
{
	_cpu_t *cpu = &_kernel.cpus[cpu_num];

	__asm__ volatile("wsr %0, " ZSR_CPU_STR : : "r"(cpu));

	/* _current must be valid before soc_mp_startup_self() runs, since that hook can call
	 * arbitrary driver code -- our esp32 port's IPI registration calls esp_intr_alloc(),
	 * which calls the generic irq_lock() -> under CONFIG_SMP that's z_smp_global_lock(),
	 * which dereferences _current->base.global_lock_count. PRO_CPU's own analogous call
	 * (arch_smp_init() -> soc_mp_startup_self(0)) already works because z_cstart()
	 * (kernel/init.c) calls z_dummy_thread_init() *before* ever reaching that call -- this
	 * mirrors that ordering for secondary cores, which previously only got their dummy
	 * thread set up later, inside smp_init_top() (kernel/smp/smp.c), i.e. *after*
	 * soc_mp_startup_self() already ran (which meant it never returned).
	 */
	z_dummy_thread_init(&_thread_dummy);

	/* Must run before cpu_active[cpu_num] is set: this is what
	 * registers and enables cpu_num's own incoming IPI line. If a
	 * directed IPI could be sent to this core before that, it would
	 * be silently lost.
	 */
	soc_mp_startup_self(cpu_num);

	cpu_active[cpu_num] = true;

	cpustart_rec[cpu_num].fn(cpustart_rec[cpu_num].arg);

	__ASSERT(false, "arch_cpu_start() handler should never return");
	for (;;) {
	}
}

bool arch_cpu_active(int cpu_num)
{
	return cpu_active[cpu_num];
}

void arch_sched_directed_ipi(uint32_t cpu_bitmap)
{
	unsigned int key = arch_irq_lock();
	unsigned int self = _current_cpu->id;
	unsigned int num_cpus = arch_num_cpus();

	for (unsigned int i = 0; i < num_cpus; i++) {
		if (i != self && cpu_active[i] && (cpu_bitmap & BIT(i)) != 0) {
			soc_ipi_trigger(i);
		}
	}

	arch_irq_unlock(key);
}

void arch_sched_broadcast_ipi(void)
{
	arch_sched_directed_ipi(IPI_ALL_CPUS_MASK);
}

void xtensa_smp_ipi_isr(void *arg)
{
	ARG_UNUSED(arg);

	soc_ipi_clear();
	z_sched_ipi();
}

int arch_smp_init(void)
{
	/* Core 0 is running Zephyr by definition and never passes through
	 * xtensa_smp_secondary_start() -- but it still needs its own
	 * incoming IPI line registered and enabled, the same as every
	 * other core. Reusing soc_mp_startup_self() here (rather than a
	 * fifth, core-0-only hook) keeps that one hook symmetric across
	 * every core in CONFIG_MP_MAX_NUM_CPUS.
	 */
	soc_mp_startup_self(0);
	cpu_active[0] = true;

	return 0;
}
