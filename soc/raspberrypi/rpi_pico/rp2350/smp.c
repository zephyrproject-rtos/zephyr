/*
 * Copyright (c) 2026 Seunghoon Park
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SMP support for the two Cortex-M33 cores of the RP2350
 *
 * Core 0 is released from reset by the bootrom and runs Zephyr's reset
 * vector. Core 1 stays in the bootrom, waiting for a launch command on the
 * inter-core FIFO, until pm_cpu_on() starts it.
 */

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/pm_cpu_ops.h>
#include <zephyr/irq.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/platform/hooks.h>
#include <cmsis_core.h>
#include <ipi.h>
#include <rp2350_smp.h>

#include <hardware/address_mapped.h>
#include <hardware/regs/addressmap.h>
#include <hardware/regs/intctrl.h>
#include <hardware/regs/sio.h>
#include <hardware/structs/m33.h>
#include <hardware/structs/psm.h>
#include <hardware/structs/sio.h>
#include <hardware/structs/timer.h>
#include <hardware/structs/watchdog.h>
#include <pico/runtime_init.h>

BUILD_ASSERT(CONFIG_MP_MAX_NUM_CPUS <= 2, "the RP2350 has two Cortex-M33 cores");

/*
 * Core 1 is started by pm_cpu_on() below. The AMP launcher of
 * soc/raspberrypi/rpi_pico/common/soc_cpu1.c would start it a second time,
 * pointed at another image, before the kernel does.
 */
BUILD_ASSERT(CONFIG_MP_MAX_NUM_CPUS == 1 || !IS_ENABLED(CONFIG_SOC_FAMILY_RPI_PICO_CPU1_ENABLE),
	     "SMP starts core 1 itself, disable CONFIG_SOC_FAMILY_RPI_PICO_CPU1_ENABLE");

/*
 * Time allowed for the core 1 launch handshake. It is counted in polling
 * iterations because neither SysTick nor the TIMER block is guaranteed to be
 * running when core 1 is started. A polling iteration takes at least three
 * core clock cycles, so the budget is never shorter than 100 ms.
 */
#define LAUNCH_TIMEOUT_MS 100
#define LAUNCH_POLL_BUDGET                                                                         \
	((DT_PROP(DT_NODELABEL(clk_sys), clock_frequency) / 1000U) * LAUNCH_TIMEOUT_MS / 3U)

/* Doorbell used for the scheduler IPI; the doorbells are per core and symmetric */
#define IPI_DOORBELL_BIT BIT(0)
#define IPI_IRQN         SIO_IRQ_BELL
#define IPI_IRQ_PRIO     0

/* Doorbell used to ask the other core to park itself in RAM, see rp2350_smp_park_other() */
#define PARK_DOORBELL_BIT BIT(1)

#ifdef CONFIG_RPI_PICO_SMP_FLASH_PARK
/*
 * Cores that pm_cpu_on() has started (core 0 is always running), and cores
 * that have enabled their doorbell interrupt and so can be parked. Bit n is
 * core n. A core is in between from the launch until its
 * soc_per_core_init_hook() has run: it executes from flash but cannot be
 * parked yet.
 */
static uint32_t cores_started = BIT(0);
static uint32_t cores_ready;

#define PARK_STATE_SHIFT 4
/* The park state is (phase | target core << PARK_STATE_SHIFT); IDLE is 0 */
#define PARK_IDLE        0U
#define PARK_REQ         1U
#define PARK_PARKED      2U
#define PARK_RELEASE     3U
#define PARK_STATE(p, c) ((p) | ((uint32_t)(c) << PARK_STATE_SHIFT))

/* Shared by both cores, it must stay in RAM and be accessed atomically */
static uint32_t park_state;

#ifdef CONFIG_RPI_PICO_SMP_FLASH_PARK_DIAG
volatile struct rp2350_smp_park_stats rp2350_smp_park_stats;
#define PARK_STAT_INC(field) (rp2350_smp_park_stats.field++)
#else
#define PARK_STAT_INC(field) ((void)0)
#endif
#endif /* CONFIG_RPI_PICO_SMP_FLASH_PARK */

/*
 * Boot parameters filled in by arch_cpu_start() (arch/arm/core/cortex_m/smp.c);
 * sp is the stack pointer core 1 will use. The struct is private to the arch
 * code, so it is repeated here member for member: the extern declaration must
 * have the same type as the definition.
 * TODO: drop this copy once the Cortex-M SMP arch code exports the struct or
 * an accessor for the secondary stack pointer.
 */
