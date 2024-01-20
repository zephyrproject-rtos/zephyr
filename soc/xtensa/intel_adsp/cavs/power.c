/*
 * Copyright (c) 2019 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <xtensa/xtruntime.h>
#include <zephyr/irq_nextlevel.h>
#include <xtensa/hal.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/pm.h>
#include <zephyr/device.h>
#include <zephyr/cache.h>
#include <cpu_init.h>

#include <adsp_memory.h>
#include <adsp_shim.h>
#include <adsp_clk.h>
#include <adsp_imr_layout.h>
#include <cavs-idc.h>
#include "soc.h"

#ifdef CONFIG_DYNAMIC_INTERRUPTS
#include <zephyr/sw_isr_table.h>
#endif

#define LOG_LEVEL CONFIG_SOC_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(soc);

# define SHIM_GPDMA_BASE_OFFSET   0x6500
# define SHIM_GPDMA_BASE(x)       (SHIM_GPDMA_BASE_OFFSET + (x) * 0x100)
# define SHIM_GPDMA_CLKCTL(x)     (SHIM_GPDMA_BASE(x) + 0x4)
# define SHIM_CLKCTL_LPGPDMAFDCGB BIT(0)

#ifdef CONFIG_PM
#define SRAM_ALIAS_BASE		0x9E000000
#define SRAM_ALIAS_MASK		0xFF000000
#define SRAM_ALIAS_OFFSET	0x20000000

#define L2_INTERRUPT_NUMBER     4
#define L2_INTERRUPT_MASK       (1<<L2_INTERRUPT_NUMBER)

#define L3_INTERRUPT_NUMBER     6
#define L3_INTERRUPT_MASK       (1<<L3_INTERRUPT_NUMBER)

#define ALL_USED_INT_LEVELS_MASK (L2_INTERRUPT_MASK | L3_INTERRUPT_MASK)

/*
 * @biref FW entry point called by ROM during normal boot flow
 */
extern void rom_entry(void);
void mp_resume_entry(void);

struct core_state {
	uint32_t a0;
	uint32_t a1;
	uint32_t excsave2;
	uint32_t intenable;
};

static struct core_state core_desc[CONFIG_MP_MAX_NUM_CPUS] = {{0}};

/**
 * @brief Power down procedure.
 *
 * Locks its code in L1 cache and shuts down memories.
 * NOTE: there's no return from this function.
 *
 * @param disable_lpsram        flag if LPSRAM is to be disabled (whole)
 * @param hpsram_pg_mask pointer to memory segments power gating mask
 * (each bit corresponds to one ebb)
 */
extern void power_down_cavs(bool disable_lpsram, uint32_t __sparse_cache * hpsram_pg_mask);

static inline void __sparse_cache *uncache_to_cache(void *address)
{
	return (void __sparse_cache *)((uintptr_t)(address) | SRAM_ALIAS_OFFSET);
}

static ALWAYS_INLINE void _save_core_context(void)
{
	uint32_t core_id = arch_proc_id();

	core_desc[core_id].excsave2 = XTENSA_RSR(ZSR_CPU_STR);
	__asm__ volatile("mov %0, a0" : "=r"(core_desc[core_id].a0));
	__asm__ volatile("mov %0, a1" : "=r"(core_desc[core_id].a1));
	sys_cache_data_flush_range(&core_desc[core_id], sizeof(struct core_state));
}

static ALWAYS_INLINE void _restore_core_context(void)
{
	uint32_t core_id = arch_proc_id();

	XTENSA_WSR(ZSR_CPU_STR, core_desc[core_id].excsave2);
	__asm__ volatile("mov a0, %0" :: "r"(core_desc[core_id].a0));
	__asm__ volatile("mov a1, %0" :: "r"(core_desc[core_id].a1));
	__asm__ volatile("rsync");
}

void power_gate_exit(void)
{
	cpu_early_init();
	sys_cache_data_flush_and_invd_all();
	_restore_core_context();

	/* Secondary core is resumed by set_dx */
	if (arch_proc_id()) {
		mp_resume_entry();
	}
}

__asm__(".align 4\n\t"
	".global dsp_restore_vector\n\t"
	"dsp_restore_vector:\n\t"
	"  movi  a0, 0\n\t"
	"  movi  a1, 1\n\t"
	"  movi  a2, 0x40020\n\t"/* PS_UM|PS_WOE */
	"  wsr   a2, PS\n\t"
	"  wsr   a1, WINDOWSTART\n\t"
	"  wsr   a0, WINDOWBASE\n\t"
	"  rsync\n\t"
	"  movi  a1, z_interrupt_stacks\n\t"
	"  rsr   a2, PRID\n\t"
	"  movi  a3, " STRINGIFY(CONFIG_ISR_STACK_SIZE) "\n\t"
	"  mull  a2, a2, a3\n\t"
	"  add   a2, a2, a3\n\t"
	"  add   a1, a1, a2\n\t"
	"  call0 power_gate_exit\n\t");

