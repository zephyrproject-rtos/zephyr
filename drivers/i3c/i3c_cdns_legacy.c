/*
 * Copyright (c) 2022 Meta Platforms, Inc. and its affiliates.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Legacy hardware-driving path for the Cadence I3C controller.
 *
 * This file holds the original "build commands into data->xfer.cmds[],
 * call cdns_i3c_start_transfer, complete via the CMDD_EMP ISR" code.
 * Compiled only when CONFIG_I3C_CADENCE_RTIO is NOT enabled. When the
 * native RTIO submit path is selected, every controller-mode hardware
 * driving entry point funnels through it instead and this entire file
 * drops out of the build.
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i3c.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

#include "i3c_cdns.h"

LOG_MODULE_DECLARE(I3C_CADENCE, CONFIG_I3C_CADENCE_LOG_LEVEL);

#ifdef CONFIG_I3C_CONTROLLER
static int cdns_i3c_read_rx_fifo_ddr_xfer(const struct cdns_i3c_config *config, void *buf,
					  uint32_t len, uint32_t ddr_header)
{
	uint16_t *ptr = buf;
	uint32_t val;
	uint32_t preamble;
	uint8_t crc5 = 0x1F;

	/*
	 * TODO: This function does not support threshold interrupts, it is expected that the
	 * whole packet to be within the FIFO and not split across multiple calls to this function.
	 */
	crc5 = i3c_cdns_crc5(crc5, (uint16_t)DDR_DATA(ddr_header));

	for (int i = 0; i < len; i++) {
		if (cdns_i3c_rx_fifo_empty(config)) {
			return -EIO;
		}
		val = sys_read32(config->base + RX_FIFO);
		preamble = (val & DDR_PREAMBLE_MASK);

		if (preamble == DDR_PREAMBLE_DATA_ABORT ||
		    preamble == DDR_PREAMBLE_DATA_ABORT_ALT) {
			*ptr++ = sys_cpu_to_be16((uint16_t)DDR_DATA(val));
			crc5 = i3c_cdns_crc5(crc5, (uint16_t)DDR_DATA(val));
		} else if ((preamble == DDR_PREAMBLE_CMD_CRC) &&
			   ((val & DDR_CRC_TOKEN_MASK) == DDR_CRC_TOKEN)) {
			uint8_t crc = (uint8_t)DDR_CRC(val);

			if (crc5 != crc) {
				LOG_ERR("DDR RX crc error");
				return -EIO;
			}
		}
	}

	return 0;
}

static void cdns_i3c_cancel_transfer(const struct device *dev)
{
	struct cdns_i3c_data *data = dev->data;
	const struct cdns_i3c_config *config = dev->config;
	uint32_t val;

	/* Disable further interrupts */
	sys_write32(MST_INT_CMDD_EMP, config->base + MST_IDR);

	/* Ignore if no pending transfer */
	if (data->xfer.num_cmds == 0) {
		return;
	}

	data->xfer.num_cmds = 0;

	/* Clear main enable bit to disable further transactions */
	sys_write32(~CTRL_DEV_EN & sys_read32(config->base + CTRL), config->base + CTRL);

	/**
	 * Spin waiting for device to go idle. It is unlikely that this will
	 * actually take any time since we only get here if a transaction didn't
	 * complete in a long time.
	 */
	bool idle = false;

	for (uint32_t i = 0; i < I3C_MAX_IDLE_CANCEL_WAIT_RETRIES; i++) {
		val = sys_read32(config->base + MST_STATUS0);
		if (val & MST_STATUS0_IDLE) {
			idle = true;
			break;
		}
		k_msleep(10);
	}
	if (!idle) {
		data->xfer.ret = -ETIMEDOUT;
	}

	/**
	 * Flush all queues.
	 */
	sys_write32(FLUSH_RX_FIFO | FLUSH_TX_FIFO | FLUSH_CMD_FIFO | FLUSH_CMD_RESP,
		    config->base + FLUSH_CTRL);

	/* Re-enable device */
	sys_write32(CTRL_DEV_EN | sys_read32(config->base + CTRL), config->base + CTRL);
}

#if defined(CONFIG_I3C_CALLBACK) || defined(CONFIG_I2C_CALLBACK)
void cdns_i3c_cb_timeout(struct k_timer *timer)
{
	struct cdns_i3c_data *data = CONTAINER_OF(timer, struct cdns_i3c_data, timeout);
	const struct device *dev = data->dev;

	cdns_i3c_cancel_transfer(dev);
	k_sem_give(&data->async_sem);
	pm_device_busy_clear(dev);
	if (data->cb) {
		data->cb(dev, -ETIMEDOUT, data->userdata);
	}
}
#endif

/**
 * @brief Start a I3C/I2C Transfer
 *
 * This is to be called from a I3C/I2C transfer function. This will write
 * all data to tx and cmd fifos
 *
 * @param dev Pointer to controller device driver instance.
 */
