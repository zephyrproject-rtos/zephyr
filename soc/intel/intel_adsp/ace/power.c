/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/pm/pm.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/device.h>
#include <zephyr/debug/sparse.h>
#include <zephyr/cache.h>
#include <cpu_init.h>
#include <soc_util.h>

#include <adsp_boot.h>
#include <adsp_power.h>
#include <adsp_memory.h>
#include <adsp_imr_layout.h>
#include <zephyr/drivers/mm/mm_drv_intel_adsp_mtl_tlb.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <mem_window.h>

#define LPSRAM_MAGIC_VALUE      0x13579BDF
#define LPSCTL_BATTR_MASK       GENMASK(16, 12)

#if CONFIG_SOC_INTEL_ACE15_MTPM
/* Used to force any pending transaction by HW issuing an upstream read before
 * power down host domain.
 */
uint8_t adsp_pending_buffer[CONFIG_DCACHE_LINE_SIZE] __aligned(CONFIG_DCACHE_LINE_SIZE);
#endif /* CONFIG_SOC_INTEL_ACE15_MTPM */

__imr void power_init(void)
{
#if CONFIG_ADSP_IDLE_CLOCK_GATING
	/* Disable idle power gating */
	DSPCS.bootctl[0].bctl |= DSPBR_BCTL_WAITIPPG;
#else
	/* Disable idle power and clock gating */
	DSPCS.bootctl[0].bctl |= DSPBR_BCTL_WAITIPCG | DSPBR_BCTL_WAITIPPG;
#endif /* CONFIG_ADSP_IDLE_CLOCK_GATING */

#if CONFIG_SOC_INTEL_ACE15_MTPM
	*((__sparse_force uint32_t *)sys_cache_cached_ptr_get(&adsp_pending_buffer)) =
		INTEL_ADSP_ACE15_MAGIC_KEY;
	sys_cache_data_flush_range((__sparse_force void *)
			sys_cache_cached_ptr_get(&adsp_pending_buffer),
			sizeof(adsp_pending_buffer));
#endif /* CONFIG_SOC_INTEL_ACE15_MTPM */
}

#ifdef CONFIG_PM

#define L2_INTERRUPT_NUMBER     4
#define L2_INTERRUPT_MASK       (1<<L2_INTERRUPT_NUMBER)

#define L3_INTERRUPT_NUMBER     6
#define L3_INTERRUPT_MASK       (1<<L3_INTERRUPT_NUMBER)

#define ALL_USED_INT_LEVELS_MASK (L2_INTERRUPT_MASK | L3_INTERRUPT_MASK)

#define CPU_POWERUP_TIMEOUT_USEC 10000

/**
 * @brief Power down procedure.
 *
 * Locks its code in L1 cache and shuts down memories.
 * NOTE: there's no return from this function.
 *
 * @param disable_lpsram        flag if LPSRAM is to be disabled (whole)
 * @param disable_hpsram        flag if HPSRAM is to be disabled (whole)
 * @param response_to_ipc       flag if ipc response should be send during power down
 */
void power_down(bool disable_lpsram, bool disable_hpsram, bool response_to_ipc);

#ifdef CONFIG_ADSP_IMR_CONTEXT_SAVE
/**
 *  @brief platform specific context restore procedure
 *
 *  Should be called when soc context restore is completed
 */
extern void platform_context_restore(void);

/*
 * @brief pointer to a persistent storage space, to be set by platform code
 */
uint8_t *global_imr_ram_storage;

/*8
 * @biref a d3 restore boot entry point
 */
extern void boot_entry_d3_restore(void);

/*
 * @brief re-enables IDC interrupt for all cores after exiting D3 state
 *
 * Called once from core 0
 */
extern void soc_mp_on_d3_exit(void);

#else

/*
 * @biref FW entry point called by ROM during normal boot flow
 */
extern void rom_entry(void);

#endif /* CONFIG_ADSP_IMR_CONTEXT_SAVE */

/* NOTE: This struct will grow with all values that have to be stored for
 * proper cpu restore after PG.
 */
struct core_state {
	uint32_t a0;
	uint32_t a1;
	uint32_t vecbase;
	uint32_t excsave2;
	uint32_t excsave3;
	uint32_t thread_ptr;
	uint32_t intenable;
	uint32_t ps;
	uint32_t bctl;
#if (XCHAL_NUM_MISC_REGS == 2)
	uint32_t misc[XCHAL_NUM_MISC_REGS];
#endif
};

