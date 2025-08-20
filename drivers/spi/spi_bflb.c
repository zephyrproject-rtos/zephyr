/*
 * Copyright (c) 2024-2025 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT bflb_spi

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/irq.h>


#define LOG_LEVEL CONFIG_SPI_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(spi_bflb, CONFIG_SPI_LOG_LEVEL);

#include <bflb_soc.h>
#include <glb_reg.h>
#include <hbn_reg.h>
#undef HBN_RTC_DLY_OPTION
#include <extra_defines.h>
#include <common_defines.h>
#include <zephyr/dt-bindings/clock/bflb_clock_common.h>
#include <zephyr/drivers/clock_control/clock_control_bflb_common.h>
#include <bouffalolab/common/spi_reg.h>

#include "spi_context.h"

#if defined(CONFIG_SOC_SERIES_BL60X) || defined(CONFIG_SOC_SERIES_BL70X)
#define GLB_SPI_MODE_ADDRESS (GLB_BASE + GLB_PARM_OFFSET)
/* be careful: counted in words here */
#define SPI_FIFO_SIZE 4
#elif defined(CONFIG_SOC_SERIES_BL61X)
#define GLB_SPI_MODE_ADDRESS (GLB_BASE + GLB_PARM_CFG0_OFFSET)
/* and counted in bytes there.
 * Because the value is returned as a different unit in the registers!
 */
#define SPI_FIFO_SIZE 32
#endif
#define SPI_WAIT_TIMEOUT_MS 250

/* Structure declarations */

struct spi_bflb_cfg {
	const struct pinctrl_dev_config *pincfg;
	uint32_t base;
	void (*irq_config_func)(const struct device *dev);
};

struct spi_bflb_data {
	struct spi_context ctx;
};

#if defined(CONFIG_SOC_SERIES_BL70X) || defined(CONFIG_SOC_SERIES_BL60X)

static uint32_t spi_bflb_get_clk(void)
{
	uint32_t uclk;
	uint32_t spi_divider;
	const struct device *clock_ctrl =  DEVICE_DT_GET_ANY(bflb_clock_controller);

	/* bclk -> spiclk */
	spi_divider = sys_read32(GLB_BASE + GLB_CLK_CFG3_OFFSET);
	spi_divider = (spi_divider & GLB_SPI_CLK_DIV_MSK) >> GLB_SPI_CLK_DIV_POS;

	clock_control_get_rate(clock_ctrl, (void *)BFLB_CLKID_CLK_BCLK, &uclk);

	return uclk / (spi_divider + 1);
}

#elif defined(CONFIG_SOC_SERIES_BL61X)

static uint32_t spi_bflb_get_clk(void)
{
	uint32_t uclk;
	uint32_t spi_divider;
	uint32_t spi_mux;
	const struct device *clock_ctrl =  DEVICE_DT_GET_ANY(bflb_clock_controller);
	uint32_t main_clock = clock_bflb_get_root_clock();

	/* mux -> spiclk */
	spi_divider = sys_read32(GLB_BASE + GLB_SPI_CFG0_OFFSET);
	spi_mux = (spi_divider & GLB_SPI_CLK_SEL_MSK) >> GLB_SPI_CLK_SEL_POS;
	spi_divider = (spi_divider & GLB_SPI_CLK_DIV_MSK) >> GLB_SPI_CLK_DIV_POS;

	if (spi_mux) {
		if (main_clock == BFLB_MAIN_CLOCK_RC32M
			|| main_clock == BFLB_MAIN_CLOCK_PLL_RC32M) {
			return BFLB_RC32M_FREQUENCY / (spi_divider + 1);
		}
		clock_control_get_rate(clock_ctrl, (void *)BFLB_CLKID_CLK_CRYSTAL, &uclk);

		return uclk / (spi_divider + 1);
	}
	/* This shouldn't happen in this driver */
	return 0;
}
#else
#error Unsupported platform
#endif

static bool spi_bflb_bus_busy(const struct device *dev)
{
	uint32_t tmp = 0;
	const struct spi_bflb_cfg *config = dev->config;

	tmp = sys_read32(config->base + SPI_BUS_BUSY_OFFSET);
	return (tmp & SPI_STS_SPI_BUS_BUSY) == 0 ? false : true;
}

