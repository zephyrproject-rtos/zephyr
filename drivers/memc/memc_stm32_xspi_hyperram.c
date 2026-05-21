/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_xspi_hyperram

#include <errno.h>
#include <soc.h>
#include <stm32_bitops.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#include <zephyr/multi_heap/shared_multi_heap.h>

LOG_MODULE_REGISTER(memc_stm32_xspi_hyperram, CONFIG_MEMC_LOG_LEVEL);

#define STM32_XSPI_NODE DT_INST_PARENT(0)

#ifdef CONFIG_SHARED_MULTI_HEAP
struct shared_multi_heap_region smh_hyperram = {
	.addr = DT_REG_ADDR(DT_NODELABEL(hyperram)),
	.size = DT_REG_SIZE(DT_NODELABEL(hyperram)),
	.attr = SMH_REG_ATTR_EXTERNAL,
};
#endif /* CONFIG_SHARED_MULTI_HEAP */

#define STM32_XSPI_CLOCK_PRESCALER_MIN  0U
#define STM32_XSPI_CLOCK_PRESCALER_MAX  255U
#define STM32_XSPI_CLOCK_COMPUTE(bus_freq, prescaler) ((bus_freq) / ((prescaler) + 1U))

#if defined(XSPI1)
#define STM32_XSPI1 XSPI1
#elif defined(OCTOSPI1)
#define STM32_XSPI1 OCTOSPI1
#endif /* XSPI1 */

#if defined(XSPI2)
#define STM32_XSPI2 XSPI2
#elif defined(OCTOSPI2)
#define STM32_XSPI2 OCTOSPI2
#endif /* XSPI2 */

struct memc_stm32_xspi_hyperram_config {
	const struct pinctrl_dev_config *pcfg;
	const struct stm32_pclken pclken;
	const struct stm32_pclken pclken_ker;
	const struct stm32_pclken pclken_mgr;
	size_t memory_size;
	uint32_t max_frequency;
	uint32_t rw_recovery_time;
	uint32_t access_time;
	bool fixed_latency;
	bool write_zero_latency;
};

struct memc_stm32_xspi_hyperram_data {
	XSPI_HandleTypeDef hxspi;
};

