/*
 * Copyright 2020 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

/**
 * @file
 * @brief codes required for AArch64 multicore and Zephyr smp support
 */

#include <string.h>
#include <zephyr/cache.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/kernel_structs.h>
#include <kernel_arch_interface.h>
#include <kernel_arch_func.h>
#include <ipi.h>
#include <zephyr/init.h>
#include <zephyr/arch/arm64/mm.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/arch/exception.h>
#include <zephyr/drivers/interrupt_controller/gic.h>
#include <zephyr/drivers/pm_cpu_ops.h>
#include <zephyr/arch/arch_interface.h>
#include <zephyr/platform/hooks.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/irq.h>
#include "boot.h"

#define INV_MPID	UINT64_MAX

#define SGI_SCHED_IPI	0
#define SGI_MMCFG_IPI	1
#define SGI_FPU_IPI	2
#define SGI_COREDUMP_FREEZE_IPI	3

/*
 * Deliberately the lowest usable priority (numerically largest, excluding
 * the reserved GIC_IDLE_PRIO=0xff sentinel), NOT IRQ_DEFAULT_PRIORITY.
 * A frozen CPU spins inside this SGI's handler without returning, so
 * _isr_wrapper() never reaches its EOI call for it -- the GIC keeps
 * treating it as the active interrupt on that CPU for the entire freeze
 * window. Per GIC priority rules, only a STRICTLY HIGHER-priority
 * interrupt can preempt a still-active one; anything at the same or
 * lower priority is held pending until this one is EOI'd. At
 * IRQ_DEFAULT_PRIORITY (same as most drivers, e.g. this board's Ethernet
 * MAC TX-done interrupt), that meant a frozen CPU couldn't service any
 * same-priority peripheral interrupt for as long as it stayed frozen --
 * confirmed on hardware as "ethernet_mac TX confirmation timed out" /
 * "cannot TX ... current free count = 0" during hs6_smp_udp_backend,
 * because the coredump UDP backend's own TX completions couldn't be
 * serviced on a CPU frozen at the same priority. Using the lowest
 * priority here means everything else in the system can always preempt
 * a frozen CPU, so drivers keep making progress normally while it waits.
 */
#define SGI_COREDUMP_FREEZE_PRIO	0xf0

struct boot_params {
	uint64_t mpid;
	char *sp;
	uint8_t voting[DT_CHILD_NUM_STATUS_OKAY(DT_PATH(cpus))];
	arch_cpustart_t fn;
	void *arg;
	int cpu_num;
};

/* Offsets used in reset.S */
BUILD_ASSERT(offsetof(struct boot_params, mpid) == BOOT_PARAM_MPID_OFFSET);
BUILD_ASSERT(offsetof(struct boot_params, sp) == BOOT_PARAM_SP_OFFSET);
BUILD_ASSERT(offsetof(struct boot_params, voting) == BOOT_PARAM_VOTING_OFFSET);

volatile struct boot_params __aligned(L1_CACHE_BYTES) arm64_cpu_boot_params = {
	.mpid = -1,
};

const uint64_t cpu_node_list[] = {
	DT_FOREACH_CPU_STATUS_OKAY_SEP(DT_REG_ADDR, (,))
};

BUILD_ASSERT(ARRAY_SIZE(cpu_node_list) == DT_CHILD_NUM_STATUS_OKAY(DT_PATH(cpus)));

/* cpu_map saves the mapping of core id and mpid */
static uint64_t cpu_map[CONFIG_MP_MAX_NUM_CPUS] = {
	[0 ... (CONFIG_MP_MAX_NUM_CPUS - 1)] = INV_MPID
};

