/*
 * Copyright (c) 2018 Karsten Koenig
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_mcp2515

#include <kernel.h>
#include <device.h>
#include <drivers/spi.h>
#include <drivers/gpio.h>

#define LOG_LEVEL CONFIG_CAN_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(mcp2515_can);

#include "can_mcp2515.h"

static int mcp2515_cmd_soft_reset(struct device *dev)
{
	uint8_t cmd_buf[] = { MCP2515_OPCODE_RESET };

	const struct spi_buf tx_buf = {
		.buf = cmd_buf, .len = sizeof(cmd_buf),
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf, .count = 1U
	};

	return spi_write(DEV_DATA(dev)->spi, &DEV_DATA(dev)->spi_cfg, &tx);
}

static int mcp2515_cmd_bit_modify(struct device *dev, uint8_t reg_addr, uint8_t mask,
				  uint8_t data)
{
	uint8_t cmd_buf[] = { MCP2515_OPCODE_BIT_MODIFY, reg_addr, mask, data };

	const struct spi_buf tx_buf = {
		.buf = cmd_buf, .len = sizeof(cmd_buf),
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf, .count = 1U
	};

	return spi_write(DEV_DATA(dev)->spi, &DEV_DATA(dev)->spi_cfg, &tx);
}

static int mcp2515_cmd_write_reg(struct device *dev, uint8_t reg_addr,
				 uint8_t *buf_data, uint8_t buf_len)
{
	uint8_t cmd_buf[] = { MCP2515_OPCODE_WRITE, reg_addr };

	struct spi_buf tx_buf[] = {
		{ .buf = cmd_buf, .len = sizeof(cmd_buf) },
		{ .buf = buf_data, .len = buf_len }
	};
	const struct spi_buf_set tx = {
		.buffers = tx_buf, .count = ARRAY_SIZE(tx_buf)
	};

	return spi_write(DEV_DATA(dev)->spi, &DEV_DATA(dev)->spi_cfg, &tx);
}

/*
 * Load TX buffer instruction
 *
 * When loading a transmit buffer, reduces the overhead of a normal WRITE
 * command by placing the Address Pointer at one of six locations, as
 * selected by parameter abc.
 *
 *   0: TX Buffer 0, Start at TXB0SIDH (0x31)
 *   1: TX Buffer 0, Start at TXB0D0 (0x36)
 *   2: TX Buffer 1, Start at TXB1SIDH (0x41)
 *   3: TX Buffer 1, Start at TXB1D0 (0x46)
 *   4: TX Buffer 2, Start at TXB2SIDH (0x51)
 *   5: TX Buffer 2, Start at TXB2D0 (0x56)
 */
static int mcp2515_cmd_load_tx_buffer(struct device *dev, uint8_t abc,
				 uint8_t *buf_data, uint8_t buf_len)
{
	__ASSERT(abc <= 5, "abc <= 5");

	uint8_t cmd_buf[] = { MCP2515_OPCODE_LOAD_TX_BUFFER | abc };

	struct spi_buf tx_buf[] = {
		{ .buf = cmd_buf, .len = sizeof(cmd_buf) },
		{ .buf = buf_data, .len = buf_len }
	};
	const struct spi_buf_set tx = {
		.buffers = tx_buf, .count = ARRAY_SIZE(tx_buf)
	};

	return spi_write(DEV_DATA(dev)->spi, &DEV_DATA(dev)->spi_cfg, &tx);
}

/*
 * Request-to-Send Instruction
 *
 * Parameter nnn is the combination of bits at positions 0, 1 and 2 in the RTS
 * opcode that respectively initiate transmission for buffers TXB0, TXB1 and
 * TXB2.
 */
static int mcp2515_cmd_rts(struct device *dev, uint8_t nnn)
{
	__ASSERT(nnn < BIT(MCP2515_TX_CNT), "nnn < BIT(MCP2515_TX_CNT)");

	uint8_t cmd_buf[] = { MCP2515_OPCODE_RTS | nnn };

	struct spi_buf tx_buf[] = {
		{ .buf = cmd_buf, .len = sizeof(cmd_buf) }
	};
	const struct spi_buf_set tx = {
		.buffers = tx_buf, .count = ARRAY_SIZE(tx_buf)
	};

	return spi_write(DEV_DATA(dev)->spi, &DEV_DATA(dev)->spi_cfg, &tx);
}