static struct core_state core_desc[CONFIG_MP_MAX_NUM_CPUS] = {{0}};

struct lpsram_header {
	uint32_t alt_reset_vector;
	uint32_t adsp_lpsram_magic;
	void *lp_restore_vector;
	uint32_t reserved;
	uint32_t slave_core_vector;
	uint8_t rom_bypass_vectors_reserved[0xC00 - 0x14];
};

static ALWAYS_INLINE void _save_core_context(uint32_t core_id)
{
	core_desc[core_id].ps = XTENSA_RSR("PS");
	core_desc[core_id].vecbase = XTENSA_RSR("VECBASE");
	core_desc[core_id].excsave2 = XTENSA_RSR("EXCSAVE2");
	core_desc[core_id].excsave3 = XTENSA_RSR("EXCSAVE3");
	core_desc[core_id].thread_ptr = XTENSA_RUR("THREADPTR");
#if (XCHAL_NUM_MISC_REGS == 2)
	core_desc[core_id].misc[0] = XTENSA_RSR("MISC0");
	core_desc[core_id].misc[1] = XTENSA_RSR("MISC1");
#endif
	__asm__ volatile("mov %0, a0" : "=r"(core_desc[core_id].a0));
	__asm__ volatile("mov %0, a1" : "=r"(core_desc[core_id].a1));

#if CONFIG_MP_MAX_NUM_CPUS == 1
	/* With one core only, the memory is mapped in cache and we need to flush
	 * it.
	 */
	sys_cache_data_flush_range(&core_desc[core_id], sizeof(struct core_state));
#endif
}

static ALWAYS_INLINE void _restore_core_context(void)
{
	uint32_t core_id = arch_proc_id();

	XTENSA_WSR("PS", core_desc[core_id].ps);
	XTENSA_WSR("VECBASE", core_desc[core_id].vecbase);
	XTENSA_WSR("EXCSAVE2", core_desc[core_id].excsave2);
	XTENSA_WSR("EXCSAVE3", core_desc[core_id].excsave3);
	XTENSA_WUR("THREADPTR", core_desc[core_id].thread_ptr);
#if (XCHAL_NUM_MISC_REGS == 2)
	XTENSA_WSR("MISC0", core_desc[core_id].misc[0]);
	XTENSA_WSR("MISC1", core_desc[core_id].misc[1]);
#endif
#ifdef CONFIG_XTENSA_MMU
	xtensa_mmu_reinit();
#endif
	__asm__ volatile("mov a0, %0" :: "r"(core_desc[core_id].a0));
	__asm__ volatile("mov a1, %0" :: "r"(core_desc[core_id].a1));
	__asm__ volatile("rsync");
}

void dsp_restore_vector(void);
void mp_resume_entry(void);

void power_gate_entry(uint32_t core_id)
{
	xthal_window_spill();
	sys_cache_data_flush_and_invd_all();
	_save_core_context(core_id);
	if (core_id == 0) {
		struct lpsram_header *lpsheader =
			(struct lpsram_header *) DT_REG_ADDR(DT_NODELABEL(sram1));

		lpsheader->adsp_lpsram_magic = LPSRAM_MAGIC_VALUE;
		lpsheader->lp_restore_vector = &dsp_restore_vector;
		sys_cache_data_flush_range(lpsheader, sizeof(struct lpsram_header));
		/* Re-enabling interrupts for core 0 because someone has to wake-up us
		 * from power gaiting.
		 */
		z_xt_ints_on(ALL_USED_INT_LEVELS_MASK);
	}

	soc_cpus_active[core_id] = false;
	k_cpu_idle();

	/* It is unlikely we get in here, but when this happens
	 * we need to lock interruptions again.
	 *
	 * @note Zephyr looks PS.INTLEVEL to check if interruptions are locked.
	 */
	(void)arch_irq_lock();
	z_xt_ints_off(0xffffffff);
}

static void __used power_gate_exit(void)
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
	"  movi  a2, " STRINGIFY(PS_UM | PS_WOE | PS_INTLEVEL(XCHAL_EXCM_LEVEL)) "\n\t"
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