static void spi_bflb_trigger_master(const struct device *dev)
{
	uint32_t tmp = 0;
	const struct spi_bflb_cfg *config = dev->config;

	tmp = sys_read32(config->base + SPI_CONFIG_OFFSET);
	tmp |= SPI_CR_SPI_M_EN;
	sys_write32(tmp, config->base + SPI_CONFIG_OFFSET);
}

static void spi_bflb_detrigger_master(const struct device *dev)
{
	uint32_t tmp = 0;
	const struct spi_bflb_cfg *config = dev->config;

	tmp = sys_read32(config->base + SPI_CONFIG_OFFSET);
	tmp &= ~SPI_CR_SPI_M_EN;
	sys_write32(tmp, config->base + SPI_CONFIG_OFFSET);
}

static int spi_bflb_configure_freqs(const struct device *dev, const struct spi_config *config)
{
	const struct spi_bflb_cfg *cfg = dev->config;
	uint32_t tmp = 0;
	uint32_t tmpb = 0;

	/* on BL61x it could be 80MHz but we use XCLK so it caps at XCLK */
	if (config->frequency > 40 * 1000 * 1000) {
		return -EINVAL;
	}

#if defined(CONFIG_SOC_SERIES_BL61X)
	tmp = sys_read32(GLB_BASE + GLB_SPI_CFG0_OFFSET);
	/* set div to 0 (1) */
	tmp = tmp & GLB_SPI_CLK_DIV_UMSK;
	tmp |= 0 << GLB_SPI_CLK_DIV_POS;
	tmp |= GLB_SPI_CLK_EN_MSK;
	/* select XCLK */
	tmp &= GLB_SPI_CLK_SEL_UMSK;
	tmp |= 1U <<  GLB_SPI_CLK_SEL_POS;
	sys_write32(tmp, GLB_BASE + GLB_SPI_CFG0_OFFSET);
#else
	tmp = sys_read32(GLB_BASE + GLB_CLK_CFG3_OFFSET);
	tmp |= GLB_SPI_CLK_EN_MSK;
	/* 40 MHz speed divider is good down to 156250 Hz */
	tmp &= ~GLB_SPI_CLK_DIV_MSK;
	tmp |= 1 << GLB_SPI_CLK_DIV_POS;
	sys_write32(tmp, GLB_BASE + GLB_CLK_CFG3_OFFSET);
#endif
	tmp = (spi_bflb_get_clk() / 2 * 10 / config->frequency + 5) / 10;
	tmp = (tmp) ? (tmp - 1) : 0;
	tmp = (tmp > 0xff) ? 0xff : tmp;

	tmpb = 0;
	tmpb |= tmp << SPI_CR_SPI_PRD_D_PH_0_SHIFT;
	tmpb |= tmp << SPI_CR_SPI_PRD_D_PH_1_SHIFT;
	tmpb |= tmp << SPI_CR_SPI_PRD_S_SHIFT;
	tmpb |= tmp << SPI_CR_SPI_PRD_P_SHIFT;
	sys_write32(tmpb, cfg->base + SPI_PRD_0_OFFSET);

	tmpb = sys_read32(cfg->base + SPI_PRD_1_OFFSET);
	tmpb &= ~SPI_CR_SPI_PRD_I_MASK;
	tmpb |= (tmp) << SPI_CR_SPI_PRD_I_SHIFT;
	sys_write32(tmpb, cfg->base + SPI_PRD_1_OFFSET);

	return 0;
}