static int mcp2515_cmd_read_reg(struct device *dev, uint8_t reg_addr,
				uint8_t *buf_data, uint8_t buf_len)
{
	uint8_t cmd_buf[] = { MCP2515_OPCODE_READ, reg_addr };

	struct spi_buf tx_buf[] = {
		{ .buf = cmd_buf, .len = sizeof(cmd_buf) },
		{ .buf = NULL, .len = buf_len }
	};
	const struct spi_buf_set tx = {
		.buffers = tx_buf, .count = ARRAY_SIZE(tx_buf)
	};
	struct spi_buf rx_buf[] = {
		{ .buf = NULL, .len = sizeof(cmd_buf) },
		{ .buf = buf_data, .len = buf_len }
	};
	const struct spi_buf_set rx = {
		.buffers = rx_buf, .count = ARRAY_SIZE(rx_buf)
	};

	return spi_transceive(DEV_DATA(dev)->spi, &DEV_DATA(dev)->spi_cfg,
			      &tx, &rx);
}

/*
 * Read RX Buffer instruction
 *
 * When reading a receive buffer, reduces the overhead of a normal READ
 * command by placing the Address Pointer at one of four locations selected by
 * parameter nm:
 *   0: Receive Buffer 0, Start at RXB0SIDH (0x61)
 *   1: Receive Buffer 0, Start at RXB0D0 (0x66)
 *   2: Receive Buffer 1, Start at RXB1SIDH (0x71)
 *   3: Receive Buffer 1, Start at RXB1D0 (0x76)
 */
static int mcp2515_cmd_read_rx_buffer(struct device *dev, uint8_t nm,
				uint8_t *buf_data, uint8_t buf_len)
{
	__ASSERT(nm <= 0x03, "nm <= 0x03");

	uint8_t cmd_buf[] = { MCP2515_OPCODE_READ_RX_BUFFER | (nm << 1) };

	struct spi_buf tx_buf[] = {
		{ .buf = cmd_buf, .len = sizeof(cmd_buf) },
		{ .buf = NULL, .len = buf_len }
	};
	const struct spi_buf_set tx = {
		.buffers = tx_buf, .count = ARRAY_SIZE(tx_buf)
	};
	struct spi_buf rx_buf[] = {
		{ .buf = NULL, .len = sizeof(cmd_buf) },
		{ .buf = buf_data, .len = buf_len }
	};
	const struct spi_buf_set rx = {
		.buffers = rx_buf, .count = ARRAY_SIZE(rx_buf)
	};

	return spi_transceive(DEV_DATA(dev)->spi, &DEV_DATA(dev)->spi_cfg,
			      &tx, &rx);
}

static uint8_t mcp2515_convert_canmode_to_mcp2515mode(enum can_mode mode)
{
	switch (mode) {
	case CAN_NORMAL_MODE:
		return MCP2515_MODE_NORMAL;
	case CAN_SILENT_MODE:
		return MCP2515_MODE_SILENT;
	case CAN_LOOPBACK_MODE:
		return MCP2515_MODE_LOOPBACK;
	default:
		LOG_ERR("Unsupported CAN Mode %u", mode);
		return MCP2515_MODE_SILENT;
	}
}

static void mcp2515_convert_zcanframe_to_mcp2515frame(const struct zcan_frame
						      *source, uint8_t *target)
{
	uint8_t rtr;
	uint8_t dlc;
	uint8_t data_idx = 0U;

	if (source->id_type == CAN_STANDARD_IDENTIFIER) {
		target[MCP2515_FRAME_OFFSET_SIDH] = source->std_id >> 3;
		target[MCP2515_FRAME_OFFSET_SIDL] =
			(source->std_id & 0x07) << 5;
	} else {
		target[MCP2515_FRAME_OFFSET_SIDH] = source->ext_id >> 21;
		target[MCP2515_FRAME_OFFSET_SIDL] =
			(((source->ext_id >> 18) & 0x07) << 5) | (BIT(3)) |
			((source->ext_id >> 16) & 0x03);
		target[MCP2515_FRAME_OFFSET_EID8] = source->ext_id >> 8;
		target[MCP2515_FRAME_OFFSET_EID0] = source->ext_id;
	}

	rtr = (source->rtr == CAN_REMOTEREQUEST) ? BIT(6) : 0;
	dlc = (source->dlc) & 0x0F;

	target[MCP2515_FRAME_OFFSET_DLC] = rtr | dlc;

	for (; data_idx < CAN_MAX_DLC; data_idx++) {
		target[MCP2515_FRAME_OFFSET_D0 + data_idx] =
			source->data[data_idx];
	}
}

