/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <zephyr/arch/common/pm_s2ram.h>
#include <zephyr/arch/arm/cortex_m/fpu.h>
#include <zephyr/arch/arm/cortex_m/scb.h>
#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/linker/sections.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/crc.h>
#include <zephyr/sys/util.h>
#include <cmsis_core.h>
#include <soc.h>
#include <fsl_cache.h>
#include <fsl_clock.h>
#include <fsl_cmc.h>
#include <fsl_spc.h>
#include <fsl_vbat.h>
#include <errno.h>
#include <string.h>

LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

BUILD_ASSERT(IS_ENABLED(CONFIG_SOC_MCXW716C) || IS_ENABLED(CONFIG_SOC_MCXW727C),
	     "MCXW S2RAM supports MCXW716C and MCXW727C application core0");
BUILD_ASSERT(!IS_ENABLED(CONFIG_NXP_NBU) && !IS_ENABLED(CONFIG_TRUSTED_EXECUTION_NONSECURE) &&
	     !IS_ENABLED(CONFIG_BUILD_WITH_TFM), "MCXW S2RAM requires standalone secure core0");
BUILD_ASSERT(!IS_ENABLED(CONFIG_CRC_HW_HANDLER), "MCXW S2RAM requires software CRC dispatch");
BUILD_ASSERT(IS_ENABLED(CONFIG_XIP), "MCXW S2RAM requires internal-flash XIP");
BUILD_ASSERT(DT_NODE_HAS_COMPAT(DT_CHOSEN(zephyr_system_timer), nxp_lptmr),
	     "MCXW S2RAM requires the LPTMR system timer");

BUILD_ASSERT(DT_REG_ADDR(DT_CHOSEN(zephyr_sram)) >=
	     MCXW7X_S2RAM_ROM_BASE + MCXW7X_S2RAM_ROM_SIZE,
	     "MCXW7x S2RAM kernel SRAM must start above the wake workspace");

#define S2RAM_CMC ((CMC_Type *)DT_REG_ADDR(DT_NODELABEL(cmc)))
#define S2RAM_SPC ((SPC_Type *)DT_REG_ADDR(DT_NODELABEL(spc)))
#define S2RAM_VBAT ((VBAT_Type *)DT_REG_ADDR(DT_NODELABEL(vbat)))
#define S2RAM_REG0 ((REGFILE_Type *)DT_REG_ADDR(DT_NODELABEL(regfile0)))
#define S2RAM_REG1 ((REGFILE_Type *)DT_REG_ADDR(DT_NODELABEL(regfile1)))
#define S2RAM_MARK 0x6d637877U
#define S2RAM_DIAGNOSTIC 0x53520000U
#define S2RAM_DIAGNOSTIC_MASK 0xffff0000U
#define S2RAM_DIAGNOSTIC_SLOT 6U
#define S2RAM_POLL_LIMIT 1000000U
#define S2RAM_NVIC_WORDS DIV_ROUND_UP(CONFIG_NUM_IRQS, 32)
#define S2RAM_MPU_REGIONS 16U

#define MRCC_OFFSET(member) offsetof(MRCC_Type, member)