static int spi_bflb_configure(const struct device *dev, const struct spi_config *config)
{
	int rc = 0;
	const struct spi_bflb_cfg *cfg = dev->config;
	struct spi_bflb_data *data = dev->data;
	uint32_t tmp = 0;
	uint32_t framesize = 0;

	if (spi_context_configured(&data->ctx, config)) {
		return 0;
	}

	tmp = sys_read32(cfg->base + SPI_CONFIG_OFFSET);
	/* detrigger SPI slave and master*/
	tmp &= ~SPI_CR_SPI_S_EN;
	tmp &= ~SPI_CR_SPI_M_EN;

	sys_write32(tmp, cfg->base + SPI_CONFIG_OFFSET);

	tmp = sys_read32(GLB_SPI_MODE_ADDRESS);
	if (SPI_OP_MODE_GET(config->operation) == SPI_OP_MODE_MASTER) {
		tmp |= 1U << GLB_REG_SPI_0_MASTER_MODE_POS;
	} else {
		/* TODO: slave mode */
		return -ENOTSUP;
		tmp &= ~(1U << GLB_REG_SPI_0_MASTER_MODE_POS);
	}
	sys_write32(tmp, GLB_SPI_MODE_ADDRESS);


	rc = spi_bflb_configure_freqs(dev, config);
	if (rc != 0) {
		return rc;
	}

	tmp = sys_read32(cfg->base + SPI_CONFIG_OFFSET);
	/* Disable deglitch */
	tmp &= ~SPI_CR_SPI_DEG_EN;
	tmp &= ~SPI_CR_SPI_DEG_CNT_MASK;
	/* enable continue transaction as long as valid */
	tmp |= SPI_CR_SPI_M_CONT_EN;
	/* disable ignore RX */
	tmp &= ~SPI_CR_SPI_RXD_IGNR_EN;

	/* bit order*/
	if ((config->operation & SPI_TRANSFER_LSB) != 0) {
		tmp |= SPI_CR_SPI_BIT_INV;
	} else {
		tmp &= ~SPI_CR_SPI_BIT_INV;
	}

	/* disable byte order inversion */
	tmp &= ~SPI_CR_SPI_BYTE_INV;

	if ((config->operation & SPI_MODE_CPOL) == 0
		&& (config->operation & SPI_MODE_CPHA) == 0) {
		tmp &= ~SPI_CR_SPI_SCLK_POL;
		tmp |= SPI_CR_SPI_SCLK_PH;
	} else if ((config->operation & SPI_MODE_CPOL) == 0
		&& (config->operation & SPI_MODE_CPHA) != 0) {
		tmp &= ~SPI_CR_SPI_SCLK_POL;
		tmp &= ~SPI_CR_SPI_SCLK_PH;
	} else if ((config->operation & SPI_MODE_CPOL) != 0
		&& (config->operation & SPI_MODE_CPHA) == 0) {
		tmp |= SPI_CR_SPI_SCLK_POL;
		tmp |= SPI_CR_SPI_SCLK_PH;
	} else if ((config->operation & SPI_MODE_CPOL) != 0
		&& (config->operation & SPI_MODE_CPHA) != 0) {
		tmp |= SPI_CR_SPI_SCLK_POL;
		tmp &= ~SPI_CR_SPI_SCLK_PH;
	}

	/* set expected data frame size*/
	framesize = SPI_WORD_SIZE_GET(config->operation);
	if (framesize != 8 && framesize != 16
		&& framesize != 24 && framesize != 32) {
		return -EINVAL;
	}
	tmp &= ~SPI_CR_SPI_FRAME_SIZE_MASK;
	if (framesize == 16) {
		tmp |=  1 << SPI_CR_SPI_FRAME_SIZE_SHIFT;
	} else if (framesize == 24) {
		tmp |= 2 << SPI_CR_SPI_FRAME_SIZE_SHIFT;
	} else if (framesize == 32) {
		tmp |= 3 << SPI_CR_SPI_FRAME_SIZE_SHIFT;
	}
	/* detrigger SPI slave and master*/
	tmp &= ~SPI_CR_SPI_S_EN;
	tmp &= ~SPI_CR_SPI_M_EN;

	sys_write32(tmp, cfg->base + SPI_CONFIG_OFFSET);

	/* clear fifo and make sure DMA is disabled */
	tmp = sys_read32(cfg->base + SPI_FIFO_CONFIG_0_OFFSET);
	tmp |= SPI_TX_FIFO_CLR;
	tmp |= SPI_RX_FIFO_CLR;
	tmp &= ~SPI_DMA_TX_EN;
	tmp &= ~SPI_DMA_RX_EN;
	sys_write32(tmp, cfg->base + SPI_FIFO_CONFIG_0_OFFSET);

	/* fifo thresholds to 0 and 8
	 * tx triggers when pushed elements > 8
	 * rx trigger when received elements > 0
	 */
	tmp = sys_read32(cfg->base + SPI_FIFO_CONFIG_1_OFFSET);
	tmp &= ~SPI_TX_FIFO_TH_MASK;
	tmp &= ~SPI_RX_FIFO_TH_MASK;
	tmp |= 8 << SPI_TX_FIFO_TH_SHIFT;
	sys_write32(tmp, cfg->base  + SPI_FIFO_CONFIG_1_OFFSET);

	tmp = sys_read32(cfg->base + SPI_INT_STS_OFFSET);

	/* enable all interrupts */
	tmp |= (SPI_CR_SPI_END_EN |
		SPI_CR_SPI_TXF_EN |
		SPI_CR_SPI_RXF_EN |
		SPI_CR_SPI_STO_EN |
		SPI_CR_SPI_TXU_EN |
		SPI_CR_SPI_FER_EN);
	/* mask all interrupts */
	tmp |= (SPI_CR_SPI_STO_MASK |
		SPI_CR_SPI_TXU_MASK |
		SPI_CR_SPI_FER_MASK |
		SPI_CR_SPI_TXF_MASK |
		SPI_CR_SPI_END_MASK |
		SPI_CR_SPI_RXF_MASK);

	sys_write32(tmp, cfg->base + SPI_INT_STS_OFFSET);

	data->ctx.config = config;

	return 0;
}

