/*
 * Copyright (c) 2020 Abram Early
 * Copyright (c) 2023 Andriy Gelman
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_mcp251xfd

#include "can_mcp251xfd.h"

#include <zephyr/device.h>
#include <zephyr/drivers/can/transceiver.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/crc.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(can_mcp251xfd, CONFIG_CAN_LOG_LEVEL);

#define SP_IS_SET(inst) DT_INST_NODE_HAS_PROP(inst, sample_point) ||

/*
 * Macro to exclude the sample point algorithm from compilation if not used
 * Without the macro, the algorithm would always waste ROM
 */
#define USE_SP_ALGO (DT_INST_FOREACH_STATUS_OKAY(SP_IS_SET) 0)

static void mcp251xfd_canframe_to_txobj(const struct can_frame *src, int mailbox_idx,
					struct mcp251xfd_txobj *dst)
{
	memset(dst, 0, sizeof(*dst));

	if ((src->flags & CAN_FRAME_IDE) != 0) {
		dst->id = FIELD_PREP(MCP251XFD_OBJ_ID_SID_MASK, src->id >> 18);
		dst->id |= FIELD_PREP(MCP251XFD_OBJ_ID_EID_MASK, src->id);

		dst->flags |= MCP251XFD_OBJ_FLAGS_IDE;
	} else {
		dst->id = FIELD_PREP(MCP251XFD_OBJ_ID_SID_MASK, src->id);
	}

	if ((src->flags & CAN_FRAME_BRS) != 0) {
		dst->flags |= MCP251XFD_OBJ_FLAGS_BRS;
	}

	dst->flags |= FIELD_PREP(MCP251XFD_OBJ_FLAGS_DLC_MASK, src->dlc);
#if defined(CONFIG_CAN_FD_MODE)
	if ((src->flags & CAN_FRAME_FDF) != 0) {
		dst->flags |= MCP251XFD_OBJ_FLAGS_FDF;
	}
#endif
	dst->flags |= FIELD_PREP(MCP251XFD_OBJ_FLAGS_SEQ_MASK, mailbox_idx);

	dst->id = sys_cpu_to_le32(dst->id);
	dst->flags = sys_cpu_to_le32(dst->flags);

	if ((src->flags & CAN_FRAME_RTR) != 0) {
		dst->flags |= MCP251XFD_OBJ_FLAGS_RTR;
	} else {
		memcpy(dst->data, src->data, MIN(can_dlc_to_bytes(src->dlc), CAN_MAX_DLEN));
	}
}

static void *mcp251xfd_read_reg(const struct device *dev, uint16_t addr, int len)
{
	const struct mcp251xfd_config *dev_cfg = dev->config;
	struct mcp251xfd_data *dev_data = dev->data;
	struct mcp251xfd_spi_data *spi_data = &dev_data->spi_data;
	uint16_t spi_cmd;
	int ret;

	spi_cmd = sys_cpu_to_be16(MCP251XFD_SPI_INSTRUCTION_READ | addr);
	memcpy(&spi_data->header[1], &spi_cmd, sizeof(spi_cmd));

	struct spi_buf tx_buf = {.buf = &spi_data->header[1], .len = MCP251XFD_SPI_CMD_LEN + len};
	struct spi_buf rx_buf = {.buf = &spi_data->header[1], .len = MCP251XFD_SPI_CMD_LEN + len};

	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};
	const struct spi_buf_set rx = {.buffers = &rx_buf, .count = 1};

	ret = spi_transceive_dt(&dev_cfg->bus, &tx, &rx);
	if (ret < 0) {
		return NULL;
	}

	return &spi_data->buf[0];
}

static void *mcp251xfd_read_crc(const struct device *dev, uint16_t addr, int len)
{
	const struct mcp251xfd_config *dev_cfg = dev->config;
	struct mcp251xfd_data *dev_data = dev->data;
	struct mcp251xfd_spi_data *spi_data = &dev_data->spi_data;
	int num_retries = CONFIG_CAN_MCP251XFD_READ_CRC_RETRIES + 1;
	int ret;

	while (num_retries-- > 0) {
		uint16_t crc_in, crc, spi_cmd;

		struct spi_buf tx_buf = {.buf = &spi_data->header[0],
					 .len = MCP251XFD_SPI_CMD_LEN +
						MCP251XFD_SPI_LEN_FIELD_LEN + len +
						MCP251XFD_SPI_CRC_LEN};

		struct spi_buf rx_buf = {.buf = &spi_data->header[0],
					 .len = MCP251XFD_SPI_CMD_LEN +
						MCP251XFD_SPI_LEN_FIELD_LEN + len +
						MCP251XFD_SPI_CRC_LEN};

		const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};
		const struct spi_buf_set rx = {.buffers = &rx_buf, .count = 1};

		spi_cmd = sys_cpu_to_be16(MCP251XFD_SPI_INSTRUCTION_READ_CRC | addr);
		memcpy(&spi_data->header[0], &spi_cmd, sizeof(spi_cmd));
		spi_data->header[2] = len;

		/*
		 * Evaluate initial crc over spi_cmd and length as these value will change after
		 * spi transaction is finished.
		 */
		crc_in = crc16(MCP251XFD_CRC_POLY, MCP251XFD_CRC_SEED,
			       (uint8_t *)(&spi_data->header[0]),
			       MCP251XFD_SPI_CMD_LEN + MCP251XFD_SPI_LEN_FIELD_LEN);

		ret = spi_transceive_dt(&dev_cfg->bus, &tx, &rx);
		if (ret < 0) {
			continue;
		}

		/* Continue crc calculation over the data field and the crc field */
		crc = crc16(MCP251XFD_CRC_POLY, crc_in, &spi_data->buf[0],
			    len + MCP251XFD_SPI_CRC_LEN);
		if (crc == 0) {
			return &spi_data->buf[0];
		}
	}

	return NULL;
}

static inline void *mcp251xfd_get_spi_buf_ptr(const struct device *dev)
{
	struct mcp251xfd_data *dev_data = dev->data;
	struct mcp251xfd_spi_data *spi_data = &dev_data->spi_data;

	return &spi_data->buf[0];
}

static int mcp251xfd_write(const struct device *dev, uint16_t addr, int len)
{
	const struct mcp251xfd_config *dev_cfg = dev->config;
	struct mcp251xfd_data *dev_data = dev->data;
	struct mcp251xfd_spi_data *spi_data = &dev_data->spi_data;
	uint16_t spi_cmd;

	struct spi_buf tx_buf = {.buf = &spi_data->header[1], .len = MCP251XFD_SPI_CMD_LEN + len};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};

	spi_cmd = sys_cpu_to_be16(MCP251XFD_SPI_INSTRUCTION_WRITE | addr);
	memcpy(&spi_data->header[1], &spi_cmd, sizeof(spi_cmd));

	return spi_write_dt(&dev_cfg->bus, &tx);
}

static int mcp251xfd_fifo_write(const struct device *dev, int mailbox_idx,
				const struct can_frame *msg)
{
	uint32_t *regs;
	struct mcp251xfd_txobj *txobj;
	uint8_t *reg_byte;
	uint16_t address;
	int tx_len;
	int ret;

	/* read fifosta and ua at the same time */
	regs = mcp251xfd_read_crc(dev, MCP251XFD_REG_TXQSTA, MCP251XFD_REG_SIZE * 2);
	if (!regs) {
		LOG_ERR("Failed to read 8 bytes from REG_TXQSTA");
		return -EINVAL;
	}

	/* check if fifo is full */
	if (!(regs[0] & MCP251XFD_REG_TXQSTA_TXQNIF)) {
		return -ENOMEM;
	}

	address = MCP251XFD_RAM_START_ADDR + regs[1];

	txobj = mcp251xfd_get_spi_buf_ptr(dev);
	mcp251xfd_canframe_to_txobj(msg, mailbox_idx, txobj);

	tx_len = MCP251XFD_OBJ_HEADER_SIZE;
	if ((msg->flags & CAN_FRAME_RTR) == 0) {
		tx_len += ROUND_UP(can_dlc_to_bytes(msg->dlc), MCP251XFD_RAM_ALIGNMENT);
	}

	ret = mcp251xfd_write(dev, address, tx_len);
	if (ret < 0) {
		return ret;
	}

	reg_byte = mcp251xfd_get_spi_buf_ptr(dev);
	*reg_byte = MCP251XFD_UINT32_FLAG_TO_BYTE_MASK(MCP251XFD_REG_TXQCON_UINC |
						       MCP251XFD_REG_TXQCON_TXREQ);

	return mcp251xfd_write(dev, MCP251XFD_REG_TXQCON + 1, 1);
}

