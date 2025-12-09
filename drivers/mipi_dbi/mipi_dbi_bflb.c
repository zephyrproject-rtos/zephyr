/*
 * Copyright 2025 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT bflb_dbi

#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/drivers/mipi_dbi.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/cache.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mipi_dbi_bflb, CONFIG_MIPI_DBI_LOG_LEVEL);

#include <soc.h>
#include <bflb_soc.h>
#include <glb_reg.h>
#include <hbn_reg.h>
#include <common_defines.h>
#include <zephyr/dt-bindings/clock/bflb_clock_common.h>
#include <zephyr/drivers/clock_control/clock_control_bflb_common.h>
#include <zephyr/dt-bindings/clock/bflb_bl61x_clock.h>
#include <bouffalolab/common/dbi_reg.h>
#include <bouffalolab/common/dma_reg.h>

#define DBI_MAX_FREQ		MHZ(80)
#define DBI_MAX_XCLK_FREQ	MHZ(20)
#define DBI_MAX_INPUT_FREQ	MHZ(160)
#define DBI_WAIT_TIMEOUT_MS	500

#define DBI_MODE_QSPI	3U
#define DBI_MODE_C_3W	2U
#define DBI_MODE_C_4W	1U
#define DBI_MODE_B_8B	0U

#define DBI_FIFO_FORMAT_BGR	0U
#define DBI_FIFO_FORMAT_RGB	1U
#define DBI_FIFO_FORMAT_RnBGR	4U
#define DBI_FIFO_FORMAT_BnRGB	5U

#define DBI_MAX_CMD_WRITE_LEN	256

#define DBI_USEC_TO_SEC		1000000
#define DBI_USEC_TO_MSEC	1000

/* Context switch takes about 10 microsecs, allocate 10 times that */
#define DBI_UNEXPECTED_WAIT_TIME 100

struct mipi_dbi_bflb_config {
	uint32_t base;
	const struct pinctrl_dev_config *pincfg;
	void (*irq_config_func)(const struct device *dev);
	const struct gpio_dt_spec reset_gpio;
	const struct gpio_dt_spec dc_gpio;
	const struct gpio_dt_spec *cs_gpios;
	size_t num_cs_gpios;
};

struct mipi_dbi_bflb_dma_data {
	const struct device *dev;
	int channel;
	struct dma_config config;
	struct dma_block_config block;
	uint8_t	*buf;
	size_t	len;
};

struct mipi_dbi_bflb_data {
	struct mipi_dbi_config configured;
	struct k_mutex lock;
	struct mipi_dbi_bflb_dma_data dma;
	bool transfer_done;
};

static inline bool mipi_dbi_has_pin(const struct gpio_dt_spec *spec)
{
	return spec->port != NULL;
}

static inline void mipi_dbi_bflb_cs_control(const struct device *dev,
					    const struct mipi_dbi_config *config,
					    bool on, bool force_off)
{
	const struct mipi_dbi_bflb_config *cfg = dev->config;

	if (config->config.slave < cfg->num_cs_gpios) {
		if (mipi_dbi_has_pin(&cfg->cs_gpios[config->config.slave])) {
			if (on) {
				gpio_pin_set_dt(&cfg->cs_gpios[config->config.slave], 1);
				k_busy_wait(config->config.cs.delay);
			} else {
				if (!force_off
				    && (config->config.operation & SPI_HOLD_ON_CS
					|| IS_ENABLED(CONFIG_MIPI_DBI_BFLB_ASSUME_HOLD_ON_CS))) {
					return;
				}
				k_busy_wait(config->config.cs.delay);
				gpio_pin_set_dt(&cfg->cs_gpios[config->config.slave], 0);
			}
		}
	}
}

static bool mipi_dbi_bflb_bus_busy(const struct device *dev)
{
	const struct mipi_dbi_bflb_config *config = dev->config;
	volatile uint32_t tmp;

	tmp = sys_read32(config->base + DBI_CONFIG_OFFSET);

	return (tmp & DBI_STS_DBI_BUS_BUSY) != 0;
}

