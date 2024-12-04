/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT snps_designware_ssi

#include <zephyr/drivers/mspi.h>
#include <zephyr/drivers/gpio.h>
#ifdef CONFIG_PINCTRL
#include <zephyr/drivers/pinctrl.h>
#endif
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>

#include "mspi_dw.h"
#include "mspi_dw_vendor_specific.h"

LOG_MODULE_REGISTER(mspi_dw, CONFIG_MSPI_LOG_LEVEL);

#define INVALID_DEV_IDX 0xFFFF
#define DUMMY_BYTE 0xAA

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

struct mspi_dw_data {
	uint32_t packets_done;
	uint8_t *buf_pos;
	const uint8_t *buf_end;

	uint32_t ctrlr0;
	uint32_t spi_ctrlr0;
	uint32_t baudr;

	uint32_t xip_freq;
	struct xip_params xip_params_stored;
	struct xip_params xip_params_active;
	uint16_t xip_enabled;
	enum mspi_cpp_mode xip_cpp;

	uint16_t dummy_bytes;
	uint8_t bytes_to_discard;
	uint8_t bytes_per_frame;
	bool standard_spi;

	struct k_sem finished;
	struct mspi_dev_id dev_id;
	struct mspi_xfer xfer;
};

struct mspi_dw_config {
	DEVICE_MMIO_ROM;
	void (*irq_config)(void);
	uint32_t clock_frequency;
#ifdef CONFIG_PINCTRL
	const struct pinctrl_dev_config *pcfg;
#endif
	const struct gpio_dt_spec *ce_gpios;
	uint8_t ce_gpios_len;
	uint8_t tx_fifo_depth;
	uint8_t rx_fifo_depth;
	uint8_t tx_fifo_threshold;
	uint8_t rx_fifo_threshold;
	DECLARE_REG_ACCESS();
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
DEFINE_MM_REG_RD_WR(dr,		0x60)
DEFINE_MM_REG_WR(spi_ctrlr0,	0xf4)

DEFINE_MM_REG_WR(xip_incr_inst,		0x100)
DEFINE_MM_REG_WR(xip_wrap_inst,		0x104)
DEFINE_MM_REG_WR(xip_ctrl,		0x108)
DEFINE_MM_REG_WR(xip_write_incr_inst,	0x140)
DEFINE_MM_REG_WR(xip_write_wrap_inst,	0x144)
DEFINE_MM_REG_WR(xip_write_ctrl,	0x148)

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
	uint8_t bytes_per_frame = dev_data->bytes_per_frame;
	uint8_t tx_fifo_depth = dev_config->tx_fifo_depth;
	uint32_t data;

	do {
		if (bytes_per_frame == 4) {
			data = buf_pos[0] << 24
			     | buf_pos[1] << 16
			     | buf_pos[2] << 8
			     | buf_pos[3] << 0;
			buf_pos += 4;
		} else if (bytes_per_frame == 2) {
			data = buf_pos[0] << 8
			     | buf_pos[1] << 0;
			buf_pos += 2;
		} else {
			data = buf_pos[0];
			buf_pos += 1;
		}
		write_dr(dev, data);

		if (buf_pos >= buf_end) {
			write_txftlr(dev, 0);
			break;
		}

		if (!--room) {
			room = tx_fifo_depth
			     - FIELD_GET(TXFLR_TXTFL_MASK, read_txflr(dev));
		}
	} while (room);

	dev_data->buf_pos = (uint8_t *)buf_pos;
}

static bool make_rx_cycles(const struct device *dev)
{
	struct mspi_dw_data *dev_data = dev->data;
	const struct mspi_dw_config *dev_config = dev->config;
	uint16_t dummy_bytes = dev_data->dummy_bytes;
	/* See tx_data(). */
	uint32_t room = 1;
	uint8_t tx_fifo_depth = dev_config->tx_fifo_depth;

	do {
		write_dr(dev, DUMMY_BYTE);

		--dummy_bytes;
		if (!dummy_bytes) {
			dev_data->dummy_bytes = 0;
			return true;
		}

		if (!--room) {
			room = tx_fifo_depth
			     - FIELD_GET(TXFLR_TXTFL_MASK, read_txflr(dev));
		}
	} while (room);

	dev_data->dummy_bytes = dummy_bytes;
	return false;
}

