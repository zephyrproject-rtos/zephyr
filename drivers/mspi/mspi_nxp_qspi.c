/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_qspi

#include <zephyr/drivers/mspi.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include "fsl_qspi.h"
#if defined(CONFIG_SOC_SERIES_MCXE24X) || defined(CONFIG_SOC_SERIES_MCXE31X)
#include "fsl_qspi_soc.h"
#endif
#include "fsl_clock.h"

LOG_MODULE_REGISTER(mspi_nxp_qspi, CONFIG_MSPI_LOG_LEVEL);

#define MSPI_NXP_QSPI_LUT_SEQ_IDX       0
#define MSPI_NXP_QSPI_LUT_SEQ_UNIT      FSL_FEATURE_QSPI_LUT_SEQ_UNIT
#define MSPI_NXP_QSPI_LUT_SEQ_INSTR_MAX (MSPI_NXP_QSPI_LUT_SEQ_UNIT * 2)
/* Max words per IP write command — covers NOR flash max page size (256B) */
#define MSPI_NXP_QSPI_MAX_TX_WORDS      (256 / 4)

/*
 * MCXE24X QSPI IP returns data with first flash byte in MSB of the 32-bit
 * IPS register, requiring byte-swap on LE ARM. MCR[END_CFG] exists in the
 * register map but is non-functional. MCXE31X QSPI IP has no END_CFG field
 * and uses native LE order.
 */
#if defined(CONFIG_SOC_SERIES_MCXE24X)
#define MSPI_NXP_QSPI_FIFO_BSWAP
#endif

struct mspi_nxp_qspi_config {
	QuadSPI_Type *base;
	uint32_t amba_base;
	uint32_t amba_size;
	const struct pinctrl_dev_config *pcfg;
	const struct device *clock_gate_dev;
	clock_control_subsys_t clock_gate_subsys;
	const struct device *clock_src_dev;
	clock_control_subsys_t clock_src_subsys;
	struct mspi_cfg mspicfg;
	uint8_t cs_setup_time;
	uint8_t cs_hold_time;
	uint8_t data_hold_time;
};

struct mspi_nxp_qspi_data {
	struct mspi_dev_cfg dev_cfg;
	const struct mspi_dev_id *dev_id;
	struct k_mutex lock;
	enum mspi_io_mode cur_io_mode;
};

static uint8_t mspi_nxp_qspi_io_mode_to_pad(enum mspi_io_mode mode)
{
	switch (mode) {
	case MSPI_IO_MODE_QUAD_1_1_4:
		return QSPI_PAD_4;
	default:
		return QSPI_PAD_1;
	}
}

static int mspi_nxp_qspi_setup_lut(QuadSPI_Type *base, int seq_idx,
				      const struct mspi_xfer *xfer,
				      const struct mspi_xfer_packet *pkt,
				      enum mspi_io_mode io_mode)
{
	uint32_t lut_base = seq_idx * MSPI_NXP_QSPI_LUT_SEQ_UNIT;
	uint32_t lut[MSPI_NXP_QSPI_LUT_SEQ_UNIT];
	uint8_t cmd[MSPI_NXP_QSPI_LUT_SEQ_INSTR_MAX];
	uint8_t operand[MSPI_NXP_QSPI_LUT_SEQ_INSTR_MAX];
	uint8_t pad[MSPI_NXP_QSPI_LUT_SEQ_INSTR_MAX];
	uint16_t dummy;
	uint8_t n = 0;

	cmd[n] = QSPI_CMD;
	pad[n] = QSPI_PAD_1;
	operand[n] = (uint8_t)(pkt->cmd & 0xFF);
	n++;

	if (xfer->addr_length > 0) {
		cmd[n] = QSPI_ADDR;
		pad[n] = QSPI_PAD_1;
		operand[n] = xfer->addr_length * 8;
		n++;
	}

	dummy = (pkt->dir == MSPI_RX) ? xfer->rx_dummy : xfer->tx_dummy;

	if (dummy > 0) {
		cmd[n] = QSPI_DUMMY;
		pad[n] = mspi_nxp_qspi_io_mode_to_pad(io_mode);
		operand[n] = (uint8_t)dummy;
		n++;
	}

	if (pkt->num_bytes > 0) {
		cmd[n] = (pkt->dir == MSPI_RX) ? QSPI_READ : QSPI_WRITE;
		pad[n] = mspi_nxp_qspi_io_mode_to_pad(io_mode);
		operand[n] = 1;
		n++;
	}

