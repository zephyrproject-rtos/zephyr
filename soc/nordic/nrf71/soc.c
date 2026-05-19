/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for Nordic Semiconductor nRF71 family processor
 *
 * This module provides routines to initialize and support board-level hardware
 * for the Nordic Semiconductor nRF71 family processor.
 */

#ifdef __NRF_TFM__
#include <zephyr/autoconf.h>
#endif

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>

#ifndef __NRF_TFM__
#include <zephyr/cache.h>
#endif

#if defined(NRF_APPLICATION)
#include <cmsis_core.h>
#include <hal/nrf_glitchdet.h>
#endif

#include <soc.h>
#include <nrfx.h>
#include <lib/nrfx_coredep.h>

#include <hal/nrf_spu.h>
#include <hal/nrf_mpc.h>
#include <hal/nrf_lfxo.h>

LOG_MODULE_REGISTER(soc, CONFIG_SOC_LOG_LEVEL);

#define LFXO_NODE DT_NODELABEL(lfxo)

#if !defined(CONFIG_TRUSTED_EXECUTION_NONSECURE)

struct mpc_region_override {
	nrf_mpc_override_config_t config;
	uintptr_t startaddr;
	uintptr_t endaddr;
	uint32_t perm;
	uint32_t permmask;
};

/*
 * Initialize the override struct with reasonable defaults. This includes:
 *
 * Use a slave number of 0 to avoid redirecting bus transactions from
 * one slave to another.
 *
 * Lock the override to prevent the code that follows from tampering
 * with the configuration.
 *
 * Enable the override so it takes effect.
 *
 * Indicate that secdom is not enabled as this driver is not used on
 * platforms with secdom.
 */
#define MPC_REGION_OVERRIDE_INIT(_startaddr, _endaddr, _secure, _privileged)			\
	{											\
		.config =  {									\
			.slave_number = 0,							\
			.lock = true,								\
			.enable = true,								\
			.secdom_enable = false,							\
			.secure_mask = false,							\
		},										\
		.startaddr = _startaddr,							\
		.endaddr = _endaddr,								\
		.perm = (									\
			(MPC_OVERRIDE_PERM_READ_Allowed << MPC_OVERRIDE_PERM_READ_Pos) |	\
			(MPC_OVERRIDE_PERM_WRITE_Allowed << MPC_OVERRIDE_PERM_WRITE_Pos) |	\
			(MPC_OVERRIDE_PERM_EXECUTE_Allowed << MPC_OVERRIDE_PERM_EXECUTE_Pos) |	\
			(_secure << MPC_OVERRIDE_PERM_SECATTR_Pos) |				\
			(_privileged << MPC_OVERRIDE_PERM_PRIVL_Pos)				\
		),										\
		.permmask = (									\
			MPC_OVERRIDE_PERM_READ_Msk |						\
			MPC_OVERRIDE_PERM_WRITE_Msk |						\
			MPC_OVERRIDE_PERM_EXECUTE_Msk |						\
			MPC_OVERRIDE_PERM_SECATTR_Msk |						\
			MPC_OVERRIDE_PERM_PRIVL_Msk						\
		),										\
	}

static const struct mpc_region_override mpc00_region_overrides[] = {
	/* Make RAM_00/01/02 (AMBIX00 + AMBIX03) accessible to all domains */
	MPC_REGION_OVERRIDE_INIT(0x20000000, 0x200E0000, 0, 0),
	/* Make MRAM accessible to all domains */
	MPC_REGION_OVERRIDE_INIT(0x00000000, 0x01000000, 0, 0),
#if CONFIG_SOC_NRF71_WIFI_DAP
	/* Allow access to Wi-Fi debug interface registers */
	MPC_REGION_OVERRIDE_INIT(0x48000000, 0x48100000, 0, 0),
#endif
};

static const struct mpc_region_override mpc03_region_overrides[] = {
	/* Make RAM_02 (AMBIX03) accessible to the Wi-Fi domain for IPC */
	MPC_REGION_OVERRIDE_INIT(0x200C0000, 0x200E0000, 0, 0),
};

static void set_mpc_region_override(NRF_MPC_Type *mpc,
				    size_t index,
				    const struct mpc_region_override *override)
{
	nrf_mpc_override_startaddr_set(mpc, index, override->startaddr);
	nrf_mpc_override_endaddr_set(mpc, index, override->endaddr);
	nrf_mpc_override_perm_set(mpc, index, override->perm);
	nrf_mpc_override_permmask_set(mpc, index, override->permmask);
	nrf_mpc_override_config_set(mpc, index, &override->config);
}