static int memc_stm32_xspi_hyperram_init(const struct device *dev)
{
	const struct memc_stm32_xspi_hyperram_config *dev_cfg = dev->config;
	struct memc_stm32_xspi_hyperram_data *dev_data = dev->data;
	XSPI_HandleTypeDef *hxspi = &dev_data->hxspi;
	uint32_t ahb_clock_freq;
	XSPI_HyperbusCfgTypeDef hcfg = {0};
	XSPI_MemoryMappedTypeDef mm_cfg = {0};
	uint32_t prescaler = STM32_XSPI_CLOCK_PRESCALER_MIN;
	int ret;

	/* Signals configuration */
	ret = pinctrl_apply_state(dev_cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("XSPI pinctrl setup failed (%d)", ret);
		return ret;
	}

	/* Clock configuration */
	if (clock_control_on(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
			     (clock_control_subsys_t)&dev_cfg->pclken) != 0) {
		LOG_ERR("Could not enable XSPI clock");
		return -EIO;
	}
	if (clock_control_get_rate(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
				   (clock_control_subsys_t)&dev_cfg->pclken,
				   &ahb_clock_freq) < 0) {
		LOG_ERR("Failed call clock_control_get_rate(pclken)");
		return -EIO;
	}

#if DT_CLOCKS_HAS_NAME(STM32_XSPI_NODE, xspi_ker)
	/* Kernel clock config for peripheral if any */
	if (clock_control_configure(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
				    (clock_control_subsys_t)&dev_cfg->pclken_ker,
				    NULL) != 0) {
		LOG_ERR("Could not select XSPI domain clock");
		return -EIO;
	}
	if (clock_control_get_rate(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
				   (clock_control_subsys_t)&dev_cfg->pclken_ker,
				   &ahb_clock_freq) < 0) {
		LOG_ERR("Failed call clock_control_get_rate(pclken_ker)");
		return -EIO;
	}
#endif /* DT_CLOCKS_HAS_NAME(STM32_XSPI_NODE, xspi_ker) */

#if DT_CLOCKS_HAS_NAME(STM32_XSPI_NODE, xspi_mgr)
	/* Clock domain corresponding to the IO-Mgr (XSPIM) */
	if (clock_control_on(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
			     (clock_control_subsys_t)&dev_cfg->pclken_mgr) != 0) {
		LOG_ERR("Could not enable XSPI Manager clock");
		return -EIO;
	}
#endif /* DT_CLOCKS_HAS_NAME(STM32_XSPI_NODE, xspi_mgr) */

	/* Compute prescaler to stay within max_frequency */
	for (; prescaler <= STM32_XSPI_CLOCK_PRESCALER_MAX; prescaler++) {
		uint32_t clk = STM32_XSPI_CLOCK_COMPUTE(ahb_clock_freq, prescaler);

		if (clk <= dev_cfg->max_frequency) {
			break;
		}
	}

	if (prescaler > STM32_XSPI_CLOCK_PRESCALER_MAX) {
		LOG_ERR("XSPI could not find valid prescaler value");
		return -EINVAL;
	}

	hxspi->Init.ClockPrescaler = prescaler;
	hxspi->Init.MemorySize = find_msb_set(dev_cfg->memory_size) - 2;

	if (HAL_XSPI_Init(hxspi) != HAL_OK) {
		LOG_ERR("XSPI Init failed");
		return -EIO;
	}

#if !defined(CONFIG_STM32_XSPIM)
	if (!IS_ENABLED(CONFIG_STM32_APP_IN_EXT_FLASH)) {
		/*
		 * Do not configure the XSPIManager if running on the ext flash
		 * since this includes stopping each XSPI instance during configuration
		 */
		XSPIM_CfgTypeDef cfg = {0};

		if (hxspi->Instance == STM32_XSPI1) {
			cfg.IOPort = HAL_XSPIM_IOPORT_1;
		} else if (hxspi->Instance == STM32_XSPI2) {
			cfg.IOPort = HAL_XSPIM_IOPORT_2;
		} else {
			LOG_ERR("XSPIMgr Instance failed");
			return -EIO;
		}
		if (HAL_XSPIM_Config(hxspi, &cfg, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
			LOG_ERR("XSPIMgr Init failed");
			return -EIO;
		}
	}
#endif /* !CONFIG_STM32_XSPIM */

	/* Configure HyperBus timing via HLCR register */
	hcfg.RWRecoveryTimeCycle = dev_cfg->rw_recovery_time;
	hcfg.AccessTimeCycle = dev_cfg->access_time;
	hcfg.WriteZeroLatency = dev_cfg->write_zero_latency ? HAL_XSPI_NO_LATENCY_ON_WRITE
							     : HAL_XSPI_LATENCY_ON_WRITE;
	hcfg.LatencyMode = dev_cfg->fixed_latency ? HAL_XSPI_FIXED_LATENCY
						   : HAL_XSPI_VARIABLE_LATENCY;
	if (HAL_XSPI_HyperbusCfg(hxspi, &hcfg, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("XSPI HyperbusCfg failed");
		return -EIO;
	}

	/*
	 * Configure CCR and WCCR for x8 HyperBus DDR memory-mapped mode.
	 *
	 * This mirrors the register writes in HAL_XSPI_HyperbusCmd() but
	 * deliberately omits the DLR/AR writes that would trigger an indirect
	 * transfer.  The state is manually advanced to CMD_CFG so that the
	 * subsequent HAL_XSPI_MemoryMapped() call is accepted.
	 *
	 * CCR and WCCR share identical bit positions for all relevant fields.
	 */
	uint32_t bus_cfg = HAL_XSPI_DQS_ENABLE |
			   XSPI_CCR_DDTR |
			   HAL_XSPI_DATA_8_LINES |
			   HAL_XSPI_ADDRESS_32_BITS |
			   XSPI_CCR_ADDTR |
			   XSPI_CCR_ADMODE_2;

	WRITE_REG(hxspi->Instance->CCR, bus_cfg);
	WRITE_REG(hxspi->Instance->WCCR, bus_cfg);
	hxspi->State = HAL_XSPI_STATE_CMD_CFG;

	/* Enter memory-mapped mode */
#if defined(XSPI_CR_NOPREF)
	mm_cfg.NoPrefetchData = HAL_XSPI_AUTOMATIC_PREFETCH_ENABLE;
#endif /* XSPI_CR_NOPREF */
#if defined(XSPI_CR_NOPREF_AXI)
	mm_cfg.NoPrefetchAXI = HAL_XSPI_AXI_PREFETCH_DISABLE;
#endif /* XSPI_CR_NOPREF_AXI */
	mm_cfg.TimeOutActivation = HAL_XSPI_TIMEOUT_COUNTER_DISABLE;

	if (HAL_XSPI_MemoryMapped(hxspi, &mm_cfg) != HAL_OK) {
		LOG_ERR("XSPI MemoryMapped failed");
		return -EIO;
	}

#if defined(XSPI_CR_NOPREF)
	/* Disable automatic prefetch; RWDS-based DQS strobe manages timing. */
	stm32_reg_modify_bits(&hxspi->Instance->CR, XSPI_CR_NOPREF,
			      HAL_XSPI_AUTOMATIC_PREFETCH_DISABLE);
#endif /* XSPI_CR_NOPREF */

#ifdef CONFIG_SHARED_MULTI_HEAP
	shared_multi_heap_pool_init();
	ret = shared_multi_heap_add(&smh_hyperram, NULL);
	if (ret < 0) {
		return ret;
	}
#endif /* CONFIG_SHARED_MULTI_HEAP */

	return 0;
}

PINCTRL_DT_DEFINE(STM32_XSPI_NODE);

static const struct memc_stm32_xspi_hyperram_config memc_stm32_xspi_hyperram_cfg = {
	.pcfg = PINCTRL_DT_DEV_CONFIG_GET(STM32_XSPI_NODE),
	.pclken = STM32_CLOCK_INFO_BY_NAME(STM32_XSPI_NODE, xspix),
#if DT_CLOCKS_HAS_NAME(STM32_XSPI_NODE, xspi_ker)
	.pclken_ker = STM32_CLOCK_INFO_BY_NAME(STM32_XSPI_NODE, xspi_ker),
#endif /* DT_CLOCKS_HAS_NAME(STM32_XSPI_NODE, xspi_ker) */
#if DT_CLOCKS_HAS_NAME(STM32_XSPI_NODE, xspi_mgr)
	.pclken_mgr = STM32_CLOCK_INFO_BY_NAME(STM32_XSPI_NODE, xspi_mgr),
#endif /* DT_CLOCKS_HAS_NAME(STM32_XSPI_NODE, xspi_mgr) */
	.memory_size = DT_INST_PROP(0, size) / 8, /* bits → bytes */
	.max_frequency = DT_INST_PROP(0, max_frequency),
	.rw_recovery_time = DT_INST_PROP(0, rw_recovery_time_cycle),
	.access_time = DT_INST_PROP(0, access_time_cycle),
	.fixed_latency = DT_INST_PROP(0, fixed_latency),
	.write_zero_latency = DT_INST_PROP(0, write_zero_latency),
};

static struct memc_stm32_xspi_hyperram_data memc_stm32_xspi_hyperram_data = {
	.hxspi = {
		.Instance = (XSPI_TypeDef *)DT_REG_ADDR(STM32_XSPI_NODE),
		.Init = {
			.FifoThresholdByte = 2U,
			.MemoryMode = HAL_XSPI_SINGLE_MEM,
			.MemoryType = HAL_XSPI_MEMTYPE_HYPERBUS,
			.ChipSelectHighTimeCycle = 2U,
			.FreeRunningClock = HAL_XSPI_FREERUNCLK_DISABLE,
			.ClockMode = HAL_XSPI_CLOCK_MODE_0,
			.WrapSize = HAL_XSPI_WRAP_32_BYTES,
			.SampleShifting = HAL_XSPI_SAMPLE_SHIFT_NONE,
			.DelayHoldQuarterCycle = HAL_XSPI_DHQC_DISABLE,
			.ChipSelectBoundary = 0U,
			.MaxTran = 0U,
			.Refresh = DT_INST_PROP(0, st_refresh),
			.MemorySelect = HAL_XSPI_CSSEL_NCS1,
		},
	},
};

DEVICE_DT_INST_DEFINE(0, &memc_stm32_xspi_hyperram_init, NULL,
		      &memc_stm32_xspi_hyperram_data, &memc_stm32_xspi_hyperram_cfg,
		      POST_KERNEL, CONFIG_MEMC_INIT_PRIORITY,
		      NULL);
