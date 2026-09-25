/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * Copyright (c) 2025-2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/arch/arm/cortex_m/fpu.h>
#include <zephyr/arch/arm/cortex_m/scb.h>
#include <zephyr/arch/arm/mpu/arm_mpu.h>
#include <zephyr/arch/common/pm_s2ram.h>
#include <zephyr/sys/util_macro.h>
#include <mxc_device.h>
#include <gcr_regs.h>
#include <mcr_regs.h>
#include <wrap_max32_lp.h>

#define NVIC_MEMBER_SIZE(member) ARRAY_SIZE(((NVIC_Type *)0)->member)

#define MXC_GCR_REVISION_LOW_MASK     GENMASK(7, 0)
#define MXC_GCR_REVISION_LOW_EXPECTED 0xA3

#define WARM_BOOT_RESERVED_START 0x3000a558
#define WARM_BOOT_RESERVED_END   (WARM_BOOT_RESERVED_START + 8)

/* Coprocessor Power Control Register Definitions */
#define SCnSCB_CPPWR_SU11_Pos 22U                            /*!< CPPWR: SU11 Position */
#define SCnSCB_CPPWR_SU11_Msk (1UL << SCnSCB_CPPWR_SU11_Pos) /*!< CPPWR: SU11 Mask */

#define SCnSCB_CPPWR_SU10_Pos 20U                            /*!< CPPWR: SU10 Position */
#define SCnSCB_CPPWR_SU10_Msk (1UL << SCnSCB_CPPWR_SU10_Pos) /*!< CPPWR: SU10 Mask */

typedef struct {
	/* NVIC components stored into RAM. */
	uint32_t ISER[NVIC_MEMBER_SIZE(ISER)];
	uint32_t ISPR[NVIC_MEMBER_SIZE(ISPR)];
	uint8_t IPR[NVIC_MEMBER_SIZE(IPR)];
} _nvic_context_t;

struct backup {
	_nvic_context_t nvic_context;
#if defined(CONFIG_ARM_MPU)
	struct z_mpu_context_retained mpu_context;
#endif
	struct scb_context scb_context;
#if defined(CONFIG_FPU) && !defined(CONFIG_FPU_SHARING)
	struct fpu_ctx_full fpu_context;
#endif
};

static __noinit struct backup backup_data;

/* Construct retention mask from SRAM devicetree definition */
#define RET_MASK                                                                                   \
	((DT_PROP(DT_NODELABEL(sram4), adi_ram_bank_retained) << 4) |                              \
	 (DT_PROP(DT_NODELABEL(sram3), adi_ram_bank_retained) << 3) |                              \
	 (DT_PROP(DT_NODELABEL(sram2), adi_ram_bank_retained) << 2) |                              \
	 (DT_PROP(DT_NODELABEL(sram1), adi_ram_bank_retained) << 1) |                              \
	 DT_PROP(DT_NODELABEL(sram0), adi_ram_bank_retained))


static void nvic_save(_nvic_context_t *backup)
{
	memcpy(backup->ISER, (uint32_t *)NVIC->ISER, sizeof(NVIC->ISER));
	memcpy(backup->ISPR, (uint32_t *)NVIC->ISPR, sizeof(NVIC->ISPR));
	memcpy(backup->IPR, (uint32_t *)NVIC->IPR, sizeof(NVIC->IPR));
}

static void nvic_restore(_nvic_context_t *backup)
{
	memcpy((uint32_t *)NVIC->ISER, backup->ISER, sizeof(NVIC->ISER));
	memcpy((uint32_t *)NVIC->ISPR, backup->ISPR, sizeof(NVIC->ISPR));
	memcpy((uint32_t *)NVIC->IPR, backup->IPR, sizeof(NVIC->IPR));
}

/* Naked wrapper that adjusts SP away from the warm-boot reserved region
 * before calling arch_pm_s2ram_suspend.
 */
#if defined(CONFIG_HAS_PM_S2RAM_CUSTOM_MARKING) && defined(CONFIG_TRUSTED_EXECUTION_SECURE) &&     \
	defined(CONFIG_SOC_MAX32657)
static void __attribute__((naked)) pm_s2ram_suspend_with_sp_pad(
	pm_s2ram_system_off_fn_t system_off, uintptr_t sp_pad)
{
	__asm__ volatile(
		"sub	sp, sp, r1\n"
		"push	{r4, r5, r6, lr}\n"
		"mov	r4, r0\n"
		"mov	r5, r1\n"
		/* Save 8 bytes from ROM stack area onto the stack */
		"ldr	r0, =" STRINGIFY(WARM_BOOT_RESERVED_START) "\n"
		"ldrd	r2, r3, [r0]\n"
		"push	{r2, r3}\n"
		/* Call arch_pm_s2ram_suspend with system_off */
		"mov	r0, r4\n"
		"bl	arch_pm_s2ram_suspend\n"
		/* Restore 8 bytes from stack to ROM stack area */
		"pop	{r2, r3}\n"
		"ldr	r0, =" STRINGIFY(WARM_BOOT_RESERVED_START) "\n"
		"strd	r2, r3, [r0]\n"
		"add	sp, sp, r5\n"
		"pop	{r4, r5, r6, pc}\n"
	);
}
#endif