static const uint16_t mrcc_offsets[] = {
	MRCC_OFFSET(MRCC_EWM0), MRCC_OFFSET(MRCC_SYSPM0), MRCC_OFFSET(MRCC_SFA0),
	MRCC_OFFSET(MRCC_CRC0), MRCC_OFFSET(MRCC_LPIT0),
#if defined(CONFIG_SOC_MCXW716C)
	MRCC_OFFSET(MRCC_TSTMR0),
#endif
	MRCC_OFFSET(MRCC_TPM0), MRCC_OFFSET(MRCC_TPM1), MRCC_OFFSET(MRCC_LPI2C0),
	MRCC_OFFSET(MRCC_LPI2C1), MRCC_OFFSET(MRCC_I3C0), MRCC_OFFSET(MRCC_LPSPI0),
	MRCC_OFFSET(MRCC_LPSPI1), MRCC_OFFSET(MRCC_LPUART0), MRCC_OFFSET(MRCC_LPUART1),
	MRCC_OFFSET(MRCC_FLEXIO0), MRCC_OFFSET(MRCC_CAN0), MRCC_OFFSET(MRCC_SEMA0),
	MRCC_OFFSET(MRCC_DATA_STREAM_2P4), MRCC_OFFSET(MRCC_PORTA), MRCC_OFFSET(MRCC_PORTB),
	MRCC_OFFSET(MRCC_PORTC), MRCC_OFFSET(MRCC_LPADC0), MRCC_OFFSET(MRCC_LPCMP0),
	MRCC_OFFSET(MRCC_LPCMP1), MRCC_OFFSET(MRCC_VREF0), MRCC_OFFSET(MRCC_GPIOA),
	MRCC_OFFSET(MRCC_GPIOB), MRCC_OFFSET(MRCC_GPIOC), MRCC_OFFSET(MRCC_DMA0),
	MRCC_OFFSET(MRCC_PFLEXNVM), MRCC_OFFSET(MRCC_SRAM0),
#if defined(CONFIG_SOC_MCXW716C)
	MRCC_OFFSET(MRCC_SRAM1),
#else
	MRCC_OFFSET(MRCC_SRAM1_ECC), MRCC_OFFSET(MRCC_MTR_MASTER), MRCC_OFFSET(MRCC_CAN1),
	MRCC_OFFSET(MRCC_SRAM0_NOECC), MRCC_OFFSET(MRCC_SRAM1_NOECC),
	MRCC_OFFSET(MRCC_DSP0), MRCC_OFFSET(MRCC_DSP0_MUA),
	MRCC_OFFSET(MRCC_LPTMR0_IPG_CLK_ERCLK_WAKE2VSYS),
	MRCC_OFFSET(MRCC_LPTMR1_IPG_CLK_ERCLK_WAKE2VSYS),
	MRCC_OFFSET(MRCC_LPTMR2_IPG_CLK_ERCLK_WAKE2VSYS),
#endif
	MRCC_OFFSET(MRCC_SRAM2), MRCC_OFFSET(MRCC_SRAM3),
};

struct s2ram_record {
	uintptr_t sp;
	void (*entry)(void);
	uint32_t reserved[10];
};

struct s2ram_context {
	struct scb_context scb;
	uint32_t nvic_enable[S2RAM_NVIC_WORDS];
	uint32_t nvic_pending[S2RAM_NVIC_WORDS];
	uint8_t nvic_priority[CONFIG_NUM_IRQS];
#if defined(__ARM_FEATURE_CMSE) && (__ARM_FEATURE_CMSE == 3)
	uint32_t nvic_target[S2RAM_NVIC_WORDS];
#endif
#if defined(CONFIG_ARM_MPU)
	uint32_t mpu_control;
	uint32_t mpu_select;
	uint32_t mpu_count;
	uint32_t mpu_attributes[2];
	struct {
		uint32_t base;
		uint32_t limit;
	} mpu_regions[S2RAM_MPU_REGIONS];
#endif
#if defined(CONFIG_FPU)
	struct fpu_ctx_full fp;
	uint32_t fpccr;
	uint32_t fpcar;
	uint32_t fpdscr;
#endif
	uint32_t mrcc[ARRAY_SIZE(mrcc_offsets)];
	uint32_t rccr;
	uint32_t firccsr;
	uint32_t firccfg;
	uint32_t firctcfg;
	uint32_t fircstat;
	uint32_t sosccsr;
#if defined(CONFIG_SOC_MCXW716C)
	uint32_t sirccsr;
#endif
	uint32_t clock_out;
	uint32_t core_frequency;
	uint32_t flash_control;
	uint32_t cache_control;
	uint32_t ram_retention;
	uint32_t vbat_fro;
	uint32_t vbat_ldo;
	uint32_t radio_control;
	uint32_t secsubsys_clock;
	uint32_t clock_mode;
	uint32_t power_modes[CMC_PMCTRL_COUNT];
	uint32_t spc_low_power;
	uint32_t old_wakeup;
	uint32_t old_crc;
	uint8_t reset_count;
	bool entry_requested;
};

