/*
 * Copyright (c) 2018 Karsten Koenig
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_mcp2515

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/can/transceiver.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(can_mcp2515, CONFIG_CAN_LOG_LEVEL);

#include "can_mcp2515.h"

/* Timeout for changing mode */
#define MCP2515_MODE_CHANGE_TIMEOUT_USEC 1000
#define MCP2515_MODE_CHANGE_RETRIES      100
#define MCP2515_MODE_CHANGE_DELAY	    \
	K_USEC(MCP2515_MODE_CHANGE_TIMEOUT_USEC / MCP2515_MODE_CHANGE_RETRIES)

static int mcp2515_cmd_soft_reset(const struct device *dev)
{
	const struct mcp2515_config *dev_cfg = dev->config;

	uint8_t cmd_buf[] = { MCP2515_OPCODE_RESET };

	const struct spi_buf tx_buf = {
		.buf = cmd_buf, .len = sizeof(cmd_buf),
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf, .count = 1U
	};

	return spi_write_dt(&dev_cfg->bus, &tx);
}

static int mcp2515_cmd_bit_modify(const struct device *dev, uint8_t reg_addr,
				  uint8_t mask,
				  uint8_t data)
{
	const struct mcp2515_config *dev_cfg = dev->config;

	uint8_t cmd_buf[] = { MCP2515_OPCODE_BIT_MODIFY, reg_addr, mask, data };

	const struct spi_buf tx_buf = {
		.buf = cmd_buf, .len = sizeof(cmd_buf),
	};
	const struct spi_buf_set tx = {
		.buffers = &tx_buf, .count = 1U
	};

	return spi_write_dt(&dev_cfg->bus, &tx);
}