static void mipi_dbi_bflb_clear_fifo(const struct device *dev)
{
	const struct mipi_dbi_bflb_config *config = dev->config;
	volatile uint32_t tmp;

	tmp = sys_read32(config->base + DBI_FIFO_CONFIG_0_OFFSET);
	tmp |= DBI_TX_FIFO_CLR;
	sys_write32(tmp, config->base + DBI_FIFO_CONFIG_0_OFFSET);
}

static int mipi_dbi_bflb_trigger(const struct device *dev)
{
	volatile uint32_t tmp;
	const struct mipi_dbi_bflb_config *config = dev->config;
	k_timepoint_t end_timeout =
		sys_timepoint_calc(K_MSEC(DBI_WAIT_TIMEOUT_MS));

	while (mipi_dbi_bflb_bus_busy(dev) && !sys_timepoint_expired(end_timeout)) {
		k_usleep(1);
	}
	if (sys_timepoint_expired(end_timeout)) {
		return -ETIMEDOUT;
	}

	tmp = sys_read32(config->base + DBI_CONFIG_OFFSET);
	tmp |= DBI_CR_DBI_EN;
	sys_write32(tmp, config->base + DBI_CONFIG_OFFSET);

	return 0;
}

static int mipi_dbi_bflb_detrigger(const struct device *dev)
{
	volatile uint32_t tmp;
	const struct mipi_dbi_bflb_config *config = dev->config;
	k_timepoint_t end_timeout =
		sys_timepoint_calc(K_MSEC(DBI_WAIT_TIMEOUT_MS));

	while (mipi_dbi_bflb_bus_busy(dev) && !sys_timepoint_expired(end_timeout)) {
		k_usleep(1);
	}
	if (sys_timepoint_expired(end_timeout)) {
		return -ETIMEDOUT;
	}

	tmp = sys_read32(config->base + DBI_CONFIG_OFFSET);
	tmp &= ~DBI_CR_DBI_EN;
	sys_write32(tmp, config->base + DBI_CONFIG_OFFSET);

	return 0;
}

static uint32_t mipi_dbi_get_clk(void)
{
	uint32_t uclk;
	uint32_t dbi_divider;
	uint32_t dbi_mux;
	const struct device *clock_ctrl =  DEVICE_DT_GET_ANY(bflb_clock_controller);
	uint32_t main_clock = clock_bflb_get_root_clock();

	/* mux -> dbiiclk */
	dbi_divider = sys_read32(GLB_BASE + GLB_DBI_CFG0_OFFSET);
	dbi_mux = (dbi_divider & GLB_DBI_CLK_SEL_MSK) >> GLB_DBI_CLK_SEL_POS;
	dbi_divider = (dbi_divider & GLB_DBI_CLK_DIV_MSK) >> GLB_DBI_CLK_DIV_POS;

	if (dbi_mux > 0) {
		if (main_clock == BFLB_MAIN_CLOCK_RC32M
			|| main_clock == BFLB_MAIN_CLOCK_PLL_RC32M) {
			return BFLB_RC32M_FREQUENCY / (dbi_divider + 1);
		}
		clock_control_get_rate(clock_ctrl, (void *)BFLB_CLKID_CLK_CRYSTAL, &uclk);
		return uclk / (dbi_divider + 1);
	}

	clock_control_get_rate(clock_ctrl, (void *)BL61X_CLKID_CLK_160M, &uclk);

	return uclk / (dbi_divider + 1);
}