	cmd[n] = QSPI_STOP;
	pad[n] = 0;
	operand[n] = 0;
	n++;

	if (n > MSPI_NXP_QSPI_LUT_SEQ_INSTR_MAX) {
		LOG_ERR("LUT sequence too long: %d instructions", n);
		return -EINVAL;
	}

	memset(lut, 0, sizeof(lut));
	for (int i = 0; i < n; i += 2) {
		uint8_t cmd0 = cmd[i];
		uint8_t pad0 = pad[i];
		uint8_t op0 = operand[i];
		uint8_t cmd1 = (i + 1 < n) ? cmd[i + 1] : QSPI_STOP;
		uint8_t pad1 = (i + 1 < n) ? pad[i + 1] : 0;
		uint8_t op1 = (i + 1 < n) ? operand[i + 1] : 0;

		lut[i / 2] = QSPI_LUT_SEQ(cmd0, pad0, op0, cmd1, pad1, op1);
	}

	base->LUTKEY = 0x5AF05AF0U;
	base->LCKCR = 0x2U;
	for (int i = 0; i < MSPI_NXP_QSPI_LUT_SEQ_UNIT; i++) {
		base->LUT[lut_base + i] = lut[i];
	}
	base->LUTKEY = 0x5AF05AF0U;
	base->LCKCR = 0x1U;

	return 0;
}

static int mspi_nxp_qspi_ip_read(QuadSPI_Type *base, uint32_t addr, uint8_t seq_idx,
			 void *buf, uint32_t len)
{
	uint32_t rx_fifo_size = 4U * FSL_FEATURE_QSPI_RXFIFO_DEPTH;
	uint32_t left = len;
	uint8_t *dst = buf;

	for (uint32_t off = 0; left > 0; off += rx_fifo_size) {
		uint32_t chunk = MIN(left, rx_fifo_size);
		uint32_t aligned = (chunk + 3U) & ~3U;
		uint32_t tmp[FSL_FEATURE_QSPI_RXFIFO_DEPTH];

		QSPI_ClearFifo(base, kQSPI_RxFifo);
		QSPI_SetIPCommandSize(base, chunk);
		QSPI_SetIPCommandAddress(base, addr + off);
		QSPI_ExecuteIPCommand(base, seq_idx);
		while (base->SR & QuadSPI_SR_BUSY_MASK) {
		}

		QSPI_ReadBlocking(base, tmp, aligned);
#ifdef MSPI_NXP_QSPI_FIFO_BSWAP
		for (uint32_t w = 0; w < aligned / 4U; w++) {
			tmp[w] = __builtin_bswap32(tmp[w]);
		}
#endif
		memcpy(dst, tmp, chunk);
		dst += chunk;
		left -= chunk;
	}

	return 0;
}

static int mspi_nxp_qspi_ip_write(QuadSPI_Type *base, uint32_t addr, uint8_t seq_idx,
			  const void *buf, uint32_t len)
{
	uint32_t aligned_size = MAX((len + 3U) & ~3U, 16U);
	uint32_t total_words = aligned_size / 4U;
	uint32_t prefill = MIN(total_words, FSL_FEATURE_QSPI_TXFIFO_DEPTH);
	uint32_t tx_words[MSPI_NXP_QSPI_MAX_TX_WORDS];

	if (total_words > MSPI_NXP_QSPI_MAX_TX_WORDS) {
		LOG_ERR("Write too large: %u bytes (max %u)",
			len, MSPI_NXP_QSPI_MAX_TX_WORDS * 4U);
		return -EINVAL;
	}

	/* Pre-build all TX words to minimize per-word overhead during transfer */
	memset(tx_words, 0xFF, aligned_size);
	memcpy(tx_words, buf, len);
#ifdef MSPI_NXP_QSPI_FIFO_BSWAP
	for (uint32_t i = 0; i < total_words; i++) {
		tx_words[i] = __builtin_bswap32(tx_words[i]);
	}
#endif

	QSPI_ClearFifo(base, kQSPI_TxFifo);

	for (uint32_t i = 0; i < prefill; i++) {
		while (QSPI_GetStatusFlags(base) & (uint32_t)kQSPI_TxBufferFull) {
		}
		base->TBDR = tx_words[i];
	}

	QSPI_SetIPCommandSize(base, aligned_size);
	QSPI_SetIPCommandAddress(base, addr);
	QSPI_ExecuteIPCommand(base, seq_idx);

	for (uint32_t i = prefill; i < total_words; i++) {
		while (QSPI_GetStatusFlags(base) & (uint32_t)kQSPI_TxBufferFull) {
		}
		base->TBDR = tx_words[i];
	}

	while (base->SR & QuadSPI_SR_BUSY_MASK) {
	}

	return 0;
}