struct arm_m_boot_params {
	uintptr_t sp;
	arch_cpustart_t fn;
	void *arg;
};

extern volatile struct arm_m_boot_params arm_m_cpu_boot_params;

/* Called from reset before any stack exists: r0 only */
__attribute__((naked)) uint32_t soc_cpu_index(void)
{
	__asm__ volatile("ldr r0, =%c0\n\t"
			 "ldr r0, [r0]\n\t"
			 "bx lr\n\t"
			 ".ltorg\n\t"
			 :
			 : "i"(SIO_BASE + SIO_CPUID_OFFSET));
}

static inline void fifo_drain(void)
{
	while ((sio_hw->fifo_st & SIO_FIFO_ST_VLD_BITS) != 0U) {
		(void)sio_hw->fifo_rd;
	}
}

static bool fifo_push(uint32_t data, uint32_t *budget)
{
	while ((sio_hw->fifo_st & SIO_FIFO_ST_RDY_BITS) == 0U) {
		if ((*budget)-- == 0U) {
			return false;
		}
	}

	sio_hw->fifo_wr = data;
	/* The bootrom on core 1 may sleep in WFE until there is something to read */
	__asm__ volatile("sev" ::: "memory");

	return true;
}

static bool fifo_pop(uint32_t *data, uint32_t *budget)
{
	while ((sio_hw->fifo_st & SIO_FIFO_ST_VLD_BITS) == 0U) {
		if ((*budget)-- == 0U) {
			return false;
		}
	}

	*data = sio_hw->fifo_rd;

	return true;
}

/*
 * Hard-reset core 1 through the power-on state machine, so that it is back
 * in the bootrom waiting for a launch command even if a previous image (or
 * a soft reset of core 0 only) left it running. Same as pico-sdk
 * multicore_reset_core1().
 */
static int core1_reset(uint32_t *budget)
{
	io_rw_32 *frce_off = &psm_hw->frce_off;
	uint32_t value;

	*(io_rw_32 *)hw_set_alias_untyped(frce_off) = PSM_FRCE_OFF_PROC1_BITS;
	/* Reading back also fences the store against the APB bridge */
	while ((*frce_off & PSM_FRCE_OFF_PROC1_BITS) == 0U) {
		if ((*budget)-- == 0U) {
			return -ETIMEDOUT;
		}
	}

	*(io_rw_32 *)hw_clear_alias_untyped(frce_off) = PSM_FRCE_OFF_PROC1_BITS;

	/* The bootrom drains its FIFO and then pushes a 0 to say it is ready */
	do {
		if (!fifo_pop(&value, budget)) {
			return -ETIMEDOUT;
		}
	} while (value != 0U);

	return 0;
}

/*
 * Launch core 1 with the bootrom handshake of pico-sdk
 * multicore_launch_core1_raw(): the sequence 0, 0, 1, vector table, sp, pc is
 * sent through the FIFO and every word has to be echoed back, otherwise the
 * sequence starts over.
 */
static int core1_launch(uint32_t vector_table, uint32_t sp, uint32_t pc, uint32_t *budget)
{
	const uint32_t cmd_sequence[] = {0, 0, 1, vector_table, sp, pc};
	uint32_t seq = 0;

	do {
		uint32_t cmd = cmd_sequence[seq];
		uint32_t response;

		if (cmd == 0U) {
			/* Always drain the read FIFO before sending a 0 */
			fifo_drain();
			/* Core 1 may be waiting for FIFO space with WFE */
			__asm__ volatile("sev" ::: "memory");
		}

		if (!fifo_push(cmd, budget) || !fifo_pop(&response, budget)) {
			return -ETIMEDOUT;
		}

		if (cmd == response) {
			seq++;
		} else {
			/* Start over, but count it so that the loop cannot run forever */
			seq = 0U;
			if ((*budget)-- == 0U) {
				return -ETIMEDOUT;
			}
		}
	} while (seq < ARRAY_SIZE(cmd_sequence));

	return 0;
}