static void mcp251xfd_rxobj_to_canframe(struct mcp251xfd_rxobj *src, struct can_frame *dst)
{
	memset(dst, 0, sizeof(*dst));

	src->id = sys_le32_to_cpu(src->id);
	src->flags = sys_le32_to_cpu(src->flags);

	if ((src->flags & MCP251XFD_OBJ_FLAGS_IDE) != 0) {
		dst->id = FIELD_GET(MCP251XFD_OBJ_ID_EID_MASK, src->id);
		dst->id |= FIELD_GET(MCP251XFD_OBJ_ID_SID_MASK, src->id) << 18;
		dst->flags |= CAN_FRAME_IDE;
	} else {
		dst->id = FIELD_GET(MCP251XFD_OBJ_ID_SID_MASK, src->id);
	}

	if ((src->flags & MCP251XFD_OBJ_FLAGS_BRS) != 0) {
		dst->flags |= CAN_FRAME_BRS;
	}

#if defined(CONFIG_CAN_FD_MODE)
	if ((src->flags & MCP251XFD_OBJ_FLAGS_FDF) != 0) {
		dst->flags |= CAN_FRAME_FDF;
	}
#endif

	dst->dlc = FIELD_GET(MCP251XFD_OBJ_FLAGS_DLC_MASK, src->flags);

#if defined(CONFIG_CAN_RX_TIMESTAMP)
	dst->timestamp = sys_le32_to_cpu(src->timestamp);
#endif

	if ((src->flags & MCP251XFD_OBJ_FLAGS_RTR) != 0) {
		dst->flags |= CAN_FRAME_RTR;
	} else {
		memcpy(dst->data, src->data, MIN(can_dlc_to_bytes(dst->dlc), CAN_MAX_DLEN));
	}
}

static int mcp251xfd_get_mode_internal(const struct device *dev, uint8_t *mode)
{
	uint8_t *reg_byte;
	uint32_t mask = MCP251XFD_UINT32_FLAG_TO_BYTE_MASK(MCP251XFD_REG_CON_OPMOD_MASK);

	reg_byte = mcp251xfd_read_crc(dev, MCP251XFD_REG_CON_B2, 1);
	if (!reg_byte) {
		return -EINVAL;
	}

	*mode = FIELD_GET(mask, *reg_byte);

	return 0;
}

static int mcp251xfd_reg_check_value_wtimeout(const struct device *dev, uint16_t addr,
					      uint32_t value, uint32_t mask,
					      uint32_t timeout_usec, int retries, bool allow_yield)
{
	uint32_t *reg;
	uint32_t delay = timeout_usec / retries;

	for (;;) {
		reg = mcp251xfd_read_crc(dev, addr, MCP251XFD_REG_SIZE);
		if (!reg) {
			return -EINVAL;
		}

		*reg = sys_le32_to_cpu(*reg);

		if ((*reg & mask) == value) {
			return 0;
		}

		if (--retries < 0) {
			LOG_ERR("Timeout validing 0x%x", addr);
			return -EIO;
		}

		if (allow_yield) {
			k_sleep(K_USEC(delay));
		} else {
			k_busy_wait(delay);
		}
	}
	return 0;
}

static int mcp251xfd_set_tdc(const struct device *dev, bool is_enabled, int tdc_offset)
{
	uint32_t *reg;
	uint32_t tmp;

	if (is_enabled &&
	    (tdc_offset < MCP251XFD_REG_TDC_TDCO_MIN || tdc_offset > MCP251XFD_REG_TDC_TDCO_MAX)) {
		return -EINVAL;
	}

	reg = mcp251xfd_get_spi_buf_ptr(dev);

	if (is_enabled) {
		tmp = FIELD_PREP(MCP251XFD_REG_TDC_TDCMOD_MASK, MCP251XFD_REG_TDC_TDCMOD_AUTO);
		tmp |= FIELD_PREP(MCP251XFD_REG_TDC_TDCO_MASK, tdc_offset);
	} else {
		tmp = FIELD_PREP(MCP251XFD_REG_TDC_TDCMOD_MASK, MCP251XFD_REG_TDC_TDCMOD_DISABLED);
	}

	*reg = sys_cpu_to_le32(tmp);

	return mcp251xfd_write(dev, MCP251XFD_REG_TDC, MCP251XFD_REG_SIZE);
}

static int mcp251xfd_set_mode_internal(const struct device *dev, uint8_t requested_mode)
{
	struct mcp251xfd_data *dev_data = dev->data;
	uint32_t *reg;
	uint32_t opmod, reg_con;
	int ret = 0;

	k_mutex_lock(&dev_data->mutex, K_FOREVER);

	reg = mcp251xfd_read_crc(dev, MCP251XFD_REG_CON, MCP251XFD_REG_SIZE);
	if (!reg) {
		ret = -EINVAL;
		goto done;
	}

	reg_con = sys_le32_to_cpu(*reg);

	opmod = FIELD_GET(MCP251XFD_REG_CON_OPMOD_MASK, reg_con);
	if (opmod == requested_mode) {
		goto done;
	}

#if defined(CONFIG_CAN_FD_MODE)
	if (dev_data->current_mcp251xfd_mode == MCP251XFD_REG_CON_MODE_CONFIG) {
		if (requested_mode ==  MCP251XFD_REG_CON_MODE_CAN2_0 ||
		    requested_mode ==  MCP251XFD_REG_CON_MODE_EXT_LOOPBACK ||
		    requested_mode == MCP251XFD_REG_CON_MODE_INT_LOOPBACK) {
			ret = mcp251xfd_set_tdc(dev, false, 0);
		} else if (requested_mode == MCP251XFD_REG_CON_MODE_MIXED) {
			ret = mcp251xfd_set_tdc(dev, true, dev_data->tdco);
		}

		if (ret < 0) {
			goto done;
		}
	}
#endif

	reg_con &= ~MCP251XFD_REG_CON_REQOP_MASK;
	reg_con |= FIELD_PREP(MCP251XFD_REG_CON_REQOP_MASK, requested_mode);

	*reg = sys_cpu_to_le32(reg_con);

	ret = mcp251xfd_write(dev, MCP251XFD_REG_CON, MCP251XFD_REG_SIZE);
	if (ret < 0) {
		LOG_ERR("Failed to write REG_CON register [%d]", MCP251XFD_REG_CON);
		goto done;
	}

	ret = mcp251xfd_reg_check_value_wtimeout(
		dev, MCP251XFD_REG_CON, FIELD_PREP(MCP251XFD_REG_CON_OPMOD_MASK, requested_mode),
		MCP251XFD_REG_CON_OPMOD_MASK, MCP251XFD_MODE_CHANGE_TIMEOUT_USEC,
		MCP251XFD_MODE_CHANGE_RETRIES, true);
done:
	k_mutex_unlock(&dev_data->mutex);
	return ret;
}

static int mcp251xfd_set_mode(const struct device *dev, can_mode_t mode)
{
	struct mcp251xfd_data *dev_data = dev->data;

	if (dev_data->common.started) {
		return -EBUSY;
	}

	/* todo: Add CAN_MODE_ONE_SHOT support */
	if ((mode & (CAN_MODE_3_SAMPLES | CAN_MODE_ONE_SHOT)) != 0) {
		return -ENOTSUP;
	}

	if (mode == CAN_MODE_NORMAL) {
		dev_data->next_mcp251xfd_mode = MCP251XFD_REG_CON_MODE_CAN2_0;
	}

	if ((mode & CAN_MODE_FD) != 0) {
#if defined(CONFIG_CAN_FD_MODE)
		dev_data->next_mcp251xfd_mode = MCP251XFD_REG_CON_MODE_MIXED;
#else
		return -ENOTSUP;
#endif
	}

	if ((mode & CAN_MODE_LISTENONLY) != 0) {
		dev_data->next_mcp251xfd_mode = MCP251XFD_REG_CON_MODE_LISTENONLY;
	}

	if ((mode & CAN_MODE_LOOPBACK) != 0) {
		dev_data->next_mcp251xfd_mode = MCP251XFD_REG_CON_MODE_EXT_LOOPBACK;
	}

	dev_data->common.mode = mode;

	return 0;
}

static int mcp251xfd_set_timing(const struct device *dev, const struct can_timing *timing)
{
	struct mcp251xfd_data *dev_data = dev->data;
	uint32_t *reg;
	uint32_t tmp;
	int ret;

	if (!timing) {
		return -EINVAL;
	}

	if (dev_data->common.started) {
		return -EBUSY;
	}

	k_mutex_lock(&dev_data->mutex, K_FOREVER);

	reg = mcp251xfd_get_spi_buf_ptr(dev);
	tmp = FIELD_PREP(MCP251XFD_REG_NBTCFG_BRP_MASK, timing->prescaler - 1);
	tmp |= FIELD_PREP(MCP251XFD_REG_NBTCFG_TSEG1_MASK,
			   timing->prop_seg + timing->phase_seg1 - 1);
	tmp |= FIELD_PREP(MCP251XFD_REG_NBTCFG_TSEG2_MASK, timing->phase_seg2 - 1);
	tmp |= FIELD_PREP(MCP251XFD_REG_NBTCFG_SJW_MASK, timing->sjw - 1);
	*reg = tmp;

	ret = mcp251xfd_write(dev, MCP251XFD_REG_NBTCFG, MCP251XFD_REG_SIZE);
	if (ret < 0) {
		LOG_ERR("Failed to write NBTCFG register [%d]", ret);
	}

	k_mutex_unlock(&dev_data->mutex);

	return ret;
}


