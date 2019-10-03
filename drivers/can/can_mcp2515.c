/*
 * Copyright (c) 2018 Karsten Koenig
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <device.h>
#include <drivers/spi.h>
#include <drivers/gpio.h>

#define LOG_LEVEL CONFIG_CAN_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(mcp2515_can);

#include "can_mcp2515.h"

static K_THREAD_STACK_DEFINE(mcp2515_int_work_stack,
			     CONFIG_CAN_MCP2515_INT_THREAD_STACK_SIZE);

static struct k_work_q mcp2515_workq;

static int mcp2515_cmd_soft_reset(struct mcp2515_data *dev_data)
{
	u8_t cmd_buf[] = { MCP2515_OPCODE_RESET };

	const struct spi_buf tx_buf = {
		.buf = cmd_buf, .len = sizeof(cmd_buf),
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf, .count = 1U
	};

	return spi_write(dev_data->spi, &dev_data->spi_cfg, &tx);
}

static int mcp2515_cmd_bit_modify(struct mcp2515_data *dev_data, u8_t reg_addr, u8_t mask,
				  u8_t data)
{
	u8_t cmd_buf[] = { MCP2515_OPCODE_BIT_MODIFY, reg_addr, mask, data };

	const struct spi_buf tx_buf = {
		.buf = cmd_buf, .len = sizeof(cmd_buf),
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf, .count = 1U
	};

	return spi_write(dev_data->spi, &dev_data->spi_cfg, &tx);
}

static int mcp2515_cmd_write_reg(struct mcp2515_data *dev_data, u8_t reg_addr,
				 u8_t *buf_data, u8_t buf_len)
{
	u8_t cmd_buf[] = { MCP2515_OPCODE_WRITE, reg_addr };

	struct spi_buf tx_buf[] = {
		{ .buf = cmd_buf, .len = sizeof(cmd_buf) },
		{ .buf = buf_data, .len = buf_len }
	};
	const struct spi_buf_set tx = {
		.buffers = tx_buf, .count = ARRAY_SIZE(tx_buf)
	};

	return spi_write(dev_data->spi, &dev_data->spi_cfg, &tx);
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
static int mcp2515_cmd_load_tx_buffer(struct mcp2515_data *dev_data, u8_t abc,
				 u8_t *buf_data, u8_t buf_len)
{
	__ASSERT(abc <= 5, "abc <= 5");

	u8_t cmd_buf[] = { MCP2515_OPCODE_LOAD_TX_BUFFER | abc };

	struct spi_buf tx_buf[] = {
		{ .buf = cmd_buf, .len = sizeof(cmd_buf) },
		{ .buf = buf_data, .len = buf_len }
	};
	const struct spi_buf_set tx = {
		.buffers = tx_buf, .count = ARRAY_SIZE(tx_buf)
	};

	return spi_write(dev_data->spi, &dev_data->spi_cfg, &tx);
}

/*
 * Request-to-Send Instruction
 *
 * Parameter nnn is the combination of bits at positions 0, 1 and 2 in the RTS
 * opcode that respectively initiate transmission for buffers TXB0, TXB1 and
 * TXB2.
 */
static int mcp2515_cmd_rts(struct mcp2515_data *dev_data, u8_t nnn)
{
	__ASSERT(nnn < BIT(MCP2515_TX_CNT), "nnn < BIT(MCP2515_TX_CNT)");

	u8_t cmd_buf[] = { MCP2515_OPCODE_RTS | nnn };

	struct spi_buf tx_buf[] = {
		{ .buf = cmd_buf, .len = sizeof(cmd_buf) }
	};
	const struct spi_buf_set tx = {
		.buffers = tx_buf, .count = ARRAY_SIZE(tx_buf)
	};

	return spi_write(dev_data->spi, &dev_data->spi_cfg, &tx);
}