static void cdns_i3c_start_transfer(const struct device *dev)
{
	struct cdns_i3c_data *data = dev->data;
	const struct cdns_i3c_config *config = dev->config;
	struct cdns_i3c_xfer *xfer = &data->xfer;

	/* Ensure no pending command response queue threshold interrupt */
	sys_write32(MST_INT_CMDD_EMP, config->base + MST_ICR);

	/* Make sure RX FIFO is empty. */
	while (!cdns_i3c_rx_fifo_empty(config)) {
		(void)sys_read32(config->base + RX_FIFO);
	}
	/* Make sure CMDR FIFO is empty too */
	while (!cdns_i3c_cmd_rsp_fifo_empty(config)) {
		(void)sys_read32(config->base + CMDR);
	}

	/* Write all tx data to fifo */
	for (unsigned int i = 0; i < xfer->num_cmds; i++) {
		if (xfer->cmds[i].hdr == I3C_DATA_RATE_SDR) {
			if (!(xfer->cmds[i].cmd0 & CMD0_FIFO_RNW)) {
				cdns_i3c_write_tx_fifo(config, xfer->cmds[i].buf,
						       xfer->cmds[i].len);
			}
		} else if (xfer->cmds[i].hdr == I3C_DATA_RATE_HDR_DDR) {
			/* DDR Xfer requires sending header block*/
			cdns_i3c_write_tx_fifo(config, &xfer->cmds[i].ddr_header,
					       DDR_CRC_AND_HEADER_SIZE);
			/* If not read operation need to send data + crc of data*/
			if (!(DDR_DATA(xfer->cmds[i].ddr_header) & HDR_CMD_RD)) {
				uint8_t *buf = (uint8_t *)xfer->cmds[i].buf;
				uint32_t ddr_message = 0;
				uint16_t ddr_data_payload = sys_get_be16(&buf[0]);
				/* HDR-DDR Data Words */
				ddr_message = (DDR_PREAMBLE_DATA_ABORT |
					       prepare_ddr_word(ddr_data_payload));
				cdns_i3c_write_tx_fifo(config, &ddr_message,
						       DDR_CRC_AND_HEADER_SIZE);
				for (int j = 2; j < ((xfer->cmds[i].len - 2) * 2); j += 2) {
					ddr_data_payload = sys_get_be16(&buf[j]);
					ddr_message = (DDR_PREAMBLE_DATA_ABORT_ALT |
						       prepare_ddr_word(ddr_data_payload));
					cdns_i3c_write_tx_fifo(config, &ddr_message,
							       DDR_CRC_AND_HEADER_SIZE);
				}
				/* HDR-DDR CRC Word */
				cdns_i3c_write_tx_fifo(config, &xfer->cmds[i].ddr_crc,
						       DDR_CRC_AND_HEADER_SIZE);
			}
		} else {
			xfer->ret = -ENOTSUP;
			return;
		}
	}

	/* Write all data to cmd fifos */
	for (unsigned int i = 0; i < xfer->num_cmds; i++) {
		/* The command ID is just the msg index. */
		xfer->cmds[i].cmd1 |= CMD1_FIFO_CMDID(i);
		sys_write32(xfer->cmds[i].cmd1, config->base + CMD1_FIFO);
		sys_write32(xfer->cmds[i].cmd0, config->base + CMD0_FIFO);

		if (xfer->cmds[i].hdr == I3C_DATA_RATE_HDR_DDR) {
			sys_write32(0x00, config->base + CMD1_FIFO);
			if ((DDR_DATA(xfer->cmds[i].ddr_header) & HDR_CMD_RD)) {
				sys_write32(CMD0_FIFO_IS_DDR | CMD0_FIFO_PL_LEN(1),
					    config->base + CMD0_FIFO);
			} else {
				sys_write32(CMD0_FIFO_IS_DDR | CMD0_FIFO_PL_LEN(xfer->cmds[i].len),
					    config->base + CMD0_FIFO);
			}
		}
	}

	/* kickoff transfer */
	sys_write32(MST_INT_CMDD_EMP, config->base + MST_IER);
	sys_write32(CTRL_MCS | sys_read32(config->base + CTRL), config->base + CTRL);
}

static k_timeout_t cdns_i3c_calc_timeout_i3c(const struct i3c_msg *msgs, uint8_t num_msgs,
					      uint32_t scl_hz)
{
	uint64_t us = 0;
	bool send_broadcast = true;

	for (uint8_t i = 0; i < num_msgs; i++) {
		uint32_t bits;

		if ((msgs[i].flags & I3C_MSG_HDR) && (msgs[i].hdr_mode & I3C_MSG_HDR_DDR)) {
			/*
			 * HDR-DDR preamble: ENTHDR0 broadcast CCC (0x7E addr + cmd code) = 18
			 * SDR bits. DDR payload: (len/2) 16-bit data words + 1 DDR command header
			 * word + 1 DDR CRC word = (len/2 + 2) words total, each taking 8 SCL
			 * cycles (DDR encodes 2 bits per SCL edge, so 16 bits = 8 cycles).
			 */
			bits = 18 + ((msgs[i].len / 2) + 2) * 8;
		} else {
			/* SDR: address frame (9 bits) + data bytes (9 bits each) */
			bits = 9 + (msgs[i].len * 9);
			if (!(msgs[i].flags & I3C_MSG_NBCH) && send_broadcast) {
				bits += 9; /* broadcast header 0x7E */
				send_broadcast = false;
			}
			if (((i + 1) == num_msgs) || (msgs[i].flags & I3C_MSG_STOP)) {
				send_broadcast = true;
			}
		}

		us += DIV_ROUND_UP((uint64_t)bits * USEC_PER_SEC, scl_hz);
	}

	us += CONFIG_I3C_CADENCE_TRANSFER_TIMEOUT_MARGIN_US;

	return K_TICKS(k_us_to_ticks_ceil64(us));
}

