/*
 * Copyright (c) 2025 ZAL Zentrum für Angewandte Luftfahrtforschung GmbH
 * Copyright (c) 2025 Mario Paja
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_ospi_psram

#include <errno.h>
#include <soc.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#include <zephyr/multi_heap/shared_multi_heap.h>

#define STM32_OSPI_CLOCK_PRESCALER_MIN                1U
#define STM32_OSPI_CLOCK_PRESCALER_MAX                256U
#define STM32_OSPI_CLOCK_COMPUTE(bus_freq, prescaler) ((bus_freq) / (prescaler))

LOG_MODULE_REGISTER(memc_stm32_ospi_psram, CONFIG_MEMC_LOG_LEVEL);

#define STM32_OSPI_NODE DT_INST_PARENT(0)

#ifdef CONFIG_SHARED_MULTI_HEAP
static const struct shared_multi_heap_region smh_psram = {
	.addr = DT_REG_ADDR(DT_NODELABEL(psram)),
	.size = DT_REG_SIZE(DT_NODELABEL(psram)),
	.attr = SMH_REG_ATTR_EXTERNAL,
};
#endif

/* Memory registers definition */
#define MR0 0x00000000U
#define MR1 0x00000001U
#define MR2 0x00000002U
#define MR3 0x00000003U
#define MR4 0x00000004U
#define MR8 0x00000008U

/* Memory commands */
#define SYNC_READ_CMD   0x00U
#define SYNC_WRITE_CMD  0x80U
#define BURST_READ_CMD  0x20U
#define BURST_WRITE_CMD 0xA0U
#define READ_REG_CMD    0x40U
#define WRITE_REG_CMD   0xC0U
#define RESET_CMD       0xFFU

/* Write Operations */
#define WRITE_CMD 0x8080

/* Read Operations */
#define READ_CMD 0x0000

/* Default dummy clocks cycles */
#define DUMMY_CLOCK_CYCLES_READ  8
#define DUMMY_CLOCK_CYCLES_WRITE 4

struct memc_stm32_ospi_psram_config {
	OCTOSPI_TypeDef *regs;
	const struct pinctrl_dev_config *pcfg;
	const struct stm32_pclken pclken;
#if DT_CLOCKS_HAS_NAME(STM32_OSPI_NODE, ospi_ker)
	const struct stm32_pclken pclken_ker; /* clock subsystem */
#endif
#if DT_CLOCKS_HAS_NAME(STM32_OSPI_NODE, ospi_mgr)
	const struct stm32_pclken pclken_mgr; /* clock subsystem */
#endif
	size_t memory_size;
	uint32_t max_frequency;
};

struct memc_stm32_ospi_psram_data {
	OSPI_HandleTypeDef hospi;
	OSPIM_CfgTypeDef ospim_cfg;
	HAL_OSPI_DLYB_CfgTypeDef dlyb_cfg;
};

