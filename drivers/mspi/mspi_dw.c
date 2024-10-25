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

LOG_MODULE_REGISTER(mspi_dw, CONFIG_MSPI_LOG_LEVEL);

#include "mspi_dw.h"

struct mspi_dw_data {
	struct mspi_xfer xfer;
	struct mspi_dev_id dev_id;
	uint32_t packets_done;
	uint16_t bytes_done;
	uint16_t rx_tx_cnt;
	uint8_t to_discard;
	uint16_t rx_dummy;
	uint16_t tx_dummy;
	uint32_t read_cmd;
	uint32_t write_cmd;
	uint8_t cmd_length;
	uint8_t addr_length;

	bool standard_spi;
	struct k_sem finished;
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

	uint32_t (*read)(mm_reg_t addr, uint32_t off);
	void (*write)(uint32_t data, mm_reg_t addr, uint32_t off);
};

/* Register access helpers. */
#define DEFINE_MM_REG_RD(reg, off) \
	static inline uint32_t read_##reg(const struct device *dev) \
	{ \
		const struct mspi_dw_config *dev_config = dev->config; \
		return dev_config->read((mm_reg_t)DEVICE_MMIO_GET(dev), off); \
	}
#define DEFINE_MM_REG_WR(reg, off) \
	static inline void write_##reg(const struct device *dev, uint32_t data) \
	{ \
		const struct mspi_dw_config *dev_config = dev->config; \
		dev_config->write(data, (mm_reg_t)DEVICE_MMIO_GET(dev),  off); \
	}
#define DEFINE_MM_REG_RD_WR(reg, off) \
	DEFINE_MM_REG_RD(reg, off) \
	DEFINE_MM_REG_WR(reg, off)

DEFINE_MM_REG_RD_WR(ctrlr0,	0x00)
DEFINE_MM_REG_RD_WR(ctrlr1,	0x04)
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
DEFINE_MM_REG_RD_WR(spi_ctrlr0,	0xf4)

static void read_rx_fifo(const struct device *dev,
			 const struct mspi_xfer_packet *packet)
{
	struct mspi_dw_data *dev_data = dev->data;
	const struct mspi_dw_config *dev_config = dev->config;
	uint32_t remaining;

	do {
		uint32_t data = read_dr(dev);

		if (dev_data->to_discard) {
			--dev_data->to_discard;
		} else {
			packet->data_buf[dev_data->bytes_done] = (uint8_t)data;

			++dev_data->bytes_done;
			if (dev_data->bytes_done >= packet->num_bytes) {
				return;
			}
		}
	} while (FIELD_GET(RXFLR_RXTFL_MASK, read_rxflr(dev)));

	remaining = packet->num_bytes - dev_data->bytes_done;
	if (remaining - 1 < dev_config->rx_fifo_threshold) {
		write_rxftlr(dev, remaining - 1);
	}
}

static bool make_rx_cycles(const struct device *dev)
{
	struct mspi_dw_data *dev_data = dev->data;
	const struct mspi_dw_config *dev_config = dev->config;
	uint32_t room = dev_config->tx_fifo_depth -
			FIELD_GET(TXFLR_TXTFL_MASK, read_txflr(dev));

	do {
		write_dr(dev, 0xAA);

		--dev_data->rx_tx_cnt;
		if (!dev_data->rx_tx_cnt) {
			return true;
		}
	} while (--room);

	return false;
}