static k_timeout_t cdns_i3c_calc_timeout_i2c(const struct i2c_msg *msgs, uint8_t num_msgs,
					      uint32_t scl_hz)
{
	uint64_t us = 0;

	for (uint8_t i = 0; i < num_msgs; i++) {
		uint32_t bits = (msgs[i].flags & I2C_MSG_ADDR_10_BITS) ? 18 : 9;

		bits += msgs[i].len * 9;
		us += DIV_ROUND_UP((uint64_t)bits * USEC_PER_SEC, scl_hz);
	}

	us += CONFIG_I3C_CADENCE_TRANSFER_TIMEOUT_MARGIN_US;

	return K_TICKS(k_us_to_ticks_ceil64(us));
}

static k_timeout_t cdns_i3c_calc_timeout_ccc(const struct i3c_ccc_payload *payload,
					      uint32_t scl_hz)
{
	/*
	 * CCC frame on the wire: broadcast addr (9) + CCC command byte (9) + optional
	 * defining/broadcast data byte(s) (9 each). Per the I3C spec these are
	 * transmitted once per CCC frame regardless of the number of addressed targets.
	 *
	 * For a direct CCC the controller then issues, per target, a repeated start +
	 * target address (9) + per-target data bytes (9 each). The Cadence IP requires
	 * one queued command per target so it knows the direction and length, but it
	 * still emits a single CCC byte / defining byte at the head of the wire frame.
	 */
	uint64_t us;
	uint32_t bits = 18 + (payload->ccc.data_len * 9);

	if (!i3c_ccc_is_payload_broadcast(payload)) {
		for (size_t i = 0; i < payload->targets.num_targets; i++) {
			bits += 9 + (payload->targets.payloads[i].data_len * 9);
		}
	}

	us = DIV_ROUND_UP((uint64_t)bits * USEC_PER_SEC, scl_hz);
	us += CONFIG_I3C_CADENCE_TRANSFER_TIMEOUT_MARGIN_US;

	return K_TICKS(k_us_to_ticks_ceil64(us));
}

