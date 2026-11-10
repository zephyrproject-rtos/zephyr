/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_lpc_spifi

#include <string.h>

#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/mspi.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include <fsl_spifi.h>

LOG_MODULE_REGISTER(mspi_nxp_lpc_spifi, CONFIG_MSPI_LOG_LEVEL);

#define SPIFI_MAX_DATALEN SPIFI_CMD_DATALEN_MASK
#define SPIFI_MAX_INTLEN  (SPIFI_CMD_INTLEN_MASK >> SPIFI_CMD_INTLEN_SHIFT)

/*
 * The first intermediate byte carries the mode bits of reads such as 0xEB. All
 * ones keep the flash out of continuous read mode, so every command sends its
 * opcode. The SPIFI sends zeros after the fourth intermediate byte.
 */
#define SPIFI_IDATA_NO_CONTINUOUS 0xFFFFFFFFU

/*
 * A dev_config call limited to these settings describes the memory-mapped read
 * command; its IO mode applies to the window only.
 */
#define SPIFI_MEMMAP_CFG_MASK                                                                      \
	(MSPI_DEVICE_CONFIG_IO_MODE | MSPI_DEVICE_CONFIG_CMD_LEN | MSPI_DEVICE_CONFIG_ADDR_LEN |   \
	 MSPI_DEVICE_CONFIG_READ_CMD | MSPI_DEVICE_CONFIG_WRITE_CMD |                              \
	 MSPI_DEVICE_CONFIG_RX_DUMMY | MSPI_DEVICE_CONFIG_TX_DUMMY)

/*
 * Per IO mode: the CMD/MCMD FIELDFORM value, the clocks per intermediate byte
 * and whether CTRL[DUAL] selects IO1:0 instead of IO3:0 for multi-bit fields.
 */
static const struct {
	uint8_t fieldform;
	uint8_t int_clocks;
	bool dual;
} spifi_io_modes[] = {
	[MSPI_IO_MODE_SINGLE] = {0U, 8U, false},     [MSPI_IO_MODE_DUAL] = {3U, 4U, true},
	[MSPI_IO_MODE_DUAL_1_1_2] = {1U, 8U, true},  [MSPI_IO_MODE_DUAL_1_2_2] = {2U, 4U, true},
	[MSPI_IO_MODE_QUAD] = {3U, 2U, false},       [MSPI_IO_MODE_QUAD_1_1_4] = {1U, 8U, false},
	[MSPI_IO_MODE_QUAD_1_4_4] = {2U, 2U, false},
};

struct mspi_nxp_lpc_spifi_read_cmd {
	enum mspi_io_mode io_mode;
	uint32_t cmd;
	uint16_t dummy;
	uint8_t cmd_length;
	uint8_t addr_length;
};

struct mspi_nxp_lpc_spifi_config {
	SPIFI_Type *base;
	uintptr_t mem_base;
	uint32_t mem_size;
	const struct pinctrl_dev_config *pcfg;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	struct mspi_cfg mspicfg;
};

struct mspi_nxp_lpc_spifi_data {
	const struct mspi_dev_id *dev_id;
	struct k_mutex lock;
	enum mspi_io_mode io_mode;
	uint32_t freq;
	/* Read command set up through dev_config, taken over by memmap_config. */
	struct mspi_nxp_lpc_spifi_read_cmd read;
	/* Memory-mapped window, fixed when memmap_config enables it. */
	bool memmap;
	struct mspi_nxp_lpc_spifi_read_cmd window;
	uint32_t window_mcmd;
	uint32_t window_size;
};

static bool mspi_nxp_lpc_spifi_io_mode_supported(enum mspi_io_mode mode)
{
	return (uint32_t)mode < ARRAY_SIZE(spifi_io_modes);
}

/*
 * Build the command word without data length and direction. MCMD uses the same
 * field layout as CMD for these fields, so the word serves for both.
 */