#ifdef CONFIG_ADSP_IMR_CONTEXT_SAVE
static ALWAYS_INLINE void power_off_exit(void)
{
	__asm__(
		"  movi  a0, 0\n\t"
		"  movi  a1, 1\n\t"
		"  movi  a2, " STRINGIFY(PS_UM | PS_WOE | PS_INTLEVEL(XCHAL_EXCM_LEVEL)) "\n\t"
		"  wsr   a2, PS\n\t"
		"  wsr   a1, WINDOWSTART\n\t"
		"  wsr   a0, WINDOWBASE\n\t"
		"  rsync\n\t");
	_restore_core_context();
}

__imr void pm_state_imr_restore(void)
{
	struct imr_layout *imr_layout = (struct imr_layout *)(IMR_LAYOUT_ADDRESS);
	/* restore lpsram power and contents */
	bmemcpy(sys_cache_uncached_ptr_get((__sparse_force void __sparse_cache *)
				   UINT_TO_POINTER(LP_SRAM_BASE)),
		imr_layout->imr_state.header.imr_ram_storage,
		LP_SRAM_SIZE);

	/* restore HPSRAM contents, mapping and power states */
	adsp_mm_restore_context(imr_layout->imr_state.header.imr_ram_storage+LP_SRAM_SIZE);

	/* this function won't return, it will restore a saved state */
	power_off_exit();
}
#endif /* CONFIG_ADSP_IMR_CONTEXT_SAVE */

void pm_state_set(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);
	uint32_t cpu = arch_proc_id();
	uint32_t battr;
	int ret;

	ARG_UNUSED(ret);

	/* save interrupt state and turn off all interrupts */
	core_desc[cpu].intenable = XTENSA_RSR("INTENABLE");
	z_xt_ints_off(0xffffffff);

	switch (state) {
	case PM_STATE_SOFT_OFF:
		core_desc[cpu].bctl = DSPCS.bootctl[cpu].bctl;
		DSPCS.bootctl[cpu].bctl &= ~DSPBR_BCTL_WAITIPCG;
		if (cpu == 0) {
			soc_cpus_active[cpu] = false;
			ret = pm_device_runtime_put(INTEL_ADSP_HST_DOMAIN_DEV);
			__ASSERT_NO_MSG(ret == 0);
#ifdef CONFIG_ADSP_IMR_CONTEXT_SAVE
			/* save storage and restore information to imr */
			__ASSERT_NO_MSG(global_imr_ram_storage != NULL);
#endif
			struct imr_layout *imr_layout = (struct imr_layout *)(IMR_LAYOUT_ADDRESS);

			imr_layout->imr_state.header.adsp_imr_magic = ADSP_IMR_MAGIC_VALUE;
#ifdef CONFIG_ADSP_IMR_CONTEXT_SAVE
			sys_cache_data_flush_and_invd_all();
			imr_layout->imr_state.header.imr_restore_vector =
					(void *)boot_entry_d3_restore;
			imr_layout->imr_state.header.imr_ram_storage = global_imr_ram_storage;
			sys_cache_data_flush_range((void *)imr_layout, sizeof(*imr_layout));

			/* save CPU context here
			 * when _restore_core_context() is called, it will return directly to
			 * the caller of this procedure
			 * any changes to CPU context after _save_core_context
			 * will be lost when power_down is executed
			 * Only data in the imr region survives
			 */
			xthal_window_spill();
			_save_core_context(cpu);

			/* save LPSRAM - a simple copy */
			memcpy(global_imr_ram_storage, (void *)LP_SRAM_BASE, LP_SRAM_SIZE);

			/* save HPSRAM - a multi step procedure, executed by a TLB driver
			 * the TLB driver will change memory mapping
			 * leaving the system not operational
			 * it must be called directly here,
			 * just before power_down
			 */
			const struct device *tlb_dev = DEVICE_DT_GET(DT_NODELABEL(tlb));

			__ASSERT_NO_MSG(tlb_dev != NULL);
			const struct intel_adsp_tlb_api *tlb_api =
					(struct intel_adsp_tlb_api *)tlb_dev->api;

			tlb_api->save_context(global_imr_ram_storage+LP_SRAM_SIZE);
#else
			imr_layout->imr_state.header.imr_restore_vector =
					(void *)rom_entry;
			sys_cache_data_flush_range((void *)imr_layout, sizeof(*imr_layout));
#endif /* CONFIG_ADSP_IMR_CONTEXT_SAVE */
			/* do power down - this function won't return */
			power_down(true, CONFIG_ADSP_POWER_DOWN_HPSRAM, true);
		} else {
			power_gate_entry(cpu);
		}
		break;

	/* Only core 0 handles this state */
	case PM_STATE_RUNTIME_IDLE:
		battr = DSPCS.bootctl[cpu].battr & (~LPSCTL_BATTR_MASK);

		DSPCS.bootctl[cpu].bctl &= ~DSPBR_BCTL_WAITIPPG;
		DSPCS.bootctl[cpu].bctl &= ~DSPBR_BCTL_WAITIPCG;
		soc_cpu_power_down(cpu);
		battr |= (DSPBR_BATTR_LPSCTL_RESTORE_BOOT & LPSCTL_BATTR_MASK);
		DSPCS.bootctl[cpu].battr = battr;

		ret = pm_device_runtime_put(INTEL_ADSP_HST_DOMAIN_DEV);
		__ASSERT_NO_MSG(ret == 0);

		power_gate_entry(cpu);
		break;
	default:
		__ASSERT(false, "invalid argument - unsupported power state");
	}
}