static void mcp2515_convert_mcp2515frame_to_zcanframe(const uint8_t *source,
						      struct zcan_frame *target)
{
	uint8_t data_idx = 0U;

	if (source[MCP2515_FRAME_OFFSET_SIDL] & BIT(3)) {
		target->id_type = CAN_EXTENDED_IDENTIFIER;
		target->ext_id =
			(source[MCP2515_FRAME_OFFSET_SIDH] << 21) |
			((source[MCP2515_FRAME_OFFSET_SIDL] >> 5) << 18) |
			((source[MCP2515_FRAME_OFFSET_SIDL] & 0x03) << 16) |
			(source[MCP2515_FRAME_OFFSET_EID8] << 8) |
			source[MCP2515_FRAME_OFFSET_EID0];
	} else {
		target->id_type = CAN_STANDARD_IDENTIFIER;
		target->std_id = (source[MCP2515_FRAME_OFFSET_SIDH] << 3) |
				 (source[MCP2515_FRAME_OFFSET_SIDL] >> 5);
	}

	target->dlc = source[MCP2515_FRAME_OFFSET_DLC] & 0x0F;
	target->rtr = source[MCP2515_FRAME_OFFSET_DLC] & BIT(6) ?
		      CAN_REMOTEREQUEST : CAN_DATAFRAME;

	for (; data_idx < CAN_MAX_DLC; data_idx++) {
		target->data[data_idx] = source[MCP2515_FRAME_OFFSET_D0 +
						data_idx];
	}
}

const int mcp2515_set_mode(struct device *dev, uint8_t mcp2515_mode)
{
	uint8_t canstat;

	mcp2515_cmd_bit_modify(dev, MCP2515_ADDR_CANCTRL,
			       MCP2515_CANCTRL_MODE_MASK,
			       mcp2515_mode << MCP2515_CANCTRL_MODE_POS);
	mcp2515_cmd_read_reg(dev, MCP2515_ADDR_CANSTAT, &canstat, 1);

	if (((canstat & MCP2515_CANSTAT_MODE_MASK) >> MCP2515_CANSTAT_MODE_POS)
	    != mcp2515_mode) {
		LOG_ERR("Failed to set MCP2515 operation mode");
		return -EIO;
	}

	return 0;
}

