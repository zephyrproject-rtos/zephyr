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

#ifdef CONFIG_LOG_FRONTEND_STMESP
#include <zephyr/logging/log_frontend_stmesp.h>
#endif

#include <hal/nrf_hsfll.h>
#include <hal/nrf_lrcconf.h>
#include <hal/nrf_spu.h>
#include <hal/nrf_memconf.h>
#include <hal/nrf_nfct.h>
#include <soc/nrfx_coredep.h>
#include <soc_lrcconf.h>
#include <dmm.h>

#if defined(CONFIG_SOC_NRF54H20_CPURAD_ENABLE)
#include <nrf_ironside/cpuconf.h>
#endif

LOG_MODULE_REGISTER(soc, CONFIG_SOC_LOG_LEVEL);

#if defined(NRF_APPLICATION)
#define HSFLL_NODE DT_NODELABEL(cpuapp_hsfll)
#elif defined(NRF_RADIOCORE)
#define HSFLL_NODE DT_NODELABEL(cpurad_hsfll)
#endif

#define FIXED_PARTITION_ADDRESS(label)                                                             \
	(DT_REG_ADDR(DT_NODELABEL(label)) +                                                        \
	 DT_REG_ADDR(COND_CODE_1(DT_FIXED_SUBPARTITION_EXISTS(DT_NODELABEL(label)),                \
			(DT_GPARENT(DT_PARENT(DT_NODELABEL(label)))),                              \
			(DT_GPARENT(DT_NODELABEL(label))))))

#ifdef CONFIG_USE_DT_CODE_PARTITION
#define FLASH_LOAD_OFFSET DT_REG_ADDR(DT_CHOSEN(zephyr_code_partition))
#elif defined(CONFIG_FLASH_LOAD_OFFSET)
#define FLASH_LOAD_OFFSET CONFIG_FLASH_LOAD_OFFSET
#endif

#define PARTITION_IS_RUNNING_APP_PARTITION(label)                                                  \
	(DT_REG_ADDR(DT_NODELABEL(label)) <= FLASH_LOAD_OFFSET &&                                  \
	 DT_REG_ADDR(DT_NODELABEL(label)) + DT_REG_SIZE(DT_NODELABEL(label)) > FLASH_LOAD_OFFSET)

sys_snode_t soc_node;

#define FICR_ADDR_GET(node_id, name)                                           \
	DT_REG_ADDR(DT_PHANDLE_BY_NAME(node_id, nordic_ficrs, name)) +         \
		DT_PHA_BY_NAME(node_id, nordic_ficrs, name, offset)

#define SPU_INSTANCE_GET(p_addr)                                               \
	((NRF_SPU_Type *)((p_addr) & (ADDRESS_REGION_Msk |                     \
				      ADDRESS_SECURITY_Msk |                   \
				      ADDRESS_DOMAIN_Msk |                     \
				      ADDRESS_BUS_Msk)))

void nrf_soc_memconf_retain_set(bool enable)
{
	uint32_t ret_mask = BIT(RAMBLOCK_RET_BIT_ICACHE) | BIT(RAMBLOCK_RET_BIT_DCACHE);

	nrf_memconf_ramblock_ret_mask_enable_set(NRF_MEMCONF, 0, ret_mask, enable);
	nrf_memconf_ramblock_ret_mask_enable_set(NRF_MEMCONF, 1, ret_mask, enable);

#if defined(RAMBLOCK_RET2_MASK)
	ret_mask = 0;
#if defined(RAMBLOCK_RET2_BIT_ICACHE)
	ret_mask |= BIT(RAMBLOCK_RET2_BIT_ICACHE);
#endif
#if defined(RAMBLOCK_RET2_BIT_DCACHE)
	ret_mask |= BIT(RAMBLOCK_RET2_BIT_DCACHE);
#endif
	nrf_memconf_ramblock_ret2_mask_enable_set(NRF_MEMCONF, 0, ret_mask, enable);
	nrf_memconf_ramblock_ret2_mask_enable_set(NRF_MEMCONF, 1, ret_mask, enable);
#endif /* defined(RAMBLOCK_RET2_MASK) */
}

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

	soc_lrcconf_poweron_request(&soc_node, NRF_LRCCONF_POWER_DOMAIN_0);
	nrf_lrcconf_poweron_force_set(NRF_LRCCONF010, NRF_LRCCONF_POWER_MAIN, false);
	nrf_soc_memconf_retain_set(false);
	nrf_memconf_ramblock_ret_mask_enable_set(NRF_MEMCONF, 0, RAMBLOCK_RET_MASK, true);
	nrf_memconf_ramblock_ret_mask_enable_set(NRF_MEMCONF, 1, RAMBLOCK_RET_MASK, true);
#if defined(RAMBLOCK_RET2_MASK)
	nrf_memconf_ramblock_ret2_mask_enable_set(NRF_MEMCONF, 0, RAMBLOCK_RET2_MASK, true);
	nrf_memconf_ramblock_ret2_mask_enable_set(NRF_MEMCONF, 1, RAMBLOCK_RET2_MASK, true);
#endif
}