static void read_rx_fifo(const struct device *dev,
			 const struct mspi_xfer_packet *packet)
{
	struct mspi_dw_data *dev_data = dev->data;
	const struct mspi_dw_config *dev_config = dev->config;
	uint8_t bytes_to_discard = dev_data->bytes_to_discard;
	uint8_t *buf_pos = dev_data->buf_pos;
	const uint8_t *buf_end = &packet->data_buf[packet->num_bytes];
	uint8_t bytes_per_frame = dev_data->bytes_per_frame;
	/* See `room` in tx_data(). */
	uint32_t in_fifo = 1;
	uint32_t remaining_bytes;

	do {
		uint32_t data = read_dr(dev);

		if (bytes_to_discard) {
			--bytes_to_discard;
		} else {
			if (bytes_per_frame == 4) {
				buf_pos[0] = (uint8_t)(data >> 24);
				buf_pos[1] = (uint8_t)(data >> 16);
				buf_pos[2] = (uint8_t)(data >> 8);
				buf_pos[3] = (uint8_t)(data >> 0);
				buf_pos += 4;
			} else if (bytes_per_frame == 2) {
				buf_pos[0] = (uint8_t)(data >> 8);
				buf_pos[1] = (uint8_t)(data >> 0);
				buf_pos += 2;
			} else {
				*buf_pos = (uint8_t)data;
				buf_pos += 1;
			}

			if (buf_pos >= buf_end) {
				dev_data->bytes_to_discard = bytes_to_discard;
				dev_data->buf_pos = buf_pos;
				return;
			}
		}

		if (!--in_fifo) {
			in_fifo = FIELD_GET(RXFLR_RXTFL_MASK, read_rxflr(dev));
		}
	} while (in_fifo);

	remaining_bytes = bytes_to_discard + buf_end - buf_pos;
	if (remaining_bytes - 1 < dev_config->rx_fifo_threshold) {
		write_rxftlr(dev, remaining_bytes - 1);
	}

	dev_data->bytes_to_discard = bytes_to_discard;
	dev_data->buf_pos = buf_pos;
}

static void mspi_dw_isr(const struct device *dev)
{
	struct mspi_dw_data *dev_data = dev->data;
	const struct mspi_xfer_packet *packet =
		&dev_data->xfer.packets[dev_data->packets_done];
	uint32_t int_status = read_isr(dev);

	if (int_status & ISR_RXFIS_BIT) {
		read_rx_fifo(dev, packet);
	}

	if (dev_data->buf_pos >= dev_data->buf_end) {
		write_imr(dev, 0);
		/* It may happen that at this point the controller is still
		 * shifting out the last frame (the last interrupt occurs when
		 * the TX FIFO is empty). Wait if it signals that it is busy.
		 */
		while (read_sr(dev) & SR_BUSY_BIT) {
		}

		k_sem_give(&dev_data->finished);
	} else {
		if (int_status & ISR_TXEIS_BIT) {
			if (dev_data->dummy_bytes) {
				if (make_rx_cycles(dev)) {
					write_imr(dev, IMR_RXFIM_BIT);
				}
			} else {
				tx_data(dev, packet);
			}
		}
	}

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
		LOG_ERR("Command length %d not supported", cmd_length);
		return false;
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
		LOG_ERR("Command length %d not supported", cmd_length);
		return false;
	}

	return true;
}

static bool apply_addr_length(struct mspi_dw_data *dev_data,
			      uint32_t addr_length)
{
	dev_data->spi_ctrlr0 |= FIELD_PREP(SPI_CTRLR0_ADDR_L_MASK,
					   addr_length * 2);

	return true;
}