static uint32_t spi_bflb_from_buff_to_frame(const struct device *dev,
					    void *buff)
{
	const struct spi_bflb_cfg *cfg = dev->config;
	uint32_t tmp = 0;
	uint32_t frame = 0;

	tmp = sys_read32(cfg->base + SPI_CONFIG_OFFSET);
	tmp = (tmp & SPI_CR_SPI_FRAME_SIZE_MASK)
		>> SPI_CR_SPI_FRAME_SIZE_SHIFT;

	if (tmp == 0) {
		frame = *(uint32_t *)buff & 0xFF;
	} else if (tmp == 1) {
		frame = *(uint32_t *)buff & 0xFFFF;
	} else if (tmp == 2) {
		frame = *(uint32_t *)buff & 0xFFFFFF;
	} else if (tmp == 3) {
		frame = *(uint32_t *)buff;
	}

	return frame;
}

static void spi_bflb_copy_frame_to_buff(const struct device *dev,
					uint32_t *frame, void *buff)
{
	const struct spi_bflb_cfg *cfg = dev->config;
	uint32_t tmp = 0;

	tmp = sys_read32(cfg->base + SPI_CONFIG_OFFSET);
	tmp = (tmp & SPI_CR_SPI_FRAME_SIZE_MASK)
		>> SPI_CR_SPI_FRAME_SIZE_SHIFT;

	if (tmp == 0) {
		(*(uint8_t *)buff) = *frame & 0xFF;
	} else if (tmp == 1) {
		(*(uint16_t *)buff) = *frame & 0xFFFF;
	} else if (tmp == 2) {
		(*(uint16_t *)buff) = *frame & 0xFFFF;
		(*(uint8_t *)((uint8_t *)buff + 2)) = (*frame & 0xFF0000) >> 16;
	} else if (tmp == 3) {
		(*(uint32_t *)buff) = *frame;
	}
}