static int mipi_dbi_bflb_configure_freqs(const struct device *dev,
					 const struct mipi_dbi_config *config)
{
	const struct mipi_dbi_bflb_config *cfg = dev->config;
	const struct device *clock_ctrl =  DEVICE_DT_GET_ANY(bflb_clock_controller);
	volatile uint32_t tmp;
	uint32_t div_0, div_1;
	uint32_t clkdiv = 0;

	tmp = sys_read32(GLB_BASE + GLB_DBI_CFG0_OFFSET);
	tmp &= GLB_DBI_CLK_DIV_UMSK;
	tmp &= GLB_DBI_CLK_SEL_UMSK;
	tmp &= GLB_DBI_CLK_EN_UMSK;
	if (config->config.frequency > DBI_MAX_FREQ) {
		return -EINVAL;
	} else if (config->config.frequency > DBI_MAX_XCLK_FREQ
		&& clock_control_get_rate(clock_ctrl, (void *)BL61X_CLKID_CLK_160M, &div_0) >= 0) {
		/* select PLL mux */
		tmp |= 0U <<  GLB_DBI_CLK_SEL_POS;
	} else {
		/* select XCLK */
		tmp |= 1U <<  GLB_DBI_CLK_SEL_POS;
	}
	sys_write32(tmp, GLB_BASE + GLB_DBI_CFG0_OFFSET);

	while (mipi_dbi_get_clk() > DBI_MAX_INPUT_FREQ) {
		clkdiv++;
		tmp = sys_read32(GLB_BASE + GLB_DBI_CFG0_OFFSET);
		tmp &= GLB_DBI_CLK_DIV_UMSK;
		tmp |= clkdiv << GLB_DBI_CLK_DIV_POS;
		sys_write32(tmp, GLB_BASE + GLB_DBI_CFG0_OFFSET);
	}

	tmp = sys_read32(GLB_BASE + GLB_DBI_CFG0_OFFSET);
	tmp |= GLB_DBI_CLK_EN_MSK;
	sys_write32(tmp, GLB_BASE + GLB_DBI_CFG0_OFFSET);

	/* Following is directly from SDK */
	tmp = (mipi_dbi_get_clk() * 10 / config->config.frequency + 5) / 10;
	div_0 = (tmp + 1) / 2;
	div_0 = (div_0 > 0xff) ? (0xff) : ((div_0 > 0) ? (div_0 - 1) : 0);
	div_1 = (tmp) / 2;
	div_1 = (div_1 > 0xff) ? (0xff) : ((div_1 > 0) ? (div_1 - 1) : 0);

	tmp = 0;
	tmp |= div_0 << DBI_CR_DBI_PRD_S_SHIFT;
	tmp |= div_1 << DBI_CR_DBI_PRD_I_SHIFT;
	tmp |= div_0 << DBI_CR_DBI_PRD_D_PH_0_SHIFT;
	tmp |= div_1 << DBI_CR_DBI_PRD_D_PH_1_SHIFT;
	sys_write32(tmp, cfg->base + DBI_PRD_OFFSET);

	return 0;
}