static int mspi_nxp_lpc_spifi_cmd_word(enum mspi_io_mode io_mode, uint8_t cmd_length,
				       uint8_t addr_length, uint16_t dummy, uint32_t opcode,
				       uint32_t *word)
{
	uint32_t clocks = spifi_io_modes[io_mode].int_clocks;
	uint32_t frameform;

	/* FRAMEFORM 1-5: opcode with 0-4 address bytes, 6-7: 3 or 4 bytes, no opcode. */
	if ((cmd_length == 1U) && (addr_length <= 4U)) {
		frameform = addr_length + 1U;
	} else if ((cmd_length == 0U) && ((addr_length == 3U) || (addr_length == 4U))) {
		frameform = addr_length + 3U;
	} else {
		LOG_ERR("Command length %u with address length %u not supported", cmd_length,
			addr_length);
		return -ENOTSUP;
	}

	/* Dummy cycles are sent as intermediate bytes. */
	if (((dummy % clocks) != 0U) || ((dummy / clocks) > SPIFI_MAX_INTLEN)) {
		LOG_ERR("%u dummy cycles not possible in IO mode %d", dummy, io_mode);
		return -ENOTSUP;
	}

	*word = SPIFI_CMD_INTLEN(dummy / clocks) |
		SPIFI_CMD_FIELDFORM(spifi_io_modes[io_mode].fieldform) |
		SPIFI_CMD_FRAMEFORM(frameform) | SPIFI_CMD_OPCODE(opcode);

	return 0;
}

static void mspi_nxp_lpc_spifi_set_dual(SPIFI_Type *base, enum mspi_io_mode mode)
{
	if (spifi_io_modes[mode].dual) {
		base->CTRL |= SPIFI_CTRL_DUAL_MASK;
	} else {
		base->CTRL &= ~SPIFI_CTRL_DUAL_MASK;
	}
}

/* Abort a command or memory mode; RESET clears when a new command can be written. */
static int mspi_nxp_lpc_spifi_reset(SPIFI_Type *base)
{
	base->STAT = SPIFI_STAT_RESET_MASK;
	if (!WAIT_FOR((base->STAT & SPIFI_STAT_RESET_MASK) == 0U,
		      CONFIG_MSPI_NXP_LPC_SPIFI_TIMEOUT_US, k_busy_wait(1))) {
		return -ETIMEDOUT;
	}

	return 0;
}

static int mspi_nxp_lpc_spifi_memmap_off(SPIFI_Type *base)
{
	if ((base->STAT & SPIFI_STAT_MCINIT_MASK) == 0U) {
		return 0;
	}

	if (mspi_nxp_lpc_spifi_reset(base) < 0) {
		LOG_ERR("Leaving memory mode timed out");
		return -ETIMEDOUT;
	}

	return 0;
}

static int mspi_nxp_lpc_spifi_memmap_on(SPIFI_Type *base,
					const struct mspi_nxp_lpc_spifi_data *data)
{
	if ((base->STAT & SPIFI_STAT_MCINIT_MASK) != 0U) {
		return 0;
	}

	mspi_nxp_lpc_spifi_set_dual(base, data->window.io_mode);
	base->IDATA = SPIFI_IDATA_NO_CONTINUOUS;
	base->MCMD = data->window_mcmd;
	if (!WAIT_FOR((base->STAT & SPIFI_STAT_MCINIT_MASK) != 0U,
		      CONFIG_MSPI_NXP_LPC_SPIFI_TIMEOUT_US, k_busy_wait(1))) {
		LOG_ERR("Entering memory mode timed out");
		return -ETIMEDOUT;
	}

	return 0;
}

static int mspi_nxp_lpc_spifi_wait_idle(SPIFI_Type *base)
{
	if (WAIT_FOR((base->STAT & SPIFI_STAT_CMD_MASK) == 0U, CONFIG_MSPI_NXP_LPC_SPIFI_TIMEOUT_US,
		     k_busy_wait(1))) {
		return 0;
	}

	if (mspi_nxp_lpc_spifi_reset(base) < 0) {
		LOG_ERR("SPIFI reset timed out");
	}

	return -ETIMEDOUT;
}

static void mspi_nxp_lpc_spifi_read_data(SPIFI_Type *base, uint8_t *buf, uint32_t len)
{
	while (len >= sizeof(uint32_t)) {
		sys_put_le32(SPIFI_ReadData(base), buf);
		buf += sizeof(uint32_t);
		len -= sizeof(uint32_t);
	}

	while (len > 0U) {
		*buf = SPIFI_ReadDataByte(base);
		buf++;
		len--;
	}
}

static void mspi_nxp_lpc_spifi_write_data(SPIFI_Type *base, const uint8_t *buf, uint32_t len)
{
	while (len >= sizeof(uint32_t)) {
		SPIFI_WriteData(base, sys_get_le32(buf));
		buf += sizeof(uint32_t);
		len -= sizeof(uint32_t);
	}

	while (len > 0U) {
		SPIFI_WriteDataByte(base, *buf);
		buf++;
		len--;
	}
}