/*
 * The cold ECC hook must clear this before architecture resume probing.
 * The default private magic cannot also qualify the ROM wake/reset cause.
 */
uint32_t __noinit nxp_mcxw7x_s2ram_mark;
static struct s2ram_context __noinit context;
static struct s2ram_record __noinit __aligned(8) record;
static uint8_t __noinit __aligned(8) resume_stack[512];

BUILD_ASSERT(sizeof(record) == 48, "ROM wake record must contain exactly 48 bytes");

extern void nxp_mcxw7x_s2ram_entry(void);
extern void arch_pm_s2ram_resume(void);

static int wait_register(const volatile uint32_t *reg, uint32_t mask, uint32_t expected)
{
	for (uint32_t i = 0; i < S2RAM_POLL_LIMIT; i++) {
		if ((*reg & mask) == expected) {
			return 0;
		}
	}

	return -ETIMEDOUT;
}

void pm_s2ram_mark_set(void)
{
	S2RAM_REG0->REG[S2RAM_DIAGNOSTIC_SLOT] = S2RAM_DIAGNOSTIC;
	nxp_mcxw7x_s2ram_mark = S2RAM_MARK;
	__DSB();
}

static bool recovery_valid(bool marked)
{
	uint32_t expected;

	if (!marked || (S2RAM_CMC->SRS & CMC_SRS_WAKEUP_MASK) == 0) {
		return false;
	}

	expected = MIN(context.reset_count + 1U, CMC_RSTCNT_COUNT_MASK);
	return S2RAM_CMC->RSTCNT == expected;
}

bool pm_s2ram_mark_check_and_clear(void)
{
	uint32_t mark = nxp_mcxw7x_s2ram_mark;

	nxp_mcxw7x_s2ram_mark = 0;
	return recovery_valid(mark == S2RAM_MARK);
}

static FUNC_NORETURN void resume_failed(uint32_t reason)
{
	S2RAM_REG0->REG[S2RAM_DIAGNOSTIC_SLOT] = S2RAM_DIAGNOSTIC | reason;
	__DSB();
	NVIC_SystemReset();
	CODE_UNREACHABLE;
}

static int report_boot_failure(void)
{
	uint32_t diagnostic = S2RAM_REG0->REG[S2RAM_DIAGNOSTIC_SLOT];

	if ((diagnostic & S2RAM_DIAGNOSTIC_MASK) == S2RAM_DIAGNOSTIC) {
		LOG_ERR("S2RAM recovery failed: %u", diagnostic & ~S2RAM_DIAGNOSTIC_MASK);
		S2RAM_REG0->REG[S2RAM_DIAGNOSTIC_SLOT] = 0;
	}

	return 0;
}

SYS_INIT(report_boot_failure, APPLICATION, 0);

static volatile uint32_t *mrcc_register(size_t index)
{
	return (volatile uint32_t *)((uintptr_t)MRCC + mrcc_offsets[index]);
}

