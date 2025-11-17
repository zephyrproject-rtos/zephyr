/*
 * Copyright (c) 2025-2026 Macronix International Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT mxicy_mspi_controller

#include "mspi_mxicy.h"
LOG_MODULE_REGISTER(mxicy_mspi_controller);

struct mspi_mxicy_config {
	DEVICE_MMIO_ROM;
	uint32_t clock_frequency;
	uint8_t ce_ports_len;
	uint8_t ce_ports[CE_PORTS_MAX_LEN];
	uint8_t rx_ss_a;
	uint8_t rx_ss_b;
	LOG_INSTANCE_PTR_DECLARE(log);
};

#define DEFINE_MM_REG_RD_WR(reg, off)                                                              \
	DEFINE_MM_REG_RD(reg, off)                                                                 \
	DEFINE_MM_REG_WR(reg, off)

DEFINE_MM_REG_WR(base_map_addr, 0x38)
DEFINE_MM_REG_WR(top_map_addr, 0xD0)
DEFINE_MM_REG_WR(hc_ver, 0x5C)
DEFINE_MM_REG_WR(hc_ctrl, 0x00)
DEFINE_MM_REG_UPDATE(hc_ctrl, 0x00)
DEFINE_MM_REG_WR(dev_ctrl, 0xC0)
DEFINE_MM_REG_UPDATE(dev_ctrl, 0xC0)
DEFINE_MM_REG_WR(clk_ctrl, 0x4C)
DEFINE_MM_REG_RD_WR(int_sts, 0x04)
DEFINE_MM_REG_WR(int_sts_en, 0x0C)
DEFINE_MM_REG_WR(int_sts_sig_en, 0x14)
DEFINE_MM_REG_WR(err_int_sts, 0x08)
DEFINE_MM_REG_WR(err_int_sts_en, 0x10)
DEFINE_MM_REG_WR(err_int_sts_sig_en, 0x18)
DEFINE_MM_REG_WR(sample_adj, 0xEC)
DEFINE_MM_REG_UPDATE(sample_adj, 0xEC)
DEFINE_MM_REG_WR(sio_idly_1, 0xF0)
DEFINE_MM_REG_WR(sio_idly_2, 0xF4)
DEFINE_MM_REG_RD_WR(tfr_ctrl, 0x20)
DEFINE_MM_REG_WR(txd, 0x70)
DEFINE_MM_REG_RD(rxd_reg, 0x80)
DEFINE_MM_REG_RD_WR(tfr_mode, 0x1C)
DEFINE_MM_REG_WR(map_rd_ctrl, 0xC4)
DEFINE_MM_REG_WR(map_wr_ctrl, 0xC8)
DEFINE_MM_REG_WR(map_cmd, 0xCC)
DEFINE_MM_REG_RD_WR(sdma_addr, 0x2C)
DEFINE_MM_REG_WR(sdma_cnt, 0x28)
DEFINE_MM_REG_RD(cap_1, 0x58)

#if defined(CONFIG_MSPI_XIP)
struct xip_params {
	uint32_t read_cmd;
	uint32_t write_cmd;
	uint16_t rx_dummy;
	uint16_t tx_dummy;
	uint8_t cmd_length;
	uint8_t addr_length;
	enum mspi_data_rate data_rate;
	enum mspi_io_mode io_mode;
};

struct xip_ctrl {
	uint32_t read;
	uint32_t write;
};
#endif

struct mspi_mxicy_data {
	DEVICE_MMIO_RAM;
	struct mspi_dev_id *dev_id;
	struct k_mutex lock;
#if defined(CONFIG_MSPI_XIP)
	uint32_t xip_freq;
	struct xip_params xip_params_stored;
	struct xip_params xip_params_active;
	uint16_t xip_enabled;
	enum mspi_cpp_mode xip_cpp;
#endif
	struct k_sem ctx_lock;
	struct k_sem cfg_lock;
	struct mspi_dev_cfg dev_cfg;
	struct mspi_xip_cfg xip_cfg;
	uint8_t data_buswidth;
	bool data_dtr;
};

static int mspi_channel_config(const struct device *dev, int ch_type, int port)
{
	uint16_t ncsbs;
	uint8_t dual_ch;
	uint32_t reg_cap1;

	reg_cap1 = read_cap_1(dev);
	ncsbs = (reg_cap1 & CAP_1_CSB_NUM_MASK) >> CAP_1_CSB_NUM_OFS;

	while (ncsbs--) {
		update_hc_ctrl(dev, HC_CTRL_CH_LUN_PORT_MASK, HC_CTRL_CH_LUN_PORT(A, 0, ncsbs));
		write_top_map_addr(dev, UEFC_TOP_MAP_ADDR);
		if (dual_ch) {
			update_hc_ctrl(dev, HC_CTRL_CH_LUN_PORT_MASK,
				       HC_CTRL_CH_LUN_PORT(B, 0, ncsbs));
			write_top_map_addr(dev, UEFC_TOP_MAP_ADDR);
		}
	}

	return 0;
}

static inline bool mspi_is_inp(const struct device *dev)
{
	struct mspi_mxicy_data *data = dev->data;

	return (k_sem_count_get(&data->ctx_lock) == 0);
}

static int dev_init(const struct device *dev)
{
	int ret = 0;
	const struct mspi_mxicy_config *cfg = dev->config;
	struct mspi_mxicy_data *data = dev->data;

	k_sem_init(&data->cfg_lock, 1, 1);
	k_sem_init(&data->ctx_lock, 1, 1);

	write_base_map_addr(dev, UEFC_BASE_MAP_ADDR);

	ret = mspi_channel_config(dev, 0, 0);
	if (0 != ret) {
		return ret;
	}

	update_hc_ctrl(dev, HC_CTRL_CH_LUN_PORT_MASK, UEFC_CH_LUN_PORT);

	write_int_sts(dev, INT_STS_ALL_CLR);
	write_int_sts_en(dev, INT_STS_EN_ALL_EN);
	write_int_sts_sig_en(dev, INT_STS_SIG_EN_ALL_EN);

	write_err_int_sts(dev, ERR_INT_STS_ALL_CLR);
	write_err_int_sts_en(dev, ERR_INT_STS_EN_ALL_EN);
	write_err_int_sts_sig_en(dev, ERR_INT_STS_SIG_EN_ALL_EN);

	write_int_sts_en(dev, INT_STS_DMA_BIT | INT_STS_EN_DMA_TFR_CMPLT_BIT);

	return 0;
}

static int mspi_poll_hc_reg(const struct device *dev, uint32_t reg, uint32_t mask)
{
	uint32_t val, n = MSPI_TIMEOUT_US;

	do {
		val = reg_read(dev, reg) & mask;
		n--;
		k_usleep(1);
	} while (!val && n);

	if (!val) {
		LOG_ERR("TIMEOUT! reg(%02Xh) & mask(%08Xh): val(%08Xh)\r\n", reg, mask, val);
		return -1;
	}
	return 0;
}

static void mspi_cs_start(const struct device *dev)
{
	/* Enable IO Mode */
	write_tfr_ctrl(dev, TFR_CTRL_IO_START_BIT);
	while (TFR_CTRL_IO_START_BIT & read_tfr_ctrl(dev)) {
		;
	}

	/* Enable host controller, reset counter */
	write_tfr_ctrl(dev, TFR_CTRL_HC_ACT_BIT);
	while (TFR_CTRL_HC_ACT_BIT & read_tfr_ctrl(dev)) {
		;
	}

	/* Assert CS */
	write_tfr_ctrl(dev, TFR_CTRL_DEV_ACT_BIT);
	while (TFR_CTRL_DEV_ACT_BIT & read_tfr_ctrl(dev)) {
		;
	}
}