/* A read with the memory-mapped command is served from the window. */
static bool mspi_nxp_lpc_spifi_use_window(const struct mspi_nxp_lpc_spifi_data *data,
					  const struct mspi_xfer *xfer,
					  const struct mspi_xfer_packet *pkt)
{
	const struct mspi_nxp_lpc_spifi_read_cmd *window = &data->window;

	return data->memmap && (pkt->dir == MSPI_RX) && (pkt->cmd == window->cmd) &&
	       (xfer->cmd_length == window->cmd_length) &&
	       (xfer->addr_length == window->addr_length) && (xfer->rx_dummy == window->dummy) &&
	       (data->io_mode == window->io_mode) && (pkt->address <= data->window_size) &&
	       (pkt->num_bytes <= (data->window_size - pkt->address));
}

static int mspi_nxp_lpc_spifi_packet(const struct mspi_nxp_lpc_spifi_config *config,
				     struct mspi_nxp_lpc_spifi_data *data,
				     const struct mspi_xfer *xfer,
				     const struct mspi_xfer_packet *pkt)
{
	SPIFI_Type *base = config->base;
	uint16_t dummy = (pkt->dir == MSPI_RX) ? xfer->rx_dummy : xfer->tx_dummy;
	uint32_t addr = pkt->address;
	uint32_t left = pkt->num_bytes;
	uint8_t *buf = pkt->data_buf;
	uint32_t cmd;
	int ret;

	if (mspi_nxp_lpc_spifi_use_window(data, xfer, pkt)) {
		ret = mspi_nxp_lpc_spifi_memmap_on(base, data);
		if (ret < 0) {
			return ret;
		}
		memcpy(buf, (const void *)(config->mem_base + addr), left);
		return 0;
	}

	ret = mspi_nxp_lpc_spifi_cmd_word(data->io_mode, xfer->cmd_length, xfer->addr_length, dummy,
					  pkt->cmd, &cmd);
	if (ret < 0) {
		return ret;
	}

	/*
	 * A read longer than one command continues at the next address. Writes
	 * cannot be split: a flash program command must not be repeated without
	 * a new write enable.
	 */
	if ((left > SPIFI_MAX_DATALEN) && ((pkt->dir == MSPI_TX) || (xfer->addr_length == 0U))) {
		LOG_ERR("%u data bytes exceed one command", left);
		return -EINVAL;
	}

	ret = mspi_nxp_lpc_spifi_memmap_off(base);
	if (ret < 0) {
		return ret;
	}

	mspi_nxp_lpc_spifi_set_dual(base, data->io_mode);
	base->IDATA = SPIFI_IDATA_NO_CONTINUOUS;
	if (pkt->dir == MSPI_TX) {
		cmd |= SPIFI_CMD_DOUT_MASK;
	}

	do {
		uint32_t chunk = MIN(left, SPIFI_MAX_DATALEN);

		base->ADDR = addr;
		base->CMD = cmd | SPIFI_CMD_DATALEN(chunk);

		if (pkt->dir == MSPI_RX) {
			mspi_nxp_lpc_spifi_read_data(base, buf, chunk);
		} else {
			mspi_nxp_lpc_spifi_write_data(base, buf, chunk);
		}

		ret = mspi_nxp_lpc_spifi_wait_idle(base);
		if (ret < 0) {
			LOG_ERR("Command 0x%02x timed out", pkt->cmd);
			return ret;
		}

		addr += chunk;
		buf += chunk;
		left -= chunk;
	} while (left > 0U);

	return 0;
}

static int mspi_nxp_lpc_spifi_api_config(const struct mspi_dt_spec *spec)
{
	const struct mspi_cfg *config = &spec->config;

	if (config->op_mode != MSPI_OP_MODE_CONTROLLER) {
		LOG_ERR("Only controller mode is supported");
		return -ENOTSUP;
	}

	if (config->duplex != MSPI_HALF_DUPLEX) {
		LOG_ERR("Only half-duplex transfers are supported");
		return -ENOTSUP;
	}

	if (config->num_periph > 1U) {
		LOG_ERR("SPIFI has one chip select");
		return -ENOTSUP;
	}

	return 0;
}