static int save_context(void)
{
	z_arm_save_scb_context(&context.scb);
	for (size_t i = 0; i < S2RAM_NVIC_WORDS; i++) {
		context.nvic_enable[i] = NVIC->ISER[i];
		context.nvic_pending[i] = NVIC->ISPR[i];
#if defined(__ARM_FEATURE_CMSE) && (__ARM_FEATURE_CMSE == 3)
		context.nvic_target[i] = NVIC->ITNS[i];
#endif
	}
	for (size_t i = 0; i < CONFIG_NUM_IRQS; i++) {
		context.nvic_priority[i] = NVIC->IPR[i];
	}
#if defined(CONFIG_ARM_MPU)
	context.mpu_count = (MPU->TYPE & MPU_TYPE_DREGION_Msk) >> MPU_TYPE_DREGION_Pos;
	if (context.mpu_count > S2RAM_MPU_REGIONS) {
		return -ENOTSUP;
	}
	context.mpu_control = MPU->CTRL;
	context.mpu_select = MPU->RNR;
	context.mpu_attributes[0] = MPU->MAIR0;
	context.mpu_attributes[1] = MPU->MAIR1;
	for (uint32_t i = 0; i < context.mpu_count; i++) {
		MPU->RNR = i;
		context.mpu_regions[i].base = MPU->RBAR;
		context.mpu_regions[i].limit = MPU->RLAR;
	}
	MPU->RNR = context.mpu_select;
#endif
	context.rccr = SCG0->RCCR;
	context.firccsr = SCG0->FIRCCSR;
	context.firccfg = SCG0->FIRCCFG;
	context.firctcfg = SCG0->FIRCTCFG;
	context.fircstat = SCG0->FIRCSTAT;
	context.sosccsr = SCG0->SOSCCSR;
#if defined(CONFIG_SOC_MCXW716C)
	context.sirccsr = SCG0->SIRCCSR;
#endif
	context.clock_out = SCG0->CLKOUTCNFG;
	context.core_frequency = SystemCoreClock;
	context.flash_control = FMU0->FCTRL;
	context.cache_control = MCM->CPCR2;
	for (size_t i = 0; i < ARRAY_SIZE(mrcc_offsets); i++) {
		context.mrcc[i] = *mrcc_register(i);
	}
#if defined(CONFIG_FPU)
	context.fp.ctx_saved = false;
	z_arm_save_fp_context(&context.fp);
	context.fpccr = FPU->FPCCR;
	context.fpcar = FPU->FPCAR;
	context.fpdscr = FPU->FPDSCR;
#endif
	return 0;
}

static void restore_radio_request(void)
{
	RFMC->RF2P4GHZ_CTRL &= ~RFMC_RF2P4GHZ_CTRL_LP_ENTER_MASK;
	RFMC->RF2P4GHZ_CTRL = (RFMC->RF2P4GHZ_CTRL & ~RFMC_RF2P4GHZ_CTRL_LP_MODE_MASK) |
			    (context.radio_control & RFMC_RF2P4GHZ_CTRL_LP_MODE_MASK);
	RFMC->RF2P4GHZ_CTRL |= context.radio_control & RFMC_RF2P4GHZ_CTRL_LP_ENTER_MASK;
}