static int mipi_dbi_bflb_configure(const struct device *dev, const struct mipi_dbi_config *config)
{
	const struct mipi_dbi_bflb_config *cfg = dev->config;
	struct mipi_dbi_bflb_data *data = dev->data;
	int ret;
	volatile uint32_t tmp;

	ret = mipi_dbi_bflb_detrigger(dev);
	if (ret < 0) {
		return ret;
	}

	if (data->configured.config.frequency != 0 && config->mode == data->configured.mode
	    && config->config.frequency == data->configured.config.frequency
	    && config->config.operation == data->configured.config.operation
	    && config->config.cs.gpio.port == data->configured.config.cs.gpio.port
	    && config->config.cs.gpio.pin == data->configured.config.cs.gpio.pin
	    && config->config.slave == data->configured.config.slave) {
		return 0;
	}

	tmp = sys_read32(cfg->base + DBI_CONFIG_OFFSET);
	tmp &= ~DBI_CR_DBI_SEL_MASK;
	switch (config->mode) {
	case MIPI_DBI_MODE_SPI_3WIRE:
		tmp |= DBI_MODE_C_3W << DBI_CR_DBI_SEL_SHIFT;
		break;
	case MIPI_DBI_MODE_SPI_4WIRE:
		tmp |= DBI_MODE_C_4W << DBI_CR_DBI_SEL_SHIFT;
		break;
	case MIPI_DBI_MODE_8080_BUS_8_BIT:
		tmp |= DBI_MODE_B_8B << DBI_CR_DBI_SEL_SHIFT;
		/* The controller supports this but the driver does not */
		return -ENOSYS;
	default:
		return -ENOTSUP;
	};

	/* This is inverse to SPI, but correct to normal CPOL and CPHA settings */
	if ((config->config.operation & SPI_MODE_CPOL) == 0
		&& (config->config.operation & SPI_MODE_CPHA) == 0) {
		tmp &= ~DBI_CR_DBI_SCL_POL;
		tmp &= ~DBI_CR_DBI_SCL_PH;
	} else if ((config->config.operation & SPI_MODE_CPOL) == 0
		&& (config->config.operation & SPI_MODE_CPHA) != 0) {
		tmp &= ~DBI_CR_DBI_SCL_POL;
		tmp |= DBI_CR_DBI_SCL_PH;
	} else if ((config->config.operation & SPI_MODE_CPOL) != 0
		&& (config->config.operation & SPI_MODE_CPHA) == 0) {
		tmp |= DBI_CR_DBI_SCL_POL;
		tmp &= !DBI_CR_DBI_SCL_PH;
	} else if ((config->config.operation & SPI_MODE_CPOL) != 0
		&& (config->config.operation & SPI_MODE_CPHA) != 0) {
		tmp |= DBI_CR_DBI_SCL_POL;
		tmp |= DBI_CR_DBI_SCL_PH;
	}

	/* Keep CS asserted between pixels */
	tmp |= DBI_CR_DBI_CONT_EN;
	/* No dummy clocks */
	tmp &= ~DBI_CR_DBI_DMY_EN;
	/* Keep CS on until we detrigger */
	tmp |= DBI_CR_DBI_CS_STRETCH;

	sys_write32(tmp, cfg->base + DBI_CONFIG_OFFSET);

	ret = mipi_dbi_bflb_configure_freqs(dev, config);
	if (ret != 0) {
		return ret;
	}

	/* TODO mode B pixel format handling */
	tmp = sys_read32(cfg->base + DBI_PIX_CNT_OFFSET);
	/* 4 bytes per pixel */
	tmp |= DBI_CR_DBI_PIX_FORMAT;
	sys_write32(tmp, cfg->base + DBI_PIX_CNT_OFFSET);

	tmp = sys_read32(cfg->base + DBI_FIFO_CONFIG_0_OFFSET);
	tmp &= ~DBI_FIFO_FORMAT_MASK;
	if ((config->config.operation & SPI_TRANSFER_LSB) != 0) {
		tmp |= DBI_FIFO_FORMAT_RGB << DBI_FIFO_FORMAT_SHIFT;
	} else {
		tmp |= DBI_FIFO_FORMAT_BGR << DBI_FIFO_FORMAT_SHIFT;
	}
	tmp |= DBI_TX_FIFO_CLR;
	tmp &= ~DBI_FIFO_YUV_MODE;
	tmp &= ~DBI_DMA_TX_EN;
	sys_write32(tmp, cfg->base + DBI_FIFO_CONFIG_0_OFFSET);

	tmp = sys_read32(cfg->base + DBI_FIFO_CONFIG_1_OFFSET);
	tmp &= ~DBI_TX_FIFO_TH_MASK;
	tmp |= 3U << DBI_TX_FIFO_TH_SHIFT;
	sys_write32(tmp, cfg->base + DBI_FIFO_CONFIG_1_OFFSET);

	data->configured = *config;

	return 0;
}