static int mspi_nxp_lpc_spifi_dev_config(const struct device *dev, const struct mspi_dev_id *dev_id,
					 const enum mspi_dev_cfg_mask param_mask,
					 const struct mspi_dev_cfg *cfg)
{
	const struct mspi_nxp_lpc_spifi_config *config = dev->config;
	struct mspi_nxp_lpc_spifi_data *data = dev->data;
	SPIFI_Type *base = config->base;
	bool window_only = ((param_mask & MSPI_DEVICE_CONFIG_READ_CMD) != 0U) &&
			   ((param_mask & ~SPIFI_MEMMAP_CFG_MASK) == 0U);
	bool locked = false;
	uint32_t freq = data->freq;
	int ret;

	if (((param_mask & MSPI_DEVICE_CONFIG_IO_MODE) != 0U) &&
	    !mspi_nxp_lpc_spifi_io_mode_supported(cfg->io_mode)) {
		LOG_ERR("IO mode %d not supported", cfg->io_mode);
		return -ENOTSUP;
	}

	if (((param_mask & MSPI_DEVICE_CONFIG_DATA_RATE) != 0U) &&
	    (cfg->data_rate != MSPI_DATA_RATE_SINGLE)) {
		LOG_ERR("Data rate %d not supported", cfg->data_rate);
		return -ENOTSUP;
	}

	if (((param_mask & MSPI_DEVICE_CONFIG_CPP) != 0U) && (cfg->cpp != MSPI_CPP_MODE_0) &&
	    (cfg->cpp != MSPI_CPP_MODE_3)) {
		LOG_ERR("Clock mode %d not supported", cfg->cpp);
		return -ENOTSUP;
	}

	if (((param_mask & MSPI_DEVICE_CONFIG_CE_NUM) != 0U) && (cfg->ce_num != 0U)) {
		LOG_ERR("Chip select %u not available", cfg->ce_num);
		return -ENOTSUP;
	}

	if (((param_mask & MSPI_DEVICE_CONFIG_CE_POL) != 0U) &&
	    (cfg->ce_polarity != MSPI_CE_ACTIVE_LOW)) {
		LOG_ERR("Chip select polarity not supported");
		return -ENOTSUP;
	}

	if (((param_mask & MSPI_DEVICE_CONFIG_DQS) != 0U) && cfg->dqs_enable) {
		LOG_ERR("DQS not supported");
		return -ENOTSUP;
	}

	if ((((param_mask & MSPI_DEVICE_CONFIG_MEM_BOUND) != 0U) && (cfg->mem_boundary != 0U)) ||
	    (((param_mask & MSPI_DEVICE_CONFIG_BREAK_TIME) != 0U) && (cfg->time_to_break != 0U))) {
		LOG_ERR("Memory boundary and break time not supported");
		return -ENOTSUP;
	}

	if ((param_mask & MSPI_DEVICE_CONFIG_FREQUENCY) != 0U) {
		if (cfg->freq == 0U) {
			LOG_ERR("Frequency 0 not supported");
			return -EINVAL;
		}
		freq = MIN(cfg->freq, config->mspicfg.max_freq);
	}

	if (data->dev_id != dev_id) {
		ret = k_mutex_lock(&data->lock, K_MSEC(CONFIG_MSPI_COMPLETION_TIMEOUT_TOLERANCE));
		if (ret < 0) {
			return -EBUSY;
		}
		data->dev_id = dev_id;
		locked = true;
	}

	if ((base->STAT & SPIFI_STAT_CMD_MASK) != 0U) {
		ret = -EBUSY;
		goto out;
	}

	/* Clock and CTRL changes are made outside memory mode. */
	if ((freq != data->freq) || ((param_mask & MSPI_DEVICE_CONFIG_CPP) != 0U)) {
		ret = mspi_nxp_lpc_spifi_memmap_off(base);
		if (ret < 0) {
			goto out;
		}
	}

	if (freq != data->freq) {
		ret = clock_control_set_rate(config->clock_dev, config->clock_subsys,
					     (clock_control_subsys_rate_t)(uintptr_t)freq);
		if (ret < 0) {
			LOG_ERR("Cannot set the SPIFI clock to %u Hz (%d)", freq, ret);
			goto out;
		}
		data->freq = freq;
	}

	if ((param_mask & MSPI_DEVICE_CONFIG_IO_MODE) != 0U) {
		if ((param_mask & MSPI_DEVICE_CONFIG_READ_CMD) != 0U) {
			data->read.io_mode = cfg->io_mode;
		}
		if (!window_only) {
			data->io_mode = cfg->io_mode;
		}
	}
	if ((param_mask & MSPI_DEVICE_CONFIG_READ_CMD) != 0U) {
		data->read.cmd = cfg->read_cmd;
	}
	if ((param_mask & MSPI_DEVICE_CONFIG_CMD_LEN) != 0U) {
		data->read.cmd_length = cfg->cmd_length;
	}
	if ((param_mask & MSPI_DEVICE_CONFIG_ADDR_LEN) != 0U) {
		data->read.addr_length = cfg->addr_length;
	}
	if ((param_mask & MSPI_DEVICE_CONFIG_RX_DUMMY) != 0U) {
		data->read.dummy = cfg->rx_dummy;
	}

	if ((param_mask & MSPI_DEVICE_CONFIG_CPP) != 0U) {
		/*
		 * MODE3, RFCLK and FBCLK must not all be set, as the last data bit
		 * would have no falling edge to be sampled on. Mode 3 samples read
		 * data on the rising edge instead.
		 */
		if (cfg->cpp == MSPI_CPP_MODE_3) {
			base->CTRL = (base->CTRL & ~SPIFI_CTRL_RFCLK_MASK) | SPIFI_CTRL_MODE3_MASK;
		} else {
			base->CTRL = (base->CTRL & ~SPIFI_CTRL_MODE3_MASK) | SPIFI_CTRL_RFCLK_MASK;
		}
	}

	ret = 0;
	if (data->memmap) {
		ret = mspi_nxp_lpc_spifi_memmap_on(base, data);
	}

out:
	/* Release the bus again if this call took it and failed. */
	if ((ret < 0) && locked && (k_mutex_unlock(&data->lock) == 0)) {
		data->dev_id = NULL;
	}

	return ret;
}