static int spi_bflb_transceive_sync_one(const struct device *dev,
					const struct spi_buf *tx_buf,
					const struct spi_buf *rx_buf)
{
	const struct spi_bflb_cfg *cfg = dev->config;
	uint32_t tmp = 0;
	uint32_t rx_x = 0;
	uint32_t tx_x = 0;
	uint8_t frame_size = 0;
	uint8_t rx_cnt = 0;
	uint8_t tx_cnt = 0;
	uint32_t zero = 0;

	tmp = sys_read32(cfg->base + SPI_CONFIG_OFFSET);
	/* in bytes */
	frame_size = ((tmp & SPI_CR_SPI_FRAME_SIZE_MASK)
		>> SPI_CR_SPI_FRAME_SIZE_SHIFT) + 1;

	while (rx_x < rx_buf->len || tx_x < tx_buf->len) {
		tmp = sys_read32(cfg->base + SPI_FIFO_CONFIG_1_OFFSET);
		rx_cnt = (tmp & SPI_RX_FIFO_CNT_MASK)
			>> SPI_RX_FIFO_CNT_SHIFT;
		tx_cnt = (tmp & SPI_TX_FIFO_CNT_MASK)
			>> SPI_TX_FIFO_CNT_SHIFT;

		if (rx_x < rx_buf->len && rx_cnt > 0) {
			tmp = sys_read32(cfg->base + SPI_FIFO_RDATA_OFFSET);
			if (rx_buf->buf != NULL) {
				spi_bflb_copy_frame_to_buff(dev, &tmp,
					(uint8_t *)rx_buf->buf + rx_x);
			}
			rx_x += frame_size;
		}

		if (tx_x < tx_buf->len && tx_cnt > 0) {
			if (tx_buf->buf != NULL) {
				sys_write32(spi_bflb_from_buff_to_frame(dev,
					(uint8_t *)tx_buf->buf + tx_x),
				cfg->base + SPI_FIFO_WDATA_OFFSET);
			} else {
				sys_write32(
				spi_bflb_from_buff_to_frame(dev, &zero),
				cfg->base + SPI_FIFO_WDATA_OFFSET);
			}
			tx_x += frame_size;
		}
	}
	return 0;
}

static int spi_bflb_transceive_sync(const struct device *dev,
					const struct spi_config *config,
					const struct spi_buf_set *tx_bufs,
					const struct spi_buf_set *rx_bufs)
{
	int rc = 0;
	uint32_t tmp = 0;
	const struct spi_bflb_cfg *cfg = dev->config;
	struct spi_bflb_data *data = dev->data;
	k_timepoint_t end_timeout =
		sys_timepoint_calc(K_MSEC(SPI_WAIT_TIMEOUT_MS));
	uint32_t rx_cnt = 0, tx_cnt = 0;
	struct spi_buf empty_buf = {0};
	struct spi_buf_set empty_buf_set = {0};

	spi_context_lock(&data->ctx, false, NULL, NULL, config);

	rc = spi_bflb_configure(dev, config);
	if (rc != 0) {
		return rc;
	}

	if (rx_bufs == NULL) {
		rx_bufs = &empty_buf_set;
	}

	if (tx_bufs == NULL) {
		tx_bufs = &empty_buf_set;
	}

	spi_bflb_detrigger_master(dev);

	while (spi_bflb_bus_busy(dev) && !sys_timepoint_expired(end_timeout)) {
	}

	if (sys_timepoint_expired(end_timeout)) {
		return -ETIMEDOUT;
	}

	/* clean up */
	tmp = sys_read32(cfg->base + SPI_FIFO_CONFIG_0_OFFSET);
	tmp |= SPI_TX_FIFO_CLR;
	tmp |= SPI_RX_FIFO_CLR;
	sys_write32(tmp, cfg->base + SPI_FIFO_CONFIG_0_OFFSET);

	spi_context_cs_control(&data->ctx, true);

	spi_bflb_trigger_master(dev);

	while (rx_cnt < rx_bufs->count || tx_cnt < tx_bufs->count) {
		if (rx_cnt < rx_bufs->count && tx_cnt < tx_bufs->count) {
			spi_bflb_transceive_sync_one(dev,
				&(tx_bufs->buffers[tx_cnt]),
				&(rx_bufs->buffers[rx_cnt]));
			tx_cnt++;
			rx_cnt++;
		} else if (rx_cnt < rx_bufs->count) {
			empty_buf.len = rx_bufs->buffers[rx_cnt].len;
			spi_bflb_transceive_sync_one(dev,
				&empty_buf,
				&(rx_bufs->buffers[rx_cnt]));
			rx_cnt++;
		} else if (tx_cnt < tx_bufs->count) {
			empty_buf.len = tx_bufs->buffers[tx_cnt].len;
			spi_bflb_transceive_sync_one(dev,
				&(tx_bufs->buffers[tx_cnt]),
				&empty_buf);
			tx_cnt++;
		}
	}
	do {
		tmp = sys_read32(cfg->base + SPI_FIFO_CONFIG_1_OFFSET);
		tx_cnt = (tmp & SPI_TX_FIFO_CNT_MASK)
			>> SPI_TX_FIFO_CNT_SHIFT;
	} while (tx_cnt < SPI_FIFO_SIZE);

	spi_context_cs_control(&data->ctx, false);
	spi_bflb_detrigger_master(dev);
	spi_context_release(&data->ctx, rc);
	return rc;
}