static int mcp2515_cmd_write_reg(const struct device *dev, uint8_t reg_addr,
				 uint8_t *buf_data, uint8_t buf_len)
{
	const struct mcp2515_config *dev_cfg = dev->config;

	uint8_t cmd_buf[] = { MCP2515_OPCODE_WRITE, reg_addr };

	struct spi_buf tx_buf[] = {
		{ .buf = cmd_buf, .len = sizeof(cmd_buf) },
		{ .buf = buf_data, .len = buf_len }
	};
	const struct spi_buf_set tx = {
		.buffers = tx_buf, .count = ARRAY_SIZE(tx_buf)
	};

	return spi_write_dt(&dev_cfg->bus, &tx);
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
static int mcp2515_cmd_load_tx_buffer(const struct device *dev, uint8_t abc,
				      uint8_t *buf_data, uint8_t buf_len)
{
	const struct mcp2515_config *dev_cfg = dev->config;

	__ASSERT(abc <= 5, "abc <= 5");

	uint8_t cmd_buf[] = { MCP2515_OPCODE_LOAD_TX_BUFFER | abc };

	struct spi_buf tx_buf[] = {
		{ .buf = cmd_buf, .len = sizeof(cmd_buf) },
		{ .buf = buf_data, .len = buf_len }
	};
	const struct spi_buf_set tx = {
		.buffers = tx_buf, .count = ARRAY_SIZE(tx_buf)
	};

	return spi_write_dt(&dev_cfg->bus, &tx);
}

/*
 * Request-to-Send Instruction
 *
 * Parameter nnn is the combination of bits at positions 0, 1 and 2 in the RTS
 * opcode that respectively initiate transmission for buffers TXB0, TXB1 and
 * TXB2.
 */
static int mcp2515_cmd_rts(const struct device *dev, uint8_t nnn)
{
	const struct mcp2515_config *dev_cfg = dev->config;

	__ASSERT(nnn < BIT(MCP2515_TX_CNT), "nnn < BIT(MCP2515_TX_CNT)");

	uint8_t cmd_buf[] = { MCP2515_OPCODE_RTS | nnn };

	struct spi_buf tx_buf[] = {
		{ .buf = cmd_buf, .len = sizeof(cmd_buf) }
	};
	const struct spi_buf_set tx = {
		.buffers = tx_buf, .count = ARRAY_SIZE(tx_buf)
	};

	return spi_write_dt(&dev_cfg->bus, &tx);
}

static int mcp2515_cmd_read_reg(const struct device *dev, uint8_t reg_addr,
				uint8_t *buf_data, uint8_t buf_len)
{
	const struct mcp2515_config *dev_cfg = dev->config;

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

	return spi_transceive_dt(&dev_cfg->bus, &tx, &rx);
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
static int mcp2515_cmd_read_rx_buffer(const struct device *dev, uint8_t nm,
				      uint8_t *buf_data, uint8_t buf_len)
{
	const struct mcp2515_config *dev_cfg = dev->config;

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

	return spi_transceive_dt(&dev_cfg->bus, &tx, &rx);
}

static void mcp2515_convert_canframe_to_mcp2515frame(const struct can_frame
						     *source, uint8_t *target)
{
	uint8_t rtr;
	uint8_t dlc;
	uint8_t data_idx;

	if ((source->flags & CAN_FRAME_IDE) != 0) {
		target[MCP2515_FRAME_OFFSET_SIDH] = source->id >> 21;
		target[MCP2515_FRAME_OFFSET_SIDL] =
			(((source->id >> 18) & 0x07) << 5) | (BIT(3)) |
			((source->id >> 16) & 0x03);
		target[MCP2515_FRAME_OFFSET_EID8] = source->id >> 8;
		target[MCP2515_FRAME_OFFSET_EID0] = source->id;
	} else {
		target[MCP2515_FRAME_OFFSET_SIDH] = source->id >> 3;
		target[MCP2515_FRAME_OFFSET_SIDL] =
			(source->id & 0x07) << 5;
	}

	rtr = (source->flags & CAN_FRAME_RTR) != 0 ? BIT(6) : 0;
	dlc = (source->dlc) & 0x0F;

	target[MCP2515_FRAME_OFFSET_DLC] = rtr | dlc;

	if (rtr == 0U) {
		for (data_idx = 0U; data_idx < dlc; data_idx++) {
			target[MCP2515_FRAME_OFFSET_D0 + data_idx] =
				source->data[data_idx];
		}
	}
}

static void mcp2515_convert_mcp2515frame_to_canframe(const uint8_t *source,
						     struct can_frame *target)
{
	uint8_t data_idx;

	memset(target, 0, sizeof(*target));

	if (source[MCP2515_FRAME_OFFSET_SIDL] & BIT(3)) {
		target->flags |= CAN_FRAME_IDE;
		target->id =
			(source[MCP2515_FRAME_OFFSET_SIDH] << 21) |
			((source[MCP2515_FRAME_OFFSET_SIDL] >> 5) << 18) |
			((source[MCP2515_FRAME_OFFSET_SIDL] & 0x03) << 16) |
			(source[MCP2515_FRAME_OFFSET_EID8] << 8) |
			source[MCP2515_FRAME_OFFSET_EID0];
	} else {
		target->id = (source[MCP2515_FRAME_OFFSET_SIDH] << 3) |
				 (source[MCP2515_FRAME_OFFSET_SIDL] >> 5);
	}

	target->dlc = source[MCP2515_FRAME_OFFSET_DLC] & 0x0F;

	if ((source[MCP2515_FRAME_OFFSET_DLC] & BIT(6)) != 0) {
		target->flags |= CAN_FRAME_RTR;
	} else {
		for (data_idx = 0U; data_idx < target->dlc; data_idx++) {
			target->data[data_idx] = source[MCP2515_FRAME_OFFSET_D0 +
							data_idx];
		}
	}
}

const int mcp2515_set_mode_int(const struct device *dev, uint8_t mcp2515_mode)
{
	int retries = MCP2515_MODE_CHANGE_RETRIES;
	uint8_t canstat;

	mcp2515_cmd_bit_modify(dev, MCP2515_ADDR_CANCTRL,
			       MCP2515_CANCTRL_MODE_MASK,
			       mcp2515_mode << MCP2515_CANCTRL_MODE_POS);
	mcp2515_cmd_read_reg(dev, MCP2515_ADDR_CANSTAT, &canstat, 1);

	while (((canstat & MCP2515_CANSTAT_MODE_MASK) >> MCP2515_CANSTAT_MODE_POS)
		!= mcp2515_mode) {
		if (--retries < 0) {
			LOG_ERR("Timeout trying to set MCP2515 operation mode");
			return -EIO;
		}

		k_sleep(MCP2515_MODE_CHANGE_DELAY);
		mcp2515_cmd_read_reg(dev, MCP2515_ADDR_CANSTAT, &canstat, 1);
	}

	return 0;
}

static void mcp2515_tx_done(const struct device *dev, uint8_t tx_idx, int status)
{
	struct mcp2515_data *dev_data = dev->data;
	can_tx_callback_t callback = dev_data->tx_cb[tx_idx].cb;

	if (callback != NULL) {
		callback(dev, status, dev_data->tx_cb[tx_idx].cb_arg);
		dev_data->tx_cb[tx_idx].cb = NULL;

		k_mutex_lock(&dev_data->mutex, K_FOREVER);
		dev_data->tx_busy_map &= ~BIT(tx_idx);
		k_mutex_unlock(&dev_data->mutex);
		k_sem_give(&dev_data->tx_sem);
	}
}

static int mcp2515_get_core_clock(const struct device *dev, uint32_t *rate)
{
	const struct mcp2515_config *dev_cfg = dev->config;

	*rate = dev_cfg->osc_freq / 2;
	return 0;
}

static int mcp2515_get_max_filters(const struct device *dev, bool ide)
{
	ARG_UNUSED(ide);

	return CONFIG_CAN_MAX_FILTER;
}

static int mcp2515_set_timing(const struct device *dev,
			      const struct can_timing *timing)
{
	struct mcp2515_data *dev_data = dev->data;
	int ret;

	if (!timing) {
		return -EINVAL;
	}

	if (dev_data->common.started) {
		return -EBUSY;
	}

	/* CNF3, CNF2, CNF1, CANINTE */
	uint8_t config_buf[4];

	/* CNF1; SJW<7:6> | BRP<5:0> */
	__ASSERT(timing->prescaler > 0, "Prescaler should be bigger than zero");
	uint8_t brp = timing->prescaler - 1;
	uint8_t sjw = (timing->sjw - 1) << 6;
	uint8_t cnf1 = sjw | brp;

	/* CNF2; BTLMODE<7>|SAM<6>|PHSEG1<5:3>|PRSEG<2:0> */
	const uint8_t btlmode = 1 << 7;
	const uint8_t sam = 0 << 6;
	const uint8_t phseg1 = (timing->phase_seg1 - 1) << 3;
	const uint8_t prseg = (timing->prop_seg - 1);

	const uint8_t cnf2 = btlmode | sam | phseg1 | prseg;

	/* CNF3; SOF<7>|WAKFIL<6>|UND<5:3>|PHSEG2<2:0> */
	const uint8_t sof = 0 << 7;
	const uint8_t wakfil = 0 << 6;
	const uint8_t und = 0 << 3;
	const uint8_t phseg2 = (timing->phase_seg2 - 1);

	const uint8_t cnf3 = sof | wakfil | und | phseg2;

	const uint8_t caninte = MCP2515_INTE_RX0IE | MCP2515_INTE_RX1IE |
			     MCP2515_INTE_TX0IE | MCP2515_INTE_TX1IE |
			     MCP2515_INTE_TX2IE | MCP2515_INTE_ERRIE;

	/* Receive everything, filtering done in driver, RXB0 roll over into
	 * RXB1 */
	const uint8_t rx0_ctrl = BIT(6) | BIT(5) | BIT(2);
	const uint8_t rx1_ctrl = BIT(6) | BIT(5);

	config_buf[0] = cnf3;
	config_buf[1] = cnf2;
	config_buf[2] = cnf1;
	config_buf[3] = caninte;

	k_mutex_lock(&dev_data->mutex, K_FOREVER);

	ret = mcp2515_cmd_write_reg(dev, MCP2515_ADDR_CNF3, config_buf,
				    sizeof(config_buf));
	if (ret < 0) {
		LOG_ERR("Failed to write the configuration [%d]", ret);
		goto done;
	}

	ret = mcp2515_cmd_bit_modify(dev, MCP2515_ADDR_RXB0CTRL, rx0_ctrl,
				     rx0_ctrl);
	if (ret < 0) {
		LOG_ERR("Failed to write RXB0CTRL [%d]", ret);
		goto done;
	}

	ret = mcp2515_cmd_bit_modify(dev, MCP2515_ADDR_RXB1CTRL, rx1_ctrl,
				     rx1_ctrl);
	if (ret < 0) {
		LOG_ERR("Failed to write RXB1CTRL [%d]", ret);
		goto done;
	}

done:
	k_mutex_unlock(&dev_data->mutex);
	return ret;
}

static int mcp2515_get_capabilities(const struct device *dev, can_mode_t *cap)
{
	ARG_UNUSED(dev);

	*cap = CAN_MODE_NORMAL | CAN_MODE_LISTENONLY | CAN_MODE_LOOPBACK;

	return 0;
}

static int mcp2515_start(const struct device *dev)
{
	const struct mcp2515_config *dev_cfg = dev->config;
	struct mcp2515_data *dev_data = dev->data;
	int ret;

	if (dev_data->common.started) {
		return -EALREADY;
	}

	if (dev_cfg->common.phy != NULL) {
		ret = can_transceiver_enable(dev_cfg->common.phy, dev_data->common.mode);
		if (ret != 0) {
			LOG_ERR("Failed to enable CAN transceiver [%d]", ret);
			return ret;
		}
	}

	CAN_STATS_RESET(dev);

	k_mutex_lock(&dev_data->mutex, K_FOREVER);

	ret = mcp2515_set_mode_int(dev, dev_data->mcp2515_mode);
	if (ret < 0) {
		LOG_ERR("Failed to set the mode [%d]", ret);

		if (dev_cfg->common.phy != NULL) {
			/* Attempt to disable the CAN transceiver in case of error */
			(void)can_transceiver_disable(dev_cfg->common.phy);
		}
	} else {
		dev_data->common.started = true;
	}

	k_mutex_unlock(&dev_data->mutex);

	return ret;
}

static int mcp2515_stop(const struct device *dev)
{
	const struct mcp2515_config *dev_cfg = dev->config;
	struct mcp2515_data *dev_data = dev->data;
	int ret;
	int i;

	if (!dev_data->common.started) {
		return -EALREADY;
	}

	k_mutex_lock(&dev_data->mutex, K_FOREVER);

	/* Abort any pending transmissions before entering configuration mode */
	mcp2515_cmd_bit_modify(dev, MCP2515_ADDR_TXB0CTRL,
			       MCP2515_TXBNCTRL_TXREQ_MASK, 0);
#if MCP2515_TX_CNT == 2
	mcp2515_cmd_bit_modify(dev, MCP2515_ADDR_TXB1CTRL,
			       MCP2515_TXBNCTRL_TXREQ_MASK, 0);
#endif /*  MCP2515_TX_CNT == 2 */
#if MCP2515_TX_CNT == 3
	mcp2515_cmd_bit_modify(dev, MCP2515_ADDR_TXB2CTRL,
			       MCP2515_TXBNCTRL_TXREQ_MASK, 0);
#endif /* MCP2515_TX_CNT == 3 */

	ret = mcp2515_set_mode_int(dev, MCP2515_MODE_CONFIGURATION);
	if (ret < 0) {
		LOG_ERR("Failed to enter configuration mode [%d]", ret);
		k_mutex_unlock(&dev_data->mutex);
		return ret;
	}

	dev_data->common.started = false;

	k_mutex_unlock(&dev_data->mutex);

	for (i = 0; i < MCP2515_TX_CNT; i++) {
		mcp2515_tx_done(dev, i, -ENETDOWN);
	}

	if (dev_cfg->common.phy != NULL) {
		ret = can_transceiver_disable(dev_cfg->common.phy);
		if (ret != 0) {
			LOG_ERR("Failed to disable CAN transceiver [%d]", ret);
			return ret;
		}
	}

	return 0;
}

static int mcp2515_set_mode(const struct device *dev, can_mode_t mode)
{
	struct mcp2515_data *dev_data = dev->data;

	if (dev_data->common.started) {
		return -EBUSY;
	}

	switch (mode) {
	case CAN_MODE_NORMAL:
		dev_data->mcp2515_mode = MCP2515_MODE_NORMAL;
		break;
	case CAN_MODE_LISTENONLY:
		dev_data->mcp2515_mode = MCP2515_MODE_SILENT;
		break;
	case CAN_MODE_LOOPBACK:
		dev_data->mcp2515_mode = MCP2515_MODE_LOOPBACK;
		break;
	default:
		LOG_ERR("Unsupported CAN Mode %u", mode);
		return -ENOTSUP;
	}

	dev_data->common.mode = mode;

	return 0;
}

static int mcp2515_send(const struct device *dev,
			const struct can_frame *frame,
			k_timeout_t timeout, can_tx_callback_t callback,
			void *user_data)
{
	struct mcp2515_data *dev_data = dev->data;
	uint8_t tx_idx = 0U;
	uint8_t abc;
	uint8_t nnn;
	uint8_t len;
	uint8_t tx_frame[MCP2515_FRAME_LEN];

	if (frame->dlc > CAN_MAX_DLC) {
		LOG_ERR("DLC of %d exceeds maximum (%d)",
			frame->dlc, CAN_MAX_DLC);
		return -EINVAL;
	}

	if ((frame->flags & ~(CAN_FRAME_IDE | CAN_FRAME_RTR)) != 0) {
		LOG_ERR("unsupported CAN frame flags 0x%02x", frame->flags);
		return -ENOTSUP;
	}

	if (!dev_data->common.started) {
		return -ENETDOWN;
	}

	if (k_sem_take(&dev_data->tx_sem, timeout) != 0) {
		return -EAGAIN;
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
		return -EIO;
	}

	dev_data->tx_cb[tx_idx].cb = callback;
	dev_data->tx_cb[tx_idx].cb_arg = user_data;

	mcp2515_convert_canframe_to_mcp2515frame(frame, tx_frame);

	/* Address Pointer selection */
	abc = 2 * tx_idx;

	/* Calculate minimum length to transfer */
	len = sizeof(tx_frame) - CAN_MAX_DLC + frame->dlc;

	mcp2515_cmd_load_tx_buffer(dev, abc, tx_frame, len);

	/* request tx slot transmission */
	nnn = BIT(tx_idx);
	mcp2515_cmd_rts(dev, nnn);

	return 0;
}

static int mcp2515_add_rx_filter(const struct device *dev,
				 can_rx_callback_t rx_cb,
				 void *cb_arg,
				 const struct can_filter *filter)
{
	struct mcp2515_data *dev_data = dev->data;
	int filter_id = 0;

	__ASSERT(rx_cb != NULL, "response_ptr can not be null");

	if ((filter->flags & ~(CAN_FILTER_IDE)) != 0) {
		LOG_ERR("unsupported CAN filter flags 0x%02x", filter->flags);
		return -ENOTSUP;
	}

	k_mutex_lock(&dev_data->mutex, K_FOREVER);

	/* find free filter */
	while ((BIT(filter_id) & dev_data->filter_usage)
	       && (filter_id < CONFIG_CAN_MAX_FILTER)) {
		filter_id++;
	}

	/* setup filter */
	if (filter_id < CONFIG_CAN_MAX_FILTER) {
		dev_data->filter_usage |= BIT(filter_id);

		dev_data->filter[filter_id] = *filter;
		dev_data->rx_cb[filter_id] = rx_cb;
		dev_data->cb_arg[filter_id] = cb_arg;

	} else {
		filter_id = -ENOSPC;
	}

	k_mutex_unlock(&dev_data->mutex);

	return filter_id;
}

static void mcp2515_remove_rx_filter(const struct device *dev, int filter_id)
{
	struct mcp2515_data *dev_data = dev->data;

	if (filter_id < 0 || filter_id >= CONFIG_CAN_MAX_FILTER) {
		LOG_ERR("filter ID %d out of bounds", filter_id);
		return;
	}

	k_mutex_lock(&dev_data->mutex, K_FOREVER);
	dev_data->filter_usage &= ~BIT(filter_id);
	k_mutex_unlock(&dev_data->mutex);
}

static void mcp2515_set_state_change_callback(const struct device *dev,
					      can_state_change_callback_t cb,
					      void *user_data)
{
	struct mcp2515_data *dev_data = dev->data;

	dev_data->common.state_change_cb = cb;
	dev_data->common.state_change_cb_user_data = user_data;
}

static void mcp2515_rx_filter(const struct device *dev,
			      struct can_frame *frame)
{
	struct mcp2515_data *dev_data = dev->data;
	uint8_t filter_id = 0U;
	can_rx_callback_t callback;
	struct can_frame tmp_frame;

#ifndef CONFIG_CAN_ACCEPT_RTR
	if ((frame->flags & CAN_FRAME_RTR) != 0U) {
		return;
	}
#endif /* !CONFIG_CAN_ACCEPT_RTR */

	k_mutex_lock(&dev_data->mutex, K_FOREVER);

	for (; filter_id < CONFIG_CAN_MAX_FILTER; filter_id++) {
		if (!(BIT(filter_id) & dev_data->filter_usage)) {
			continue; /* filter slot empty */
		}

		if (!can_frame_matches_filter(frame, &dev_data->filter[filter_id])) {
			continue; /* filter did not match */
		}

		callback = dev_data->rx_cb[filter_id];
		/*Make a temporary copy in case the user modifies the message*/
		tmp_frame = *frame;

		callback(dev, &tmp_frame, dev_data->cb_arg[filter_id]);
	}

	k_mutex_unlock(&dev_data->mutex);
}

static void mcp2515_rx(const struct device *dev, uint8_t rx_idx)
{
	__ASSERT(rx_idx < MCP2515_RX_CNT, "rx_idx < MCP2515_RX_CNT");

	struct can_frame frame;
	uint8_t rx_frame[MCP2515_FRAME_LEN];
	uint8_t nm;

	/* Address Pointer selection */
	nm = 2 * rx_idx;

	/* Fetch rx buffer */
	mcp2515_cmd_read_rx_buffer(dev, nm, rx_frame, sizeof(rx_frame));
	mcp2515_convert_mcp2515frame_to_canframe(rx_frame, &frame);
	mcp2515_rx_filter(dev, &frame);
}

static int mcp2515_get_state(const struct device *dev, enum can_state *state,
			     struct can_bus_err_cnt *err_cnt)
{
	struct mcp2515_data *dev_data = dev->data;
	uint8_t eflg;
	uint8_t err_cnt_buf[2];
	int ret;

	ret = mcp2515_cmd_read_reg(dev, MCP2515_ADDR_EFLG, &eflg, sizeof(eflg));
	if (ret < 0) {
		LOG_ERR("Failed to read error register [%d]", ret);
		return -EIO;
	}

	if (state != NULL) {
		if (!dev_data->common.started) {
			*state = CAN_STATE_STOPPED;
		} else if (eflg & MCP2515_EFLG_TXBO) {
			*state = CAN_STATE_BUS_OFF;
		} else if ((eflg & MCP2515_EFLG_RXEP) || (eflg & MCP2515_EFLG_TXEP)) {
			*state = CAN_STATE_ERROR_PASSIVE;
		} else if (eflg & MCP2515_EFLG_EWARN) {
			*state = CAN_STATE_ERROR_WARNING;
		} else {
			*state = CAN_STATE_ERROR_ACTIVE;
		}
	}

	if (err_cnt != NULL) {
		ret = mcp2515_cmd_read_reg(dev, MCP2515_ADDR_TEC, err_cnt_buf,
					   sizeof(err_cnt_buf));
		if (ret < 0) {
			LOG_ERR("Failed to read error counters [%d]", ret);
			return -EIO;
		}

		err_cnt->tx_err_cnt = err_cnt_buf[0];
		err_cnt->rx_err_cnt = err_cnt_buf[1];
	}

#ifdef CONFIG_CAN_STATS
	if ((eflg & (MCP2515_EFLG_RX0OVR | MCP2515_EFLG_RX1OVR)) != 0U) {
		CAN_STATS_RX_OVERRUN_INC(dev);

		ret = mcp2515_cmd_bit_modify(dev, MCP2515_ADDR_EFLG,
					     eflg & (MCP2515_EFLG_RX0OVR | MCP2515_EFLG_RX1OVR),
					     0U);
		if (ret < 0) {
			LOG_ERR("Failed to clear RX overrun flags [%d]", ret);
			return -EIO;
		}
	}
#endif /* CONFIG_CAN_STATS */

	return 0;
}

static void mcp2515_handle_errors(const struct device *dev)
{
	struct mcp2515_data *dev_data = dev->data;
	can_state_change_callback_t state_change_cb = dev_data->common.state_change_cb;
	void *state_change_cb_data = dev_data->common.state_change_cb_user_data;
	enum can_state state;
	struct can_bus_err_cnt err_cnt;
	int err;

	err = mcp2515_get_state(dev, &state, state_change_cb ? &err_cnt : NULL);
	if (err != 0) {
		LOG_ERR("Failed to get CAN controller state [%d]", err);
		return;
	}

	if (state_change_cb && dev_data->old_state != state) {
		dev_data->old_state = state;
		state_change_cb(dev, state, err_cnt, state_change_cb_data);
	}
}

static void mcp2515_handle_interrupts(const struct device *dev)
{
	const struct mcp2515_config *dev_cfg = dev->config;
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
			mcp2515_tx_done(dev, 0, 0);
		}

		if (canintf & MCP2515_CANINTF_TX1IF) {
			mcp2515_tx_done(dev, 1, 0);
		}

		if (canintf & MCP2515_CANINTF_TX2IF) {
			mcp2515_tx_done(dev, 2, 0);
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
		ret = gpio_pin_get_dt(&dev_cfg->int_gpio);
		if (ret < 0) {
			LOG_ERR("Couldn't read INT pin");
		} else if (ret == 0) {
			/* All interrupt flags handled */
			break;
		}
	}
}

static void mcp2515_int_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	const struct device *dev = p1;
	struct mcp2515_data *dev_data = dev->data;

	while (1) {
		k_sem_take(&dev_data->int_sem, K_FOREVER);
		mcp2515_handle_interrupts(dev);
	}
}