#if defined(CONFIG_CAN_FD_MODE)
static int mcp251xfd_set_timing_data(const struct device *dev, const struct can_timing *timing)
{
	struct mcp251xfd_data *dev_data = dev->data;
	uint32_t *reg;
	uint32_t tmp;
	int ret;

	if (!timing) {
		return -EINVAL;
	}

	if (dev_data->common.started) {
		return -EBUSY;
	}

	k_mutex_lock(&dev_data->mutex, K_FOREVER);

	reg = mcp251xfd_get_spi_buf_ptr(dev);

	tmp = FIELD_PREP(MCP251XFD_REG_DBTCFG_BRP_MASK, timing->prescaler - 1);
	tmp |= FIELD_PREP(MCP251XFD_REG_DBTCFG_TSEG1_MASK,
			  timing->prop_seg + timing->phase_seg1 - 1);
	tmp |= FIELD_PREP(MCP251XFD_REG_DBTCFG_TSEG2_MASK, timing->phase_seg2 - 1);
	tmp |= FIELD_PREP(MCP251XFD_REG_DBTCFG_SJW_MASK, timing->sjw - 1);

	*reg = sys_cpu_to_le32(tmp);

	dev_data->tdco = timing->prescaler * (timing->prop_seg + timing->phase_seg1);

	ret = mcp251xfd_write(dev, MCP251XFD_REG_DBTCFG, MCP251XFD_REG_SIZE);
	if (ret < 0) {
		LOG_ERR("Failed to write DBTCFG register [%d]", ret);
	}

	k_mutex_unlock(&dev_data->mutex);

	return ret;
}
#endif

static int mcp251xfd_send(const struct device *dev, const struct can_frame *msg,
			  k_timeout_t timeout, can_tx_callback_t callback, void *callback_arg)
{
	struct mcp251xfd_data *dev_data = dev->data;
	uint8_t mailbox_idx;
	int ret = 0;

	LOG_DBG("Sending %d bytes. Id: 0x%x, ID type: %s %s %s %s", can_dlc_to_bytes(msg->dlc),
		msg->id, msg->flags & CAN_FRAME_IDE ? "extended" : "standard",
		msg->flags & CAN_FRAME_RTR ? "RTR" : "",
		msg->flags & CAN_FRAME_FDF ? "FD frame" : "",
		msg->flags & CAN_FRAME_BRS ? "BRS" : "");

	__ASSERT_NO_MSG(callback != NULL);

	if (!dev_data->common.started) {
		return -ENETDOWN;
	}

	if (dev_data->state == CAN_STATE_BUS_OFF) {
		return -ENETUNREACH;
	}

	if ((msg->flags & CAN_FRAME_FDF) == 0 && msg->dlc > CAN_MAX_DLC) {
		LOG_ERR("DLC of %d without fd flag set.", msg->dlc);
		return -EINVAL;
	}

	if ((msg->flags & CAN_FRAME_FDF) && !(dev_data->common.mode & CAN_MODE_FD)) {
		return -ENOTSUP;
	}

	if (k_sem_take(&dev_data->tx_sem, timeout) != 0) {
		return -EAGAIN;
	}

	k_mutex_lock(&dev_data->mutex, K_FOREVER);
	for (mailbox_idx = 0; mailbox_idx < MCP251XFD_TX_QUEUE_ITEMS; mailbox_idx++) {
		if ((BIT(mailbox_idx) & dev_data->mailbox_usage) == 0) {
			dev_data->mailbox_usage |= BIT(mailbox_idx);
			break;
		}
	}

	if (mailbox_idx >= MCP251XFD_TX_QUEUE_ITEMS) {
		k_sem_give(&dev_data->tx_sem);
		ret = -EIO;
		goto done;
	}

	dev_data->mailbox[mailbox_idx].cb = callback;
	dev_data->mailbox[mailbox_idx].cb_arg = callback_arg;

	ret = mcp251xfd_fifo_write(dev, mailbox_idx, msg);

	if (ret < 0) {
		dev_data->mailbox_usage &= ~BIT(mailbox_idx);
		dev_data->mailbox[mailbox_idx].cb = NULL;
		k_sem_give(&dev_data->tx_sem);
	}

done:
	k_mutex_unlock(&dev_data->mutex);
	return ret;
}

static int mcp251xfd_add_rx_filter(const struct device *dev, can_rx_callback_t rx_cb, void *cb_arg,
				   const struct can_filter *filter)
{
	struct mcp251xfd_data *dev_data = dev->data;
	uint32_t *reg;
	uint32_t tmp;
	uint8_t *reg_byte;
	int filter_idx;
	int ret;

	__ASSERT(rx_cb != NULL, "rx_cb can not be null");
	k_mutex_lock(&dev_data->mutex, K_FOREVER);

	for (filter_idx = 0; filter_idx < CONFIG_CAN_MAX_FILTER ; filter_idx++) {
		if ((BIT(filter_idx) & dev_data->filter_usage) == 0) {
			break;
		}
	}

	if (filter_idx >= CONFIG_CAN_MAX_FILTER) {
		filter_idx = -ENOSPC;
		goto done;
	}

	reg = mcp251xfd_get_spi_buf_ptr(dev);

	if ((filter->flags & CAN_FILTER_IDE) != 0) {
		tmp = FIELD_PREP(MCP251XFD_REG_FLTOBJ_SID_MASK, filter->id >> 18);
		tmp |= FIELD_PREP(MCP251XFD_REG_FLTOBJ_EID_MASK, filter->id);
		tmp |= MCP251XFD_REG_FLTOBJ_EXIDE;
	} else {
		tmp = FIELD_PREP(MCP251XFD_REG_FLTOBJ_SID_MASK, filter->id);
	}

	*reg = sys_cpu_to_le32(tmp);
	ret = mcp251xfd_write(dev, MCP251XFD_REG_FLTOBJ(filter_idx), MCP251XFD_REG_SIZE);
	if (ret < 0) {
		LOG_ERR("Failed to write FLTOBJ register [%d]", ret);
		goto done;
	}

	reg = mcp251xfd_get_spi_buf_ptr(dev);
	if ((filter->flags & CAN_FILTER_IDE) != 0) {
		tmp = FIELD_PREP(MCP251XFD_REG_MASK_MSID_MASK, filter->mask >> 18);
		tmp |= FIELD_PREP(MCP251XFD_REG_MASK_MEID_MASK, filter->mask);
	} else {
		tmp = FIELD_PREP(MCP251XFD_REG_MASK_MSID_MASK, filter->mask);
	}
	tmp |= MCP251XFD_REG_MASK_MIDE;

	*reg = sys_cpu_to_le32(tmp);

	ret = mcp251xfd_write(dev, MCP251XFD_REG_FLTMASK(filter_idx), MCP251XFD_REG_SIZE);
	if (ret < 0) {
		LOG_ERR("Failed to write FLTMASK register [%d]", ret);
		goto done;
	}

	reg_byte = mcp251xfd_get_spi_buf_ptr(dev);
	*reg_byte = MCP251XFD_REG_BYTE_FLTCON_FLTEN;
	*reg_byte |= FIELD_PREP(MCP251XFD_REG_BYTE_FLTCON_FBP_MASK, MCP251XFD_RX_FIFO_IDX);

	ret = mcp251xfd_write(dev, MCP251XFD_REG_BYTE_FLTCON(filter_idx), 1);
	if (ret < 0) {
		LOG_ERR("Failed to write FLTCON register [%d]", ret);
		goto done;
	}

	dev_data->filter_usage |= BIT(filter_idx);
	dev_data->filter[filter_idx] = *filter;
	dev_data->rx_cb[filter_idx] = rx_cb;
	dev_data->cb_arg[filter_idx] = cb_arg;

done:
	k_mutex_unlock(&dev_data->mutex);

	return filter_idx;
}

static void mcp251xfd_remove_rx_filter(const struct device *dev, int filter_idx)
{
	struct mcp251xfd_data *dev_data = dev->data;
	uint8_t *reg_byte;
	uint32_t *reg;
	int ret;

	if (filter_idx < 0 || filter_idx >= CONFIG_CAN_MAX_FILTER) {
		LOG_ERR("Filter ID %d out of bounds", filter_idx);
		return;
	}

	k_mutex_lock(&dev_data->mutex, K_FOREVER);

	reg_byte = mcp251xfd_get_spi_buf_ptr(dev);
	*reg_byte = 0;

	ret = mcp251xfd_write(dev, MCP251XFD_REG_BYTE_FLTCON(filter_idx), 1);
	if (ret < 0) {
		LOG_ERR("Failed to write FLTCON register [%d]", ret);
		goto done;
	}

	dev_data->filter_usage &= ~BIT(filter_idx);

	reg = mcp251xfd_get_spi_buf_ptr(dev);
	reg[0] = 0;

	ret = mcp251xfd_write(dev, MCP251XFD_REG_FLTCON(filter_idx), MCP251XFD_REG_SIZE);
	if (ret < 0) {
		LOG_ERR("Failed to write FLTCON register [%d]", ret);
	}

done:
	k_mutex_unlock(&dev_data->mutex);
}

