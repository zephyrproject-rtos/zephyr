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

#include <hardware/address_mapped.h>
#include <hardware/regs/addressmap.h>
#include <hardware/regs/intctrl.h>
#include <hardware/regs/sio.h>
#include <hardware/structs/m33.h>
#include <hardware/structs/psm.h>
#include <hardware/structs/sio.h>
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

	ret = core1_reset(&budget);
	if (ret == 0) {
		ret = core1_launch(entry_point, sp, pc, &budget);
	}

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
		sio_hw->doorbell_in_clr = IPI_DOORBELL_BIT;
		z_arm_irq_priority_set(IPI_IRQN, IPI_IRQ_PRIO, 0);
		irq_enable(IPI_IRQN);
	}
}