static int trim_hsfll(void)
{
#if defined(HSFLL_NODE)

	NRF_HSFLL_Type *hsfll = (NRF_HSFLL_Type *)DT_REG_ADDR(HSFLL_NODE);
	nrf_hsfll_trim_t trim = {
		.vsup = sys_read32(FICR_ADDR_GET(HSFLL_NODE, vsup)),
		.coarse = sys_read32(FICR_ADDR_GET(HSFLL_NODE, coarse)),
		.fine = sys_read32(FICR_ADDR_GET(HSFLL_NODE, fine))
	};

	LOG_DBG("Trim: HSFLL VSUP: 0x%.8x", trim.vsup);
	LOG_DBG("Trim: HSFLL COARSE: 0x%.8x", trim.coarse);
	LOG_DBG("Trim: HSFLL FINE: 0x%.8x", trim.fine);

	nrf_hsfll_clkctrl_mult_set(hsfll,
				   DT_PROP(HSFLL_NODE, clock_frequency) /
					   DT_PROP(DT_CLOCKS_CTLR(HSFLL_NODE), clock_frequency));
	nrf_hsfll_trim_set(hsfll, &trim);

	nrf_hsfll_task_trigger(hsfll, NRF_HSFLL_TASK_FREQ_CHANGE);
	/* HSFLL task frequency change needs to be triggered twice to take effect.*/
	nrf_hsfll_task_trigger(hsfll, NRF_HSFLL_TASK_FREQ_CHANGE);

	LOG_DBG("NRF_HSFLL->TRIM.VSUP = %d", hsfll->TRIM.VSUP);
	LOG_DBG("NRF_HSFLL->TRIM.COARSE = %d", hsfll->TRIM.COARSE);
	LOG_DBG("NRF_HSFLL->TRIM.FINE = %d", hsfll->TRIM.FINE);

#endif /* defined(HSFLL_NODE) */

	return 0;
}

#if defined(CONFIG_ARM_ON_ENTER_CPU_IDLE_HOOK)
bool z_arm_on_enter_cpu_idle(void)
{
#ifdef CONFIG_LOG_FRONTEND_STMESP
	log_frontend_stmesp_pre_sleep();
#endif
	return true;
}
#endif

void soc_early_init_hook(void)
{
	int err;

	sys_cache_instr_enable();
	sys_cache_data_enable();

	power_domain_init();

	trim_hsfll();

	err = dmm_init();
	__ASSERT_NO_MSG(err == 0);

#if DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(ccm030))
	/* DMASEC is set to non-secure by default, which prevents CCM from
	 * accessing secure memory. Change DMASEC to secure.
	 */
	uint32_t ccm030_addr = DT_REG_ADDR(DT_NODELABEL(ccm030));
	NRF_SPU_Type *spu = SPU_INSTANCE_GET(ccm030_addr);

	nrf_spu_periph_perm_dmasec_set(spu, nrf_address_slave_get(ccm030_addr), true);
#endif

	if (((IS_ENABLED(CONFIG_SOC_NRF54H20_CPUAPP) &&
	      DT_NODE_HAS_STATUS(DT_NODELABEL(nfct), disabled)) ||
	     DT_NODE_HAS_STATUS(DT_NODELABEL(nfct), reserved)) &&
	    DT_PROP_OR(DT_NODELABEL(nfct), nfct_pins_as_gpios, 0)) {
		nrf_nfct_pad_config_enable_set(NRF_NFCT, false);
	}
}

#if defined(CONFIG_SOC_LATE_INIT_HOOK)

void soc_late_init_hook(void)
{
#if defined(CONFIG_SOC_NRF54H20_CPURAD_ENABLE)
	int err_cpuconf;

	/* The msg will be used for communication prior to IPC
	 * communication being set up. But at this moment no such
	 * communication is required.
	 */
	uint8_t *msg = NULL;
	size_t msg_size = 0;
	void *radiocore_address = NULL;

#if DT_NODE_EXISTS(DT_NODELABEL(cpurad_slot1_partition))
	if (PARTITION_IS_RUNNING_APP_PARTITION(cpuapp_slot1_partition)) {
		radiocore_address = (void *)(FIXED_PARTITION_ADDRESS(cpurad_slot1_partition) +
					     CONFIG_ROM_START_OFFSET);
	} else {
		radiocore_address = (void *)(FIXED_PARTITION_ADDRESS(cpurad_slot0_partition) +
					     CONFIG_ROM_START_OFFSET);
	}
#else
	radiocore_address =
		(void *)(FIXED_PARTITION_ADDRESS(cpurad_slot0_partition) + CONFIG_ROM_START_OFFSET);
#endif

	if (IS_ENABLED(CONFIG_SOC_NRF54H20_CPURAD_ENABLE_CHECK_VTOR) &&
	    sys_read32((mem_addr_t)radiocore_address) == 0xFFFFFFFFUL) {
		LOG_ERR("Radiocore is not programmed, it will not be started");

		return;
	}

	bool cpu_wait = IS_ENABLED(CONFIG_SOC_NRF54H20_CPURAD_ENABLE_DEBUG_WAIT);

	err_cpuconf = ironside_cpuconf(NRF_PROCESSOR_RADIOCORE, radiocore_address, cpu_wait, msg,
				       msg_size);
	__ASSERT(err_cpuconf == 0, "err_cpuconf was %d", err_cpuconf);
#endif /* CONFIG_SOC_NRF54H20_CPURAD_ENABLE */
}
#endif

void arch_busy_wait(uint32_t time_us)
{
	nrfx_coredep_delay_us(time_us);
}