static void tx_data(const struct device *dev,
		    const struct mspi_xfer_packet *packet)
{
	struct mspi_dw_data *dev_data = dev->data;
	const struct mspi_dw_config *dev_config = dev->config;
	uint32_t room = dev_config->tx_fifo_depth -
			FIELD_GET(TXFLR_TXTFL_MASK, read_txflr(dev));

	do {
		write_dr(dev, packet->data_buf[dev_data->bytes_done]);

		++dev_data->bytes_done;
		if (dev_data->bytes_done >= packet->num_bytes) {
			write_txftlr(dev, 0);
			break;
		}
	} while (--room);
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

	if (dev_data->bytes_done >= packet->num_bytes) {
		write_imr(dev, 0);
		/* It may happen that the controller still shifts out the last
		 * frame (the last interrupt occurs when the TX fifo is empty).
		 * Wait if it still signals that it is busy.
		 */
		while (read_sr(dev) & SR_BUSY_BIT) {
		}

		k_sem_give(&dev_data->finished);
	} else {
		if (int_status & ISR_TXEIS_BIT) {
			if (dev_data->rx_tx_cnt) {
				if (make_rx_cycles(dev)) {
					write_imr(dev, IMR_RXFIM_BIT);
				}
			} else {
				tx_data(dev, packet);
			}
		}
	}

	/* TODO: provide more generic solution */
#ifdef CONFIG_HAS_NRFX
	NRF_EXMIF->EVENTS_CORE = 0;
#endif
}

static int api_config(const struct mspi_dt_spec *spec)
{
	ARG_UNUSED(spec);

	return -ENOTSUP;
}

static int apply_io_mode(const struct device *dev, enum mspi_io_mode io_mode,
			 uint32_t *ctrlr0, uint32_t *spi_ctrlr0)
{
	struct mspi_dw_data *dev_data = dev->data;

	/* Frame format used for transferring data. */

	if (io_mode == MSPI_IO_MODE_SINGLE) {
		*ctrlr0 |= FIELD_PREP(CTRLR0_SPI_FRF_MASK,
				      CTRLR0_SPI_FRF_STANDARD);
		dev_data->standard_spi = true;
		return 0;
	}

	dev_data->standard_spi = false;

	switch (io_mode) {
	case MSPI_IO_MODE_DUAL:
	case MSPI_IO_MODE_DUAL_1_1_2:
	case MSPI_IO_MODE_DUAL_1_2_2:
		*ctrlr0 |= FIELD_PREP(CTRLR0_SPI_FRF_MASK,
				      CTRLR0_SPI_FRF_DUAL);
		break;
	case MSPI_IO_MODE_QUAD:
	case MSPI_IO_MODE_QUAD_1_1_4:
	case MSPI_IO_MODE_QUAD_1_4_4:
		*ctrlr0 |= FIELD_PREP(CTRLR0_SPI_FRF_MASK,
				      CTRLR0_SPI_FRF_QUAD);
		break;
	case MSPI_IO_MODE_OCTAL:
	case MSPI_IO_MODE_OCTAL_1_1_8:
	case MSPI_IO_MODE_OCTAL_1_8_8:
		*ctrlr0 |= FIELD_PREP(CTRLR0_SPI_FRF_MASK,
				      CTRLR0_SPI_FRF_OCTAL);
		break;
	default:
		LOG_ERR("IO mode %d not supported", io_mode);
		return -EINVAL;
	}

	/* Transfer format used for Address and Instruction: */

	switch (io_mode) {
	case MSPI_IO_MODE_DUAL_1_1_2:
	case MSPI_IO_MODE_QUAD_1_1_4:
	case MSPI_IO_MODE_OCTAL_1_1_8:
		/* - both sent in Standard SPI mode */
		*spi_ctrlr0 |= FIELD_PREP(SPI_CTRLR0_TRANS_TYPE_MASK,
					  SPI_CTRLR0_TRANS_TYPE_TT0);
		break;
	case MSPI_IO_MODE_DUAL_1_2_2:
	case MSPI_IO_MODE_QUAD_1_4_4:
	case MSPI_IO_MODE_OCTAL_1_8_8:
		/* - Instruction sent in Standard SPI mode,
		 *   Address sent the same way as data
		 */
		*spi_ctrlr0 |= FIELD_PREP(SPI_CTRLR0_TRANS_TYPE_MASK,
					  SPI_CTRLR0_TRANS_TYPE_TT1);
		break;
	default:
		/* - both sent the same way as data. */
		*spi_ctrlr0 |= FIELD_PREP(SPI_CTRLR0_TRANS_TYPE_MASK,
					  SPI_CTRLR0_TRANS_TYPE_TT2);
		break;
	}

	return 0;
}