static void mspi_cs_end(const struct device *dev)
{
	/* De-assert CS */
	write_tfr_ctrl(dev, TFR_CTRL_DEV_DIS_BIT);
	while (TFR_CTRL_DEV_DIS_BIT & read_tfr_ctrl(dev)) {
		;
	}

	/* Disable IO Mode */
	write_tfr_ctrl(dev, TFR_CTRL_IO_END_BIT);
	while (TFR_CTRL_IO_END_BIT & read_tfr_ctrl(dev)) {
		;
	}
}

static int mspi_io_mode_xfer(const struct device *dev, void *tx, void *rx, uint32_t len,
			     uint8_t is_data)
{
	uint32_t nbytes, tmp = 0, ofs = 0;
	struct mspi_mxicy_data *data_1 = dev->data;

	uint8_t data_octal_dtr = is_data && data_1->data_dtr && (8 == data_1->data_buswidth);

	while (ofs < len) {
		int ret;

		nbytes = len - ofs;
		uint32_t data = MSPI_DATA_PATTERN;

		if (nbytes > 4) {
			nbytes = 4;
		}

		if (tx) {
			memcpy(&data, tx + ofs, nbytes);
		}

		if (data_octal_dtr && (nbytes % 2)) {
			tmp = nbytes;
			nbytes++;
		}

		ret = mspi_poll_hc_reg(dev, PRES_STS, PRES_STS_TX_NFULL);
		if (0 != ret) {
			return ret;
		}
		reg_write(data, dev, TXD(nbytes % 4));

		ret = mspi_poll_hc_reg(dev, PRES_STS, PRES_STS_RX_NEMPT);
		if (0 != ret) {
			return ret;
		}

		data = read_rxd_reg(dev);
		if (rx) {
			memcpy(rx + ofs, &data, tmp ? tmp : nbytes);
		}

		ofs += nbytes;
	}

	return 0;
}

