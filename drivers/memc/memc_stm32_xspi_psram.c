/*
 * Copyright (c) 2025 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT st_stm32_xspi_psram

#include <errno.h>
#include <soc.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/stm32_clock_control.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(memc_stm32_xspi_psram, CONFIG_MEMC_LOG_LEVEL);

#define STM32_XSPI_NODE DT_INST_PARENT(0)

/* Memory registers definition */
#define MR0		0x00000000U
#define MR1		0x00000001U
#define MR2		0x00000002U
#define MR3		0x00000003U
#define MR4		0x00000004U
#define MR8		0x00000008U

/* Memory commands */
#define SYNC_READ_CMD	0x00U
#define SYNC_WRITE_CMD	0x80U
#define BURST_READ_CMD	0x20U
#define BURST_WRITE_CMD	0xA0U
#define READ_REG_CMD	0x40U
#define WRITE_REG_CMD	0xC0U
#define RESET_CMD	0xFFU

/* Memory default dummy clocks cycles */
#define DUMMY_CLK_CYCLES_READ	6U
#define DUMMY_CLK_CYCLES_WRITE	6U

struct memc_stm32_xspi_psram_config {
	const struct pinctrl_dev_config *pcfg;
	const struct stm32_pclken pclken;
	const struct stm32_pclken pclken_ker;
	const struct stm32_pclken pclken_mgr;
	size_t memory_size;
};

struct memc_stm32_xspi_psram_data {
	XSPI_HandleTypeDef hxspi;
};

