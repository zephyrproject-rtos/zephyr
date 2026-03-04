/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_flexspi_mspi_controller

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/mspi.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/util.h>

#include <soc.h>
#include <fsl_flexspi.h>

LOG_MODULE_REGISTER(mspi_mcux_flexspi, CONFIG_MSPI_LOG_LEVEL);

#define MCUX_FLEXSPI_LUT_INDEX        0U
#define MCUX_FLEXSPI_LUT_COUNT        4U
#define MCUX_FLEXSPI_LUT_SEQ_NUM      1U
#define MCUX_FLEXSPI_DATA_LUT_OPERAND 0x04U

#if defined(FSL_FEATURE_FLEXSPI_HAS_NO_MCR0_COMBINATIONEN) &&                                      \
	(FSL_FEATURE_FLEXSPI_HAS_NO_MCR0_COMBINATIONEN)
#define MCUX_FLEXSPI_HAS_MCR0_COMBINATIONEN 0
#else
#define MCUX_FLEXSPI_HAS_MCR0_COMBINATIONEN 1
#endif

#if defined(FSL_FEATURE_FLEXSPI_HAS_NO_MCR2_SCKBDIFFOPT) &&                                        \
	(FSL_FEATURE_FLEXSPI_HAS_NO_MCR2_SCKBDIFFOPT)
#define MCUX_FLEXSPI_HAS_MCR2_SCKBDIFFOPT 0
#else
#define MCUX_FLEXSPI_HAS_MCR2_SCKBDIFFOPT 1
#endif

#if defined(FSL_FEATURE_FLEXSPI_SUPPORT_SEPE\
RATE_RXCLKSRC_PORTB) &&                                                                            \
	(FSL_FEATURE_FLEXSPI_SUPPORT_SEPE\
RATE_RXCLKSRC_PORTB)
#define MCUX_FLEXSPI_HAS_RXCLKSRC_PORTB 1
#else
#define MCUX_FLEXSPI_HAS_RXCLKSRC_PORTB 0
#endif

struct mspi_mcux_flexspi_buf_cfg {
	uint16_t prefetch;
	uint16_t priority;
	uint16_t master_id;
	uint16_t buf_size;
} __packed;

struct mspi_mcux_flexspi_config {
	FLEXSPI_Type *base;
	uintptr_t ahb_base;
	size_t ahb_size;
	const struct pinctrl_dev_config *pincfg;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	const uint32_t *flash_size_kb;
	struct mspi_cfg mspicfg;
	struct mspi_mcux_flexspi_buf_cfg *buf_cfg;
	uint8_t buf_cfg_cnt;
	uint8_t ahb_boundary;
	bool ahb_bufferable;
	bool ahb_cacheable;
	bool ahb_prefetch;
	bool ahb_read_addr_opt;
	bool combination_mode;
	bool sck_differential_clock;
	flexspi_read_sample_clock_t rx_sample_clock;
#if MCUX_FLEXSPI_HAS_RXCLKSRC_PORTB
	flexspi_read_sample_clock_t rx_sample_clock_b;
#endif
	uint32_t packet_data_limit;
};

struct mspi_mcux_flexspi_data {
	struct mspi_cfg mspicfg;
	struct mspi_dev_cfg dev_cfg;
	const struct mspi_dev_id *dev_id;
	struct k_sem cfg_lock;
	struct k_mutex xfer_lock;
};

struct mspi_mcux_flexspi_instr {
	uint8_t cmd;
	flexspi_pad_t pad;
	uint8_t op;
};

enum mspi_mcux_flexspi_phase {
	MSPI_MCUX_FLEXSPI_PHASE_COMMAND,
	MSPI_MCUX_FLEXSPI_PHASE_ADDRESS,
	MSPI_MCUX_FLEXSPI_PHASE_DATA,
};

enum mspi_mcux_flexspi_lut_kind {
	MSPI_MCUX_FLEXSPI_LUT_KIND_COMMAND,
	MSPI_MCUX_FLEXSPI_LUT_KIND_ADDRESS,
	MSPI_MCUX_FLEXSPI_LUT_KIND_DUMMY,
	MSPI_MCUX_FLEXSPI_LUT_KIND_READ,
	MSPI_MCUX_FLEXSPI_LUT_KIND_WRITE,
};

struct mspi_mcux_flexspi_io_cfg {
	flexspi_pad_t cmd_pad;
	flexspi_pad_t addr_pad;
	flexspi_pad_t data_pad;
};

static bool mspi_mcux_flexspi_running_from_xip_window(const struct mspi_mcux_flexspi_config *cfg)
{
	uintptr_t pc;

	if ((cfg->ahb_size == 0U) || (cfg->ahb_base > UINTPTR_MAX - cfg->ahb_size)) {
		return false;
	}

	__asm__ volatile("mov %0, pc" : "=r"(pc));

	return (pc >= cfg->ahb_base) && (pc < (cfg->ahb_base + cfg->ahb_size));
}

static uint32_t mspi_mcux_flexspi_get_port_offset(const struct mspi_mcux_flexspi_config *cfg,
						  uint8_t port)
{
	uint32_t offset = 0U;

	for (uint8_t idx = 0U; idx < port; idx++) {
		offset += cfg->flash_size_kb[idx] * KB(1);
	}

	return offset;
}