static int mcp2515_configure(struct device *dev, enum can_mode mode,
			     uint32_t bitrate)
{
	const struct mcp2515_config *dev_cfg = DEV_CFG(dev);
	struct mcp2515_data *dev_data = DEV_DATA(dev);
	int ret;

	/* CNF3, CNF2, CNF1, CANINTE */
	uint8_t config_buf[4];

	if (bitrate == 0) {
		bitrate = dev_cfg->bus_speed;
	}

	const uint8_t bit_length = 1 + dev_cfg->tq_prop + dev_cfg->tq_bs1 +
				dev_cfg->tq_bs2;

	/* CNF1; SJW<7:6> | BRP<5:0> */
	uint8_t brp = (dev_cfg->osc_freq / (bit_length * bitrate * 2)) - 1;
	const uint8_t sjw = (dev_cfg->tq_sjw - 1) << 6;
	uint8_t cnf1 = sjw | brp;

	/* CNF2; BTLMODE<7>|SAM<6>|PHSEG1<5:3>|PRSEG<2:0> */
	const uint8_t btlmode = 1 << 7;
	const uint8_t sam = 0 << 6;
	const uint8_t phseg1 = (dev_cfg->tq_bs1 - 1) << 3;
	const uint8_t prseg = (dev_cfg->tq_prop - 1);

	const uint8_t cnf2 = btlmode | sam | phseg1 | prseg;

	/* CNF3; SOF<7>|WAKFIL<6>|UND<5:3>|PHSEG2<2:0> */
	const uint8_t sof = 0 << 7;
	const uint8_t wakfil = 0 << 6;
	const uint8_t und = 0 << 3;
	const uint8_t phseg2 = (dev_cfg->tq_bs2 - 1);

	const uint8_t cnf3 = sof | wakfil | und | phseg2;

	const uint8_t caninte = MCP2515_INTE_RX0IE | MCP2515_INTE_RX1IE |
			     MCP2515_INTE_TX0IE | MCP2515_INTE_TX1IE |
			     MCP2515_INTE_TX2IE | MCP2515_INTE_ERRIE;

	/* Receive everything, filtering done in driver, RXB0 roll over into
	 * RXB1 */
	const uint8_t rx0_ctrl = BIT(6) | BIT(5) | BIT(2);
	const uint8_t rx1_ctrl = BIT(6) | BIT(5);

	__ASSERT((dev_cfg->tq_sjw >= 1) && (dev_cfg->tq_sjw <= 4),
		 "1 <= SJW <= 4");
	__ASSERT((dev_cfg->tq_prop >= 1) && (dev_cfg->tq_prop <= 8),
		 "1 <= PROP <= 8");
	__ASSERT((dev_cfg->tq_bs1 >= 1) && (dev_cfg->tq_bs1 <= 8),
		 "1 <= BS1 <= 8");
	__ASSERT((dev_cfg->tq_bs2 >= 2) && (dev_cfg->tq_bs2 <= 8),
		 "2 <= BS2 <= 8");
	__ASSERT(dev_cfg->tq_prop + dev_cfg->tq_bs1 >= dev_cfg->tq_bs2,
		 "PROP + BS1 >= BS2");
	__ASSERT(dev_cfg->tq_bs2 > dev_cfg->tq_sjw, "BS2 > SJW");

	if (dev_cfg->osc_freq % (bit_length * bitrate * 2)) {
		LOG_ERR("Prescaler is not a natural number! "
			"prescaler = osc_rate / ((PROP + SEG1 + SEG2 + 1) "
			"* bitrate * 2)\n"
			"prescaler = %d / ((%d + %d + %d + 1) * %d * 2)",
			dev_cfg->osc_freq, dev_cfg->tq_prop,
			dev_cfg->tq_bs1, dev_cfg->tq_bs2, bitrate);
	}

	config_buf[0] = cnf3;
	config_buf[1] = cnf2;
	config_buf[2] = cnf1;
	config_buf[3] = caninte;

	k_mutex_lock(&dev_data->mutex, K_FOREVER);
	/* will enter configuration mode automatically */
	ret = mcp2515_cmd_soft_reset(dev);
	if (ret < 0) {
		LOG_ERR("Failed to reset the device [%d]", ret);
		goto done;
	}

	ret = mcp2515_cmd_write_reg(dev, MCP2515_ADDR_CNF3, config_buf,
				    sizeof(config_buf));
	if (ret < 0) {
		LOG_ERR("Failed to write the configuration [%d]", ret);
	}

	ret = mcp2515_cmd_bit_modify(dev, MCP2515_ADDR_RXB0CTRL, rx0_ctrl,
				     rx0_ctrl);
	if (ret < 0) {
		LOG_ERR("Failed to write RXB0CTRL [%d]", ret);
	}

	ret = mcp2515_cmd_bit_modify(dev, MCP2515_ADDR_RXB1CTRL, rx1_ctrl,
				     rx1_ctrl);
	if (ret < 0) {
		LOG_ERR("Failed to write RXB1CTRL [%d]", ret);
	}

done:
	ret = mcp2515_set_mode(dev,
			       mcp2515_convert_canmode_to_mcp2515mode(mode));
	if (ret < 0) {
		LOG_ERR("Failed to set the mode [%d]", ret);
	}

	k_mutex_unlock(&dev_data->mutex);
	return ret;
}

