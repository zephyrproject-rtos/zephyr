/*
 * Copyright (c) 2020 Abram Early
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microchip_mcp25xxfd

#include <kernel.h>
#include <device.h>
#include <drivers/spi.h>
#include <drivers/gpio.h>

#define LOG_LEVEL CONFIG_CAN_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(mcp25xxfd_can);

#include "can_mcp25xxfd.h"

#define SP_IS_SET(inst) DT_INST_NODE_HAS_PROP(inst, sample_point) ||

/* Macro to exclude the sample point algorithm from compilation if not used
 * Without the macro, the algorithm would always waste ROM
 */
#define USE_SP_ALGO (DT_INST_FOREACH_STATUS_OKAY(SP_IS_SET) 0)

static int mcp25xxfd_reset(const struct device *dev)
{
	uint8_t cmd_buf[] = { 0x00, 0x00 };
	const struct spi_buf tx_buf = {
		.buf = cmd_buf,
		.len = sizeof(cmd_buf),
	};
	const struct spi_buf_set tx = { .buffers = &tx_buf, .count = 1U };

	return spi_write(DEV_DATA(dev)->spi, &DEV_DATA(dev)->spi_cfg, &tx);
}

static int mcp25xxfd_read(const struct device *dev, uint16_t address, void *rxd,
			  uint8_t rx_len)
{
	__ASSERT(address < 0x400 || address >= 0xC00 ||
		 (address % 4 == 0 && rx_len % 4 == 0),
		 "Address and Length must be word aligned in RAM");
	uint8_t cmd_buf[2 + rx_len];

	cmd_buf[0] = (MCP25XXFD_OPCODE_READ << 4) + ((address >> 8) & 0x0F);
	cmd_buf[1] = address & 0xFF;
	const struct spi_buf tx_buf[] = {
		{ .buf = cmd_buf, .len = sizeof(cmd_buf) },
	};
	const struct spi_buf rx_buf[] = {
		{ .buf = cmd_buf, .len = sizeof(cmd_buf) },
	};
	const struct spi_buf_set tx = { .buffers = tx_buf,
					.count = ARRAY_SIZE(tx_buf) };
	const struct spi_buf_set rx = { .buffers = rx_buf,
					.count = ARRAY_SIZE(rx_buf) };
	int ret;

	ret = spi_transceive(DEV_DATA(dev)->spi, &DEV_DATA(dev)->spi_cfg,
			     &tx, &rx);
	memcpy(rxd, &cmd_buf[2], rx_len);
	if (ret < 0) {
		LOG_ERR("Failed to read %d bytes from 0x%03x", rx_len, address);
	}
	return ret;
}

static int mcp25xxfd_write(const struct device *dev, uint16_t address,
			   void *txd, uint8_t tx_len)
{
	__ASSERT(address < 0x400 || address >= 0xC00 ||
		 (address % 4 == 0 && tx_len % 4 == 0),
		 "Address and Length must be word aligned in RAM");
	uint8_t cmd_buf[2 + tx_len];

	cmd_buf[0] = (MCP25XXFD_OPCODE_WRITE << 4) +
		     ((address >> 8) & 0xF);
	cmd_buf[1] = address & 0xFF;
	const struct spi_buf tx_buf[] = {
		{ .buf = cmd_buf, .len = sizeof(cmd_buf) },
	};
	const struct spi_buf_set tx = { .buffers = tx_buf,
					.count = ARRAY_SIZE(tx_buf) };
	int ret;

	memcpy(&cmd_buf[2], txd, tx_len);
	ret = spi_write(DEV_DATA(dev)->spi, &DEV_DATA(dev)->spi_cfg, &tx);
	if (ret < 0) {
		LOG_ERR("Failed to write %d bytes to 0x%03x", tx_len, address);
	}
	return ret;
}

static inline int mcp25xxfd_readb(const struct device *dev, uint16_t address,
				  void *rxd)
{
	return mcp25xxfd_read(dev, address, rxd, 1);
}

static inline int mcp25xxfd_writeb(const struct device *dev, uint16_t address,
				   void *txd)
{
	return mcp25xxfd_write(dev, address, txd, 1);
}

static inline int mcp25xxfd_readw(const struct device *dev, uint16_t address,
				  void *rxd)
{
	return mcp25xxfd_read(dev, address, rxd, 4);
}

static inline int mcp25xxfd_writew(const struct device *dev, uint16_t address,
				   void *txd)
{
	return mcp25xxfd_write(dev, address, txd, 4);
}

static int mcp25xxfd_fifo_read(const struct device *dev, uint16_t fifo_address,
			       void *rxd, uint8_t rx_len)
{
	struct mcp25xxfd_data *dev_data = DEV_DATA(dev);
	union mcp25xxfd_fifo fiforegs;
	int ret;

	k_mutex_lock(&dev_data->mutex, K_FOREVER);

	ret = mcp25xxfd_read(dev, fifo_address, &fiforegs, sizeof(fiforegs));
	if (ret < 0) {
		goto done;
	}

	if (!fiforegs.sta.FNEIF) {
		ret = -ENODATA;
		goto done;
	}

	ret = mcp25xxfd_read(dev, 0x400 + fiforegs.ua, rxd, rx_len);
	if (ret < 0) {
		goto done;
	}

	fiforegs.con.UINC = 1;
	ret = mcp25xxfd_writeb(dev, fifo_address + 1, &fiforegs.con.bytes[1]);

done:
	k_mutex_unlock(&dev_data->mutex);
	return ret;
}

