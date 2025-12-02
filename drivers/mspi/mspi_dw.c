/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * Copyright (c) 2025 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT snps_designware_ssi

#include <zephyr/drivers/mspi.h>
#include <zephyr/drivers/gpio.h>
#if defined(CONFIG_PINCTRL)
#include <zephyr/drivers/pinctrl.h>
#endif
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/mspi/mspi_dw.h>

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
	uint32_t rx_sample_dly;

#if defined(CONFIG_MSPI_XIP)
	uint32_t xip_freq;
	struct xip_params xip_params_stored;
	struct xip_params xip_params_active;
	uint16_t xip_enabled;
	enum mspi_cpp_mode xip_cpp;
#endif

	uint32_t dummy_bytes;
	uint8_t bytes_to_discard;
	uint8_t bytes_per_frame_exp;
	bool standard_spi;
	bool suspended;

#if defined(CONFIG_MULTITHREADING)
	const struct device *dev;

	struct k_sem finished;
	/* For synchronization of API calls made from different contexts. */
	struct k_sem ctx_lock;
	/* For locking of controller configuration. */
	struct k_sem cfg_lock;

	struct k_timer async_timer;
	struct k_work async_timeout_work;
	struct k_work async_packet_work;

	mspi_callback_handler_t cbs[MSPI_BUS_EVENT_MAX];
	struct mspi_callback_context *cb_ctxs[MSPI_BUS_EVENT_MAX];
#else
	volatile bool finished;
	bool cfg_lock;
#endif

	struct mspi_xfer xfer;

#if defined(CONFIG_MSPI_DW_HANDLE_FIFOS_IN_SYSTEM_WORKQUEUE)
	struct k_work fifo_work;
	uint32_t imr;
#endif
};