static void mcp2515_int_gpio_callback(const struct device *dev,
				      struct gpio_callback *cb, uint32_t pins)
{
	struct mcp2515_data *dev_data =
		CONTAINER_OF(cb, struct mcp2515_data, int_gpio_cb);

	k_sem_give(&dev_data->int_sem);
}

static const struct can_driver_api can_api_funcs = {
	.get_capabilities = mcp2515_get_capabilities,
	.set_timing = mcp2515_set_timing,
	.start = mcp2515_start,
	.stop = mcp2515_stop,
	.set_mode = mcp2515_set_mode,
	.send = mcp2515_send,
	.add_rx_filter = mcp2515_add_rx_filter,
	.remove_rx_filter = mcp2515_remove_rx_filter,
	.get_state = mcp2515_get_state,
	.set_state_change_callback = mcp2515_set_state_change_callback,
	.get_core_clock = mcp2515_get_core_clock,
	.get_max_filters = mcp2515_get_max_filters,
	.timing_min = {
		.sjw = 0x1,
		.prop_seg = 0x01,
		.phase_seg1 = 0x01,
		.phase_seg2 = 0x02,
		.prescaler = 0x01
	},
	.timing_max = {
		.sjw = 0x04,
		.prop_seg = 0x08,
		.phase_seg1 = 0x08,
		.phase_seg2 = 0x08,
		.prescaler = 0x40
	}
};


