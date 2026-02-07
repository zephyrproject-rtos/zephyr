/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "spi_nrfx_spim_common.h"

#include <string.h>

#ifdef CONFIG_SOC_NRF5340_CPUAPP
#include <hal/nrf_clock.h>
#endif

LOG_MODULE_REGISTER(spi_nrfx_spim, CONFIG_SPI_LOG_LEVEL);

static void evt_handler(nrfx_spim_event_t const *evt, void *data)
{
	const struct device *dev = data;
	const struct spi_nrfx_common_config *dev_config = dev->config;

	dev_config->evt_handler(dev, (nrfx_spim_event_t *)evt);
}

static void spi_config_copy(struct spi_config *des, const struct spi_config *src)
{
	memcpy(des, src, sizeof(struct spi_config));
}

static bool spi_config_equal(const struct spi_config *a, const struct spi_config *b)
{
	return memcmp(a, b, sizeof(struct spi_config)) == 0;
}

static nrf_spim_mode_t mode_from_op(uint16_t operation)
{
	if (SPI_MODE_GET(operation) & SPI_MODE_CPOL) {
		if (SPI_MODE_GET(operation) & SPI_MODE_CPHA) {
			return NRF_SPIM_MODE_3;
		} else {
			return NRF_SPIM_MODE_2;
		}
	} else {
		if (SPI_MODE_GET(operation) & SPI_MODE_CPHA) {
			return NRF_SPIM_MODE_1;
		} else {
			return NRF_SPIM_MODE_0;
		}
	}
}

static nrf_spim_bit_order_t bit_order_from_op(uint16_t operation)
{
	if (operation & SPI_TRANSFER_LSB) {
		return NRF_SPIM_BIT_ORDER_LSB_FIRST;
	} else {
		return NRF_SPIM_BIT_ORDER_MSB_FIRST;
	}
}

uint32_t resolve_freq(uint32_t frequency)
{
#if defined(CONFIG_SOC_NRF5340_CPUAPP)
	/*
	 * On nRF5340, the 32 Mbps speed is supported by the application core
	 * when it is running at 128 MHz (see the Timing specifications section
	 * in the nRF5340 PS).
	 */
	if (frequency > MHZ(16) &&
	    nrf_clock_hfclk_div_get(NRF_CLOCK) != NRF_CLOCK_HFCLK_DIV_1) {
		frequency = MHZ(16);
	}
#endif

	if (NRF_SPIM_HAS_PRESCALER) {
		return frequency;
	}

	/* Get the highest supported frequency not exceeding the requested one */
	if (frequency >= MHZ(32) && NRFX_ERROR_INVALID_PARAM) {
		return MHZ(32);
	} else if (frequency >= MHZ(16) && NRF_SPIM_HAS_16_MHZ_FREQ) {
		return MHZ(16);
	} else if (frequency >= MHZ(8)) {
		return MHZ(8);
	} else if (frequency >= MHZ(4)) {
		return MHZ(4);
	} else if (frequency >= MHZ(2)) {
		return MHZ(2);
	} else if (frequency >= MHZ(1)) {
		return MHZ(1);
	} else if (frequency >= KHZ(500)) {
		return KHZ(500);
	} else if (frequency >= KHZ(250)) {
		return KHZ(250);
	} else {
		return KHZ(125);
	}
}

#if SPI_NRFX_HAS_RAM_BUF || CONFIG_HAS_NORDIC_DMM
#if SPI_NRFX_HAS_RAM_BUF
static int prepare_tx_ram_buf(const struct device *dev,
			      const uint8_t **tx_buf,
			      size_t tx_buf_len)
{
	const struct spi_nrfx_common_config *dev_config = dev->config;
	NRF_SPIM_Type *spim_reg = dev_config->spim.p_reg;

	if (nrf_dma_accessible_check(spim_reg, *tx_buf)) {
		return 0;
	}

	if (tx_buf_len > SPI_NRFX_RAM_BUF_SIZE) {
		return -ENOSPC;
	}

	memcpy(dev_config->tx_ram_buf, *tx_buf, tx_buf_len);
	*tx_buf = dev_config->tx_ram_buf;
	return 0;
}