int cdns_i3c_do_ccc_do(const struct device *dev, struct i3c_ccc_payload *payload, bool async,
		       i3c_callback_t cb, void *userdata)
{
	const struct cdns_i3c_config *config = dev->config;
	struct cdns_i3c_data *data = dev->data;
	struct cdns_i3c_cmd *cmd;
	int ret = 0;
	uint8_t num_cmds = 0;

	__ASSERT_NO_MSG(payload != NULL);

	/*
	 * Ensure data will fit within FIFOs.
	 *
	 * TODO: This limitation prevents burst transfers greater than the
	 *       FIFO sizes and should be replaced with an implementation that
	 *       utilizes the RX/TX data threshold interrupts.
	 */
	uint32_t num_msgs =
		1 + ((payload->ccc.data_len > 0) ? payload->targets.num_targets
						 : MAX(payload->targets.num_targets - 1, 0));
	if (num_msgs > data->hw_cfg.cmd_mem_depth || num_msgs > data->hw_cfg.cmdr_mem_depth) {
		LOG_ERR("%s: Too many messages", dev->name);
		return -ENOMEM;
	}

	uint32_t rxsize = 0;
	/* defining byte is stored in a separate register for direct CCCs */
	uint32_t txsize =
		i3c_ccc_is_payload_broadcast(payload) ? ROUND_UP(payload->ccc.data_len, 4) : 0;

	for (int i = 0; i < payload->targets.num_targets; i++) {
		if (payload->targets.payloads[i].rnw) {
			rxsize += ROUND_UP(payload->targets.payloads[i].data_len, 4);
		} else {
			txsize += ROUND_UP(payload->targets.payloads[i].data_len, 4);
		}
	}
	if ((rxsize > data->hw_cfg.rx_mem_depth) || (txsize > data->hw_cfg.tx_mem_depth)) {
		LOG_ERR("%s: Total RX and/or TX transfer larger than FIFO", dev->name);
		return -ENOMEM;
	}

	k_mutex_lock(&data->bus_lock, K_FOREVER);
#ifdef CONFIG_I3C_CALLBACK
	k_sem_take(&data->async_sem, K_FOREVER);
	if (async) {
		data->cb = cb;
		data->userdata = userdata;
	} else {
		data->cb = NULL;
		data->userdata = NULL;
	}
#endif
	pm_device_busy_set(dev);

	/* make sure we are currently the active controller */
	if (!(sys_read32(config->base + MST_STATUS0) & MST_STATUS0_MASTER_MODE)) {
		ret = -EACCES;
		goto error;
	}

	/* wait for idle */
	ret = cdns_i3c_wait_for_idle(dev);
	if (ret != 0) {
		goto error;
	}

	/* if this is a direct CCC */
	if (!i3c_ccc_is_payload_broadcast(payload)) {
		/* if the CCC has no data bytes, then the target payload must be in
		 * the same command buffer
		 */
		for (int i = 0; i < payload->targets.num_targets; i++) {
			cmd = &data->xfer.cmds[i];
			num_cmds++;
			cmd->cmd1 = CMD1_FIFO_CCC(payload->ccc.id);
			cmd->cmd0 = CMD0_FIFO_IS_CCC;
			/* if there is a defining byte */
			if (payload->ccc.data_len == 1) {
				/* Only revision 1p7 supports defining byte for direct CCCs */
				if (REV_ID_REV(data->hw_cfg.rev_id) >= REV_ID_VERSION(1, 7)) {
					cmd->cmd0 |= CMD0_FIFO_IS_DB;
					cmd->cmd1 |= CMD1_FIFO_DB(payload->ccc.data[0]);
				} else {
					LOG_ERR("%s: Defining Byte with Direct CCC not supported "
						"with rev %lup%lu",
						dev->name, REV_ID_REV_MAJOR(data->hw_cfg.rev_id),
						REV_ID_REV_MINOR(data->hw_cfg.rev_id));
					ret = -ENOTSUP;
					goto error;
				}
			} else if (payload->ccc.data_len > 1) {
				LOG_ERR("%s: Defining Byte length greater than 1", dev->name);
				ret = -EINVAL;
				goto error;
			}
			/* for a short CCC, i.e. where a direct ccc has multiple targets,
			 * BCH must be 0 for subsequent targets and RSBC must be 1, otherwise
			 * if there is just one target, RSBC must be 0 on the first target
			 */
			if (i == 0) {
				cmd->cmd0 |= CMD0_FIFO_BCH;
			}
			if (i < (payload->targets.num_targets - 1)) {
				cmd->cmd0 |= CMD0_FIFO_RSBC;
			}
			cmd->buf = payload->targets.payloads[i].data;
			cmd->len = payload->targets.payloads[i].data_len;
			cmd->cmd0 |= CMD0_FIFO_DEV_ADDR(payload->targets.payloads[i].addr) |
				     CMD0_FIFO_PL_LEN(payload->targets.payloads[i].data_len);
			if (payload->targets.payloads[i].rnw) {
				cmd->cmd0 |= CMD0_FIFO_RNW;
			}
			cmd->hdr = I3C_DATA_RATE_SDR;
			/*
			 * write the address of num_xfer and err which is to be updated upon
			 * message completion
			 */
			cmd->num_xfer = (uint32_t *)&(payload->targets.payloads[i].num_xfer);
			cmd->sdr_err = &(payload->targets.payloads[i].err);
		}
	} else {
		cmd = &data->xfer.cmds[0];
		num_cmds++;
		cmd->cmd1 = CMD1_FIFO_CCC(payload->ccc.id);
		cmd->cmd0 = CMD0_FIFO_IS_CCC | CMD0_FIFO_BCH;
		cmd->hdr = I3C_DATA_RATE_SDR;

		if (payload->ccc.data_len > 0) {
			/* Write additional data for CCC if needed */
			cmd->buf = payload->ccc.data;
			cmd->len = payload->ccc.data_len;
			cmd->cmd0 |= CMD0_FIFO_PL_LEN(payload->ccc.data_len);
			/* write the address of num_xfer which is to be updated upon message
			 * completion
			 */
			cmd->num_xfer = (uint32_t *)&(payload->ccc.num_xfer);
		} else {
			/* no data to transfer */
			cmd->len = 0;
			cmd->num_xfer = NULL;
		}
		cmd->sdr_err = &(payload->ccc.err);
	}

	data->xfer.ret = -ETIMEDOUT;
	data->xfer.num_cmds = num_cmds;

	k_timeout_t xfer_timeout = cdns_i3c_calc_timeout_ccc(payload,
							      data->common.ctrl_config.scl.i3c);

	cdns_i3c_start_transfer(dev);
	if (!async) {
		if (k_sem_take(&data->xfer.complete, xfer_timeout) != 0) {
			LOG_ERR("%s: transfer timed out", dev->name);
			cdns_i3c_cancel_transfer(dev);
		}
	}
#ifdef CONFIG_I3C_CALLBACK
	else {
		k_timer_start(&data->timeout, xfer_timeout, K_NO_WAIT);
	}
#endif

	if (!async && data->xfer.ret < 0) {
		LOG_ERR("%s: CCC[0x%02x] error (%d)", dev->name, payload->ccc.id, data->xfer.ret);
	}

	if (!async) {
		ret = data->xfer.ret;
	} else {
		ret = 0;
	}
#if defined(CONFIG_I3C_CONTROLLER) && defined(CONFIG_I3C_TARGET)
	/* TODO: decide if this is the right approach or add a new separate API for CH */
	/* Wait for Controller Handoff to finish */
	if (payload->ccc.id == I3C_CCC_GETACCCR) {
		ret = k_sem_take(&data->ch_complete, K_MSEC(1000));
	}
#endif /* CONFIG_I3C_CONTROLLER && CONFIG_I3C_TARGET */
error:
#ifdef CONFIG_I3C_CALLBACK
	if (!async || ret != 0) {
		pm_device_busy_clear(dev);
		k_sem_give(&data->async_sem);
	}
#else
	pm_device_busy_clear(dev);
#endif
	k_mutex_unlock(&data->bus_lock);

	return ret;
}