static bool apply_xip_addr_length(const struct mspi_dw_data *dev_data,
				  struct xip_ctrl *ctrl)
{
	uint8_t addr_length = dev_data->xip_params_active.addr_length;

	ctrl->read |= FIELD_PREP(XIP_CTRL_ADDR_L_MASK, addr_length * 2);
	ctrl->write |= FIELD_PREP(XIP_WRITE_CTRL_ADDR_L_MASK, addr_length * 2);

	return true;
}

static int api_dev_config(const struct device *dev,
			  const struct mspi_dev_id *dev_id,
			  const enum mspi_dev_cfg_mask param_mask,
			  const struct mspi_dev_cfg *cfg)
{
	const struct mspi_dw_config *dev_config = dev->config;
	struct mspi_dw_data *dev_data = dev->data;
	bool xip_enabled = dev_data->xip_enabled != 0;
	unsigned int key;
	uint32_t baudr = 0;

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
		dev_data->xip_params_stored.io_mode = cfg->io_mode;

		if (!apply_io_mode(dev_data, cfg->io_mode)) {
			return -EINVAL;
		}
	}

	if (param_mask & MSPI_DEVICE_CONFIG_CPP) {
		/* Make sure the new setting is compatible with the one used
		 * for XIP if it is enabled.
		 */
		if (!dev_data->xip_enabled) {
			dev_data->xip_cpp = cfg->cpp;
		} else if (dev_data->xip_cpp != cfg->cpp) {
			LOG_ERR("Conflict with configuration used for XIP.");
			return -EINVAL;
		}

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

		/* Make sure the new setting is compatible with the one used
		 * for XIP if it is enabled.
		 */
		if (!dev_data->xip_enabled) {
			dev_data->xip_freq = cfg->freq;
		} else if (dev_data->xip_freq != cfg->freq) {
			LOG_ERR("Conflict with configuration used for XIP.");
			return -EINVAL;
		}

		baudr = dev_config->clock_frequency / cfg->freq;
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
			return -ENOTSUP;
		}
	}

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

	/* Always use Motorola SPI frame format. */
	dev_data->ctrlr0 |= FIELD_PREP(CTRLR0_FRF_MASK, CTRLR0_FRF_SPI);
	/* Enable clock stretching. */
	dev_data->spi_ctrlr0 |= SPI_CTRLR0_CLK_STRETCH_EN_BIT;

	if (xip_enabled) {
		key = irq_lock();
		write_ssienr(dev, 0);
	}

	if (baudr) {
		dev_data->baudr = baudr;
		write_baudr(dev, dev_data->baudr);
	}
	write_ctrlr0(dev, dev_data->ctrlr0);
	write_spi_ctrlr0(dev, dev_data->spi_ctrlr0);
	write_ser(dev, BIT(dev_id->dev_idx));

	if (xip_enabled) {
		write_ssienr(dev, SSIENR_SSIC_EN_BIT);
		irq_unlock(key);
	}

	dev_data->dev_id = *dev_id;

	return 0;
}