static int restore_clocks(void)
{
	const scg_sys_clk_config_t safe = {
		.divCore = kSCG_SysClkDivBy1,
		.divBus = kSCG_SysClkDivBy1,
		.divSlow = kSCG_SysClkDivBy4,
		.src = kSCG_SysClkSrcSirc,
	};
	const uint32_t firc_controls = SCG_FIRCCSR_FIRCEN_MASK | SCG_FIRCCSR_FIRCSTEN_MASK |
				      SCG_FIRCCSR_FIRCTREN_MASK | SCG_FIRCCSR_FIRCTRUP_MASK |
				      SCG_FIRCCSR_COARSE_TRIM_BYPASS_MASK;
	const uint32_t sosc_controls = SCG_SOSCCSR_SOSCEN_MASK | SCG_SOSCCSR_SOSCSTEN_MASK |
				      SCG_SOSCCSR_SOSCCM_MASK | SCG_SOSCCSR_SOSCCMRE_MASK;
	int ret;

	restore_radio_request();
	ret = wait_register(&RFMC->RF2P4GHZ_STAT, RFMC_RF2P4GHZ_STAT_FRO_CLK_VLD_STAT_MASK,
			    RFMC_RF2P4GHZ_STAT_FRO_CLK_VLD_STAT_MASK);
	if (ret != 0) {
		return ret;
	}
	CLOCK_SetRunModeSysClkConfig(&safe);
	ret = wait_register(&SCG0->CSR, SCG_CSR_SCS_MASK, SCG_CSR_SCS(safe.src));
	if (ret != 0) {
		return ret;
	}
	CLOCK_UnlockFircControlStatusReg();
	SCG0->FIRCCSR &= ~SCG_FIRCCSR_FIRCEN_MASK;
	ret = wait_register(&SCG0->FIRCCSR, SCG_FIRCCSR_FIRCVLD_MASK, 0);
	if (ret != 0) {
		return ret;
	}
	SCG0->FIRCCFG = context.firccfg;
	SCG0->FIRCTCFG = context.firctcfg;
	SCG0->FIRCSTAT = context.fircstat & SCG_FIRCSTAT_TRIMFINE_MASK;
	SCG0->FIRCCSR = (SCG0->FIRCCSR & ~firc_controls) | (context.firccsr & firc_controls);
	if ((context.firccsr & SCG_FIRCCSR_FIRCEN_MASK) != 0) {
		ret = wait_register(&SCG0->FIRCCSR, SCG_FIRCCSR_FIRCVLD_MASK,
				    SCG_FIRCCSR_FIRCVLD_MASK);
		if (ret != 0) {
			return ret;
		}
	}
	CLOCK_UnlockSysOscControlStatusReg();
	SCG0->SOSCCSR = (SCG0->SOSCCSR & ~sosc_controls) | (context.sosccsr & sosc_controls);
	if ((context.sosccsr & SCG_SOSCCSR_SOSCEN_MASK) != 0) {
		ret = wait_register(&SCG0->SOSCCSR, SCG_SOSCCSR_SOSCVLD_MASK,
				    SCG_SOSCCSR_SOSCVLD_MASK);
		if (ret != 0) {
			return ret;
		}
	}
	ret = wait_register(&S2RAM_SPC->SC, SPC_SC_BUSY_MASK, 0);
	if (ret != 0) {
		return ret;
	}
	FMU0->FCTRL = (FMU0->FCTRL & ~FMU_FCTRL_RWSC_MASK) |
		      (context.flash_control & FMU_FCTRL_RWSC_MASK);
	SCG0->RCCR = context.rccr;
	ret = wait_register(&SCG0->CSR, SCG_CSR_SCS_MASK, context.rccr & SCG_RCCR_SCS_MASK);
	if (ret != 0) {
		return ret;
	}
#if defined(CONFIG_SOC_MCXW716C)
	CLOCK_UnlockSircControlStatusReg();
	SCG0->SIRCCSR = (SCG0->SIRCCSR & ~SCG_SIRCCSR_SIRCSTEN_MASK) |
		       (context.sirccsr & SCG_SIRCCSR_SIRCSTEN_MASK);
	SCG0->SIRCCSR |= context.sirccsr & SCG_SIRCCSR_LK_MASK;
#endif
	SCG0->FIRCCSR |= context.firccsr & SCG_FIRCCSR_LK_MASK;
	SCG0->SOSCCSR |= context.sosccsr & SCG_SOSCCSR_LK_MASK;
	SCG0->CLKOUTCNFG = context.clock_out;
	for (size_t i = 0; i < ARRAY_SIZE(mrcc_offsets); i++) {
		*mrcc_register(i) = context.mrcc[i];
	}
	SystemCoreClock = CLOCK_GetCoreSysClkFreq();
	return SystemCoreClock == context.core_frequency ? 0 : -EIO;
}

