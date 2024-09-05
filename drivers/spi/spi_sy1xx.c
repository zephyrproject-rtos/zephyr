/*
 * Copyright (c) 2024 sensry.io
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT sensry_sy1xx_spi

#define LOG_LEVEL CONFIG_SPI_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_sy1xx);

#include "spi_context.h"
#include <errno.h>
#include <zephyr/spinlock.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>
#include <udma.h>
#include <zephyr/drivers/pinctrl.h>

/* SPI udma command interface definitions */
#define SPI_CMD_OFFSET (4)

/* Commands for SPI UDMA */
#define SPI_CMD_CFG       (0 << SPI_CMD_OFFSET)
#define SPI_CMD_SOT       (1 << SPI_CMD_OFFSET)
#define SPI_CMD_SEND_CMD  (2 << SPI_CMD_OFFSET)
#define SPI_CMD_SEND_ADDR (3 << SPI_CMD_OFFSET)
#define SPI_CMD_DUMMY     (4 << SPI_CMD_OFFSET)
#define SPI_CMD_WAIT      (5 << SPI_CMD_OFFSET)
#define SPI_CMD_TX_DATA   (6 << SPI_CMD_OFFSET)
#define SPI_CMD_RX_DATA   (7 << SPI_CMD_OFFSET)
#define SPI_CMD_RPT       (8 << SPI_CMD_OFFSET)
#define SPI_CMD_EOT       (9 << SPI_CMD_OFFSET)
#define SPI_CMD_RPT_END   (10 << SPI_CMD_OFFSET)
#define SPI_CMD_RX_CHECK  (11 << SPI_CMD_OFFSET)
#define SPI_CMD_FULL_DPLX (12 << SPI_CMD_OFFSET)

/* CMD CFG */
#define SPI_CMD_CFG_0()           (SPI_CMD_CFG)
#define SPI_CMD_CFG_1()           (0)
#define SPI_CMD_CFG_2(cpol, cpha) (((cpol & 0x1) << 1) | ((cpha & 0x1) << 0))
#define SPI_CMD_CFG_3(div)        (div & 0xff)

/* CMD SOT */
#define SPI_CMD_SOT_0()   (SPI_CMD_SOT)
#define SPI_CMD_SOT_1()   (0)
#define SPI_CMD_SOT_2()   (0)
#define SPI_CMD_SOT_3(cs) (cs & 0x1)

/* CMD SEND_ADDR */
#define SPI_CMD_SEND_ADDR0(qspi)     (SPI_CMD_SEND_ADDR | ((qspi & 0x1) << 3))
#define SPI_CMD_SEND_ADDR1(num_bits) ((num_bits - 1) & 0x1f)
#define SPI_CMD_SEND_ADDR2()         (0)
#define SPI_CMD_SEND_ADDR3()         (0)

/* CMD SEND_DATA */
#define SPI_CMD_SEND_DATA0(qspi)     (SPI_CMD_TX_DATA | ((qspi & 0x1) << 3))
#define SPI_CMD_SEND_DATA1()         (0)
#define SPI_CMD_SEND_DATA2(num_bits) (UINT16_BYTE0((num_bits - 1)) & 0xffff)
#define SPI_CMD_SEND_DATA3(num_bits) (UINT16_BYTE1((num_bits - 1)) & 0xffff)

/* CMD READ_DATA */
#define SPI_CMD_READ_DATA0(qspi, align)                                                            \
	(SPI_CMD_RX_DATA | ((qspi & 0x1) << 3) | ((align & 0x3) << 1))
#define SPI_CMD_READ_DATA1()         (0)
#define SPI_CMD_READ_DATA2(num_bits) (UINT16_BYTE0((num_bits - 1)) & 0xffff)
#define SPI_CMD_READ_DATA3(num_bits) (UINT16_BYTE1((num_bits - 1)) & 0xffff)

/* CMD FULL_DPLX */
#define SPI_CMD_FULL_DPLX_DATA0(qspi, align)                                                       \
	(SPI_CMD_FULL_DPLX | ((qspi & 0x1) << 3) | ((align & 0x3) << 1))