static int mipi_dbi_bflb_command_write(const struct device *dev,
				     const struct mipi_dbi_config *dbi_config,
				     uint8_t cmds, const uint8_t *buf, size_t len)
{
	const struct mipi_dbi_bflb_config *cfg = dev->config;
	struct mipi_dbi_bflb_data *data = dev->data;
	k_timepoint_t end_timeout =
		sys_timepoint_calc(K_MSEC(DBI_WAIT_TIMEOUT_MS));
	int ret;
	volatile uint32_t tmp;
	uint8_t j;
	size_t loaded = 0;
	bool triggered = false;

	if (len > DBI_MAX_CMD_WRITE_LEN) {
		LOG_ERR("Max command and data write len is 256 bytes");
		return -EINVAL;
	}

	ret = k_mutex_lock(&data->lock, K_FOREVER);
	if (ret < 0) {
		return ret;
	}

	mipi_dbi_bflb_cs_control(dev, dbi_config, true, false);

	ret = mipi_dbi_bflb_configure(dev, dbi_config);
	if (ret < 0) {
		goto out;
	}

	tmp = sys_read32(cfg->base + DBI_CONFIG_OFFSET);
	tmp &= ~DBI_CR_DBI_DAT_TP;
	tmp |= DBI_CR_DBI_CMD_EN;
	tmp |= DBI_CR_DBI_DAT_WR;
	if (len == 0) {
		tmp &= ~DBI_CR_DBI_DAT_EN;
	} else {
		tmp |= DBI_CR_DBI_DAT_EN;
		tmp &= ~DBI_CR_DBI_DAT_BC_MASK;
		tmp |= (len - 1) << DBI_CR_DBI_DAT_BC_SHIFT;
	}
	sys_write32(tmp, cfg->base + DBI_CONFIG_OFFSET);

	tmp = sys_read32(cfg->base + DBI_CMD_OFFSET);
	tmp &= ~DBI_CR_DBI_CMD_MASK;
	tmp |= cmds << DBI_CR_DBI_CMD_SHIFT;
	sys_write32(tmp, cfg->base + DBI_CMD_OFFSET);

	mipi_dbi_bflb_clear_fifo(dev);

	tmp = sys_read32(cfg->base + DBI_INT_STS_OFFSET);
	tmp |= DBI_CR_DBI_END_CLR;
	sys_write32(tmp, cfg->base + DBI_INT_STS_OFFSET);

	LOG_DBG("cmd write: %x, len: %d", cmds, len);

	if (mipi_dbi_has_pin(&cfg->dc_gpio)) {
		ret = gpio_pin_set_dt(&cfg->dc_gpio, 1);
		if (ret < 0) {
			goto out;
		}
	}

	if (len > 0) {
		while (loaded < len  && !sys_timepoint_expired(end_timeout)) {
			tmp = sys_read32(cfg->base + DBI_FIFO_CONFIG_1_OFFSET);
			if ((tmp & DBI_TX_FIFO_CNT_MASK) > 0) {
				tmp = 0;
				j = 0;
				for (; j < 4 && loaded + j < len; j++) {
					tmp |= (buf[loaded + j]) << (j * 8);
				}
				sys_write32(tmp, cfg->base + DBI_FIFO_WDATA_OFFSET);
				loaded += j;
			}
			if (!triggered) {
				mipi_dbi_bflb_trigger(dev);
				triggered = true;
			}
		}
	} else {
		mipi_dbi_bflb_trigger(dev);
	}

	do {
		clock_bflb_settle();
		tmp = sys_read32(cfg->base + DBI_INT_STS_OFFSET);
	} while ((tmp & DBI_END_INT) == 0  && !sys_timepoint_expired(end_timeout));

	if (sys_timepoint_expired(end_timeout)) {
		ret = -ETIMEDOUT;
		goto out;
	}
	tmp |= DBI_CR_DBI_END_CLR;
	sys_write32(tmp, cfg->base + DBI_INT_STS_OFFSET);

	ret = mipi_dbi_bflb_detrigger(dev);

	mipi_dbi_bflb_cs_control(dev, dbi_config, false, false);
out:
	k_mutex_unlock(&data->lock);

	return ret;
}