struct mspi_dw_config {
	DEVICE_MMIO_ROM;
	void *wrapper_regs;
	void (*irq_config)(void);
	uint32_t clock_frequency;
#if defined(CONFIG_PINCTRL)
	const struct pinctrl_dev_config *pcfg;
#endif
	const struct gpio_dt_spec *ce_gpios;
	uint8_t ce_gpios_len;
	uint8_t tx_fifo_depth_minus_1;
	/* Maximum number of items allowed in the TX FIFO when transmitting
	 * dummy bytes; it must be at least one less than the RX FIFO depth
	 * to account for a byte that can be partially received (i.e. in
	 * the shifting register) when tx_dummy_bytes() calculates how many
	 * bytes can be written to the TX FIFO to not overflow the RX FIFO.
	 */
	uint8_t max_queued_dummy_bytes;
	uint8_t tx_fifo_threshold;
	uint8_t rx_fifo_threshold;
#ifdef CONFIG_MSPI_DMA
	uint8_t dma_tx_data_level;
	uint8_t dma_rx_data_level;
#endif
	void *vendor_specific_data;
	DECLARE_REG_ACCESS();
	bool sw_multi_periph;
	enum mspi_op_mode op_mode;
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
DEFINE_MM_REG_RD_WR(imr,	0x2c)
DEFINE_MM_REG_RD(isr,		0x30)
DEFINE_MM_REG_RD(risr,		0x34)
DEFINE_MM_REG_RD_WR(dr,		0x60)
DEFINE_MM_REG_WR(rx_sample_dly,	0xf0)
DEFINE_MM_REG_WR(spi_ctrlr0,	0xf4)
DEFINE_MM_REG_WR(txd_drive_edge, 0xf8)
#if defined(CONFIG_MSPI_DMA)
DEFINE_MM_REG_WR(dmacr,		0x4C)
DEFINE_MM_REG_WR(dmatdlr,	0x50)
DEFINE_MM_REG_WR(dmardlr,	0x54)
#endif

#if defined(CONFIG_MSPI_XIP)
DEFINE_MM_REG_WR(xip_incr_inst,		0x100)
DEFINE_MM_REG_WR(xip_wrap_inst,		0x104)
DEFINE_MM_REG_WR(xip_ctrl,		0x108)
DEFINE_MM_REG_WR(xip_write_incr_inst,	0x140)
DEFINE_MM_REG_WR(xip_write_wrap_inst,	0x144)
DEFINE_MM_REG_WR(xip_write_ctrl,	0x148)
#endif

#include "mspi_dw_vendor_specific.h"

static int start_next_packet(const struct device *dev);
static int finalize_packet(const struct device *dev, int rc);
static int finalize_transceive(const struct device *dev, int rc);

#if defined(CONFIG_MULTITHREADING)
/* Common function to setup callback context and call user callback */
static void call_user_callback_with_context(const struct device *dev,
					    enum mspi_bus_event evt_type,
					    uint32_t packet_idx,
					    int status)
{
	struct mspi_dw_data *dev_data = dev->data;
	const struct mspi_xfer_packet *packet =
		&dev_data->xfer.packets[packet_idx];
	struct mspi_callback_context *cb_ctx = dev_data->cb_ctxs[evt_type];

	if (!(packet->cb_mask & BIT(evt_type)) ||
	    !dev_data->cbs[evt_type]) {
		return;
	}

	LOG_DBG("Calling user function with evt_type: %u", evt_type);

	cb_ctx->mspi_evt.evt_type = evt_type;
	cb_ctx->mspi_evt.evt_data.controller = dev;
	cb_ctx->mspi_evt.evt_data.dev_id = dev_data->dev_id;
	cb_ctx->mspi_evt.evt_data.packet = packet;
	cb_ctx->mspi_evt.evt_data.packet_idx = packet_idx;
	cb_ctx->mspi_evt.evt_data.status = status;

	dev_data->cbs[evt_type](cb_ctx);
}

static void async_timeout_timer_handler(struct k_timer *timer)
{
	struct mspi_dw_data *dev_data =
		CONTAINER_OF(timer, struct mspi_dw_data, async_timer);

	/* Submit work to handle timeout in proper context */
	k_work_submit(&dev_data->async_timeout_work);
}

static void async_timeout_work_handler(struct k_work *work)
{
	struct mspi_dw_data *dev_data =
		CONTAINER_OF(work, struct mspi_dw_data, async_timeout_work);
	const struct device *dev = dev_data->dev;
	int rc;

	LOG_ERR("Async transfer timed out");

	rc = finalize_packet(dev, -ETIMEDOUT);
	rc = finalize_transceive(dev, rc);

	/* Call user callback with timeout error (outside of any locks) */
	call_user_callback_with_context(dev, MSPI_BUS_TIMEOUT,
					dev_data->packets_done, rc);
}

static void async_packet_work_handler(struct k_work *work)
{
	struct mspi_dw_data *dev_data =
		CONTAINER_OF(work, struct mspi_dw_data, async_packet_work);
	const struct device *dev = dev_data->dev;
	uint32_t packet_idx = dev_data->packets_done;
	int rc;

	LOG_DBG("Processing async work in thread context");

	rc = finalize_packet(dev, 0);
	if (rc >= 0) {
		++dev_data->packets_done;
		if (dev_data->packets_done < dev_data->xfer.num_packet) {
			LOG_DBG("Starting next packet (%d/%d)",
				dev_data->packets_done + 1,
				dev_data->xfer.num_packet);

			rc = start_next_packet(dev);
			if (rc >= 0) {
				return;
			}

			++packet_idx;
		}
	}

	rc = finalize_transceive(dev, rc);
	call_user_callback_with_context(dev,
		rc < 0 ? MSPI_BUS_ERROR
		       : MSPI_BUS_XFER_COMPLETE,
		packet_idx, rc);
}
#endif /* defined(CONFIG_MULTITHREADING) */

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

static bool tx_dummy_bytes(const struct device *dev, bool *repeat)
{
	struct mspi_dw_data *dev_data = dev->data;
	const struct mspi_dw_config *dev_config = dev->config;
	uint8_t fifo_room = dev_config->max_queued_dummy_bytes
			  - FIELD_GET(TXFLR_TXTFL_MASK, read_txflr(dev));
	uint8_t rx_fifo_items = FIELD_GET(RXFLR_RXTFL_MASK, read_rxflr(dev));
	uint32_t dummy_bytes = dev_data->dummy_bytes;
	const uint8_t dummy_val = 0;

	/* Subtract the number of items that are already stored in the RX
	 * FIFO to avoid overflowing it; `max_queued_dummy_bytes` accounts
	 * that one byte that can be partially received, thus not included
	 * in the value from the RXFLR register.
	 * This check also handles the case when the function is called but
	 * the TX FIFO is already filled up (fifo_room == 0).
	 */
	if (fifo_room <= rx_fifo_items) {
		if (repeat) {
			/* If no dummy bytes can be sent now, there is no point
			 * in repeating the loop that reads the RX FIFO.
			 */
			*repeat = false;
		}

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

	dev_data->dummy_bytes = 0;

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
	uint32_t remaining_frames;
	uint32_t in_fifo = FIELD_GET(RXFLR_RXTFL_MASK, read_rxflr(dev));

	if (in_fifo == 0) {
		return false;
	}

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

static inline void set_imr(const struct device *dev, uint32_t imr)
{
#if defined(CONFIG_MSPI_DW_HANDLE_FIFOS_IN_SYSTEM_WORKQUEUE)
	struct mspi_dw_data *dev_data = dev->data;

	dev_data->imr = imr;
#else
	write_imr(dev, imr);
#endif
}

static void handle_end_of_packet(struct mspi_dw_data *dev_data)

{
#if defined(CONFIG_MULTITHREADING)
	if (dev_data->xfer.async) {
		k_timer_stop(&dev_data->async_timer);

		k_work_submit(&dev_data->async_packet_work);
	} else {
		k_sem_give(&dev_data->finished);
	}
#else
	dev_data->finished = true;
#endif
}

static void handle_fifos(const struct device *dev)
{
	struct mspi_dw_data *dev_data = dev->data;
	const struct mspi_xfer_packet *packet =
		&dev_data->xfer.packets[dev_data->packets_done];
	bool finished = false;

	if (packet->dir == MSPI_TX) {
		if (dev_data->buf_pos < dev_data->buf_end) {
			tx_data(dev, packet);
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
		bool repeat = true;

		do {
			/* Always read everything from the RX FIFO, regardless
			 * of the interrupt status.
			 * tx_dummy_bytes() subtracts the number of items that
			 * are present in the RX FIFO from the number of dummy
			 * bytes it is allowed to send, so it can potentially
			 * not fill the TX FIFO above its transfer start level
			 * in some iteration of this loop. If in such case the
			 * interrupt handler exited without emptying the RX FIFO
			 * (seeing the RXFIS flag not set due to not enough
			 * items in the RX FIFO), this could lead to a situation
			 * in which a transfer stopped temporarily (after the TX
			 * FIFO got empty) is not resumed (since the TX FIFO is
			 * not filled above its transfer start level), so no
			 * further dummy bytes are transmitted and the RX FIFO
			 * has no chance to get new entries, hence no further
			 * interrupts are generated and the transfer gets stuck.
			 */
			if (read_rx_fifo(dev, packet)) {
				finished = true;
				break;
			}

			/* Use RISR, not ISR, because when this function is
			 * executed through the system workqueue, all interrupts
			 * are masked (IMR is 0).
			 */
			uint32_t int_status = read_risr(dev);

			if (int_status & RISR_RXOIR_BIT) {
				finished = true;
				break;
			}

			/* If there are still some dummy bytes to transmit,
			 * always try to put some into the TX FIFO, no matter
			 * what's the TXE interrupt status - the TX FIFO may be
			 * filled above its threshold level (then its interrupt
			 * flag is not set), but below its transfer start level,
			 * so the controller may be waiting for more items to
			 * appear there.
			 */
			if (dev_data->dummy_bytes == 0) {
				break;
			}

			if (tx_dummy_bytes(dev, &repeat)) {
				/* All the required dummy bytes were written
				 * to the FIFO; disable the TXE interrupt,
				 * as it's no longer needed.
				 */
				set_imr(dev, IMR_RXFIM_BIT);
			}

			/* Repeat the loop only if any dummy bytes were sent. */
		} while (repeat);
	}

	if (finished) {
		set_imr(dev, 0);

		handle_end_of_packet(dev_data);

	}
}

#if defined(CONFIG_MSPI_DW_HANDLE_FIFOS_IN_SYSTEM_WORKQUEUE)
static void fifo_work_handler(struct k_work *work)
{
	struct mspi_dw_data *dev_data =
		CONTAINER_OF(work, struct mspi_dw_data, fifo_work);
	const struct device *dev = dev_data->dev;

	handle_fifos(dev);

	write_imr(dev, dev_data->imr);
}
#endif

static void mspi_dw_isr(const struct device *dev)
{
#if defined(CONFIG_MSPI_DMA)
	struct mspi_dw_data *dev_data = dev->data;

	if (dev_data->xfer.xfer_mode == MSPI_DMA) {
		if (vendor_specific_read_dma_irq(dev)) {
			set_imr(dev, 0);
			handle_end_of_packet(dev_data);
		}
		vendor_specific_irq_clear(dev);
		return;
	}
#endif

#if defined(CONFIG_MSPI_DW_HANDLE_FIFOS_IN_SYSTEM_WORKQUEUE)
	struct mspi_dw_data *dev_data = dev->data;
	int rc;

	dev_data->imr = read_imr(dev);
	write_imr(dev, 0);

	rc = k_work_submit(&dev_data->fifo_work);
	if (rc < 0) {
		LOG_ERR("k_work_submit failed: %d\n", rc);
	}
#else
	handle_fifos(dev);
#endif

	vendor_specific_irq_clear(dev);
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
		dev_data->spi_ctrlr0 &= ~(SPI_CTRLR0_SPI_DDR_EN_BIT |
					  SPI_CTRLR0_INST_DDR_EN_BIT);
		switch (cfg->data_rate) {
		case MSPI_DATA_RATE_SINGLE:
			break;
#if defined(CONFIG_MSPI_DW_DDR)
		case MSPI_DATA_RATE_DUAL:
			dev_data->spi_ctrlr0 |= SPI_CTRLR0_INST_DDR_EN_BIT;
			/* Also need to set DDR_EN bit */
			__fallthrough;
		case MSPI_DATA_RATE_S_D_D:
			dev_data->spi_ctrlr0 |= SPI_CTRLR0_SPI_DDR_EN_BIT;
			break;
#endif
		default:
			LOG_ERR("Data rate %d not supported",
				cfg->data_rate);
			return -ENOTSUP;
		}
	}

	if (param_mask & MSPI_DEVICE_CONFIG_DQS) {
		dev_data->spi_ctrlr0 &= ~SPI_CTRLR0_SPI_RXDS_EN_BIT;

		if (cfg->dqs_enable) {
			dev_data->spi_ctrlr0 |= SPI_CTRLR0_SPI_RXDS_EN_BIT;
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
#if defined(CONFIG_MULTITHREADING)
		rc = k_sem_take(&dev_data->cfg_lock,
				K_MSEC(CONFIG_MSPI_COMPLETION_TIMEOUT_TOLERANCE));
#else
		if (dev_data->cfg_lock) {
			rc = -1;
		} else {
			dev_data->cfg_lock = true;
			rc = 0;
		}
#endif
		if (rc < 0) {
			LOG_ERR("Failed to switch controller to device");
			return -EBUSY;
		}

		dev_data->dev_id = dev_id;

#if defined(CONFIG_MULTITHREADING)
		memset(dev_data->cbs, 0, sizeof(dev_data->cbs));
#endif
	}

	if (param_mask == MSPI_DEVICE_CONFIG_NONE &&
	    !dev_config->sw_multi_periph) {
		return 0;
	}

#if defined(CONFIG_MULTITHREADING)
	(void)k_sem_take(&dev_data->ctx_lock, K_FOREVER);
#endif

	rc = _api_dev_config(dev, param_mask, cfg);

#if defined(CONFIG_MULTITHREADING)
	k_sem_give(&dev_data->ctx_lock);
#endif

	if (rc < 0) {
		dev_data->dev_id = NULL;
#if defined(CONFIG_MULTITHREADING)
		k_sem_give(&dev_data->cfg_lock);
#else
		dev_data->cfg_lock = false;
#endif
	}

	return rc;
}

static int api_get_channel_status(const struct device *dev, uint8_t ch)
{
	ARG_UNUSED(ch);

	struct mspi_dw_data *dev_data = dev->data;

#if defined(CONFIG_MULTITHREADING)
	(void)k_sem_take(&dev_data->ctx_lock, K_FOREVER);
#endif

	dev_data->dev_id = NULL;
#if defined(CONFIG_MULTITHREADING)
	k_sem_give(&dev_data->cfg_lock);
	k_sem_give(&dev_data->ctx_lock);
#else
	dev_data->cfg_lock = false;
#endif

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

static int start_next_packet(const struct device *dev)
{
	const struct mspi_dw_config *dev_config = dev->config;
	struct mspi_dw_data *dev_data = dev->data;
	const struct mspi_xfer_packet *packet =
		&dev_data->xfer.packets[dev_data->packets_done];
	bool xip_enabled = COND_CODE_1(CONFIG_MSPI_XIP,
				       (dev_data->xip_enabled != 0),
				       (false));
	unsigned int key;
	uint32_t packet_frames;
	uint32_t imr = 0;
	int rc = 0;

	if (packet->num_bytes == 0 &&
	    dev_data->xfer.cmd_length == 0 &&
	    dev_data->xfer.addr_length == 0) {
		return 0;
	}

	dev_data->dummy_bytes = 0;
	dev_data->bytes_to_discard = 0;

	dev_data->ctrlr0 &= ~(CTRLR0_TMOD_MASK)
			  & ~(CTRLR0_DFS_MASK)
			  & ~(CTRLR0_DFS32_MASK);

	dev_data->spi_ctrlr0 &= ~SPI_CTRLR0_WAIT_CYCLES_MASK;

	if (dev_data->standard_spi &&
	    (dev_data->xfer.cmd_length != 0 ||
	     dev_data->xfer.addr_length != 0)) {
		dev_data->bytes_per_frame_exp = 0;
		dev_data->ctrlr0 |= FIELD_PREP(CTRLR0_DFS_MASK, 7);
		dev_data->ctrlr0 |= FIELD_PREP(CTRLR0_DFS32_MASK, 7);
	} else {
		if ((packet->num_bytes % 4) == 0) {
			dev_data->bytes_per_frame_exp = 2;
			dev_data->ctrlr0 |= FIELD_PREP(CTRLR0_DFS_MASK, 31);
			dev_data->ctrlr0 |= FIELD_PREP(CTRLR0_DFS32_MASK, 31);
		} else if ((packet->num_bytes % 2) == 0) {
			dev_data->bytes_per_frame_exp = 1;
			dev_data->ctrlr0 |= FIELD_PREP(CTRLR0_DFS_MASK, 15);
			dev_data->ctrlr0 |= FIELD_PREP(CTRLR0_DFS32_MASK, 15);
		} else {
			dev_data->bytes_per_frame_exp = 0;
			dev_data->ctrlr0 |= FIELD_PREP(CTRLR0_DFS_MASK, 7);
			dev_data->ctrlr0 |= FIELD_PREP(CTRLR0_DFS32_MASK, 7);
		}
	}

	packet_frames = packet->num_bytes >> dev_data->bytes_per_frame_exp;

	if (packet_frames > UINT16_MAX + 1) {
		LOG_ERR("Packet length (%u) exceeds supported maximum",
			packet->num_bytes);
		return -EINVAL;
	}

#if defined(CONFIG_MSPI_DMA)
	if (dev_data->xfer.xfer_mode == MSPI_DMA) {
		/* Check if the packet buffer is accessible */
		if (packet->num_bytes > 0 &&
		    !vendor_specific_dma_accessible_check(dev, packet->data_buf)) {
			LOG_ERR("Buffer not DMA accessible: ptr=0x%lx, size=%u",
				(uintptr_t)packet->data_buf, packet->num_bytes);
			return -EINVAL;
		}
	}
#endif

	if (packet->dir == MSPI_TX || packet->num_bytes == 0) {
		imr = IMR_TXEIM_BIT;
		dev_data->ctrlr0 |= FIELD_PREP(CTRLR0_TMOD_MASK,
					       CTRLR0_TMOD_TX);
		dev_data->spi_ctrlr0 |= FIELD_PREP(SPI_CTRLR0_WAIT_CYCLES_MASK,
						   dev_data->xfer.tx_dummy);

		write_rxftlr(dev, 0);
#if defined(CONFIG_MSPI_DMA)
	} else if (dev_data->xfer.xfer_mode == MSPI_DMA) {
		dev_data->ctrlr0 |= FIELD_PREP(CTRLR0_TMOD_MASK, CTRLR0_TMOD_RX);
		dev_data->spi_ctrlr0 |= FIELD_PREP(SPI_CTRLR0_WAIT_CYCLES_MASK,
							dev_data->xfer.rx_dummy);
#endif
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
	write_rx_sample_dly(dev, dev_data->rx_sample_dly);
#if defined(CONFIG_MSPI_DW_DDR)
	if (dev_data->spi_ctrlr0 & (SPI_CTRLR0_SPI_DDR_EN_BIT |
				    SPI_CTRLR0_INST_DDR_EN_BIT)) {
		int txd = (CONFIG_MSPI_DW_TXD_MUL * dev_data->baudr) /
			CONFIG_MSPI_DW_TXD_DIV;

		write_txd_drive_edge(dev, txd);
	} else {
		write_txd_drive_edge(dev, 0);
	}
#endif

	if (xip_enabled) {
		write_ssienr(dev, SSIENR_SSIC_EN_BIT);
		irq_unlock(key);
	}

#if defined(CONFIG_MSPI_DMA)
	if (dev_data->xfer.xfer_mode == MSPI_DMA) {
		/* For DMA mode, set start level based on transfer length to prevent underflow */
		uint32_t total_transfer_bytes = packet->num_bytes + dev_data->xfer.addr_length +
						dev_data->xfer.cmd_length;
		uint32_t transfer_frames = total_transfer_bytes >> dev_data->bytes_per_frame_exp;

		/* Use minimum of transfer length or FIFO depth, but at least 1 */
		uint8_t dma_start_level = MIN(transfer_frames - 1,
					      dev_config->tx_fifo_depth_minus_1);

		dma_start_level = (dma_start_level > 0 ? dma_start_level : 1);

		/* Only TXFTHR needs to be set to the minimum number of frames */
		write_txftlr(dev, FIELD_PREP(TXFTLR_TXFTHR_MASK, dma_start_level));
		write_dmatdlr(dev, FIELD_PREP(DMATDLR_DMATDL_MASK, dev_config->dma_tx_data_level));
		write_dmardlr(dev, FIELD_PREP(DMARDLR_DMARDL_MASK, dev_config->dma_rx_data_level));
		write_dmacr(dev, DMACR_TDMAE_BIT | DMACR_RDMAE_BIT);
		write_imr(dev, 0);
		write_ssienr(dev, SSIENR_SSIC_EN_BIT);

		vendor_specific_start_dma_xfer(dev);
	} else {
#endif
		/* PIO mode */
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

		/* Prefill TX FIFO with any data we can */
		if (dev_data->dummy_bytes && tx_dummy_bytes(dev, NULL)) {
			imr = IMR_RXFIM_BIT;
		} else if (packet->dir == MSPI_TX && packet->num_bytes) {
			tx_data(dev, packet);
		}

		/* Enable interrupts now and wait until the packet is done unless async. */
		write_imr(dev, imr);
#if defined(CONFIG_MSPI_DMA)
	}
#endif

	/* Write SER to start transfer */
	write_ser(dev, BIT(dev_data->dev_id->dev_idx));

#if defined(CONFIG_MULTITHREADING)
	k_timeout_t timeout = K_MSEC(dev_data->xfer.timeout);

	/* For async transfer, start the timeout timer and exit. */
	if (dev_data->xfer.async) {
		k_timer_start(&dev_data->async_timer, timeout, K_NO_WAIT);

		return 0;
	}

	/* For sync transfer, wait until the packet is finished. */
	rc = k_sem_take(&dev_data->finished, timeout);
	if (rc < 0) {
		rc = -ETIMEDOUT;
	}
#else
	if (!WAIT_FOR(dev_data->finished,
		      dev_data->xfer.timeout * USEC_PER_MSEC,
		      NULL)) {
		rc = -ETIMEDOUT;
	}

	dev_data->finished = false;
#endif

	return finalize_packet(dev, rc);
}

static int finalize_packet(const struct device *dev, int rc)
{
	struct mspi_dw_data *dev_data = dev->data;
	bool xip_enabled = COND_CODE_1(CONFIG_MSPI_XIP,
				       (dev_data->xip_enabled != 0),
				       (false));

	if (read_risr(dev) & RISR_RXOIR_BIT) {
		LOG_ERR("RX FIFO overflow occurred");
		rc = -EIO;
	} else if (rc == -ETIMEDOUT) {
		LOG_ERR("Transfer timed out");
	}

	/* Disable the controller. This will immediately halt the transfer
	 * if it hasn't finished yet.
	 */
	if (xip_enabled) {
		/* If XIP is enabled, the controller must be kept enabled,
		 * so disable it only momentarily if there's a need to halt
		 * a transfer that ended up with an error.
		 */
		if (rc < 0) {
			unsigned int key = irq_lock();

			write_ssienr(dev, 0);
			write_ssienr(dev, SSIENR_SSIC_EN_BIT);

			irq_unlock(key);
		}
	} else {
		write_ssienr(dev, 0);
	}

	/* Clear SER */
	write_ser(dev, 0);

	if (dev_data->dev_id->ce.port) {
		int rc2;

		/* Do not use `rc` to not overwrite potential packet error. */
		rc2 = gpio_pin_set_dt(&dev_data->dev_id->ce, 0);
		if (rc2 < 0) {
			LOG_ERR("Failed to deactivate CE line (%d)", rc2);
			return rc2;
		}
	}

	return rc;
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

	/* For async, only the first packet is started here, next ones, if any,
	 * are started by ISR.
	 */
	if (req->async) {
		dev_data->packets_done = 0;
		return start_next_packet(dev);
	}

	for (dev_data->packets_done = 0;
	     dev_data->packets_done < dev_data->xfer.num_packet;
	     dev_data->packets_done++) {
		rc = start_next_packet(dev);
		if (rc < 0) {
			return rc;
		}
	}

	return 0;
}

static int api_transceive(const struct device *dev,
			  const struct mspi_dev_id *dev_id,
			  const struct mspi_xfer *req)
{
	struct mspi_dw_data *dev_data = dev->data;
	int rc;

	if (dev_id != dev_data->dev_id) {
		LOG_ERR("Controller is not configured for this device");
		return -EINVAL;
	}

	if (req->async && !IS_ENABLED(CONFIG_MULTITHREADING)) {
		LOG_ERR("Asynchronous transfers require multithreading");
		return -ENOTSUP;
	}

	rc = pm_device_runtime_get(dev);
	if (rc < 0) {
		LOG_ERR("pm_device_runtime_get() failed: %d", rc);
		return rc;
	}

#if defined(CONFIG_MULTITHREADING)
	(void)k_sem_take(&dev_data->ctx_lock, K_FOREVER);
#endif

	if (dev_data->suspended) {
		rc = -EFAULT;
	} else {
		rc = _api_transceive(dev, req);
	}

	if (req->async && rc >= 0) {
		return rc;
	}

	return finalize_transceive(dev,  rc);
}

static int finalize_transceive(const struct device *dev, int rc)
{
	int rc2;

#if defined(CONFIG_MULTITHREADING)
	struct mspi_dw_data *dev_data = dev->data;

	k_sem_give(&dev_data->ctx_lock);
#endif

	rc2 = pm_device_runtime_put(dev);
	if (rc2 < 0) {
		LOG_ERR("pm_device_runtime_put() failed: %d", rc2);
		rc = (rc < 0 ? rc : rc2);
	}

	return rc;
}

#if defined(CONFIG_MULTITHREADING)
static int api_register_callback(const struct device *dev,
				 const struct mspi_dev_id *dev_id,
				 const enum mspi_bus_event evt_type,
				 mspi_callback_handler_t cb,
				 struct mspi_callback_context *ctx)
{
	struct mspi_dw_data *dev_data = dev->data;

	if (dev_id != dev_data->dev_id) {
		LOG_ERR("Controller is not configured for this device");
		return -EINVAL;
	}

	if (evt_type != MSPI_BUS_ERROR &&
	    evt_type != MSPI_BUS_XFER_COMPLETE &&
	    evt_type != MSPI_BUS_TIMEOUT) {
		LOG_ERR("Callback type %d not supported", evt_type);
		return -ENOTSUP;
	}

	dev_data->cbs[evt_type] = cb;
	dev_data->cb_ctxs[evt_type] = ctx;
	return 0;
}
#endif /* defined(CONFIG_MULTITHREADING) */

#if defined(CONFIG_MSPI_TIMING)
static int api_timing_config(const struct device *dev,
			     const struct mspi_dev_id *dev_id,
			     const uint32_t param_mask, void *cfg)
{
	struct mspi_dw_data *dev_data = dev->data;
	struct mspi_dw_timing_cfg *config = cfg;

	if (param_mask & MSPI_DW_RX_TIMING_CFG) {
		dev_data->rx_sample_dly = config->rx_sample_dly;
		return 0;
	}
	return -ENOTSUP;
}
#endif /* defined(CONFIG_MSPI_TIMING) */

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

#if defined(CONFIG_MULTITHREADING)
	(void)k_sem_take(&dev_data->ctx_lock, K_FOREVER);
#endif

	if (dev_data->suspended) {
		rc = -EFAULT;
	} else {
		rc = _api_xip_config(dev, dev_id, cfg);
	}

#if defined(CONFIG_MULTITHREADING)
	k_sem_give(&dev_data->ctx_lock);
#endif

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
		vendor_specific_resume(dev);

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
#if defined(CONFIG_MULTITHREADING)
		if (xip_enabled ||
		    k_sem_take(&dev_data->ctx_lock, K_NO_WAIT) != 0) {
#else
		if (xip_enabled) {
#endif
			LOG_ERR("Controller in use, cannot be suspended");
			return -EBUSY;
		}

		dev_data->suspended = true;

		vendor_specific_suspend(dev);

#if defined(CONFIG_MULTITHREADING)
		k_sem_give(&dev_data->ctx_lock);
#endif

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

	vendor_specific_init(dev);

	dev_data->ctrlr0 |= FIELD_PREP(CTRLR0_SSI_IS_MST_BIT,
				       dev_config->op_mode == MSPI_OP_MODE_CONTROLLER);

	dev_config->irq_config();

#if defined(CONFIG_MULTITHREADING)
	dev_data->dev = dev;
	k_sem_init(&dev_data->finished, 0, 1);
	k_sem_init(&dev_data->cfg_lock, 1, 1);
	k_sem_init(&dev_data->ctx_lock, 1, 1);

	k_timer_init(&dev_data->async_timer, async_timeout_timer_handler, NULL);
	k_work_init(&dev_data->async_timeout_work, async_timeout_work_handler);
	k_work_init(&dev_data->async_packet_work, async_packet_work_handler);
#endif

#if defined(CONFIG_MSPI_DW_HANDLE_FIFOS_IN_SYSTEM_WORKQUEUE)
	k_work_init(&dev_data->fifo_work, fifo_work_handler);
#endif

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

	rc = pm_device_driver_init(dev, dev_pm_action_cb);
	if (rc < 0) {
		return rc;
	}

	/* Make sure controller is disabled. */
	write_ssienr(dev, 0);

	return 0;
}

static DEVICE_API(mspi, drv_api) = {
	.config             = api_config,
	.dev_config         = api_dev_config,
	.get_channel_status = api_get_channel_status,
	.transceive         = api_transceive,
#if defined(CONFIG_MULTITHREADING)
	.register_callback  = api_register_callback,
#endif
#if defined(CONFIG_MSPI_TIMING)
	.timing_config      = api_timing_config,
#endif
#if defined(CONFIG_MSPI_XIP)
	.xip_config         = api_xip_config,
#endif
};

#define MSPI_DW_INST_IRQ(idx, inst)					\
	IRQ_CONNECT(DT_INST_IRQN_BY_IDX(inst, idx),			\
		    DT_INST_IRQ_BY_IDX(inst, idx, priority),		\
		    mspi_dw_isr, DEVICE_DT_INST_GET(inst), 0);		\
	irq_enable(DT_INST_IRQN_BY_IDX(inst, idx))

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
				1 * RX_FIFO_DEPTH(inst) / 8 - 1)

#define MSPI_DW_DMA_DATA_LEVELS(inst)					\
	.dma_tx_data_level =						\
		DT_INST_PROP_OR(inst, dma_transmit_data_level, 0),	\
	.dma_rx_data_level =						\
		DT_INST_PROP_OR(inst, dma_receive_data_level, 0)

#define MSPI_DW_INST(inst)						\
	PM_DEVICE_DT_INST_DEFINE(inst, dev_pm_action_cb);		\
	IF_ENABLED(CONFIG_PINCTRL, (PINCTRL_DT_INST_DEFINE(inst);))	\
	VENDOR_SPECIFIC_DATA_DEFINE(inst);				\
	static void irq_config##inst(void)				\
	{								\
		LISTIFY(DT_INST_NUM_IRQS(inst),				\
			MSPI_DW_INST_IRQ, (;), inst);			\
	}								\
	static struct mspi_dw_data dev##inst##_data;			\
	static const struct mspi_dw_config dev##inst##_config = {	\
		MSPI_DW_MMIO_ROM_INIT(DT_DRV_INST(inst)),		\
		.wrapper_regs = (void *)DT_INST_REG_ADDR(inst),          \
		.irq_config = irq_config##inst,				\
		.clock_frequency = MSPI_DW_CLOCK_FREQUENCY(inst),	\
	IF_ENABLED(CONFIG_PINCTRL,					\
		(.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),))	\
	IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, ce_gpios),		\
		(MSPI_DW_CE_GPIOS(inst),))				\
		MSPI_DW_FIFO_PROPS(inst),				\
	IF_ENABLED(CONFIG_MSPI_DMA, (MSPI_DW_DMA_DATA_LEVELS(inst),))	\
		.vendor_specific_data = VENDOR_SPECIFIC_DATA_GET(inst),	\
		DEFINE_REG_ACCESS(inst)					\
		.sw_multi_periph =					\
			DT_INST_PROP(inst, software_multiperipheral),	\
		.op_mode = DT_INST_STRING_TOKEN(inst, op_mode),		\
	};								\
	DEVICE_DT_INST_DEFINE(inst,					\
		dev_init, PM_DEVICE_DT_INST_GET(inst),			\
		&dev##inst##_data, &dev##inst##_config,			\
		POST_KERNEL, CONFIG_MSPI_INIT_PRIORITY,			\
		&drv_api);

DT_INST_FOREACH_STATUS_OKAY(MSPI_DW_INST)