static int mspi_nxp_qspi_ip_cmd(QuadSPI_Type *base, uint32_t addr, uint8_t seq_idx)
{
	QSPI_SetIPCommandAddress(base, addr);
	QSPI_ExecuteIPCommand(base, seq_idx);
	while (base->SR & QuadSPI_SR_BUSY_MASK) {
	}
	return 0;
}

#if defined(CONFIG_SOC_SERIES_MCXE24X)
static void mspi_nxp_qspi_soc_configure(QuadSPI_Type *base,
					  uint32_t src_freq, uint32_t max_freq)
{
	uint8_t clk_div = CLAMP((src_freq + max_freq - 1) / max_freq, 1, 8);
	qspi_soc_config_t cfg = {0};

	cfg.inputBufEnable = true;
	cfg.divEnable = true;
	cfg.internalClk = kQSPI_PllDiv1Clock;
	cfg.clkMode = kQSPI_SysClock;
	cfg.clkDiv = clk_div;

	QSPI_Enable(base, false);
	QSPI_SocConfigure(base, &cfg);
	QSPI_Enable(base, true);
}
#elif defined(CONFIG_SOC_SERIES_MCXE31X)
static void mspi_nxp_qspi_soc_configure(QuadSPI_Type *base)
{
	qspi_delay_chain_config_t cfg = {0};

	cfg.dqsDelayEnable = true;
	QSPI_SetDelayChainConfig(base, &cfg);
}
#endif

static int mspi_nxp_qspi_config(const struct mspi_dt_spec *spec)
{
	const struct device *dev = spec->bus;
	const struct mspi_nxp_qspi_config *cfg = dev->config;
	qspi_flash_config_t flash_cfg = {0};
	QuadSPI_Type *base = cfg->base;
	qspi_config_t qspi_cfg;
	uint32_t clock_rate;
	int ret;

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		LOG_ERR("Failed to apply pinctrl: %d", ret);
		return ret;
	}

	ret = clock_control_on(cfg->clock_gate_dev, cfg->clock_gate_subsys);
	if (ret != 0) {
		LOG_ERR("Failed to enable clock: %d", ret);
		return ret;
	}

	ret = clock_control_get_rate(cfg->clock_src_dev, cfg->clock_src_subsys,
				     &clock_rate);
	if (ret != 0) {
		LOG_ERR("Failed to get clock rate: %d", ret);
		return ret;
	}

	QSPI_GetDefaultQspiConfig(&qspi_cfg);
	qspi_cfg.rxWatermark = 8U;
	qspi_cfg.enableQspi = true;
	QSPI_Init(base, &qspi_cfg, clock_rate);

	QSPI_SetReadDataArea(base, kQSPI_ReadIP);

#if defined(CONFIG_SOC_SERIES_MCXE24X)
	mspi_nxp_qspi_soc_configure(base, clock_rate, spec->config.max_freq);
#elif defined(CONFIG_SOC_SERIES_MCXE31X)
	mspi_nxp_qspi_soc_configure(base);
#endif

	flash_cfg.flashA1Size = cfg->amba_size;
	flash_cfg.CSHoldTime = cfg->cs_hold_time;
	flash_cfg.CSSetupTime = cfg->cs_setup_time;
#if !defined(FSL_FEATURE_QSPI_HAS_NO_TDH) || (!FSL_FEATURE_QSPI_HAS_NO_TDH)
	flash_cfg.dataHoldTime = cfg->data_hold_time;
#endif
	memset(flash_cfg.lookuptable, 0, sizeof(flash_cfg.lookuptable));
	QSPI_SetFlashConfig(base, &flash_cfg);

	return 0;
}