static int mcp25xxfd_fifo_write(const struct device *dev, uint16_t fifo_address,
				void *txd, uint8_t tx_len)
{
	struct mcp25xxfd_data *dev_data = DEV_DATA(dev);
	union mcp25xxfd_fifo fiforegs;
	int ret;

	k_mutex_lock(&dev_data->mutex, K_FOREVER);

	ret = mcp25xxfd_read(dev, fifo_address, &fiforegs, sizeof(fiforegs));
	if (ret < 0) {
		goto done;
	}

	if (!fiforegs.sta.FNEIF) {
		ret = -ENOMEM;
		goto done;
	}

	ret = mcp25xxfd_write(dev, 0x400 + fiforegs.ua, txd, tx_len);
	if (ret < 0) {
		goto done;
	}

	fiforegs.con.UINC = 1;
	fiforegs.con.TXREQ = 1;
	ret = mcp25xxfd_writeb(dev, fifo_address + 1, &fiforegs.con.bytes[1]);

done:
	k_mutex_unlock(&dev_data->mutex);
	return ret;
}

static void mcp25xxfd_zcanframe_to_txobj(const struct zcan_frame *src,
					   struct mcp25xxfd_txobj *dst)
{
	memset(dst, 0, offsetof(struct mcp25xxfd_txobj, DATA));

	if (src->id_type == CAN_STANDARD_IDENTIFIER) {
		dst->SID = src->id;
	} else {
		dst->SID = src->id >> 18;
		dst->EID = src->id;
		dst->IDE = 1;
	}
	dst->BRS = src->brs;
	dst->RTR = src->rtr == CAN_REMOTEREQUEST;
	dst->DLC = src->dlc;
#if defined(CONFIG_CAN_FD_MODE)
	dst->FDF = src->fd;
#endif

	memcpy(dst->DATA, src->data, MIN(can_dlc_to_bytes(src->dlc), CAN_MAX_DLEN));
}

static void mcp25xxfd_rxobj_to_zcanframe(const struct mcp25xxfd_rxobj *src,
					 struct zcan_frame *dst)
{
	memset(dst, 0, offsetof(struct zcan_frame, data));

	if (src->IDE) {
		dst->id = src->EID | (src->SID << 18);
		dst->id_type = CAN_EXTENDED_IDENTIFIER;
	} else {
		dst->id = src->SID;
		dst->id_type = CAN_STANDARD_IDENTIFIER;
	}
	dst->brs = src->BRS;
	dst->rtr = src->RTR;
	dst->dlc = src->DLC;
#if defined(CONFIG_CAN_FD_MODE)
	dst->fd = src->FDF;
#endif
#if defined(CONFIG_CAN_RX_TIMESTAMP)
	dst->timestamp = src->RXMSGTS;
#endif

	memcpy(dst->data, src->DATA, MIN(can_dlc_to_bytes(src->DLC), CAN_MAX_DLEN));
}

static int mcp25xxfd_get_raw_mode(const struct device *dev, uint8_t *mode)
{
	struct mcp25xxfd_data *dev_data = DEV_DATA(dev);
	union mcp25xxfd_con con;
	int ret;

	k_mutex_lock(&dev_data->mutex, K_FOREVER);
	ret = mcp25xxfd_readb(dev, MCP25XXFD_REG_CON + 2, &con.byte[2]);
	k_mutex_unlock(&dev_data->mutex);
	*mode = con.OPMOD;
	return ret;
}

static int mcp25xxfd_set_raw_mode(const struct device *dev, uint8_t mode)
{
	struct mcp25xxfd_data *dev_data = DEV_DATA(dev);
	union mcp25xxfd_con con;
	int ret;

	while (true) {
		k_mutex_lock(&dev_data->mutex, K_FOREVER);
		ret = mcp25xxfd_readw(dev, MCP25XXFD_REG_CON, &con);
		if (ret < 0 || con.OPMOD == mode) {
			k_mutex_unlock(&dev_data->mutex);
			break;
		}

		if (con.OPMOD == MCP25XXFD_OPMODE_CONFIGURATION) {
			/* Configuration mode can be switched to any other mode */
			con.REQMOD = mode;
		} else if (con.OPMOD == MCP25XXFD_OPMODE_NORMAL_CANFD ||
			   con.OPMOD == MCP25XXFD_OPMODE_NORMAL_CAN2) {
			/* Normal modes can only be directly switched to Sleep, Restricted, or Listen-Only modes */
			if (mode == MCP25XXFD_OPMODE_SLEEP ||
			    mode == MCP25XXFD_OPMODE_RESTRICTED ||
			    mode == MCP25XXFD_OPMODE_LISTEN_ONLY) {
				con.REQMOD = mode;
			} else {
				con.REQMOD = MCP25XXFD_OPMODE_CONFIGURATION;
			}
		} else if (con.OPMOD == MCP25XXFD_OPMODE_LISTEN_ONLY) {
			/* Listen-Only mode can be directly switched back to normal modes */
			if (mode == MCP25XXFD_OPMODE_NORMAL_CANFD ||
			    mode == MCP25XXFD_OPMODE_NORMAL_CAN2) {
				con.REQMOD = mode;
			} else {
				con.REQMOD = MCP25XXFD_OPMODE_CONFIGURATION;
			}
		} else {
			/* Otherwise, the device must be put into configuration mode first */
			con.REQMOD = MCP25XXFD_OPMODE_CONFIGURATION;
		}

		LOG_DBG("OPMOD: #%d, REQMOD #%d", con.OPMOD, con.REQMOD);
		ret = mcp25xxfd_writeb(dev, MCP25XXFD_REG_CON + 3,
				       &con.byte[3]);
		k_mutex_unlock(&dev_data->mutex);
		if (ret < 0) {
			break;
		}

		ret = k_sem_take(&dev_data->mode_sem, K_MSEC(2));
		if (ret == -EAGAIN) {
			return CAN_TIMEOUT;
		}
	}
	return ret;
}