static uint32_t mspi_set_line(struct mspi_mxicy_data *data, enum mspi_io_mode io_mode,
			      enum mspi_data_rate data_rate)
{

	uint32_t cmd_bus = 0;
	uint32_t addr_bus = 0;
	uint32_t data_bus = 0;

	bool cmd_ddr = false;
	bool addr_ddr = false;
	bool data_ddr = false;

	switch (data_rate) {
	case MSPI_DATA_RATE_SINGLE:
		break;
	case MSPI_DATA_RATE_S_S_D:
		data_ddr = true;
		break;
	case MSPI_DATA_RATE_S_D_D:
		addr_ddr = true;
		data_ddr = true;
		break;
	case MSPI_DATA_RATE_DUAL:
		cmd_ddr = addr_ddr = data_ddr = true;
		break;
	default:
		break;
	}

	uint8_t cmd_lines = 1;
	uint8_t addr_lines = 1;
	uint8_t data_lines = 1;

	switch (io_mode) {
	case MSPI_IO_MODE_SINGLE:
		break;
	case MSPI_IO_MODE_DUAL:
		cmd_lines = addr_lines = data_lines = 2;
	case MSPI_IO_MODE_DUAL_1_1_2:
		data_lines = 2;
		break;
	case MSPI_IO_MODE_DUAL_1_2_2:
		addr_lines = data_lines = 2;
		break;
	case MSPI_IO_MODE_QUAD:
		cmd_lines = addr_lines = data_lines = 4;
	case MSPI_IO_MODE_QUAD_1_4_4:
		addr_lines = data_lines = 4;
		break;
	case MSPI_IO_MODE_QUAD_1_1_4:
		data_lines = 4;
		break;
	case MSPI_IO_MODE_OCTAL:
		cmd_lines = addr_lines = data_lines = 8;
	case MSPI_IO_MODE_OCTAL_1_8_8:
		addr_lines = data_lines = 8;
		break;
	case MSPI_IO_MODE_OCTAL_1_1_8:
		data_lines = 8;
		break;

	default:
		break;
	}

	cmd_bus = MSPI_LINES_TO_BUSWIDTH(cmd_lines);
	addr_bus = MSPI_LINES_TO_BUSWIDTH(addr_lines);
	data_bus = MSPI_LINES_TO_BUSWIDTH(data_lines);

	uint32_t conf =
		FIELD_PREP(TFR_MODE_CMD_BUSW_MASK, cmd_bus) | cmd_ddr ? TFR_MODE_CMD_DTR_BIT : 0;

	conf |= FIELD_PREP(TFR_MODE_ADDR_BUSW_MASK, addr_bus) | addr_ddr ? TFR_MODE_ADDR_DTR_BIT
									 : 0;

	conf |= FIELD_PREP(TFR_MODE_DATA_BUSW_MASK, data_bus) | data_ddr ? TFR_MODE_DATA_DTR_BIT
									 : 0;