/* Called from Zephyr initialization */
void arch_cpu_start(int cpu_num, k_thread_stack_t *stack, int sz,
		    arch_cpustart_t fn, void *arg)
{
	int cpu_count;
	static int i;
	uint64_t cpu_mpid = 0;
	uint64_t primary_core_mpid;

	/* Now it is on primary core */
	__ASSERT(arch_curr_cpu()->id == 0, "");
	primary_core_mpid = MPIDR_TO_CORE(GET_MPIDR());

	cpu_count = ARRAY_SIZE(cpu_node_list);

#ifdef CONFIG_ARM64_FALLBACK_ON_RESERVED_CORES
	__ASSERT(cpu_count >= CONFIG_MP_MAX_NUM_CPUS,
		"The count of CPU Core nodes in dts is not greater or equal to CONFIG_MP_MAX_NUM_CPUS\n");
#else
	__ASSERT(cpu_count == CONFIG_MP_MAX_NUM_CPUS,
		"The count of CPU Cores nodes in dts is not equal to CONFIG_MP_MAX_NUM_CPUS\n");
#endif

	arm64_cpu_boot_params.sp = K_KERNEL_STACK_BUFFER(stack) + sz;
	arm64_cpu_boot_params.fn = fn;
	arm64_cpu_boot_params.arg = arg;
	arm64_cpu_boot_params.cpu_num = cpu_num;

	for (; i < cpu_count; i++) {
		if (cpu_node_list[i] == primary_core_mpid) {
			continue;
		}

		cpu_mpid = cpu_node_list[i];

		barrier_dsync_fence_full();

		/* store mpid last as this is our synchronization point */
		arm64_cpu_boot_params.mpid = cpu_mpid;

		sys_cache_data_flush_range((void *)&arm64_cpu_boot_params,
					  sizeof(arm64_cpu_boot_params));

		if (pm_cpu_on(cpu_mpid, (uint64_t)&__start)) {
			printk("Failed to boot secondary CPU core %d (MPID:%#llx)\n",
			       cpu_num, cpu_mpid);
#ifdef CONFIG_ARM64_FALLBACK_ON_RESERVED_CORES
			printk("Falling back on reserved cores\n");
			continue;
#else
			k_panic();
#endif
		}

		break;
	}
	if (i++ == cpu_count) {
		printk("Can't find CPU Core %d from dts and failed to boot it\n", cpu_num);
		k_panic();
	}

	/* Wait secondary cores up, see arch_secondary_cpu_init */
	while (arm64_cpu_boot_params.fn) {
		wfe();
	}

	cpu_map[cpu_num] = cpu_mpid;

	printk("Secondary CPU core %d (MPID:%#llx) is up\n", cpu_num, cpu_mpid);
}

/* the C entry of secondary cores */
FUNC_NORETURN void arch_secondary_cpu_init(void)
{
	int cpu_num = arm64_cpu_boot_params.cpu_num;
	arch_cpustart_t fn;
	void *arg;

	__ASSERT(arm64_cpu_boot_params.mpid == MPIDR_TO_CORE(GET_MPIDR()), "");

	/* Initialize tpidrro_el0 with our struct _cpu instance address */
	write_tpidrro_el0((uintptr_t)&_kernel.cpus[cpu_num]);

	z_arm64_mm_init(false);

#ifdef CONFIG_ARM64_SAFE_EXCEPTION_STACK
	z_arm64_safe_exception_stack_init();
#endif

#ifdef CONFIG_SMP
	arm_gic_secondary_init();

	irq_enable(SGI_SCHED_IPI);
#ifdef CONFIG_USERSPACE
	irq_enable(SGI_MMCFG_IPI);
#endif
#ifdef CONFIG_FPU_SHARING
	irq_enable(SGI_FPU_IPI);
#endif
#ifdef CONFIG_DEBUG_COREDUMP_SMP_FREEZE_CPUS
	irq_enable(SGI_COREDUMP_FREEZE_IPI);
#endif
#endif

	soc_per_core_init_hook();

	fn = arm64_cpu_boot_params.fn;
	arg = arm64_cpu_boot_params.arg;
	barrier_dsync_fence_full();

	/*
	 * Secondary core clears .fn to announce its presence.
	 * Primary core is polling for this. We no longer own
	 * arm64_cpu_boot_params afterwards.
	 */
	arm64_cpu_boot_params.fn = NULL;
	barrier_dsync_fence_full();
	sev();

	fn(arg);

	CODE_UNREACHABLE;
}

#ifdef CONFIG_SMP

static void send_ipi(unsigned int ipi, uint32_t cpu_bitmap)
{
	uint64_t mpidr = MPIDR_TO_CORE(GET_MPIDR());

	/*
	 * Send SGI to all cores except itself
	 */
	unsigned int num_cpus = arch_num_cpus();

	for (int i = 0; i < num_cpus; i++) {
		if ((cpu_bitmap & BIT(i)) == 0) {
			continue;
		}

		uint64_t target_mpidr = cpu_map[i];
		uint8_t aff0;

		if (mpidr == target_mpidr || target_mpidr == INV_MPID) {
			continue;
		}

		aff0 = MPIDR_AFFLVL(target_mpidr, 0);
		gic_raise_sgi(ipi, target_mpidr, 1 << aff0);
	}
}