static int mcp25xxfd_set_mode(const struct device *dev, enum can_mode mode)
{
	switch (mode) {
	case CAN_NORMAL_MODE:
#if defined(CONFIG_CAN_FD_MODE)
		return mcp25xxfd_set_raw_mode(dev,
					      MCP25XXFD_OPMODE_NORMAL_CANFD);
#else
		return mcp25xxfd_set_raw_mode(dev,
					      MCP25XXFD_OPMODE_NORMAL_CAN2);
#endif
	case CAN_LOOPBACK_MODE:
		return mcp25xxfd_set_raw_mode(dev,
					      MCP25XXFD_OPMODE_EXT_LOOPBACK);
	default:
		LOG_ERR("Unsupported CAN Mode %u", mode);
	case CAN_SILENT_MODE:
		return mcp25xxfd_set_raw_mode(dev,
					      MCP25XXFD_OPMODE_LISTEN_ONLY);
	}
}

static int mcp25xxfd_set_timing(const struct device *dev,
				const struct can_timing *timing,
				const struct can_timing *timing_data)
{
	struct mcp25xxfd_data *dev_data = DEV_DATA(dev);
	uint8_t mode;
	int ret;

#if defined(CONFIG_CAN_FD_MODE)
	if (!timing || !timing_data) {
#else
	if (!timing) {
#endif
		return -EINVAL;
	}

	k_mutex_lock(&dev_data->mutex, K_FOREVER);

	ret = mcp25xxfd_get_raw_mode(dev, &mode);
	if (ret < 0) {
		goto done;
	}
	ret = mcp25xxfd_set_raw_mode(dev, MCP25XXFD_OPMODE_CONFIGURATION);
	if (ret < 0) {
		goto done;
	}

	union mcp25xxfd_nbtcfg nbtcfg = {
		.BRP = timing->prescaler - 1,
		.TSEG1 = (timing->prop_seg + timing->phase_seg1) - 1,
		.TSEG2 = timing->phase_seg2 - 1,
		.SJW = timing->sjw - 1,
	};
	ret = mcp25xxfd_writew(dev, MCP25XXFD_REG_NBTCFG, nbtcfg.byte);
	if (ret < 0) {
		LOG_ERR("Failed to write device configuration [%d]", ret);
		goto done;
	}

#if defined(CONFIG_CAN_FD_MODE)
	union mcp25xxfd_dbtcfg ndtcfg = {
		.BRP = timing_data->prescaler - 1,
		.TSEG1 = (timing_data->prop_seg + timing_data->phase_seg1) - 1,
		.TSEG2 = timing_data->phase_seg2 - 1,
		.SJW = timing_data->sjw - 1,
	};
	ret = mcp25xxfd_writew(dev, MCP25XXFD_REG_DBTCFG, ndtcfg.byte);
	if (ret < 0) {
		LOG_ERR("Failed to write device configuration [%d]", ret);
		goto done;
	}
#endif

	union mcp25xxfd_tdc tdc = {
		.EDGFLTEN = 0,
		.SID11EN = 0,
#if defined(CONFIG_CAN_FD_MODE)
		.TDCMOD = MCP25XXFD_TDCMOD_AUTO,
		.TDCO = timing_data->prescaler *
			(timing_data->prop_seg + timing_data->phase_seg1),
#else
		.TDCMOD = MCP25XXFD_TDCMOD_DISABLED,
#endif
	};
	ret = mcp25xxfd_writew(dev, MCP25XXFD_REG_TDC, &tdc);
	if (ret < 0) {
		LOG_ERR("Failed to write device configuration [%d]", ret);
		goto done;
	}

#if defined(CONFIG_CAN_RX_TIMESTAMP)
	union mcp25xxfd_tscon tscon = {
		.TBCEN = 1,
		.TSRES = 0,
		.TSEOF = 0,
		.TBCPRE = timing->prescaler - 1,
	};
	ret = mcp25xxfd_writew(dev, MCP25XXFD_REG_TSCON, &tscon);
	if (ret < 0) {
		LOG_ERR("Failed to write device configuration [%d]", ret);
		goto done;
	}
#endif

	ret = mcp25xxfd_set_raw_mode(dev, mode);
	if (ret < 0) {
		goto done;
	}

done:
	k_mutex_unlock(&dev_data->mutex);

	return ret;
}

static int mcp25xxfd_send(const struct device *dev,
			  const struct zcan_frame *msg, k_timeout_t timeout,
			  can_tx_callback_t callback, void *callback_arg)
{
	struct mcp25xxfd_data *dev_data = DEV_DATA(dev);
	struct mcp25xxfd_txobj tx_frame;
	uint8_t mailbox_idx = 0;
	int ret;

	LOG_DBG("Sending %d bytes. Id: 0x%x, ID type: %s %s %s %s",
		can_dlc_to_bytes(msg->dlc), msg->id,
		msg->id_type == CAN_STANDARD_IDENTIFIER ?
		"standard" : "extended",
		msg->rtr == CAN_DATAFRAME ? "" : "RTR",
		msg->fd == CAN_DATAFRAME ? "" : "FD frame",
		msg->brs == CAN_DATAFRAME ? "" : "BRS");

	if (msg->fd != 1 && msg->dlc > CAN_MAX_DLC) {
		LOG_ERR("DLC of %d without fd flag set.", msg->dlc);
		return CAN_TX_EINVAL;
	}

	if (k_sem_take(&dev_data->tx_sem, timeout) != 0) {
		return CAN_TIMEOUT;
	}

	k_mutex_lock(&dev_data->mutex, K_FOREVER);
	for (; mailbox_idx < MCP25XXFD_TXFIFOS; mailbox_idx++) {
		if ((BIT(mailbox_idx) & dev_data->mailbox_usage) == 0) {
			dev_data->mailbox_usage |= BIT(mailbox_idx);
			break;
		}
	}
	k_mutex_unlock(&dev_data->mutex);

	if (mailbox_idx >= MCP25XXFD_TXFIFOS) {
		k_sem_give(&dev_data->tx_sem);
		return CAN_TX_ERR;
	}

	dev_data->mailbox[mailbox_idx].cb = callback;
	dev_data->mailbox[mailbox_idx].cb_arg = callback_arg;

	mcp25xxfd_zcanframe_to_txobj(msg, &tx_frame);
	tx_frame.SEQ = mailbox_idx;
	ret = mcp25xxfd_fifo_write(dev, MCP25XXFD_REG_FIFOCON(mailbox_idx), &tx_frame,
				   offsetof(struct mcp25xxfd_txobj, DATA) + ROUND_UP(can_dlc_to_bytes(msg->dlc), 4));

	if (ret >= 0) {
		if (callback == NULL) {
			k_sem_take(&dev_data->mailbox[mailbox_idx].tx_sem,
				   timeout);
		}
	} else {
		k_mutex_lock(&dev_data->mutex, K_FOREVER);
		dev_data->mailbox_usage &= ~BIT(mailbox_idx);
		k_mutex_unlock(&dev_data->mutex);
		k_sem_give(&dev_data->tx_sem);
	}

	return ret;
}

static int mcp25xxfd_attach_isr(const struct device *dev,
				can_rx_callback_t rx_cb, void *cb_arg,
				const struct zcan_filter *filter)
{
	struct mcp25xxfd_data *dev_data = DEV_DATA(dev);
	union mcp25xxfd_fltcon fltcon;
	union mcp25xxfd_fltobj fltobj = { .word = 0 };
	union mcp25xxfd_mask mask = { .word = 0 };
	int filter_idx = 0;
	int ret;

	__ASSERT(rx_cb != NULL, "rx_cb can not be null");
	k_mutex_lock(&dev_data->mutex, K_FOREVER);

	while ((BIT(filter_idx) & dev_data->filter_usage) &&
	       (filter_idx < CONFIG_CAN_MAX_FILTER)) {
		filter_idx++;
	}

	if (filter_idx < CONFIG_CAN_MAX_FILTER) {
		if (filter->id_type == CAN_STANDARD_IDENTIFIER) {
			fltobj.SID = filter->id;
			mask.MSID = filter->id_mask;
		} else {
			fltobj.SID = filter->id >> 18;
			mask.MSID = filter->id_mask >> 18;
			fltobj.EID = filter->id;
			mask.MEID = filter->id_mask;
			fltobj.EXIDE = 1;
		}
		mask.MIDE = 1;
		ret = mcp25xxfd_writew(dev, MCP25XXFD_REG_FLTOBJ(filter_idx),
				       &fltobj);
		if (ret < 0) {
			LOG_ERR("Failed to write register [%d]", ret);
			goto done;
		}
		ret = mcp25xxfd_writew(dev, MCP25XXFD_REG_MASK(filter_idx),
				       &mask);
		if (ret < 0) {
			LOG_ERR("Failed to write register [%d]", ret);
			goto done;
		}

		fltcon.FLTEN = 1;
		fltcon.FLTBP = MCP25XXFD_RXFIFO_IDX;
		ret = mcp25xxfd_writeb(dev, MCP21518FD_REG_FLTCON(filter_idx),
				       fltcon.byte);
		if (ret < 0) {
			LOG_ERR("Failed to write register [%d]", ret);
			goto done;
		}

		dev_data->filter_usage |= BIT(filter_idx);
		dev_data->filter[filter_idx] = *filter;
		dev_data->rx_cb[filter_idx] = rx_cb;
		dev_data->cb_arg[filter_idx] = cb_arg;
	} else {
		filter_idx = CAN_NO_FREE_FILTER;
	}
done:
	k_mutex_unlock(&dev_data->mutex);

	return filter_idx;
}

static void mcp25xxfd_detach(const struct device *dev, int filter_nr)
{
	struct mcp25xxfd_data *dev_data = DEV_DATA(dev);
	union mcp25xxfd_fltcon fltcon;

	k_mutex_lock(&dev_data->mutex, K_FOREVER);

	dev_data->filter_usage &= ~BIT(filter_nr);
	fltcon.FLTEN = 0;
	mcp25xxfd_writeb(dev, MCP21518FD_REG_FLTCON(filter_nr), &fltcon);

	k_mutex_unlock(&dev_data->mutex);
}

static void mcp25xxfd_register_state_change_isr(const struct device *dev,
						can_state_change_isr_t isr)
{
	struct mcp25xxfd_data *dev_data = DEV_DATA(dev);

	dev_data->state_change_isr = isr;
}

static enum can_state mcp25xxfd_get_state(const struct device *dev,
					  struct can_bus_err_cnt *err_cnt)
{
	return DEV_DATA(dev)->state;
}

static int mcp25xxfd_get_core_clock(const struct device *dev, uint32_t *rate)
{
	const struct mcp25xxfd_config *dev_cfg = DEV_CFG(dev);

	*rate = dev_cfg->osc_freq;
	return 0;
}

#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
static void mcp25xxfd_recover(const struct device *dev, k_timeout_t timeout)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(timeout);
}
#endif