static int mspi_nxp_qspi_dev_config(const struct device *dev,
				const struct mspi_dev_id *dev_id,
				const enum mspi_dev_cfg_mask param_mask,
				const struct mspi_dev_cfg *cfg)
{
	struct mspi_nxp_qspi_data *data = dev->data;

	if (param_mask & MSPI_DEVICE_CONFIG_IO_MODE) {
		switch (cfg->io_mode) {
		case MSPI_IO_MODE_SINGLE:
		case MSPI_IO_MODE_QUAD_1_1_4:
			break;
		default:
			LOG_ERR("Unsupported IO mode: %d", cfg->io_mode);
			return -ENOTSUP;
		}
	}

	if ((param_mask & MSPI_DEVICE_CONFIG_DATA_RATE) &&
	    (cfg->data_rate != MSPI_DATA_RATE_SINGLE)) {
		LOG_ERR("Unsupported data rate: %d", cfg->data_rate);
		return -ENOTSUP;
	}

	if (data->dev_id != dev_id) {
		if (k_mutex_lock(&data->lock,
				 K_MSEC(CONFIG_MSPI_COMPLETION_TIMEOUT_TOLERANCE))) {
			return -EBUSY;
		}
	}

	data->dev_id = dev_id;

	if (param_mask == MSPI_DEVICE_CONFIG_NONE) {
		return 0;
	}

	if (param_mask & MSPI_DEVICE_CONFIG_FREQUENCY) {
		data->dev_cfg.freq = cfg->freq;
	}
	if (param_mask & MSPI_DEVICE_CONFIG_IO_MODE) {
		data->dev_cfg.io_mode = cfg->io_mode;
		data->cur_io_mode = cfg->io_mode;
	}
	if (param_mask & MSPI_DEVICE_CONFIG_DATA_RATE) {
		data->dev_cfg.data_rate = cfg->data_rate;
	}
	if (param_mask & MSPI_DEVICE_CONFIG_CPP) {
		data->dev_cfg.cpp = cfg->cpp;
	}
	if (param_mask & MSPI_DEVICE_CONFIG_ENDIAN) {
		data->dev_cfg.endian = cfg->endian;
	}
	if (param_mask & MSPI_DEVICE_CONFIG_CE_NUM) {
		data->dev_cfg.ce_num = cfg->ce_num;
	}
	if (param_mask & MSPI_DEVICE_CONFIG_CE_POL) {
		data->dev_cfg.ce_polarity = cfg->ce_polarity;
	}
	if (param_mask & MSPI_DEVICE_CONFIG_DQS) {
		data->dev_cfg.dqs_enable = cfg->dqs_enable;
	}
	if (param_mask & MSPI_DEVICE_CONFIG_RX_DUMMY) {
		data->dev_cfg.rx_dummy = cfg->rx_dummy;
	}
	if (param_mask & MSPI_DEVICE_CONFIG_TX_DUMMY) {
		data->dev_cfg.tx_dummy = cfg->tx_dummy;
	}
	if (param_mask & MSPI_DEVICE_CONFIG_READ_CMD) {
		data->dev_cfg.read_cmd = cfg->read_cmd;
	}
	if (param_mask & MSPI_DEVICE_CONFIG_WRITE_CMD) {
		data->dev_cfg.write_cmd = cfg->write_cmd;
	}
	if (param_mask & MSPI_DEVICE_CONFIG_CMD_LEN) {
		data->dev_cfg.cmd_length = cfg->cmd_length;
	}
	if (param_mask & MSPI_DEVICE_CONFIG_ADDR_LEN) {
		data->dev_cfg.addr_length = cfg->addr_length;
	}
	if (param_mask & MSPI_DEVICE_CONFIG_MEM_BOUND) {
		data->dev_cfg.mem_boundary = cfg->mem_boundary;
	}
	if (param_mask & MSPI_DEVICE_CONFIG_BREAK_TIME) {
		data->dev_cfg.time_to_break = cfg->time_to_break;
	}

	return 0;
}