static uint8_t mspi_mcux_flexspi_lut_opcode(enum mspi_mcux_flexspi_lut_kind kind, bool ddr)
{
	switch (kind) {
	case MSPI_MCUX_FLEXSPI_LUT_KIND_COMMAND:
		return ddr ? kFLEXSPI_Command_DDR : kFLEXSPI_Command_SDR;
	case MSPI_MCUX_FLEXSPI_LUT_KIND_ADDRESS:
		return ddr ? kFLEXSPI_Command_RADDR_DDR : kFLEXSPI_Command_RADDR_SDR;
	case MSPI_MCUX_FLEXSPI_LUT_KIND_DUMMY:
		return ddr ? kFLEXSPI_Command_DUMMY_DDR : kFLEXSPI_Command_DUMMY_SDR;
	case MSPI_MCUX_FLEXSPI_LUT_KIND_READ:
		return ddr ? kFLEXSPI_Command_READ_DDR : kFLEXSPI_Command_READ_SDR;
	case MSPI_MCUX_FLEXSPI_LUT_KIND_WRITE:
		return ddr ? kFLEXSPI_Command_WRITE_DDR : kFLEXSPI_Command_WRITE_SDR;
	default:
		return kFLEXSPI_Command_STOP;
	}
}

static bool mspi_mcux_flexspi_phase_is_ddr(enum mspi_data_rate data_rate,
					   enum mspi_mcux_flexspi_phase phase)
{
	switch (data_rate) {
	case MSPI_DATA_RATE_SINGLE:
		return false;
	case MSPI_DATA_RATE_S_S_D:
		return phase == MSPI_MCUX_FLEXSPI_PHASE_DATA;
	case MSPI_DATA_RATE_S_D_D:
		return phase != MSPI_MCUX_FLEXSPI_PHASE_COMMAND;
	case MSPI_DATA_RATE_DUAL:
		return true;
	default:
		return false;
	}
}