static void mcp251xfd_set_state_change_callback(const struct device *dev,
						can_state_change_callback_t cb, void *user_data)
{
	struct mcp251xfd_data *dev_data = dev->data;

	dev_data->common.state_change_cb = cb;
	dev_data->common.state_change_cb_user_data = user_data;
}

static int mcp251xfd_get_state(const struct device *dev, enum can_state *state,
			       struct can_bus_err_cnt *err_cnt)
{
	struct mcp251xfd_data *dev_data = dev->data;
	uint32_t *reg;
	uint32_t tmp;
	int ret = 0;

	k_mutex_lock(&dev_data->mutex, K_FOREVER);

	reg = mcp251xfd_read_crc(dev, MCP251XFD_REG_TREC, MCP251XFD_REG_SIZE);
	if (!reg) {
		ret = -EINVAL;
		goto done;
	}

	tmp = sys_le32_to_cpu(*reg);

	if (err_cnt != NULL) {
		err_cnt->tx_err_cnt = FIELD_GET(MCP251XFD_REG_TREC_TEC_MASK, tmp);
		err_cnt->rx_err_cnt = FIELD_GET(MCP251XFD_REG_TREC_REC_MASK, tmp);
	}

	if (state == NULL) {
		goto done;
	}

	if (!dev_data->common.started) {
		*state = CAN_STATE_STOPPED;
		goto done;
	}

	if ((tmp & MCP251XFD_REG_TREC_TXBO) != 0) {
		*state = CAN_STATE_BUS_OFF;
	} else if ((tmp & MCP251XFD_REG_TREC_TXBP) != 0) {
		*state = CAN_STATE_ERROR_PASSIVE;
	} else if ((tmp & MCP251XFD_REG_TREC_RXBP) != 0) {
		*state = CAN_STATE_ERROR_PASSIVE;
	} else if ((tmp & MCP251XFD_REG_TREC_TXWARN) != 0) {
		*state = CAN_STATE_ERROR_WARNING;
	} else if ((tmp & MCP251XFD_REG_TREC_RXWARN) != 0) {
		*state = CAN_STATE_ERROR_WARNING;
	} else {
		*state = CAN_STATE_ERROR_ACTIVE;
	}

done:
	k_mutex_unlock(&dev_data->mutex);
	return 0;
}

static int mcp251xfd_get_core_clock(const struct device *dev, uint32_t *rate)
{
	const struct mcp251xfd_config *dev_cfg = dev->config;

	*rate = dev_cfg->osc_freq;
	return 0;
}

static int mcp251xfd_get_max_filters(const struct device *dev, bool ide)
{
	ARG_UNUSED(ide);

	return CONFIG_CAN_MAX_FILTER;
}

#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
static int mcp251xfd_recover(const struct device *dev, k_timeout_t timeout)
{
	struct mcp251xfd_data *dev_data = dev->data;

	ARG_UNUSED(timeout);

	if (!dev_data->common.started) {
		return -ENETDOWN;
	}

	return -ENOTSUP;
}
#endif

static int mcp251xfd_handle_fifo_read(const struct device *dev, const struct mcp251xfd_fifo *fifo,
				      uint8_t fifo_type)
{
	int ret = 0;
	struct mcp251xfd_data *dev_data = dev->data;
	uint32_t *regs, fifosta, ua;
	uint8_t *reg_byte;

	int len;
	int fetch_total = 0;
	int ui_inc = 0;
	uint32_t fifo_tail_index, fifo_tail_addr;
	uint8_t fifo_head_index;

	k_mutex_lock(&dev_data->mutex, K_FOREVER);

	/* read in FIFOSTA and FIFOUA at the same time */
	regs = mcp251xfd_read_crc(dev, MCP251XFD_REG_FIFOCON_TO_STA(fifo->reg_fifocon_addr),
				  2 * MCP251XFD_REG_SIZE);
	if (!regs) {
		ret = -EINVAL;
		goto done;
	}
	fifosta = sys_le32_to_cpu(regs[0]);
	ua = sys_le32_to_cpu(regs[1]);

	/* is there any data in the fifo? */
	if (!(fifosta & MCP251XFD_REG_FIFOSTA_TFNRFNIF)) {
		goto done;
	}

	fifo_tail_addr = ua;
	fifo_tail_index = (fifo_tail_addr - fifo->ram_start_addr) / fifo->item_size;

	if (fifo_type == MCP251XFD_FIFO_TYPE_RX) {
		/*
		 * fifo_head_index points where the next message will be written.
		 * It points to one past the end of the fifo.
		 */
		fifo_head_index = FIELD_GET(MCP251XFD_REG_FIFOSTA_FIFOCI_MASK, fifosta);
		if (fifo_head_index == 0) {
			fifo_head_index = fifo->capacity - 1;
		} else {
			fifo_head_index -= 1;
		}

		if (fifo_tail_index > fifo_head_index) {
			/* fetch to the end of the memory and then wrap to the start */
			fetch_total = fifo->capacity - 1 - fifo_tail_index + 1;
			fetch_total += fifo_head_index + 1;
		} else {
			fetch_total = fifo_head_index - fifo_tail_index + 1;
		}
	} else if (fifo_type == MCP251XFD_FIFO_TYPE_TEF) {
		/* FIFOCI doesn't exist for TEF queues, so fetch one message at a time */
		fifo_head_index = fifo_tail_index;
		fetch_total = 1;
	} else {
		ret = -EINVAL;
		goto done;
	}

	while (fetch_total > 0) {
		uint16_t memory_addr;
		uint8_t *data;

		if (fifo_tail_index > fifo_head_index) {
			len = fifo->capacity - 1 - fifo_tail_index + 1;
		} else {
			len = fifo_head_index - fifo_tail_index + 1;
		}

		memory_addr = MCP251XFD_RAM_START_ADDR + fifo->ram_start_addr +
			      fifo_tail_index * fifo->item_size;

		data = mcp251xfd_read_reg(dev, memory_addr, len * fifo->item_size);
		if (!data) {
			LOG_ERR("Error fetching batch message");
			ret = -EINVAL;
			goto done;
		}

		for (int i = 0; i < len; i++) {
			fifo->msg_handler(dev, (void *)(&data[i * fifo->item_size]));
		}

		fifo_tail_index = (fifo_tail_index + len) % fifo->capacity;
		fetch_total -= len;
		ui_inc += len;
	}

	reg_byte = mcp251xfd_get_spi_buf_ptr(dev);
	*reg_byte = MCP251XFD_UINT32_FLAG_TO_BYTE_MASK(MCP251XFD_REG_FIFOCON_UINC);

	for (int i = 0; i < ui_inc; i++) {
		ret = mcp251xfd_write(dev, fifo->reg_fifocon_addr + 1, 1);
		if (ret < 0) {
			LOG_ERR("Failed to increment pointer");
			goto done;
		}
	}

done:
	k_mutex_unlock(&dev_data->mutex);
	return ret;
}

static void mcp251xfd_reset_tx_fifos(const struct device *dev, int status)
{
	struct mcp251xfd_data *dev_data = dev->data;

	LOG_INF("All FIFOs Reset");
	k_mutex_lock(&dev_data->mutex, K_FOREVER);
	for (int i = 0; i < MCP251XFD_TX_QUEUE_ITEMS; i++) {
		can_tx_callback_t callback;

		if (!(dev_data->mailbox_usage & BIT(i))) {
			continue;
		}

		callback = dev_data->mailbox[i].cb;
		if (callback) {
			callback(dev, status, dev_data->mailbox[i].cb_arg);
		}

		dev_data->mailbox_usage &= ~BIT(i);
		dev_data->mailbox[i].cb = NULL;
		k_sem_give(&dev_data->tx_sem);
	}
	k_mutex_unlock(&dev_data->mutex);
}

/*
 * CERRIF will be set each time a threshold in the TEC/REC counter is crossed by the following
 * conditions:
 * • TEC or REC exceeds the Error Warning state threshold
 * • The transmitter or receiver transitions to Error Passive state
 * • The transmitter transitions to Bus Off state
 * • The transmitter or receiver transitions from Error Passive to Error Active state
 * • The module transitions from Bus Off to Error Active state, after the bus off recovery
 * sequence
 * When the user clears CERRIF, it will remain clear until a new counter crossing occurs.
 */