	data->data_buswidth = data_lines;
	data->data_dtr = data_ddr;

	return conf;
}

static int _api_dev_config(const struct device *dev, const enum mspi_dev_cfg_mask param_mask,
			   const struct mspi_dev_cfg *cfg)
{
	struct mspi_mxicy_data *dev_data = dev->data;
	enum mspi_io_mode io_mode = cfg->io_mode;
	struct mspi_mxicy_config *dev_config = dev->config;

	enum mspi_data_rate data_rate = cfg->data_rate;
	bool dqs_enable = cfg->dqs_enable;
	uint32_t freq = cfg->freq;
	int ret = 0;

	if (param_mask & MSPI_DEVICE_CONFIG_ENDIAN) {
		if (cfg->endian != MSPI_XFER_BIG_ENDIAN) {
			LOG_ERR("Only big endian transfers are supported.");
			return -ENOTSUP;
		}
	}

	if (param_mask & MSPI_DEVICE_CONFIG_CE_POL) {
		if (cfg->ce_polarity != MSPI_CE_ACTIVE_LOW) {
			LOG_ERR("Only active low CE is supported.");
			return -ENOTSUP;
		}
	}
	if (param_mask & MSPI_DEVICE_CONFIG_MEM_BOUND) {
		if (cfg->mem_boundary) {
			LOG_ERR("Auto CE break is not supported.");
			return -ENOTSUP;
		}
	}
	if (param_mask & MSPI_DEVICE_CONFIG_BREAK_TIME) {
		if (cfg->time_to_break) {
			LOG_ERR("Auto CE break is not supported.");
			return -ENOTSUP;
		}
	}

	if (param_mask & MSPI_DEVICE_CONFIG_CPP) {
		if (cfg->cpp) {
			LOG_ERR("Only SPI mode 0 is supported.");
			return -ENOTSUP;
		}
	}
	if (param_mask & MSPI_DEVICE_CONFIG_IO_MODE ||
	    (param_mask & MSPI_DEVICE_CONFIG_DATA_RATE)) {
#if defined(CONFIG_MSPI_XIP)
		dev_data->xip_params_stored.io_mode = cfg->io_mode;
		dev_data->xip_params_stored.data_rate = cfg->data_rate;
#endif
		uint32_t conf = mspi_set_line(dev_data, io_mode, data_rate);
		write_tfr_mode(dev, conf);
	}
	if (param_mask & MSPI_DEVICE_CONFIG_FREQUENCY) {
#if defined(CONFIG_MSPI_XIP)
		/* Make sure the new setting is compatible with the one used
		 * for XIP if it is enabled.
		 */
		if (!dev_data->xip_enabled) {
			dev_data->xip_freq = cfg->freq;
		} else if (dev_data->xip_freq != cfg->freq) {
			LOG_ERR("Conflict with configuration used for XIP.");
			return -EINVAL;
		}
#endif
	}
	if (param_mask & MSPI_DEVICE_CONFIG_DQS) {
		if (cfg->dqs_enable) {
			update_dev_ctrl(dev, DEV_CTRL_DQS_EN,
					cfg->dqs_enable ? DEV_CTRL_DQS_EN : 0);
			update_hc_ctrl(dev, HC_CTRL_DATA_ORDER,
				       cfg->dqs_enable ? HC_CTRL_DATA_ORDER : 0);
		}
	}
#if defined(CONFIG_MSPI_XIP)
	if (param_mask & MSPI_DEVICE_CONFIG_READ_CMD) {
		dev_data->xip_params_stored.read_cmd = cfg->read_cmd;
	}
	if (param_mask & MSPI_DEVICE_CONFIG_WRITE_CMD) {
		dev_data->xip_params_stored.write_cmd = cfg->write_cmd;
	}
	if (param_mask & MSPI_DEVICE_CONFIG_RX_DUMMY) {
		dev_data->xip_params_stored.rx_dummy = cfg->rx_dummy;
	}
	if (param_mask & MSPI_DEVICE_CONFIG_TX_DUMMY) {
		dev_data->xip_params_stored.tx_dummy = cfg->tx_dummy;
	}
	if (param_mask & MSPI_DEVICE_CONFIG_CMD_LEN) {
		dev_data->xip_params_stored.cmd_length = cfg->cmd_length;
	}
	if (param_mask & MSPI_DEVICE_CONFIG_ADDR_LEN) {
		dev_data->xip_params_stored.addr_length = cfg->addr_length;
	}
#endif

	return ret;
}