static int prepare_rx_ram_buf(const struct device *dev,
			      uint8_t **rx_buf,
			      size_t rx_buf_len)
{
	const struct spi_nrfx_common_config *dev_config = dev->config;
	NRF_SPIM_Type *spim_reg = dev_config->spim.p_reg;

	if (nrf_dma_accessible_check(spim_reg, *rx_buf)) {
		return 0;
	}

	if (rx_buf_len > SPI_NRFX_RAM_BUF_SIZE) {
		return -ENOSPC;
	}

	*rx_buf = dev_config->rx_ram_buf;
	return 0;
}

static void release_rx_ram_buf(const struct device *dev, const uint8_t *rx_buf)
{
	struct spi_nrfx_common_data *dev_data = dev->data;
	const struct spi_nrfx_common_config *dev_config = dev->config;

	if (rx_buf == dev_data->rx_user_buf) {
		return;
	}

	if (rx_buf != dev_config->rx_buf) {
		return;
	}

	memcpy(dev_data->rx_user_buf, rx_buf, dev_data->rx_user_buf_len);
}

#else /* SPI_NRFX_HAS_RAM_BUF */

static int prepare_tx_ram_buf(const struct device *dev,
			      const uint8_t **tx_buf,
			      size_t tx_buf_len)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(tx_buf);
	ARG_UNUSED(tx_buf_len);
	return 0;
}

static int prepare_rx_ram_buf(const struct device *dev,
			      const uint8_t **rx_buf,
			      size_t rx_buf_len)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(rx_buf);
	ARG_UNUSED(rx_buf_len);
	return 0;
}

static void release_rx_ram_buf(const struct device *dev, const uint8_t *rx_buf)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(rx_buf);
}
#endif /* SPI_NRFX_HAS_RAM_BUF */

#if CONFIG_HAS_NORDIC_DMM
static int prepare_tx_dmm_buf(const struct device *dev,
			      const uint8_t **tx_buf,
			      size_t tx_buf_len)
{
	const struct spi_nrfx_common_config *dev_config = dev->config;

	if (*tx_buf == NULL || tx_buf_len == 0) {
		return 0;
	}

	return dmm_buffer_out_prepare(dev_config->mem_reg, *tx_buf, tx_buf_len, tx_buf);
}

static int prepare_rx_dmm_buf(const struct device *dev,
			      const uint8_t **rx_buf,
			      size_t rx_buf_len)
{
	const struct spi_nrfx_common_config *dev_config = dev->config;

	if (*rx_buf == NULL || rx_buf_len == 0) {
		return 0;
	}

	return dmm_buffer_in_prepare(dev_config->mem_reg, *rx_buf, rx_buf_len, rx_buf);
}

static void release_tx_dmm_buf(const struct device *dev, const uint8_t *tx_buf)
{
	struct spi_nrfx_common_data *dev_data = dev->data;
	const struct spi_nrfx_common_config *dev_config = dev->config;

	dmm_buffer_out_release(dev_config->mem_reg, tx_buf);
}

static void release_rx_dmm_buf(const struct device *dev, const uint8_t *rx_buf)
{
	struct spi_nrfx_common_data *dev_data = dev->data;
	const struct spi_nrfx_common_config *dev_config = dev->config;

	dmm_buffer_in_release(dev_config->mem_reg,
			      dev_data->rx_user_buf,
			      dev_data->rx_user_buf_len,
			      (void *)rx_buf);
}

#else /* CONFIG_HAS_NORDIC_DMM */

static int prepare_tx_dmm_buf(const struct device *dev,
			      const uint8_t **tx_buf,
			      size_t tx_buf_len)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(tx_buf);
	ARG_UNUSED(tx_buf_len);
	return 0;
}

static int prepare_rx_dmm_buf(const struct device *dev,
			      uint8_t **rx_buf,
			      size_t rx_buf_len)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(rx_buf);
	ARG_UNUSED(rx_buf_len);
	return 0;
}

static void release_tx_dmm_buf(const struct device *dev, const uint8_t *tx_buf)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(tx_buf);
}

static void release_rx_dmm_buf(const struct device *dev, uint8_t *rx_buf)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(rx_buf);
}
#endif /* CONFIG_HAS_NORDIC_DMM */