static int mcp251xfd_handle_cerrif(const struct device *dev)
{
	enum can_state new_state;
	struct mcp251xfd_data *dev_data = dev->data;
	struct can_bus_err_cnt err_cnt;
	int ret;

	k_mutex_lock(&dev_data->mutex, K_FOREVER);

	ret = mcp251xfd_get_state(dev, &new_state, &err_cnt);
	if (ret < 0) {
		goto done;
	}

	if (new_state == dev_data->state) {
		goto done;
	}

	LOG_INF("State %d -> %d (tx: %d, rx: %d)", dev_data->state, new_state, err_cnt.tx_err_cnt,
		err_cnt.rx_err_cnt);

	/* Upon entering bus-off, all the fifos are reset. */
	dev_data->state = new_state;
	if (new_state == CAN_STATE_BUS_OFF) {
		mcp251xfd_reset_tx_fifos(dev, -ENETDOWN);
	}

	if (dev_data->common.state_change_cb) {
		dev_data->common.state_change_cb(dev, new_state, err_cnt,
						 dev_data->common.state_change_cb_user_data);
	}

done:
	k_mutex_unlock(&dev_data->mutex);
	return ret;
}

static int mcp251xfd_handle_modif(const struct device *dev)
{
	struct mcp251xfd_data *dev_data = dev->data;
	uint8_t mode;
	int ret;

	k_mutex_lock(&dev_data->mutex, K_FOREVER);

	ret = mcp251xfd_get_mode_internal(dev, &mode);
	if (ret < 0) {
		goto finish;
	}

	dev_data->current_mcp251xfd_mode = mode;

	LOG_INF("Switched to mode %d", mode);

	if (mode == dev_data->next_mcp251xfd_mode) {
		ret = 0;
		goto finish;
	}

	/* try to transition back into our target mode */
	if (dev_data->common.started) {
		LOG_INF("Switching back into mode %d", dev_data->next_mcp251xfd_mode);
		ret =  mcp251xfd_set_mode_internal(dev, dev_data->next_mcp251xfd_mode);
	}

finish:
	k_mutex_unlock(&dev_data->mutex);
	return ret;
}

static int mcp251xfd_handle_ivmif(const struct device *dev)
{
	uint32_t *reg;
	struct mcp251xfd_data *dev_data = dev->data;
	int ret;

	k_mutex_lock(&dev_data->mutex, K_FOREVER);

	reg = mcp251xfd_read_crc(dev, MCP251XFD_REG_BDIAG1, MCP251XFD_REG_SIZE);
	if (!reg) {
		ret = -EINVAL;
		goto done;
	}

	*reg = sys_le32_to_cpu(*reg);

	if ((*reg & MCP251XFD_REG_BDIAG1_TXBOERR) != 0) {
		LOG_INF("ivmif bus-off error");
		mcp251xfd_reset_tx_fifos(dev, -ENETDOWN);
	}

	/* Clear the values in diag */
	reg = mcp251xfd_get_spi_buf_ptr(dev);
	reg[0] = 0;
	ret = mcp251xfd_write(dev, MCP251XFD_REG_BDIAG1, MCP251XFD_REG_SIZE);

done:
	k_mutex_unlock(&dev_data->mutex);
	return ret;
}

static void mcp251xfd_handle_interrupts(const struct device *dev)
{
	const struct mcp251xfd_config *dev_cfg = dev->config;
	struct mcp251xfd_data *dev_data = dev->data;
	uint16_t *reg_int_hw;
	uint32_t reg_int;
	int ret;
	uint8_t consecutive_calls = 0;

	while (1) {
		k_mutex_lock(&dev_data->mutex, K_FOREVER);
		reg_int_hw = mcp251xfd_read_crc(dev, MCP251XFD_REG_INT, sizeof(*reg_int_hw));

		if (!reg_int_hw) {
			k_mutex_unlock(&dev_data->mutex);
			continue;
		}

		*reg_int_hw = sys_le16_to_cpu(*reg_int_hw);

		reg_int = *reg_int_hw;

		/* these interrupt flags need to be explicitly cleared */
		if (reg_int & MCP251XFD_REG_INT_IF_CLEARABLE_MASK) {

			*reg_int_hw &= ~MCP251XFD_REG_INT_IF_CLEARABLE_MASK;

			*reg_int_hw = sys_cpu_to_le16(*reg_int_hw);

			ret = mcp251xfd_write(dev, MCP251XFD_REG_INT, sizeof(*reg_int_hw));
			if (ret) {
				LOG_ERR("Error clearing REG_INT interrupts [%d]", ret);
			}
		}

		k_mutex_unlock(&dev_data->mutex);

		if ((reg_int & MCP251XFD_REG_INT_RXIF) != 0) {
			ret = mcp251xfd_handle_fifo_read(dev, &dev_cfg->rx_fifo,
							 MCP251XFD_FIFO_TYPE_RX);
			if (ret < 0) {
				LOG_ERR("Error handling RXIF [%d]", ret);
			}
		}

		if ((reg_int & MCP251XFD_REG_INT_TEFIF) != 0) {
			ret = mcp251xfd_handle_fifo_read(dev, &dev_cfg->tef_fifo,
							 MCP251XFD_FIFO_TYPE_TEF);
			if (ret < 0) {
				LOG_ERR("Error handling TEFIF [%d]", ret);
			}
		}

		if ((reg_int & MCP251XFD_REG_INT_IVMIF) != 0) {
			ret = mcp251xfd_handle_ivmif(dev);
			if (ret < 0) {
				LOG_ERR("Error handling IVMIF [%d]", ret);
			}
		}

		if ((reg_int & MCP251XFD_REG_INT_MODIF) != 0) {
			ret = mcp251xfd_handle_modif(dev);
			if (ret < 0) {
				LOG_ERR("Error handling MODIF [%d]", ret);
			}
		}

		/*
		 * From Linux mcp251xfd driver
		 * On the MCP2527FD and MCP2518FD, we don't get a CERRIF IRQ on the transition
		 * TX ERROR_WARNING -> TX ERROR_ACTIVE.
		 */
		if ((reg_int & MCP251XFD_REG_INT_CERRIF) ||
		    dev_data->state > CAN_STATE_ERROR_ACTIVE) {
			ret = mcp251xfd_handle_cerrif(dev);
			if (ret < 0) {
				LOG_ERR("Error handling CERRIF [%d]", ret);
			}
		}

		/* Break from loop if INT pin is inactive */
		consecutive_calls++;
		ret = gpio_pin_get_dt(&dev_cfg->int_gpio_dt);
		if (ret < 0) {
			LOG_ERR("Couldn't read INT pin [%d]", ret);
		} else if (ret == 0) {
			/* All interrupt flags handled */
			break;
		} else if (consecutive_calls % MCP251XFD_MAX_INT_HANDLER_CALLS == 0) {
			/* If there are clock problems, then MODIF cannot be cleared. */
			/* This is detected if there are too many consecutive calls. */
			/* Sleep this thread if this happens. */
			k_sleep(K_USEC(MCP251XFD_INT_HANDLER_SLEEP_USEC));
		}
	}
}

static void mcp251xfd_int_thread(const struct device *dev)
{
	const struct mcp251xfd_config *dev_cfg = dev->config;
	struct mcp251xfd_data *dev_data = dev->data;

	while (1) {
		int ret;

		k_sem_take(&dev_data->int_sem, K_FOREVER);
		mcp251xfd_handle_interrupts(dev);

		/* Re-enable pin interrupts */
		ret = gpio_pin_interrupt_configure_dt(&dev_cfg->int_gpio_dt, GPIO_INT_LEVEL_ACTIVE);
		if (ret < 0) {
			LOG_ERR("Couldn't enable pin interrupt [%d]", ret);
			k_oops();
		}
	}
}

static void mcp251xfd_int_gpio_callback(const struct device *dev_gpio, struct gpio_callback *cb,
					uint32_t pins)
{
	ARG_UNUSED(dev_gpio);
	struct mcp251xfd_data *dev_data = CONTAINER_OF(cb, struct mcp251xfd_data, int_gpio_cb);
	const struct device *dev = dev_data->dev;
	const struct mcp251xfd_config *dev_cfg = dev->config;
	int ret;

	/* Disable pin interrupts */
	ret = gpio_pin_interrupt_configure_dt(&dev_cfg->int_gpio_dt, GPIO_INT_DISABLE);
	if (ret < 0) {
		LOG_ERR("Couldn't disable pin interrupt [%d]", ret);
		k_oops();
	}

	k_sem_give(&dev_data->int_sem);
}

static int mcp251xfd_get_capabilities(const struct device *dev, can_mode_t *cap)
{
	ARG_UNUSED(dev);

	*cap = CAN_MODE_NORMAL | CAN_MODE_LISTENONLY | CAN_MODE_LOOPBACK;

#if defined(CONFIG_CAN_FD_MODE)
	*cap |= CAN_MODE_FD;
#endif

	return 0;
}