void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);
	uint32_t cpu = arch_proc_id();

	if (state == PM_STATE_SOFT_OFF) {
		core_desc[cpu].intenable = XTENSA_RSR("INTENABLE");
		z_xt_ints_off(0xffffffff);
		xthal_window_spill();
		_save_core_context();
		soc_cpus_active[cpu] = false;
		sys_cache_data_flush_and_invd_all();
		if (cpu == 0) {
			uint32_t hpsram_mask[HPSRAM_SEGMENTS] = {0};

			struct imr_header hdr = {
				.adsp_imr_magic = ADSP_IMR_MAGIC_VALUE,
				.imr_restore_vector = rom_entry,
			};
			struct imr_layout *imr_layout =
				z_soc_uncached_ptr((__sparse_force void __sparse_cache *)
						   L3_MEM_BASE_ADDR);

			imr_layout->imr_state.header = hdr;

#ifdef CONFIG_ADSP_POWER_DOWN_HPSRAM
			/* turn off all HPSRAM banks - get a full bitmap */
			for (int i = 0; i < HPSRAM_SEGMENTS; i++)
				hpsram_mask[i] = HPSRAM_MEMMASK(i);
#endif /* CONFIG_ADSP_POWER_DOWN_HPSRAM */
			/* do power down - this function won't return */
			power_down_cavs(true, uncache_to_cache(&hpsram_mask[0]));
		} else {
			k_cpu_atomic_idle(arch_irq_lock());
		}
	} else {
		__ASSERT(false, "invalid argument - unsupported power state");
	}
}

/* Handle SOC specific activity after Low Power Mode Exit */
void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);
	uint32_t cpu = arch_proc_id();

	if (state == PM_STATE_SOFT_OFF) {
		soc_cpus_active[cpu] = true;
		sys_cache_data_flush_and_invd_all();
		z_xt_ints_on(core_desc[cpu].intenable);
	} else {
		__ASSERT(false, "invalid argument - unsupported power state");
	}

	/**
	 * We don't have the key used to lock interruptions here.
	 * Just set PS.INTLEVEL to 0.
	 */
	__asm__ volatile ("rsil a2, 0");
}
#endif /* CONFIG_PM */

#ifdef CONFIG_ARCH_CPU_IDLE_CUSTOM
/* xt-clang removes any NOPs more than 8. So we need to set
 * no optimization to avoid those NOPs from being removed.
 *
 * This function is simply enough and full of hand written
 * assembly that optimization is not really meaningful
 * anyway. So we can skip optimization unconditionally.
 * Re-evalulate its use and add #ifdef if this assumption
 * is no longer valid.
 */
__no_optimization
void arch_cpu_idle(void)
{
	sys_trace_idle();

	/* Just spin forever with interrupts unmasked, for platforms
	 * where WAITI can't be used or where its behavior is
	 * complicated (Intel DSPs will power gate on idle entry under
	 * some circumstances)
	 */
	if (IS_ENABLED(CONFIG_XTENSA_CPU_IDLE_SPIN)) {
		__asm__ volatile("rsil a0, 0");
		__asm__ volatile("loop_forever: j loop_forever");
		return;
	}

	/* Cribbed from SOF: workaround for a bug in some versions of
	 * the LX6 IP.	Preprocessor ugliness avoids the need to
	 * figure out how to get the compiler to unroll a loop.
	 */
	if (IS_ENABLED(CONFIG_XTENSA_WAITI_BUG)) {
#define NOP4 __asm__ volatile("nop; nop; nop; nop");
#define NOP32 NOP4 NOP4 NOP4 NOP4 NOP4 NOP4 NOP4 NOP4
#define NOP128() NOP32 NOP32 NOP32 NOP32
		NOP128();
#undef NOP128
#undef NOP32
#undef NOP4
		__asm__ volatile("isync; extw");
	}

__asm__ volatile ("waiti 0");
}
#endif

__imr void power_init(void)
{
	/* Request HP ring oscillator and
	 * wait for status to indicate it's ready.
	 */
	CAVS_SHIM.clkctl |= CAVS_CLKCTL_RHROSCC;
	while ((CAVS_SHIM.clkctl & CAVS_CLKCTL_RHROSCC) != CAVS_CLKCTL_RHROSCC) {
		k_busy_wait(10);
	}

	/* Request HP Ring Oscillator
	 * Select HP Ring Oscillator
	 * High Power Domain PLL Clock Select device by 2
	 * Low Power Domain PLL Clock Select device by 4
	 * Disable Tensilica Core(s) Prevent Local Clock Gating
	 *   - Disabling "prevent clock gating" means allowing clock gating
	 */
	CAVS_SHIM.clkctl = (CAVS_CLKCTL_RHROSCC |
			    CAVS_CLKCTL_OCS |
			    CAVS_CLKCTL_LMCS);

	/* Prevent LP GPDMA 0 & 1 clock gating */
	sys_write32(SHIM_CLKCTL_LPGPDMAFDCGB, SHIM_GPDMA_CLKCTL(0));
	sys_write32(SHIM_CLKCTL_LPGPDMAFDCGB, SHIM_GPDMA_CLKCTL(1));

	/* Disable power gating for first cores */
	CAVS_SHIM.pwrctl |= CAVS_PWRCTL_TCPDSPPG(0);

	/* On cAVS 1.8+, we must demand ownership of the timestamping
	 * and clock generator registers.  Lacking the former will
	 * prevent wall clock timer interrupts from arriving, even
	 * though the device itself is operational.
	 */
	sys_write32(GENO_MDIVOSEL | GENO_DIOPTOSEL, DSP_INIT_GENO);
	sys_write32(IOPO_DMIC_FLAG | IOPO_I2SSEL_MASK, DSP_INIT_IOPO);
}