static int mcp2515_init(const struct device *dev)
{
	const struct mcp2515_config *dev_cfg = dev->config;
	struct mcp2515_data *dev_data = dev->data;
	struct can_timing timing = { 0 };
	k_tid_t tid;
	int ret;

	k_sem_init(&dev_data->int_sem, 0, 1);
	k_mutex_init(&dev_data->mutex);
	k_sem_init(&dev_data->tx_sem, MCP2515_TX_CNT, MCP2515_TX_CNT);

	if (dev_cfg->common.phy != NULL) {
		if (!device_is_ready(dev_cfg->common.phy)) {
			LOG_ERR("CAN transceiver not ready");
			return -ENODEV;
		}
	}

	if (!spi_is_ready_dt(&dev_cfg->bus)) {
		LOG_ERR("SPI bus %s not ready", dev_cfg->bus.bus->name);
		return -ENODEV;
	}

	/* Reset MCP2515 */
	if (mcp2515_cmd_soft_reset(dev)) {
		LOG_ERR("Soft-reset failed");
		return -EIO;
	}

	/* Initialize interrupt handling  */
	if (!gpio_is_ready_dt(&dev_cfg->int_gpio)) {
		LOG_ERR("Interrupt GPIO port not ready");
		return -ENODEV;
	}

	if (gpio_pin_configure_dt(&dev_cfg->int_gpio, GPIO_INPUT)) {
		LOG_ERR("Unable to configure interrupt GPIO");
		return -EINVAL;
	}

	gpio_init_callback(&(dev_data->int_gpio_cb), mcp2515_int_gpio_callback,
			   BIT(dev_cfg->int_gpio.pin));

	if (gpio_add_callback(dev_cfg->int_gpio.port,
			      &(dev_data->int_gpio_cb))) {
		return -EINVAL;
	}

	if (gpio_pin_interrupt_configure_dt(&dev_cfg->int_gpio,
					    GPIO_INT_EDGE_TO_ACTIVE)) {
		return -EINVAL;
	}

	tid = k_thread_create(&dev_data->int_thread, dev_data->int_thread_stack,
			      dev_cfg->int_thread_stack_size,
			      mcp2515_int_thread, (void *)dev,
			      NULL, NULL, K_PRIO_COOP(dev_cfg->int_thread_priority),
			      0, K_NO_WAIT);
	(void)k_thread_name_set(tid, "mcp2515");

	(void)memset(dev_data->rx_cb, 0, sizeof(dev_data->rx_cb));
	(void)memset(dev_data->filter, 0, sizeof(dev_data->filter));
	dev_data->old_state = CAN_STATE_ERROR_ACTIVE;

	ret = can_calc_timing(dev, &timing, dev_cfg->common.bus_speed,
			      dev_cfg->common.sample_point);
	if (ret == -EINVAL) {
		LOG_ERR("Can't find timing for given param");
		return -EIO;
	}

	LOG_DBG("Presc: %d, BS1: %d, BS2: %d",
		timing.prescaler, timing.phase_seg1, timing.phase_seg2);
	LOG_DBG("Sample-point err : %d", ret);

	k_usleep(MCP2515_OSC_STARTUP_US);

	ret = can_set_timing(dev, &timing);
	if (ret) {
		return ret;
	}

	ret = can_set_mode(dev, CAN_MODE_NORMAL);

	return ret;
}