void sched_ipi_handler(const void *unused)
{
	ARG_UNUSED(unused);

	z_sched_ipi();
}

void arch_sched_broadcast_ipi(void)
{
	send_ipi(SGI_SCHED_IPI, IPI_ALL_CPUS_MASK);
}

void arch_sched_directed_ipi(uint32_t cpu_bitmap)
{
	send_ipi(SGI_SCHED_IPI, cpu_bitmap);
}

#ifdef CONFIG_USERSPACE
void mem_cfg_ipi_handler(const void *unused)
{
	ARG_UNUSED(unused);
	unsigned int key = arch_irq_lock();

	/*
	 * Make sure a domain switch by another CPU is effective on this CPU.
	 * This is a no-op if the page table is already the right one.
	 * Lock irq to prevent the interrupt during mem region switch.
	 */
	z_arm64_swap_mem_domains(_current);
	arch_irq_unlock(key);
}

void z_arm64_mem_cfg_ipi(void)
{
	send_ipi(SGI_MMCFG_IPI, IPI_ALL_CPUS_MASK);
}
#endif

#ifdef CONFIG_FPU_SHARING
void flush_fpu_ipi_handler(const void *unused)
{
	ARG_UNUSED(unused);

	disable_irq();
	arch_flush_local_fpu();
	/* no need to re-enable IRQs here */
}

void arch_flush_fpu_ipi(unsigned int cpu)
{
	const uint64_t mpidr = cpu_map[cpu];
	uint8_t aff0;

	if (mpidr == INV_MPID) {
		return;
	}

	aff0 = MPIDR_AFFLVL(mpidr, 0);
	gic_raise_sgi(SGI_FPU_IPI, mpidr, 1 << aff0);
}

/*
 * Make sure there is no pending FPU flush request for this CPU while
 * waiting for a contended spinlock to become available. This prevents
 * a deadlock when the lock we need is already taken by another CPU
 * that also wants its FPU content to be reinstated while such content
 * is still live in this CPU's FPU.
 */
void arch_spin_relax(void)
{
	if (arm_gic_irq_is_pending(SGI_FPU_IPI)) {
		arm_gic_irq_clear_pending(SGI_FPU_IPI);
		/*
		 * We may not be in IRQ context here hence cannot use
		 * arch_flush_local_fpu() directly.
		 */
		arch_float_disable(_current_cpu->arch.fpu_owner);
	}
}
#endif

#ifdef CONFIG_DEBUG_COREDUMP_SMP_FREEZE_CPUS

enum coredump_freeze_state {
	FREEZE_IDLE = 0,
	FREEZE_REQUESTED,
	FREEZE_CAPTURED,
};

struct coredump_frozen_cpu {
	struct arch_esf esf;
	uint64_t x19_x29[11];	/* x19..x28, x29 (fp) */
	uintptr_t sp;		/* pre-interrupt SP -- see capture site for why
				 * this can't just be derived later from esf's
				 * (by-then-copied) address
				 */
	struct k_thread *thread;
	bool valid;
};

static atomic_t freeze_state[CONFIG_MP_MAX_NUM_CPUS];
static struct coredump_frozen_cpu frozen_cpu[CONFIG_MP_MAX_NUM_CPUS];

/*
 * Deliberately separate from freeze_state above. If a CPU's SGI is slow to
 * arrive (e.g. it's mid-critical-section when freeze is requested) the
 * freeze-side wait below can time out and move on before that CPU ever
 * reaches the handler. When it eventually does, it must still see a
 * release signal that reflects thaw having *already run* -- if that
 * signal were folded into freeze_state (e.g. a FREEZE_RELEASE enum value),
 * the handler's own "atomic_set(freeze_state, FREEZE_CAPTURED)" on arrival
 * would stomp it right back to "not yet released", and since thaw only
 * runs once, that CPU would then spin forever with nothing left to wake
 * it. A dedicated flag that only this code writes to (never overwritten
 * by the capture step) has no such race.
 */
static atomic_t release_requested[CONFIG_MP_MAX_NUM_CPUS];

/* Rough iteration-based timeout, not a calibrated time value: the
 * scheduler/timers can't be trusted while another CPU may be panicking.
 */
