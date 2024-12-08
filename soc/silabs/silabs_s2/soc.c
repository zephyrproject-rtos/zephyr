/*
 * Copyright (c) 2024 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief SoC initialization for Silicon Labs Series 2 products
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <em_chip.h>
#include <sl_device_init_dcdc.h>
#include <sl_clock_manager_init.h>
#include <sl_hfxo_manager.h>
#include <sl_power_manager.h>

#if defined(CONFIG_PRINTK) || defined(CONFIG_LOG)
#define PR_EXC(...) LOG_ERR(__VA_ARGS__)
#else
#define PR_EXC(...)
#endif /* CONFIG_PRINTK || CONFIG_LOG */

#if (CONFIG_FAULT_DUMP == 2)
#define PR_FAULT_INFO(...) PR_EXC(__VA_ARGS__)
#else
#define PR_FAULT_INFO(...)
#endif

LOG_MODULE_REGISTER(soc, CONFIG_SOC_LOG_LEVEL);

#if defined(CONFIG_SOC_SILABS_HFXO_MANAGER)
Z_ISR_DECLARE_DIRECT(DT_IRQ(DT_NODELABEL(hfxo), irq), 0, sl_hfxo_manager_irq_handler);
#endif

void soc_early_init_hook(void)
{
	/* Handle chip errata */
	CHIP_Init();

	if (DT_HAS_COMPAT_STATUS_OKAY(silabs_series2_dcdc)) {
		sl_device_init_dcdc();
	}
	sl_clock_manager_init();

	if (IS_ENABLED(CONFIG_SOC_SILABS_HFXO_MANAGER)) {
		sl_hfxo_manager_init_hardware();
		sl_hfxo_manager_init();
	}
	if (IS_ENABLED(CONFIG_PM)) {
		sl_power_manager_init();
	}
}

#if defined(CONFIG_ARM_SECURE_FIRMWARE) && !defined(CONFIG_ARM_FIRMWARE_HAS_SECURE_ENTRY_FUNCS)
static void smu_fault(void)
{
	PR_FAULT_INFO("***** SMU FAULT *****");

	if (SMU->IF & SMU_IF_BMPUSEC) {
		PR_FAULT_INFO("Bus Manager Fault");
		PR_EXC("SMU.BMPUFS=%d", SMU->BMPUFS);
	}
	if (SMU->IF & SMU_IF_PPUSEC) {
		PR_FAULT_INFO("Peripheral Access Fault");
		PR_EXC("SMU.PPUFS=%d", SMU->PPUFS);
	}

	z_fatal_error(K_ERR_CPU_EXCEPTION, NULL);
}
#endif

void soc_prep_hook(void)
{
	/* Initialize TrustZone state of the device.
	 * If this is a secure app with no non-secure callable functions, it is a secure-only app.
	 * Configure all peripherals except the SMU and SEMAILBOX to non-secure aliases, and make
	 * all bus transactions from the CPU have non-secure attribution.
	 * This makes the secure-only app behave more like a non-secure app, allowing the use of
	 * libraries that only expect to use non-secure peripherals, such as the radio subsystem.
	 */
#if defined(CONFIG_ARM_SECURE_FIRMWARE) && !defined(CONFIG_ARM_FIRMWARE_HAS_SECURE_ENTRY_FUNCS)
#if defined(CMU_CLKEN1_SMU)
	CMU_S->CLKEN1_SET = CMU_CLKEN1_SMU;
#endif
	SMU->PPUSATD0_CLR = _SMU_PPUSATD0_MASK;
#if defined(SEMAILBOX_PRESENT)
	SMU->PPUSATD1_CLR = (_SMU_PPUSATD1_MASK & (~SMU_PPUSATD1_SMU & ~SMU_PPUSATD1_SEMAILBOX));
#else
	SMU->PPUSATD1_CLR = (_SMU_PPUSATD1_MASK & ~SMU_PPUSATD1_SMU);
#endif

	SAU->CTRL = SAU_CTRL_ALLNS_Msk;
	__DSB();
	__ISB();

	NVIC_ClearPendingIRQ(SMU_SECURE_IRQn);
	SMU->IF_CLR = SMU_IF_PPUSEC | SMU_IF_BMPUSEC;
	SMU->IEN = SMU_IEN_PPUSEC | SMU_IEN_BMPUSEC;

	IRQ_DIRECT_CONNECT(SMU_SECURE_IRQn, 0, smu_fault, 0);
	irq_enable(SMU_SECURE_IRQn);
#endif
}