static void mcp25xxfd_rx(const struct device *dev, int fifo_idx)
{
	struct mcp25xxfd_data *dev_data = DEV_DATA(dev);
	struct mcp25xxfd_rxobj rx_frame;
	struct zcan_frame msg;

	while (mcp25xxfd_fifo_read(dev, MCP25XXFD_REG_FIFOCON(fifo_idx), &rx_frame,
				   sizeof(rx_frame)) >= 0) {
		mcp25xxfd_rxobj_to_zcanframe(&rx_frame, &msg);
		if (dev_data->filter_usage & BIT(rx_frame.FILHIT)) {
			dev_data->rx_cb[rx_frame.FILHIT](
				&msg, dev_data->cb_arg[rx_frame.FILHIT]);
		}
	}
}

static void mcp25xxfd_tx_done(const struct device *dev)
{
	struct mcp25xxfd_data *dev_data = DEV_DATA(dev);
	struct mcp25xxfd_tefobj tefobj;
	uint8_t mailbox_idx;

	while (mcp25xxfd_fifo_read(dev, MCP25XXFD_REG_TEFCON, &tefobj,
				   sizeof(tefobj)) >= 0) {
		mailbox_idx = tefobj.SEQ;
		if (dev_data->mailbox[mailbox_idx].cb == NULL) {
			k_sem_give(&dev_data->mailbox[mailbox_idx].tx_sem);
		} else {
			dev_data->mailbox[mailbox_idx].cb(
				0, dev_data->mailbox[mailbox_idx].cb_arg);
		}
		k_mutex_lock(&dev_data->mutex, K_FOREVER);
		dev_data->mailbox_usage &= ~BIT(mailbox_idx);
		k_mutex_unlock(&dev_data->mutex);
		k_sem_give(&dev_data->tx_sem);
	}
}