#define COREDUMP_FREEZE_TIMEOUT_LOOPS	1000000

/*
 * Hard backstop so a frozen CPU can never be stuck forever, no matter what
 * goes wrong in the freeze/thaw handshake above: give up and resume on its
 * own after this much wall time. Generous on purpose -- real dumps
 * (e.g. CONFIG_LOG_MODE_IMMEDIATE, which serializes every line over slow
 * UART) can legitimately take tens of seconds, and this must never fire
 * during a dump that's actually still progressing normally.
 */
#define COREDUMP_FREEZE_HANDLER_MAX_SECONDS	60

/*
 * IPI handler: capture this CPU's live register state and spin until
 * released. Runs as a normal interrupt handler, so whatever this CPU was
 * doing when interrupted stays exactly as it was -- including any lock it
 * held -- until we return from here via the standard exception-return path.
 */
static void coredump_freeze_ipi_handler(const void *unused)
{
	ARG_UNUSED(unused);

	unsigned int cpu = arch_curr_cpu()->id;
	struct coredump_frozen_cpu *fc = &frozen_cpu[cpu];

	/*
	 * x19-x29 are guaranteed to still hold the exact values the
	 * interrupted code had: every function called between the original
	 * interruption and here (the ISR dispatch, the GIC driver call to
	 * find the active IRQ, ...) must have preserved them as callee-saved
	 * registers per AAPCS64.
	 */
	__asm__ volatile(
		"stp x19, x20, [%0, #0]\n\t"
		"stp x21, x22, [%0, #16]\n\t"
		"stp x23, x24, [%0, #32]\n\t"
		"stp x25, x26, [%0, #48]\n\t"
		"stp x27, x28, [%0, #64]\n\t"
		"str x29, [%0, #80]\n\t"
		:
		: "r" (fc->x19_x29)
		: "memory"
	);

	/*
	 * _isr_wrapper() always saves the pre-interrupt sp at a fixed slot
	 * just below the top of this CPU's IRQ stack before dispatching to
	 * any registered handler, regardless of interrupt nesting depth
	 * (only the outermost entry performs the stack switch and this
	 * save). That pointer is the arch_esf that z_arm64_enter_exc() built
	 * for whatever this CPU was doing at the moment it was interrupted.
	 */
	{
		uint64_t *irq_stack_top = (uint64_t *)arch_curr_cpu()->irq_stack;
		uintptr_t orig_esf = (uintptr_t)irq_stack_top[-2];

		memcpy(&fc->esf, (void *)orig_esf, sizeof(fc->esf));

		/*
		 * Same convention as arch_coredump_info_dump(): the
		 * pre-exception SP sits right above the ESF on the real
		 * stack. Must be computed from the ESF's *original* address
		 * (orig_esf) here, not from &fc->esf after the memcpy above --
		 * that's a separate, unrelated static buffer, and doing the
		 * same "+sizeof(esf)" arithmetic on its address would land
		 * inside frozen_cpu[] itself rather than on the real stack.
		 */
		fc->sp = orig_esf + sizeof(fc->esf);
	}

	fc->thread = arch_curr_cpu()->current;

	barrier_dsync_fence_full();
	atomic_set(&freeze_state[cpu], FREEZE_CAPTURED);

	/*
	 * Spin here -- still logically "inside" whatever context this CPU
	 * was interrupted from. Any lock that context held remains held;
	 * that's safe because we never touch it, we simply delay returning.
	 * If this CPU never reaches this point at all (e.g. it's stuck with
	 * IRQs masked), arch_coredump_freeze_other_cpus() below times out
	 * and moves on without it -- this loop is never what a stuck dump
	 * ends up waiting on. Bounded by wall time (not the freeze-side
	 * iteration timeout) as a last-resort backstop -- see
	 * release_requested's comment for why a plain "wait for thaw" flag
	 * isn't enough on its own to rule out every bad interleaving.
	 */
	{
		uint32_t start = k_cycle_get_32();
		uint32_t max_cycles = COREDUMP_FREEZE_HANDLER_MAX_SECONDS *
				      (uint32_t)CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC;

		while (atomic_get(&release_requested[cpu]) == 0) {
			if ((uint32_t)(k_cycle_get_32() - start) >= max_cycles) {
				break;
			}
		}
	}

	atomic_set(&release_requested[cpu], 0);
	atomic_set(&freeze_state[cpu], FREEZE_IDLE);
	/* Returning here resumes exactly where this CPU was interrupted. */
}