static int mspi_nxp_qspi_transceive(const struct device *dev,
				const struct mspi_dev_id *dev_id,
				const struct mspi_xfer *xfer)
{
	const struct mspi_nxp_qspi_config *cfg = dev->config;
	struct mspi_nxp_qspi_data *data = dev->data;
	QuadSPI_Type *base = cfg->base;
	int ret = 0;

	if (dev_id != data->dev_id) {
		LOG_ERR("Device ID mismatch");
		return -ESTALE;
	}

	if (xfer->num_packet == 0 || xfer->packets == NULL) {
		return -EINVAL;
	}

	for (uint32_t i = 0; i < xfer->num_packet; i++) {
		const struct mspi_xfer_packet *pkt = &xfer->packets[i];
		uint32_t addr = cfg->amba_base + pkt->address;

		ret = mspi_nxp_qspi_setup_lut(base, MSPI_NXP_QSPI_LUT_SEQ_IDX,
					       xfer, pkt, data->cur_io_mode);
		if (ret != 0) {
			break;
		}

		if (pkt->num_bytes == 0) {
			ret = mspi_nxp_qspi_ip_cmd(base, addr,
					       MSPI_NXP_QSPI_LUT_SEQ_IDX);
		} else if (pkt->dir == MSPI_RX) {
			ret = mspi_nxp_qspi_ip_read(base, addr,
					   MSPI_NXP_QSPI_LUT_SEQ_IDX,
					   pkt->data_buf, pkt->num_bytes);
		} else {
			ret = mspi_nxp_qspi_ip_write(base, addr,
					    MSPI_NXP_QSPI_LUT_SEQ_IDX,
					    pkt->data_buf, pkt->num_bytes);
		}

		if (ret != 0) {
			LOG_ERR("Transfer failed (cmd=0x%02x): %d",
				pkt->cmd, ret);
			break;
		}
	}

	return ret;
}

static int mspi_nxp_qspi_get_channel_status(const struct device *dev, uint8_t ch)
{
	struct mspi_nxp_qspi_data *data = dev->data;

	ARG_UNUSED(ch);
	data->dev_id = NULL;
	k_mutex_unlock(&data->lock);

	return 0;
}

static int mspi_nxp_qspi_init(const struct device *dev)
{
	const struct mspi_nxp_qspi_config *cfg = dev->config;
	struct mspi_nxp_qspi_data *data = dev->data;

	k_mutex_init(&data->lock);
	data->cur_io_mode = MSPI_IO_MODE_SINGLE;

	const struct mspi_dt_spec spec = {
		.bus = dev,
		.config = cfg->mspicfg,
	};

	return mspi_nxp_qspi_config(&spec);
}

static DEVICE_API(mspi, mspi_nxp_qspi_api) = {
	.config = mspi_nxp_qspi_config,
	.dev_config = mspi_nxp_qspi_dev_config,
	.get_channel_status = mspi_nxp_qspi_get_channel_status,
	.transceive = mspi_nxp_qspi_transceive,
};

#define NXP_QSPI_INIT(n)                                                      \
	PINCTRL_DT_INST_DEFINE(n);                                             \
	static const struct mspi_nxp_qspi_config mspi_nxp_qspi_cfg_##n = {    \
		.base = (QuadSPI_Type *)DT_INST_REG_ADDR(n),                  \
		.amba_base = DT_INST_REG_ADDR_BY_IDX(n, 1),                   \
		.amba_size = DT_INST_REG_SIZE_BY_IDX(n, 1),                   \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                    \
		.clock_gate_dev = DEVICE_DT_GET(                               \
			DT_INST_CLOCKS_CTLR_BY_NAME(n, gate)),                \
		.clock_gate_subsys = (clock_control_subsys_t)                  \
			DT_INST_CLOCKS_CELL_BY_NAME(n, gate, name),            \
		.clock_src_dev = DEVICE_DT_GET(                                \
			DT_INST_CLOCKS_CTLR_BY_NAME(n, clksrc)),              \
		.clock_src_subsys = (clock_control_subsys_t)                   \
			DT_INST_CLOCKS_CELL_BY_NAME(n, clksrc, name),          \
		.mspicfg = {                                                   \
			.op_mode = MSPI_OP_MODE_CONTROLLER,                    \
			.duplex = MSPI_HALF_DUPLEX,                            \
			.max_freq = DT_INST_PROP_OR(n, clock_frequency, 0),    \
			.num_periph = DT_INST_CHILD_NUM(n),                    \
		},                                                             \
		.cs_setup_time = DT_INST_PROP(n, cs_setup_time),               \
		.cs_hold_time = DT_INST_PROP(n, cs_hold_time),                 \
		.data_hold_time = DT_INST_PROP(n, data_hold_time),             \
	};                                                                     \
	static struct mspi_nxp_qspi_data mspi_nxp_qspi_data_##n;              \
	DEVICE_DT_INST_DEFINE(n,                                               \
			      mspi_nxp_qspi_init,                                   \
			      NULL,                                            \
			      &mspi_nxp_qspi_data_##n,                        \
			      &mspi_nxp_qspi_cfg_##n,                         \
			      POST_KERNEL,                                     \
			      CONFIG_MSPI_INIT_PRIORITY,                       \
			      &mspi_nxp_qspi_api);

DT_INST_FOREACH_STATUS_OKAY(NXP_QSPI_INIT)
