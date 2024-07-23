/*
 * Copyright (c) 2024 Nordic Semiconductor ASA.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/cache.h>
#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <hal/nrf_lrcconf.h>
#include <hal/nrf_spu.h>
#include <hal/nrf_memconf.h>
#include <soc/nrfx_coredep.h>

LOG_MODULE_REGISTER(soc, CONFIG_SOC_LOG_LEVEL);

#define SPU_INSTANCE_GET(p_addr)                                               \
	((NRF_SPU_Type *)((p_addr) & (ADDRESS_REGION_Msk |                     \
				      ADDRESS_SECURITY_Msk |                   \
				      ADDRESS_DOMAIN_Msk |                     \
				      ADDRESS_BUS_Msk)))

static void power_domain_init(void)
{
	/*
	 * Set:
	 *  - LRCCONF010.POWERON.MAIN: 1
	 *  - LRCCONF010.POWERON.ACT: 1
	 *  - LRCCONF010.RETAIN.MAIN: 1
	 *  - LRCCONF010.RETAIN.ACT: 1
	 *
	 *  This is done here at boot so that when the idle routine will hit
	 *  WFI the power domain will be correctly retained.
	 */

	nrf_lrcconf_poweron_force_set(NRF_LRCCONF010, NRF_LRCCONF_POWER_DOMAIN_0,
			!IS_ENABLED(CONFIG_SOC_NRF54H20_CPURAD));
	nrf_lrcconf_poweron_force_set(NRF_LRCCONF010, NRF_LRCCONF_POWER_MAIN,
			!IS_ENABLED(CONFIG_SOC_NRF54H20_CPURAD));

	nrf_lrcconf_retain_set(NRF_LRCCONF010, NRF_LRCCONF_POWER_MAIN, true);
	nrf_lrcconf_retain_set(NRF_LRCCONF010, NRF_LRCCONF_POWER_DOMAIN_0, true);
	nrf_memconf_ramblock_ret_enable_set(NRF_MEMCONF, 0, RAMBLOCK_RET_BIT_ICACHE, false);
	nrf_memconf_ramblock_ret_enable_set(NRF_MEMCONF, 0, RAMBLOCK_RET_BIT_DCACHE, false);
	nrf_memconf_ramblock_ret_enable_set(NRF_MEMCONF, 1, RAMBLOCK_RET_BIT_ICACHE, false);
	nrf_memconf_ramblock_ret_enable_set(NRF_MEMCONF, 1, RAMBLOCK_RET_BIT_DCACHE, false);
#if defined(RAMBLOCK_RET2_BIT_ICACHE)
	nrf_memconf_ramblock_ret2_enable_set(NRF_MEMCONF, 0, RAMBLOCK_RET2_BIT_ICACHE, false);
	nrf_memconf_ramblock_ret2_enable_set(NRF_MEMCONF, 1, RAMBLOCK_RET2_BIT_ICACHE, false);
#endif
#if defined(RAMBLOCK_RET2_BIT_DCACHE)
	nrf_memconf_ramblock_ret2_enable_set(NRF_MEMCONF, 0, RAMBLOCK_RET2_BIT_DCACHE, false);
	nrf_memconf_ramblock_ret2_enable_set(NRF_MEMCONF, 1, RAMBLOCK_RET2_BIT_DCACHE, false);
#endif
	nrf_memconf_ramblock_ret_mask_enable_set(NRF_MEMCONF, 0, RAMBLOCK_RET_MASK, true);
	nrf_memconf_ramblock_ret_mask_enable_set(NRF_MEMCONF, 1, RAMBLOCK_RET_MASK, true);
#if defined(RAMBLOCK_RET2_MASK)
	/*
	 * TODO: Use nrf_memconf_ramblock_ret2_mask_enable_set() function
	 * when will be provided by HAL.
	 */
	NRF_MEMCONF->POWER[0].RET2 = RAMBLOCK_RET2_MASK;
	NRF_MEMCONF->POWER[1].RET2 = RAMBLOCK_RET2_MASK;
#endif
}

static int nordicsemi_nrf54h_init(void)
{
	sys_cache_instr_enable();
	sys_cache_data_enable();

	power_domain_init();

#if DT_NODE_HAS_STATUS(DT_NODELABEL(ccm030), okay)
	/* DMASEC is set to non-secure by default, which prevents CCM from
	 * accessing secure memory. Change DMASEC to secure.
	 */
	uint32_t ccm030_addr = DT_REG_ADDR(DT_NODELABEL(ccm030));
	NRF_SPU_Type *spu = SPU_INSTANCE_GET(ccm030_addr);

	nrf_spu_periph_perm_dmasec_set(spu, nrf_address_slave_get(ccm030_addr), true);
#endif

	return 0;
}

void arch_busy_wait(uint32_t time_us)
{
	nrfx_coredep_delay_us(time_us);
}

SYS_INIT(nordicsemi_nrf54h_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