void arch_coredump_freeze_other_cpus(void)
{
	unsigned int self = arch_curr_cpu()->id;
	unsigned int num_cpus = arch_num_cpus();

	for (unsigned int i = 0; i < num_cpus; i++) {
		if (i == self) {
			continue;
		}

		frozen_cpu[i].valid = false;
		atomic_set(&release_requested[i], 0);
		atomic_set(&freeze_state[i], FREEZE_REQUESTED);
	}

	for (unsigned int i = 0; i < num_cpus; i++) {
		uint64_t mpidr;
		uint8_t aff0;

		if (i == self) {
			continue;
		}

		mpidr = cpu_map[i];
		if (mpidr == INV_MPID) {
			/* Never booted -- nothing to freeze. */
			atomic_set(&freeze_state[i], FREEZE_IDLE);
			continue;
		}

		aff0 = MPIDR_AFFLVL(mpidr, 0);
		gic_raise_sgi(SGI_COREDUMP_FREEZE_IPI, mpidr, 1 << aff0);
	}

	for (unsigned int i = 0; i < num_cpus; i++) {
		unsigned int loops = COREDUMP_FREEZE_TIMEOUT_LOOPS;

		if (i == self) {
			continue;
		}

		while (atomic_get(&freeze_state[i]) == FREEZE_REQUESTED && loops-- > 0) {
			/* busy-wait */
		}

		frozen_cpu[i].valid = (atomic_get(&freeze_state[i]) == FREEZE_CAPTURED);
	}
}

void arch_coredump_thaw_other_cpus(void)
{
	unsigned int self = arch_curr_cpu()->id;
	unsigned int num_cpus = arch_num_cpus();

	for (unsigned int i = 0; i < num_cpus; i++) {
		if (i == self) {
			continue;
		}

		/*
		 * Unconditional, regardless of freeze_state: a CPU whose SGI
		 * arrived late (after arch_coredump_freeze_other_cpus()'s
		 * wait already gave up on it) may not have reached
		 * FREEZE_CAPTURED yet. It still needs to see this once it
		 * does -- see release_requested's comment.
		 */
		atomic_set(&release_requested[i], 1);
	}
}

bool arm64_coredump_freeze_get_snapshot(unsigned int cpu,
					 const struct arch_esf **esf,
					 const uint64_t **x19_x29,
					 uintptr_t *sp,
					 struct k_thread **thread)
{
	if (cpu >= CONFIG_MP_MAX_NUM_CPUS || !frozen_cpu[cpu].valid) {
		return false;
	}

	*esf = &frozen_cpu[cpu].esf;
	*x19_x29 = frozen_cpu[cpu].x19_x29;
	*sp = frozen_cpu[cpu].sp;
	*thread = frozen_cpu[cpu].thread;

	return true;
}

#endif /* CONFIG_DEBUG_COREDUMP_SMP_FREEZE_CPUS */

int arch_smp_init(void)
{
	cpu_map[0] = MPIDR_TO_CORE(GET_MPIDR());

	/*
	 * SGI0 is use for sched ipi, this might be changed to use Kconfig
	 * option
	 */
	IRQ_CONNECT(SGI_SCHED_IPI, IRQ_DEFAULT_PRIORITY, sched_ipi_handler, NULL, 0);
	irq_enable(SGI_SCHED_IPI);

#ifdef CONFIG_USERSPACE
	IRQ_CONNECT(SGI_MMCFG_IPI, IRQ_DEFAULT_PRIORITY,
			mem_cfg_ipi_handler, NULL, 0);
	irq_enable(SGI_MMCFG_IPI);
#endif
#ifdef CONFIG_FPU_SHARING
	IRQ_CONNECT(SGI_FPU_IPI, IRQ_DEFAULT_PRIORITY, flush_fpu_ipi_handler, NULL, 0);
	irq_enable(SGI_FPU_IPI);
#endif
#ifdef CONFIG_DEBUG_COREDUMP_SMP_FREEZE_CPUS
	IRQ_CONNECT(SGI_COREDUMP_FREEZE_IPI, SGI_COREDUMP_FREEZE_PRIO,
			coredump_freeze_ipi_handler, NULL, 0);
	irq_enable(SGI_COREDUMP_FREEZE_IPI);
#endif

	return 0;
}

#endif