static int api_dev_config(const struct device *dev, const struct mspi_dev_id *dev_id,
			  const enum mspi_dev_cfg_mask param_mask, const struct mspi_dev_cfg *cfg)
{
	const struct mspi_mxicy_config *dev_config = dev->config;
	struct mspi_mxicy_data *dev_data = dev->data;
	int rc;

	if (dev_id != dev_data->dev_id) {
		rc = k_sem_take(&dev_data->cfg_lock,
				K_MSEC(CONFIG_MSPI_COMPLETION_TIMEOUT_TOLERANCE));
		if (rc < 0) {
			LOG_ERR("Failed to switch controller to device");
			return -EBUSY;
		}

		dev_data->dev_id = dev_id;
	}

	(void)k_sem_take(&dev_data->ctx_lock, K_FOREVER);

	rc = _api_dev_config(dev, param_mask, cfg);

	k_sem_give(&dev_data->ctx_lock);

	if (rc < 0) {
		dev_data->dev_id = NULL;
		k_sem_give(&dev_data->cfg_lock);
	}

	return rc;
}

#if defined(CONFIG_MSPI_XIP)
static int _api_xip_config(const struct device *dev, const struct mspi_dev_id *dev_id,
			   const struct mspi_xip_cfg *cfg)
{
	struct mspi_mxicy_data *dev_data = dev->data;
	int rc;

	if (!cfg->enable) {
		write_tfr_ctrl(dev, TFR_CTRL_IO_START_BIT);

		dev_data->xip_enabled &= ~BIT(dev_id->dev_idx);
		return 0;
	}

	if (!dev_data->xip_enabled) {
		struct xip_params *params = &dev_data->xip_params_active;
		struct xip_ctrl ctrl = {0};

		*params = dev_data->xip_params_stored;

		uint32_t read_cmd = params->read_cmd;
		uint32_t write_cmd = params->write_cmd;
		uint8_t cmd_length = params->cmd_length;

		write_tfr_ctrl(dev, TFR_CTRL_IO_END_BIT);

		enum mspi_io_mode io_mode = params->io_mode;
		enum mspi_data_rate data_rate = params->data_rate;
		uint16_t rx_dummy = params->rx_dummy;
		uint16_t tx_dummy = params->tx_dummy;
		uint32_t conf = mspi_set_line(dev_data, io_mode, data_rate);
		ctrl.read |= conf;
		ctrl.read |= OP_DD_RD_BIT | OP_CMD_CNT(cmd_length) |
			     FIELD_PREP(TFR_MODE_ADDR_CNT_MASK, params->addr_length);

		ctrl.write |= conf;
		ctrl.write |= OP_CMD_CNT(cmd_length) |
			      FIELD_PREP(TFR_MODE_ADDR_CNT_MASK, params->addr_length);

		ctrl.read |= OP_DMY_CNT(rx_dummy, dev_data->data_dtr, dev_data->data_buswidth);
		ctrl.write |= OP_DMY_CNT(tx_dummy, dev_data->data_dtr, dev_data->data_buswidth);

		write_map_rd_ctrl(dev, ctrl.read);
		write_map_wr_ctrl(dev, ctrl.write);
		write_map_cmd(dev, read_cmd | write_cmd << MAP_WR_CMD_SHIFT);
	} else if (dev_data->xip_params_active.read_cmd != dev_data->xip_params_stored.read_cmd ||
		   dev_data->xip_params_active.write_cmd != dev_data->xip_params_stored.write_cmd ||
		   dev_data->xip_params_active.cmd_length !=
			   dev_data->xip_params_stored.cmd_length ||
		   dev_data->xip_params_active.addr_length !=
			   dev_data->xip_params_stored.addr_length ||
		   dev_data->xip_params_active.rx_dummy != dev_data->xip_params_stored.rx_dummy ||
		   dev_data->xip_params_active.tx_dummy != dev_data->xip_params_stored.tx_dummy) {
		LOG_ERR("Conflict with configuration already used for XIP.");
		return -EINVAL;
	}
	dev_data->xip_enabled |= BIT(dev_id->dev_idx);

	return 0;
}