#if !defined(CONFIG_TRUSTED_EXECUTION_NONSECURE)
static void mpc_configuration(void)
{
	ARRAY_FOR_EACH(mpc00_region_overrides, i) {
		set_mpc_region_override(NRF_MPC00, i, &mpc00_region_overrides[i]);
	}

	ARRAY_FOR_EACH(mpc03_region_overrides, i) {
		set_mpc_region_override(NRF_MPC03, i, &mpc03_region_overrides[i]);
	}
}
#endif

/**
 * Return the SPU instance that can be used to configure the
 * peripheral at the given base address.
 */
static inline NRF_SPU_Type *spu_instance_from_peripheral_addr(uint32_t peripheral_addr)
{
	/* See the SPU chapter in the IPS for how this is calculated */

	uint32_t apb_bus_number = peripheral_addr & 0x00FC0000;

	return (NRF_SPU_Type *)(0x50000000 | apb_bus_number);
}

static void grtc_configuration(void)
{
	/* Split security configuration to let Wi-Fi access GRTC */
	nrf_spu_feature_secattr_set(NRF_SPU20, NRF_SPU_FEATURE_GRTC_CC, 15, 0, 0);
	nrf_spu_feature_secattr_set(NRF_SPU20, NRF_SPU_FEATURE_GRTC_CC, 14, 0, 0);
	nrf_spu_feature_secattr_set(NRF_SPU20, NRF_SPU_FEATURE_GRTC_INTERRUPT, 4, 0, 0);
	nrf_spu_feature_secattr_set(NRF_SPU20, NRF_SPU_FEATURE_GRTC_INTERRUPT, 5, 0, 0);
	nrf_spu_feature_secattr_set(NRF_SPU20, NRF_SPU_FEATURE_GRTC_SYSCOUNTER, 0, 0, 0);
}
#endif /* CONFIG_TRUSTED_EXECUTION_NONSECURE */

#if defined(CONFIG_SOC_NRF71_WIFI_BOOT)
#if !defined(CONFIG_TRUSTED_EXECUTION_NONSECURE) || defined(__NRF_TFM__)
static void wifi_setup(void)
{
	/* Kickstart the LMAC processor */
	NRF_WIFICORE_LRCCONF_LRC0->POWERON =
		(LRCCONF_POWERON_MAIN_AlwaysOn << LRCCONF_POWERON_MAIN_Pos);
	NRF_WIFICORE_LMAC_VPR->INITPC = NRF_WICR->RESERVED[0];
	NRF_WIFICORE_LMAC_VPR->CPURUN = (VPR_CPURUN_EN_Running << VPR_CPURUN_EN_Pos);
#endif
}
#endif

void soc_early_init_hook(void)
{
	/* Update the SystemCoreClock global variable with current core clock
	 * retrieved from hardware state.
	 */
#if !defined(CONFIG_TRUSTED_EXECUTION_NONSECURE) || defined(__NRF_TFM__)
	/* Currently not supported for non-secure */
	SystemCoreClockUpdate();

#if !defined(CONFIG_TRUSTED_EXECUTION_NONSECURE)
	/* Skip for tf-m, configuration exist in target_cfg_71.c */
	mpc_configuration();
	grtc_configuration();
#endif

#if defined(CONFIG_SOC_NRF71_WIFI_BOOT)
	wifi_setup();
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_pwr_antswc)
	*(volatile uint32_t *)PWR_ANTSWC_REG |= PWR_ANTSWC_ENABLE;
#endif

	/* Configure LFXO capacitive load if internal load capacitors are used */
#if DT_ENUM_HAS_VALUE(LFXO_NODE, load_capacitors, internal)
	nrf_lfxo_cload_set(NRF_LFXO,
			(uint8_t)(DT_PROP(LFXO_NODE, load_capacitance_femtofarad) / 1000));
#endif
#endif /* !CONFIG_TRUSTED_EXECUTION_NONSECURE || __NRF_TFM__ */

#ifdef __NRF_TFM__
	/* TF-M enables the instruction cache from target_cfg_71.c, so we
	 * don't need to enable it here.
	 */
#else
	/* Enable ICACHE */
	sys_cache_instr_enable();
#endif
}

void arch_busy_wait(uint32_t time_us)
{
	nrfx_coredep_delay_us(time_us);
}

#ifdef CONFIG_SOC_RESET_HOOK
void soc_reset_hook(void)
{
	SystemInit();
}
#endif