#define SPI_CMD_FULL_DPLX_DATA1()         (0)
#define SPI_CMD_FULL_DPLX_DATA2(num_bits) (UINT16_BYTE0((num_bits - 1)) & 0xffff)
#define SPI_CMD_FULL_DPLX_DATA3(num_bits) (UINT16_BYTE1((num_bits - 1)) & 0xffff)

/* CMD EOT */
#define SPI_CMD_EOT0()    (SPI_CMD_EOT)
#define SPI_CMD_EOT1()    (0)
#define SPI_CMD_EOT2()    (0)
#define SPI_CMD_EOT3(evt) (evt & 0x1)

/* CMD Wait */
#define SPI_CMD_WAIT0() (SPI_CMD_DUMMY)
#define SPI_CMD_WAIT1() (0xff)
#define SPI_CMD_WAIT2() (0xff)
#define SPI_CMD_WAIT3() (0xff)

/* Hardware chip select dt slave fields; set reg, if hw chip select shall be used */
#define SY1XX_CS_HW_SELECT_0 0x80
#define SY1XX_CS_HW_SELECT_1 0x81

#define UINT16_BYTE0(uint16) ((uint16 >> 8) & 0xff)
#define UINT16_BYTE1(uint16) ((uint16 >> 0) & 0xff)

#define SY1XX_SPI_MIN_FREQUENCY   (250000)
#define SY1XX_SPI_MAX_FREQUENCY   (62500000)
#define SY1XX_SPI_WORD_SIZE_8_BIT (8)
#define SY1XX_SPI_WORD_ALIGN      (0)
#define SY1XX_SPI_MAX_BIT_COUNT   (10240)
#define SY1XX_SPI_MAX_BUFFER_SIZE (SY1XX_SPI_MAX_BIT_COUNT / 8)

struct sy1xx_spi_config {
	/* dma base address */
	uint32_t base;
	/* pin ctrl for all spi related pins */
	const struct pinctrl_dev_config *pcfg;
	/* number of instance, spi0, spi1, ... */
	uint32_t inst;
	/* quad spi enabled */
	uint8_t quad_spi;
	/* character used to fill tx fifo, while reading (default: 0xff) */
	uint8_t overrun_char;
};

/* place buffer in dma section */
struct sy1xx_spi_dma_buffer {
	uint8_t write[SY1XX_SPI_MAX_BUFFER_SIZE];
	uint8_t read[SY1XX_SPI_MAX_BUFFER_SIZE];
};

struct sy1xx_spi_data {
	struct spi_context ctx;
	/* pin only used, if not using gpio and explicitly selected in dt */
	int32_t cs_pin;
	/* reference to dma buffers */
	struct sy1xx_spi_dma_buffer *dma;
	/* current bus configuration */
	uint8_t cpol;
	uint8_t cpha;
	uint8_t div;
};

static int sy1xx_spi_release(const struct device *dev, const struct spi_config *config);

static int sy1xx_spi_init(const struct device *dev)
{

	struct sy1xx_spi_config *cfg = (struct sy1xx_spi_config *)dev->config;
	struct sy1xx_spi_data *data = dev->data;
	int32_t ret;

	/* UDMA clock enable */
	sy1xx_udma_enable_clock(SY1XX_UDMA_MODULE_SPI, cfg->inst);

	/* PAD config */
	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("SPI failed to set pin config for %u", cfg->inst);
		return ret;
	}

	/* reset udma */
	SY1XX_UDMA_CANCEL_RX(cfg->base);
	SY1XX_UDMA_CANCEL_TX(cfg->base);

	SY1XX_UDMA_WAIT_FOR_FINISHED_TX(cfg->base);
	SY1XX_UDMA_WAIT_FOR_FINISHED_RX(cfg->base);

	/* prepare context for cs */
	ret = spi_context_cs_configure_all(&data->ctx);
	if (ret < 0) {
		LOG_ERR("SPI failed to configure");
		return ret;
	}

	/* reset config, expected to come with first transfer */
	data->ctx.config = NULL;

	return sy1xx_spi_release(dev, NULL);
}

