/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for Nordic Semiconductor nRF53 family processor
 *
 * This module provides routines to initialize and support board-level hardware
 * for the Nordic Semiconductor nRF53 family processor.
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/arch/arm/aarch32/cortex_m/cmsis.h>
#include <soc/nrfx_coredep.h>
#include <zephyr/logging/log.h>
#include <nrf_erratas.h>
#if defined(CONFIG_SOC_NRF5340_CPUAPP)
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>
#include <hal/nrf_cache.h>
#include <hal/nrf_gpio.h>
#include <hal/nrf_oscillators.h>
#include <hal/nrf_regulators.h>
#elif defined(CONFIG_SOC_NRF5340_CPUNET)
#include <hal/nrf_nvmc.h>
#endif
#if defined(CONFIG_PM_S2RAM)
#include <hal/nrf_vmc.h>
#endif
#include <soc_secure.h>

#define PIN_XL1 0
#define PIN_XL2 1

#ifdef CONFIG_RUNTIME_NMI
extern void z_arm_nmi_init(void);
#define NMI_INIT() z_arm_nmi_init()
#else
#define NMI_INIT()
#endif

#if defined(CONFIG_SOC_NRF5340_CPUAPP)
#include <system_nrf5340_application.h>
#elif defined(CONFIG_SOC_NRF5340_CPUNET)
#include <system_nrf5340_network.h>
#else
#error "Unknown nRF53 SoC."
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(nordic_nrf_gpio_forwarder) && \
	defined(CONFIG_BOARD_ENABLE_CPUNET) && \
	(!defined(CONFIG_TRUSTED_EXECUTION_NONSECURE) || defined(CONFIG_BUILD_WITH_TFM))
#define NRF_GPIO_FORWARDER_FOR_NRF5340_CPUAPP_ENABLED
#endif

#if defined(NRF_GPIO_FORWARDER_FOR_NRF5340_CPUAPP_ENABLED)
#define GPIOS_PSEL_BY_IDX(node_id, prop, idx) \
	NRF_DT_GPIOS_TO_PSEL_BY_IDX(node_id, prop, idx),
#define ALL_GPIOS_IN_NODE(node_id) \
	DT_FOREACH_PROP_ELEM(node_id, gpios, GPIOS_PSEL_BY_IDX)
#define ALL_GPIOS_IN_FORWARDER(node_id) \
	DT_FOREACH_CHILD(node_id, ALL_GPIOS_IN_NODE)
#endif

#define LOG_LEVEL CONFIG_SOC_LOG_LEVEL
LOG_MODULE_REGISTER(soc);

#if defined(CONFIG_PM_S2RAM)

#if defined(CONFIG_SOC_NRF5340_CPUAPP)
#define RAM_N_BLOCK	(8)
#elif defined(CONFIG_SOC_NRF5340_CPUNET)
#define RAM_N_BLOCK	(4)
#endif /* CONFIG_SOC_NRF5340_CPUAPP || CONFIG_SOC_NRF5340_CPUNET */

#define MASK_ALL_SECT	(VMC_RAM_POWER_S0RETENTION_Msk  | VMC_RAM_POWER_S1RETENTION_Msk  | \
			 VMC_RAM_POWER_S2RETENTION_Msk  | VMC_RAM_POWER_S3RETENTION_Msk  | \
			 VMC_RAM_POWER_S4RETENTION_Msk  | VMC_RAM_POWER_S5RETENTION_Msk  | \
			 VMC_RAM_POWER_S6RETENTION_Msk  | VMC_RAM_POWER_S7RETENTION_Msk  | \
			 VMC_RAM_POWER_S8RETENTION_Msk  | VMC_RAM_POWER_S9RETENTION_Msk  | \
			 VMC_RAM_POWER_S10RETENTION_Msk | VMC_RAM_POWER_S11RETENTION_Msk | \
			 VMC_RAM_POWER_S12RETENTION_Msk | VMC_RAM_POWER_S13RETENTION_Msk | \
			 VMC_RAM_POWER_S14RETENTION_Msk | VMC_RAM_POWER_S15RETENTION_Msk)

static void enable_ram_retention(void)
{
	/*
	 * Enable RAM retention for *ALL* the SRAM
	 */
	for (size_t n = 0; n < RAM_N_BLOCK; n++) {
		nrf_vmc_ram_block_retention_set(NRF_VMC, n, MASK_ALL_SECT);
	}

}
#endif /* CONFIG_PM_S2RAM */