#if defined(CONFIG_FPU)
static void fpu_power_down(void)
{
	SCB->CPACR &= (~(CPACR_CP10_Msk | CPACR_CP11_Msk));
	SCnSCB->CPPWR |= (SCnSCB_CPPWR_SU11_Msk | SCnSCB_CPPWR_SU10_Msk);
	__DSB();
	__ISB();
}

static void fpu_power_up(void)
{
	SCnSCB->CPPWR &= (~(SCnSCB_CPPWR_SU11_Msk | SCnSCB_CPPWR_SU10_Msk));
	SCB->CPACR |= (CPACR_CP10_Msk | CPACR_CP11_Msk);
	__DSB();
	__ISB();
}
#endif /* defined(CONFIG_FPU) */

void pm_s2ram_suspend(pm_s2ram_system_off_fn_t system_off)
{
	Wrap_MXC_LP_EnableRetentionReg();
	Wrap_MXC_LP_EnableSramRetention(RET_MASK);

	/* Save FPU, SCB and MPU states */
	z_arm_save_scb_context(&backup_data.scb_context);
#if defined(CONFIG_FPU)
#if !defined(CONFIG_FPU_SHARING)
	z_arm_save_fp_context(&backup_data.fpu_context);
#endif
	fpu_power_down();
#endif
	nvic_save(&backup_data.nvic_context);
#if defined(CONFIG_ARM_MPU)
	z_arm_save_mpu_context(&backup_data.mpu_context);
#endif

#if defined(CONFIG_HAS_PM_S2RAM_CUSTOM_MARKING) && defined(CONFIG_TRUSTED_EXECUTION_SECURE) &&     \
	defined(CONFIG_SOC_MAX32657)
	if (FIELD_GET(MXC_GCR_REVISION_LOW_MASK, MXC_GCR->revision) >=
	    MXC_GCR_REVISION_LOW_EXPECTED) {
		arch_pm_s2ram_suspend(system_off);
	} else {
		/*
		 * Apply the workaround for only the revisions of MAX32657 that has
		 * the issue with warm boot reserved region.
		 * If SP is in or near the warm boot reserved region, pad it below
		 * so the naked function's pushes don't corrupt the reserved data
		 * before it gets saved.
		 */
		uintptr_t sp = (__get_IPSR() != 0U) ? (uintptr_t)__get_MSP()
						    : ((__get_CONTROL() & CONTROL_SPSEL_Msk)
							       ? (uintptr_t)__get_PSP()
							       : (uintptr_t)__get_MSP());
		uintptr_t sp_pad = 0;

		if (sp > (uintptr_t)WARM_BOOT_RESERVED_START &&
		    sp < (uintptr_t)WARM_BOOT_RESERVED_END + 24U) {
			sp_pad = ROUND_UP(sp - (uintptr_t)WARM_BOOT_RESERVED_START, 8U);
		}

		pm_s2ram_suspend_with_sp_pad(system_off, sp_pad);
	}
#else
	arch_pm_s2ram_suspend(system_off);
#endif

	/* Restore MPU, SCB and FPU states */
#if defined(CONFIG_FPU)
	fpu_power_up();
#if !defined(CONFIG_FPU_SHARING)
	z_arm_restore_fp_context(&backup_data.fpu_context);
#endif
#endif

#if defined(CONFIG_ARM_MPU)
	z_arm_restore_mpu_context(&backup_data.mpu_context);
#endif
	nvic_restore(&backup_data.nvic_context);
	z_arm_restore_scb_context(&backup_data.scb_context);

	Wrap_MXC_LP_DisableSramRetention();
}

#if defined(CONFIG_HAS_PM_S2RAM_CUSTOM_MARKING) && defined(CONFIG_TRUSTED_EXECUTION_SECURE)
void __attribute__((naked)) pm_s2ram_mark_set(void)
{
	__asm__ volatile(
		/* Set warm-boot register */
		"str	%[_bypass_val], [%[_byp_reg]]\n"

		"bx	lr\n"
		:
		: [_bypass_val] "r"(MXC_S_MCR_BYPASS0), [_byp_reg] "r"(&MXC_MCR->bypass0)
		: "r1", "r4", "memory");
}

bool __attribute__((naked)) pm_s2ram_mark_check_and_clear(void)
{
	__asm__ volatile(
		/* Set return value to 0 */
		"mov	r0, #0\n"

		/* Check the marker */
		"ldr	r3, [%[_byp_reg]]\n"
		"cmp	r3, %[_bypass_val]\n"
		"bne	exit\n"

		/*
		 * Reset the marker
		 */
		"str	r0, [%[_byp_reg]]\n"

		/*
		 * Set return value to 1
		 */
		"mov	r0, #1\n"

		"exit:\n"
		"bx lr\n"

		:
		: [_bypass_val] "r"(MXC_S_MCR_BYPASS0), [_byp_reg] "r"(&MXC_MCR->bypass0)
		: "r0", "r1", "r3", "r4", "memory");
}
#endif