static int sy1xx_spi_configure(const struct device *dev, const struct spi_config *config)
{
	struct sy1xx_spi_data *data = dev->data;
	uint32_t frequency;

	if (spi_context_configured(&data->ctx, config)) {
		return 0;
	}

	if (!spi_cs_is_gpio(config)) {
		/* cs is not a real gpio, that indicates, we are using the hw cs */
		switch (config->slave) {
		case SY1XX_CS_HW_SELECT_0:
			data->cs_pin = 0;
			break;
		case SY1XX_CS_HW_SELECT_1:
			data->cs_pin = 1;
			break;
		default:
			return -EINVAL;
		}
	} else {
		data->cs_pin = -1;
	}

	if (SPI_OP_MODE_GET(config->operation) != SPI_OP_MODE_MASTER) {
		/* Slave mode is not implemented. */
		return -ENOTSUP;
	}

	if (SPI_WORD_SIZE_GET(config->operation) != SY1XX_SPI_WORD_SIZE_8_BIT) {
		/* only 8 bit mode is implemented. */
		return -ENOTSUP;
	}

	if ((config->operation & SPI_MODE_CPOL) == 0U) {
		data->cpol = 0;
	} else {
		data->cpol = 1;
	}

	if ((config->operation & SPI_MODE_CPHA) == 0U) {
		data->cpha = 0;
	} else {
		data->cpha = 1;
	}

	frequency = CLAMP(config->frequency, SY1XX_SPI_MIN_FREQUENCY, SY1XX_SPI_MAX_FREQUENCY);

	/* peripheral pre-scaler 1:2 */
	data->div = sy1xx_soc_get_peripheral_clock() / 2 / frequency;
	if (data->div == 0) {
		data->div = 1;
	}

	data->ctx.config = config;

	return 0;
}

static int sy1xx_spi_set_cs(const struct device *dev)
{
	struct sy1xx_spi_config *cfg = (struct sy1xx_spi_config *)dev->config;
	struct sy1xx_spi_data *data = dev->data;
	uint8_t *cmd_buf;
	uint32_t count;

	cmd_buf = &data->dma->write[0];
	count = 0;

	/* prepare bus cfg */
	cmd_buf[count++] = SPI_CMD_CFG_3(data->div);
	cmd_buf[count++] = SPI_CMD_CFG_2(data->cpol, data->cpha);
	cmd_buf[count++] = SPI_CMD_CFG_1();
	cmd_buf[count++] = SPI_CMD_CFG_0();

	/* check if spi controller chip select is configured */
	if (data->cs_pin >= 0) {

		/* start with selecting hardware chip-select */
		cmd_buf[count++] = SPI_CMD_SOT_3(data->cs_pin);
		cmd_buf[count++] = SPI_CMD_SOT_2();
		cmd_buf[count++] = SPI_CMD_SOT_1();
		cmd_buf[count++] = SPI_CMD_SOT_0();
	}

	/* transfer configuration via udma to spi controller */
	SY1XX_UDMA_START_TX(cfg->base, (uint32_t)cmd_buf, count, 0);
	SY1XX_UDMA_WAIT_FOR_FINISHED_TX(cfg->base);

	if (SY1XX_UDMA_GET_REMAINING_TX(cfg->base)) {
		return -EINVAL;
	}

	/* enable gpio cs (if configured) */
	spi_context_cs_control(&data->ctx, true);

	return 0;
}

