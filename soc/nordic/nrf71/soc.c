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

#include <nrfx.h>
#include <lib/nrfx_coredep.h>

#include <hal/nrf_spu.h>
#include <hal/nrf_mpc.h>
#include <hal/nrf_lfxo.h>

LOG_MODULE_REGISTER(soc, CONFIG_SOC_LOG_LEVEL);

#define LFXO_NODE DT_NODELABEL(lfxo)

#if !defined(CONFIG_TRUSTED_EXECUTION_NONSECURE)
/* Copied from TF-M native driver */
struct mpc_region_override {
	nrf_mpc_override_config_t config;
	nrf_owner_t owner_id;
	uintptr_t start_address;
	uintptr_t endaddr;
	uint32_t perm;
	uint32_t permmask;
	size_t index;
};

static void mpc_configure_override(NRF_MPC_Type *mpc, struct mpc_region_override *override)
{
	nrf_mpc_override_startaddr_set(mpc, override->index, override->start_address);
	nrf_mpc_override_endaddr_set(mpc, override->index, override->endaddr);
	nrf_mpc_override_perm_set(mpc, override->index, override->perm);
	nrf_mpc_override_permmask_set(mpc, override->index, override->permmask);
#if defined(NRF_MPC_HAS_OVERRIDE_OWNERID) && NRF_MPC_HAS_OVERRIDE_OWNERID
	nrf_mpc_override_ownerid_set(mpc, override->index, override->owner_id);
#endif
	nrf_mpc_override_config_set(mpc, override->index, &override->config);
}

/*
 * Configure the override struct with reasonable defaults. This includes:
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
static void init_mpc_region_override(struct mpc_region_override *override)
{
	*override = (struct mpc_region_override){
		.config =
			(nrf_mpc_override_config_t){
				.slave_number = 0,
				.lock = true,
				.enable = true,
				.secdom_enable = false,
				.secure_mask = false,
			},
		/* 0-NS R,W,X =1 */
		.perm = 0x7,
		.permmask = 0xF,
		.owner_id = 0,
	};
}

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

/* End of TF-M native driver */

static void wifi_mpc_configuration(void)
{
	struct mpc_region_override override;
	uint32_t index = 0;

	/* Make RAM_00/01/02 (AMBIX00 + AMBIX03) accessible to the Wi-Fi domain*/
	init_mpc_region_override(&override);
	override.start_address = 0x20000000;
	override.endaddr = 0x200E0000;
	override.index = index++;
	mpc_configure_override(NRF_MPC00, &override);

	/* MRAM MPC overrides for wifi */
	init_mpc_region_override(&override);
	override.start_address = 0x00000000;
	override.endaddr = 0x01000000;
	override.index = index++;
	mpc_configure_override(NRF_MPC00, &override);

	/* Make RAM_02 (AMBIX03) accessible to the Wi-Fi domain for IPC */
	init_mpc_region_override(&override);
	override.start_address = 0x200C0000;
	override.endaddr = 0x200E0000;
	override.index = 0;
	mpc_configure_override(NRF_MPC03, &override);
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

#if !defined(CONFIG_TRUSTED_EXECUTION_NONSECURE) || defined(__NRF_TFM__)
static void wifi_setup(void)
{
#if !defined(CONFIG_TRUSTED_EXECUTION_NONSECURE)
	/* Skip for tf-m, configuration exist in target_cfg_71.c */
	wifi_mpc_configuration();

	grtc_configuration();

#endif

	/* Kickstart the LMAC processor */
	NRF_WIFICORE_LRCCONF_LRC0->POWERON =
		(LRCCONF_POWERON_MAIN_AlwaysOn << LRCCONF_POWERON_MAIN_Pos);
	NRF_WIFICORE_LMAC_VPR->INITPC = NRF_WICR->RESERVED[0];
	NRF_WIFICORE_LMAC_VPR->CPURUN = (VPR_CPURUN_EN_Running << VPR_CPURUN_EN_Pos);
}
#endif

void soc_early_init_hook(void)
{
#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_pwr_antswc)
	*(volatile uint32_t *)PWR_ANTSWC_REG |= PWR_ANTSWC_ENABLE;
#endif
	/* Update the SystemCoreClock global variable with current core clock
	 * retrieved from hardware state.
	 */
#if !defined(CONFIG_TRUSTED_EXECUTION_NONSECURE) || defined(__NRF_TFM__)
	/* Currently not supported for non-secure */
	SystemCoreClockUpdate();
	wifi_setup();

	/* Configure LFXO capacitive load if internal load capacitors are used */
#if DT_ENUM_HAS_VALUE(LFXO_NODE, load_capacitors, internal)
	nrf_lfxo_cload_set(NRF_LFXO,
			(uint8_t)(DT_PROP(LFXO_NODE, load_capacitance_femtofarad) / 1000));
#endif
#endif

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