static int mipi_dbi_bflb_start_dma_write(const struct device *dev,
				       const struct mipi_dbi_config *dbi_config,
				       const uint8_t *buf, size_t len)
{
	const struct mipi_dbi_bflb_config *cfg = dev->config;
	struct mipi_dbi_bflb_data *data = dev->data;
	size_t len_todo = len > DBI_MAX_CMD_WRITE_LEN ? DBI_MAX_CMD_WRITE_LEN : len;
	int ret;
	volatile uint32_t tmp;

	dma_stop(data->dma.dev, data->dma.channel);

	LOG_DBG("DMA write: %lx, len %d", (uintptr_t)buf, len);

	data->dma.block.source_addr_adj = DMA_ADDR_ADJ_INCREMENT;
	data->dma.block.dest_addr_adj = DMA_ADDR_ADJ_NO_CHANGE;
	data->dma.block.source_address = (uint32_t)buf;
	data->dma.block.dest_address = cfg->base + DBI_FIFO_WDATA_OFFSET;
	data->dma.block.block_size = len_todo;

	data->dma.config.block_count = 1;
	data->dma.config.channel_direction = MEMORY_TO_PERIPHERAL;
	data->dma.config.source_data_size = 4;
	data->dma.config.dest_data_size = 4;
	data->dma.config.source_burst_length = 1;
	data->dma.config.dest_burst_length = 1;
	data->dma.config.head_block = &data->dma.block;
	data->dma.config.dma_callback = NULL;

	data->dma.buf = (uint8_t *)&(buf[len_todo]);
	data->dma.len = len - len_todo;

	tmp = sys_read32(cfg->base + DBI_CONFIG_OFFSET);
	tmp &= ~DBI_CR_DBI_DAT_TP;
	tmp &= ~DBI_CR_DBI_CMD_EN;
	tmp |= DBI_CR_DBI_DAT_WR;
	tmp |= DBI_CR_DBI_DAT_EN;
	tmp &= ~DBI_CR_DBI_DAT_BC_MASK;
	tmp |= (len_todo - 1) << DBI_CR_DBI_DAT_BC_SHIFT;
	sys_write32(tmp, cfg->base + DBI_CONFIG_OFFSET);

	tmp = sys_read32(cfg->base + DBI_FIFO_CONFIG_0_OFFSET);
	tmp |= DBI_DMA_TX_EN;
	sys_write32(tmp, cfg->base + DBI_FIFO_CONFIG_0_OFFSET);

	ret = dma_config(data->dma.dev, data->dma.channel, &data->dma.config);
	if (ret < 0) {
		return ret;
	}

	ret = dma_start(data->dma.dev, data->dma.channel);
	if (ret < 0) {
		return ret;
	}

	mipi_dbi_bflb_trigger(dev);

	return 0;
}

static int mipi_dbi_bflb_write_display(const struct device *dev,
				       const struct mipi_dbi_config *dbi_config,
				       const uint8_t *framebuf,
				       struct display_buffer_descriptor *desc,
				       enum display_pixel_format pixfmt)
{
	const struct mipi_dbi_bflb_config *cfg = dev->config;
	struct mipi_dbi_bflb_data *data = dev->data;
	int ret;
	volatile uint32_t tmp;
	int32_t expected_wait = (desc->buf_size * DBI_USEC_TO_SEC) / dbi_config->config.frequency;
	k_timepoint_t end_timeout =
		sys_timepoint_calc(K_MSEC(DBI_WAIT_TIMEOUT_MS));

	if ((desc->buf_size % 4) != 0) {
		LOG_ERR("Write size must be multiple of 4");
		return -ENOTSUP;
	}

	if ((expected_wait / DBI_USEC_TO_MSEC) > DBI_WAIT_TIMEOUT_MS) {
		LOG_ERR("Expected transfer time greater than timeout");
		return -EINVAL;
	}

	ret = k_mutex_lock(&data->lock, K_FOREVER);
	if (ret < 0) {
		return ret;
	}

	ret = mipi_dbi_bflb_configure(dev, dbi_config);
	if (ret < 0) {
		goto out;
	}

	mipi_dbi_bflb_cs_control(dev, dbi_config, true, false);

	if (mipi_dbi_has_pin(&cfg->dc_gpio)) {
		ret = gpio_pin_set_dt(&cfg->dc_gpio, 0);
		if (ret < 0) {
			goto out;
		}
	}

	mipi_dbi_bflb_clear_fifo(dev);

	tmp = sys_read32(cfg->base + DBI_INT_STS_OFFSET);
	tmp &= ~DBI_CR_DBI_END_MASK;
	tmp |= DBI_CR_DBI_END_CLR;
	sys_write32(tmp, cfg->base + DBI_INT_STS_OFFSET);

	tmp = sys_read32(cfg->base + DBI_FIFO_CONFIG_0_OFFSET);
	tmp &= ~DBI_DMA_TX_EN;
	sys_write32(tmp, cfg->base + DBI_FIFO_CONFIG_0_OFFSET);

	data->transfer_done = false;

	ret = mipi_dbi_bflb_start_dma_write(dev, dbi_config, framebuf, desc->buf_size);

	while (!data->transfer_done && !sys_timepoint_expired(end_timeout)) {
		k_usleep(expected_wait);
		expected_wait = DBI_UNEXPECTED_WAIT_TIME;
	}

	mipi_dbi_bflb_cs_control(dev, &data->configured, false, true);
out:
	k_mutex_unlock(&data->lock);

