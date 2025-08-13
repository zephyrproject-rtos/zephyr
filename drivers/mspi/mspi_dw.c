/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT snps_designware_ssi

#include <zephyr/drivers/mspi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include "mspi_dw.h"

LOG_MODULE_REGISTER(mspi_dw, CONFIG_MSPI_LOG_LEVEL);

#if defined(CONFIG_MSPI_XIP)
struct xip_params {
	uint32_t read_cmd;
	uint32_t write_cmd;
	uint16_t rx_dummy;
	uint16_t tx_dummy;
	uint8_t cmd_length;
	uint8_t addr_length;
	enum mspi_io_mode io_mode;
};

struct xip_ctrl {
	uint32_t read;
	uint32_t write;
};
#endif

struct mspi_dw_data {
	const struct mspi_dev_id *dev_id;
	uint32_t packets_done;
	uint8_t *buf_pos;
	const uint8_t *buf_end;

	uint32_t ctrlr0;
	uint32_t spi_ctrlr0;
	uint32_t baudr;
	uint32_t dma_cr;

#if defined(CONFIG_MSPI_XIP)
	uint32_t xip_freq;
	struct xip_params xip_params_stored;
	struct xip_params xip_params_active;
	uint16_t xip_enabled;
	enum mspi_cpp_mode xip_cpp;
#endif

	uint16_t dummy_bytes;
	uint8_t bytes_to_discard;
	uint8_t bytes_per_frame_exp;
	bool standard_spi;
	bool suspended;

	mspi_callback_handler_t         cbs[MSPI_BUS_EVENT_MAX];
	struct mspi_callback_context    *cb_ctxs[MSPI_BUS_EVENT_MAX];

	struct k_sem finished;
	/* For synchronization of API calls made from different contexts. */
	struct k_sem ctx_lock;
	/* For locking of controller configuration. */
	struct k_sem cfg_lock;
	struct mspi_xfer xfer;
	void * dma_transfer_list;
	/* Flag to track if async transfer is in progress */
	/* TODO: Change to atomic variable when concurrent transactions are supported*/
	volatile bool async_in_progress;
};

/* Register access helpers. */
#define DEFINE_MM_REG_RD_WR(reg, off) \
	DEFINE_MM_REG_RD(reg, off) \
	DEFINE_MM_REG_WR(reg, off)

DEFINE_MM_REG_WR(ctrlr0,	0x00)
DEFINE_MM_REG_WR(ctrlr1,	0x04)
DEFINE_MM_REG_WR(ssienr,	0x08)
DEFINE_MM_REG_WR(ser,		0x10)
DEFINE_MM_REG_WR(baudr,		0x14)
DEFINE_MM_REG_RD_WR(txftlr,	0x18)
DEFINE_MM_REG_RD_WR(rxftlr,	0x1c)
DEFINE_MM_REG_RD(txflr,		0x20)
DEFINE_MM_REG_RD(rxflr,		0x24)
DEFINE_MM_REG_RD(sr,		0x28)
DEFINE_MM_REG_WR(imr,		0x2c)
DEFINE_MM_REG_RD(isr,		0x30)
DEFINE_MM_REG_RD(risr,		0x34)
DEFINE_MM_REG_RD_WR(dr,		0x60)
DEFINE_MM_REG_WR(spi_ctrlr0,	0xf4)
DEFINE_MM_REG_WR(dmacr,		0x4C)
DEFINE_MM_REG_WR(dmatdlr,	0x50)
DEFINE_MM_REG_WR(dmardlr,	0x54)

#if defined(CONFIG_MSPI_XIP)
DEFINE_MM_REG_WR(xip_incr_inst,		0x100)
DEFINE_MM_REG_WR(xip_wrap_inst,		0x104)
DEFINE_MM_REG_WR(xip_ctrl,		0x108)
DEFINE_MM_REG_WR(xip_write_incr_inst,	0x140)
DEFINE_MM_REG_WR(xip_write_wrap_inst,	0x144)
DEFINE_MM_REG_WR(xip_write_ctrl,	0x148)
#endif

#include "mspi_dw_vendor_specific.h"

static int start_next_packet(const struct device *dev, k_timeout_t timeout);

static void tx_data(const struct device *dev,
		    const struct mspi_xfer_packet *packet)
{
	struct mspi_dw_data *dev_data = dev->data;
	const struct mspi_dw_config *dev_config = dev->config;
	const uint8_t *buf_pos = dev_data->buf_pos;
	const uint8_t *buf_end = dev_data->buf_end;
	/* When the function is called, it is known that at least one item
	 * can be written to the FIFO. The loop below writes to the FIFO
	 * the number of items that is known to fit and then updates that
	 * number basing on the actual FIFO level (because some data may get
	 * sent while the FIFO is written; especially for high frequencies
	 * this may often occur) and continues until the FIFO is filled up
	 * or the buffer end is reached.
	 */
	uint32_t room = 1;
	uint8_t bytes_per_frame_exp = dev_data->bytes_per_frame_exp;
	uint8_t tx_fifo_depth = dev_config->tx_fifo_depth_minus_1 + 1;
	uint32_t data;

	do {
		if (bytes_per_frame_exp == 2) {
			data = sys_get_be32(buf_pos);
			buf_pos += 4;
		} else if (bytes_per_frame_exp == 1) {
			data = sys_get_be16(buf_pos);
			buf_pos += 2;
		} else {
			data = *buf_pos;
			buf_pos += 1;
		}
		write_dr(dev, data);

		if (buf_pos >= buf_end) {
			/* Set the threshold to 0 to get the next interrupt
			 * when the FIFO is completely emptied. This also sets
			 * the TX start level to 0, so if the transmission was
			 * not started so far because the FIFO was not filled
			 * up completely (the start level was set to maximum
			 * in start_next_packet()), it will be started now.
			 */
			write_txftlr(dev, 0);
			break;
		}

		if (--room == 0) {
			room = tx_fifo_depth
			     - FIELD_GET(TXFLR_TXTFL_MASK, read_txflr(dev));
		}
	} while (room);

	dev_data->buf_pos = (uint8_t *)buf_pos;
}

static bool tx_dummy_bytes(const struct device *dev)
{
	struct mspi_dw_data *dev_data = dev->data;
	const struct mspi_dw_config *dev_config = dev->config;
	uint8_t fifo_room = dev_config->max_queued_dummy_bytes
			  - FIELD_GET(TXFLR_TXTFL_MASK, read_txflr(dev));
	uint8_t rx_fifo_items = FIELD_GET(RXFLR_RXTFL_MASK, read_rxflr(dev));
	uint16_t dummy_bytes = dev_data->dummy_bytes;
	const uint8_t dummy_val = 0;

	/* Subtract the number of items that are already stored in the RX
	 * FIFO to avoid overflowing it; `max_queued_dummy_bytes` accounts
	 * that one byte that can be partially received, thus not included
	 * in the value from the RXFLR register.
	 */
	if (fifo_room <= rx_fifo_items) {
		return false;
	}
	fifo_room -= rx_fifo_items;

	if (dummy_bytes > fifo_room) {
		dev_data->dummy_bytes = dummy_bytes - fifo_room;

		do {
			write_dr(dev, dummy_val);
		} while (--fifo_room);

		return false;
	}

	do {
		write_dr(dev, dummy_val);
	} while (--dummy_bytes);

	/* Set the TX start level to 0, so that the transmission will be
	 * started now if it hasn't been yet. The threshold value is also
	 * set to 0 here, but it doesn't really matter, as the interrupt
	 * will be anyway disabled.
	 */
	write_txftlr(dev, 0);

	return true;
}