static int prepare_tx_buf(const struct device *dev,
			  const uint8_t **tx_buf,
			  size_t tx_buf_len)
{
	struct spi_nrfx_common_data *dev_data = dev->data;
	int ret;

	dev_data->tx_user_buf = tx_buf;

	ret = prepare_tx_ram_buf(dev, tx_buf, tx_buf_len);
	if (ret) {
		return ret;
	}

	ret = prepare_tx_dmm_buf(dev, tx_buf, tx_buf_len);
	if (ret) {
		return ret;
	}

	return 0;
}

static int prepare_rx_buf(const struct device *dev,
			  uint8_t **rx_buf,
			  size_t rx_buf_len)
{
	struct spi_nrfx_common_data *dev_data = dev->data;
	int ret;

	dev_data->rx_user_buf = rx_buf;
	dev_data->rx_user_buf_len = rx_buf_len;

	ret = prepare_rx_ram_buf(dev, rx_buf, rx_buf_len);
	if (ret) {
		return ret;
	}

	ret = prepare_rx_dmm_buf(dev, rx_buf, rx_buf_len);
	if (ret) {
		return ret;
	}

	return 0;
}

static void release_tx_buf(const struct device *dev, const uint8_t *tx_buf)
{
	release_tx_dmm_buf(dev, tx_buf);
}

static void release_rx_buf(const struct device *dev, const uint8_t *rx_buf)
{
	release_rx_ram_buf(dev, rx_buf);
	release_rx_dmm_buf(dev, rx_buf);
}

#else /* SPI_NRFX_HAS_RAM_BUF || CONFIG_HAS_NORDIC_DMM */

static int prepare_tx_buf(const struct device *dev,
			  const uint8_t **tx_buf,
			  size_t tx_buf_len)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(tx_buf);
	ARG_UNUSED(tx_buf_len);
	return 0;
}

static int prepare_rx_buf(const struct device *dev,
			  uint8_t **rx_buf,
			  size_t rx_buf_len)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(rx_buf);
	ARG_UNUSED(rx_buf_len);
	return 0;
}

static void release_tx_buf(const struct device *dev, const uint8_t *tx_buf)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(tx_buf);
}

static void release_rx_buf(const struct device *dev, const uint8_t *rx_buf)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(rx_buf);
}
#endif /* SPI_NRFX_HAS_RAM_BUF || CONFIG_HAS_NORDIC_DMM */

int spi_nrfx_spim_common_transfer_start(const struct device *dev,
					const uint8_t *tx_buf,
					size_t tx_buf_len,
					uint8_t *rx_buf,
					size_t rx_buf_len)
{
	struct spi_nrfx_common_data *dev_data = dev->data;
	struct nrfy_spim_xfer_desc_t xfer;
	int ret;

	ret = prepare_tx_buf(dev, &tx_buf, tx_buf_len);
	if (ret) {
		return ret;
	}

	ret = prepare_rx_buf(dev, &rx_buf, rx_buf_len);
	if (ret) {
		release_tx_buf(dev, tx_buf);
		return ret;
	}

	/* Set the accessible and aligned buffers */
	xfer.p_tx_buffer = (uint8_t *)tx_buf,
	xfer.tx_length = tx_buf_len,
	xfer.p_rx_buffer = rx_buf,
	xfer.rx_length = rx_buf_len,

	/* TODO something 320MHz nrfy enable */

	ret = nrfx_spim_xfer(&dev_data->spim, &xfer, 0);
	if (ret) {
		release_tx_buf(dev, tx_buf);
		release_rx_buf(dev, rx_buf);
		return ret;
	}

	return 0;
}

void spi_nrfx_spim_common_transfer_end(const struct device *dev,
				       const nrfx_spim_xfer_desc_t *xfer)
{
	release_tx_buf(dev, (const uint8_t *)xfer->p_tx_buffer);
	release_rx_buf(dev, (const uint8_t *)xfer->p_rx_buffer);
}