static int api_xip_config(const struct device *dev, const struct mspi_dev_id *dev_id,
			  const struct mspi_xip_cfg *cfg)
{
	struct mspi_mxicy_data *dev_data = dev->data;
	int rc;

	if (cfg->enable && dev_id != dev_data->dev_id) {
		LOG_ERR("Controller is not configured for this device");
		return -EINVAL;
	}

	(void)k_sem_take(&dev_data->ctx_lock, K_FOREVER);

	rc = _api_xip_config(dev, dev_id, cfg);

	k_sem_give(&dev_data->ctx_lock);

	return rc;
}
#endif /* defined(CONFIG_MSPI_XIP) */

static int mspi_pio_prepare(const struct device *dev, struct mspi_xfer *xfer)
{
	const struct mspi_mxicy_config *cfg = dev->config;

	struct mspi_mxicy_data *data = dev->data;

	int ret = 0;

	uint32_t conf = read_tfr_mode(dev);

	uint16_t dummy_len = DIR_IN == xfer->packets->dir ? xfer->rx_dummy : xfer->tx_dummy;

	conf &= ~(TFR_MODE_ADDR_CNT_MASK | TFR_MODE_CMD_CNT | TFR_MODE_DMY_MASK | OP_DD_RD_BIT);

	conf |= OP_CMD_CNT(xfer->cmd_length) |
		FIELD_PREP(TFR_MODE_ADDR_CNT_MASK, xfer->addr_length);

	conf |= OP_DMY_CNT(dummy_len, data->data_dtr, data->data_buswidth);

	conf |= (DIR_IN == xfer->packets->dir ? OP_DD_RD_BIT : 0);

	if (MSPI_DMA == xfer->xfer_mode) {
		conf |= TFR_MODE_DMA_EN_BIT;
	}

	write_tfr_mode(dev, conf);

	return ret;
}

static int mspi_pio_transceive(const struct device *dev, const struct mspi_xfer *xfer)
{
	const struct mspi_mxicy_config *cfg = dev->config;
	struct mspi_mxicy_data *data = dev->data;
	const struct mspi_xfer_packet *packet;
	int ret = 0;

	if (data->dev_id) {
		update_hc_ctrl(dev, HC_CTRL_CH_LUN_PORT_MASK,
			       HC_CTRL_CH_LUN_PORT(A, 0, data->dev_id->dev_idx));
	}
	mspi_cs_start(dev);

	/* Set up command  */
	if (xfer->cmd_length != 0) {
		uint32_t cmd = (xfer->cmd_length == MSPI_2BYTE_CMD ? BSWAP_16(xfer->packets->cmd)
								   : xfer->packets->cmd);

		ret = mspi_io_mode_xfer(dev, (uint8_t *)&cmd, 0, xfer->cmd_length, 0);

		if (0 != ret) {
			mspi_cs_end(dev);
			return ret;
		}
	}
	/* Set up address */
	if (xfer->addr_length != 0) {
		uint32_t addr =
			(xfer->addr_length == MSPI_4BYTE_ADDR ? BSWAP_32(xfer->packets->address)
							      : BSWAP_24(xfer->packets->address));

		ret = mspi_io_mode_xfer(dev, (uint8_t *)&addr, 0, xfer->addr_length, 0);

		if (0 != ret) {
			mspi_cs_end(dev);
		}
	}
	uint32_t dummy_length = (MSPI_TX == xfer->packets->dir) ? xfer->tx_dummy : xfer->rx_dummy;

	/* Setup dummy: dummy's bus width and DTR are determined by the data */
	if (dummy_length != 0) {
		uint32_t dummy_len =
			(dummy_length * (data->data_dtr + 1)) / (8 / (data->data_buswidth));

		ret = mspi_io_mode_xfer(dev, 0, 0, dummy_len, 0);
		if (0 != ret) {
			mspi_cs_end(dev);
			return ret;
		}
	}

	/* Set up read/write Data */
	if (xfer->packets->data_buf != NULL) {

		ret = mspi_io_mode_xfer(dev,
					MSPI_TX == xfer->packets->dir ? xfer->packets->data_buf : 0,
					MSPI_RX == xfer->packets->dir ? xfer->packets->data_buf : 0,
					xfer->packets->num_bytes, 1);

		if (0 != ret) {
			mspi_cs_end(dev);
			return ret;
		}
	}

	mspi_cs_end(dev);

	return ret;
}