int pm_cpu_on(unsigned long cpuid, uintptr_t entry_point)
{
	uint32_t budget = LAUNCH_POLL_BUDGET;
	const uint32_t *vector_table = (const uint32_t *)entry_point;
	/*
	 * Core 1 is started like a hardware reset of a Cortex-M: the reset
	 * handler is word 1 of the vector table (the Thumb bit is already set
	 * there), so reset.S sees core 1 and branches to z_arm_secondary_reset.
	 * The bootrom needs a stack pointer to hand over, which reset.S does
	 * not use before it installs the one from arm_m_cpu_boot_params, so
	 * pass that one instead of the main stack of core 0 (word 0).
	 */
	uint32_t pc = vector_table[1];
	uint32_t sp = (uint32_t)arm_m_cpu_boot_params.sp;
	/* The FIFO interrupt may be in use (pico-mbox), keep it out of the handshake */
	bool fifo_irq_enabled = irq_is_enabled(SIO_IRQ_FIFO) != 0;
	int ret;

	if (cpuid != 1) {
		return -EINVAL;
	}

	if (fifo_irq_enabled) {
		irq_disable(SIO_IRQ_FIFO);
	}

#ifdef CONFIG_RPI_PICO_SMP_FLASH_PARK
	/* From now on core 1 may run from flash, a flash write has to wait for it */
	__atomic_fetch_or(&cores_started, BIT(1), __ATOMIC_SEQ_CST);
#endif

	ret = core1_reset(&budget);
	if (ret == 0) {
		ret = core1_launch(entry_point, sp, pc, &budget);
	}

#ifdef CONFIG_RPI_PICO_SMP_FLASH_PARK
	if (ret != 0) {
		__atomic_fetch_and(&cores_started, ~BIT(1), __ATOMIC_SEQ_CST);
	}
#endif

	if (fifo_irq_enabled) {
		irq_enable(SIO_IRQ_FIFO);
	}

	return ret;
}

/*
 * SYSRESETREQ resets only the core that requests it on the RP2350, so the
 * default Cortex-M reboot would leave the other core running over the RAM of
 * the core that is starting again.
 *
 * On core 0, put core 1 back into the bootrom first (the same state it has
 * after power on) and then reset this core.
 *
 * On core 1 (any thread can call sys_reboot() with SMP) reset the whole chip
 * through the watchdog instead, as the Pico SDK does in watchdog_reboot():
 * every PSM domain but the oscillators. The mirror image of the core 0 path
 * (core 1 forcing core 0 off and on, then resetting itself) was tried and
 * left the chip unresponsive on hardware; why was not investigated.
 */
void sys_arch_reboot(int type)
{
	io_rw_32 *frce_off = &psm_hw->frce_off;

	ARG_UNUSED(type);

	(void)arch_irq_lock();

	if (soc_cpu_index() != 0U) {
		hw_clear_bits(&watchdog_hw->ctrl, WATCHDOG_CTRL_ENABLE_BITS);
		psm_hw->wdsel = PSM_WDSEL_BITS & ~(PSM_WDSEL_ROSC_BITS | PSM_WDSEL_XOSC_BITS);
		hw_set_bits(&watchdog_hw->ctrl, WATCHDOG_CTRL_TRIGGER_BITS);
		while (true) {
		}
	}

	*(io_rw_32 *)hw_set_alias_untyped(frce_off) = PSM_FRCE_OFF_PROC1_BITS;
	while ((*frce_off & PSM_FRCE_OFF_PROC1_BITS) == 0U) {
	}
	*(io_rw_32 *)hw_clear_alias_untyped(frce_off) = PSM_FRCE_OFF_PROC1_BITS;

	barrier_dsync_fence_full();
	NVIC_SystemReset();
}

#ifdef CONFIG_SMP
#ifdef CONFIG_RPI_PICO_SMP_FLASH_PARK
/*
 * The doorbell 1 handler of the core that is asked to park. It and
 * everything it uses must be in RAM: once the other core is parked, the
 * flash is not accessible until this function returns. In particular the
 * atomic operations are the compiler builtins, which are inline code on the
 * Cortex-M33, not calls to a library in flash.
 *
 * All interrupts (PRIMASK, not just BASEPRI) are masked, so that no handler
 * in flash can run. The vector fetch and the code of ipi_isr() up to this
 * call ran before the request was accepted, and the code after the return
 * runs after the flash is accessible again.
 */