static int apply_cmd_length(uint32_t *spi_ctrlr0, uint32_t cmd_length)
{
	switch (cmd_length) {
	case 0:
		*spi_ctrlr0 |= FIELD_PREP(SPI_CTRLR0_INST_L_MASK,
					  SPI_CTRLR0_INST_L0);
		break;
	case 1:
		*spi_ctrlr0 |= FIELD_PREP(SPI_CTRLR0_INST_L_MASK,
					  SPI_CTRLR0_INST_L8);
		break;
	case 2:
		*spi_ctrlr0 |= FIELD_PREP(SPI_CTRLR0_INST_L_MASK,
					  SPI_CTRLR0_INST_L16);
		break;
	default:
		LOG_ERR("Command length %d not supported", cmd_length);
		return -EINVAL;
	}

	return 0;
}

static int apply_addr_length(uint32_t *spi_ctrlr0, uint32_t addr_length)
{
	*spi_ctrlr0 |= FIELD_PREP(SPI_CTRLR0_ADDR_L_MASK, addr_length * 2);

	return 0;
}

static int api_dev_config(const struct device *dev,
			  const struct mspi_dev_id *dev_id,
			  const enum mspi_dev_cfg_mask param_mask,
			  const struct mspi_dev_cfg *cfg)
{
	const struct mspi_dw_config *dev_config = dev->config;
	struct mspi_dw_data *dev_data = dev->data;
	uint32_t ctrlr0 = 0;
	uint32_t spi_ctrlr0 = 0;
	int rc;

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
		rc = apply_io_mode(dev, cfg->io_mode, &ctrlr0, &spi_ctrlr0);
		if (rc < 0) {
			return rc;
		}
	}

	if (param_mask & MSPI_DEVICE_CONFIG_CPP) {
		switch (cfg->cpp) {
		default:
		case MSPI_CPP_MODE_0:
			ctrlr0 |= FIELD_PREP(CTRLR0_SCPOL_BIT, 0) |
				  FIELD_PREP(CTRLR0_SCPH_BIT,  0);
			break;
		case MSPI_CPP_MODE_1:
			ctrlr0 |= FIELD_PREP(CTRLR0_SCPOL_BIT, 0) |
				  FIELD_PREP(CTRLR0_SCPH_BIT,  1);
			break;
		case MSPI_CPP_MODE_2:
			ctrlr0 |= FIELD_PREP(CTRLR0_SCPOL_BIT, 1) |
				  FIELD_PREP(CTRLR0_SCPH_BIT,  0);
			break;
		case MSPI_CPP_MODE_3:
			ctrlr0 |= FIELD_PREP(CTRLR0_SCPOL_BIT, 1) |
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

		write_baudr(dev, dev_config->clock_frequency / cfg->freq);
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

	if (param_mask & MSPI_DEVICE_CONFIG_RX_DUMMY) {
		dev_data->rx_dummy = cfg->rx_dummy;
	}

	if (param_mask & MSPI_DEVICE_CONFIG_TX_DUMMY) {
		dev_data->tx_dummy = cfg->tx_dummy;
	}

	if (param_mask & MSPI_DEVICE_CONFIG_READ_CMD) {
		dev_data->read_cmd = cfg->read_cmd;
	}

	if (param_mask & MSPI_DEVICE_CONFIG_WRITE_CMD) {
		dev_data->write_cmd = cfg->write_cmd;
	}

	if (param_mask & MSPI_DEVICE_CONFIG_CMD_LEN) {
		dev_data->cmd_length = cfg->cmd_length;
	}

	if (param_mask & MSPI_DEVICE_CONFIG_ADDR_LEN) {
		dev_data->addr_length = cfg->addr_length;
	}

	/* Always use Motorola SPI frame format and 8-bit data frame size. */
	ctrlr0 |= FIELD_PREP(CTRLR0_FRF_MASK, CTRLR0_FRF_SPI)
	       |  FIELD_PREP(CTRLR0_DFS_MASK, 7);

	spi_ctrlr0 |= FIELD_PREP(SPI_CTRLR0_CLK_STRETCH_EN_MASK, 1);

	write_ctrlr0(dev, ctrlr0);
	write_spi_ctrlr0(dev, spi_ctrlr0);

	dev_data->dev_id = *dev_id;
	write_ser(dev, BIT(dev_id->dev_idx));

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

static int start_next_packet(const struct device *dev,
			     k_timeout_t timeout)
{
	const struct mspi_dw_config *dev_config = dev->config;
	struct mspi_dw_data *dev_data = dev->data;
	const struct mspi_xfer_packet *packet =
		&dev_data->xfer.packets[dev_data->packets_done];
	uint32_t ctrlr0 = read_ctrlr0(dev);
	uint32_t spi_ctrlr0 = read_spi_ctrlr0(dev);
	uint8_t tx_fifo_threshold;
	uint32_t imr;
	int rc;

	/* TODO: support longer transfers */
	if (packet->num_bytes >= UINT16_MAX) {
		return -EINVAL;
	}

	dev_data->rx_tx_cnt = 0;

	ctrlr0 &= ~CTRLR0_TMOD_MASK;
	spi_ctrlr0 &= ~SPI_CTRLR0_WAIT_CYCLES_MASK;

	/* Set the transfer start level to maximum so that the transfer does
	 * not start before TX FIFO is properly filled.
	 */
	write_txftlr(dev, FIELD_PREP(TXFTLR_TXFTHR_MASK,
				     dev_config->tx_fifo_depth - 1));

	if (packet->dir == MSPI_TX || packet->num_bytes == 0) {
		imr = IMR_TXEIM_BIT;
		ctrlr0 |= FIELD_PREP(CTRLR0_TMOD_MASK, CTRLR0_TMOD_TX);
		spi_ctrlr0 |= FIELD_PREP(SPI_CTRLR0_WAIT_CYCLES_MASK,
					 dev_data->xfer.tx_dummy);

		write_rxftlr(dev, 0);
		tx_fifo_threshold = dev_config->tx_fifo_threshold;
	} else {
		uint32_t tmod;
		uint8_t rx_fifo_threshold;

		if (!dev_data->standard_spi ||
		    (dev_data->xfer.cmd_length == 0 &&
		     dev_data->xfer.addr_length == 0)) {
			imr = IMR_RXFIM_BIT;
			tmod = CTRLR0_TMOD_RX;
			tx_fifo_threshold = 0;
			rx_fifo_threshold = MIN(packet->num_bytes - 1,
						dev_config->rx_fifo_threshold);
		} else {
			imr = IMR_TXEIM_BIT | IMR_RXFIM_BIT;
			tmod = CTRLR0_TMOD_TX_RX;
			tx_fifo_threshold = dev_config->tx_fifo_threshold;

			dev_data->rx_tx_cnt = packet->num_bytes;
			dev_data->to_discard = dev_data->xfer.cmd_length
					     + dev_data->xfer.addr_length;

			rx_fifo_threshold =
				MIN(dev_data->to_discard + packet->num_bytes - 1,
				    dev_config->rx_fifo_threshold);
		}

		ctrlr0 |= FIELD_PREP(CTRLR0_TMOD_MASK, tmod);
		spi_ctrlr0 |= FIELD_PREP(SPI_CTRLR0_WAIT_CYCLES_MASK,
					 dev_data->xfer.rx_dummy);

		write_rxftlr(dev, FIELD_PREP(RXFTLR_RFT_MASK,
					     rx_fifo_threshold));
	}

	write_ctrlr0(dev, ctrlr0);
	write_ctrlr1(dev, packet->num_bytes > 0
		? FIELD_PREP(CTRLR1_NDF_MASK, packet->num_bytes - 1)
		: 0);
	write_spi_ctrlr0(dev, spi_ctrlr0);

	if (dev_data->dev_id.ce.port) {
		gpio_pin_set_dt(&dev_data->dev_id.ce, 1);
	}

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

	dev_data->bytes_done = 0;

	if (dev_data->rx_tx_cnt) {
		if (make_rx_cycles(dev)) {
			imr = IMR_RXFIM_BIT;
		}
	} else if (packet->dir == MSPI_TX && packet->num_bytes) {
		tx_data(dev, packet);
	}

	write_txftlr(dev, dev_data->bytes_done < packet->num_bytes
		? FIELD_PREP(TXFTLR_TFT_MASK, tx_fifo_threshold)
		: 0);
	write_imr(dev, imr);

	rc = k_sem_take(&dev_data->finished, timeout);
	if (rc < 0) {
		rc = -ETIMEDOUT;
	}

	/* Disable the controller. Any transfers will stop immediately. */
	write_ssienr(dev, 0);

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
	uint32_t spi_ctrlr0 = read_spi_ctrlr0(dev);
	int rc;

	/* TODO: add support for asynchronous transfers */
	if (req->async) {
		return -ENOTSUP;
	}

	spi_ctrlr0 &= ~SPI_CTRLR0_WAIT_CYCLES_MASK
		   &  ~SPI_CTRLR0_INST_L_MASK
		   &  ~SPI_CTRLR0_ADDR_L_MASK;

	rc = apply_cmd_length(&spi_ctrlr0, req->cmd_length);
	if (rc < 0) {
		return rc;
	}

	rc = apply_addr_length(&spi_ctrlr0, req->addr_length);
	if (rc < 0) {
		return rc;
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

	write_spi_ctrlr0(dev, spi_ctrlr0);

	dev_data->xfer = *req;
	dev_data->packets_done = 0;

	while (dev_data->packets_done < dev_data->xfer.num_packet) {
		rc = start_next_packet(dev, K_MSEC(dev_data->xfer.timeout));
		if (rc < 0) {
			return rc;
		}

		++dev_data->packets_done;
	}

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
	pinctrl_apply_state(dev_config->pcfg, PINCTRL_STATE_DEFAULT);
#endif

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	/* TODO: provide more generic solution */
#ifdef CONFIG_HAS_NRFX
	NRF_EXMIF->INTENSET = BIT(0);
	NRF_EXMIF->TASKS_START = 1;
#endif

	dev_config->irq_config();

	write_ssienr(dev, 0);
	write_imr(dev, 0);

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
};

#define USES_AUX_REG(inst) + DT_INST_PROP(inst, aux_reg)
#define AUX_REG_INSTANCES (0 DT_INST_FOREACH_STATUS_OKAY(USES_AUX_REG))

#if AUX_REG_INSTANCES != 0
static uint32_t aux_reg_read(mm_reg_t addr, uint32_t off)
{
	return sys_in32(addr + off/4);
}
static void aux_reg_write(uint32_t data, mm_reg_t addr, uint32_t off)
{
	sys_out32(data, addr + off/4);
}
#endif

#if AUX_REG_INSTANCES != DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT)
static uint32_t reg_read(mm_reg_t addr, uint32_t off)
{
	return sys_read32(addr + off);
}
static void reg_write(uint32_t data, mm_reg_t addr, uint32_t off)
{
	sys_write32(data, addr + off);
}
#endif

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
				1 * TX_FIFO_DEPTH(inst) / 4 - 1),	\
	.rx_fifo_threshold =						\
		DT_INST_PROP_OR(inst, rx_fifo_threshold,		\
				3 * RX_FIFO_DEPTH(inst) / 4 - 1)

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
		(MSPI_DW_CE_GPIOS(inst))),				\
		MSPI_DW_FIFO_PROPS(inst),				\
	COND_CODE_1(DT_INST_PROP(inst, aux_reg),			\
		(.read = aux_reg_read,					\
		.write = aux_reg_write,),				\
		(.read = reg_read,					\
		.write = reg_write,))					\
	};								\
	DEVICE_DT_INST_DEFINE(inst,					\
		dev_init, PM_DEVICE_DT_INST_GET(inst),			\
		&dev##inst##_data, &dev##inst##_config,			\
		POST_KERNEL, CONFIG_MSPI_INIT_PRIORITY,			\
		&drv_api);

DT_INST_FOREACH_STATUS_OKAY(MSPI_DW_INST)