static int mcp251xfd_start(const struct device *dev)
{
	struct mcp251xfd_data *dev_data = dev->data;
	const struct mcp251xfd_config *dev_cfg = dev->config;
	int ret;

	if (dev_data->common.started) {
		return -EALREADY;
	}

	/* in case of a race between mcp251xfd_send() and mcp251xfd_stop() */
	mcp251xfd_reset_tx_fifos(dev, -ENETDOWN);

	if (dev_cfg->common.phy != NULL) {
		ret = can_transceiver_enable(dev_cfg->common.phy, dev_data->common.mode);
		if (ret < 0) {
			LOG_ERR("Failed to enable CAN transceiver [%d]", ret);
			return ret;
		}
	}

	k_mutex_lock(&dev_data->mutex, K_FOREVER);

	ret = mcp251xfd_set_mode_internal(dev, dev_data->next_mcp251xfd_mode);
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

static int mcp251xfd_stop(const struct device *dev)
{
	struct mcp251xfd_data *dev_data = dev->data;
	const struct mcp251xfd_config *dev_cfg = dev->config;
	uint8_t *reg_byte;
	int ret;

	if (!dev_data->common.started) {
		return -EALREADY;
	}

	k_mutex_lock(&dev_data->mutex, K_FOREVER);

	/* abort all transmissions */
	reg_byte = mcp251xfd_get_spi_buf_ptr(dev);
	*reg_byte = MCP251XFD_UINT32_FLAG_TO_BYTE_MASK(MCP251XFD_REG_CON_ABAT);

	ret = mcp251xfd_write(dev, MCP251XFD_REG_CON_B3, 1);
	if (ret < 0) {
		k_mutex_unlock(&dev_data->mutex);
		return ret;
	}

	/* wait for all the messages to be aborted */
	while (1) {
		reg_byte = mcp251xfd_read_crc(dev, MCP251XFD_REG_CON_B3, 1);

		if (!reg_byte ||
		    (*reg_byte & MCP251XFD_UINT32_FLAG_TO_BYTE_MASK(MCP251XFD_REG_CON_ABAT)) == 0) {
			break;
		}
	}

	mcp251xfd_reset_tx_fifos(dev, -ENETDOWN);

	ret = mcp251xfd_set_mode_internal(dev, MCP251XFD_REG_CON_MODE_CONFIG);
	if (ret < 0) {
		k_mutex_unlock(&dev_data->mutex);
		return ret;
	}

	dev_data->common.started = false;
	k_mutex_unlock(&dev_data->mutex);

	if (dev_cfg->common.phy != NULL) {
		ret = can_transceiver_disable(dev_cfg->common.phy);
		if (ret < 0) {
			LOG_ERR("Failed to disable CAN transceiver [%d]", ret);
			return ret;
		}
	}

	return 0;
}

static void mcp251xfd_rx_fifo_handler(const struct device *dev, void *data)
{
	struct can_frame dst;
	struct mcp251xfd_data *dev_data = dev->data;
	struct mcp251xfd_rxobj *rxobj = data;
	uint32_t filhit;

	mcp251xfd_rxobj_to_canframe(rxobj, &dst);

#ifndef CONFIG_CAN_ACCEPT_RTR
	if ((dst.flags & CAN_FRAME_RTR) != 0U) {
		return;
	}
#endif /* !CONFIG_CAN_ACCEPT_RTR */

	filhit = FIELD_GET(MCP251XFD_OBJ_FILHIT_MASK, rxobj->flags);
	if ((dev_data->filter_usage & BIT(filhit)) != 0) {
		LOG_DBG("Received msg CAN id: 0x%x", dst.id);
		dev_data->rx_cb[filhit](dev, &dst, dev_data->cb_arg[filhit]);
	}
}

static void mcp251xfd_tef_fifo_handler(const struct device *dev, void *data)
{
	struct mcp251xfd_data *dev_data = dev->data;
	can_tx_callback_t callback;
	struct mcp251xfd_tefobj *tefobj = data;
	uint8_t mailbox_idx;

	mailbox_idx = FIELD_GET(MCP251XFD_OBJ_FLAGS_SEQ_MASK, tefobj->flags);
	if (mailbox_idx >= MCP251XFD_TX_QUEUE_ITEMS) {
		mcp251xfd_reset_tx_fifos(dev, -EIO);
		LOG_ERR("Invalid mailbox index");
		return;
	}

	callback = dev_data->mailbox[mailbox_idx].cb;
	if (callback != NULL) {
		callback(dev, 0, dev_data->mailbox[mailbox_idx].cb_arg);
	}

	dev_data->mailbox_usage &= ~BIT(mailbox_idx);
	dev_data->mailbox[mailbox_idx].cb = NULL;
	k_sem_give(&dev_data->tx_sem);
}

#if defined(CONFIG_CAN_FD_MODE)
static int mcp251xfd_init_timing_struct_data(struct can_timing *timing,
					     const struct device *dev,
					     const struct mcp251xfd_timing_params *timing_params)
{
	const struct mcp251xfd_config *dev_cfg = dev->config;
	int ret;

	if (USE_SP_ALGO && dev_cfg->common.sample_point_data > 0) {
		ret = can_calc_timing_data(dev, timing, dev_cfg->common.bus_speed_data,
					   dev_cfg->common.sample_point_data);
		if (ret < 0) {
			return ret;
		}
		LOG_DBG("Data phase Presc: %d, BS1: %d, BS2: %d", timing->prescaler,
			timing->phase_seg1, timing->phase_seg2);
		LOG_DBG("Data phase Sample-point err : %d", ret);
	} else {
		timing->sjw = timing_params->sjw;
		timing->prop_seg = timing_params->prop_seg;
		timing->phase_seg1 = timing_params->phase_seg1;
		timing->phase_seg2 = timing_params->phase_seg2;
		ret = can_calc_prescaler(dev, timing, dev_cfg->common.bus_speed_data);
		if (ret > 0) {
			LOG_WRN("Data phase Bitrate error: %d", ret);
		}
	}

	return ret;
}
#endif

static int mcp251xfd_init_timing_struct(struct can_timing *timing,
					const struct device *dev,
					const struct mcp251xfd_timing_params *timing_params)
{
	const struct mcp251xfd_config *dev_cfg = dev->config;
	int ret;

	if (USE_SP_ALGO && dev_cfg->common.sample_point > 0) {
		ret = can_calc_timing(dev, timing, dev_cfg->common.bus_speed,
				      dev_cfg->common.sample_point);
		if (ret < 0) {
			return ret;
		}
		LOG_DBG("Presc: %d, BS1: %d, BS2: %d", timing->prescaler, timing->phase_seg1,
			timing->phase_seg2);
		LOG_DBG("Sample-point err : %d", ret);
	} else {
		timing->sjw = timing_params->sjw;
		timing->prop_seg = timing_params->prop_seg;
		timing->phase_seg1 = timing_params->phase_seg1;
		timing->phase_seg2 = timing_params->phase_seg2;
		ret = can_calc_prescaler(dev, timing, dev_cfg->common.bus_speed);
		if (ret > 0) {
			LOG_WRN("Bitrate error: %d", ret);
		}
	}

	return ret;
}

static inline int mcp251xfd_init_con_reg(const struct device *dev)
{
	uint32_t *reg;
	uint32_t tmp;

	reg = mcp251xfd_get_spi_buf_ptr(dev);
	tmp = MCP251XFD_REG_CON_ISOCRCEN | MCP251XFD_REG_CON_WAKFIL | MCP251XFD_REG_CON_TXQEN |
	      MCP251XFD_REG_CON_STEF;
	tmp |= FIELD_PREP(MCP251XFD_REG_CON_WFT_MASK, MCP251XFD_REG_CON_WFT_T11FILTER) |
		FIELD_PREP(MCP251XFD_REG_CON_REQOP_MASK, MCP251XFD_REG_CON_MODE_CONFIG);
	*reg = tmp;

	return mcp251xfd_write(dev, MCP251XFD_REG_CON, MCP251XFD_REG_SIZE);
}

static inline int mcp251xfd_init_osc_reg(const struct device *dev)
{
	int ret;
	const struct mcp251xfd_config *dev_cfg = dev->config;
	uint32_t *reg = mcp251xfd_get_spi_buf_ptr(dev);
	uint32_t reg_value = MCP251XFD_REG_OSC_OSCRDY;
	uint32_t tmp;

	tmp = FIELD_PREP(MCP251XFD_REG_OSC_CLKODIV_MASK, dev_cfg->clko_div);
	if (dev_cfg->pll_enable) {
		tmp |= MCP251XFD_REG_OSC_PLLEN;
		reg_value |= MCP251XFD_REG_OSC_PLLRDY;
	}

	*reg = sys_cpu_to_le32(tmp);

	ret = mcp251xfd_write(dev, MCP251XFD_REG_OSC, MCP251XFD_REG_SIZE);
	if (ret < 0) {
		return ret;
	}

	return mcp251xfd_reg_check_value_wtimeout(dev, MCP251XFD_REG_OSC, reg_value, reg_value,
						  MCP251XFD_PLLRDY_TIMEOUT_USEC,
						  MCP251XFD_PLLRDY_RETRIES, false);
}

static inline int mcp251xfd_init_iocon_reg(const struct device *dev)
{
	const struct mcp251xfd_config *dev_cfg = dev->config;
	uint32_t *reg = mcp251xfd_get_spi_buf_ptr(dev);
	uint32_t tmp;

/*
 *         MCP2518FD Errata: DS80000789
 *         Writing Byte 2/3 of the IOCON register using single SPI write cleat LAT0 and LAT1.
 *         This has no effect in the current version since LAT0/1 are set to zero anyway.
 *         However, it needs to be properly handled if other values are needed. Errata suggests
 *         to do single byte writes instead.
 */

	tmp = MCP251XFD_REG_IOCON_TRIS0 | MCP251XFD_REG_IOCON_TRIS1 | MCP251XFD_REG_IOCON_PM0 |
	      MCP251XFD_REG_IOCON_PM1;

	if (dev_cfg->sof_on_clko) {
		tmp |= MCP251XFD_REG_IOCON_SOF;
	}

	*reg = sys_cpu_to_le32(tmp);

	return  mcp251xfd_write(dev, MCP251XFD_REG_IOCON, MCP251XFD_REG_SIZE);
}

static inline int mcp251xfd_init_int_reg(const struct device *dev)
{
	uint32_t *reg = mcp251xfd_get_spi_buf_ptr(dev);
	uint32_t tmp;

	tmp = MCP251XFD_REG_INT_RXIE | MCP251XFD_REG_INT_MODIE | MCP251XFD_REG_INT_TEFIE |
	      MCP251XFD_REG_INT_CERRIE;

	*reg = sys_cpu_to_le32(tmp);

	return mcp251xfd_write(dev, MCP251XFD_REG_INT, MCP251XFD_REG_SIZE);
}

static inline int mcp251xfd_init_tef_fifo(const struct device *dev)
{
	uint32_t *reg = mcp251xfd_get_spi_buf_ptr(dev);
	uint32_t tmp;

	tmp = MCP251XFD_REG_TEFCON_TEFNEIE | MCP251XFD_REG_TEFCON_FRESET;
	tmp |= FIELD_PREP(MCP251XFD_REG_TEFCON_FSIZE_MASK, MCP251XFD_TX_QUEUE_ITEMS - 1);

	*reg = sys_cpu_to_le32(tmp);

	return mcp251xfd_write(dev, MCP251XFD_REG_TEFCON, MCP251XFD_REG_SIZE);
}

static inline int mcp251xfd_init_tx_queue(const struct device *dev)
{
	uint32_t *reg = mcp251xfd_get_spi_buf_ptr(dev);
	uint32_t tmp;

	tmp = MCP251XFD_REG_TXQCON_TXEN | MCP251XFD_REG_TXQCON_FRESET;
	tmp |= FIELD_PREP(MCP251XFD_REG_TXQCON_TXAT_MASK, MCP251XFD_REG_TXQCON_TXAT_UNLIMITED);
	tmp |= FIELD_PREP(MCP251XFD_REG_TXQCON_FSIZE_MASK, MCP251XFD_TX_QUEUE_ITEMS - 1);
	tmp |= FIELD_PREP(MCP251XFD_REG_TXQCON_PLSIZE_MASK,
			  can_bytes_to_dlc(MCP251XFD_PAYLOAD_SIZE) - 8);

	*reg = sys_cpu_to_le32(tmp);

	return mcp251xfd_write(dev, MCP251XFD_REG_TXQCON, MCP251XFD_REG_SIZE);
}

static inline int mcp251xfd_init_rx_fifo(const struct device *dev)
{
	uint32_t *reg = mcp251xfd_get_spi_buf_ptr(dev);
	uint32_t tmp;

	tmp = MCP251XFD_REG_FIFOCON_TFNRFNIE | MCP251XFD_REG_FIFOCON_FRESET;
	tmp |= FIELD_PREP(MCP251XFD_REG_FIFOCON_FSIZE_MASK, MCP251XFD_RX_FIFO_ITEMS - 1);
	tmp |= FIELD_PREP(MCP251XFD_REG_FIFOCON_PLSIZE_MASK,
			  can_bytes_to_dlc(MCP251XFD_PAYLOAD_SIZE) - 8);
#if defined(CONFIG_CAN_RX_TIMESTAMP)
	tmp |= MCP251XFD_REG_FIFOCON_RXTSEN;
#endif

	*reg = sys_cpu_to_le32(tmp);

	return mcp251xfd_write(dev, MCP251XFD_REG_FIFOCON(MCP251XFD_RX_FIFO_IDX),
			       MCP251XFD_REG_SIZE);
}

#if defined(CONFIG_CAN_RX_TIMESTAMP)
static int mcp251xfd_init_tscon(const struct device *dev)
{
	uint32_t *reg = mcp251xfd_get_spi_buf_ptr(dev);
	const struct mcp251xfd_config *dev_cfg = dev->config;
	uint32_t tmp;

	tmp = MCP251XFD_REG_TSCON_TBCEN;
	tmp |= FIELD_PREP(MCP251XFD_REG_TSCON_TBCPRE_MASK,
			  dev_cfg->timestamp_prescaler - 1);

	*reg = sys_cpu_to_le32(tmp);

	return mcp251xfd_write(dev, MCP251XFD_REG_TSCON, MCP251XFD_REG_SIZE);
}
#endif

static int mcp251xfd_reset(const struct device *dev)
{
	const struct mcp251xfd_config *dev_cfg = dev->config;
	uint16_t cmd = sys_cpu_to_be16(MCP251XFD_SPI_INSTRUCTION_RESET);
	const struct spi_buf tx_buf = {.buf = &cmd, .len = sizeof(cmd),};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};
	int ret;

	/* device can only be reset when in configuration mode */
	ret = mcp251xfd_set_mode_internal(dev, MCP251XFD_REG_CON_MODE_CONFIG);
	if (ret < 0) {
		return ret;
	}

	return spi_write_dt(&dev_cfg->bus, &tx);
}