static void mcp25xxfd_int_thread(const struct device *dev)
{
	const struct mcp25xxfd_config *dev_cfg = DEV_CFG(dev);
	struct mcp25xxfd_data *dev_data = DEV_DATA(dev);
	union mcp25xxfd_intregs intregs;
	union mcp25xxfd_trec trec;
	uint32_t ints_before;
	int ret;

	while (1) {
		k_sem_take(&dev_data->int_sem, K_FOREVER);

		while (1) {
			ret = mcp25xxfd_read(dev, MCP25XXFD_REG_INTREGS,
					     &intregs, sizeof(intregs));
			if (ret < 0) {
				continue;
			}
			ints_before = intregs.ints.word;

			if (intregs.ints.RXIF) {
				mcp25xxfd_rx(dev, MCP25XXFD_RXFIFO_IDX);
			}

			if (intregs.ints.TEFIF) {
				mcp25xxfd_tx_done(dev);
			}

			if (intregs.ints.MODIF) {
				k_sem_give(&dev_data->mode_sem);
				intregs.ints.MODIF = 0;
			}

			if (intregs.ints.CERRIF) {
				ret = mcp25xxfd_readw(dev, MCP25XXFD_REG_TREC,
						      &trec);
				if (ret >= 0) {
					enum can_state new_state;

					if (trec.TXBO) {
						new_state = CAN_BUS_OFF;

						/* Upon entering bus-off, all the fifos are reset. */
						LOG_DBG("All FIFOs Reset");
						k_mutex_lock(&dev_data->mutex, K_FOREVER);
						for (int i = 0; i < MCP25XXFD_TXFIFOS; i++) {
							if (!(dev_data->mailbox_usage & BIT(i))) {
								continue;
							}
							if (dev_data->mailbox[i].cb == NULL) {
								k_sem_give(&dev_data->mailbox[i].tx_sem);
							} else {
								dev_data->mailbox[i].cb(
									CAN_TX_BUS_OFF, dev_data->mailbox[i].cb_arg);
							}
							dev_data->mailbox_usage &= ~BIT(i);
							k_sem_give(&dev_data->tx_sem);
						}
						k_mutex_unlock(&dev_data->mutex);
					} else if (trec.TXBP || trec.RXBP) {
						new_state = CAN_ERROR_PASSIVE;
					} else {
						new_state = CAN_ERROR_ACTIVE;
					}
					if (dev_data->state != new_state) {
						LOG_DBG("State %d -> %d (tx: %d, rx: %d)", dev_data->state, new_state, trec.TEC, trec.REC);
						dev_data->state = new_state;
						if (dev_data->state_change_isr) {
							struct can_bus_err_cnt
								err_cnt;
							err_cnt.rx_err_cnt =
								trec.REC;
							err_cnt.tx_err_cnt =
								trec.TEC;
							dev_data->state_change_isr(
								new_state,
								err_cnt);
						}
					}

					intregs.ints.CERRIF = 0;
				}
			}

			if (ints_before != intregs.ints.word) {
				mcp25xxfd_writew(dev, MCP25XXFD_REG_INT, &intregs.ints);
			}

			/* Break from loop if INT pin is inactive */
			ret = gpio_pin_get(dev_data->int_gpio,
					   dev_cfg->int_pin);
			if (ret <= 0) {
				/* All interrupt flags handled, but we'll abort if
				 * an error occurs to avoid deadlock. */
				break;
			}
		}

		/* Re-enable pin interrupts */
		if (gpio_pin_interrupt_configure(dev_data->int_gpio, dev_cfg->int_pin,
						 GPIO_INT_LEVEL_ACTIVE)) {
			LOG_ERR("Couldn't enable pin interrupt");
			k_oops();
		}
	}
}