static int mcp2515_cmd_read_reg(struct mcp2515_data *dev_data, u8_t reg_addr,
				u8_t *buf_data, u8_t buf_len)
{
	u8_t cmd_buf[] = { MCP2515_OPCODE_READ, reg_addr };

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

	return spi_transceive(dev_data->spi, &dev_data->spi_cfg,
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
static int mcp2515_cmd_read_rx_buffer(struct mcp2515_data *dev_data, u8_t nm,
				u8_t *buf_data, u8_t buf_len)
{
	__ASSERT(nm <= 0x03, "nm <= 0x03");

	u8_t cmd_buf[] = { MCP2515_OPCODE_READ_RX_BUFFER | (nm << 1) };

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

	return spi_transceive(dev_data->spi, &dev_data->spi_cfg,
			      &tx, &rx);
}

static u8_t mcp2515_convert_canmode_to_mcp2515mode(enum can_mode mode)
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
						      *source, u8_t *target)
{
	u8_t rtr;
	u8_t dlc;
	u8_t data_idx = 0U;

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

static void mcp2515_convert_mcp2515frame_to_zcanframe(const u8_t *source,
						      struct zcan_frame *target)
{
	u8_t data_idx = 0U;

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

const int mcp2515_set_mode(struct mcp2515_data *dev_data, u8_t mcp2515_mode)
{
	u8_t canstat;

	mcp2515_cmd_bit_modify(dev_data, MCP2515_ADDR_CANCTRL,
			       MCP2515_CANCTRL_MODE_MASK,
			       mcp2515_mode << MCP2515_CANCTRL_MODE_POS);
	mcp2515_cmd_read_reg(dev_data, MCP2515_ADDR_CANSTAT, &canstat, 1);

	if (((canstat & MCP2515_CANSTAT_MODE_MASK) >> MCP2515_CANSTAT_MODE_POS)
	    != mcp2515_mode) {
		LOG_ERR("Failed to set MCP2515 operation mode");
		return -EIO;
	}

	return 0;
}

static int mcp2515_configure(struct device *dev, enum can_mode mode,
			     u32_t bitrate)
{
	const struct mcp2515_config *dev_cfg = DEV_CFG(dev);
	struct mcp2515_data *dev_data = DEV_DATA(dev);
	int ret;

	/* CNF3, CNF2, CNF1, CANINTE */
	u8_t config_buf[4];

	if (bitrate == 0) {
		bitrate = dev_cfg->bus_speed;
	}

	const u8_t bit_length = 1 + dev_cfg->tq_prop + dev_cfg->tq_bs1 +
				dev_cfg->tq_bs2;

	/* CNF1; SJW<7:6> | BRP<5:0> */
	u8_t brp = (dev_cfg->osc_freq / (bit_length * bitrate * 2)) - 1;
	const u8_t sjw = (dev_cfg->tq_sjw - 1) << 6;
	u8_t cnf1 = sjw | brp;

	/* CNF2; BTLMODE<7>|SAM<6>|PHSEG1<5:3>|PRSEG<2:0> */
	const u8_t btlmode = 1 << 7;
	const u8_t sam = 0 << 6;
	const u8_t phseg1 = (dev_cfg->tq_bs1 - 1) << 3;
	const u8_t prseg = (dev_cfg->tq_prop - 1);

	const u8_t cnf2 = btlmode | sam | phseg1 | prseg;

	/* CNF3; SOF<7>|WAKFIL<6>|UND<5:3>|PHSEG2<2:0> */
	const u8_t sof = 0 << 7;
	const u8_t wakfil = 0 << 6;
	const u8_t und = 0 << 3;
	const u8_t phseg2 = (dev_cfg->tq_bs2 - 1);

	const u8_t cnf3 = sof | wakfil | und | phseg2;

	const u8_t caninte = MCP2515_INTE_RX0IE | MCP2515_INTE_RX1IE |
			     MCP2515_INTE_TX0IE | MCP2515_INTE_TX1IE |
			     MCP2515_INTE_TX2IE | MCP2515_INTE_ERRIE;

	/* Receive everything, filtering done in driver, RXB0 roll over into
	 * RXB1 */
	const u8_t rx0_ctrl = BIT(6) | BIT(5) | BIT(2);
	const u8_t rx1_ctrl = BIT(6) | BIT(5);

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
	ret = mcp2515_cmd_soft_reset(dev_data);
	if (ret < 0) {
		LOG_ERR("Failed to reset the device [%d]", ret);
		goto done;
	}

	ret = mcp2515_cmd_write_reg(dev_data, MCP2515_ADDR_CNF3, config_buf,
				    sizeof(config_buf));
	if (ret < 0) {
		LOG_ERR("Failed to write the configuration [%d]", ret);
	}

	ret = mcp2515_cmd_bit_modify(dev_data, MCP2515_ADDR_RXB0CTRL, rx0_ctrl,
				     rx0_ctrl);
	if (ret < 0) {
		LOG_ERR("Failed to write RXB0CTRL [%d]", ret);
	}
	ret = mcp2515_cmd_bit_modify(dev_data, MCP2515_ADDR_RXB1CTRL, rx1_ctrl,
				     rx1_ctrl);
	if (ret < 0) {
		LOG_ERR("Failed to write RXB1CTRL [%d]", ret);
	}

done:
	ret = mcp2515_set_mode(dev_data,
			       mcp2515_convert_canmode_to_mcp2515mode(mode));
	if (ret < 0) {
		LOG_ERR("Failed to set the mode [%d]", ret);
	}

	k_mutex_unlock(&dev_data->mutex);
	return ret;
}

static int mcp2515_tx_get_err(struct mcp2515_data *dev_data, int tx_idx)
{
	u8_t addr_tx_ctrl = MCP2515_ADDR_TXB0CTRL +
			    (tx_idx * MCP2515_ADDR_OFFSET_FRAME2FRAME);
	u8_t txb_ctrl;
	int ret;

	__ASSERT(tx_idx >= 0 && tx_idx <= MCP2515_TX_CNT,
		 "Index out of bounds [%d", tx_idx);

	ret = mcp2515_cmd_read_reg(dev_data, addr_tx_ctrl, &txb_ctrl,
				   sizeof(txb_ctrl));
	if (ret < 0) {
		LOG_ERR("Failed to read error of mb %d [%d]", tx_idx, ret);
	}

	if (txb_ctrl & MCP2515_TXCTRL_MLOA) {
		return CAN_TX_ARB_LOST;
	}

	if (txb_ctrl & MCP2515_TXCTRL_TXERR) {
		return CAN_TX_ERR;
	}

	return CAN_TX_TIMEOUT;
}

static int mcp2515_tx_abort(struct mcp2515_data *dev_data, int tx_idx)
{
	u8_t addr_tx_ctrl = MCP2515_ADDR_TXB0CTRL +
			    (tx_idx * MCP2515_ADDR_OFFSET_FRAME2FRAME);
	int ret;

	__ASSERT(tx_idx >= 0 && tx_idx <= MCP2515_TX_CNT,
		 "Index out of bounds [%d", tx_idx);

	ret = mcp2515_cmd_bit_modify(dev_data, addr_tx_ctrl,
				     MCP2515_TXCTRL_TXREQ, 0);

	if (ret < 0) {
		LOG_ERR("TX abort failed. TX index: %d", tx_idx);
	}

	ret = mcp2515_tx_get_err(dev_data, tx_idx);

	atomic_clear_bit(dev_data->tx_busy_map, BIT(tx_idx));
	k_sem_give(&dev_data->tx_sem);

	return ret;
}

static inline struct mcp2515_tx_cb *mcp2515_get_cb_from_work(struct k_work *work)
{
	struct k_delayed_work *delayed_work =
				CONTAINER_OF(work, struct k_delayed_work, work);
	return CONTAINER_OF(delayed_work, struct mcp2515_tx_cb, to_work);
}

static void mcp2515_tx0_abort_handler(struct k_work *work)
{
	struct mcp2515_tx_cb *cb = mcp2515_get_cb_from_work(work);
	struct mcp2515_data *dev_data =
			CONTAINER_OF(cb, struct mcp2515_data, tx_cb0);
	int err;

	err = mcp2515_tx_abort(dev_data, 0);
	cb->cb(err, cb->cb_arg);
}

static void mcp2515_tx1_abort_handler(struct k_work *work)
{
	struct mcp2515_tx_cb *cb = mcp2515_get_cb_from_work(work);
	struct mcp2515_data *dev_data =
			CONTAINER_OF(cb, struct mcp2515_data, tx_cb1);
	int err;

	err = mcp2515_tx_abort(dev_data, 1);
	cb->cb(err, cb->cb_arg);
}

static void mcp2515_tx2_abort_handler(struct k_work *work)
{
	struct mcp2515_tx_cb *cb = mcp2515_get_cb_from_work(work);
	struct mcp2515_data *dev_data =
			CONTAINER_OF(cb, struct mcp2515_data, tx_cb2);
	int err;

	err = mcp2515_tx_abort(dev_data, 2);
	cb->cb(err, cb->cb_arg);
}

static int mcp2515_send(struct device *dev, const struct zcan_frame *msg,
		 s32_t mb_timeout, s32_t send_timeout,
		 can_tx_callback_t callback, void *callback_arg)
{
	struct mcp2515_data *dev_data = DEV_DATA(dev);
	u8_t tx_idx = 0U;
	u8_t abc;
	u8_t nnn;
	u8_t len;
	u8_t tx_frame[MCP2515_FRAME_LEN];
	struct mcp2515_tx_cb *tx_cb;
	int ret;

	__ASSERT(callback != NULL, "callback is null");

	if (msg->dlc > CAN_MAX_DLC) {
		LOG_ERR("DLC of %d exceeds maximum (%d)", msg->dlc, CAN_MAX_DLC);
		return CAN_TX_EINVAL;
	}

	ret = k_sem_take(&dev_data->tx_sem, mb_timeout);
	if (ret != 0) {
		return CAN_TIMEOUT;
	}

	/* find a free tx slot */
	for (; tx_idx < MCP2515_TX_CNT; tx_idx++) {
		if (!atomic_test_and_set_bit(dev_data->tx_busy_map, BIT(tx_idx))) {
			break;
		}
	}

	switch (tx_idx) {
	case 0:
		tx_cb = &dev_data->tx_cb0;
		break;

	case 1:
		tx_cb = &dev_data->tx_cb1;
		break;

	case 2:
		tx_cb = &dev_data->tx_cb2;
		break;

	default:
		LOG_WRN("no free tx slot available");
		return CAN_TX_ERR;

	}

	tx_cb->cb = callback;
	tx_cb->cb_arg = callback_arg;

	mcp2515_convert_zcanframe_to_mcp2515frame(msg, tx_frame);

	/* Address Pointer selection */
	abc = 2 * tx_idx;

	/* Calculate minimum length to transfer */
	len = sizeof(tx_frame) - CAN_MAX_DLC + msg->dlc;

	mcp2515_cmd_load_tx_buffer(dev_data, abc, tx_frame, len);

	/* request tx slot transmission */
	nnn = BIT(tx_idx);
	mcp2515_cmd_rts(dev_data, nnn);

	__ASSERT(send_timeout != K_NO_WAIT, "Message can't be sent instantly");

	if (send_timeout != K_FOREVER) {
		k_delayed_work_submit_to_queue(&mcp2515_workq,
					       &tx_cb->to_work,
					       send_timeout);
	}

	return CAN_TX_OK;
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

static u8_t mcp2515_filter_match(struct zcan_frame *msg,
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

static void mcp2515_rx_filter(struct mcp2515_data *dev_data,
			      struct zcan_frame *msg)
{
	u8_t filter_idx = 0U;
	can_rx_callback_t callback;
	struct zcan_frame tmp_msg;

	k_mutex_lock(&dev_data->mutex, K_FOREVER);

	for (; filter_idx < CONFIG_CAN_MCP2515_MAX_FILTER; filter_idx++) {
		if (!(BIT(filter_idx) & dev_data->filter_usage)) {
			continue; /* filter slot empty */
		}

		if (!mcp2515_filter_match(msg, &dev_data->filter[filter_idx])) {
			continue; /* filter did not match */
		}

		callback = dev_data->rx_cb[filter_idx];
		/*Make a temporary copy in case the user modifies the message*/
		tmp_msg = *msg;

		callback(&tmp_msg, dev_data->cb_arg[filter_idx]);
	}

	k_mutex_unlock(&dev_data->mutex);
}

static void mcp2515_rx(struct mcp2515_data *dev_data, u8_t rx_idx)
{
	__ASSERT(rx_idx < MCP2515_RX_CNT, "rx_idx < MCP2515_RX_CNT");

	struct zcan_frame msg;
	u8_t rx_frame[MCP2515_FRAME_LEN];
	u8_t nm;

	/* Address Pointer selection */
	nm = 2 * rx_idx;

	/* Fetch rx buffer */
	mcp2515_cmd_read_rx_buffer(dev_data, nm, rx_frame, sizeof(rx_frame));
	mcp2515_convert_mcp2515frame_to_zcanframe(rx_frame, &msg);
	mcp2515_rx_filter(dev_data, &msg);
}

static void mcp2515_tx_done(struct mcp2515_data *dev_data,
			    struct mcp2515_tx_cb *tx_cb, u8_t tx_idx)
{

	k_delayed_work_cancel(&tx_cb->to_work);
	tx_cb->cb(0, tx_cb->cb_arg);

	atomic_clear_bit(dev_data->tx_busy_map, BIT(tx_idx));
	k_sem_give(&dev_data->tx_sem);
}

static enum can_state mcp2515_get_state(struct mcp2515_data *dev_data,
					struct can_bus_err_cnt *err_cnt)
{
	u8_t eflg;
	u8_t err_cnt_buf[2];
	int ret;

	ret = mcp2515_cmd_read_reg(dev_data, MCP2515_ADDR_EFLG, &eflg,
				   sizeof(eflg));
	if (ret < 0) {
		LOG_ERR("Failed to read error register [%d]", ret);
		return CAN_BUS_UNKNOWN;
	}

	if (err_cnt) {
		ret = mcp2515_cmd_read_reg(dev_data, MCP2515_ADDR_TEC,
					   err_cnt_buf, sizeof(err_cnt_buf));
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

static enum can_state mcp2515_get_state_api(struct device *dev,
					    struct can_bus_err_cnt *err_cnt)
{
	return mcp2515_get_state(DEV_DATA(dev), err_cnt);
}

static void mcp2515_handle_errors(struct mcp2515_data *dev_data)
{
	can_state_change_isr_t state_change_isr = dev_data->state_change_isr;
	enum can_state state;
	struct can_bus_err_cnt err_cnt;

	state = mcp2515_get_state(dev_data, state_change_isr ? &err_cnt : NULL);

	if (state_change_isr && dev_data->old_state != state) {
		dev_data->old_state = state;
		state_change_isr(state, err_cnt);
	}
}

#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
static void mcp2515_recover(struct device *dev, s32_t timeout)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(timeout);
}
#endif

static void mcp2515_handle_interrupts(struct k_work *work)
{
	struct mcp2515_data *dev_data =
			      CONTAINER_OF(work, struct mcp2515_data, int_work);
	const struct mcp2515_config *dev_cfg = DEV_CFG(dev_data->this_device);
	u32_t pin;
	int ret;
	u8_t canintf;

	/* Loop until INT pin is high (all interrupt flags handled) */
	while (1) {
		ret = mcp2515_cmd_read_reg(dev_data, MCP2515_ADDR_CANINTF,
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
			mcp2515_rx(dev_data, 0);

			/* RX0IF flag cleared automatically during read */
			canintf &= ~MCP2515_CANINTF_RX0IF;
		}

		if (canintf & MCP2515_CANINTF_RX1IF) {
			mcp2515_rx(dev_data, 1);

			/* RX1IF flag cleared automatically during read */
			canintf &= ~MCP2515_CANINTF_RX1IF;
		}

		if (canintf & MCP2515_CANINTF_TX0IF) {
			mcp2515_tx_done(dev_data, &dev_data->tx_cb0, 0);
		}

		if (canintf & MCP2515_CANINTF_TX1IF) {
			mcp2515_tx_done(dev_data, &dev_data->tx_cb1, 1);
		}

		if (canintf & MCP2515_CANINTF_TX2IF) {
			mcp2515_tx_done(dev_data, &dev_data->tx_cb2, 2);
		}

		if (canintf & MCP2515_CANINTF_ERRIF) {
			mcp2515_handle_errors(dev_data);
		}

		if (canintf != 0) {
			/* Clear remaining flags */
			mcp2515_cmd_bit_modify(dev_data, MCP2515_ADDR_CANINTF,
					       canintf, ~canintf);
		}

		/* Break from loop if INT pin is no longer low */
		ret = gpio_pin_read(dev_data->int_gpio, dev_cfg->int_pin, &pin);
		if (ret != 0) {
			LOG_ERR("Couldn't read INT pin");
		} else if (pin != 0) {
			/* All interrupt flags handled */
			break;
		}
	}
}

static void mcp2515_int_gpio_callback(struct device *dev,
				      struct gpio_callback *cb, u32_t pins)
{
	struct mcp2515_data *dev_data =
		CONTAINER_OF(cb, struct mcp2515_data, int_gpio_cb);

	k_work_submit_to_queue(&mcp2515_workq, &dev_data->int_work);
}

static const struct can_driver_api can_api_funcs = {
	.configure = mcp2515_configure,
	.send = mcp2515_send,
	.attach_isr = mcp2515_attach_isr,
	.detach = mcp2515_detach,
	.get_state = mcp2515_get_state_api,
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

	k_mutex_init(&dev_data->mutex);
	k_sem_init(&dev_data->tx_sem, MCP2515_TX_CNT, MCP2515_TX_CNT);

	/* SPI config */
	dev_data->spi_cfg.operation = SPI_WORD_SET(8);
	dev_data->spi_cfg.frequency = dev_cfg->spi_freq;
	dev_data->spi_cfg.slave = dev_cfg->spi_slave;

	dev_data->spi = device_get_binding(dev_cfg->spi_port);
	if (!dev_data->spi) {
		LOG_ERR("SPI master port %s not found", dev_cfg->spi_port);
		return -EINVAL;
	}

#ifdef DT_INST_0_MICROCHIP_MCP2515_CS_GPIOS_PIN
	dev_data->spi_cs_ctrl.gpio_dev =
		device_get_binding(dev_cfg->spi_cs_port);
	if (!dev_data->spi_cs_ctrl.gpio_dev) {
		LOG_ERR("Unable to get GPIO SPI CS device");
		return -ENODEV;
	}

	dev_data->spi_cs_ctrl.gpio_pin = dev_cfg->spi_cs_pin;
	dev_data->spi_cs_ctrl.delay = 0U;

	dev_data->spi_cfg.cs = &dev_data->spi_cs_ctrl;
#else
	dev_data->spi_cfg.cs = NULL;
#endif  /* DT_INST_0_MICROCHIP_MCP2515_CS_GPIOS_PIN */

	/* Reset MCP2515 */
	if (mcp2515_cmd_soft_reset(dev_data)) {
		LOG_ERR("Soft-reset failed");
		return -EIO;
	}

	/* Initialize interrupt handling  */
	k_work_init(&dev_data->int_work, mcp2515_handle_interrupts);

	k_delayed_work_init(&dev_data->tx_cb0.to_work, mcp2515_tx0_abort_handler);
	k_delayed_work_init(&dev_data->tx_cb1.to_work, mcp2515_tx1_abort_handler);
	k_delayed_work_init(&dev_data->tx_cb2.to_work, mcp2515_tx2_abort_handler);

	dev_data->int_gpio = device_get_binding(dev_cfg->int_port);
	if (dev_data->int_gpio == NULL) {
		LOG_ERR("GPIO port %s not found", dev_cfg->int_port);
		return -EINVAL;
	}

	if (gpio_pin_configure(dev_data->int_gpio, dev_cfg->int_pin,
			       (GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE
				| GPIO_INT_ACTIVE_LOW | GPIO_INT_DEBOUNCE))) {
		LOG_ERR("Unable to configure GPIO pin %u", dev_cfg->int_pin);
		return -EINVAL;
	}

	gpio_init_callback(&(dev_data->int_gpio_cb), mcp2515_int_gpio_callback,
			   BIT(dev_cfg->int_pin));

	if (gpio_add_callback(dev_data->int_gpio, &(dev_data->int_gpio_cb))) {
		return -EINVAL;
	}

	if (gpio_pin_enable_callback(dev_data->int_gpio, dev_cfg->int_pin)) {
		return -EINVAL;
	}

	k_work_q_start(&mcp2515_workq, mcp2515_int_work_stack,
			K_THREAD_STACK_SIZEOF(mcp2515_int_work_stack),
			K_PRIO_COOP(CONFIG_CAN_MCP2515_INT_THREAD_PRIO));
	k_thread_name_set(&mcp2515_workq.thread, "mcp2515_int_work");

	(void)memset(dev_data->rx_cb, 0, sizeof(dev_data->rx_cb));
	(void)memset(dev_data->filter, 0, sizeof(dev_data->filter));
	dev_data->old_state = CAN_ERROR_ACTIVE;

	ret = mcp2515_configure(dev, CAN_NORMAL_MODE, dev_cfg->bus_speed);

	return ret;
}

#ifdef CONFIG_CAN_1

static struct mcp2515_data mcp2515_data_1 = {
	.filter_usage = 0U,
};

static const struct mcp2515_config mcp2515_config_1 = {
	.spi_port = DT_INST_0_MICROCHIP_MCP2515_BUS_NAME,
	.spi_freq = DT_INST_0_MICROCHIP_MCP2515_SPI_MAX_FREQUENCY,
	.spi_slave = DT_INST_0_MICROCHIP_MCP2515_BASE_ADDRESS,
	.int_pin = DT_INST_0_MICROCHIP_MCP2515_INT_GPIOS_PIN,
	.int_port = DT_INST_0_MICROCHIP_MCP2515_INT_GPIOS_CONTROLLER,
#ifdef DT_INST_0_MICROCHIP_MCP2515_CS_GPIOS_PIN
	.spi_cs_pin = DT_INST_0_MICROCHIP_MCP2515_CS_GPIOS_PIN,
	.spi_cs_port = DT_INST_0_MICROCHIP_MCP2515_CS_GPIOS_CONTROLLER,
#endif  /* DT_INST_0_MICROCHIP_MCP2515_CS_GPIOS_PIN */
	.tq_sjw = DT_INST_0_MICROCHIP_MCP2515_SJW,
	.tq_prop = DT_INST_0_MICROCHIP_MCP2515_PROP_SEG,
	.tq_bs1 = DT_INST_0_MICROCHIP_MCP2515_PHASE_SEG1,
	.tq_bs2 = DT_INST_0_MICROCHIP_MCP2515_PHASE_SEG2,
	.bus_speed = DT_INST_0_MICROCHIP_MCP2515_BUS_SPEED,
	.osc_freq = DT_INST_0_MICROCHIP_MCP2515_OSC_FREQ
};

DEVICE_AND_API_INIT(can_mcp2515_1, DT_INST_0_MICROCHIP_MCP2515_LABEL, &mcp2515_init,
		    &mcp2515_data_1, &mcp2515_config_1, POST_KERNEL,
		    CONFIG_CAN_MCP2515_INIT_PRIORITY, &can_api_funcs);

#endif /* CONFIG_CAN_1 */