static int mcp251xfd_init(const struct device *dev)
{
	const struct mcp251xfd_config *dev_cfg = dev->config;
	struct mcp251xfd_data *dev_data = dev->data;
	uint32_t *reg;
	uint8_t opmod;
	int ret;
	struct can_timing timing = { 0 };
#if defined(CONFIG_CAN_FD_MODE)
	struct can_timing timing_data = { 0 };
#endif

	dev_data->dev = dev;

	if (dev_cfg->clk_dev != NULL) {
		uint32_t clk_id = dev_cfg->clk_id;

		if (!device_is_ready(dev_cfg->clk_dev)) {
			LOG_ERR("Clock controller not ready");
			return -ENODEV;
		}

		ret = clock_control_on(dev_cfg->clk_dev, (clock_control_subsys_t)clk_id);
		if (ret < 0) {
			LOG_ERR("Failed to enable clock [%d]", ret);
			return ret;
		}
	}

	k_sem_init(&dev_data->int_sem, 0, 1);
	k_sem_init(&dev_data->tx_sem, MCP251XFD_TX_QUEUE_ITEMS, MCP251XFD_TX_QUEUE_ITEMS);

	k_mutex_init(&dev_data->mutex);

	if (!spi_is_ready_dt(&dev_cfg->bus)) {
		LOG_ERR("SPI bus %s not ready", dev_cfg->bus.bus->name);
		return -ENODEV;
	}

	if (!gpio_is_ready_dt(&dev_cfg->int_gpio_dt)) {
		LOG_ERR("GPIO port not ready");
		return -ENODEV;
	}

	if (gpio_pin_configure_dt(&dev_cfg->int_gpio_dt, GPIO_INPUT) < 0) {
		LOG_ERR("Unable to configure GPIO pin");
		return -EINVAL;
	}

	gpio_init_callback(&dev_data->int_gpio_cb, mcp251xfd_int_gpio_callback,
			   BIT(dev_cfg->int_gpio_dt.pin));

	if (gpio_add_callback_dt(&dev_cfg->int_gpio_dt, &dev_data->int_gpio_cb) < 0) {
		return -EINVAL;
	}

	if (gpio_pin_interrupt_configure_dt(&dev_cfg->int_gpio_dt, GPIO_INT_LEVEL_ACTIVE) < 0) {
		return -EINVAL;
	}

	k_thread_create(&dev_data->int_thread, dev_data->int_thread_stack,
			CONFIG_CAN_MCP251XFD_INT_THREAD_STACK_SIZE,
			(k_thread_entry_t)mcp251xfd_int_thread, (void *)dev, NULL, NULL,
			K_PRIO_COOP(CONFIG_CAN_MCP251XFD_INT_THREAD_PRIO), 0, K_NO_WAIT);

	(void)k_thread_name_set(&dev_data->int_thread, "MCP251XFD interrupt thread");

	ret = mcp251xfd_reset(dev);
	if (ret < 0) {
		LOG_ERR("Failed to reset the device [%d]", ret);
		goto done;
	}

	ret = mcp251xfd_init_timing_struct(&timing, dev, &dev_cfg->timing_params);
	if (ret < 0) {
		LOG_ERR("Can't find timing for given param");
		goto done;
	}

#if defined(CONFIG_CAN_FD_MODE)
	ret = mcp251xfd_init_timing_struct_data(&timing_data, dev, &dev_cfg->timing_params_data);
	if (ret < 0) {
		LOG_ERR("Can't find data timing for given param");
		goto done;
	}
#endif

	reg = mcp251xfd_read_crc(dev, MCP251XFD_REG_CON, MCP251XFD_REG_SIZE);
	if (!reg) {
		ret = -EINVAL;
		goto done;
	}

	*reg = sys_le32_to_cpu(*reg);

	opmod = FIELD_GET(MCP251XFD_REG_CON_OPMOD_MASK, *reg);

	if (opmod != MCP251XFD_REG_CON_MODE_CONFIG) {
		LOG_ERR("Device did not reset into configuration mode [%d]", opmod);
		ret = -EIO;
		goto done;
	}

	dev_data->current_mcp251xfd_mode = MCP251XFD_REG_CON_MODE_CONFIG;

	ret = mcp251xfd_init_con_reg(dev);
	if (ret < 0) {
		goto done;
	}

	ret = mcp251xfd_init_osc_reg(dev);
	if (ret < 0) {
		goto done;
	}

	ret = mcp251xfd_init_iocon_reg(dev);
	if (ret < 0) {
		goto done;
	}

	ret = mcp251xfd_init_int_reg(dev);
	if (ret < 0) {
		goto done;
	}

	ret = mcp251xfd_set_tdc(dev, false, 0);
	if (ret < 0) {
		goto done;
	}

#if defined(CONFIG_CAN_RX_TIMESTAMP)
	ret = mcp251xfd_init_tscon(dev);
	if (ret < 0) {
		goto done;
	}
#endif

	ret = mcp251xfd_init_tef_fifo(dev);
	if (ret < 0) {
		goto done;
	}

	ret = mcp251xfd_init_tx_queue(dev);
	if (ret < 0) {
		goto done;
	}

	ret = mcp251xfd_init_rx_fifo(dev);
	if (ret < 0) {
		goto done;
	}

	LOG_DBG("%d TX FIFOS: 1 element", MCP251XFD_TX_QUEUE_ITEMS);
	LOG_DBG("1 RX FIFO: %d elements", MCP251XFD_RX_FIFO_ITEMS);
	LOG_DBG("%db of %db RAM Allocated",
		MCP251XFD_TEF_FIFO_SIZE + MCP251XFD_TX_QUEUE_SIZE + MCP251XFD_RX_FIFO_SIZE,
		MCP251XFD_RAM_SIZE);

done:
	ret = can_set_timing(dev, &timing);
	if (ret < 0) {
		return ret;
	}

#if defined(CONFIG_CAN_FD_MODE)
	ret = can_set_timing_data(dev, &timing_data);
	if (ret < 0) {
		return ret;
	}
#endif

	return ret;
}