static void restore_core(void)
{
	MCM->CPCR2 = context.cache_control | MCM_CPCR2_DCBC_MASK;
	L1CACHE_InvalidateCodeCache();
	MCM->CPCR2 = context.cache_control & ~MCM_CPCR2_CCBC_MASK;
	z_arm_restore_scb_context(&context.scb);
#if defined(CONFIG_ARM_MPU)
	MPU->CTRL = 0;
	__DSB();
	__ISB();
	MPU->MAIR0 = context.mpu_attributes[0];
	MPU->MAIR1 = context.mpu_attributes[1];
	for (uint32_t i = 0; i < context.mpu_count; i++) {
		MPU->RNR = i;
		MPU->RBAR = context.mpu_regions[i].base;
		MPU->RLAR = context.mpu_regions[i].limit;
	}
	MPU->RNR = context.mpu_select;
	MPU->CTRL = context.mpu_control;
	__DSB();
	__ISB();
#endif
#if defined(CONFIG_FPU)
	FPU->FPCCR = context.fpccr;
	FPU->FPCAR = context.fpcar;
	FPU->FPDSCR = context.fpdscr;
#endif
	for (size_t i = 0; i < S2RAM_NVIC_WORDS; i++) {
		NVIC->ICER[i] = UINT32_MAX;
#if defined(__ARM_FEATURE_CMSE) && (__ARM_FEATURE_CMSE == 3)
		NVIC->ITNS[i] = context.nvic_target[i];
#endif
	}
	for (size_t i = 0; i < CONFIG_NUM_IRQS; i++) {
		NVIC->IPR[i] = context.nvic_priority[i];
	}
	for (size_t i = 0; i < S2RAM_NVIC_WORDS; i++) {
		NVIC->ISPR[i] = context.nvic_pending[i] | NVIC->ISPR[i];
		NVIC->ISER[i] = context.nvic_enable[i];
	}
	__DSB();
	__ISB();
}

FUNC_NORETURN void nxp_mcxw7x_s2ram_resume(void)
{
	bool marked = nxp_mcxw7x_s2ram_mark == S2RAM_MARK;

#if defined(CONFIG_SOC_MCXW727C)
	nxp_mcxw72_trdc_init();
#endif
	if (!recovery_valid(marked)) {
		resume_failed(1);
	}
	if (restore_clocks() != 0) {
		resume_failed(2);
	}
	restore_core();
	arch_pm_s2ram_resume();
	resume_failed(3);
}

static int enable_retention(void)
{
	if ((S2RAM_VBAT->FROLCKA & VBAT_FROLCKA_LOCK_MASK) != 0 ||
	    (S2RAM_VBAT->LDOLCKA & VBAT_LDOLCKA_LOCK_MASK) != 0) {
		return -EBUSY;
	}
	VBAT_EnableFRO16k(S2RAM_VBAT, true);
	if (!VBAT_CheckFRO16kEnabled(S2RAM_VBAT) ||
	    VBAT_EnableBandgap(S2RAM_VBAT, true) != kStatus_Success) {
		return -EIO;
	}
	if (!VBAT_CheckBandgapEnabled(S2RAM_VBAT)) {
		return -EIO;
	}
	/* The HAL's LDO-ready loop does not stop when its timeout expires. */
	S2RAM_VBAT->LDOCTLA |= VBAT_LDOCTLA_LDO_EN_MASK;
	int ret = wait_register(&S2RAM_VBAT->STATUSA, VBAT_STATUSA_LDO_RDY_MASK,
				VBAT_STATUSA_LDO_RDY_MASK);

	if (ret == 0) {
		CMC_PowerOnSRAMLowPowerOnly(S2RAM_CMC, CMC_SRAMRET_RET_MASK);
	}
	return ret;
}

static void restore_entry_settings(void)
{
	S2RAM_CMC->SRAMRET[0] = context.ram_retention;
	S2RAM_SPC->LP_CFG = (S2RAM_SPC->LP_CFG & ~SPC_LP_CFG_COREVDD_IVS_EN_MASK) |
			    (context.spc_low_power & SPC_LP_CFG_COREVDD_IVS_EN_MASK);
	if (context.entry_requested) {
		restore_radio_request();
		S2RAM_CMC->CKCTRL = context.clock_mode;
		for (size_t i = 0; i < CMC_PMCTRL_COUNT; i++) {
			S2RAM_CMC->PMCTRL[i] = context.power_modes[i];
		}
		MRCC->MRCC_SECSUBSYS = (MRCC->MRCC_SECSUBSYS & ~MRCC_MRCC_SECSUBSYS_CC_MASK) |
				      (context.secsubsys_clock & MRCC_MRCC_SECSUBSYS_CC_MASK);
	}
	S2RAM_VBAT->LDOCTLA = context.vbat_ldo;
	S2RAM_VBAT->FROCTLA = context.vbat_fro;
	S2RAM_SPC->WAKEUP = context.old_wakeup;
	S2RAM_REG1->REG[0] = context.old_crc;
	S2RAM_REG0->REG[S2RAM_DIAGNOSTIC_SLOT] = 0;
}