int spi_nrfx_spim_common_configure(const struct device *dev, const struct spi_config *spi_cfg)
{
	struct spi_nrfx_common_data *dev_data = dev->data;
	const struct spi_nrfx_common_config *dev_config = dev->config;
	nrfx_spim_config_t spim_cfg;
	int ret;

	if (dev_data->configured && spi_config_equal(&dev_data->spi_cfg, spi_cfg)) {
		return 0;
	}

	if (spi_cfg->operation & SPI_HALF_DUPLEX) {
		LOG_ERR("Half-duplex not supported");
		return -ENOTSUP;
	}

	if (SPI_OP_MODE_GET(spi_cfg->operation) != SPI_OP_MODE_MASTER) {
		LOG_ERR("Slave mode is not supported on %s", dev->name);
		return -EINVAL;
	}

	if (spi_cfg->operation & SPI_MODE_LOOP) {
		LOG_ERR("Loopback mode is not supported");
		return -EINVAL;
	}

	if (IS_ENABLED(CONFIG_SPI_EXTENDED_MODES) &&
	    (spi_cfg->operation & SPI_LINES_MASK) != SPI_LINES_SINGLE) {
		LOG_ERR("Only single line mode is supported");
		return -EINVAL;
	}

	if (SPI_WORD_SIZE_GET(spi_cfg->operation) != 8) {
		LOG_ERR("Word sizes other than 8 bits are not supported");
		return -EINVAL;
	}

	if (spi_cfg->frequency < KHZ(125)) {
		LOG_ERR("Frequencies lower than 125 kHz are not supported");
		return -EINVAL;
	}

	spim_cfg.ss_pin = NRF_SPIM_PIN_NOT_CONNECTED;
	spim_cfg.orc = dev_config->orc;
	spim_cfg.frequency = resolve_freq(spi_cfg->frequency);
	spim_cfg.mode = mode_from_op(spi_cfg->operation);
	spim_cfg.bit_order = bit_order_from_op(spi_cfg->operation);
#if NRF_SPIM_HAS_DCX
	spim_cfg.dcx_pin = NRF_SPIM_PIN_NOT_CONNECTED;
#endif
#if NRF_SPIM_HAS_RXDELAY
	spim_cfg.rx_delay = dev_config->rx_delay;
#endif
	spim_cfg.skip_gpio_cfg = true;
	spim_cfg.skip_psel_cfg = true;

if (dev_data->configured) {
		ret = nrfx_spim_reconfigure(&dev_data->spim, &spim_cfg);
	} else {
		ret = nrfx_spim_init(&dev_data->spim, &spim_cfg, evt_handler, (void *)dev);
	}

	if (ret) {
		LOG_ERR("Failed to configure nrfx driver: %d", ret);
		return ret;
	}

	spi_config_copy(&dev_data->spi_cfg, spi_cfg);
	dev_data->configured = true;
	return 0;
}

static int pm_suspend(const struct device *dev)
{
	struct spi_nrfx_common_data *dev_data = dev->data;
	const struct spi_nrfx_common_config *dev_config = dev->config;

	if (dev_data->configured) {
		nrfx_spim_uninit(&dev_data->spim);
		dev_data->configured = false;
	}

	/*
	err = spi_context_cs_put_all(&dev_data->ctx);
	*/

	return pinctrl_apply_state(dev_config->pcfg, PINCTRL_STATE_SLEEP);
}

static int pm_resume(const struct device *dev)
{
	const struct spi_nrfx_common_config *dev_config = dev->config;

	return pinctrl_apply_state(dev_config->pcfg, PINCTRL_STATE_DEFAULT);

	/*
	if (spi_context_cs_get_all(&dev_data->ctx)) {
		return -EAGAIN;
	}
	*/
}

int spi_nrfx_spim_common_pm_action(const struct device *dev, enum pm_device_action action)
{
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		ret = pm_suspend(dev);
		break;

	case PM_DEVICE_ACTION_RESUME:
		ret = pm_resume(dev);
		break;

	case PM_DEVICE_ACTION_TURN_OFF:
	case PM_DEVICE_ACTION_TURN_ON:
		ret = -ENOTSUP;
		break;

	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

int spi_nrfx_spim_common_init(const struct device *dev)
{
	const struct spi_nrfx_common_config *dev_config = dev->config;

	dev_config->irq_connect();
	return 0;
}

#ifdef CONFIG_DEVICE_DEINIT_SUPPORT
int spi_nrfx_spim_common_deinit(const struct device *dev)
{
	return pm_device_driver_deinit(dev, spi_nrfx_spim_common_pm_action);
}
#endif