static int mcp2515_send(struct device *dev, const struct zcan_frame *msg,
		 k_timeout_t timeout, can_tx_callback_t callback,
		 void *callback_arg)
{
	struct mcp2515_data *dev_data = DEV_DATA(dev);
	uint8_t tx_idx = 0U;
	uint8_t abc;
	uint8_t nnn;
	uint8_t len;
	uint8_t tx_frame[MCP2515_FRAME_LEN];

	if (msg->dlc > CAN_MAX_DLC) {
		LOG_ERR("DLC of %d exceeds maximum (%d)",
			msg->dlc, CAN_MAX_DLC);
		return CAN_TX_EINVAL;
	}

	if (k_sem_take(&dev_data->tx_sem, timeout) != 0) {
		return CAN_TIMEOUT;
	}

	k_mutex_lock(&dev_data->mutex, K_FOREVER);

	/* find a free tx slot */
	for (; tx_idx < MCP2515_TX_CNT; tx_idx++) {
		if ((BIT(tx_idx) & dev_data->tx_busy_map) == 0) {
			dev_data->tx_busy_map |= BIT(tx_idx);
			break;
		}
	}

	k_mutex_unlock(&dev_data->mutex);

	if (tx_idx == MCP2515_TX_CNT) {
		LOG_WRN("no free tx slot available");
		return CAN_TX_ERR;
	}

	dev_data->tx_cb[tx_idx].cb = callback;
	dev_data->tx_cb[tx_idx].cb_arg = callback_arg;

	mcp2515_convert_zcanframe_to_mcp2515frame(msg, tx_frame);

	/* Address Pointer selection */
	abc = 2 * tx_idx;

	/* Calculate minimum length to transfer */
	len = sizeof(tx_frame) - CAN_MAX_DLC + msg->dlc;

	mcp2515_cmd_load_tx_buffer(dev, abc, tx_frame, len);

	/* request tx slot transmission */
	nnn = BIT(tx_idx);
	mcp2515_cmd_rts(dev, nnn);

	if (callback == NULL) {
		k_sem_take(&dev_data->tx_cb[tx_idx].sem, K_FOREVER);
	}

	return 0;
}

static int mcp2515_attach_isr(struct device *dev, can_rx_callback_t rx_cb,
			      void *cb_arg,
			      const struct zcan_filter *filter)
{
	struct mcp2515_data *dev_data = DEV_DATA(dev);
	int filter_idx = 0;

	__ASSERT(rx_cb != NULL, "response_ptr can not be null");

	k_mutex_lock(&dev_data->mutex, K_FOREVER);

	/* find free filter */
	while ((BIT(filter_idx) & dev_data->filter_usage)
	       && (filter_idx < CONFIG_CAN_MCP2515_MAX_FILTER)) {
		filter_idx++;
	}

	/* setup filter */
	if (filter_idx < CONFIG_CAN_MCP2515_MAX_FILTER) {
		dev_data->filter_usage |= BIT(filter_idx);

		dev_data->filter[filter_idx] = *filter;
		dev_data->rx_cb[filter_idx] = rx_cb;
		dev_data->cb_arg[filter_idx] = cb_arg;

	} else {
		filter_idx = CAN_NO_FREE_FILTER;
	}

	k_mutex_unlock(&dev_data->mutex);

	return filter_idx;
}

static void mcp2515_detach(struct device *dev, int filter_nr)
{
	struct mcp2515_data *dev_data = DEV_DATA(dev);

	k_mutex_lock(&dev_data->mutex, K_FOREVER);
	dev_data->filter_usage &= ~BIT(filter_nr);
	k_mutex_unlock(&dev_data->mutex);
}

static void mcp2515_register_state_change_isr(struct device *dev,
						can_state_change_isr_t isr)
{
	struct mcp2515_data *dev_data = DEV_DATA(dev);

	dev_data->state_change_isr = isr;
}

static uint8_t mcp2515_filter_match(struct zcan_frame *msg,
				 struct zcan_filter *filter)
{
	if (msg->id_type != filter->id_type) {
		return 0;
	}

	if ((msg->rtr ^ filter->rtr) & filter->rtr_mask) {
		return 0;
	}

	if (msg->id_type == CAN_STANDARD_IDENTIFIER) {
		if ((msg->std_id ^ filter->std_id) & filter->std_id_mask) {
			return 0;
		}
	} else {
		if ((msg->ext_id ^ filter->ext_id) & filter->ext_id_mask) {
			return 0;
		}
	}

	return 1;
}

static void mcp2515_rx_filter(struct device *dev, struct zcan_frame *msg)
{
	struct mcp2515_data *dev_data = DEV_DATA(dev);
	uint8_t filter_idx = 0U;
	can_rx_callback_t callback;
	struct zcan_frame tmp_msg;

	k_mutex_lock(&dev_data->mutex, K_FOREVER);

	for (; filter_idx < CONFIG_CAN_MCP2515_MAX_FILTER; filter_idx++) {
		if (!(BIT(filter_idx) & dev_data->filter_usage)) {
			continue; /* filter slot empty */
		}

		if (!mcp2515_filter_match(msg,
					  &dev_data->filter[filter_idx])) {
			continue; /* filter did not match */
		}

		callback = dev_data->rx_cb[filter_idx];
		/*Make a temporary copy in case the user modifies the message*/
		tmp_msg = *msg;

		callback(&tmp_msg, dev_data->cb_arg[filter_idx]);
	}

	k_mutex_unlock(&dev_data->mutex);
}