static void mcp25xxfd_int_gpio_callback(const struct device *dev,
					struct gpio_callback *cb, uint32_t pins)
{
	struct mcp25xxfd_data *dev_data =
		CONTAINER_OF(cb, struct mcp25xxfd_data, int_gpio_cb);

	/* Disable pin interrupts */
	if (gpio_pin_interrupt_configure(dev, dev_data->int_pin, GPIO_INT_DISABLE)) {
		LOG_ERR("Couldn't disable pin interrupt");
		k_oops();
	}

	k_sem_give(&dev_data->int_sem);
}

static const struct can_driver_api can_api_funcs = {
	.set_mode = mcp25xxfd_set_mode,
	.set_timing = mcp25xxfd_set_timing,
	.send = mcp25xxfd_send,
	.attach_isr = mcp25xxfd_attach_isr,
	.detach = mcp25xxfd_detach,
#ifndef CONFIG_CAN_AUTO_BUS_OFF_RECOVERY
	.recover = mcp25xxfd_recover,
#endif
	.get_state = mcp25xxfd_get_state,
	.register_state_change_isr = mcp25xxfd_register_state_change_isr,
	.get_core_clock = mcp25xxfd_get_core_clock,
	.timing_min = { .sjw = 1,
			.prop_seg = 0x0,
			.phase_seg1 = 2,
			.phase_seg2 = 1,
			.prescaler = 1 },
	.timing_max = { .sjw = 128,
			.prop_seg = 0x0,
			.phase_seg1 = 256,
			.phase_seg2 = 128,
			.prescaler = 256 },
#if defined(CONFIG_CAN_FD_MODE)
	.timing_min_data = { .sjw = 1,
			     .prop_seg = 0x0,
			     .phase_seg1 = 1,
			     .phase_seg2 = 1,
			     .prescaler = 1 },
	.timing_max_data = { .sjw = 16,
			     .prop_seg = 0x0,
			     .phase_seg1 = 32,
			     .phase_seg2 = 16,
			     .prescaler = 256 }
#endif
};