static bool read_rx_fifo(const struct device *dev,
			 const struct mspi_xfer_packet *packet)
{
	struct mspi_dw_data *dev_data = dev->data;
	const struct mspi_dw_config *dev_config = dev->config;
	uint8_t bytes_to_discard = dev_data->bytes_to_discard;
	uint8_t *buf_pos = dev_data->buf_pos;
	const uint8_t *buf_end = &packet->data_buf[packet->num_bytes];
	uint8_t bytes_per_frame_exp = dev_data->bytes_per_frame_exp;
	/* See `room` in tx_data(). */
	uint32_t in_fifo = 1;
	uint32_t remaining_frames;

	do {
		uint32_t data = read_dr(dev);

		if (bytes_to_discard) {
			--bytes_to_discard;
		} else {
			if (bytes_per_frame_exp == 2) {
				sys_put_be32(data, buf_pos);
				buf_pos += 4;
			} else if (bytes_per_frame_exp == 1) {
				sys_put_be16(data, buf_pos);
				buf_pos += 2;
			} else {
				*buf_pos = (uint8_t)data;
				buf_pos += 1;
			}

			if (buf_pos >= buf_end) {
				dev_data->buf_pos = buf_pos;
				return true;
			}
		}

		if (--in_fifo == 0) {
			in_fifo = FIELD_GET(RXFLR_RXTFL_MASK, read_rxflr(dev));
		}
	} while (in_fifo);

	remaining_frames = (bytes_to_discard + buf_end - buf_pos)
			   >> bytes_per_frame_exp;
	if (remaining_frames - 1 < dev_config->rx_fifo_threshold) {
		write_rxftlr(dev, remaining_frames - 1);
	}

	dev_data->bytes_to_discard = bytes_to_discard;
	dev_data->buf_pos = buf_pos;
	return false;
}