static int mspi_nxp_lpc_spifi_transceive(const struct device *dev, const struct mspi_dev_id *dev_id,
					 const struct mspi_xfer *xfer)
{
	const struct mspi_nxp_lpc_spifi_config *config = dev->config;
	struct mspi_nxp_lpc_spifi_data *data = dev->data;
	int ret = 0;

	if (dev_id != data->dev_id) {
		LOG_ERR("Controller is not configured for this device");
		return -ESTALE;
	}

	if (xfer->async || (xfer->xfer_mode != MSPI_PIO) || xfer->hold_ce) {
		return -ENOTSUP;
	}

	if ((xfer->num_packet == 0U) || (xfer->packets == NULL)) {
		return -EINVAL;
	}

	for (uint32_t i = 0U; i < xfer->num_packet; i++) {
		ret = mspi_nxp_lpc_spifi_packet(config, data, xfer, &xfer->packets[i]);
		if (ret < 0) {
			break;
		}
	}

	/* Keep the window readable between transfers. */
	if (data->memmap) {
		int memmap_ret = mspi_nxp_lpc_spifi_memmap_on(config->base, data);

		if (ret == 0) {
			ret = memmap_ret;
		}
	}

	return ret;
}

static int mspi_nxp_lpc_spifi_memmap_config(const struct device *dev,
					    const struct mspi_dev_id *dev_id,
					    const struct mspi_memmap_cfg *memmap_cfg)
{
	const struct mspi_nxp_lpc_spifi_config *config = dev->config;
	struct mspi_nxp_lpc_spifi_data *data = dev->data;
	int ret;

	if (dev_id != data->dev_id) {
		LOG_ERR("Controller is not configured for this device");
		return -ESTALE;
	}

	if (!memmap_cfg->enable) {
		data->memmap = false;
		return mspi_nxp_lpc_spifi_memmap_off(config->base);
	}

	/* The window always starts at flash address 0 and is read-only. */
	if ((memmap_cfg->address_offset != 0U) || (memmap_cfg->size > config->mem_size) ||
	    (memmap_cfg->permission != MSPI_MEMMAP_READ_ONLY)) {
		LOG_ERR("Memory map offset 0x%x size 0x%x permission %d not supported",
			memmap_cfg->address_offset, memmap_cfg->size, memmap_cfg->permission);
		return -ENOTSUP;
	}

	/* Memory mode only takes a new read command after leaving it. */
	data->memmap = false;
	ret = mspi_nxp_lpc_spifi_memmap_off(config->base);
	if (ret < 0) {
		return ret;
	}

	ret = mspi_nxp_lpc_spifi_cmd_word(data->read.io_mode, data->read.cmd_length,
					  data->read.addr_length, data->read.dummy, data->read.cmd,
					  &data->window_mcmd);
	if (ret < 0) {
		return ret;
	}

	data->window = data->read;
	data->window_size = memmap_cfg->size;
	data->memmap = true;

	return mspi_nxp_lpc_spifi_memmap_on(config->base, data);
}