static int mcp25xxfd_init(const struct device *dev)
{
	const struct mcp25xxfd_config *dev_cfg = DEV_CFG(dev);
	struct mcp25xxfd_data *dev_data = DEV_DATA(dev);
	int ret;
	struct can_timing timing;

#if defined(CONFIG_CAN_FD_MODE)
	struct can_timing timing_data;
#endif

	k_sem_init(&dev_data->int_sem, 0, 1);
	k_sem_init(&dev_data->mode_sem, 0, 1);
	k_sem_init(&dev_data->tx_sem, MCP25XXFD_TXFIFOS,
		   MCP25XXFD_TXFIFOS);
	for (int i = 0; i < MCP25XXFD_TXFIFOS; i++) {
		k_sem_init(&dev_data->mailbox[i].tx_sem, 0, 1);
	}
	k_mutex_init(&dev_data->mutex);

	/* SPI Init */
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
#endif /* DT_INST_SPI_DEV_HAS_CS_GPIOS(0) */

	ret = mcp25xxfd_reset(dev);
	if (ret < 0) {
		LOG_ERR("Failed to reset the device [%d]", ret);
		return -EIO;
	}

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

	gpio_init_callback(&(dev_data->int_gpio_cb),
			   mcp25xxfd_int_gpio_callback, BIT(dev_cfg->int_pin));
	dev_data->int_pin = dev_cfg->int_pin;

	if (gpio_add_callback(dev_data->int_gpio, &(dev_data->int_gpio_cb))) {
		return -EINVAL;
	}

	if (gpio_pin_interrupt_configure(dev_data->int_gpio, dev_cfg->int_pin,
					 GPIO_INT_LEVEL_ACTIVE)) {
		return -EINVAL;
	}

	k_thread_create(&dev_data->int_thread, dev_data->int_thread_stack,
			dev_cfg->int_thread_stack_size,
			(k_thread_entry_t)mcp25xxfd_int_thread, (void *)dev,
			NULL, NULL, K_PRIO_COOP(dev_cfg->int_thread_priority),
			0, K_NO_WAIT);

	timing.sjw = dev_cfg->tq_sjw;
	if (dev_cfg->sample_point && USE_SP_ALGO) {
		ret = can_calc_timing(dev, &timing, dev_cfg->bus_speed,
				      dev_cfg->sample_point);
		if (ret == -EINVAL) {
			LOG_ERR("Can't find timing for given param");
			return -EIO;
		}
		LOG_DBG("Presc: %d, BS1: %d, BS2: %d", timing.prescaler,
			timing.phase_seg1, timing.phase_seg2);
		LOG_DBG("Sample-point err : %d", ret);
	} else {
		timing.prop_seg = dev_cfg->tq_prop;
		timing.phase_seg1 = dev_cfg->tq_bs1;
		timing.phase_seg2 = dev_cfg->tq_bs2;
		ret = can_calc_prescaler(dev, &timing, dev_cfg->bus_speed);
		if (ret) {
			LOG_WRN("Bitrate error: %d", ret);
		}
	}

#if defined(CONFIG_CAN_FD_MODE)
	timing_data.sjw = dev_cfg->tq_sjw_data;
	if (dev_cfg->sample_point && USE_SP_ALGO) {
		ret = can_calc_timing(dev, &timing_data,
				      dev_cfg->bus_speed_data,
				      dev_cfg->sample_point_data);
		if (ret == -EINVAL) {
			LOG_ERR("Can't find timing for given param");
			return -EIO;
		}
		LOG_DBG("Presc: %d, BS1: %d, BS2: %d", timing.prescaler,
			timing.phase_seg1, timing.phase_seg2);
		LOG_DBG("Sample-point err : %d", ret);
	} else {
		timing_data.prop_seg = dev_cfg->tq_prop_data;
		timing_data.phase_seg1 = dev_cfg->tq_bs1_data;
		timing_data.phase_seg2 = dev_cfg->tq_bs2_data;
		ret = can_calc_prescaler(dev, &timing_data,
					 dev_cfg->bus_speed_data);
		if (ret) {
			LOG_WRN("Bitrate error: %d", ret);
		}
	}
#endif

	k_mutex_lock(&dev_data->mutex, K_FOREVER);

	union mcp25xxfd_con con;
	union mcp25xxfd_int regint = { .word = 0x00000000 };
	union mcp25xxfd_iocon iocon;
	union mcp25xxfd_osc osc;
	union mcp25xxfd_fifocon tefcon = { .word = 0x00000400 };
	union mcp25xxfd_fifocon txfifocon = { .word = 0x00600400 };
	union mcp25xxfd_fifocon fifocon = { .word = 0x00600400 };

	ret = mcp25xxfd_readw(dev, MCP25XXFD_REG_CON, &con);
	if (ret < 0) {
		goto done;
	} else if (con.OPMOD != MCP25XXFD_OPMODE_CONFIGURATION) {
		LOG_ERR("Device did not reset into configuration mode [%d]",
			con.OPMOD);
		ret = -EIO;
		goto done;
	}
	con.TXBWS = 0;
	con.ABAT = 0;
	con.REQMOD = MCP25XXFD_OPMODE_CONFIGURATION;
	con.TXQEN = 1;
	con.STEF = 1;
	con.SERR2LOM = 0;
	con.ESIGM = 0;
	con.RTXAT = 0;
	con.BRSDIS = 0;
	con.BUSY = 0;
	con.WFT = MCP25XXFD_WFT_T11FILTER;
	con.WAKFIL = 1;
	con.PXEDIS = 0;
	con.ISOCRCEN = 1;
	con.DNCNT = 0;
	ret = mcp25xxfd_writew(dev, MCP25XXFD_REG_CON, con.byte);
	if (ret < 0) {
		goto done;
	}

	osc.PLLEN = 0;
	osc.OSCDIS = 0;
	osc.LPMEN = 0;
	osc.SCLKDIV = 0;
	osc.CLKODIV = dev_cfg->clko_div;
	ret = mcp25xxfd_writew(dev, MCP25XXFD_REG_OSC, &osc.word);
	if (ret < 0) {
		goto done;
	}

	iocon.TRIS0 = 1;
	iocon.TRIS1 = 1;
	iocon.XSTBYEN = 0;
	iocon.LAT0 = 0;
	iocon.LAT1 = 0;
	iocon.PM0 = 1;
	iocon.PM1 = 1;
	iocon.TXCANOD = 0;
	iocon.SOF = dev_cfg->sof_on_clko ? 1 : 0;
	iocon.INTOD = 0;
	ret = mcp25xxfd_writew(dev, MCP25XXFD_REG_IOCON, &iocon.word);
	if (ret < 0) {
		goto done;
	}

	regint.RXIE = 1;
	regint.MODIE = 1;
	regint.TEFIE = 1;
	regint.CERRIE = 1;
	ret = mcp25xxfd_writew(dev, MCP25XXFD_REG_INT, &regint);
	if (ret < 0) {
		goto done;
	}

	tefcon.FSIZE = MCP25XXFD_TXFIFOS - 1;
	tefcon.FNEIE = 1;
	ret = mcp25xxfd_writew(dev, MCP25XXFD_REG_TEFCON, &tefcon);
	if (ret < 0) {
		goto done;
	}

	txfifocon.PLSIZE = can_bytes_to_dlc(MCP25XXFD_PAYLOAD_SIZE) - 8;
	txfifocon.FSIZE = 0;
	txfifocon.TXPRI = 0;
	txfifocon.TXEN = 1;
	for (int i = 0; i < MCP25XXFD_TXFIFOS; i++) {
		ret = mcp25xxfd_writew(dev, MCP25XXFD_REG_FIFOCON(i), &txfifocon);
		if (ret < 0) {
			goto done;
		}
	}

	fifocon.PLSIZE = can_bytes_to_dlc(MCP25XXFD_PAYLOAD_SIZE) - 8;
	fifocon.FSIZE = MCP25XXFD_RXFIFO_LENGTH - 1;
#if defined(CONFIG_CAN_RX_TIMESTAMP)
	fifocon.TSEN = 1;
#endif
	fifocon.FNEIE = 1;
	ret = mcp25xxfd_writew(dev, MCP25XXFD_REG_FIFOCON(MCP25XXFD_RXFIFO_IDX), &fifocon);
	if (ret < 0) {
		goto done;
	}

	LOG_DBG("%d TX FIFOS: 1 element", MCP25XXFD_TXFIFOS);
	LOG_DBG("1 RX FIFO: %ld elements", MCP25XXFD_RXFIFO_LENGTH);
	LOG_DBG("%ldb of %db RAM Allocated", MCP25XXFD_TEF_SIZE + MCP25XXFD_TXFIFOS_SIZE + MCP25XXFD_RXFIFO_SIZE, MCP25XXFD_RAM_SIZE);

done:
	k_mutex_unlock(&dev_data->mutex);

#if defined(CONFIG_CAN_FD_MODE)
	ret = can_set_timing(dev, &timing, &timing_data);
#else
	ret = can_set_timing(dev, &timing, NULL);
#endif
	if (ret < 0) {
		return ret;
	}

	ret = can_set_mode(dev, CAN_NORMAL_MODE);

	return ret;
}