static int mspi_dma_transceive(const struct device *dev, const struct mspi_xfer *xfer)
{
	const struct mspi_mxicy_config *cfg = dev->config;
	struct mspi_mxicy_data *data = dev->data;
	uint32_t reg_int_sts;
	int ret = 0;

	if ((xfer->packets->num_bytes % 4) != 0) {
		return -EINVAL;
	}

	write_int_sts(dev, INT_STS_DMA_TFR_CMPLT_BIT | INT_STS_DMA_INT_BIT);

	mspi_cs_start(dev);

	/* Set up command  */
	if (xfer->cmd_length != 0) {
		uint16_t cmd = (xfer->cmd_length == MSPI_2BYTE_CMD ? BSWAP_16(xfer->packets->cmd)
								   : xfer->packets->cmd);

		ret = mspi_io_mode_xfer(dev, (uint8_t *)&cmd, 0, xfer->cmd_length, 0);

		if (0 != ret) {
			mspi_cs_end(dev);
			return ret;
		}
	}

	/* Set up address */
	if (xfer->addr_length != 0) {
		uint32_t addr =
			(xfer->addr_length == MSPI_4BYTE_ADDR ? BSWAP_32(xfer->packets->address)
							      : BSWAP_24(xfer->packets->address));

		ret = mspi_io_mode_xfer(dev, (uint8_t *)&addr, 0, xfer->addr_length, 0);

		if (0 != ret) {
			mspi_cs_end(dev);
		}
	}

	uint32_t dummy_length = (MSPI_TX == xfer->packets->dir) ? xfer->tx_dummy : xfer->rx_dummy;

	/* Setup dummy: dummy's bus width and DTR are determined by the data */
	if (dummy_length != 0) {
		uint32_t dummy_len =
			(dummy_length * (data->data_dtr + 1)) / (8 / (data->data_buswidth));

		ret = mspi_io_mode_xfer(dev, 0, 0, dummy_len, 0);
		if (0 != ret) {
			mspi_cs_end(dev);
			return ret;
		}
	}

	write_sdma_cnt(dev, xfer->packets->num_bytes);

	write_sdma_addr(dev, xfer->packets->data_buf);

	/* Set up read/write Data */
	if (xfer->packets->data_buf != NULL) {
		do {
			reg_int_sts = read_int_sts(dev);

			if (INT_STS_DMA_INT_BIT & reg_int_sts) {
				write_int_sts(dev, INT_STS_DMA_INT_BIT);
				write_sdma_addr(dev, xfer->packets->data_buf);
			}

		} while (!(INT_STS_DMA_TFR_CMPLT_BIT & reg_int_sts));
	}

	mspi_cs_end(dev);
}

static int mspi_mxicy_transceive(const struct device *dev, const struct mspi_dev_id *dev_id,
				 const struct mspi_xfer *xfer)
{
	const struct mspi_mxicy_config *cfg = dev->config;
	struct mspi_mxicy_data *dev_data = dev->data;
	int rc = 0;

	if (dev_id != dev_data->dev_id) {
		LOG_ERR("Controller is not configured for this device");
		return -EINVAL;
	}

	/* TODO: add support for asynchronous transfers */
	if (xfer->async) {
		LOG_ERR("Asynchronous transfers are not supported");
		return -ENOTSUP;
	}

	(void)k_sem_take(&dev_data->ctx_lock, K_FOREVER);

	mspi_pio_prepare(dev, xfer);

	if (xfer->xfer_mode == MSPI_PIO) {
		rc = mspi_pio_transceive(dev, xfer);
	} else if (xfer->xfer_mode == MSPI_DMA) {
		rc = mspi_dma_transceive(dev, xfer);
	} else {
		goto out;
	}

out:
	k_sem_give(&dev_data->ctx_lock);
	return rc;
}