static int32_t sy1xx_spi_reset_cs(const struct device *dev)
{
	struct sy1xx_spi_config *cfg = (struct sy1xx_spi_config *)dev->config;
	struct sy1xx_spi_data *data = dev->data;
	uint8_t *cmd_buf;
	uint32_t count;

	count = 0;
	cmd_buf = &data->dma->write[0];

	/* prepare end of transmission (includes resetting any enabled chip selects) */
	cmd_buf[count++] = SPI_CMD_EOT3(0);
	cmd_buf[count++] = SPI_CMD_EOT2();
	cmd_buf[count++] = SPI_CMD_EOT1();
	cmd_buf[count++] = SPI_CMD_EOT0();

	SY1XX_UDMA_START_TX(cfg->base, (uint32_t)cmd_buf, count, 0);
	SY1XX_UDMA_WAIT_FOR_FINISHED_TX(cfg->base);

	/* reset gpio chip select */
	spi_context_cs_control(&data->ctx, false);

	if (SY1XX_UDMA_GET_REMAINING_TX(cfg->base)) {
		return -EINVAL;
	}

	return 0;
}

static int32_t sy1xx_spi_full_duplex_transfer(const struct device *dev, uint8_t *tx_buf,
					      uint8_t *rx_buf, uint32_t xfer_len)
{
	struct sy1xx_spi_config *cfg = (struct sy1xx_spi_config *)dev->config;
	struct sy1xx_spi_data *data = (struct sy1xx_spi_data *)dev->data;
	uint8_t *cmd_buf;
	uint32_t count;
	uint32_t padded_len;

	if (xfer_len == 0) {
		return -EINVAL;
	}

	if (xfer_len >= SY1XX_SPI_MAX_BUFFER_SIZE) {
		return -ENOBUFS;
	}

	cmd_buf = &data->dma->write[0];
	count = 0;

	/* expected data config (bitlen in bits) and spi transfer type */
	cmd_buf[count++] = SPI_CMD_FULL_DPLX_DATA3(xfer_len * 8);
	cmd_buf[count++] = SPI_CMD_FULL_DPLX_DATA2(xfer_len * 8);
	cmd_buf[count++] = SPI_CMD_FULL_DPLX_DATA1();
	cmd_buf[count++] = SPI_CMD_FULL_DPLX_DATA0(cfg->quad_spi, SY1XX_SPI_WORD_ALIGN);

	/* data; we need to fill multiple of 32bit size */
	padded_len = ROUND_UP(xfer_len, 4);

	for (uint32_t i = 0; i < padded_len; i++) {
		if (tx_buf) {
			/* fill data */
			cmd_buf[count + i] = (i < xfer_len) ? tx_buf[i] : cfg->overrun_char;
		} else {
			/* fill placeholder */
			cmd_buf[count + i] = cfg->overrun_char;
		}
	}

	count += padded_len;

	SY1XX_UDMA_START_RX(cfg->base, (uint32_t)data->dma->read, xfer_len,
			    SY1XX_UDMA_RX_DATA_ADDR_INC_SIZE_32);
	SY1XX_UDMA_START_TX(cfg->base, (uint32_t)data->dma->write, count, 0);

	SY1XX_UDMA_WAIT_FOR_FINISHED_TX(cfg->base);
	SY1XX_UDMA_WAIT_FOR_FINISHED_RX(cfg->base);

	if (SY1XX_UDMA_GET_REMAINING_TX(cfg->base)) {
		LOG_ERR("not all bytes sent");
		return -EINVAL;
	}
	if (SY1XX_UDMA_GET_REMAINING_RX(cfg->base)) {
		LOG_ERR("not all bytes received");
		return -EINVAL;
	}

	if (rx_buf) {
		/* transfer from dma buffer to provided receive buffer */
		memcpy(rx_buf, &data->dma->read[0], xfer_len);
	}

	return 0;
}