static int api_get_channel_status(const struct device *dev, uint8_t ch)
{
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

static int start_next_packet(const struct device *dev, k_timeout_t timeout)
{
	const struct mspi_dw_config *dev_config = dev->config;
	struct mspi_dw_data *dev_data = dev->data;
	const struct mspi_xfer_packet *packet =
		&dev_data->xfer.packets[dev_data->packets_done];
	bool xip_enabled = dev_data->xip_enabled != 0;
	unsigned int key;
	uint8_t tx_fifo_threshold;
	uint32_t imr;
	int rc = 0;

	if (packet->num_bytes == 0 &&
	    dev_data->xfer.cmd_length == 0 &&
	    dev_data->xfer.addr_length == 0) {
		return 0;
	}

	/* TODO: support longer transfers */
	if (packet->num_bytes >= UINT16_MAX) {
		return -EINVAL;
	}

	dev_data->dummy_bytes = 0;

	dev_data->ctrlr0 &= ~CTRLR0_TMOD_MASK
			 &  ~CTRLR0_DFS_MASK;

	dev_data->spi_ctrlr0 &= ~SPI_CTRLR0_WAIT_CYCLES_MASK;

	/* TODO: support receiving data in bigger frames when possible */
	if (dev_data->standard_spi || packet->dir == MSPI_RX) {
		dev_data->bytes_per_frame = 1;
		dev_data->ctrlr0 |= FIELD_PREP(CTRLR0_DFS_MASK, 7);
	} else {
		if ((packet->num_bytes % 4) == 0) {
			dev_data->bytes_per_frame = 4;
			dev_data->ctrlr0 |= FIELD_PREP(CTRLR0_DFS_MASK, 31);
		} else if ((packet->num_bytes % 2) == 0) {
			dev_data->bytes_per_frame = 2;
			dev_data->ctrlr0 |= FIELD_PREP(CTRLR0_DFS_MASK, 15);
		} else {
			dev_data->bytes_per_frame = 1;
			dev_data->ctrlr0 |= FIELD_PREP(CTRLR0_DFS_MASK, 7);
		}
	}

	if (packet->dir == MSPI_TX || packet->num_bytes == 0) {
		imr = IMR_TXEIM_BIT;
		dev_data->ctrlr0 |= FIELD_PREP(CTRLR0_TMOD_MASK,
					       CTRLR0_TMOD_TX);
		dev_data->spi_ctrlr0 |= FIELD_PREP(SPI_CTRLR0_WAIT_CYCLES_MASK,
						   dev_data->xfer.tx_dummy);

		write_rxftlr(dev, 0);
		tx_fifo_threshold = dev_config->tx_fifo_threshold;
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
			uint32_t rx_total;

			dev_data->bytes_to_discard = dev_data->xfer.cmd_length
						   + dev_data->xfer.addr_length;
			rx_total = dev_data->bytes_to_discard
				 + packet->num_bytes;

			dev_data->dummy_bytes = packet->num_bytes;

			imr = IMR_TXEIM_BIT | IMR_RXFIM_BIT;
			tmod = CTRLR0_TMOD_TX_RX;
			tx_fifo_threshold = dev_config->tx_fifo_threshold;
			rx_fifo_threshold = MIN(rx_total - 1,
						dev_config->rx_fifo_threshold);
		} else {
			imr = IMR_RXFIM_BIT;
			tmod = CTRLR0_TMOD_RX;
			tx_fifo_threshold = 0;
			rx_fifo_threshold = MIN(packet->num_bytes - 1,
						dev_config->rx_fifo_threshold);
		}

		dev_data->ctrlr0 |= FIELD_PREP(CTRLR0_TMOD_MASK, tmod);
		dev_data->spi_ctrlr0 |= FIELD_PREP(SPI_CTRLR0_WAIT_CYCLES_MASK,
						   dev_data->xfer.rx_dummy);

		write_rxftlr(dev, FIELD_PREP(RXFTLR_RFT_MASK,
					     rx_fifo_threshold));
	}

	if (xip_enabled) {
		key = irq_lock();
		write_ssienr(dev, 0);
	}

	write_ctrlr0(dev, dev_data->ctrlr0);
	write_ctrlr1(dev, packet->num_bytes > 0
		? FIELD_PREP(CTRLR1_NDF_MASK,
			     packet->num_bytes / dev_data->bytes_per_frame - 1)
		: 0);
	write_spi_ctrlr0(dev, dev_data->spi_ctrlr0);

	if (xip_enabled) {
		write_ssienr(dev, SSIENR_SSIC_EN_BIT);
		irq_unlock(key);
	}

	if (dev_data->dev_id.ce.port) {
		gpio_pin_set_dt(&dev_data->dev_id.ce, 1);
	}

	dev_data->buf_pos = packet->data_buf;
	dev_data->buf_end = &packet->data_buf[packet->num_bytes];

	if ((imr & IMR_TXEIM_BIT) && dev_data->buf_pos < dev_data->buf_end) {
		uint32_t start_level = tx_fifo_threshold;

		if (dev_data->dummy_bytes) {
			uint32_t tx_total = dev_data->bytes_to_discard
					  + dev_data->dummy_bytes;

			if (start_level > tx_total - 1) {
				start_level = tx_total - 1;
			}
		}

		write_txftlr(dev,
			FIELD_PREP(TXFTLR_TXFTHR_MASK, start_level) |
			FIELD_PREP(TXFTLR_TFT_MASK, tx_fifo_threshold));
	} else {
		write_txftlr(dev, 0);
	}

	/* Ensure that there will be no interrupt from the controller yet. */
	write_imr(dev, 0);
	/* Enable the controller. This must be done before DR is written. */
	write_ssienr(dev, SSIENR_SSIC_EN_BIT);

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

	if (dev_data->dummy_bytes) {
		if (make_rx_cycles(dev)) {
			imr = IMR_RXFIM_BIT;
		}
	} else if (packet->dir == MSPI_TX && packet->num_bytes) {
		tx_data(dev, packet);
	}

	/* Enable interrupts now and wait until the packet is done. */
	write_imr(dev, imr);

	rc = k_sem_take(&dev_data->finished, timeout);
	if (rc < 0) {
		rc = -ETIMEDOUT;
	}

	/* Disable the controller. This will immediately halt the transfer
	 * if it hasn't finished yet.
	 */
	if (dev_data->xip_enabled) {
		key = irq_lock();

		write_ssienr(dev, 0);
		write_ssienr(dev, SSIENR_SSIC_EN_BIT);

		irq_unlock(key);
	} else {
		write_ssienr(dev, 0);
	}

	if (dev_data->dev_id.ce.port) {
		gpio_pin_set_dt(&dev_data->dev_id.ce, 0);
	}

	return rc;
}