static int api_get_channel_status(const struct device *dev, uint8_t ch)
{
	ARG_UNUSED(ch);

	struct mspi_mxicy_data *dev_data = dev->data;

	(void)k_sem_take(&dev_data->ctx_lock, K_FOREVER);

	dev_data->dev_id = NULL;

	k_sem_give(&dev_data->cfg_lock);
	k_sem_give(&dev_data->ctx_lock);

	return 0;
}

static int mspi_mxicy_timing_config(const struct device *dev, const struct mspi_dev_id *dev_id,
				    const uint32_t param_mask, void *timing_cfg)
{
	const struct mspi_mxicy_config *cfg = dev->config;
	struct mspi_mxicy_data *data = dev->data;
	struct mspi_mxicy_timing_cfg *time_cfg = timing_cfg;
	int ret = 0;

	if (mspi_is_inp(dev)) {
		return -EBUSY;
	}

	if (dev_id != data->dev_id) {
		LOG_INST_ERR(cfg->log, "%u, dev_id don't match.", __LINE__);
		return -ESTALE;
	}

	if (param_mask & MSPI_MXICY_SET_SIO_SHIFTER) {
		update_hc_ctrl(dev, HC_CTRL_SIO_SHIFTER_MASK,
			       FIELD_PREP(HC_CTRL_SIO_SHIFTER_MASK, time_cfg->ui8SioShifter));
	}

	if (param_mask & MSPI_MXICY_SET_DQS_DDR_DELAY) {
		update_sample_adj(
			dev, SAMPLE_ADJ_DQS_IDLY_DOPI_MASK,
			FIELD_PREP(SAMPLE_ADJ_DQS_IDLY_DOPI_MASK, time_cfg->ui8DQSDdrDelay));
	}

	if (param_mask & MSPI_MXICY_SET_DDR_DELAY) {
		update_sample_adj(dev, SAMPLE_ADJ_POINT_SEL_DDR_MASK,
				  FIELD_PREP(SAMPLE_ADJ_POINT_SEL_DDR_MASK, time_cfg->ui8DdrDelay));
	}

	if (param_mask & MSPI_MXICY_SET_SDR_DELAY) {
		update_sample_adj(dev, SAMPLE_ADJ_POINT_SEL_SDR_MASK,
				  FIELD_PREP(SAMPLE_ADJ_POINT_SEL_SDR_MASK, time_cfg->ui8SdrDelay));
	}

	if (param_mask & MSPI_MXICY_SET_SIO_LOW_DELAY) {
		write_sio_idly_1(dev, time_cfg->ui32SioLowDelay);
	}

	if (param_mask & MSPI_MXICY_SET_SIO_HIGH_DELAY) {
		write_sio_idly_2(dev, time_cfg->ui32SioHighDelay);
	}

	return ret;
}

static int api_config(const struct mspi_dt_spec *spec)
{
	ARG_UNUSED(spec);

	return -ENOTSUP;
}

static struct mspi_driver_api drv_api = {
	.config = api_config,
	.dev_config = api_dev_config,
	.get_channel_status = api_get_channel_status,
	.transceive = mspi_mxicy_transceive,
#if defined(CONFIG_MSPI_XIP)
	.xip_config = api_xip_config,
#endif
	.timing_config = mspi_mxicy_timing_config,
};

#define MSPI_MXICY_INST(inst)                                                                      \
	static struct mspi_mxicy_data dev##inst##_data;                                            \
	static const struct mspi_mxicy_config dev##inst##_config = {                               \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(inst)),                                           \
		.clock_frequency = DT_INST_PROP(inst, clock_frequency),                            \
		.rx_ss_a = DT_INST_PROP(inst, rx_ss_a),                                            \
		.rx_ss_b = DT_INST_PROP(inst, rx_ss_b),                                            \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, dev_init, NULL, &dev##inst##_data, &dev##inst##_config,        \
			      POST_KERNEL, CONFIG_MSPI_INIT_PRIORITY, &drv_api);

DT_INST_FOREACH_STATUS_OKAY(MSPI_MXICY_INST)