static int power_down(void)
{
	const cmc_power_domain_config_t power = {
		.clock_mode = kCMC_GateAllSystemClocksEnterLowPowerMode,
		.main_domain = kCMC_PowerDownMode,
		.wake_domain = kCMC_DeepSleepMode,
	};

	for (size_t i = 0; i < S2RAM_NVIC_WORDS; i++) {
		if ((NVIC->ISPR[i] & NVIC->ISER[i]) != 0) {
			return -EBUSY;
		}
	}
	context.entry_requested = true;
	SPC_EnableLowPowerModeCoreVDDInternalVoltageScaling(S2RAM_SPC, true);
	RFMC->RF2P4GHZ_CTRL &= ~RFMC_RF2P4GHZ_CTRL_LP_ENTER_MASK;
	RFMC->RF2P4GHZ_CTRL = (RFMC->RF2P4GHZ_CTRL & ~RFMC_RF2P4GHZ_CTRL_LP_MODE_MASK) |
			     RFMC_RF2P4GHZ_CTRL_LP_MODE(7);
	RFMC->RF2P4GHZ_CTRL |= RFMC_RF2P4GHZ_CTRL_LP_ENTER_MASK;
	CLOCK_DisableClock(kCLOCK_Secsubsys);
	CMC_EnterLowPowerMode(S2RAM_CMC, &power);
	return -EBUSY;
}

int nxp_mcxw7x_s2ram_suspend(void)
{
	int ret;

	context.entry_requested = false;
	if ((S2RAM_CMC->CKCTRL & CMC_CKCTRL_LOCK_MASK) != 0) {
		return -EBUSY;
	}
	context.ram_retention = S2RAM_CMC->SRAMRET[0];
	context.vbat_fro = S2RAM_VBAT->FROCTLA;
	context.vbat_ldo = S2RAM_VBAT->LDOCTLA;
	context.radio_control = RFMC->RF2P4GHZ_CTRL;
	context.secsubsys_clock = MRCC->MRCC_SECSUBSYS;
	context.clock_mode = S2RAM_CMC->CKCTRL;
	for (size_t i = 0; i < CMC_PMCTRL_COUNT; i++) {
		context.power_modes[i] = S2RAM_CMC->PMCTRL[i];
	}
	context.spc_low_power = S2RAM_SPC->LP_CFG;
	context.old_wakeup = S2RAM_SPC->WAKEUP;
	context.old_crc = S2RAM_REG1->REG[0];
	context.reset_count = S2RAM_CMC->RSTCNT;
	ret = enable_retention();
	if (ret != 0) {
		restore_entry_settings();
		return ret;
	}
	ret = save_context();
	if (ret != 0) {
		restore_entry_settings();
		return ret;
	}
	memset(&record, 0, sizeof(record));
	record.sp = (uintptr_t)(resume_stack + sizeof(resume_stack));
	record.entry = nxp_mcxw7x_s2ram_entry;
	S2RAM_REG1->REG[0] = crc32_mpeg2((const uint8_t *)&record, sizeof(record));
	SPC_SetWakeUpValue(S2RAM_SPC, (uintptr_t)&record);
	__DSB();
	ret = arch_pm_s2ram_suspend(power_down);
#if defined(CONFIG_FPU)
	/* FPCA must be restored after the architecture restores its saved CONTROL. */
	z_arm_restore_fp_context(&context.fp);
#endif
	restore_entry_settings();
	return ret;
}