void cdns_i3c_complete_transfer(const struct device *dev)
{
	struct cdns_i3c_data *data = dev->data;
	const struct cdns_i3c_config *config = dev->config;
	uint32_t cmdr;
	uint32_t id = 0;
	uint32_t xfer = 0;
	int ret = 0;
	struct cdns_i3c_cmd *cmd;
	bool was_full;

	/* Used only to determine in the case of a controller abort */
	was_full = cdns_i3c_rx_fifo_full(config);

	/* Disable further interrupts */
	sys_write32(MST_INT_CMDD_EMP, config->base + MST_IDR);

	/* Ignore if no pending transfer */
	if (data->xfer.num_cmds == 0) {
		return;
	}

	/* Process all results in fifo */
	for (uint32_t status0 = sys_read32(config->base + MST_STATUS0);
	     !(status0 & MST_STATUS0_CMDR_EMP); status0 = sys_read32(config->base + MST_STATUS0)) {
		cmdr = sys_read32(config->base + CMDR);
		id = CMDR_CMDID(cmdr);

		if (id == CMDR_CMDID_HJACK_DISEC || id == CMDR_CMDID_HJACK_ENTDAA ||
		    id >= data->xfer.num_cmds) {
			continue;
		}

		cmd = &data->xfer.cmds[id];

		xfer = MIN(CMDR_XFER_BYTES(cmdr), cmd->len);
		if (cmd->num_xfer != NULL) {
			*cmd->num_xfer = xfer;
		}
		/* Read any rx data into buffer */
		if (cmd->cmd0 & CMD0_FIFO_RNW) {
			ret = cdns_i3c_read_rx_fifo(config, cmd->buf, xfer);
		}

		if ((cmd->hdr == I3C_DATA_RATE_HDR_DDR) &&
		    (DDR_DATA(cmd->ddr_header) & HDR_CMD_RD)) {
			ret = cdns_i3c_read_rx_fifo_ddr_xfer(config, cmd->buf, xfer,
							     cmd->ddr_header);
		}

		/* Record error */
		cmd->error = CMDR_ERROR(cmdr);
	}

	for (int i = 0; i < data->xfer.num_cmds; i++) {
		switch (data->xfer.cmds[i].error) {
		case CMDR_NO_ERROR:
			if (data->xfer.cmds[i].sdr_err) {
				*data->xfer.cmds[i].sdr_err = I3C_ERROR_CE_NONE;
			}
			break;

		case CMDR_MST_ABORT:
			/*
			 * A controller abort is forced if the RX FIFO fills up
			 * There is also the case where the fifo can be full as
			 * the len of the packet is the same length of the fifo
			 * Check that the requested len is greater than the total
			 * transferred to confirm that is not case. Otherwise the
			 * abort was caused by the buffer length being meet and
			 * the target did not give an End of Data (EoD) in the T
			 * bit. Do not treat that condition as an error because
			 * some targets will just auto-increment the read address
			 * way beyond the buffer not giving an EoD.
			 */
			if ((was_full) && (data->xfer.cmds[i].len > *data->xfer.cmds[i].num_xfer)) {
				ret = -ENOSPC;
			} else {
				LOG_DBG("%s: Controller Abort due to buffer length exceeded with "
					"no EoD from target",
					dev->name);
			}
			if (data->xfer.cmds[i].sdr_err) {
				*data->xfer.cmds[i].sdr_err = I3C_ERROR_CE_NONE;
			}
			break;

		case CMDR_M0_ERROR: {
			uint8_t ccc = data->xfer.cmds[i].cmd1 & 0xFF;
			/*
			 * The M0 is an illegally formatted CCC. i.e. the Controller
			 * receives 1 byte instead of 2 with the GETMWL CCC. This can
			 * be problematic for CCCs that can have variable length such
			 * as GETMXDS and GETCAPS. Verify the number of bytes received matches
			 * what's expected from the specification and ignore the error. The IP will
			 * still retramsit the same CCC and there's nothing that can be done to
			 * prevent this. It is still up to the application to read `num_xfer` to
			 * determine the number of bytes returned.
			 */
			if (ccc == I3C_CCC_GETMXDS) {
				/*
				 * Whether GETMXDS format 1 and format 2 can't be known ahead of
				 * time which will be returned.
				 */
				if ((*data->xfer.cmds[i].num_xfer !=
				     SIZEOF_FIELD(union i3c_ccc_getmxds, fmt1)) &&
				    (*data->xfer.cmds[i].num_xfer !=
				     SIZEOF_FIELD(union i3c_ccc_getmxds, fmt2))) {
					ret = -EIO;
				}
			} else if (ccc == I3C_CCC_GETCAPS) {
				/* GETCAPS can only return 1-4 bytes */
				if (*data->xfer.cmds[i].num_xfer > sizeof(union i3c_ccc_getcaps)) {
					ret = -EIO;
				}
			} else {
				if (data->xfer.cmds[i].sdr_err) {
					*data->xfer.cmds[i].sdr_err = I3C_ERROR_CE0;
				}
				ret = -EIO;
			}
			break;
		}

		case CMDR_M1_ERROR:
			if (data->xfer.cmds[i].sdr_err) {
				*data->xfer.cmds[i].sdr_err = I3C_ERROR_CE1;
			}
			ret = -EIO;
			break;
		case CMDR_M2_ERROR:
			if (data->xfer.cmds[i].sdr_err) {
				*data->xfer.cmds[i].sdr_err = I3C_ERROR_CE2;
			}
			ret = -EIO;
			break;

		case CMDR_DDR_PREAMBLE_ERROR:
		case CMDR_DDR_PARITY_ERROR:
		case CMDR_NACK_RESP:
		case CMDR_DDR_DROPPED:
			if (data->xfer.cmds[i].sdr_err) {
				*data->xfer.cmds[i].sdr_err = I3C_ERROR_CE_UNKNOWN;
			}
			ret = -EIO;
			break;

		case CMDR_DDR_RX_FIFO_OVF:
		case CMDR_DDR_TX_FIFO_UNF:
			if (data->xfer.cmds[i].sdr_err) {
				*data->xfer.cmds[i].sdr_err = I3C_ERROR_CE_UNKNOWN;
			}
			ret = -ENOSPC;
			break;

		case CMDR_INVALID_DA:
		default:
			if (data->xfer.cmds[i].sdr_err) {
				*data->xfer.cmds[i].sdr_err = I3C_ERROR_CE_UNKNOWN;
			}
			ret = -EINVAL;
			break;
		}
	}

	data->xfer.ret = ret;

	/* Indicate no transfer is pending */
	data->xfer.num_cmds = 0;
#if defined(CONFIG_I3C_CALLBACK) || defined(CONFIG_I2C_CALLBACK)
	pm_device_busy_clear(dev);
	if (data->cb) {
		k_timer_stop(&data->timeout);
		data->cb(dev, ret, data->userdata);
		k_sem_give(&data->async_sem);
	} else {
		k_sem_give(&data->xfer.complete);
	}
#else
	k_sem_give(&data->xfer.complete);
#endif
}