static void mspi_dw_isr(const struct device *dev)
{
	struct mspi_dw_data *dev_data = dev->data;
	struct mspi_xfer xfer = dev_data->xfer;
	const struct mspi_xfer_packet *packet =
		&dev_data->xfer.packets[dev_data->packets_done];
	bool finished = false;
	int rc;
	bool dma_irq = vendor_specific_read_dma_irq(dev, dev->config);

	uint32_t int_status = read_isr(dev);

	if(xfer.xfer_mode == MSPI_DMA && dma_irq) {
		/* No need to read FIFO by CPU */
		finished = true;
	}
	else {

		if (packet->dir == MSPI_TX) {
			if (dev_data->buf_pos < dev_data->buf_end) {
				if (int_status & ISR_TXEIS_BIT) {
					tx_data(dev, packet);
				}
			} else {
				/* It may happen that at this point the controller is
				* still shifting out the last frame (the last interrupt
				* occurs when the TX FIFO is empty). Wait if it signals
				* that it is busy.
				*/
				while (read_sr(dev) & SR_BUSY_BIT) {
				}

				finished = true;
			}
		} else {
			do {
				if (int_status & ISR_RXFIS_BIT) {
					if (read_rx_fifo(dev, packet)) {
						finished = true;
						break;
					}

					if (read_risr(dev) & RISR_RXOIR_BIT) {
						finished = true;
						break;
					}

					int_status = read_isr(dev);
				}

			if (int_status & ISR_TXEIS_BIT) {
				if (tx_dummy_bytes(dev)) {
					/* All the required dummy bytes were
					 * written to the FIFO; disable the TXE
					 * interrupt, as it's no longer needed.
					 */
					write_imr(dev, IMR_RXFIM_BIT);
				}

				int_status = read_isr(dev);
			}
		} while (int_status != 0);
	}

	if (finished) {
		write_imr(dev, 0);
		// For async, call the registered callback with event context
		if (xfer.async){
			write_ssienr(dev, 0);

			if(dev_data->cb_ctxs[MSPI_BUS_XFER_COMPLETE]){
				struct mspi_callback_context *cb_ctx = dev_data->cb_ctxs[MSPI_BUS_XFER_COMPLETE];
				if (cb_ctx) {
					cb_ctx->mspi_evt.evt_type = MSPI_BUS_XFER_COMPLETE;
					cb_ctx->mspi_evt.evt_data.controller = dev;
					cb_ctx->mspi_evt.evt_data.dev_id = dev_data->dev_id;
					cb_ctx->mspi_evt.evt_data.packet = packet;
					cb_ctx->mspi_evt.evt_data.packet_idx = dev_data->packets_done;
				}
				LOG_DBG("Calling user set function");
				dev_data->cbs[MSPI_BUS_XFER_COMPLETE](cb_ctx);
				rc = pm_device_runtime_put(dev);
				if (rc < 0) {
					LOG_ERR("pm_device_runtime_put() failed: %d", rc);
				}
			}
			k_sem_give(&dev_data->finished);

			/* In async mode, the next packet is started from the isr */
			/* TODO: Should put this into a work queue instead */
			if(++dev_data->packets_done != xfer.num_packet) {
				LOG_DBG("Another packet to be sent");
				vendor_specific_irq_clear(dev, dev->config);
				rc = start_next_packet(dev, K_MSEC(dev_data->xfer.timeout));
				if (rc < 0) {
					LOG_ERR("start_next_packet() failed: %d", rc);
					dev_data->async_in_progress = false;
					dev_data->packets_done = 0;
				}
			} else {
				/* All packets completed - clear busy flag and reset packets done */
				dev_data->async_in_progress = false;
				dev_data->packets_done = 0;
			}
		}
		else {
			/* Only give semaphore for sync mode or when all packets are done in async mode */
			k_sem_give(&dev_data->finished);
		}


		/* Free transfer list saved to heap */
		if (dev_data->xfer.xfer_mode == MSPI_DMA && dev_data->dma_transfer_list){
			vendor_specific_free_dma_transfer_list(dev, dev_data);
			dev_data->dma_transfer_list = NULL;
		}

	}

	vendor_specific_irq_clear(dev, dev->config);
}

static int api_config(const struct mspi_dt_spec *spec)
{
	ARG_UNUSED(spec);

	return -ENOTSUP;
}

static bool apply_io_mode(struct mspi_dw_data *dev_data,
			  enum mspi_io_mode io_mode)
{
	dev_data->ctrlr0 &= ~CTRLR0_SPI_FRF_MASK;
	dev_data->spi_ctrlr0 &= ~SPI_CTRLR0_TRANS_TYPE_MASK;

	/* Frame format used for transferring data. */

	if (io_mode == MSPI_IO_MODE_SINGLE) {
		dev_data->ctrlr0 |= FIELD_PREP(CTRLR0_SPI_FRF_MASK,
					       CTRLR0_SPI_FRF_STANDARD);
		dev_data->standard_spi = true;
		return true;
	}

	dev_data->standard_spi = false;

	switch (io_mode) {
	case MSPI_IO_MODE_DUAL:
	case MSPI_IO_MODE_DUAL_1_1_2:
	case MSPI_IO_MODE_DUAL_1_2_2:
		dev_data->ctrlr0 |= FIELD_PREP(CTRLR0_SPI_FRF_MASK,
					       CTRLR0_SPI_FRF_DUAL);
		break;
	case MSPI_IO_MODE_QUAD:
	case MSPI_IO_MODE_QUAD_1_1_4:
	case MSPI_IO_MODE_QUAD_1_4_4:
		dev_data->ctrlr0 |= FIELD_PREP(CTRLR0_SPI_FRF_MASK,
					       CTRLR0_SPI_FRF_QUAD);
		break;
	case MSPI_IO_MODE_OCTAL:
	case MSPI_IO_MODE_OCTAL_1_1_8:
	case MSPI_IO_MODE_OCTAL_1_8_8:
		dev_data->ctrlr0 |= FIELD_PREP(CTRLR0_SPI_FRF_MASK,
					       CTRLR0_SPI_FRF_OCTAL);
		break;
	default:
		LOG_ERR("IO mode %d not supported", io_mode);
		return false;
	}

	/* Transfer format used for Address and Instruction: */

	switch (io_mode) {
	case MSPI_IO_MODE_DUAL_1_1_2:
	case MSPI_IO_MODE_QUAD_1_1_4:
	case MSPI_IO_MODE_OCTAL_1_1_8:
		/* - both sent in Standard SPI mode */
		dev_data->spi_ctrlr0 |= FIELD_PREP(SPI_CTRLR0_TRANS_TYPE_MASK,
						   SPI_CTRLR0_TRANS_TYPE_TT0);
		break;
	case MSPI_IO_MODE_DUAL_1_2_2:
	case MSPI_IO_MODE_QUAD_1_4_4:
	case MSPI_IO_MODE_OCTAL_1_8_8:
		/* - Instruction sent in Standard SPI mode,
		 *   Address sent the same way as data
		 */
		dev_data->spi_ctrlr0 |= FIELD_PREP(SPI_CTRLR0_TRANS_TYPE_MASK,
						   SPI_CTRLR0_TRANS_TYPE_TT1);
		break;
	default:
		/* - both sent the same way as data. */
		dev_data->spi_ctrlr0 |= FIELD_PREP(SPI_CTRLR0_TRANS_TYPE_MASK,
						   SPI_CTRLR0_TRANS_TYPE_TT2);
		break;
	}

	return true;
}

static bool apply_cmd_length(struct mspi_dw_data *dev_data, uint32_t cmd_length)
{
	switch (cmd_length) {
	case 0:
		dev_data->spi_ctrlr0 |= FIELD_PREP(SPI_CTRLR0_INST_L_MASK,
						   SPI_CTRLR0_INST_L0);
		break;
	case 1:
		dev_data->spi_ctrlr0 |= FIELD_PREP(SPI_CTRLR0_INST_L_MASK,
						   SPI_CTRLR0_INST_L8);
		break;
	case 2:
		dev_data->spi_ctrlr0 |= FIELD_PREP(SPI_CTRLR0_INST_L_MASK,
						   SPI_CTRLR0_INST_L16);
		break;
	default:
		LOG_ERR("Command length %u not supported", cmd_length);
		return false;
	}

	return true;
}

static bool apply_addr_length(struct mspi_dw_data *dev_data,
			      uint32_t addr_length)
{
	if (addr_length > 4) {
		LOG_ERR("Address length %u not supported", addr_length);
		return false;
	}

	dev_data->spi_ctrlr0 |= FIELD_PREP(SPI_CTRLR0_ADDR_L_MASK,
					   addr_length * 2);

	return true;
}

#if defined(CONFIG_MSPI_XIP)
static bool apply_xip_io_mode(const struct mspi_dw_data *dev_data,
			      struct xip_ctrl *ctrl)
{
	enum mspi_io_mode io_mode = dev_data->xip_params_active.io_mode;

	/* Frame format used for transferring data. */

	if (io_mode == MSPI_IO_MODE_SINGLE) {
		LOG_ERR("XIP not available in single line mode");
		return false;
	}

	switch (io_mode) {
	case MSPI_IO_MODE_DUAL:
	case MSPI_IO_MODE_DUAL_1_1_2:
	case MSPI_IO_MODE_DUAL_1_2_2:
		ctrl->read |= FIELD_PREP(XIP_CTRL_FRF_MASK,
					 XIP_CTRL_FRF_DUAL);
		ctrl->write |= FIELD_PREP(XIP_WRITE_CTRL_FRF_MASK,
					  XIP_WRITE_CTRL_FRF_DUAL);
		break;
	case MSPI_IO_MODE_QUAD:
	case MSPI_IO_MODE_QUAD_1_1_4:
	case MSPI_IO_MODE_QUAD_1_4_4:
		ctrl->read |= FIELD_PREP(XIP_CTRL_FRF_MASK,
					 XIP_CTRL_FRF_QUAD);
		ctrl->write |= FIELD_PREP(XIP_WRITE_CTRL_FRF_MASK,
					  XIP_WRITE_CTRL_FRF_QUAD);
		break;
	case MSPI_IO_MODE_OCTAL:
	case MSPI_IO_MODE_OCTAL_1_1_8:
	case MSPI_IO_MODE_OCTAL_1_8_8:
		ctrl->read |= FIELD_PREP(XIP_CTRL_FRF_MASK,
					 XIP_CTRL_FRF_OCTAL);
		ctrl->write |= FIELD_PREP(XIP_WRITE_CTRL_FRF_MASK,
					  XIP_WRITE_CTRL_FRF_OCTAL);
		break;
	default:
		LOG_ERR("IO mode %d not supported", io_mode);
		return false;
	}

	/* Transfer format used for Address and Instruction: */

	switch (io_mode) {
	case MSPI_IO_MODE_DUAL_1_1_2:
	case MSPI_IO_MODE_QUAD_1_1_4:
	case MSPI_IO_MODE_OCTAL_1_1_8:
		/* - both sent in Standard SPI mode */
		ctrl->read |= FIELD_PREP(XIP_CTRL_TRANS_TYPE_MASK,
					 XIP_CTRL_TRANS_TYPE_TT0);
		ctrl->write |= FIELD_PREP(XIP_WRITE_CTRL_TRANS_TYPE_MASK,
					  XIP_WRITE_CTRL_TRANS_TYPE_TT0);
		break;
	case MSPI_IO_MODE_DUAL_1_2_2:
	case MSPI_IO_MODE_QUAD_1_4_4:
	case MSPI_IO_MODE_OCTAL_1_8_8:
		/* - Instruction sent in Standard SPI mode,
		 *   Address sent the same way as data
		 */
		ctrl->read |= FIELD_PREP(XIP_CTRL_TRANS_TYPE_MASK,
					 XIP_CTRL_TRANS_TYPE_TT1);
		ctrl->write |= FIELD_PREP(XIP_WRITE_CTRL_TRANS_TYPE_MASK,
					  XIP_WRITE_CTRL_TRANS_TYPE_TT1);
		break;
	default:
		/* - both sent the same way as data. */
		ctrl->read |= FIELD_PREP(XIP_CTRL_TRANS_TYPE_MASK,
					 XIP_CTRL_TRANS_TYPE_TT2);
		ctrl->write |= FIELD_PREP(XIP_WRITE_CTRL_TRANS_TYPE_MASK,
					  XIP_WRITE_CTRL_TRANS_TYPE_TT2);
		break;
	}

	return true;
}

static bool apply_xip_cmd_length(const struct mspi_dw_data *dev_data,
				 struct xip_ctrl *ctrl)
{
	uint8_t cmd_length = dev_data->xip_params_active.cmd_length;

	switch (cmd_length) {
	case 0:
		ctrl->read |= FIELD_PREP(XIP_CTRL_INST_L_MASK,
					 XIP_CTRL_INST_L0);
		ctrl->write |= FIELD_PREP(XIP_WRITE_CTRL_INST_L_MASK,
					  XIP_WRITE_CTRL_INST_L0);
		break;
	case 1:
		ctrl->read |= XIP_CTRL_INST_EN_BIT
			   |  FIELD_PREP(XIP_CTRL_INST_L_MASK,
					 XIP_CTRL_INST_L8);
		ctrl->write |= FIELD_PREP(XIP_WRITE_CTRL_INST_L_MASK,
					  XIP_WRITE_CTRL_INST_L8);
		break;
	case 2:
		ctrl->read |= XIP_CTRL_INST_EN_BIT
			   |  FIELD_PREP(XIP_CTRL_INST_L_MASK,
					 XIP_CTRL_INST_L16);
		ctrl->write |= FIELD_PREP(XIP_WRITE_CTRL_INST_L_MASK,
					  XIP_WRITE_CTRL_INST_L16);
		break;
	default:
		LOG_ERR("Command length %u not supported", cmd_length);
		return false;
	}

	return true;
}

static bool apply_xip_addr_length(const struct mspi_dw_data *dev_data,
				  struct xip_ctrl *ctrl)
{
	uint8_t addr_length = dev_data->xip_params_active.addr_length;

	if (addr_length > 4) {
		LOG_ERR("Address length %u not supported", addr_length);
		return false;
	}

	ctrl->read |= FIELD_PREP(XIP_CTRL_ADDR_L_MASK, addr_length * 2);
	ctrl->write |= FIELD_PREP(XIP_WRITE_CTRL_ADDR_L_MASK, addr_length * 2);

	return true;
}
#endif /* defined(CONFIG_MSPI_XIP) */

static int _api_dev_config(const struct device *dev,
			   const enum mspi_dev_cfg_mask param_mask,
			   const struct mspi_dev_cfg *cfg)
{
	const struct mspi_dw_config *dev_config = dev->config;
	struct mspi_dw_data *dev_data = dev->data;

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

	if (param_mask & MSPI_DEVICE_CONFIG_IO_MODE) {
#if defined(CONFIG_MSPI_XIP)
		dev_data->xip_params_stored.io_mode = cfg->io_mode;
#endif

		if (!apply_io_mode(dev_data, cfg->io_mode)) {
			return -EINVAL;
		}
	}

	if (param_mask & MSPI_DEVICE_CONFIG_CPP) {
#if defined(CONFIG_MSPI_XIP)
		/* Make sure the new setting is compatible with the one used
		 * for XIP if it is enabled.
		 */
		if (!dev_data->xip_enabled) {
			dev_data->xip_cpp = cfg->cpp;
		} else if (dev_data->xip_cpp != cfg->cpp) {
			LOG_ERR("Conflict with configuration used for XIP.");
			return -EINVAL;
		}
#endif

		dev_data->ctrlr0 &= ~(CTRLR0_SCPOL_BIT | CTRLR0_SCPH_BIT);

		switch (cfg->cpp) {
		default:
		case MSPI_CPP_MODE_0:
			dev_data->ctrlr0 |= FIELD_PREP(CTRLR0_SCPOL_BIT, 0) |
					    FIELD_PREP(CTRLR0_SCPH_BIT,  0);
			break;
		case MSPI_CPP_MODE_1:
			dev_data->ctrlr0 |= FIELD_PREP(CTRLR0_SCPOL_BIT, 0) |
					    FIELD_PREP(CTRLR0_SCPH_BIT,  1);
			break;
		case MSPI_CPP_MODE_2:
			dev_data->ctrlr0 |= FIELD_PREP(CTRLR0_SCPOL_BIT, 1) |
					    FIELD_PREP(CTRLR0_SCPH_BIT,  0);
			break;
		case MSPI_CPP_MODE_3:
			dev_data->ctrlr0 |= FIELD_PREP(CTRLR0_SCPOL_BIT, 1) |
					    FIELD_PREP(CTRLR0_SCPH_BIT,  1);
			break;
		}
	}

	if (param_mask & MSPI_DEVICE_CONFIG_FREQUENCY) {
		if (cfg->freq > dev_config->clock_frequency / 2 ||
		    cfg->freq < dev_config->clock_frequency / 65534) {
			LOG_ERR("Invalid frequency: %u, MIN: %u, MAX: %u",
				cfg->freq, dev_config->clock_frequency / 65534,
				dev_config->clock_frequency / 2);
			return -EINVAL;
		}

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

		dev_data->baudr = dev_config->clock_frequency / cfg->freq;
	}

	if (param_mask & MSPI_DEVICE_CONFIG_DATA_RATE) {
		/* TODO: add support for DDR */
		if (cfg->data_rate != MSPI_DATA_RATE_SINGLE) {
			LOG_ERR("Only single data rate is supported.");
			return -ENOTSUP;
		}
	}

	if (param_mask & MSPI_DEVICE_CONFIG_DQS) {
		/* TODO: add support for DQS */
		if (cfg->dqs_enable) {
			LOG_ERR("DQS line is not supported.");
			return -ENOTSUP;
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

	/* Always use Motorola SPI frame format. */
	dev_data->ctrlr0 |= FIELD_PREP(CTRLR0_FRF_MASK, CTRLR0_FRF_SPI);
	/* Enable clock stretching. */
	dev_data->spi_ctrlr0 |= SPI_CTRLR0_CLK_STRETCH_EN_BIT;

	return 0;
}

static int api_dev_config(const struct device *dev,
			  const struct mspi_dev_id *dev_id,
			  const enum mspi_dev_cfg_mask param_mask,
			  const struct mspi_dev_cfg *cfg)
{
	const struct mspi_dw_config *dev_config = dev->config;
	struct mspi_dw_data *dev_data = dev->data;
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

	if (param_mask == MSPI_DEVICE_CONFIG_NONE &&
	    !dev_config->sw_multi_periph) {
		return 0;
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

static int api_get_channel_status(const struct device *dev, uint8_t ch)
{
	ARG_UNUSED(ch);

	struct mspi_dw_data *dev_data = dev->data;

	(void)k_sem_take(&dev_data->ctx_lock, K_FOREVER);

	dev_data->dev_id = NULL;
	k_sem_give(&dev_data->cfg_lock);

	k_sem_give(&dev_data->ctx_lock);

	return 0;
}

static void tx_control_field(const struct device *dev,
			     uint32_t field, uint8_t len)
{
	uint8_t shift = 8 * len;

	do {
		shift -= 8;
		write_dr(dev, field >> shift);
	} while (shift);
}

static int start_next_packet_dma(const struct device *dev, k_timeout_t timeout)
{
	const struct mspi_dw_config *dev_config = dev->config;
	struct mspi_dw_data *dev_data = dev->data;
	struct mspi_xfer xfer = dev_data->xfer;
	const struct mspi_xfer_packet *packet =
		&dev_data->xfer.packets[dev_data->packets_done];
	bool xip_enabled = COND_CODE_1(CONFIG_MSPI_XIP,
				       (dev_data->xip_enabled != 0),
				       (false));
	unsigned int key;
	uint32_t packet_frames;
	uint32_t imr;
	int rc;

	uint8_t *dma_buffer = packet->data_buf;
	uint32_t total_transfer_len = packet->num_bytes + xfer.addr_length + xfer.cmd_length;

	if (packet->num_bytes == 0 &&
	    dev_data->xfer.cmd_length == 0 &&
	    dev_data->xfer.addr_length == 0) {
		return 0;
	}

	/* DMA buffer validation - check if the packet buffer is accessible */
	if (packet->num_bytes > 0 && !vendor_specific_dma_accessible_check(dev, dev_config, dma_buffer)) {
		LOG_ERR("Buffer not DMA accessible: ptr=0x%lx, size=%u", 
			(uintptr_t)dma_buffer, packet->num_bytes);
		return -EINVAL;
	}

	dev_data->ctrlr0 &= ~CTRLR0_TMOD_MASK & ~CTRLR0_DFS_MASK;
	dev_data->spi_ctrlr0 &= ~SPI_CTRLR0_WAIT_CYCLES_MASK;

	/* Set data frame size based on total transfer size */
	if ((total_transfer_len % 4) == 0) {
		dev_data->bytes_per_frame_exp = 2;
		dev_data->ctrlr0 |= FIELD_PREP(CTRLR0_DFS_MASK, 31);
	} else if ((total_transfer_len % 2) == 0) {
		dev_data->bytes_per_frame_exp = 1;
		dev_data->ctrlr0 |= FIELD_PREP(CTRLR0_DFS_MASK, 15);
	} else {
		dev_data->bytes_per_frame_exp = 0;
		dev_data->ctrlr0 |= FIELD_PREP(CTRLR0_DFS_MASK, 7);
	}

	packet_frames = total_transfer_len >> dev_data->bytes_per_frame_exp;
	if (packet_frames > UINT16_MAX + 1) {
		LOG_ERR("Total transfer length (%u) exceeds supported maximum", total_transfer_len);
		return -EINVAL;
	}

	dev_data->dma_cr |= DMA_CR_TDMAE_EN_BIT | DMA_CR_RDMAE_EN_BIT;

	/* Configure transfer mode and interrupts for DMA */
	if (packet->dir == MSPI_TX) {
		imr = IMR_TXEIM_BIT;
		dev_data->ctrlr0 |= FIELD_PREP(CTRLR0_TMOD_MASK, CTRLR0_TMOD_TX);
		dev_data->spi_ctrlr0 |= FIELD_PREP(SPI_CTRLR0_WAIT_CYCLES_MASK,
						   dev_data->xfer.tx_dummy);

	} else if (packet->dir == MSPI_RX) {
		imr = IMR_RXFIM_BIT;
		dev_data->ctrlr0 |= FIELD_PREP(CTRLR0_TMOD_MASK, CTRLR0_TMOD_RX);
		dev_data->spi_ctrlr0 |= FIELD_PREP(SPI_CTRLR0_WAIT_CYCLES_MASK,
						   dev_data->xfer.rx_dummy);
	}

	/* Activate chip select */
	if (dev_data->dev_id->ce.port) {
		rc = gpio_pin_set_dt(&dev_data->dev_id->ce, 1);
		if (rc < 0) {
			LOG_ERR("Failed to activate CE line (%d)", rc);
			return rc;
		}
	}

	if (xip_enabled) {
		key = irq_lock();
		write_ssienr(dev, 0);
	}

	LOG_DBG("packet_frames = %d", packet_frames);
	write_ctrlr0(dev, dev_data->ctrlr0);
	write_ctrlr1(dev, packet_frames > 0
		? FIELD_PREP(CTRLR1_NDF_MASK, packet_frames - 1)
		: 0);
	write_spi_ctrlr0(dev, dev_data->spi_ctrlr0);
	write_baudr(dev, dev_data->baudr);
	write_ser(dev, BIT(dev_data->dev_id->dev_idx));

	write_txftlr(dev, ((2 << QSPI_CORE_CORE_TXFTLR_TXFTHR_Pos)  & QSPI_CORE_CORE_TXFTLR_TXFTHR_Msk) | 
                             ((0    << QSPI_CORE_CORE_TXFTLR_TFT_Pos   )  & QSPI_CORE_CORE_TXFTLR_TFT_Msk   ));
	write_dmatdlr(dev, FIELD_PREP(DMA_TDLR_DMATDL_MASK, dev_config->dma_tx_data_level));
	write_dmardlr(dev, FIELD_PREP(DMA_RDLR_DMARDL_MASK, dev_config->dma_rx_data_level));
	write_dmacr(dev, dev_data->dma_cr);
	rc = vendor_specific_setup_dma_xfer(dev, dev_config, packet, &xfer, dev_data);

	if(rc < 0) {
		LOG_ERR("Vendor specific DMA xfer failure %d", rc);
		return rc;
	}

	if (xip_enabled) {
		write_ssienr(dev, SSIENR_SSIC_EN_BIT);
		irq_unlock(key);
	}
	write_ssienr(dev, SSIENR_SSIC_EN_BIT);
	vendor_specific_start_dma_xfer(dev, dev_config);

	

	/* Wait for completion if synchronous */
	if (!xfer.async) {
		rc = k_sem_take(&dev_data->finished, timeout);
		if (rc < 0) {
			LOG_ERR("DMA transfer timed out %d", rc);
			rc = -ETIMEDOUT;
		}
	}

	/* Deactivate chip select */
	if (dev_data->dev_id->ce.port) {
		int rc2 = gpio_pin_set_dt(&dev_data->dev_id->ce, 0);
		if (rc2 < 0) {
			LOG_ERR("Failed to deactivate CE line (%d)", rc2);
			return rc2;
		}
	}

	LOG_DBG("Ended dma packet");

	return rc;
}

static int start_next_packet_pio(const struct device *dev, k_timeout_t timeout)
{
	const struct mspi_dw_config *dev_config = dev->config;
	struct mspi_dw_data *dev_data = dev->data;
	struct mspi_xfer xfer = dev_data->xfer;
	const struct mspi_xfer_packet *packet =
		&dev_data->xfer.packets[dev_data->packets_done];
	bool xip_enabled = COND_CODE_1(CONFIG_MSPI_XIP,
				       (dev_data->xip_enabled != 0),
				       (false));
	unsigned int key;
	uint32_t packet_frames;
	uint32_t imr;
	int rc = 0;
	if (packet->num_bytes == 0 &&
	    dev_data->xfer.cmd_length == 0 &&
	    dev_data->xfer.addr_length == 0) {
		return 0;
	}

	dev_data->dummy_bytes = 0;
	dev_data->bytes_to_discard = 0;

	dev_data->ctrlr0 &= ~CTRLR0_TMOD_MASK
			 &  ~CTRLR0_DFS_MASK;

	dev_data->spi_ctrlr0 &= ~SPI_CTRLR0_WAIT_CYCLES_MASK;

	if (dev_data->standard_spi &&
	    (dev_data->xfer.cmd_length != 0 ||
	     dev_data->xfer.addr_length != 0)) {
		dev_data->bytes_per_frame_exp = 0;
		dev_data->ctrlr0 |= FIELD_PREP(CTRLR0_DFS_MASK, 7);
	} else {
		if ((packet->num_bytes % 4) == 0) {
			dev_data->bytes_per_frame_exp = 2;
			dev_data->ctrlr0 |= FIELD_PREP(CTRLR0_DFS_MASK, 31);
		} else if ((packet->num_bytes % 2) == 0) {
			dev_data->bytes_per_frame_exp = 1;
			dev_data->ctrlr0 |= FIELD_PREP(CTRLR0_DFS_MASK, 15);
		} else {
			dev_data->bytes_per_frame_exp = 0;
			dev_data->ctrlr0 |= FIELD_PREP(CTRLR0_DFS_MASK, 7);
		}
	}

	packet_frames = packet->num_bytes >> dev_data->bytes_per_frame_exp;

	if (packet_frames > UINT16_MAX + 1) {
		LOG_ERR("Packet length (%u) exceeds supported maximum",
			packet->num_bytes);
		return -EINVAL;
	}

	if (packet->dir == MSPI_TX || packet->num_bytes == 0) {
		imr = IMR_TXEIM_BIT;
		dev_data->ctrlr0 |= FIELD_PREP(CTRLR0_TMOD_MASK,
					       CTRLR0_TMOD_TX);
		dev_data->spi_ctrlr0 |= FIELD_PREP(SPI_CTRLR0_WAIT_CYCLES_MASK,
						   dev_data->xfer.tx_dummy);

		write_rxftlr(dev, 0);
	} else {
		uint32_t tmod;
		uint8_t rx_fifo_threshold;

		/* In Standard SPI Mode, the controller does not support
		 * sending the command and address fields separately, they
		 * need to be sent as data; hence, for RX packets with these
		 * fields, the TX/RX transfer mode needs to be used and
		 * consequently, dummy bytes need to be transmitted so that
		 * clock cycles for the RX part are provided (the controller
		 * does not do it automatically in the TX/RX mode).
		 */
		if (dev_data->standard_spi &&
		    (dev_data->xfer.cmd_length != 0 ||
		     dev_data->xfer.addr_length != 0)) {
			uint32_t rx_total_bytes;
			uint32_t dummy_cycles = dev_data->xfer.rx_dummy;

			dev_data->bytes_to_discard = dev_data->xfer.cmd_length
						   + dev_data->xfer.addr_length
						   + dummy_cycles / 8;
			rx_total_bytes = dev_data->bytes_to_discard
				       + packet->num_bytes;

			dev_data->dummy_bytes = dummy_cycles / 8
					      + packet->num_bytes;

			imr = IMR_TXEIM_BIT | IMR_RXFIM_BIT;
			tmod = CTRLR0_TMOD_TX_RX;
			/* For standard SPI, only 1-byte frames are used. */
			rx_fifo_threshold = MIN(rx_total_bytes - 1,
						dev_config->rx_fifo_threshold);
		} else {
			imr = IMR_RXFIM_BIT;
			tmod = CTRLR0_TMOD_RX;
			rx_fifo_threshold = MIN(packet_frames - 1,
						dev_config->rx_fifo_threshold);

			dev_data->spi_ctrlr0 |=
				FIELD_PREP(SPI_CTRLR0_WAIT_CYCLES_MASK,
					   dev_data->xfer.rx_dummy);
		}

		dev_data->ctrlr0 |= FIELD_PREP(CTRLR0_TMOD_MASK, tmod);

		write_rxftlr(dev, FIELD_PREP(RXFTLR_RFT_MASK,
					     rx_fifo_threshold));
	}

	if (dev_data->dev_id->ce.port) {
		rc = gpio_pin_set_dt(&dev_data->dev_id->ce, 1);
		if (rc < 0) {
			LOG_ERR("Failed to activate CE line (%d)", rc);
			return rc;
		}
	}

	if (xip_enabled) {
		key = irq_lock();
		write_ssienr(dev, 0);
	}

	/* These registers cannot be written when the controller is enabled,
	 * that's why it is temporarily disabled above; with locked interrupts,
	 * to prevent potential XIP transfers during that period.
	 */
	write_ctrlr0(dev, dev_data->ctrlr0);
	write_ctrlr1(dev, packet_frames > 0
		? FIELD_PREP(CTRLR1_NDF_MASK, packet_frames - 1)
		: 0);
	write_spi_ctrlr0(dev, dev_data->spi_ctrlr0);
	write_baudr(dev, dev_data->baudr);
	write_ser(dev, BIT(dev_data->dev_id->dev_idx));

	if (xip_enabled) {
		write_ssienr(dev, SSIENR_SSIC_EN_BIT);
		irq_unlock(key);
	}

	dev_data->buf_pos = packet->data_buf;
	dev_data->buf_end = &packet->data_buf[packet->num_bytes];

	/* Set the TX FIFO threshold and its transmit start level. */
	if (packet->num_bytes) {
		/* If there is some data to send/receive, set the threshold to
		 * the value configured for the driver instance and the start
		 * level to the maximum possible value (it will be updated later
		 * in tx_fifo() or tx_dummy_bytes() when TX is to be finished).
		 * This helps avoid a situation when the TX FIFO becomes empty
		 * before the transfer is complete and the SSI core finishes the
		 * transaction and deactivates the CE line. This could occur
		 * right before the data phase in enhanced SPI modes, when the
		 * clock stretching feature does not work yet, or in Standard
		 * SPI mode, where the clock stretching is not available at all.
		 */
		uint8_t start_level = dev_data->dummy_bytes != 0
				    ? dev_config->max_queued_dummy_bytes - 1
				    : dev_config->tx_fifo_depth_minus_1;

		write_txftlr(dev, FIELD_PREP(TXFTLR_TXFTHR_MASK, start_level) |
				  FIELD_PREP(TXFTLR_TFT_MASK,
					     dev_config->tx_fifo_threshold));
	} else {
		uint32_t total_tx_entries = 0;

		/* It the whole transfer is to contain only the command and/or
		 * address, set up the transfer to start right after entries
		 * for those appear in the TX FIFO, and the threshold to 0,
		 * so that the interrupt occurs when the TX FIFO gets emptied.
		 */
		if (dev_data->xfer.cmd_length) {
			if (dev_data->standard_spi) {
				total_tx_entries += dev_data->xfer.cmd_length;
			} else {
				total_tx_entries += 1;
			}
		}

		if (dev_data->xfer.addr_length) {
			if (dev_data->standard_spi) {
				total_tx_entries += dev_data->xfer.addr_length;
			} else {
				total_tx_entries += 1;
			}
		}

		write_txftlr(dev, FIELD_PREP(TXFTLR_TXFTHR_MASK,
					     total_tx_entries - 1));
	}

	/* Ensure that there will be no interrupt from the controller yet. */
	write_imr(dev, 0);
	/* Enable the controller. This must be done before DR is written. */
	write_ssienr(dev, SSIENR_SSIC_EN_BIT);

	/* Since the FIFO depth in SSI is always at least 8, it can be safely
	 * assumed that the command and address fields (max. 2 and 4 bytes,
	 * respectively) can be written here before the TX FIFO gets filled up.
	 */
	if (dev_data->standard_spi) {
		if (dev_data->xfer.cmd_length) {
			tx_control_field(dev, packet->cmd,
					 dev_data->xfer.cmd_length);
		}

		if (dev_data->xfer.addr_length) {
			tx_control_field(dev, packet->address,
					 dev_data->xfer.addr_length);
		}
	} else {
		if (dev_data->xfer.cmd_length) {
			write_dr(dev, packet->cmd);
		}

		if (dev_data->xfer.addr_length) {
			write_dr(dev, packet->address);
		}
	}

	if (packet->dir == MSPI_TX && packet->num_bytes) {
		tx_data(dev, packet);
	}

	/* Enable interrupts now and wait until the packet is done unless async. */
	write_imr(dev, imr);

	if (!xfer.async) {
		rc = k_sem_take(&dev_data->finished, timeout);
		/* TODO: detect overflow in async mode*/
		if (read_risr(dev) & RISR_RXOIR_BIT) {
			LOG_ERR("RX FIFO overflow occurred");
			rc = -EIO;
		} else if (rc < 0) {
			LOG_ERR("Transfer timed out %d", rc);
			rc = -ETIMEDOUT;
		}
	}

	/* Disable the controller. This will immediately halt the transfer
	 * if it hasn't finished yet.
	 */
	if (xip_enabled) {
		/* If XIP is enabled, the controller must be kept enabled,
		 * so disable it only momentarily if there's a need to halt
		 * a transfer that has timeout out.
		 */
		if (rc == -ETIMEDOUT) {
			key = irq_lock();

			write_ssienr(dev, 0);
			write_ssienr(dev, SSIENR_SSIC_EN_BIT);

			irq_unlock(key);
		}
	} else {
		if (!xfer.async) {
			write_ssienr(dev, 0);
		}
	}

	if (dev_data->dev_id->ce.port) {
		int rc2;

		/* Do not use `rc` to not overwrite potential timeout error. */
		rc2 = gpio_pin_set_dt(&dev_data->dev_id->ce, 0);
		if (rc2 < 0) {
			LOG_ERR("Failed to deactivate CE line (%d)", rc2);
			return rc2;
		}
	}

	return rc;
}

static int start_next_packet(const struct device *dev, k_timeout_t timeout)
{
	struct mspi_dw_data * dev_data = dev->data;
    switch (dev_data->xfer.xfer_mode) {
    case MSPI_DMA:
        return start_next_packet_dma(dev, timeout);
    case MSPI_PIO:
        return start_next_packet_pio(dev, timeout);
    default:
        LOG_ERR("Xfer mode not supported");
        return -EIO;
    }
}

static int _api_transceive(const struct device *dev,
			   const struct mspi_xfer *req)
{
	struct mspi_dw_data *dev_data = dev->data;
	int rc;

	dev_data->spi_ctrlr0 &= ~SPI_CTRLR0_INST_L_MASK
			     &  ~SPI_CTRLR0_ADDR_L_MASK;

	if (!apply_cmd_length(dev_data, req->cmd_length) ||
	    !apply_addr_length(dev_data, req->addr_length)) {
		return -EINVAL;
	}

	if (dev_data->standard_spi) {
		if (req->tx_dummy) {
			LOG_ERR("TX dummy cycles unsupported in single line mode");
			return -EINVAL;
		}
		if (req->rx_dummy % 8) {
			LOG_ERR("Unsupported RX (%u) dummy cycles", req->rx_dummy);
			return -EINVAL;
		}
	} else if (req->rx_dummy > SPI_CTRLR0_WAIT_CYCLES_MAX ||
		   req->tx_dummy > SPI_CTRLR0_WAIT_CYCLES_MAX) {
		LOG_ERR("Unsupported RX (%u) or TX (%u) dummy cycles",
			req->rx_dummy, req->tx_dummy);
		return -EINVAL;
	}

	dev_data->xfer = *req;
	/* 
	 * For async, next packet is started by ISR, also choose between DMA and PIO 
	 * mode based on transfer configuration 
	 */
	if (req->async) {
    rc = start_next_packet(dev, K_MSEC(dev_data->xfer.timeout));
	} else {
		for (dev_data->packets_done = 0;
	     dev_data->packets_done < dev_data->xfer.num_packet;
	     dev_data->packets_done++) {
			rc = start_next_packet(dev, K_MSEC(dev_data->xfer.timeout));
			if (rc < 0) {
				return rc;
			}
		}
	}

	return 0;
}

static int api_transceive(const struct device *dev,
			  const struct mspi_dev_id *dev_id,
			  const struct mspi_xfer *req)
{
	struct mspi_dw_data *dev_data = dev->data;
	int rc, rc2;

	mspi_callback_handler_t cb = NULL;
	struct mspi_callback_context *cb_ctx = NULL;

	if (dev_id != dev_data->dev_id) {
		LOG_ERR("Controller is not configured for this device");
		return -EINVAL;
	}

	if (req->async) {
		LOG_DBG("async transfer selected");
		cb = dev_data->cbs[MSPI_BUS_XFER_COMPLETE];
		cb_ctx = dev_data->cb_ctxs[MSPI_BUS_XFER_COMPLETE];
	}

	rc = pm_device_runtime_get(dev);
	if (rc < 0) {
		LOG_ERR("pm_device_runtime_get() failed: %d", rc);
		return rc;
	}

	(void)k_sem_take(&dev_data->ctx_lock, K_FOREVER);

	// Add check for ongoing async transfer
	if (req->async && dev_data->async_in_progress) {
		LOG_ERR("Async transfer already in progress");
		rc = -EBUSY;
		goto out_unlock;
	}

	if (dev_data->suspended) {
		rc = -EFAULT;
	} else {
		/* Set async busy flag before starting transfer */
		if (req->async) {
			dev_data->async_in_progress = true;
		}
		rc = _api_transceive(dev, req);
	}

out_unlock:
	k_sem_give(&dev_data->ctx_lock);

	rc2 = pm_device_runtime_put(dev);
	if (rc2 < 0) {
		LOG_ERR("pm_device_runtime_put() failed: %d", rc2);
		rc = (rc < 0 ? rc : rc2);
	}

	return rc;
}

static int api_register_callback(const struct device *dev,
					const struct mspi_dev_id *dev_id,
					const enum mspi_bus_event evt_type,
					mspi_callback_handler_t cb,
					struct mspi_callback_context *ctx)
{
	// const struct mspi_dw_config *cfg = dev->config;
	struct mspi_dw_data *dev_data = dev->data;

	if (dev_id != dev_data->dev_id) {
		LOG_ERR("Controller is not configured for this device");
		return -EINVAL;
	}

	if (evt_type != MSPI_BUS_XFER_COMPLETE) {
		LOG_ERR("Callback type not supported");
		return -ENOTSUP;
	}

	dev_data->cbs[evt_type] = cb;
	dev_data->cb_ctxs[evt_type] = ctx;
	return 0;
}

#if defined(CONFIG_MSPI_XIP)
static int _api_xip_config(const struct device *dev,
			   const struct mspi_dev_id *dev_id,
			   const struct mspi_xip_cfg *cfg)
{
	struct mspi_dw_data *dev_data = dev->data;
	int rc;

	if (!cfg->enable) {
		rc = vendor_specific_xip_disable(dev, dev_id, cfg);
		if (rc < 0) {
			return rc;
		}

		dev_data->xip_enabled &= ~BIT(dev_id->dev_idx);

		if (!dev_data->xip_enabled) {
			write_ssienr(dev, 0);

			/* Since XIP is disabled, it is okay for the controller
			 * to be suspended.
			 */
			rc = pm_device_runtime_put(dev);
			if (rc < 0) {
				LOG_ERR("pm_device_runtime_put() failed: %d", rc);
				return rc;
			}
		}

		return 0;
	}

	if (!dev_data->xip_enabled) {
		struct xip_params *params = &dev_data->xip_params_active;
		struct xip_ctrl ctrl = {0};

		*params = dev_data->xip_params_stored;

		if (!apply_xip_io_mode(dev_data, &ctrl) ||
		    !apply_xip_cmd_length(dev_data, &ctrl) ||
		    !apply_xip_addr_length(dev_data, &ctrl)) {
			return -EINVAL;
		}

		if (params->rx_dummy > XIP_CTRL_WAIT_CYCLES_MAX ||
		    params->tx_dummy > XIP_WRITE_CTRL_WAIT_CYCLES_MAX) {
			LOG_ERR("Unsupported RX (%u) or TX (%u) dummy cycles",
				params->rx_dummy, params->tx_dummy);
			return -EINVAL;
		}

		/* Increase usage count additionally to prevent the controller
		 * from being suspended as long as XIP is active.
		 */
		rc = pm_device_runtime_get(dev);
		if (rc < 0) {
			LOG_ERR("pm_device_runtime_get() failed: %d", rc);
			return rc;
		}

		ctrl.read |= FIELD_PREP(XIP_CTRL_WAIT_CYCLES_MASK,
					params->rx_dummy);
		ctrl.write |= FIELD_PREP(XIP_WRITE_CTRL_WAIT_CYCLES_MASK,
					 params->tx_dummy);

		/* Make sure the baud rate and serial clock phase/polarity
		 * registers are configured properly. They may not be if
		 * non-XIP transfers have not been performed yet.
		 */
		write_ctrlr0(dev, dev_data->ctrlr0);
		write_baudr(dev, dev_data->baudr);

		write_xip_incr_inst(dev, params->read_cmd);
		write_xip_wrap_inst(dev, params->read_cmd);
		write_xip_ctrl(dev, ctrl.read);
		write_xip_write_incr_inst(dev, params->write_cmd);
		write_xip_write_wrap_inst(dev, params->write_cmd);
		write_xip_write_ctrl(dev, ctrl.write);
	} else if (dev_data->xip_params_active.read_cmd !=
		   dev_data->xip_params_stored.read_cmd ||
		   dev_data->xip_params_active.write_cmd !=
		   dev_data->xip_params_stored.write_cmd ||
		   dev_data->xip_params_active.cmd_length !=
		   dev_data->xip_params_stored.cmd_length ||
		   dev_data->xip_params_active.addr_length !=
		   dev_data->xip_params_stored.addr_length ||
		   dev_data->xip_params_active.rx_dummy !=
		   dev_data->xip_params_stored.rx_dummy ||
		   dev_data->xip_params_active.tx_dummy !=
		   dev_data->xip_params_stored.tx_dummy) {
		LOG_ERR("Conflict with configuration already used for XIP.");
		return -EINVAL;
	}

	rc = vendor_specific_xip_enable(dev, dev_id, cfg);
	if (rc < 0) {
		return rc;
	}

	write_ssienr(dev, SSIENR_SSIC_EN_BIT);

	dev_data->xip_enabled |= BIT(dev_id->dev_idx);

	return 0;
}

static int api_xip_config(const struct device *dev,
			  const struct mspi_dev_id *dev_id,
			  const struct mspi_xip_cfg *cfg)
{
	struct mspi_dw_data *dev_data = dev->data;
	int rc, rc2;

	if (cfg->enable && dev_id != dev_data->dev_id) {
		LOG_ERR("Controller is not configured for this device");
		return -EINVAL;
	}

	rc = pm_device_runtime_get(dev);
	if (rc < 0) {
		LOG_ERR("pm_device_runtime_get() failed: %d", rc);
		return rc;
	}

	(void)k_sem_take(&dev_data->ctx_lock, K_FOREVER);

	if (dev_data->suspended) {
		rc = -EFAULT;
	} else {
		rc = _api_xip_config(dev, dev_id, cfg);
	}

	k_sem_give(&dev_data->ctx_lock);

	rc2 = pm_device_runtime_put(dev);
	if (rc2 < 0) {
		LOG_ERR("pm_device_runtime_put() failed: %d", rc2);
		rc = (rc < 0 ? rc : rc2);
	}

	return rc;
}
#endif /* defined(CONFIG_MSPI_XIP) */

static int dev_pm_action_cb(const struct device *dev,
			    enum pm_device_action action)
{
	struct mspi_dw_data *dev_data = dev->data;

	if (action == PM_DEVICE_ACTION_RESUME) {
#if defined(CONFIG_PINCTRL)
		const struct mspi_dw_config *dev_config = dev->config;
		int rc = pinctrl_apply_state(dev_config->pcfg,
					     PINCTRL_STATE_DEFAULT);

		if (rc < 0) {
			LOG_ERR("Cannot apply default pins state (%d)", rc);
			return rc;
		}
#endif
		vendor_specific_resume(dev, dev->config);

		dev_data->suspended = false;

		return 0;
	}

	if (IS_ENABLED(CONFIG_PM_DEVICE) &&
	    action == PM_DEVICE_ACTION_SUSPEND) {
		bool xip_enabled = COND_CODE_1(CONFIG_MSPI_XIP,
					       (dev_data->xip_enabled != 0),
					       (false));

#if defined(CONFIG_PINCTRL)
		const struct mspi_dw_config *dev_config = dev->config;
		int rc = pinctrl_apply_state(dev_config->pcfg,
					     PINCTRL_STATE_SLEEP);

		if (rc < 0) {
			LOG_ERR("Cannot apply sleep pins state (%d)", rc);
			return rc;
		}
#endif
		if (xip_enabled ||
		    k_sem_take(&dev_data->ctx_lock, K_NO_WAIT) != 0) {
			LOG_ERR("Controller in use, cannot be suspended");
			return -EBUSY;
		}

		dev_data->suspended = true;

		vendor_specific_suspend(dev, dev->config);

		k_sem_give(&dev_data->ctx_lock);

		return 0;
	}

	return -ENOTSUP;
}

static int dev_init(const struct device *dev)
{
	struct mspi_dw_data *dev_data = dev->data;
	const struct mspi_dw_config *dev_config = dev->config;
	const struct gpio_dt_spec *ce_gpio;
	int rc;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	vendor_specific_init(dev, dev->config);

	dev_data->ctrlr0 |= FIELD_PREP(CTRLR0_SSIISMST_BIT, !(dev_config->op_mode));

	dev_config->irq_config();

	k_sem_init(&dev_data->finished, 0, 1);
	k_sem_init(&dev_data->cfg_lock, 1, 1);
	k_sem_init(&dev_data->ctx_lock, 1, 1);
	dev_data->async_in_progress = false;

	for (ce_gpio = dev_config->ce_gpios;
	     ce_gpio < &dev_config->ce_gpios[dev_config->ce_gpios_len];
	     ce_gpio++) {
		if (!device_is_ready(ce_gpio->port)) {
			LOG_ERR("CE GPIO port %s is not ready",
				ce_gpio->port->name);
			return -ENODEV;
		}

		rc = gpio_pin_configure_dt(ce_gpio, GPIO_OUTPUT_INACTIVE);
		if (rc < 0) {
			return rc;
		}
	}

#if defined(CONFIG_PINCTRL)
	if (IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)) {
		rc = pinctrl_apply_state(dev_config->pcfg, PINCTRL_STATE_SLEEP);
		if (rc < 0) {
			LOG_ERR("Cannot apply sleep pins state (%d)", rc);
			return rc;
		}
	}
#endif

	return pm_device_driver_init(dev, dev_pm_action_cb);
}

static DEVICE_API(mspi, drv_api) = {
	.config             = api_config,
	.dev_config         = api_dev_config,
	.get_channel_status = api_get_channel_status,
	.transceive         = api_transceive,
	.register_callback  = api_register_callback
#if defined(CONFIG_MSPI_XIP)
	.xip_config         = api_xip_config,
#endif
};

#define MSPI_DW_INST_IRQ(idx, inst)					\
	IRQ_CONNECT(DT_INST_IRQ_BY_IDX(inst, idx, irq),			\
		    DT_INST_IRQ_BY_IDX(inst, idx, priority),		\
		    mspi_dw_isr, DEVICE_DT_INST_GET(inst), 0);		\
	irq_enable(DT_INST_IRQ_BY_IDX(inst, idx, irq))

#define MSPI_DW_MMIO_ROM_INIT(node_id)					\
	COND_CODE_1(DT_REG_HAS_NAME(node_id, core),			\
		(Z_DEVICE_MMIO_NAMED_ROM_INITIALIZER(core, node_id)),	\
		(DEVICE_MMIO_ROM_INIT(node_id)))

#define MSPI_DW_CLOCK_FREQUENCY(inst)					\
	COND_CODE_1(DT_NODE_HAS_PROP(DT_INST_PHANDLE(inst, clocks),	\
				     clock_frequency),			\
		(DT_INST_PROP_BY_PHANDLE(inst, clocks,			\
					 clock_frequency)),		\
		(DT_INST_PROP(inst, clock_frequency)))

#define MSPI_DW_DT_INST_PROP(inst, prop) .prop = DT_INST_PROP(inst, prop)

#define FOREACH_CE_GPIOS_ELEM(inst)					\
	DT_INST_FOREACH_PROP_ELEM_SEP(inst, ce_gpios,			\
				      GPIO_DT_SPEC_GET_BY_IDX, (,))
#define MSPI_DW_CE_GPIOS(inst)						\
	.ce_gpios = (const struct gpio_dt_spec [])			\
		{ FOREACH_CE_GPIOS_ELEM(inst) },			\
	.ce_gpios_len = DT_INST_PROP_LEN(inst, ce_gpios)

#define TX_FIFO_DEPTH(inst) DT_INST_PROP(inst, fifo_depth)
#define RX_FIFO_DEPTH(inst) DT_INST_PROP_OR(inst, rx_fifo_depth,	\
					    TX_FIFO_DEPTH(inst))
#define MSPI_DW_FIFO_PROPS(inst)					\
	.tx_fifo_depth_minus_1 = TX_FIFO_DEPTH(inst) - 1,		\
	.max_queued_dummy_bytes = MIN(RX_FIFO_DEPTH(inst) - 1,		\
				      TX_FIFO_DEPTH(inst)),		\
	.tx_fifo_threshold =						\
		DT_INST_PROP_OR(inst, tx_fifo_threshold,		\
				7 * TX_FIFO_DEPTH(inst) / 8 - 1),	\
	.rx_fifo_threshold =						\
		DT_INST_PROP_OR(inst, rx_fifo_threshold,		\
				1 * RX_FIFO_DEPTH(inst) / 8 - 1),	\
	.dma_tx_data_level =						\
		DT_INST_PROP_OR(inst, dma_transmit_data_level, 0),	\
	.dma_rx_data_level =						\
		DT_INST_PROP_OR(inst, dma_recieve_data_level, 0)
#define MSPI_DW_INST(inst)						\
	PM_DEVICE_DT_INST_DEFINE(inst, dev_pm_action_cb);		\
	IF_ENABLED(CONFIG_PINCTRL, (PINCTRL_DT_INST_DEFINE(inst);))	\
	static void irq_config##inst(void)				\
	{								\
		LISTIFY(DT_INST_NUM_IRQS(inst),				\
			MSPI_DW_INST_IRQ, (;), inst);			\
	}								\
	static struct mspi_dw_data dev##inst##_data;			\
	static const struct mspi_dw_config dev##inst##_config = {	\
		MSPI_DW_MMIO_ROM_INIT(DT_DRV_INST(inst)),		\
                .wrapper_regs = (void*)DT_INST_REG_ADDR(inst),          \
		.irq_config = irq_config##inst,				\
		.clock_frequency = MSPI_DW_CLOCK_FREQUENCY(inst),	\
	IF_ENABLED(CONFIG_PINCTRL,					\
		(.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),))	\
	IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, ce_gpios),		\
		(MSPI_DW_CE_GPIOS(inst),))				\
		MSPI_DW_FIFO_PROPS(inst),				\
		DEFINE_REG_ACCESS(inst)					\
		.sw_multi_periph =					\
			DT_INST_PROP(inst, software_multiperipheral),	\
		.op_mode = DT_STRING_TOKEN(DT_INST(inst, 		\
					   DT_DRV_COMPAT), op_mode),	\
	};								\
	DEVICE_DT_INST_DEFINE(inst,					\
		dev_init, PM_DEVICE_DT_INST_GET(inst),			\
		&dev##inst##_data, &dev##inst##_config,			\
		POST_KERNEL, CONFIG_MSPI_INIT_PRIORITY,			\
		&drv_api);

DT_INST_FOREACH_STATUS_OKAY(MSPI_DW_INST)