static int sy1xx_spi_transceive_sync(const struct device *dev, const struct spi_config *config,
				     const struct spi_buf_set *tx_bufs,
				     const struct spi_buf_set *rx_bufs)
{
	struct sy1xx_spi_data *data = dev->data;

	int ret = 0;
	size_t tx_count = 0;
	size_t rx_count = 0;
	const struct spi_buf *tx = NULL;
	const struct spi_buf *rx = NULL;

	if (tx_bufs) {
		tx = tx_bufs->buffers;
		tx_count = tx_bufs->count;
	}

	if (rx_bufs) {
		rx = rx_bufs->buffers;
		rx_count = rx_bufs->count;
	}

	spi_context_lock(&data->ctx, false, NULL, NULL, config);

	ret = sy1xx_spi_configure(dev, config);
	if (ret != 0) {
		goto done;
	}

	ret = sy1xx_spi_set_cs(dev);
	if (ret) {
		LOG_ERR("SPI set cs failed");
		goto done;
	}

	/* handle symmetrical tx and rx transfers */
	while (tx_count != 0 && rx_count != 0) {
		if (tx->buf == NULL) {
			/* read */
			ret = sy1xx_spi_full_duplex_transfer(dev, NULL, rx->buf, rx->len);
		} else if (rx->buf == NULL) {
			/* write */
			ret = sy1xx_spi_full_duplex_transfer(dev, tx->buf, NULL, tx->len);
		} else if (rx->len == tx->len) {
			/* read / write */
			ret = sy1xx_spi_full_duplex_transfer(dev, tx->buf, rx->buf, rx->len);
		} else {
			__ASSERT_NO_MSG("Invalid SPI transfer configuration");
		}

		if (ret) {
			LOG_ERR("Failed to transfer data - %d", ret);
			goto done;
		}

		tx++;
		tx_count--;
		rx++;
		rx_count--;
	}

	/* handle the left-overs for tx only */
	for (; tx_count != 0; tx_count--) {
		ret = sy1xx_spi_full_duplex_transfer(dev, tx->buf, NULL, tx->len);
		if (ret < 0) {
			LOG_ERR("Failed to transfer data");
			goto done;
		}
		tx++;
	}

	/* handle the left-overs for rx only */
	for (; rx_count != 0; rx_count--) {
		ret = sy1xx_spi_full_duplex_transfer(dev, NULL, rx->buf, rx->len);
		if (ret < 0) {
			LOG_ERR("Failed to transfer data");
			goto done;
		}
		rx++;
	}

done:
	sy1xx_spi_reset_cs(dev);
	spi_context_release(&data->ctx, ret);

	return ret;
}

static int sy1xx_spi_release(const struct device *dev, const struct spi_config *config)
{
	struct sy1xx_spi_data *data = dev->data;

	spi_context_unlock_unconditionally(&data->ctx);
	return 0;
}

static DEVICE_API(spi, sy1xx_spi_driver_api) = {
	.transceive = sy1xx_spi_transceive_sync,
	.release = sy1xx_spi_release,
};

#define SPI_SY1XX_DEVICE_INIT(n)                                                                   \
                                                                                                   \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
                                                                                                   \
	static const struct sy1xx_spi_config sy1xx_spi_dev_config_##n = {                          \
		.base = (uint32_t)DT_INST_REG_ADDR(n),                                             \
		.inst = (uint32_t)DT_INST_PROP(n, instance),                                       \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.quad_spi = DT_INST_PROP_OR(n, quad_spi, 0),                                       \
		.overrun_char = DT_INST_PROP_OR(n, overrun_character, 0xff),                       \
	};                                                                                         \
                                                                                                   \
	static struct sy1xx_spi_dma_buffer __attribute__((section(".udma_access")))                \
	__aligned(4) sy1xx_spi_dev_dma_##n = {};                                                   \
                                                                                                   \
	static struct sy1xx_spi_data sy1xx_spi_dev_data_##n = {                                    \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(n), ctx)                               \
			SPI_CONTEXT_INIT_LOCK(sy1xx_spi_dev_data_##n, ctx),                        \
		SPI_CONTEXT_INIT_SYNC(sy1xx_spi_dev_data_##n, ctx),                                \
		.dma = &sy1xx_spi_dev_dma_##n,                                                     \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &sy1xx_spi_init, NULL, &sy1xx_spi_dev_data_##n,                   \
			      &sy1xx_spi_dev_config_##n, POST_KERNEL, CONFIG_SPI_INIT_PRIORITY,    \
			      &sy1xx_spi_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SPI_SY1XX_DEVICE_INIT)