static int cdns_i3c_i2c_transfer_do(const struct device *dev, struct i3c_i2c_device_desc *i2c_dev,
				    struct i2c_msg *msgs, uint8_t num_msgs, bool async,
				    i2c_callback_t cb, void *userdata)
{
	const struct cdns_i3c_config *config = dev->config;
	struct cdns_i3c_data *data = dev->data;
	uint32_t txsize = 0;
	uint32_t rxsize = 0;
	int ret;
	k_timeout_t xfer_timeout;

	__ASSERT_NO_MSG(num_msgs > 0);

	if (num_msgs > data->hw_cfg.cmd_mem_depth || num_msgs > data->hw_cfg.cmdr_mem_depth) {
		LOG_ERR("%s: Too many messages", dev->name);
		return -ENOMEM;
	}

	/*
	 * Ensure data will fit within FIFOs
	 */
	for (unsigned int i = 0; i < num_msgs; i++) {
		if ((msgs[i].flags & I2C_MSG_RW_MASK) == I2C_MSG_READ) {
			rxsize += ROUND_UP(msgs[i].len, 4);
		} else {
			txsize += ROUND_UP(msgs[i].len, 4);
		}
	}
	if ((rxsize > data->hw_cfg.rx_mem_depth) || (txsize > data->hw_cfg.tx_mem_depth)) {
		LOG_ERR("%s: Total RX and/or TX transfer larger than FIFO", dev->name);
		return -ENOMEM;
	}

	k_mutex_lock(&data->bus_lock, K_FOREVER);
#ifdef CONFIG_I2C_CALLBACK
	k_sem_take(&data->async_sem, K_FOREVER);
	if (async) {
		data->cb = cb;
		data->userdata = userdata;
	} else {
		data->cb = NULL;
		data->userdata = NULL;
	}
#endif
	pm_device_busy_set(dev);

	/* make sure we are currently the active controller */
	if (!(sys_read32(config->base + MST_STATUS0) & MST_STATUS0_MASTER_MODE)) {
		ret = -EACCES;
		goto error;
	}

	/* wait for idle */
	ret = cdns_i3c_wait_for_idle(dev);
	if (ret != 0) {
		goto error;
	}

	for (unsigned int i = 0; i < num_msgs; i++) {
		struct cdns_i3c_cmd *cmd = &data->xfer.cmds[i];

		cmd->len = msgs[i].len;
		cmd->buf = msgs[i].buf;
		/* not an i3c transfer, but must be set to sdr */
		cmd->hdr = I3C_DATA_RATE_SDR;

		cmd->cmd0 = CMD0_FIFO_PRIV_XMIT_MODE(XMIT_BURST_WITHOUT_SUBADDR);
		cmd->cmd0 |= CMD0_FIFO_DEV_ADDR(i2c_dev->addr);
		cmd->cmd0 |= CMD0_FIFO_PL_LEN(msgs[i].len);

		/* Send repeated start on all transfers except the last or those marked STOP. */
		if ((i < (num_msgs - 1)) && ((msgs[i].flags & I2C_MSG_STOP) == 0)) {
			cmd->cmd0 |= CMD0_FIFO_RSBC;
		}

		if (msgs[i].flags & I2C_MSG_ADDR_10_BITS) {
			cmd->cmd0 |= CMD0_FIFO_IS_10B;
		}

		if ((msgs[i].flags & I2C_MSG_RW_MASK) == I2C_MSG_READ) {
			cmd->cmd0 |= CMD0_FIFO_RNW;
		}

		/* i2c transfers are a don't care for num_xfer and sdr error */
		cmd->num_xfer = NULL;
		cmd->sdr_err = NULL;
	}

	data->xfer.ret = -ETIMEDOUT;
	data->xfer.num_cmds = num_msgs;

	xfer_timeout = cdns_i3c_calc_timeout_i2c(msgs, num_msgs,
						  data->common.ctrl_config.scl.i2c);

	cdns_i3c_start_transfer(dev);
	if (!async) {
		if (k_sem_take(&data->xfer.complete, xfer_timeout) != 0) {
			cdns_i3c_cancel_transfer(dev);
		}
		ret = data->xfer.ret;
	}
#ifdef CONFIG_I2C_CALLBACK
	else {
		k_timer_start(&data->timeout, xfer_timeout, K_NO_WAIT);
		ret = 0;
	}
#endif
error:
#ifdef CONFIG_I2C_CALLBACK
	if (!async || ret != 0) {
		pm_device_busy_clear(dev);
		k_sem_give(&data->async_sem);
	}
#else
	pm_device_busy_clear(dev);
#endif
	k_mutex_unlock(&data->bus_lock);

	return ret;
}