static int api_transceive(const struct device *dev,
			  const struct mspi_dev_id *dev_id,
			  const struct mspi_xfer *req)
{
	struct mspi_dw_data *dev_data = dev->data;
	int rc;

	if (dev_id->dev_idx != dev_data->dev_id.dev_idx) {
		return -EINVAL;
	}

	/* TODO: add support for asynchronous transfers */
	if (req->async) {
		return -ENOTSUP;
	}

	dev_data->spi_ctrlr0 &= ~SPI_CTRLR0_WAIT_CYCLES_MASK
			     &  ~SPI_CTRLR0_INST_L_MASK
			     &  ~SPI_CTRLR0_ADDR_L_MASK;

	if (!apply_cmd_length(dev_data, req->cmd_length) ||
	    !apply_addr_length(dev_data, req->addr_length)) {
		return -EINVAL;
	}

	if (dev_data->standard_spi) {
		if ((req->rx_dummy % 8) || (req->tx_dummy % 8)) {
			return -EINVAL;
		}
	} else {
		if (req->rx_dummy > SPI_CTRLR0_WAIT_CYCLES_MAX ||
		    req->tx_dummy > SPI_CTRLR0_WAIT_CYCLES_MAX) {
			return -EINVAL;
		}
	}

	dev_data->xfer = *req;

	for (dev_data->packets_done = 0;
	     dev_data->packets_done < dev_data->xfer.num_packet;
	     dev_data->packets_done++) {
		rc = start_next_packet(dev, K_MSEC(dev_data->xfer.timeout));
		if (rc < 0) {
			return rc;
		}
	}

	return 0;
}