static int nordicsemi_nrf53_init(const struct device *arg)
{
	uint32_t key;

	ARG_UNUSED(arg);

	key = irq_lock();

#if defined(CONFIG_SOC_NRF5340_CPUAPP) && defined(CONFIG_NRF_ENABLE_CACHE)
#if !defined(CONFIG_BUILD_WITH_TFM)
	/* Enable the instruction & data cache.
	 * This can only be done from secure code.
	 * This is handled by the TF-M platform so we skip it when TF-M is
	 * enabled.
	 */
	nrf_cache_enable(NRF_CACHE);
#endif
#elif defined(CONFIG_SOC_NRF5340_CPUNET) && defined(CONFIG_NRF_ENABLE_CACHE)
	nrf_nvmc_icache_config_set(NRF_NVMC, NRF_NVMC_ICACHE_ENABLE);
#endif

#if defined(CONFIG_SOC_ENABLE_LFXO)
	nrf_oscillators_lfxo_cap_set(NRF_OSCILLATORS,
		IS_ENABLED(CONFIG_SOC_LFXO_CAP_INT_6PF) ?
			NRF_OSCILLATORS_LFXO_CAP_6PF :
		IS_ENABLED(CONFIG_SOC_LFXO_CAP_INT_7PF) ?
			NRF_OSCILLATORS_LFXO_CAP_7PF :
		IS_ENABLED(CONFIG_SOC_LFXO_CAP_INT_9PF) ?
			NRF_OSCILLATORS_LFXO_CAP_9PF :
			NRF_OSCILLATORS_LFXO_CAP_EXTERNAL);
#if !defined(CONFIG_BUILD_WITH_TFM)
	/* This can only be done from secure code.
	 * This is handled by the TF-M platform so we skip it when TF-M is
	 * enabled.
	 */
	nrf_gpio_pin_mcu_select(PIN_XL1, NRF_GPIO_PIN_MCUSEL_PERIPHERAL);
	nrf_gpio_pin_mcu_select(PIN_XL2, NRF_GPIO_PIN_MCUSEL_PERIPHERAL);
#endif /* !defined(CONFIG_BUILD_WITH_TFM) */
#endif /* defined(CONFIG_SOC_ENABLE_LFXO) */
#if defined(CONFIG_SOC_HFXO_CAP_INTERNAL)
	/* This register is only accessible from secure code. */
	uint32_t xosc32mtrim = soc_secure_read_xosc32mtrim();
	/* As specified in the nRF5340 PS:
	 * CAPVALUE = (((FICR->XOSC32MTRIM.SLOPE+56)*(CAPACITANCE*2-14))
	 *            +((FICR->XOSC32MTRIM.OFFSET-8)<<4)+32)>>6;
	 * where CAPACITANCE is the desired capacitor value in pF, holding any
	 * value between 7.0 pF and 20.0 pF in 0.5 pF steps.
	 */
	uint32_t slope = (xosc32mtrim & FICR_XOSC32MTRIM_SLOPE_Msk)
			 >> FICR_XOSC32MTRIM_SLOPE_Pos;
	uint32_t offset = (xosc32mtrim & FICR_XOSC32MTRIM_OFFSET_Msk)
			  >> FICR_XOSC32MTRIM_OFFSET_Pos;
	uint32_t capvalue =
		((slope + 56) * (CONFIG_SOC_HFXO_CAP_INT_VALUE_X2 - 14)
		 + ((offset - 8) << 4) + 32) >> 6;

	nrf_oscillators_hfxo_cap_set(NRF_OSCILLATORS, true, capvalue);
#elif defined(CONFIG_SOC_HFXO_CAP_EXTERNAL)
	nrf_oscillators_hfxo_cap_set(NRF_OSCILLATORS, false, 0);
#endif

#if defined(CONFIG_SOC_DCDC_NRF53X_APP)
	nrf_regulators_dcdcen_set(NRF_REGULATORS, true);
#endif
#if defined(CONFIG_SOC_DCDC_NRF53X_NET)
	nrf_regulators_dcdcen_radio_set(NRF_REGULATORS, true);
#endif
#if defined(CONFIG_SOC_DCDC_NRF53X_HV)
	nrf_regulators_dcdcen_vddh_set(NRF_REGULATORS, true);
#endif

#if defined(NRF_GPIO_FORWARDER_FOR_NRF5340_CPUAPP_ENABLED)
	static const uint8_t forwarded_psels[] = {
		DT_FOREACH_STATUS_OKAY(nordic_nrf_gpio_forwarder, ALL_GPIOS_IN_FORWARDER)
	};

	for (int i = 0; i < ARRAY_SIZE(forwarded_psels); i++) {
		soc_secure_gpio_pin_mcu_select(forwarded_psels[i], NRF_GPIO_PIN_MCUSEL_NETWORK);
	}

#endif

#if defined(CONFIG_PM_S2RAM)
	enable_ram_retention();
#endif /* CONFIG_PM_S2RAM */

	/* Install default handler that simply resets the CPU
	 * if configured in the kernel, NOP otherwise
	 */
	NMI_INIT();

	irq_unlock(key);

	return 0;
}

void arch_busy_wait(uint32_t time_us)
{
	nrfx_coredep_delay_us(time_us);
}

SYS_INIT(nordicsemi_nrf53_init, PRE_KERNEL_1, 0);