__ramfunc static void park_handler(void)
{
	uint32_t cpu = sio_hw->cpuid;
	uint32_t primask = __get_PRIMASK();
	uint32_t expected = PARK_STATE(PARK_REQ, cpu);

	sio_hw->doorbell_in_clr = PARK_DOORBELL_BIT;
	__disable_irq();

	/*
	 * One step from "requested of me" to "parked": a request that was
	 * withdrawn, or is for the other core, is not parked for.
	 */
	if (__atomic_compare_exchange_n(&park_state, &expected, PARK_STATE(PARK_PARKED, cpu), false,
					__ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) {
#ifdef CONFIG_RPI_PICO_SMP_FLASH_PARK_DIAG
		uint32_t t0 = timer1_hw->timerawl;
#endif
		/* Plain loop, no WFE: it is not known yet that SEV wakes the other core */
		while (__atomic_load_n(&park_state, __ATOMIC_ACQUIRE) !=
		       PARK_STATE(PARK_RELEASE, cpu)) {
		}
		__atomic_store_n(&park_state, PARK_IDLE, __ATOMIC_RELEASE);
#ifdef CONFIG_RPI_PICO_SMP_FLASH_PARK_DIAG
		uint32_t dt = timer1_hw->timerawl - t0;

		rp2350_smp_park_stats.victim_parked[cpu]++;
		if (dt > rp2350_smp_park_stats.victim_max_us[cpu]) {
			rp2350_smp_park_stats.victim_max_us[cpu] = dt;
		}
#endif
	} else {
		PARK_STAT_INC(victim_stale[cpu]);
	}

	__set_PRIMASK(primask);
}
#endif /* CONFIG_RPI_PICO_SMP_FLASH_PARK */

/*
 * The scheduler IPI is a SIO doorbell. Doorbell interrupts are routed to the
 * core that owns the doorbell register, so no other core's interrupt
 * controller is involved. The FIFO interrupt (SIO_IRQ_FIFO) would do the same
 * job but it is the interrupt of the raspberrypi,pico-mbox driver.
 *
 * Only a Secure image (the default) is supported; a Non-secure image would
 * need the Non-secure doorbell interrupt instead.
 */
BUILD_ASSERT(!IS_ENABLED(CONFIG_TRUSTED_EXECUTION_NONSECURE),
	     "RP2350 SMP uses the Secure SIO doorbell interrupt");

static void ipi_isr(const void *arg)
{
	ARG_UNUSED(arg);

#ifdef CONFIG_RPI_PICO_SMP_FLASH_PARK
	if ((sio_hw->doorbell_in_clr & PARK_DOORBELL_BIT) != 0U) {
		park_handler();
	}
#endif

	sio_hw->doorbell_in_clr = IPI_DOORBELL_BIT;
	z_sched_ipi();
}

static void ipi_send(uint32_t cpu_bitmap)
{
	/* Only the other core can be signalled, a doorbell never loops back */
	if ((cpu_bitmap & BIT(1U - soc_cpu_index())) != 0U) {
		/* Publish the data the other core will look at before the doorbell */
		barrier_dmem_fence_full();
		sio_hw->doorbell_out_set = IPI_DOORBELL_BIT;
	}
}

void arch_sched_broadcast_ipi(void)
{
	ipi_send(IPI_ALL_CPUS_MASK);
}

void arch_sched_directed_ipi(uint32_t cpu_bitmap)
{
	ipi_send(cpu_bitmap);
}

#ifdef CONFIG_RPI_PICO_SMP_FLASH_PARK
/* Short waits are counted in system timer cycles, they are a few microseconds long */
#define PARK_SHORT_WAIT_CYC k_us_to_cyc_ceil32(CONFIG_RPI_PICO_SMP_FLASH_PARK_SHORT_WAIT_US)
#define PARK_RETRY_GAP_US   10
#define UNPARK_WAIT_US      1000

int rp2350_smp_park_other(unsigned int *key, k_timeout_t timeout)
{
	k_timepoint_t end = sys_timepoint_calc(timeout);

	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	while (true) {
		unsigned int k = arch_irq_lock();
		uint32_t cpu, other, requested, parked;

		/* The thread cannot move while its interrupts are locked */
		cpu = soc_cpu_index();
		other = 1U - cpu;
		requested = PARK_STATE(PARK_REQ, other);
		parked = PARK_STATE(PARK_PARKED, other);

		if ((__atomic_load_n(&cores_started, __ATOMIC_ACQUIRE) & BIT(other)) == 0U) {
			/* The other core is still in the bootrom: nothing to park */
			*key = k;
			return 0;
		}

		/* Interrupts were locked already: a retry would not let the other core in */
		if (!arch_irq_unlocked(k)) {
			arch_irq_unlock(k);
			return -EWOULDBLOCK;
		}

		if ((__atomic_load_n(&cores_ready, __ATOMIC_ACQUIRE) & BIT(other)) == 0U) {
			/* It runs from flash but cannot be parked yet */
			PARK_STAT_INC(park_not_ready);
		} else {
			uint32_t expected = PARK_IDLE;

			if (__atomic_compare_exchange_n(&park_state, &expected, requested, false,
							__ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) {
				uint32_t start = k_cycle_get_32();

				/* The state is visible to the other core before the doorbell */
				sio_hw->doorbell_out_set = PARK_DOORBELL_BIT;

				while (__atomic_load_n(&park_state, __ATOMIC_ACQUIRE) != parked) {
					if ((k_cycle_get_32() - start) >= PARK_SHORT_WAIT_CYC) {
						break;
					}
				}

				expected = requested;
				/*
				 * Withdrawing and being parked race in one compare and swap:
				 * whichever comes first wins. If the other core won, go on.
				 */
				if (__atomic_compare_exchange_n(&park_state, &expected, PARK_IDLE,
								false, __ATOMIC_SEQ_CST,
								__ATOMIC_SEQ_CST)) {
					/* A late doorbell finds no request and does nothing */
					PARK_STAT_INC(park_cancel);
				} else {
					__ASSERT_NO_MSG(expected == parked);
					PARK_STAT_INC(park_ok);
					*key = k;
					return 0;
				}
			} else {
				PARK_STAT_INC(park_busy);
			}
		}

		/*
		 * Let this core take what is pending for it (the other core may be
		 * waiting for an IPI answer with its interrupts masked), then try again.
		 */
		arch_irq_unlock(k);

		if (sys_timepoint_expired(end)) {
			PARK_STAT_INC(park_ebusy);
			return -EBUSY;
		}

		k_busy_wait(PARK_RETRY_GAP_US);
	}
}

void rp2350_smp_unpark_other(unsigned int key)
{
	uint32_t other = 1U - soc_cpu_index();
	uint32_t parked = PARK_STATE(PARK_PARKED, other);

	/* With this core's interrupts locked, "parked" can only be this call's own park */
	if (__atomic_load_n(&park_state, __ATOMIC_ACQUIRE) == parked) {
		uint32_t start = k_cycle_get_32();

		__atomic_store_n(&park_state, PARK_STATE(PARK_RELEASE, other), __ATOMIC_SEQ_CST);
		/*
		 * Wait for it to be running again: a new request finds the state
		 * idle, and the next write cannot start before this core has the
		 * flash back.
		 */
		while (__atomic_load_n(&park_state, __ATOMIC_ACQUIRE) != PARK_IDLE) {
			if ((k_cycle_get_32() - start) >= k_us_to_cyc_ceil32(UNPARK_WAIT_US)) {
				break;
			}
		}
	}

	arch_irq_unlock(key);
}
#endif /* CONFIG_RPI_PICO_SMP_FLASH_PARK */

int arch_smp_init(void)
{
	/* The handler table is shared by both cores, the NVIC is not */
	IRQ_CONNECT(IPI_IRQN, IPI_IRQ_PRIO, ipi_isr, NULL, 0);

	return 0;
}
#endif /* CONFIG_SMP */

/*
 * Runs on each core from arch_kernel_init(). The state set up here is per
 * core: the NVIC, the coprocessor access control register.
 */
void soc_per_core_init_hook(void)
{
	/*
	 * Exclusive loads and stores (the kernel's atomics, spinlocks and the
	 * global irq_lock) only see the other core through the global exclusive
	 * monitor. With the MPU off, SRAM is Non-shareable in the default memory
	 * map, so without EXTEXCLALL each core checks its local monitor only and
	 * atomics between the cores are lost: atomic_inc() from three threads on
	 * two cores lost up to a third of 60000 increments on hardware. The Pico
	 * SDK sets the same bit per core (spinlock_set_extexclall()).
	 */
	m33_hw->actlr |= M33_ACTLR_EXTEXCLALL_BITS;
	barrier_isync_fence_full();

	/* Core 0 already did this in soc_reset_hook(), which core 1 never runs */
	if (soc_cpu_index() != 0U) {
		runtime_init_per_core_enable_coprocessors();
	}

	/* arch_smp_init() connects the handler only with more than one CPU */
	if (IS_ENABLED(CONFIG_SMP) && (CONFIG_MP_MAX_NUM_CPUS > 1)) {
		/*
		 * A reboot resets only the calling core, so a doorbell can be left
		 * over from before it. Priority and enable are NVIC state, so every
		 * core sets them itself.
		 */
		sio_hw->doorbell_in_clr = IPI_DOORBELL_BIT | PARK_DOORBELL_BIT;
		z_arm_irq_priority_set(IPI_IRQN, IPI_IRQ_PRIO, 0);
		irq_enable(IPI_IRQN);
#ifdef CONFIG_RPI_PICO_SMP_FLASH_PARK
		/* From here this core takes a park request */
		__atomic_fetch_or(&cores_ready, BIT(soc_cpu_index()), __ATOMIC_SEQ_CST);
#endif
	}
}