static void mcp2515_rx(struct device *dev, uint8_t rx_idx)
{
	__ASSERT(rx_idx < MCP2515_RX_CNT, "rx_idx < MCP2515_RX_CNT");

	struct zcan_frame msg;
	uint8_t rx_frame[MCP2515_FRAME_LEN];
	uint8_t nm;

	/* Address Pointer selection */
	nm = 2 * rx_idx;

	/* Fetch rx buffer */
	mcp2515_cmd_read_rx_buffer(dev, nm, rx_frame, sizeof(rx_frame));
	mcp2515_convert_mcp2515frame_to_zcanframe(rx_frame, &msg);
	mcp2515_rx_filter(dev, &msg);
}

static void mcp2515_tx_done(struct device *dev, uint8_t tx_idx)
{
	struct mcp2515_data *dev_data = DEV_DATA(dev);

	if (dev_data->tx_cb[tx_idx].cb == NULL) {
		k_sem_give(&dev_data->tx_cb[tx_idx].sem);
	} else {
		dev_data->tx_cb[tx_idx].cb(0, dev_data->tx_cb[tx_idx].cb_arg);
	}

	k_mutex_lock(&dev_data->mutex, K_FOREVER);
	dev_data->tx_busy_map &= ~BIT(tx_idx);
	k_mutex_unlock(&dev_data->mutex);
	k_sem_give(&dev_data->tx_sem);
}

static enum can_state mcp2515_get_state(struct device *dev,
					struct can_bus_err_cnt *err_cnt)
{
	uint8_t eflg;
	uint8_t err_cnt_buf[2];
	int ret;

	ret = mcp2515_cmd_read_reg(dev, MCP2515_ADDR_EFLG, &eflg, sizeof(eflg));
	if (ret < 0) {
		LOG_ERR("Failed to read error register [%d]", ret);
		return CAN_BUS_UNKNOWN;
	}

	if (err_cnt) {
		ret = mcp2515_cmd_read_reg(dev, MCP2515_ADDR_TEC, err_cnt_buf,
					   sizeof(err_cnt_buf));
		if (ret < 0) {
			LOG_ERR("Failed to read error counters [%d]", ret);
			return CAN_BUS_UNKNOWN;
		}

		err_cnt->tx_err_cnt = err_cnt_buf[0];
		err_cnt->rx_err_cnt = err_cnt_buf[1];
	}

	if (eflg & MCP2515_EFLG_TXBO) {
		return CAN_BUS_OFF;
	}

	if ((eflg & MCP2515_EFLG_RXEP) || (eflg & MCP2515_EFLG_TXEP)) {
		return CAN_ERROR_PASSIVE;
	}

	return CAN_ERROR_ACTIVE;
}

static void mcp2515_handle_errors(struct device *dev)
{
	struct mcp2515_data *dev_data = DEV_DATA(dev);
	can_state_change_isr_t state_change_isr = dev_data->state_change_isr;
	enum can_state state;
	struct can_bus_err_cnt err_cnt;

	state = mcp2515_get_state(dev, state_change_isr ? &err_cnt : NULL);

	if (state_change_isr && dev_data->old_state != state) {
		dev_data->old_state = state;
		state_change_isr(state, err_cnt);
	}
}

#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
static void mcp2515_recover(struct device *dev, k_timeout_t timeout)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(timeout);
}
#endif