static int ap_memory_write_reg(OSPI_HandleTypeDef *hospi, uint32_t address, uint8_t *value)
{
	OSPI_RegularCmdTypeDef cmd = {0};

	/* Initialize the write register command */
	cmd.OperationType = HAL_OSPI_OPTYPE_COMMON_CFG;
	cmd.InstructionMode = HAL_OSPI_INSTRUCTION_8_LINES;
	cmd.InstructionSize = HAL_OSPI_INSTRUCTION_8_BITS;
	cmd.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
	cmd.Instruction = WRITE_REG_CMD;
	cmd.AddressMode = HAL_OSPI_ADDRESS_8_LINES;
	cmd.AddressSize = HAL_OSPI_ADDRESS_32_BITS;
	cmd.AddressDtrMode = HAL_OSPI_ADDRESS_DTR_ENABLE;
	cmd.Address = address;
	cmd.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
	cmd.DataMode = HAL_OSPI_DATA_8_LINES;
	cmd.DataDtrMode = HAL_OSPI_DATA_DTR_ENABLE;
	cmd.NbData = 2;
	cmd.DummyCycles = 0;
	cmd.DQSMode = HAL_OSPI_DQS_DISABLE;
	cmd.SIOOMode = HAL_OSPI_SIOO_INST_EVERY_CMD;

	/* Configure the command */
	if (HAL_OSPI_Command(hospi, &cmd, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("OSPI write command failed");
		return -EIO;
	}

	/* Transmission of the data */
	if (HAL_OSPI_Transmit(hospi, value, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("OSPI transmit failed");
		return -EIO;
	}

	return 0;
}

static int ap_memory_read_reg(OSPI_HandleTypeDef *hospi, uint32_t address, uint8_t *value,
			      uint32_t latency_cycles)
{
	OSPI_RegularCmdTypeDef cmd = {0};

	/* Initialize the read register command */
	cmd.OperationType = HAL_OSPI_OPTYPE_COMMON_CFG;
	cmd.InstructionMode = HAL_OSPI_INSTRUCTION_8_LINES;
	cmd.InstructionSize = HAL_OSPI_INSTRUCTION_8_BITS;
	cmd.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_DISABLE;
	cmd.Instruction = READ_REG_CMD;
	cmd.AddressMode = HAL_OSPI_ADDRESS_8_LINES;
	cmd.AddressSize = HAL_OSPI_ADDRESS_32_BITS;
	cmd.AddressDtrMode = HAL_OSPI_ADDRESS_DTR_ENABLE;
	cmd.Address = address;
	cmd.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
	cmd.DataMode = HAL_OSPI_DATA_8_LINES;
	cmd.DataDtrMode = HAL_OSPI_DATA_DTR_ENABLE;
	cmd.NbData = 2;
	cmd.DummyCycles = latency_cycles;
	cmd.DQSMode = HAL_OSPI_DQS_ENABLE;
	cmd.SIOOMode = HAL_OSPI_SIOO_INST_EVERY_CMD;

	/* Configure the command */
	if (HAL_OSPI_Command(hospi, &cmd, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("OSPI read command failed");
		return -EIO;
	}

	/* Reception of the data */
	if (HAL_OSPI_Receive(hospi, value, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("OSPI receive failed");
		return -EIO;
	}

	return 0;
}

static int ap_memory_configure(OSPI_HandleTypeDef *hospi)
{
	uint8_t read_latency_code = DT_INST_PROP(0, read_latency);
	uint8_t read_latency_cycles = read_latency_code + 3U; /* Code 0 <=> 3 cycles... */

	/* MR0 register for read and write */
	uint8_t regW_MR0[2] = {(DT_INST_PROP(0, fixed_latency) ? 0x20U : 0x00U) |
				       (read_latency_code << 2) | (DT_INST_PROP(0, drive_strength)),
			       0x0D};
	uint8_t regR_MR0[2] = {0};

	/* MR4 register for read and write */
	uint8_t regW_MR4[2] = {(DT_INST_PROP(0, write_latency) << 5) |
				       (DT_INST_PROP(0, adaptive_refresh_rate) << 3) |
				       (DT_INST_PROP(0, pasr)),
			       0x05U};
	uint8_t regR_MR4[2] = {0};

	/* MR8 register for read and write */
	uint8_t regW_MR8[2] = {(DT_INST_PROP(0, rbx) ? 0x08U : 0x00U) |
				       (DT_INST_PROP(0, burst_type_hybrid_wrap) ? 0x04U : 0x00U) |
				       (DT_INST_PROP(0, burst_length)),
			       0x08U};
	uint8_t regR_MR8[2] = {0};

	/* Configure Read Latency and drive Strength */
	if (ap_memory_write_reg(hospi, MR0, regW_MR0) != 0) {
		return -EIO;
	}

	if (ap_memory_read_reg(hospi, MR0, regR_MR0, read_latency_cycles) != 0) {
		return -EIO;
	}

	if (regR_MR0[0] != regW_MR0[0]) {
		return -EIO;
	}

	/* Configure Write Latency and refresh rate */
	if (ap_memory_write_reg(hospi, MR4, regW_MR4) != 0) {
		return -EIO;
	}

	/* Check MR4 configuration */
	if (ap_memory_read_reg(hospi, MR4, regR_MR4, read_latency_cycles) != 0) {
		return -EIO;
	}
	if (regR_MR4[0] != regW_MR4[0]) {
		return -EIO;
	}

	/* Configure Burst Length */
	if (ap_memory_write_reg(hospi, MR8, regW_MR8) != HAL_OK) {
		return -EIO;
	}

	/* Check MR8 configuration */
	if (ap_memory_read_reg(hospi, MR8, regR_MR8, read_latency_cycles) != HAL_OK) {
		return -EIO;
	}

	if (regR_MR8[0] != regW_MR8[0]) {
		return -EIO;
	}

	return 0;
}

static int config_memory_mapped(const struct device *dev)
{
	struct memc_stm32_ospi_psram_data *dev_data = dev->data;
	OSPI_HandleTypeDef *hospi = &dev_data->hospi;
	OSPI_RegularCmdTypeDef cmd = {0};
	OSPI_MemoryMappedTypeDef mem_mapped_cfg = {0};

	/*Configure Memory Mapped mode*/
	cmd.OperationType = HAL_OSPI_OPTYPE_WRITE_CFG;
	cmd.FlashId = HAL_OSPI_FLASH_ID_1;
	cmd.Instruction = WRITE_CMD;
	cmd.InstructionMode = HAL_OSPI_INSTRUCTION_8_LINES;
	cmd.InstructionSize = HAL_OSPI_INSTRUCTION_16_BITS;
	cmd.InstructionDtrMode = HAL_OSPI_INSTRUCTION_DTR_ENABLE;
	cmd.AddressMode = HAL_OSPI_ADDRESS_8_LINES;
	cmd.AddressSize = HAL_OSPI_ADDRESS_32_BITS;
	cmd.AddressDtrMode = HAL_OSPI_ADDRESS_DTR_ENABLE;
	cmd.AlternateBytesMode = HAL_OSPI_ALTERNATE_BYTES_NONE;
	cmd.DataMode = HAL_OSPI_DATA_8_LINES;
	cmd.DataDtrMode = HAL_OSPI_DATA_DTR_ENABLE;
	cmd.DummyCycles = DUMMY_CLOCK_CYCLES_WRITE;
	cmd.DQSMode = HAL_OSPI_DQS_ENABLE;
	cmd.SIOOMode = HAL_OSPI_SIOO_INST_EVERY_CMD;

	if (HAL_OSPI_Command(hospi, &cmd, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		return -EIO;
	}

	cmd.OperationType = HAL_OSPI_OPTYPE_READ_CFG;
	cmd.Instruction = READ_CMD;
	cmd.DummyCycles = DUMMY_CLOCK_CYCLES_READ;

	if (HAL_OSPI_Command(hospi, &cmd, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		return -EIO;
	}

	mem_mapped_cfg.TimeOutActivation = HAL_OSPI_TIMEOUT_COUNTER_ENABLE;
	mem_mapped_cfg.TimeOutPeriod = 0x34;

	if (HAL_OSPI_MemoryMapped(hospi, &mem_mapped_cfg) != HAL_OK) {
		return -EIO;
	}

	return 0;
}

static int memc_stm32_ospi_psram_init(const struct device *dev)
{
	const struct memc_stm32_ospi_psram_config *dev_cfg = dev->config;
	struct memc_stm32_ospi_psram_data *dev_data = dev->data;
	OSPI_HandleTypeDef *hospi = &dev_data->hospi;
	OSPIM_CfgTypeDef *ospim_cfg = &dev_data->ospim_cfg;
	HAL_OSPI_DLYB_CfgTypeDef *dlyb_cfg = &dev_data->dlyb_cfg;
	LL_DLYB_CfgTypeDef ll_dlyb_cfg, ll_dlyb_cfg_test;
	uint32_t ahb_clock_freq;
	uint32_t prescaler;
	int ret;

	ret = pinctrl_apply_state(dev_cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("OSPI pinctrl setup failed (%d)", ret);
		return ret;
	}

	if (!device_is_ready(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE))) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	if (clock_control_on(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
			     (clock_control_subsys_t)&dev_cfg->pclken) != 0) {
		LOG_ERR("Could not enable OSPI clock");
		return -EIO;
	}

	/* Alternate clock config for peripheral if any */
#if DT_CLOCKS_HAS_NAME(STM32_OSPI_NODE, ospi_ker)
	if (clock_control_configure(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
				    (clock_control_subsys_t)&dev_cfg->pclken_ker, NULL) != 0) {
		LOG_ERR("Could not select OSPI domain clock");
		return -EIO;
	}
	if (clock_control_get_rate(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
				   (clock_control_subsys_t)&dev_cfg->pclken_ker,
				   &ahb_clock_freq) < 0) {
		LOG_ERR("Failed call clock_control_get_rate(pclken_ker)");
		return -EIO;
	}
#else
	if (clock_control_get_rate(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
				   (clock_control_subsys_t)&dev_cfg->pclken, &ahb_clock_freq) < 0) {
		return -EIO;
	}
#endif
#if DT_CLOCKS_HAS_NAME(STM32_OSPI_NODE, ospi_mgr)
	if (clock_control_on(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
			     (clock_control_subsys_t)&dev_cfg->pclken_mgr) != 0) {
		LOG_ERR("Could not enable OSPI Manager clock");
		return -EIO;
	}
#endif

	LOG_DBG("OSPI AHB clock frequency: %d Hz", ahb_clock_freq);
	LOG_DBG("OSPI max frequency: %d Hz", dev_cfg->max_frequency);

	prescaler = STM32_OSPI_CLOCK_PRESCALER_MIN;
	for (; prescaler <= STM32_OSPI_CLOCK_PRESCALER_MAX; prescaler++) {
		uint32_t clk = STM32_OSPI_CLOCK_COMPUTE(ahb_clock_freq, prescaler);

		if (clk <= dev_cfg->max_frequency) {
			LOG_DBG("clk: %d, prescaler: %d", clk, prescaler);
			break;
		}
	}

	if (prescaler > STM32_OSPI_CLOCK_PRESCALER_MAX) {
		LOG_ERR("OSPI could not find valid prescaler value");
		return -EINVAL;
	}

	hospi->Init.ClockPrescaler = prescaler;

	LOG_DBG("MSB set: %d", find_msb_set(dev_cfg->memory_size) - 1);
	hospi->Init.DeviceSize = find_msb_set(dev_cfg->memory_size) - 1;

	/* OSPI Init */
	if (HAL_OSPI_Init(hospi) != HAL_OK) {
		LOG_ERR("HAL_OSPI_Init(): <FAILED>");
		return -EIO;
	}

	if (HAL_OSPIM_Config(hospi, ospim_cfg, HAL_OSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("HAL_OSPIM_Config(): <FAILED>");
		return -EIO;
	}

	if (HAL_OSPI_DLYB_SetConfig(hospi, dlyb_cfg) != HAL_OK) {
		LOG_ERR("HAL_OSPI_DLYB_SetConfig(): <FAILED>");
		return -EIO;
	}

	if (HAL_OSPI_DLYB_GetClockPeriod(hospi, &ll_dlyb_cfg) != HAL_OK) {
		LOG_ERR("HAL_OSPI_DLYB_GetClockPeriod(): <FAILED>");
		return -EIO;
	}

	/* When DTR, PhaseSel is divided by 4 (emperic value) */
	ll_dlyb_cfg.PhaseSel /= 4;
	ll_dlyb_cfg_test = ll_dlyb_cfg;

	HAL_OSPI_DLYB_SetConfig(hospi, &ll_dlyb_cfg);
	HAL_OSPI_DLYB_GetConfig(hospi, &ll_dlyb_cfg);

	if ((ll_dlyb_cfg.PhaseSel != ll_dlyb_cfg_test.PhaseSel) ||
	    (ll_dlyb_cfg.Units != ll_dlyb_cfg_test.Units)) {
		LOG_ERR("PhaseSel: <FAILED>");
		return -EIO;
	}

	if (ap_memory_configure(hospi) != 0) {
		LOG_ERR("ap_memory_configure(): <FAILED>");
		return -EIO;
	}
	if (config_memory_mapped(dev) != 0) {
		LOG_ERR("config_memory_mapped(): <FAILED>");
		return -EIO;
	}

#ifdef CONFIG_SHARED_MULTI_HEAP
	ret = shared_multi_heap_pool_init();
	if (ret < 0) {
		return ret;
	}
	ret = shared_multi_heap_add(&smh_psram, NULL);
	if (ret < 0) {
		return ret;
	}
#endif

	return 0;
}

PINCTRL_DT_DEFINE(STM32_OSPI_NODE);

static struct memc_stm32_ospi_psram_config memc_stm32_ospi_config = {
	.pclken = {.bus = DT_CLOCKS_CELL_BY_NAME(STM32_OSPI_NODE, ospix, bus),
		   .enr = DT_CLOCKS_CELL_BY_NAME(STM32_OSPI_NODE, ospix, bits)},
	.pcfg = PINCTRL_DT_DEV_CONFIG_GET(STM32_OSPI_NODE),
	.memory_size = DT_INST_PROP(0, size) / 8, /* In Bytes */
	.max_frequency = DT_INST_PROP(0, max_frequency),
#if DT_CLOCKS_HAS_NAME(STM32_OSPI_NODE, ospi_ker)
	.pclken_ker = {.bus = DT_CLOCKS_CELL_BY_NAME(STM32_OSPI_NODE, ospi_ker, bus),
		       .enr = DT_CLOCKS_CELL_BY_NAME(STM32_OSPI_NODE, ospi_ker, bits)},
#endif
#if DT_CLOCKS_HAS_NAME(STM32_OSPI_NODE, ospi_mgr)
	.pclken_mgr = {.bus = DT_CLOCKS_CELL_BY_NAME(STM32_OSPI_NODE, ospi_mgr, bus),
		       .enr = DT_CLOCKS_CELL_BY_NAME(STM32_OSPI_NODE, ospi_mgr, bits)},
#endif
};

static struct memc_stm32_ospi_psram_data memc_stm32_ospi_data = {
	.hospi = {
		.Instance = (OCTOSPI_TypeDef *)DT_REG_ADDR(STM32_OSPI_NODE),
		.Init = {
			.FifoThreshold = 1,
			.DualQuad = HAL_OSPI_DUALQUAD_DISABLE,
			.MemoryType = HAL_OSPI_MEMTYPE_APMEMORY,
			.ChipSelectHighTime = 2,
			.FreeRunningClock = HAL_OSPI_FREERUNCLK_DISABLE,
			.ClockMode = HAL_OSPI_CLOCK_MODE_0,
			.WrapSize = HAL_OSPI_WRAP_NOT_SUPPORTED,
			.SampleShifting = HAL_OSPI_SAMPLE_SHIFTING_NONE,
			.DelayHoldQuarterCycle = HAL_OSPI_DHQC_ENABLE,
			.ChipSelectBoundary = 10,
			.DelayBlockBypass = HAL_OSPI_DELAY_BLOCK_USED,
			.MaxTran = 0,
			.Refresh = 320,
		},
	},
	.ospim_cfg = {
		.ClkPort = 1,
		.DQSPort = 1,
		.NCSPort = 1,
		.IOLowPort = HAL_OSPIM_IOPORT_1_LOW,
		.IOHighPort = HAL_OSPIM_IOPORT_1_HIGH,
		.Req2AckTime = 0,
	},
	.dlyb_cfg = {
		.Units = 0,
		.PhaseSel = 0,
	},
};

DEVICE_DT_INST_DEFINE(0, &memc_stm32_ospi_psram_init, NULL, &memc_stm32_ospi_data,
		      &memc_stm32_ospi_config, POST_KERNEL,
		      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);