static int mspi_nxp_lpc_spifi_get_channel_status(const struct device *dev, uint8_t ch)
{
	const struct mspi_nxp_lpc_spifi_config *config = dev->config;
	struct mspi_nxp_lpc_spifi_data *data = dev->data;

	ARG_UNUSED(ch);

	if ((config->base->STAT & SPIFI_STAT_CMD_MASK) != 0U) {
		return -EBUSY;
	}

	if (data->dev_id != NULL) {
		data->dev_id = NULL;
		return k_mutex_unlock(&data->lock);
	}

	return 0;
}

static int mspi_nxp_lpc_spifi_init(const struct device *dev)
{
	const struct mspi_nxp_lpc_spifi_config *config = dev->config;
	struct mspi_nxp_lpc_spifi_data *data = dev->data;
	spifi_config_t spifi_cfg;
	int ret;

	k_mutex_init(&data->lock);
	data->io_mode = MSPI_IO_MODE_SINGLE;

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("Clock controller not ready");
		return -ENODEV;
	}

	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("Failed to apply pinctrl: %d", ret);
		return ret;
	}

	/* The divider is halted out of reset: set it before the block is clocked. */
	ret = clock_control_set_rate(
		config->clock_dev, config->clock_subsys,
		(clock_control_subsys_rate_t)(uintptr_t)config->mspicfg.max_freq);
	if (ret < 0) {
		LOG_ERR("Cannot set the SPIFI clock to %u Hz (%d)", config->mspicfg.max_freq, ret);
		return ret;
	}
	data->freq = config->mspicfg.max_freq;

	ret = clock_control_on(config->clock_dev, config->clock_subsys);
	if (ret < 0) {
		LOG_ERR("Failed to enable the SPIFI clock: %d", ret);
		return ret;
	}

	SPIFI_GetDefaultConfig(&spifi_cfg);
	SPIFI_Init(config->base, &spifi_cfg);

	return 0;
}

static DEVICE_API(mspi, mspi_nxp_lpc_spifi_api) = {
	.config = mspi_nxp_lpc_spifi_api_config,
	.dev_config = mspi_nxp_lpc_spifi_dev_config,
	.get_channel_status = mspi_nxp_lpc_spifi_get_channel_status,
	.transceive = mspi_nxp_lpc_spifi_transceive,
	.memmap_config = mspi_nxp_lpc_spifi_memmap_config,
};

#define MSPI_NXP_LPC_SPIFI_INIT(n)                                                                 \
	BUILD_ASSERT(DT_INST_CHILD_NUM(n) <= 1, "SPIFI has one chip select");                      \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
                                                                                                   \
	static const struct mspi_nxp_lpc_spifi_config mspi_nxp_lpc_spifi_config_##n = {            \
		.base = (SPIFI_Type *)DT_INST_REG_ADDR_BY_NAME(n, control),                        \
		.mem_base = DT_INST_REG_ADDR_BY_NAME(n, memory),                                   \
		.mem_size = DT_INST_REG_SIZE_BY_NAME(n, memory),                                   \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                \
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, name),              \
		.mspicfg = {                                                                       \
			.op_mode = MSPI_OP_MODE_CONTROLLER,                                        \
			.duplex = MSPI_HALF_DUPLEX,                                                \
			.max_freq = DT_INST_PROP(n, clock_frequency),                              \
			.num_periph = DT_INST_CHILD_NUM(n),                                        \
			.sw_multi_periph = DT_INST_PROP(n, software_multiperipheral),              \
		},                                                                                 \
	};                                                                                         \
                                                                                                   \
	static struct mspi_nxp_lpc_spifi_data mspi_nxp_lpc_spifi_data_##n;                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, mspi_nxp_lpc_spifi_init, NULL, &mspi_nxp_lpc_spifi_data_##n,      \
			      &mspi_nxp_lpc_spifi_config_##n, POST_KERNEL,                         \
			      CONFIG_MSPI_INIT_PRIORITY, &mspi_nxp_lpc_spifi_api);

DT_INST_FOREACH_STATUS_OKAY(MSPI_NXP_LPC_SPIFI_INIT)