	return ret;
}

static int mipi_dbi_bflb_release(const struct device *dev,
				const struct mipi_dbi_config *dbi_config)
{
	const struct mipi_dbi_bflb_config *config = dev->config;
	struct mipi_dbi_bflb_data *data = dev->data;
	int ret;

	ret = k_mutex_lock(&data->lock, K_FOREVER);
	if (ret < 0) {
		return ret;
	}

	mipi_dbi_bflb_cs_control(dev, dbi_config, false, true);

	if (mipi_dbi_has_pin(&config->dc_gpio)) {
		ret = gpio_pin_set_dt(&config->dc_gpio, 0);
		if (ret < 0) {
			k_mutex_unlock(&data->lock);
			return ret;
		}
	}

	k_mutex_unlock(&data->lock);

	return 0;
}

static int mipi_dbi_bflb_reset(const struct device *dev, k_timeout_t delay)
{
	const struct mipi_dbi_bflb_config *config = dev->config;
	struct mipi_dbi_bflb_data *data = dev->data;
	int ret;

	if (!mipi_dbi_has_pin(&config->reset_gpio)) {
		return 0;
	}

	ret = k_mutex_lock(&data->lock, K_FOREVER);
	if (ret < 0) {
		return ret;
	}

	ret = gpio_pin_set_dt(&config->reset_gpio, 1);
	if (ret < 0) {
		goto out;
	}

	k_sleep(delay);

	ret = gpio_pin_set_dt(&config->reset_gpio, 0);

out:
	k_mutex_unlock(&data->lock);

	return ret;
}

static int mipi_dbi_bflb_init(const struct device *dev)
{
	const struct mipi_dbi_bflb_config *config = dev->config;
	struct mipi_dbi_bflb_data *data = dev->data;
	int ret;
	uint32_t tmp;
	const struct gpio_dt_spec *cs_gpio;

	k_mutex_init(&data->lock);

	ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	for (cs_gpio = config->cs_gpios; cs_gpio < &config->cs_gpios[config->num_cs_gpios];
	     cs_gpio++) {
		if (!device_is_ready(cs_gpio->port)) {
			LOG_ERR("CS GPIO port %s pin %d is not ready",
				cs_gpio->port->name, cs_gpio->pin);
			return -ENODEV;
		}

		ret = gpio_pin_configure_dt(cs_gpio, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			return ret;
		}
	}

	if (mipi_dbi_has_pin(&config->reset_gpio)) {
		if (!gpio_is_ready_dt(&config->reset_gpio)) {
			return -ENODEV;
		}
		ret = gpio_pin_configure_dt(&config->reset_gpio, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			LOG_ERR("Could not configure reset GPIO (%d)", ret);
			return ret;
		}
	}

	if (mipi_dbi_has_pin(&config->dc_gpio)) {
		if (!gpio_is_ready_dt(&config->dc_gpio)) {
			return -ENODEV;
		}
		ret = gpio_pin_configure_dt(&config->dc_gpio, GPIO_OUTPUT_INACTIVE);
		if (ret < 0) {
			LOG_ERR("Could not configure D/C GPIO (%d)", ret);
			return ret;
		}
	}

	mipi_dbi_bflb_detrigger(dev);

	tmp = sys_read32(config->base + DBI_INT_STS_OFFSET);
	tmp |= DBI_CR_DBI_END_CLR;
	tmp |= (DBI_CR_DBI_END_MASK
		| DBI_CR_DBI_TXF_MASK
		| DBI_CR_DBI_FER_MASK);
	tmp |= (DBI_CR_DBI_END_EN
		| DBI_CR_DBI_TXF_EN
		| DBI_CR_DBI_FER_EN);
	sys_write32(tmp, config->base + DBI_INT_STS_OFFSET);

	config->irq_config_func(dev);

	return 0;
}

static DEVICE_API(mipi_dbi, mipi_dbi_bflb_driver_api) = {
	.reset = mipi_dbi_bflb_reset,
	.command_write = mipi_dbi_bflb_command_write,
	.write_display = mipi_dbi_bflb_write_display,
	.release = mipi_dbi_bflb_release,
};