static int ap_memory_write_reg(XSPI_HandleTypeDef *hxspi, uint32_t address, uint8_t *value)
{
	XSPI_RegularCmdTypeDef cmd = {0};

	/* Initialize the write register command.
	 * The following fields are already set to 0 thanks to cmd = {0}.
	 * They are kept in comment for better understanding of the command.
	 * cmd.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG;
	 * cmd.InstructionWidth = HAL_XSPI_INSTRUCTION_8_BITS;
	 * cmd.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
	 * cmd.DQSMode = HAL_XSPI_DQS_DISABLE;
	 */
	cmd.Instruction = WRITE_REG_CMD;
	cmd.InstructionMode = HAL_XSPI_INSTRUCTION_8_LINES;
	cmd.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_DISABLE;
	cmd.Address = address;
	cmd.AddressMode = HAL_XSPI_ADDRESS_8_LINES;
	cmd.AddressWidth = HAL_XSPI_ADDRESS_32_BITS;
	cmd.AddressDTRMode = HAL_XSPI_ADDRESS_DTR_ENABLE;
	cmd.DataMode = HAL_XSPI_DATA_8_LINES;
	cmd.DataLength = 2U;
	cmd.DataDTRMode = HAL_XSPI_DATA_DTR_ENABLE;

	/* Configure the command */
	if (HAL_XSPI_Command(hxspi, &cmd, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("XSPI write command failed");
		return -EIO;
	}

	/* Transmission of the data */
	if (HAL_XSPI_Transmit(hxspi, value, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("XSPI transmit failed");
		return -EIO;
	}

	return 0;
}

static int ap_memory_read_reg(XSPI_HandleTypeDef *hxspi, uint32_t address, uint8_t *value,
			      uint32_t latency_cycles)
{
	XSPI_RegularCmdTypeDef cmd = {0};

	/* Initialize the read register command
	 * The following fields are already set to 0 thanks to cmd = {0}.
	 * They are kept in comment for better understanding of the command.
	 * cmd.OperationType = HAL_XSPI_OPTYPE_COMMON_CFG;
	 * cmd.InstructionWidth = HAL_XSPI_INSTRUCTION_8_BITS;
	 * cmd.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_DISABLE;
	 * cmd.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
	 */
	cmd.Instruction = READ_REG_CMD;
	cmd.InstructionMode = HAL_XSPI_INSTRUCTION_8_LINES;
	cmd.Address = address;
	cmd.AddressMode = HAL_XSPI_ADDRESS_8_LINES;
	cmd.AddressWidth = HAL_XSPI_ADDRESS_32_BITS;
	cmd.AddressDTRMode = HAL_XSPI_ADDRESS_DTR_ENABLE;
	cmd.DataMode = HAL_XSPI_DATA_8_LINES;
	cmd.DataLength = 2U;
	cmd.DataDTRMode = HAL_XSPI_DATA_DTR_ENABLE;
	cmd.DummyCycles = latency_cycles;
	cmd.DQSMode = HAL_XSPI_DQS_ENABLE;

	/* Configure the command */
	if (HAL_XSPI_Command(hxspi, &cmd, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("XSPI read command failed");
		return -EIO;
	}

	/* Reception of the data */
	if (HAL_XSPI_Receive(hxspi, value, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("XSPI receive failed");
		return -EIO;
	}

	return 0;
}

static int ap_memory_configure(XSPI_HandleTypeDef *hxspi)
{
	uint8_t read_latency_code = DT_INST_PROP(0, read_latency);
	uint8_t read_latency_cycles = read_latency_code + 3U; /* Code 0 <=> 3 cycles... */

	/* MR0 register for read and write */
	uint8_t regW_MR0[2] = {(DT_INST_PROP(0, fixed_latency) ? 0x20U : 0x00U) |
			       (read_latency_code << 2) |
			       (DT_INST_PROP(0, drive_strength)), 0x8DU};
	uint8_t regR_MR0[2] = {0};

	/* MR4 register for read and write */
	uint8_t regW_MR4[2] = {(DT_INST_PROP(0, write_latency) << 5) |
			      (DT_INST_PROP(0, refresh_rate) << 3) |
			      (DT_INST_PROP(0, pasr)), 0x05U};
	uint8_t regR_MR4[2] = {0};

	/* MR8 register for read and write */
	uint8_t regW_MR8[2] = {(DT_INST_PROP(0, io_x16_mode) ? 0x40U : 0x00U) |
			       (DT_INST_PROP(0, rbx) ? 0x08U : 0x00U) |
			       (DT_INST_PROP(0, burst_type_hybrid_wrap) ? 0x04U : 0x00U) |
			       (DT_INST_PROP(0, burst_length)), 0x08U};
	uint8_t regR_MR8[2] = {0};

	/* Configure Read Latency and drive Strength */
	if (ap_memory_write_reg(hxspi, MR0, regW_MR0) != 0) {
		return -EIO;
	}

	/* Check MR0 configuration */
	if (ap_memory_read_reg(hxspi, MR0, regR_MR0, read_latency_cycles) != 0) {
		return -EIO;
	}
	if (regR_MR0[0] != regW_MR0[0]) {
		return -EIO;
	}

	/* Configure Write Latency and refresh rate */
	if (ap_memory_write_reg(hxspi, MR4, regW_MR4) != 0) {
		return -EIO;
	}

	/* Check MR4 configuration */
	if (ap_memory_read_reg(hxspi, MR4, regR_MR4, read_latency_cycles) != 0) {
		return -EIO;
	}
	if (regR_MR4[0] != regW_MR4[0]) {
		return -EIO;
	}

	/* Configure Burst Length */
	if (ap_memory_write_reg(hxspi, MR8, regW_MR8) != 0) {
		return -EIO;
	}

	/* Check MR8 configuration */
	if (ap_memory_read_reg(hxspi, MR8, regR_MR8, read_latency_cycles) != 0) {
		return -EIO;
	}
	if (regR_MR8[0] != regW_MR8[0]) {
		return -EIO;
	}

	return 0;
}

static int memc_stm32_xspi_psram_init(const struct device *dev)
{
	const struct memc_stm32_xspi_psram_config *dev_cfg = dev->config;
	struct memc_stm32_xspi_psram_data *dev_data = dev->data;
	XSPI_HandleTypeDef hxspi = dev_data->hxspi;
	XSPI_TypeDef *xspi = hxspi.Instance;
	uint32_t ahb_clock_freq;
	XSPIM_CfgTypeDef cfg = {0};
	XSPI_RegularCmdTypeDef cmd = {0};
	XSPI_MemoryMappedTypeDef mem_mapped_cfg = {0};
	int ret;

	/* Signals configuration */
	ret = pinctrl_apply_state(dev_cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("XSPI pinctrl setup failed (%d)", ret);
		return ret;
	}

	if (!device_is_ready(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE))) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	/* Clock configuration */
	if (clock_control_on(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
			     (clock_control_subsys_t) &dev_cfg->pclken) != 0) {
		LOG_ERR("Could not enable XSPI clock");
		return -EIO;
	}
	if (clock_control_get_rate(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
				   (clock_control_subsys_t) &dev_cfg->pclken,
				   &ahb_clock_freq) < 0) {
		LOG_ERR("Failed call clock_control_get_rate(pclken)");
		return -EIO;
	}

#if DT_CLOCKS_HAS_NAME(STM32_XSPI_NODE, xspi_ker)
	/* Kernel clock config for peripheral if any */
	if (clock_control_configure(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
					(clock_control_subsys_t) &dev_cfg->pclken_ker,
					NULL) != 0) {
		LOG_ERR("Could not select XSPI domain clock");
		return -EIO;
	}

	if (clock_control_get_rate(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
					(clock_control_subsys_t) &dev_cfg->pclken_ker,
					&ahb_clock_freq) < 0) {
		LOG_ERR("Failed call clock_control_get_rate(pclken_ker)");
		return -EIO;
	}
#endif

#if DT_CLOCKS_HAS_NAME(STM32_XSPI_NODE, xspi_mgr)
	/* Clock domain corresponding to the IO-Mgr (XSPIM) */
	if (clock_control_on(DEVICE_DT_GET(STM32_CLOCK_CONTROL_NODE),
				(clock_control_subsys_t) &dev_cfg->pclken_mgr) != 0) {
		LOG_ERR("Could not enable XSPI Manager clock");
		return -EIO;
	}
#endif

	hxspi.Init.MemorySize = find_msb_set(dev_cfg->memory_size) - 2;

	if (HAL_XSPI_Init(&hxspi) != HAL_OK) {
		LOG_ERR("XSPI Init failed");
		return -EIO;
	}

	cfg.nCSOverride = HAL_XSPI_CSSEL_OVR_NCS1;
	cfg.IOPort = HAL_XSPIM_IOPORT_1;

	if (HAL_XSPIM_Config(&hxspi, &cfg, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		LOG_ERR("XSPIMgr Init failed");
		return -EIO;
	}

	/* Configure AP memory registers */
	ret = ap_memory_configure(&hxspi);
	if (ret != 0) {
		LOG_ERR("AP memory configuration failed");
		return -EIO;
	}

	/* The following fields are already set to 0 thanks to cmd = {0}.
	 * They are kept in comment for better understanding of the command.
	 * cmd.InstructionWidth = HAL_XSPI_INSTRUCTION_8_BITS;
	 * cmd.InstructionDTRMode = HAL_XSPI_INSTRUCTION_DTR_DISABLE;
	 * cmd.Address = 0x0U;
	 * cmd.AlternateBytesMode = HAL_XSPI_ALT_BYTES_NONE;
	 */
	cmd.OperationType = HAL_XSPI_OPTYPE_WRITE_CFG;
	cmd.InstructionMode = HAL_XSPI_INSTRUCTION_8_LINES;
	cmd.Instruction = BURST_WRITE_CMD;
	cmd.AddressMode = HAL_XSPI_ADDRESS_8_LINES;
	cmd.AddressWidth = HAL_XSPI_ADDRESS_32_BITS;
	cmd.AddressDTRMode = HAL_XSPI_ADDRESS_DTR_ENABLE;
	cmd.DataMode = HAL_XSPI_DATA_16_LINES;
	cmd.DataDTRMode = HAL_XSPI_DATA_DTR_ENABLE;
	cmd.DummyCycles = DUMMY_CLK_CYCLES_WRITE;
	cmd.DQSMode = HAL_XSPI_DQS_ENABLE;

	if (HAL_XSPI_Command(&hxspi, &cmd, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		return -EIO;
	}

	cmd.OperationType = HAL_XSPI_OPTYPE_READ_CFG;
	cmd.Instruction = BURST_READ_CMD;
	cmd.DummyCycles = DUMMY_CLK_CYCLES_READ;

	if (HAL_XSPI_Command(&hxspi, &cmd, HAL_XSPI_TIMEOUT_DEFAULT_VALUE) != HAL_OK) {
		return -EIO;
	}

	mem_mapped_cfg.TimeOutActivation = HAL_XSPI_TIMEOUT_COUNTER_DISABLE;
	mem_mapped_cfg.NoPrefetchData = HAL_XSPI_AUTOMATIC_PREFETCH_ENABLE;
	mem_mapped_cfg.NoPrefetchAXI = HAL_XSPI_AXI_PREFETCH_DISABLE;

	if (HAL_XSPI_MemoryMapped(&hxspi, &mem_mapped_cfg) != HAL_OK) {
		return -EIO;
	}

	MODIFY_REG(xspi->CR, XSPI_CR_NOPREF, HAL_XSPI_AUTOMATIC_PREFETCH_DISABLE);

	return 0;
}

PINCTRL_DT_DEFINE(STM32_XSPI_NODE);

static const struct memc_stm32_xspi_psram_config memc_stm32_xspi_cfg = {
	.pcfg = PINCTRL_DT_DEV_CONFIG_GET(STM32_XSPI_NODE),
	.pclken = {.bus = DT_CLOCKS_CELL_BY_NAME(STM32_XSPI_NODE, xspix, bus),
		   .enr = DT_CLOCKS_CELL_BY_NAME(STM32_XSPI_NODE, xspix, bits)},
#if DT_CLOCKS_HAS_NAME(STM32_XSPI_NODE, xspi_ker)
	.pclken_ker = {.bus = DT_CLOCKS_CELL_BY_NAME(STM32_XSPI_NODE, xspi_ker, bus),
		       .enr = DT_CLOCKS_CELL_BY_NAME(STM32_XSPI_NODE, xspi_ker, bits)},
#endif
#if DT_CLOCKS_HAS_NAME(STM32_XSPI_NODE, xspi_mgr)
	.pclken_mgr = {.bus = DT_CLOCKS_CELL_BY_NAME(STM32_XSPI_NODE, xspi_mgr, bus),
		       .enr = DT_CLOCKS_CELL_BY_NAME(STM32_XSPI_NODE, xspi_mgr, bits)},
#endif
	.memory_size = DT_INST_REG_ADDR_BY_IDX(0, 1),
};

static struct memc_stm32_xspi_psram_data memc_stm32_xspi_data = {
	.hxspi = {
		.Instance = (XSPI_TypeDef *)DT_REG_ADDR(STM32_XSPI_NODE),
		.Init = {
			.FifoThresholdByte = 8U,
			.MemoryMode = HAL_XSPI_SINGLE_MEM,
			.MemoryType = (DT_INST_PROP(0, io_x16_mode) ?
					HAL_XSPI_MEMTYPE_APMEM_16BITS :
					HAL_XSPI_MEMTYPE_APMEM),
			.ChipSelectHighTimeCycle = 1U,
			.FreeRunningClock = HAL_XSPI_FREERUNCLK_DISABLE,
			.ClockMode = HAL_XSPI_CLOCK_MODE_0,
			.WrapSize = HAL_XSPI_WRAP_NOT_SUPPORTED,
			.ClockPrescaler = 3U,
			.SampleShifting = HAL_XSPI_SAMPLE_SHIFT_NONE,
			.DelayHoldQuarterCycle = HAL_XSPI_DHQC_ENABLE,
			.ChipSelectBoundary = HAL_XSPI_BONDARYOF_16KB,
			.MaxTran = 0U,
			.Refresh = 0x81U,
			.MemorySelect = HAL_XSPI_CSSEL_NCS1,
		},
	},
};

DEVICE_DT_INST_DEFINE(0, &memc_stm32_xspi_psram_init, NULL,
		      &memc_stm32_xspi_data, &memc_stm32_xspi_cfg,
		      POST_KERNEL, CONFIG_MEMC_INIT_PRIORITY,
		      NULL);