static void mcp2515_handle_interrupts(struct device *dev)
{
	const struct mcp2515_config *dev_cfg = DEV_CFG(dev);
	struct mcp2515_data *dev_data = DEV_DATA(dev);
	int ret;
	uint8_t canintf;

	/* Loop until INT pin is inactive (all interrupt flags handled) */
	while (1) {
		ret = mcp2515_cmd_read_reg(dev, MCP2515_ADDR_CANINTF,
				&canintf, 1);
		if (ret != 0) {
			LOG_ERR("Couldn't read INTF register %d", ret);
			continue;
		}

		if (canintf == 0) {
			/* No interrupt flags set */
			break;
		}

		if (canintf & MCP2515_CANINTF_RX0IF) {
			mcp2515_rx(dev, 0);

			/* RX0IF flag cleared automatically during read */
			canintf &= ~MCP2515_CANINTF_RX0IF;
		}

		if (canintf & MCP2515_CANINTF_RX1IF) {
			mcp2515_rx(dev, 1);

			/* RX1IF flag cleared automatically during read */
			canintf &= ~MCP2515_CANINTF_RX1IF;
		}

		if (canintf & MCP2515_CANINTF_TX0IF) {
			mcp2515_tx_done(dev, 0);
		}

		if (canintf & MCP2515_CANINTF_TX1IF) {
			mcp2515_tx_done(dev, 1);
		}

		if (canintf & MCP2515_CANINTF_TX2IF) {
			mcp2515_tx_done(dev, 2);
		}

		if (canintf & MCP2515_CANINTF_ERRIF) {
			mcp2515_handle_errors(dev);
		}

		if (canintf != 0) {
			/* Clear remaining flags */
			mcp2515_cmd_bit_modify(dev, MCP2515_ADDR_CANINTF,
					canintf, ~canintf);
		}

		/* Break from loop if INT pin is inactive */
		ret = gpio_pin_get(dev_data->int_gpio, dev_cfg->int_pin);
		if (ret < 0) {
			LOG_ERR("Couldn't read INT pin");
		} else if (ret == 0) {
			/* All interrupt flags handled */
			break;
		}
	}
}

static void mcp2515_int_thread(struct device *dev)
{
	struct mcp2515_data *dev_data = DEV_DATA(dev);

	while (1) {
		k_sem_take(&dev_data->int_sem, K_FOREVER);
		mcp2515_handle_interrupts(dev);
	}
}

static void mcp2515_int_gpio_callback(struct device *dev,
				      struct gpio_callback *cb, uint32_t pins)
{
	struct mcp2515_data *dev_data =
		CONTAINER_OF(cb, struct mcp2515_data, int_gpio_cb);

	k_sem_give(&dev_data->int_sem);
}

static const struct can_driver_api can_api_funcs = {
	.configure = mcp2515_configure,
	.send = mcp2515_send,
	.attach_isr = mcp2515_attach_isr,
	.detach = mcp2515_detach,
	.get_state = mcp2515_get_state,
#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
	.recover = mcp2515_recover,
#endif
	.register_state_change_isr = mcp2515_register_state_change_isr
};


static int mcp2515_init(struct device *dev)
{
	const struct mcp2515_config *dev_cfg = DEV_CFG(dev);
	struct mcp2515_data *dev_data = DEV_DATA(dev);
	int ret;

	k_sem_init(&dev_data->int_sem, 0, 1);
	k_mutex_init(&dev_data->mutex);
	k_sem_init(&dev_data->tx_sem, MCP2515_TX_CNT, MCP2515_TX_CNT);
	k_sem_init(&dev_data->tx_cb[0].sem, 0, 1);
	k_sem_init(&dev_data->tx_cb[1].sem, 0, 1);
	k_sem_init(&dev_data->tx_cb[2].sem, 0, 1);

	/* SPI config */
	dev_data->spi_cfg.operation = SPI_WORD_SET(8);
	dev_data->spi_cfg.frequency = dev_cfg->spi_freq;
	dev_data->spi_cfg.slave = dev_cfg->spi_slave;

	dev_data->spi = device_get_binding(dev_cfg->spi_port);
	if (!dev_data->spi) {
		LOG_ERR("SPI master port %s not found", dev_cfg->spi_port);
		return -EINVAL;
	}

#if DT_INST_SPI_DEV_HAS_CS_GPIOS(0)
	dev_data->spi_cs_ctrl.gpio_dev =
		device_get_binding(dev_cfg->spi_cs_port);
	if (!dev_data->spi_cs_ctrl.gpio_dev) {
		LOG_ERR("Unable to get GPIO SPI CS device");
		return -ENODEV;
	}

	dev_data->spi_cs_ctrl.gpio_pin = dev_cfg->spi_cs_pin;
	dev_data->spi_cs_ctrl.gpio_dt_flags = dev_cfg->spi_cs_flags;
	dev_data->spi_cs_ctrl.delay = 0U;

	dev_data->spi_cfg.cs = &dev_data->spi_cs_ctrl;
#else
	dev_data->spi_cfg.cs = NULL;
#endif  /* DT_INST_SPI_DEV_HAS_CS_GPIOS(0) */

	/* Reset MCP2515 */
	if (mcp2515_cmd_soft_reset(dev)) {
		LOG_ERR("Soft-reset failed");
		return -EIO;
	}

	/* Initialize interrupt handling  */
	dev_data->int_gpio = device_get_binding(dev_cfg->int_port);
	if (dev_data->int_gpio == NULL) {
		LOG_ERR("GPIO port %s not found", dev_cfg->int_port);
		return -EINVAL;
	}

	if (gpio_pin_configure(dev_data->int_gpio, dev_cfg->int_pin,
			       (GPIO_INPUT |
				DT_INST_GPIO_FLAGS(0, int_gpios)))) {
		LOG_ERR("Unable to configure GPIO pin %u", dev_cfg->int_pin);
		return -EINVAL;
	}

	gpio_init_callback(&(dev_data->int_gpio_cb), mcp2515_int_gpio_callback,
			   BIT(dev_cfg->int_pin));

	if (gpio_add_callback(dev_data->int_gpio, &(dev_data->int_gpio_cb))) {
		return -EINVAL;
	}

	if (gpio_pin_interrupt_configure(dev_data->int_gpio, dev_cfg->int_pin,
					 GPIO_INT_EDGE_TO_ACTIVE)) {
		return -EINVAL;
	}

	k_thread_create(&dev_data->int_thread, dev_data->int_thread_stack,
			dev_cfg->int_thread_stack_size,
			(k_thread_entry_t) mcp2515_int_thread, (void *)dev,
			NULL, NULL, K_PRIO_COOP(dev_cfg->int_thread_priority),
			0, K_NO_WAIT);

	(void)memset(dev_data->rx_cb, 0, sizeof(dev_data->rx_cb));
	(void)memset(dev_data->filter, 0, sizeof(dev_data->filter));
	dev_data->old_state = CAN_ERROR_ACTIVE;

	ret = mcp2515_configure(dev, CAN_NORMAL_MODE, dev_cfg->bus_speed);

	return ret;
}