int cdns_i3c_i2c_transfer(const struct device *dev, struct i3c_i2c_device_desc *i2c_dev,
			  struct i2c_msg *msgs, uint8_t num_msgs)
{
	return cdns_i3c_i2c_transfer_do(dev, i2c_dev, msgs, num_msgs, false, NULL, NULL);
}

#ifdef CONFIG_I2C_CALLBACK
int cdns_i3c_i2c_transfer_cb(const struct device *dev, struct i3c_i2c_device_desc *i2c_dev,
			     struct i2c_msg *msgs, uint8_t num_msgs, i2c_callback_t cb,
			     void *userdata)
{
	return cdns_i3c_i2c_transfer_do(dev, i2c_dev, msgs, num_msgs, true, cb, userdata);
}
#endif

int cdns_i3c_transfer_do(const struct device *dev, struct i3c_device_desc *target,
			 struct i3c_msg *msgs, uint8_t num_msgs, bool async, i3c_callback_t cb,
			 void *userdata)
{
	const struct cdns_i3c_config *config = dev->config;
	struct cdns_i3c_data *data = dev->data;
	int txsize = 0;
	int rxsize = 0;
	int ret;
	k_timeout_t xfer_timeout;

	__ASSERT_NO_MSG(num_msgs > 0);

	if (num_msgs > data->hw_cfg.cmd_mem_depth || num_msgs > data->hw_cfg.cmdr_mem_depth) {
		LOG_ERR("%s: Too many messages", dev->name);
		return -ENOMEM;
	}

	/*
	 * Ensure data will fit within FIFOs.
	 *
	 * TODO: This limitation prevents burst transfers greater than the
	 *       FIFO sizes and should be replaced with an implementation that
	 *       utilizes the RX/TX data interrupts.
	 */
	for (int i = 0; i < num_msgs; i++) {
		if ((msgs[i].flags & I3C_MSG_RW_MASK) == I3C_MSG_READ) {
			rxsize += ROUND_UP(msgs[i].len, 4);
		} else {
			txsize += ROUND_UP(msgs[i].len, 4);
		}
	}
	if ((rxsize > data->hw_cfg.rx_mem_depth) || (txsize > data->hw_cfg.tx_mem_depth)) {
		LOG_ERR("%s: Total RX and/or TX transfer larger than FIFO", dev->name);
		return -ENOMEM;
	}

	k_mutex_lock(&data->bus_lock, K_FOREVER);
#ifdef CONFIG_I3C_CALLBACK
	k_sem_take(&data->async_sem, K_FOREVER);
	if (async) {
		data->cb = cb;
		data->userdata = userdata;
	} else {
		data->cb = NULL;
		data->userdata = NULL;
	}
#endif
	pm_device_busy_set(dev);

	/* make sure we are currently the active controller */
	if (!(sys_read32(config->base + MST_STATUS0) & MST_STATUS0_MASTER_MODE)) {
		ret = -EACCES;
		goto error;
	}

	/* wait for idle */
	ret = cdns_i3c_wait_for_idle(dev);
	if (ret != 0) {
		goto error;
	}

	/*
	 * Prepare transfer commands. Currently there is only a single transfer
	 * in-flight but it would be possible to keep a queue of transfers. If so,
	 * this preparation could be completed outside of the bus lock allowing
	 * greater parallelism.
	 */
	bool send_broadcast = true;

	for (int i = 0; i < num_msgs; i++) {
		struct cdns_i3c_cmd *cmd = &data->xfer.cmds[i];
		uint32_t pl = msgs[i].len;
		/* check hdr mode */
		if ((!(msgs[i].flags & I3C_MSG_HDR)) ||
		    ((msgs[i].flags & I3C_MSG_HDR) && (msgs[i].hdr_mode == 0))) {
			/* HDR message flag is not set or if hdr flag is set but no hdr mode is set
			 */
			cmd->len = pl;
			cmd->buf = msgs[i].buf;

			cmd->cmd0 = CMD0_FIFO_PRIV_XMIT_MODE(XMIT_BURST_WITHOUT_SUBADDR);
			cmd->cmd0 |= CMD0_FIFO_DEV_ADDR(target->dynamic_addr);
			if ((msgs[i].flags & I3C_MSG_RW_MASK) == I3C_MSG_READ) {
				cmd->cmd0 |= CMD0_FIFO_RNW;
			}
			/*
			 * For I3C_XMIT_MODE_NO_ADDR reads in SDN mode,
			 * CMD0_FIFO_PL_LEN specifies the abort limit not bytes to read
			 */
			cmd->cmd0 |= CMD0_FIFO_PL_LEN(pl);

			/* Send broadcast header on first transfer or after a STOP. */
			if (!(msgs[i].flags & I3C_MSG_NBCH) && (send_broadcast)) {
				cmd->cmd0 |= CMD0_FIFO_BCH;
				send_broadcast = false;
			}

			/*
			 * Send repeated start on all transfers except the last or those marked
			 * STOP.
			 */
			if ((i < (num_msgs - 1)) && ((msgs[i].flags & I3C_MSG_STOP) == 0)) {
				cmd->cmd0 |= CMD0_FIFO_RSBC;
			} else {
				send_broadcast = true;
			}

			/*
			 * write the address of num_xfer which is to be updated upon message
			 * completion
			 */
			cmd->num_xfer = &(msgs[i].num_xfer);
			cmd->sdr_err = &(msgs[i].err);
			cmd->hdr = I3C_DATA_RATE_SDR;
		} else if ((data->common.ctrl_config.supported_hdr & I3C_MSG_HDR_DDR) &&
			   (msgs[i].hdr_mode == I3C_MSG_HDR_DDR) && (msgs[i].flags & I3C_MSG_HDR)) {
			uint16_t ddr_header_payload;

			/* DDR sends data out in 16b, so len must be a multiple of 2 */
			if (!((pl % 2) == 0)) {
				ret = -EINVAL;
				goto error;
			}
			/* HDR message flag is set and hdr mode is DDR */
			cmd->buf = msgs[i].buf;
			if ((msgs[i].flags & I3C_MSG_RW_MASK) == I3C_MSG_READ) {
				/* HDR-DDR Read */
				ddr_header_payload = HDR_CMD_RD |
						     HDR_CMD_CODE(msgs[i].hdr_cmd_code) |
						     (target->dynamic_addr << 1);
				/* Parity Adjustment Bit for Reads */
				ddr_header_payload =
					prepare_ddr_cmd_parity_adjustment_bit(ddr_header_payload);
				/* HDR-DDR Command Word */
				cmd->ddr_header =
					DDR_PREAMBLE_CMD_CRC | prepare_ddr_word(ddr_header_payload);
			} else {
				uint8_t crc5 = 0x1F;
				/* HDR-DDR Write */
				ddr_header_payload = HDR_CMD_CODE(msgs[i].hdr_cmd_code) |
						     (target->dynamic_addr << 1);
				/* HDR-DDR Command Word */
				cmd->ddr_header =
					DDR_PREAMBLE_CMD_CRC | prepare_ddr_word(ddr_header_payload);
				/* calculate crc5 */
				crc5 = i3c_cdns_crc5(crc5, ddr_header_payload);
				for (int j = 0; j < pl; j += 2) {
					crc5 = i3c_cdns_crc5(
						crc5,
						sys_get_be16((void *)((uintptr_t)cmd->buf + j)));
				}
				cmd->ddr_crc = DDR_PREAMBLE_CMD_CRC | DDR_CRC_TOKEN | (crc5 << 9) |
					       DDR_CRC_WR_SETUP;
			}
			/* Length of DDR Transfer is length of payload (in 16b) + header and CRC
			 * blocks
			 */
			cmd->len = ((pl / 2) + 2);

			/* prep command FIFO for ENTHDR0 */
			cmd->cmd0 = CMD0_FIFO_IS_CCC;
			cmd->cmd1 = I3C_CCC_ENTHDR0;
			/* write the address of num_xfer which is to be updated upon message
			 * completion
			 */
			cmd->num_xfer = &(msgs[i].num_xfer);
			cmd->sdr_err = &(msgs[i].err);
			cmd->hdr = I3C_DATA_RATE_HDR_DDR;
		} else {
			LOG_ERR("%s: Unsupported HDR Mode %d", dev->name, msgs[i].hdr_mode);
			ret = -ENOTSUP;
			goto error;
		}
	}

	data->xfer.ret = -ETIMEDOUT;
	data->xfer.num_cmds = num_msgs;

	xfer_timeout = cdns_i3c_calc_timeout_i3c(msgs, num_msgs,
						  data->common.ctrl_config.scl.i3c);

	cdns_i3c_start_transfer(dev);
	if (!async) {
		if (k_sem_take(&data->xfer.complete, xfer_timeout) != 0) {
			LOG_ERR("%s: transfer timed out", dev->name);
			cdns_i3c_cancel_transfer(dev);
		}
	}
#ifdef CONFIG_I3C_CALLBACK
	else {
		k_timer_start(&data->timeout, xfer_timeout, K_NO_WAIT);
	}
#endif
	if (!async) {
		ret = data->xfer.ret;
	} else {
		ret = 0;
	}
error:
#ifdef CONFIG_I3C_CALLBACK
	if (!async || ret != 0) {
		pm_device_busy_clear(dev);
		k_sem_give(&data->async_sem);
	}
#else
	pm_device_busy_clear(dev);
#endif
	k_mutex_unlock(&data->bus_lock);

	return ret;
}

struct i3c_i2c_device_desc *cdns_i3c_i2c_device_find(const struct device *dev, uint16_t addr)
{
	return i3c_dev_list_i2c_addr_find(dev, addr);
}
#endif /* CONFIG_I3C_CONTROLLER */