#if DT_NODE_HAS_STATUS(DT_DRV_INST(0), okay)

static K_KERNEL_STACK_DEFINE(mcp25xxfd_int_stack_0,
			     CONFIG_CAN_MCP25XXFD_INT_THREAD_STACK_SIZE);
static struct mcp25xxfd_data mcp25xxfd_data_0 = {
	.int_thread_stack = mcp25xxfd_int_stack_0
};
static const struct mcp25xxfd_config mcp25xxfd_config_0 = {
	.spi_port = DT_INST_BUS_LABEL(0),
	.spi_freq = DT_INST_PROP(0, spi_max_frequency),
	.spi_slave = DT_INST_REG_ADDR(0),
#if DT_INST_SPI_DEV_HAS_CS_GPIOS(0)
	.spi_cs_pin = DT_INST_SPI_DEV_CS_GPIOS_PIN(0),
	.spi_cs_port = DT_INST_SPI_DEV_CS_GPIOS_LABEL(0),
	.spi_cs_flags = DT_INST_SPI_DEV_CS_GPIOS_FLAGS(0),
#endif /* DT_INST_SPI_DEV_HAS_CS_GPIOS(0) */

	.int_pin = DT_INST_GPIO_PIN(0, int_gpios),
	.int_port = DT_INST_GPIO_LABEL(0, int_gpios),
	.int_thread_stack_size = CONFIG_CAN_MCP25XXFD_INT_THREAD_STACK_SIZE,
	.int_thread_priority = CONFIG_CAN_MCP25XXFD_INT_THREAD_PRIO,

	.sof_on_clko = DT_INST_PROP(0, sof_on_clko),
	.clko_div = DT_ENUM_IDX(DT_DRV_INST(0), clko_div),

	.osc_freq = DT_INST_PROP(0, osc_freq),
	.tq_sjw = DT_INST_PROP(0, sjw),
	.tq_prop = DT_INST_PROP_OR(0, prop_seg, 0),
	.tq_bs1 = DT_INST_PROP_OR(0, phase_seg1, 0),
	.tq_bs2 = DT_INST_PROP_OR(0, phase_seg2, 0),
	.bus_speed = DT_INST_PROP(0, bus_speed),
	.sample_point = DT_INST_PROP_OR(0, sample_point, 0),

#if defined(CONFIG_CAN_FD_MODE)
	.tq_sjw_data = DT_INST_PROP(0, sjw_data),
	.tq_prop_data = DT_INST_PROP_OR(0, prop_seg_data, 0),
	.tq_bs1_data = DT_INST_PROP_OR(0, phase_seg1_data, 0),
	.tq_bs2_data = DT_INST_PROP_OR(0, phase_seg2_data, 0),
	.bus_speed_data = DT_INST_PROP(0, bus_speed_data),
	.sample_point_data = DT_INST_PROP_OR(0, sample_point_data, 0)
#endif
};

DEVICE_DT_INST_DEFINE(0, &mcp25xxfd_init, NULL,
		      &mcp25xxfd_data_0, &mcp25xxfd_config_0, POST_KERNEL,
		      CONFIG_CAN_MCP25XXFD_INIT_PRIORITY, &can_api_funcs);

#endif /* DT_NODE_HAS_STATUS(DT_DRV_INST(0), okay) */