static int api_xip_config(const struct device *dev,
			  const struct mspi_dev_id *dev_id,
			  const struct mspi_xip_cfg *cfg)
{
	struct mspi_dw_data *dev_data = dev->data;
	int rc;

	if (dev_id->dev_idx != dev_data->dev_id.dev_idx) {
		return -EINVAL;
	}

	if (!cfg->enable) {
		rc = vendor_specific_xip_disable(dev, dev_id, cfg);
		if (rc < 0) {
			return rc;
		}

		dev_data->xip_enabled &= ~BIT(dev_id->dev_idx);

		if (!dev_data->xip_enabled) {
			write_ssienr(dev, 0);
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

		ctrl.read |= FIELD_PREP(XIP_CTRL_WAIT_CYCLES_MASK,
					params->rx_dummy);
		ctrl.write |= FIELD_PREP(XIP_WRITE_CTRL_WAIT_CYCLES_MASK,
					 params->tx_dummy);

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

static int dev_pm_action_cb(const struct device *dev,
			    enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		break;
	case PM_DEVICE_ACTION_RESUME:
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int dev_init(const struct device *dev)
{
	struct mspi_dw_data *dev_data = dev->data;
	const struct mspi_dw_config *dev_config = dev->config;
	const struct gpio_dt_spec *ce_gpio;
	int rc;

#ifdef CONFIG_PINCTRL
	rc = pinctrl_apply_state(dev_config->pcfg, PINCTRL_STATE_DEFAULT);
	if (rc < 0) {
		return rc;
	}
#endif

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	vendor_specific_init(dev);

	dev_config->irq_config();

	dev_data->dev_id.dev_idx = INVALID_DEV_IDX;
	k_sem_init(&dev_data->finished, 0, 1);

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

	return pm_device_driver_init(dev, dev_pm_action_cb);
}

static const struct mspi_driver_api drv_api = {
	.config             = api_config,
	.dev_config         = api_dev_config,
	.get_channel_status = api_get_channel_status,
	.transceive         = api_transceive,
	.xip_config         = api_xip_config,
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
	.tx_fifo_depth = TX_FIFO_DEPTH(inst),				\
	.rx_fifo_depth = RX_FIFO_DEPTH(inst),				\
	.tx_fifo_threshold =						\
		DT_INST_PROP_OR(inst, tx_fifo_threshold,		\
				7 * TX_FIFO_DEPTH(inst) / 8 - 1),	\
	.rx_fifo_threshold =						\
		DT_INST_PROP_OR(inst, rx_fifo_threshold,		\
				1 * RX_FIFO_DEPTH(inst) / 8 - 1)

#define MSPI_DW_INST(inst)						\
	PM_DEVICE_DT_INST_DEFINE(inst, dev_pm_action_cb);		\
	IF_ENABLED(CONFIG_PINCTRL, (PINCTRL_DT_INST_DEFINE(inst);))	\
	static void irq_config##inst(void)				\
	{								\
		LISTIFY(DT_NUM_IRQS(DT_DRV_INST(inst)),			\
			MSPI_DW_INST_IRQ, (;), inst);			\
	}								\
	static struct mspi_dw_data dev##inst##_data;			\
	static const struct mspi_dw_config dev##inst##_config = {	\
		MSPI_DW_MMIO_ROM_INIT(DT_DRV_INST(inst)),		\
		.irq_config = irq_config##inst,				\
		.clock_frequency = MSPI_DW_CLOCK_FREQUENCY(inst),	\
	IF_ENABLED(CONFIG_PINCTRL,					\
		(.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),))	\
	IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, ce_gpios),		\
		(MSPI_DW_CE_GPIOS(inst),))				\
		MSPI_DW_FIFO_PROPS(inst),				\
		DEFINE_REG_ACCESS(inst)					\
	};								\
	DEVICE_DT_INST_DEFINE(inst,					\
		dev_init, PM_DEVICE_DT_INST_GET(inst),			\
		&dev##inst##_data, &dev##inst##_config,			\
		POST_KERNEL, CONFIG_MSPI_INIT_PRIORITY,			\
		&drv_api);

DT_INST_FOREACH_STATUS_OKAY(MSPI_DW_INST)