static int spi_bflb_init(const struct device *dev)
{
	int rc = 0;
	const struct spi_bflb_cfg *cfg = dev->config;
	struct spi_bflb_data *data = dev->data;
	uint32_t tmp = 0;

	rc = pinctrl_apply_state(cfg->pincfg, PINCTRL_STATE_DEFAULT);
	if (rc < 0) {
		return rc;
	}

	rc = spi_context_cs_configure_all(&data->ctx);
	if (rc < 0) {
		return rc;
	}

	tmp = sys_read32(cfg->base + SPI_INT_STS_OFFSET);

	/* mask interrupts */
	tmp |= (SPI_CR_SPI_STO_MASK |
		SPI_CR_SPI_TXU_MASK |
		SPI_CR_SPI_FER_MASK |
		SPI_CR_SPI_TXF_MASK |
		SPI_CR_SPI_RXF_MASK |
		SPI_CR_SPI_END_MASK);

	sys_write32(tmp, cfg->base + SPI_INT_STS_OFFSET);
	cfg->irq_config_func(dev);

	spi_context_unlock_unconditionally(&data->ctx);

	return rc;
}


static int spi_bflb_release(const struct device *dev,
		       const struct spi_config *config)
{
	struct spi_bflb_data *data = dev->data;

	spi_context_unlock_unconditionally(&data->ctx);
	return 0;
}

static void spi_bflb_isr(const struct device *dev)
{
	const struct spi_bflb_cfg *config = dev->config;
	uint32_t tmp = 0;

	tmp = sys_read32(config->base + SPI_INT_STS_OFFSET);

	if ((tmp & SPI_END_INT) != 0) {
		tmp |= SPI_CR_SPI_END_CLR;
	}
	sys_write32(tmp, config->base + SPI_INT_STS_OFFSET);
}

static const struct spi_driver_api spi_bflb_driver_api = {
	.transceive = spi_bflb_transceive_sync,
#ifdef CONFIG_SPI_RTIO
	.iodev_submit = spi_rtio_iodev_default_submit,
#endif
	.release = spi_bflb_release,
};


/* Device instantiation */

#define SPI_BFLB_IRQ_HANDLER_DECL(n)					\
	static void spi_bflb_config_func_##n(const struct device *dev);
#define SPI_BFLB_IRQ_HANDLER_FUNC(n)					\
	.irq_config_func = spi_bflb_config_func_##n
#define SPI_BFLB_IRQ_HANDLER(n)						\
	static void spi_bflb_config_func_##n(const struct device *dev)	\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(n),				\
			    DT_INST_IRQ(n, priority),			\
			    spi_bflb_isr,				\
			    DEVICE_DT_INST_GET(n),			\
			    0);						\
		irq_enable(DT_INST_IRQN(n));				\
	}


#define SPI_BFLB_INIT(n)                                                                           \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	SPI_BFLB_IRQ_HANDLER_DECL(n)                                                               \
	static struct spi_bflb_data spi##n##_bflb_data = {                                         \
		SPI_CONTEXT_INIT_LOCK(spi##n##_bflb_data, ctx),                                    \
		SPI_CONTEXT_INIT_SYNC(spi##n##_bflb_data, ctx),                                    \
		SPI_CONTEXT_CS_GPIOS_INITIALIZE(DT_DRV_INST(n), ctx)                               \
	};                                                                                         \
	static const struct spi_bflb_cfg spi_bflb_cfg_##n = {                                      \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                       \
		.base = DT_INST_REG_ADDR(n),                                                       \
		SPI_BFLB_IRQ_HANDLER_FUNC(n)                                                       \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n,                                                                   \
			spi_bflb_init,                                                             \
			NULL,                                                                      \
			&spi##n##_bflb_data,                                                       \
			&spi_bflb_cfg_##n,                                                         \
			POST_KERNEL,                                                               \
			CONFIG_SPI_INIT_PRIORITY,                                                  \
			&spi_bflb_driver_api);                                                     \
	SPI_BFLB_IRQ_HANDLER(n)

DT_INST_FOREACH_STATUS_OKAY(SPI_BFLB_INIT)