static const struct can_driver_api mcp251xfd_api_funcs = {
	.get_capabilities = mcp251xfd_get_capabilities,
	.set_mode = mcp251xfd_set_mode,
	.set_timing = mcp251xfd_set_timing,
#if defined(CONFIG_CAN_FD_MODE)
	.set_timing_data = mcp251xfd_set_timing_data,
#endif
	.start = mcp251xfd_start,
	.stop = mcp251xfd_stop,
	.send = mcp251xfd_send,
	.add_rx_filter = mcp251xfd_add_rx_filter,
	.remove_rx_filter = mcp251xfd_remove_rx_filter,
#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
	.recover = mcp251xfd_recover,
#endif
	.get_state = mcp251xfd_get_state,
	.set_state_change_callback = mcp251xfd_set_state_change_callback,
	.get_core_clock = mcp251xfd_get_core_clock,
	.get_max_filters = mcp251xfd_get_max_filters,
	.timing_min = {
		.sjw = 1,
		.prop_seg = 0,
		.phase_seg1 = 2,
		.phase_seg2 = 1,
		.prescaler = 1,
	},
	.timing_max = {
		.sjw = 128,
		.prop_seg = 0,
		.phase_seg1 = 256,
		.phase_seg2 = 128,
		.prescaler = 256,
	},
#if defined(CONFIG_CAN_FD_MODE)
	.timing_data_min = {
		.sjw = 1,
		.prop_seg = 0,
		.phase_seg1 = 1,
		.phase_seg2 = 1,
		.prescaler = 1,
	},
	.timing_data_max = {
		.sjw = 16,
		.prop_seg = 0,
		.phase_seg1 = 32,
		.phase_seg2 = 16,
		.prescaler = 256,
	},
#endif
};

#define MCP251XFD_SET_TIMING_MACRO(inst, type)                                                     \
	.timing_params##type = {                                                                   \
		.sjw = DT_INST_PROP(inst, sjw##type),                                              \
		.prop_seg = DT_INST_PROP_OR(inst, prop_seg##type, 0),                              \
		.phase_seg1 = DT_INST_PROP_OR(inst, phase_seg1##type, 0),                          \
		.phase_seg2 = DT_INST_PROP_OR(inst, phase_seg2##type, 0),                          \
	}

#if defined(CONFIG_CAN_FD_MODE)
#define MCP251XFD_SET_TIMING(inst)                                                                 \
	MCP251XFD_SET_TIMING_MACRO(inst,),                                                         \
	MCP251XFD_SET_TIMING_MACRO(inst, _data)
#else
#define MCP251XFD_SET_TIMING(inst)                                                                 \
	MCP251XFD_SET_TIMING_MACRO(inst,)
#endif

#define MCP251XFD_SET_CLOCK(inst)                                                                  \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, clocks),                                           \
		    (.clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),                          \
		     .clk_id = DT_INST_CLOCKS_CELL(inst, id)),                                     \
		    ())

#define MCP251XFD_INIT(inst)                                                                       \
	static K_KERNEL_STACK_DEFINE(mcp251xfd_int_stack_##inst,                                   \
				     CONFIG_CAN_MCP251XFD_INT_THREAD_STACK_SIZE);                  \
                                                                                                   \
	static struct mcp251xfd_data mcp251xfd_data_##inst = {                                     \
		.int_thread_stack = mcp251xfd_int_stack_##inst,                                    \
	};                                                                                         \
	static const struct mcp251xfd_config mcp251xfd_config_##inst = {                           \
		.common = CAN_DT_DRIVER_CONFIG_INST_GET(inst, 8000000),                            \
		.bus = SPI_DT_SPEC_INST_GET(inst, SPI_WORD_SET(8), 0),                             \
		.int_gpio_dt = GPIO_DT_SPEC_INST_GET(inst, int_gpios),                             \
                                                                                                   \
		.sof_on_clko = DT_INST_PROP(inst, sof_on_clko),                                    \
		.clko_div = DT_INST_ENUM_IDX(inst, clko_div),                                      \
		.pll_enable = DT_INST_PROP(inst, pll_enable),                                      \
		.timestamp_prescaler = DT_INST_PROP(inst, timestamp_prescaler),                    \
                                                                                                   \
		.osc_freq = DT_INST_PROP(inst, osc_freq),                                          \
		MCP251XFD_SET_TIMING(inst),                                                        \
		.rx_fifo = {.ram_start_addr = MCP251XFD_RX_FIFO_START_ADDR,                        \
			    .reg_fifocon_addr = MCP251XFD_REG_FIFOCON(MCP251XFD_RX_FIFO_IDX),      \
			    .capacity = MCP251XFD_RX_FIFO_ITEMS,                                   \
			    .item_size = MCP251XFD_RX_FIFO_ITEM_SIZE,                              \
			    .msg_handler = mcp251xfd_rx_fifo_handler},                             \
		.tef_fifo = {.ram_start_addr = MCP251XFD_TEF_FIFO_START_ADDR,                      \
			     .reg_fifocon_addr = MCP251XFD_REG_TEFCON,                             \
			     .capacity = MCP251XFD_TEF_FIFO_ITEMS,                                 \
			     .item_size = MCP251XFD_TEF_FIFO_ITEM_SIZE,                            \
			     .msg_handler = mcp251xfd_tef_fifo_handler},                           \
		MCP251XFD_SET_CLOCK(inst)                                                          \
	};                                                                                         \
                                                                                                   \
	CAN_DEVICE_DT_INST_DEFINE(inst, mcp251xfd_init, NULL, &mcp251xfd_data_##inst,             \
				  &mcp251xfd_config_##inst, POST_KERNEL, CONFIG_CAN_INIT_PRIORITY, \
				  &mcp251xfd_api_funcs);

DT_INST_FOREACH_STATUS_OKAY(MCP251XFD_INIT)