#define MCP2515_INIT(inst)                                                                         \
	static K_KERNEL_STACK_DEFINE(mcp2515_int_thread_stack_##inst,                              \
				     CONFIG_CAN_MCP2515_INT_THREAD_STACK_SIZE);                    \
                                                                                                   \
	static struct mcp2515_data mcp2515_data_##inst = {                                         \
		.int_thread_stack = mcp2515_int_thread_stack_##inst,                               \
		.tx_busy_map = 0U,                                                                 \
		.filter_usage = 0U,                                                                \
	};                                                                                         \
                                                                                                   \
	static const struct mcp2515_config mcp2515_config_##inst = {                               \
		.common = CAN_DT_DRIVER_CONFIG_INST_GET(inst, 0, 1000000),                         \
		.bus = SPI_DT_SPEC_INST_GET(inst, SPI_WORD_SET(8), 0),                             \
		.int_gpio = GPIO_DT_SPEC_INST_GET(inst, int_gpios),                                \
		.int_thread_stack_size = CONFIG_CAN_MCP2515_INT_THREAD_STACK_SIZE,                 \
		.int_thread_priority = CONFIG_CAN_MCP2515_INT_THREAD_PRIO,                         \
		.osc_freq = DT_INST_PROP(inst, osc_freq),                                          \
	};                                                                                         \
                                                                                                   \
	CAN_DEVICE_DT_INST_DEFINE(inst, mcp2515_init, NULL, &mcp2515_data_##inst,                  \
				  &mcp2515_config_##inst, POST_KERNEL, CONFIG_CAN_INIT_PRIORITY,   \
				  &can_api_funcs);

DT_INST_FOREACH_STATUS_OKAY(MCP2515_INIT)