static int mspi_mcux_flexspi_get_io_cfg(enum mspi_io_mode io_mode,
					struct mspi_mcux_flexspi_io_cfg *io_cfg)
{
	switch (io_mode) {
	case MSPI_IO_MODE_SINGLE:
		io_cfg->cmd_pad = kFLEXSPI_1PAD;
		io_cfg->addr_pad = kFLEXSPI_1PAD;
		io_cfg->data_pad = kFLEXSPI_1PAD;
		return 0;
	case MSPI_IO_MODE_DUAL:
		io_cfg->cmd_pad = kFLEXSPI_2PAD;
		io_cfg->addr_pad = kFLEXSPI_2PAD;
		io_cfg->data_pad = kFLEXSPI_2PAD;
		return 0;
	case MSPI_IO_MODE_DUAL_1_1_2:
		io_cfg->cmd_pad = kFLEXSPI_1PAD;
		io_cfg->addr_pad = kFLEXSPI_1PAD;
		io_cfg->data_pad = kFLEXSPI_2PAD;
		return 0;
	case MSPI_IO_MODE_DUAL_1_2_2:
		io_cfg->cmd_pad = kFLEXSPI_1PAD;
		io_cfg->addr_pad = kFLEXSPI_2PAD;
		io_cfg->data_pad = kFLEXSPI_2PAD;
		return 0;
	case MSPI_IO_MODE_QUAD:
		io_cfg->cmd_pad = kFLEXSPI_4PAD;
		io_cfg->addr_pad = kFLEXSPI_4PAD;
		io_cfg->data_pad = kFLEXSPI_4PAD;
		return 0;
	case MSPI_IO_MODE_QUAD_1_1_4:
		io_cfg->cmd_pad = kFLEXSPI_1PAD;
		io_cfg->addr_pad = kFLEXSPI_1PAD;
		io_cfg->data_pad = kFLEXSPI_4PAD;
		return 0;
	case MSPI_IO_MODE_QUAD_1_4_4:
		io_cfg->cmd_pad = kFLEXSPI_1PAD;
		io_cfg->addr_pad = kFLEXSPI_4PAD;
		io_cfg->data_pad = kFLEXSPI_4PAD;
		return 0;
	case MSPI_IO_MODE_OCTAL:
		io_cfg->cmd_pad = kFLEXSPI_8PAD;
		io_cfg->addr_pad = kFLEXSPI_8PAD;
		io_cfg->data_pad = kFLEXSPI_8PAD;
		return 0;
	case MSPI_IO_MODE_OCTAL_1_1_8:
		io_cfg->cmd_pad = kFLEXSPI_1PAD;
		io_cfg->addr_pad = kFLEXSPI_1PAD;
		io_cfg->data_pad = kFLEXSPI_8PAD;
		return 0;
	case MSPI_IO_MODE_OCTAL_1_8_8:
		io_cfg->cmd_pad = kFLEXSPI_1PAD;
		io_cfg->addr_pad = kFLEXSPI_8PAD;
		io_cfg->data_pad = kFLEXSPI_8PAD;
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int mspi_mcux_flexspi_push_instr(struct mspi_mcux_flexspi_instr *instr, uint8_t *count,
					uint8_t cmd, flexspi_pad_t pad, uint8_t operand)
{
	if (*count >= (MCUX_FLEXSPI_LUT_COUNT * 2U)) {
		return -EINVAL;
	}

	instr[*count].cmd = cmd;
	instr[*count].pad = pad;
	instr[*count].op = operand;
	(*count)++;

	return 0;
}

static void mspi_mcux_flexspi_pack_lut(const struct mspi_mcux_flexspi_instr *instr, uint8_t count,
				       uint32_t *lut)
{
	for (uint8_t lut_idx = 0U; lut_idx < MCUX_FLEXSPI_LUT_COUNT; lut_idx++) {
		const uint8_t op0_idx = lut_idx * 2U;
		const uint8_t op1_idx = op0_idx + 1U;
		struct mspi_mcux_flexspi_instr op0 = {
			.cmd = kFLEXSPI_Command_STOP,
			.pad = kFLEXSPI_1PAD,
			.op = 0U,
		};
		struct mspi_mcux_flexspi_instr op1 = {
			.cmd = kFLEXSPI_Command_STOP,
			.pad = kFLEXSPI_1PAD,
			.op = 0U,
		};

		if (op0_idx < count) {
			op0 = instr[op0_idx];
		}

		if (op1_idx < count) {
			op1 = instr[op1_idx];
		}

		lut[lut_idx] = FLEXSPI_LUT_SEQ(op0.cmd, op0.pad, op0.op, op1.cmd, op1.pad, op1.op);
	}
}

static int mspi_mcux_flexspi_status_to_errno(status_t status)
{
	switch (status) {
	case kStatus_Success:
		return 0;
	case kStatus_FLEXSPI_Busy:
		return -EBUSY;
	case kStatus_FLEXSPI_SequenceExecutionTimeout:
	case kStatus_FLEXSPI_IpCommandGrantTimeout:
		return -ETIMEDOUT;
	default:
		return -EIO;
	}
}

static int mspi_mcux_flexspi_get_port(const struct mspi_dev_cfg *dev_cfg,
				      const struct mspi_dev_id *dev_id, uint8_t *port)
{
	if (dev_cfg->ce_num < kFLEXSPI_PortCount) {
		*port = dev_cfg->ce_num;
		return 0;
	}

	if (dev_id->dev_idx >= kFLEXSPI_PortCount) {
		return -EINVAL;
	}

	*port = dev_id->dev_idx;
	return 0;
}

static int mspi_mcux_flexspi_prepare_lut(const struct device *dev, const struct mspi_xfer *xfer,
					 const struct mspi_xfer_packet *packet, uint32_t *lut)
{
	struct mspi_mcux_flexspi_data *data = dev->data;
	struct mspi_mcux_flexspi_instr instr[MCUX_FLEXSPI_LUT_COUNT * 2U] = {0};
	struct mspi_mcux_flexspi_io_cfg io_cfg;
	uint8_t instr_count = 0U;
	uint8_t cmd_length = xfer->cmd_length ? xfer->cmd_length : data->dev_cfg.cmd_length;
	uint8_t addr_length = xfer->addr_length ? xfer->addr_length : data->dev_cfg.addr_length;
	uint16_t dummy_cycles;
	int ret;

	ret = mspi_mcux_flexspi_get_io_cfg(data->dev_cfg.io_mode, &io_cfg);
	if (ret < 0) {
		return ret;
	}

	if (cmd_length > 1U) {
		return -ENOTSUP;
	}

	if (addr_length > 4U) {
		return -EINVAL;
	}

	if (cmd_length > 0U && packet->cmd > UINT8_MAX) {
		return -ENOTSUP;
	}

	if (packet->dir == MSPI_RX) {
		dummy_cycles = (xfer->rx_dummy != 0U) ? xfer->rx_dummy : data->dev_cfg.rx_dummy;
	} else {
		dummy_cycles = (xfer->tx_dummy != 0U) ? xfer->tx_dummy : data->dev_cfg.tx_dummy;
	}

	if (cmd_length > 0U) {
		ret = mspi_mcux_flexspi_push_instr(
			instr, &instr_count,
			mspi_mcux_flexspi_lut_opcode(
				MSPI_MCUX_FLEXSPI_LUT_KIND_COMMAND,
				mspi_mcux_flexspi_phase_is_ddr(data->dev_cfg.data_rate,
							       MSPI_MCUX_FLEXSPI_PHASE_COMMAND)),
			io_cfg.cmd_pad, (uint8_t)packet->cmd);
		if (ret < 0) {
			return ret;
		}
	}

	if (addr_length > 0U) {
		ret = mspi_mcux_flexspi_push_instr(
			instr, &instr_count,
			mspi_mcux_flexspi_lut_opcode(
				MSPI_MCUX_FLEXSPI_LUT_KIND_ADDRESS,
				mspi_mcux_flexspi_phase_is_ddr(data->dev_cfg.data_rate,
							       MSPI_MCUX_FLEXSPI_PHASE_ADDRESS)),
			io_cfg.addr_pad, addr_length * 8U);
		if (ret < 0) {
			return ret;
		}
	}

	if (dummy_cycles > 0U) {
		ret = mspi_mcux_flexspi_push_instr(
			instr, &instr_count,
			mspi_mcux_flexspi_lut_opcode(
				MSPI_MCUX_FLEXSPI_LUT_KIND_DUMMY,
				mspi_mcux_flexspi_phase_is_ddr(data->dev_cfg.data_rate,
							       MSPI_MCUX_FLEXSPI_PHASE_DATA)),
			io_cfg.data_pad, (uint8_t)MIN(dummy_cycles, UINT8_MAX));
		if (ret < 0) {
			return ret;
		}
	}

	if (packet->num_bytes > 0U) {
		ret = mspi_mcux_flexspi_push_instr(
			instr, &instr_count,
			mspi_mcux_flexspi_lut_opcode(
				packet->dir == MSPI_RX ? MSPI_MCUX_FLEXSPI_LUT_KIND_READ
						       : MSPI_MCUX_FLEXSPI_LUT_KIND_WRITE,
				mspi_mcux_flexspi_phase_is_ddr(data->dev_cfg.data_rate,
							       MSPI_MCUX_FLEXSPI_PHASE_DATA)),
			io_cfg.data_pad, MCUX_FLEXSPI_DATA_LUT_OPERAND);
		if (ret < 0) {
			return ret;
		}
	}

	ret = mspi_mcux_flexspi_push_instr(instr, &instr_count, kFLEXSPI_Command_STOP,
					   kFLEXSPI_1PAD, 0U);
	if (ret < 0) {
		return ret;
	}

	mspi_mcux_flexspi_pack_lut(instr, instr_count, lut);

	return 0;
}

static int mspi_mcux_flexspi_update_clock(const struct device *dev,
					  flexspi_device_config_t *flash_cfg, flexspi_port_t port,
					  uint32_t target_hz)
{
	const struct mspi_mcux_flexspi_config *cfg = dev->config;
	uint32_t root_hz;
	uint32_t divider;
	uint32_t active_hz;
	uint32_t key;
	uint32_t clk_backup;
	int ret;

	ret = clock_control_get_rate(cfg->clock_dev, cfg->clock_subsys, &root_hz);
	if (ret < 0) {
		LOG_ERR("Failed to get FlexSPI root clock: %d", ret);
		return ret;
	}

	target_hz = MIN(target_hz, flash_cfg->flexspiRootClk);

	divider = (cfg->base->MCR0 & FLEXSPI_MCR0_SERCLKDIV_MASK) >> FLEXSPI_MCR0_SERCLKDIV_SHIFT;
	active_hz = root_hz / (divider + 1U);
	if (active_hz == target_hz) {
		return 0;
	}

	key = irq_lock();

	while (!FLEXSPI_GetBusIdleStatus(cfg->base)) {
	}

	FLEXSPI_Enable(cfg->base, false);

	divider = ((root_hz + (target_hz - 1U)) / target_hz) - 1U;
	divider = MIN(divider, FLEXSPI_MCR0_SERCLKDIV_MASK >> FLEXSPI_MCR0_SERCLKDIV_SHIFT);

	cfg->base->MCR0 &= ~FLEXSPI_MCR0_SERCLKDIV_MASK;
	cfg->base->MCR0 |= FLEXSPI_MCR0_SERCLKDIV(divider);

	clk_backup = flash_cfg->flexspiRootClk;
	flash_cfg->flexspiRootClk = root_hz / (divider + 1U);
	FLEXSPI_UpdateDllValue(cfg->base, flash_cfg, port);
	flash_cfg->flexspiRootClk = clk_backup;

	FLEXSPI_Enable(cfg->base, true);
	FLEXSPI_SoftwareReset(cfg->base);

	irq_unlock(key);

	return 0;
}

static int mspi_mcux_flexspi_apply_dev_cfg(const struct device *dev,
					   const struct mspi_dev_cfg *dev_cfg)
{
	const struct mspi_mcux_flexspi_config *cfg = dev->config;
	struct mspi_mcux_flexspi_data *data = dev->data;
	flexspi_device_config_t flash_cfg = {
		.flexspiRootClk = dev_cfg->freq ? dev_cfg->freq : data->mspicfg.max_freq,
		.isSck2Enabled = false,
		.flashSize = 0U,
		.CSIntervalUnit = kFLEXSPI_CsIntervalUnit1SckCycle,
		.CSInterval = 0U,
		.CSHoldTime = 3U,
		.CSSetupTime = 3U,
		.dataValidTime = 0U,
		.columnspace = 0U,
		.enableWordAddress = false,
		.AWRSeqIndex = MCUX_FLEXSPI_LUT_INDEX,
		.AWRSeqNumber = MCUX_FLEXSPI_LUT_SEQ_NUM,
		.ARDSeqIndex = MCUX_FLEXSPI_LUT_INDEX,
		.ARDSeqNumber = MCUX_FLEXSPI_LUT_SEQ_NUM,
		.AHBWriteWaitUnit = kFLEXSPI_AhbWriteWaitUnit2AhbCycle,
		.AHBWriteWaitInterval = 0U,
		.enableWriteMask = false,
	};
	uint8_t port;
	int ret;

	ret = mspi_mcux_flexspi_get_port(dev_cfg, data->dev_id, &port);
	if (ret < 0) {
		return ret;
	}

	if (cfg->flash_size_kb[port] != 0U) {
		flash_cfg.flashSize = cfg->flash_size_kb[port];
	} else {
		flash_cfg.flashSize = FLEXSPI_FLSHCR0_FLSHSZ_MASK;
	}

	if (mspi_mcux_flexspi_running_from_xip_window(cfg)) {
		LOG_DBG("Skip FlexSPI reconfiguration while executing in XIP window");
		return 0;
	}

	ret = mspi_mcux_flexspi_update_clock(dev, &flash_cfg, (flexspi_port_t)port,
					     flash_cfg.flexspiRootClk);
	if (ret < 0) {
		return ret;
	}

	FLEXSPI_SetFlashConfig(cfg->base, &flash_cfg, (flexspi_port_t)port);
	FLEXSPI_SoftwareReset(cfg->base);

	return 0;
}

static int mspi_mcux_flexspi_check_controller_cfg(const struct mspi_cfg *fixed_cfg,
						  const struct mspi_cfg *cfg)
{
	if (cfg->op_mode != MSPI_OP_MODE_CONTROLLER) {
		return -ENOTSUP;
	}

	if (cfg->duplex != MSPI_HALF_DUPLEX) {
		return -ENOTSUP;
	}

	if (cfg->num_periph > kFLEXSPI_PortCount) {
		return -ENOTSUP;
	}

	if (cfg->max_freq > fixed_cfg->max_freq) {
		return -ENOTSUP;
	}

	if (cfg->dqs_support && !fixed_cfg->dqs_support) {
		return -ENOTSUP;
	}

	return 0;
}

static int mspi_mcux_flexspi_check_dev_cfg(const struct mspi_mcux_flexspi_data *data,
					   enum mspi_dev_cfg_mask param_mask,
					   const struct mspi_dev_cfg *cfg)
{
	struct mspi_mcux_flexspi_io_cfg io_cfg;

	if (param_mask == MSPI_DEVICE_CONFIG_NONE) {
		return 0;
	}

	if (param_mask > MSPI_DEVICE_CONFIG_ALL) {
		return -EINVAL;
	}

	if ((param_mask & MSPI_DEVICE_CONFIG_CE_NUM) != 0U && cfg->ce_num >= kFLEXSPI_PortCount) {
		return -EINVAL;
	}

	if ((param_mask & MSPI_DEVICE_CONFIG_FREQUENCY) != 0U &&
	    cfg->freq > data->mspicfg.max_freq) {
		return -ENOTSUP;
	}

	if ((param_mask & MSPI_DEVICE_CONFIG_IO_MODE) != 0U &&
	    mspi_mcux_flexspi_get_io_cfg(cfg->io_mode, &io_cfg) < 0) {
		return -ENOTSUP;
	}

	if ((param_mask & MSPI_DEVICE_CONFIG_DATA_RATE) != 0U &&
	    cfg->data_rate >= MSPI_DATA_RATE_MAX) {
		return -EINVAL;
	}

	if ((param_mask & MSPI_DEVICE_CONFIG_CPP) != 0U && cfg->cpp != MSPI_CPP_MODE_0) {
		return -ENOTSUP;
	}

	if ((param_mask & MSPI_DEVICE_CONFIG_ENDIAN) != 0U && cfg->endian > MSPI_XFER_BIG_ENDIAN) {
		return -EINVAL;
	}

	if ((param_mask & MSPI_DEVICE_CONFIG_CE_POL) != 0U &&
	    cfg->ce_polarity != MSPI_CE_ACTIVE_LOW) {
		return -ENOTSUP;
	}

	if ((param_mask & MSPI_DEVICE_CONFIG_DQS) != 0U && cfg->dqs_enable &&
	    !data->mspicfg.dqs_support) {
		return -ENOTSUP;
	}

	if ((param_mask & MSPI_DEVICE_CONFIG_CMD_LEN) != 0U && cfg->cmd_length > 1U) {
		return -ENOTSUP;
	}

	if ((param_mask & MSPI_DEVICE_CONFIG_ADDR_LEN) != 0U && cfg->addr_length > 4U) {
		return -EINVAL;
	}

	return 0;
}

static void mspi_mcux_flexspi_merge_dev_cfg(struct mspi_dev_cfg *dst,
					    enum mspi_dev_cfg_mask param_mask,
					    const struct mspi_dev_cfg *src)
{
	if (param_mask == MSPI_DEVICE_CONFIG_ALL) {
		*dst = *src;
		return;
	}

	if ((param_mask & MSPI_DEVICE_CONFIG_CE_NUM) != 0U) {
		dst->ce_num = src->ce_num;
	}
	if ((param_mask & MSPI_DEVICE_CONFIG_FREQUENCY) != 0U) {
		dst->freq = src->freq;
	}
	if ((param_mask & MSPI_DEVICE_CONFIG_IO_MODE) != 0U) {
		dst->io_mode = src->io_mode;
	}
	if ((param_mask & MSPI_DEVICE_CONFIG_DATA_RATE) != 0U) {
		dst->data_rate = src->data_rate;
	}
	if ((param_mask & MSPI_DEVICE_CONFIG_CPP) != 0U) {
		dst->cpp = src->cpp;
	}
	if ((param_mask & MSPI_DEVICE_CONFIG_ENDIAN) != 0U) {
		dst->endian = src->endian;
	}
	if ((param_mask & MSPI_DEVICE_CONFIG_CE_POL) != 0U) {
		dst->ce_polarity = src->ce_polarity;
	}
	if ((param_mask & MSPI_DEVICE_CONFIG_DQS) != 0U) {
		dst->dqs_enable = src->dqs_enable;
	}
	if ((param_mask & MSPI_DEVICE_CONFIG_RX_DUMMY) != 0U) {
		dst->rx_dummy = src->rx_dummy;
	}
	if ((param_mask & MSPI_DEVICE_CONFIG_TX_DUMMY) != 0U) {
		dst->tx_dummy = src->tx_dummy;
	}
	if ((param_mask & MSPI_DEVICE_CONFIG_READ_CMD) != 0U) {
		dst->read_cmd = src->read_cmd;
	}
	if ((param_mask & MSPI_DEVICE_CONFIG_WRITE_CMD) != 0U) {
		dst->write_cmd = src->write_cmd;
	}
	if ((param_mask & MSPI_DEVICE_CONFIG_CMD_LEN) != 0U) {
		dst->cmd_length = src->cmd_length;
	}
	if ((param_mask & MSPI_DEVICE_CONFIG_ADDR_LEN) != 0U) {
		dst->addr_length = src->addr_length;
	}
	if ((param_mask & MSPI_DEVICE_CONFIG_MEM_BOUND) != 0U) {
		dst->mem_boundary = src->mem_boundary;
	}
	if ((param_mask & MSPI_DEVICE_CONFIG_BREAK_TIME) != 0U) {
		dst->time_to_break = src->time_to_break;
	}
}

static int mspi_mcux_flexspi_hw_init(const struct device *dev, const struct mspi_cfg *cfg)
{
	const struct mspi_mcux_flexspi_config *mcux_cfg = dev->config;
	flexspi_config_t flexspi_cfg;
	int ret;

	ret = pinctrl_apply_state(mcux_cfg->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0 && ret != -ENOENT) {
		return ret;
	}

	FLEXSPI_GetDefaultConfig(&flexspi_cfg);

	flexspi_cfg.ahbConfig.enableAHBBufferable = mcux_cfg->ahb_bufferable;
	flexspi_cfg.ahbConfig.enableAHBCachable = mcux_cfg->ahb_cacheable;
	flexspi_cfg.ahbConfig.enableAHBPrefetch = mcux_cfg->ahb_prefetch;
	flexspi_cfg.ahbConfig.enableReadAddressOpt = mcux_cfg->ahb_read_addr_opt;
#if MCUX_FLEXSPI_HAS_MCR0_COMBINATIONEN
	flexspi_cfg.enableCombination = mcux_cfg->combination_mode;
#endif
#if MCUX_FLEXSPI_HAS_MCR2_SCKBDIFFOPT
	flexspi_cfg.enableSckBDiffOpt = mcux_cfg->sck_differential_clock;
#endif
	flexspi_cfg.rxSampleClock = mcux_cfg->rx_sample_clock;
#if MCUX_FLEXSPI_HAS_RXCLKSRC_PORTB
	flexspi_cfg.rxSampleClockPortB = mcux_cfg->rx_sample_clock_b;
#if defined(FSL_FEATURE_FLEXSPI_SUPPORT_RXCLKSRC_DIFF) && FSL_FEATURE_FLEXSPI_SUPPORT_RXCLKSRC_DIFF
	if (flexspi_cfg.rxSampleClock != flexspi_cfg.rxSampleClockPortB) {
		flexspi_cfg.rxSampleClockDiff = true;
	}
#endif
#endif

	__ASSERT_NO_MSG(mcux_cfg->buf_cfg_cnt < FSL_FEATURE_FLEXSPI_AHB_BUFFER_COUNT);
	for (uint8_t i = 0U; i < mcux_cfg->buf_cfg_cnt; i++) {
		flexspi_cfg.ahbConfig.buffer[i].enablePrefetch = mcux_cfg->buf_cfg[i].prefetch;
		flexspi_cfg.ahbConfig.buffer[i].priority = mcux_cfg->buf_cfg[i].priority;
		flexspi_cfg.ahbConfig.buffer[i].masterIndex = mcux_cfg->buf_cfg[i].master_id;
		flexspi_cfg.ahbConfig.buffer[i].bufferSize = mcux_cfg->buf_cfg[i].buf_size;
	}

	if (mspi_mcux_flexspi_running_from_xip_window(mcux_cfg)) {
		LOG_DBG("Skip FlexSPI HW init while executing in XIP window");
		ARG_UNUSED(cfg);
		return 0;
	}

	FLEXSPI_Init(mcux_cfg->base, &flexspi_cfg);

#if defined(FLEXSPI_AHBCR_ALIGNMENT_MASK)
	mcux_cfg->base->AHBCR = (mcux_cfg->base->AHBCR & ~FLEXSPI_AHBCR_ALIGNMENT_MASK) |
				FLEXSPI_AHBCR_ALIGNMENT(mcux_cfg->ahb_boundary);
#endif

	return 0;
}

static int mspi_mcux_flexspi_config(const struct mspi_dt_spec *spec)
{
	const struct device *controller = spec->bus;
	const struct mspi_mcux_flexspi_config *cfg = controller->config;
	struct mspi_mcux_flexspi_data *data = controller->data;
	int ret;

	ret = mspi_mcux_flexspi_check_controller_cfg(&cfg->mspicfg, &spec->config);
	if (ret < 0) {
		return ret;
	}

	data->mspicfg = spec->config;

	if (!spec->config.re_init) {
		return 0;
	}

	if (k_sem_take(&data->cfg_lock, K_MSEC(CONFIG_MSPI_COMPLETION_TIMEOUT_TOLERANCE)) < 0) {
		return -EBUSY;
	}

	ret = mspi_mcux_flexspi_hw_init(controller, &data->mspicfg);

	data->dev_id = NULL;
	k_sem_give(&data->cfg_lock);

	return ret;
}

static int mspi_mcux_flexspi_dev_config(const struct device *controller,
					const struct mspi_dev_id *dev_id,
					enum mspi_dev_cfg_mask param_mask,
					const struct mspi_dev_cfg *dev_cfg)
{
	const struct mspi_mcux_flexspi_config *cfg = controller->config;
	struct mspi_mcux_flexspi_data *data = controller->data;
	struct mspi_dev_cfg next_cfg = data->dev_cfg;
	bool lock_taken = false;
	int ret;

	if (data->dev_id != dev_id) {
		ret = k_sem_take(&data->cfg_lock, K_MSEC(CONFIG_MSPI_COMPLETION_TIMEOUT_TOLERANCE));
		if (ret < 0) {
			return -EBUSY;
		}
		lock_taken = true;
	}

	if ((param_mask == MSPI_DEVICE_CONFIG_NONE) && !cfg->mspicfg.sw_multi_periph) {
		data->dev_id = dev_id;
		return 0;
	}

	ret = mspi_mcux_flexspi_check_dev_cfg(data, param_mask, dev_cfg);
	if (ret < 0) {
		goto error;
	}

	data->dev_id = dev_id;
	mspi_mcux_flexspi_merge_dev_cfg(&next_cfg, param_mask, dev_cfg);

	if (next_cfg.ce_num >= kFLEXSPI_PortCount && dev_id->dev_idx < kFLEXSPI_PortCount) {
		next_cfg.ce_num = dev_id->dev_idx;
	}

	ret = mspi_mcux_flexspi_apply_dev_cfg(controller, &next_cfg);
	if (ret < 0) {
		goto error;
	}

	data->dev_cfg = next_cfg;
	return 0;

error:
	if (lock_taken) {
		data->dev_id = NULL;
		k_sem_give(&data->cfg_lock);
	}

	return ret;
}

static int mspi_mcux_flexspi_get_channel_status(const struct device *controller, uint8_t channel)
{
	const struct mspi_mcux_flexspi_config *cfg = controller->config;
	struct mspi_mcux_flexspi_data *data = controller->data;

	ARG_UNUSED(channel);

	if (!FLEXSPI_GetBusIdleStatus(cfg->base)) {
		return -EBUSY;
	}

	data->dev_id = NULL;
	if (!k_sem_count_get(&data->cfg_lock)) {
		k_sem_give(&data->cfg_lock);
	}

	return 0;
}

static int mspi_mcux_flexspi_transceive(const struct device *controller,
					const struct mspi_dev_id *dev_id,
					const struct mspi_xfer *xfer)
{
	const struct mspi_mcux_flexspi_config *cfg = controller->config;
	struct mspi_mcux_flexspi_data *data = controller->data;
	uint32_t lut[MCUX_FLEXSPI_LUT_COUNT] = {0};
	uint8_t port;
	int ret;

	if (data->dev_id != dev_id) {
		return -ESTALE;
	}

	if (xfer->async) {
		return -ENOTSUP;
	}

	if (xfer->xfer_mode == MSPI_DMA) {
		return -ENOTSUP;
	}

	if ((xfer->num_packet == 0U) || (xfer->packets == NULL)) {
		return -EINVAL;
	}

	if (xfer->ce_sw_ctrl.gpio.port != NULL) {
		return -ENOTSUP;
	}

	ret = mspi_mcux_flexspi_get_port(&data->dev_cfg, dev_id, &port);
	if (ret < 0) {
		return ret;
	}

	ret = k_mutex_lock(&data->xfer_lock, K_FOREVER);
	if (ret < 0) {
		return -EBUSY;
	}

	for (uint32_t idx = 0U; idx < xfer->num_packet; idx++) {
		const struct mspi_xfer_packet *packet = &xfer->packets[idx];
		flexspi_transfer_t transfer = {0};
		status_t status;

		if ((packet->num_bytes > 0U) && (packet->data_buf == NULL)) {
			ret = -EINVAL;
			break;
		}

		if ((cfg->packet_data_limit != 0U) &&
		    (packet->num_bytes > cfg->packet_data_limit)) {
			ret = -EINVAL;
			break;
		}

		ret = mspi_mcux_flexspi_prepare_lut(controller, xfer, packet, lut);
		if (ret < 0) {
			break;
		}

		FLEXSPI_UpdateLUT(cfg->base, MCUX_FLEXSPI_LUT_INDEX, lut, MCUX_FLEXSPI_LUT_COUNT);

		transfer.port = (flexspi_port_t)port;
		transfer.seqIndex = MCUX_FLEXSPI_LUT_INDEX;
		transfer.SeqNumber = MCUX_FLEXSPI_LUT_SEQ_NUM;
		transfer.deviceAddress =
			packet->address + mspi_mcux_flexspi_get_port_offset(cfg, port);
		transfer.data = (uint32_t *)packet->data_buf;
		transfer.dataSize = packet->num_bytes;

		if (packet->num_bytes == 0U) {
			transfer.cmdType = kFLEXSPI_Command;
		} else if (packet->dir == MSPI_RX) {
			transfer.cmdType = kFLEXSPI_Read;
		} else if (packet->dir == MSPI_TX) {
			transfer.cmdType = kFLEXSPI_Write;
		} else {
			ret = -EINVAL;
			break;
		}

		status = FLEXSPI_TransferBlocking(cfg->base, &transfer);
		ret = mspi_mcux_flexspi_status_to_errno(status);
		if (ret < 0) {
			break;
		}
	}

	k_mutex_unlock(&data->xfer_lock);
	return ret;
}

static int mspi_mcux_flexspi_xip_config(const struct device *controller,
					const struct mspi_dev_id *dev_id,
					const struct mspi_xip_cfg *xip_cfg)
{
	struct mspi_mcux_flexspi_data *data = controller->data;

	if (data->dev_id != dev_id) {
		return -ESTALE;
	}

	if (xip_cfg->enable) {
		return -ENOTSUP;
	}

	return 0;
}

static DEVICE_API(mspi, mspi_mcux_flexspi_driver_api) = {
	.config = mspi_mcux_flexspi_config,
	.dev_config = mspi_mcux_flexspi_dev_config,
	.get_channel_status = mspi_mcux_flexspi_get_channel_status,
	.transceive = mspi_mcux_flexspi_transceive,
	.xip_config = mspi_mcux_flexspi_xip_config,
};

#ifdef CONFIG_PM_DEVICE
static int mspi_mcux_flexspi_pm_action(const struct device *dev, enum pm_device_action action)
{
	const struct mspi_mcux_flexspi_config *cfg = dev->config;
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		ret = pinctrl_apply_state(cfg->pincfg, PINCTRL_STATE_DEFAULT);
		if (ret < 0 && ret != -ENOENT) {
			return ret;
		}
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		ret = pinctrl_apply_state(cfg->pincfg, PINCTRL_STATE_SLEEP);
		if (ret < 0 && ret != -ENOENT) {
			return ret;
		}
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif

static int mspi_mcux_flexspi_init(const struct device *dev)
{
	const struct mspi_mcux_flexspi_config *cfg = dev->config;
	struct mspi_mcux_flexspi_data *data = dev->data;
	const struct mspi_dt_spec spec = {
		.bus = dev,
		.config = cfg->mspicfg,
	};
	int ret;

	if (!device_is_ready(cfg->clock_dev)) {
		return -ENODEV;
	}

	ret = mspi_mcux_flexspi_hw_init(dev, &cfg->mspicfg);
	if (ret < 0) {
		return ret;
	}

	k_sem_init(&data->cfg_lock, 1, 1);
	k_mutex_init(&data->xfer_lock);

	data->mspicfg = cfg->mspicfg;
	data->dev_cfg = (struct mspi_dev_cfg){
		.ce_num = 0U,
		.freq = cfg->mspicfg.max_freq,
		.io_mode = MSPI_IO_MODE_SINGLE,
		.data_rate = MSPI_DATA_RATE_SINGLE,
		.cpp = MSPI_CPP_MODE_0,
		.endian = MSPI_XFER_BIG_ENDIAN,
		.ce_polarity = MSPI_CE_ACTIVE_LOW,
		.dqs_enable = false,
		.cmd_length = 1U,
		.addr_length = 3U,
	};
	data->dev_id = NULL;

	return mspi_mcux_flexspi_config(&spec);
}

#if MCUX_FLEXSPI_HAS_RXCLKSRC_PORTB
#define MCUX_FLEXSPI_RXCLK_B(inst) .rx_sample_clock_b = DT_INST_PROP(inst, rx_clock_source_b),
#else
#define MCUX_FLEXSPI_RXCLK_B(inst)
#endif

#define MCUX_FLEXSPI_PORT_SIZE_INIT(child)                                                         \
	[DT_REG_ADDR(child)] = DT_PROP_OR(child, size, 0) / (8U * 1024U),

#define MCUX_FLEXSPI_CFG(inst)                                                                     \
	{                                                                                          \
		.channel_num = inst,                                                               \
		.op_mode = DT_INST_ENUM_IDX_OR(inst, op_mode, MSPI_OP_MODE_CONTROLLER),            \
		.duplex = DT_INST_ENUM_IDX_OR(inst, duplex, MSPI_HALF_DUPLEX),                     \
		.dqs_support = DT_INST_PROP(inst, dqs_support),                                    \
		.sw_multi_periph = DT_INST_PROP(inst, software_multiperipheral),                   \
		.ce_group = ce_gpios_##inst,                                                       \
		.num_ce_gpios = ARRAY_SIZE(ce_gpios_##inst),                                       \
		.num_periph = DT_INST_CHILD_NUM(inst),                                             \
		.max_freq = DT_INST_PROP(inst, clock_frequency),                                   \
	}

#define MSPI_MCUX_FLEXSPI_INIT(inst)                                                               \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
	static uint16_t buf_cfg_##inst[] = DT_INST_PROP_OR(inst, rx_buffer_config, {0});           \
	static struct gpio_dt_spec ce_gpios_##inst[] = MSPI_CE_GPIOS_DT_SPEC_INST_GET(inst);       \
	static const uint32_t flash_size_kb_##inst[kFLEXSPI_PortCount] = {                         \
		DT_FOREACH_CHILD_STATUS_OKAY(DT_DRV_INST(inst), MCUX_FLEXSPI_PORT_SIZE_INIT)};     \
	static const struct mspi_mcux_flexspi_config mspi_mcux_flexspi_cfg_##inst = {              \
		.base = (FLEXSPI_Type *)DT_INST_REG_ADDR(inst),                                    \
		.ahb_base = DT_INST_REG_ADDR_BY_IDX(inst, 1),                                      \
		.ahb_size = DT_INST_REG_SIZE_BY_IDX(inst, 1),                                      \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                    \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),                             \
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(inst, name),           \
		.flash_size_kb = flash_size_kb_##inst,                                             \
		.mspicfg = MCUX_FLEXSPI_CFG(inst),                                                 \
		.buf_cfg = (struct mspi_mcux_flexspi_buf_cfg *)buf_cfg_##inst,                     \
		.buf_cfg_cnt = sizeof(buf_cfg_##inst) / sizeof(struct mspi_mcux_flexspi_buf_cfg),  \
		.ahb_boundary = DT_INST_ENUM_IDX(inst, ahb_boundary),                              \
		.ahb_bufferable = DT_INST_PROP(inst, ahb_bufferable),                              \
		.ahb_cacheable = DT_INST_PROP(inst, ahb_cacheable),                                \
		.ahb_prefetch = DT_INST_PROP(inst, ahb_prefetch),                                  \
		.ahb_read_addr_opt = DT_INST_PROP(inst, ahb_read_addr_opt),                        \
		.combination_mode = DT_INST_PROP(inst, combination_mode),                          \
		.sck_differential_clock = DT_INST_PROP(inst, sck_differential_clock),              \
		.rx_sample_clock = DT_INST_PROP(inst, rx_clock_source),                            \
		MCUX_FLEXSPI_RXCLK_B(inst).packet_data_limit =                                     \
			DT_INST_PROP_OR(inst, packet_data_limit, 0),                               \
	};                                                                                         \
	static struct mspi_mcux_flexspi_data mspi_mcux_flexspi_data_##inst;                        \
	PM_DEVICE_DT_INST_DEFINE(inst, mspi_mcux_flexspi_pm_action);                               \
	DEVICE_DT_INST_DEFINE(inst, mspi_mcux_flexspi_init, PM_DEVICE_DT_INST_GET(inst),           \
			      &mspi_mcux_flexspi_data_##inst, &mspi_mcux_flexspi_cfg_##inst,       \
			      POST_KERNEL, CONFIG_MSPI_INIT_PRIORITY,                              \
			      &mspi_mcux_flexspi_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MSPI_MCUX_FLEXSPI_INIT)