static void mipi_dbi_bflb_isr(const struct device *dev)
{
	const struct mipi_dbi_bflb_config *config = dev->config;
	struct mipi_dbi_bflb_data *data = dev->data;
	volatile uint32_t tmp;

	tmp = sys_read32(config->base + DBI_INT_STS_OFFSET);

	if ((tmp & DBI_END_INT) != 0) {
		tmp |= DBI_CR_DBI_END_CLR;
		mipi_dbi_bflb_detrigger(dev);

		if (data->dma.len > 0) {
			mipi_dbi_bflb_start_dma_write(dev, &data->configured, data->dma.buf,
						      data->dma.len);
		} else {
			tmp |= DBI_CR_DBI_END_MASK;
			data->transfer_done = true;
		}
	}
	sys_write32(tmp, config->base + DBI_INT_STS_OFFSET);
}

#define MIPI_DBI_BFLB_IRQ_HANDLER_FUNC(n)					\
	.irq_config_func = mipi_dbi_bflb_config_func_##n
#define MIPI_DBI_BFLB_IRQ_HANDLER(n)						\
	static void mipi_dbi_bflb_config_func_##n(const struct device *dev)	\
	{								\
		IRQ_CONNECT(DT_INST_IRQN(n),				\
			    DT_INST_IRQ(n, priority),			\
			    mipi_dbi_bflb_isr,				\
			    DEVICE_DT_INST_GET(n),			\
			    0);						\
		irq_enable(DT_INST_IRQN(n));				\
	}

#define DBI_GPIO_SPEC_ELEM(inst, _prop, _idx)				\
	GPIO_DT_SPEC_GET_BY_IDX(inst, _prop, _idx),

/* This does not work with inst preprocessor functions for some reason */
#define DBI_CONTEXT_CS_GPIOS_FOREACH_ELEM(inst)				\
	DT_FOREACH_PROP_ELEM(inst, cs_gpios, DBI_GPIO_SPEC_ELEM)

#define MIPI_DBI_BFLB_INIT(n)									   \
	PINCTRL_DT_INST_DEFINE(n);								   \
	MIPI_DBI_BFLB_IRQ_HANDLER(n)								   \
	static const struct mipi_dbi_bflb_config mipi_dbi_bflb_config_##n = {			   \
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),					   \
		.base = DT_INST_REG_ADDR(n),							   \
		.reset_gpio = GPIO_DT_SPEC_INST_GET_OR(n, reset_gpios, {}),			   \
		.dc_gpio = GPIO_DT_SPEC_INST_GET_OR(n, dc_gpios, {}),				   \
		.cs_gpios = (const struct gpio_dt_spec []) {					   \
			COND_CODE_1(DT_NODE_HAS_PROP(DT_INST(n, DT_DRV_COMPAT), cs_gpios),	   \
			    (DBI_CONTEXT_CS_GPIOS_FOREACH_ELEM(DT_INST(n, DT_DRV_COMPAT))), ({0})) \
		},										   \
		.num_cs_gpios = DT_INST_PROP_LEN_OR(n, cs_gpios, 0),				   \
		MIPI_DBI_BFLB_IRQ_HANDLER_FUNC(n)						   \
	};											   \
	static struct mipi_dbi_bflb_data mipi_dbi_bflb_data_##n = {				   \
		.dma.dev = DEVICE_DT_GET(DT_INST_DMAS_CTLR_BY_NAME(n, tx)),			   \
		.dma.channel = DT_INST_DMAS_CELL_BY_NAME(n, tx, channel),			   \
		.dma.config.dma_slot = DT_INST_DMAS_CELL_BY_NAME(n, tx, trigsrc),		   \
		.configured.config.frequency = 0,						   \
	};											   \
												   \
	DEVICE_DT_INST_DEFINE(n, mipi_dbi_bflb_init, NULL,					   \
			&mipi_dbi_bflb_data_##n,						   \
			&mipi_dbi_bflb_config_##n,						   \
			POST_KERNEL,								   \
			CONFIG_MIPI_DBI_INIT_PRIORITY,						   \
			&mipi_dbi_bflb_driver_api);						   \

DT_INST_FOREACH_STATUS_OKAY(MIPI_DBI_BFLB_INIT)