#if DT_NODE_HAS_STATUS(DT_DRV_INST(0), okay)

static K_KERNEL_STACK_DEFINE(mcp2515_int_thread_stack,
			     CONFIG_CAN_MCP2515_INT_THREAD_STACK_SIZE);

static struct mcp2515_data mcp2515_data_1 = {
	.int_thread_stack = mcp2515_int_thread_stack,
	.tx_cb[0].cb = NULL,
	.tx_cb[1].cb = NULL,
	.tx_cb[2].cb = NULL,
	.tx_busy_map = 0U,
	.filter_usage = 0U,
};

static const struct mcp2515_config mcp2515_config_1 = {
	.spi_port = DT_INST_BUS_LABEL(0),
	.spi_freq = DT_INST_PROP(0, spi_max_frequency),
	.spi_slave = DT_INST_REG_ADDR(0),
	.int_pin = DT_INST_GPIO_PIN(0, int_gpios),
	.int_port = DT_INST_GPIO_LABEL(0, int_gpios),
	.int_thread_stack_size = CONFIG_CAN_MCP2515_INT_THREAD_STACK_SIZE,
	.int_thread_priority = CONFIG_CAN_MCP2515_INT_THREAD_PRIO,
#if DT_INST_SPI_DEV_HAS_CS_GPIOS(0)
	.spi_cs_pin = DT_INST_SPI_DEV_CS_GPIOS_PIN(0),
	.spi_cs_port = DT_INST_SPI_DEV_CS_GPIOS_LABEL(0),
	.spi_cs_flags = DT_INST_SPI_DEV_CS_GPIOS_FLAGS(0),
#endif  /* DT_INST_SPI_DEV_HAS_CS_GPIOS(0) */
	.tq_sjw = DT_INST_PROP(0, sjw),
	.tq_prop = DT_INST_PROP(0, prop_seg),
	.tq_bs1 = DT_INST_PROP(0, phase_seg1),
	.tq_bs2 = DT_INST_PROP(0, phase_seg2),
	.bus_speed = DT_INST_PROP(0, bus_speed),
	.osc_freq = DT_INST_PROP(0, osc_freq)
};

DEVICE_AND_API_INIT(can_mcp2515_1, DT_INST_LABEL(0), &mcp2515_init,
		    &mcp2515_data_1, &mcp2515_config_1, POST_KERNEL,
		    CONFIG_CAN_MCP2515_INIT_PRIORITY, &can_api_funcs);

#endif /* DT_NODE_HAS_STATUS(DT_DRV_INST(0), okay) */