/* Handle SOC specific activity after Low Power Mode Exit */
void pm_state_exit_post_ops(enum pm_state state, uint8_t substate_id)
{
	ARG_UNUSED(substate_id);
	uint32_t cpu = arch_proc_id();

	if (cpu == 0) {
		int ret = pm_device_runtime_get(INTEL_ADSP_HST_DOMAIN_DEV);

		ARG_UNUSED(ret);
		__ASSERT_NO_MSG(ret == 0);
	}

	if (state == PM_STATE_SOFT_OFF) {
		/* restore clock gating state */
		DSPCS.bootctl[cpu].bctl |=
			(core_desc[cpu].bctl & DSPBR_BCTL_WAITIPCG);

#ifdef CONFIG_ADSP_IMR_CONTEXT_SAVE
		if (cpu == 0) {
			struct imr_layout *imr_layout = (struct imr_layout *)(IMR_LAYOUT_ADDRESS);

			/* clean storage and restore information */
			sys_cache_data_invd_range(imr_layout, sizeof(*imr_layout));
			imr_layout->imr_state.header.adsp_imr_magic = 0;
			imr_layout->imr_state.header.imr_restore_vector = NULL;
			imr_layout->imr_state.header.imr_ram_storage = NULL;
			intel_adsp_clock_soft_off_exit();
			mem_window_idle_exit();
			soc_mp_on_d3_exit();
		}
#endif /* CONFIG_ADSP_IMR_CONTEXT_SAVE */
		soc_cpus_active[cpu] = true;
		sys_cache_data_flush_and_invd_all();
	} else if (state == PM_STATE_RUNTIME_IDLE) {
		soc_cpu_power_up(cpu);

		if (!WAIT_FOR(soc_cpu_is_powered(cpu),
			      CPU_POWERUP_TIMEOUT_USEC, k_busy_wait(HW_STATE_CHECK_DELAY))) {
			k_panic();
		}

#if CONFIG_ADSP_IDLE_CLOCK_GATING
		DSPCS.bootctl[cpu].bctl |= DSPBR_BCTL_WAITIPPG;
#else
		DSPCS.bootctl[cpu].bctl |= DSPBR_BCTL_WAITIPCG | DSPBR_BCTL_WAITIPPG;
#endif /* CONFIG_ADSP_IDLE_CLOCK_GATING */
		DSPCS.bootctl[cpu].battr &= (~LPSCTL_BATTR_MASK);

		soc_cpus_active[cpu] = true;
		sys_cache_data_flush_and_invd_all();
	} else {
		__ASSERT(false, "invalid argument - unsupported power state");
	}

	z_xt_ints_on(core_desc[cpu].intenable);

	/* We don't have the key used to lock interruptions here.
	 * Just set PS.INTLEVEL to 0.
	 */
	__asm__ volatile ("rsil a2, 0");
}

#endif /* CONFIG_PM */

#ifdef CONFIG_ARCH_HAS_CUSTOM_CPU_IDLE

__no_optimization
void arch_cpu_idle(void)
{
	uint32_t cpu = arch_proc_id();

	sys_trace_idle();

	/*
	 * unlock and invalidate icache if clock gating is allowed
	 */
	if (!(DSPCS.bootctl[cpu].bctl & DSPBR_BCTL_WAITIPCG)) {
		xthal_icache_all_unlock();
		xthal_icache_all_invalidate();
	}

	__asm__ volatile ("waiti 0");
}

#endif /* CONFIG_ARCH_HAS_CUSTOM_CPU_IDLE */
