/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "spi_nrfx_spim_common.h"

#include <string.h>

#ifdef CONFIG_SOC_NRF5340_CPUAPP
#include <hal/nrf_clock.h>
#endif

#include <gpiote_nrfx.h>

#define WAKE_PIN_START_FLAGS (GPIO_INPUT | GPIO_PULL_UP)
#define WAKE_PIN_STOP_FLAGS (GPIO_INPUT | GPIO_PULL_DOWN | GPIO_DISCONNECTED)

LOG_MODULE_REGISTER(spi_nrfx_spim, CONFIG_SPI_LOG_LEVEL);

#if SPI_NRFX_HAS_WAKE
void spi_nrfx_spim_common_wake_start(const struct device *dev,
				     spi_nrfx_data_common_wake_handler handler)
{
	struct spi_nrfx_common_data *dev_data = dev->data;
	const struct spi_nrfx_common_config *dev_config = dev->config;

	dev_data->wake_handler = handler;

	k_timer_start(&dev_data->wake_timer,
		      K_USEC(CONFIG_SPI_NRFX_WAKE_TIMEOUT_US),
		      K_FOREVER);

	gpio_pin_configure_dt(&dev_config->wake_pin, WAKE_PIN_START_FLAGS);
	gpio_pin_interrupt_configure_dt(&dev_config->wake_pin, GPIO_INT_EDGE_FALLING);
}

#else

void spi_nrfx_spim_common_wake_start(const struct device *dev,
				     spi_nrfx_data_common_wake_handler handler)
{
	handler(dev);
}
#endif

void spi_nrfx_spim_common_cs_set(const struct device *dev, const struct spi_config *spi_cfg)
{
	struct spi_nrfx_common_data *dev_data = dev->data;
	NRF_SPIM_Type *spim_reg = dev_data->spim.p_reg;

	/*
	 * Enable the SPIM peripheral so it drives the SPI bus, ensuring correct initial bus state
	 * before transfer starts, and setting CS if its controlled by the SPIM peripheral.
	 */
	nrfy_spim_enable(spim_reg);

	if (spi_cfg->cs.cs_is_gpio) {
		gpio_pin_set_dt(&spi_cfg->cs.gpio, 1);
	}

	/* Wait only if we control the CS pin */
	if (spi_cfg->cs.cs_is_gpio || NRF_SPIM_IS_320MHZ_SPIM(spim_reg)) {
		k_busy_wait(spi_cfg->cs.delay);
	}
}

void spi_nrfx_spim_common_cs_clear(const struct device *dev, const struct spi_config *spi_cfg)
{
	struct spi_nrfx_common_data *dev_data = dev->data;
	NRF_SPIM_Type *spim_reg = dev_data->spim.p_reg;

	/* Wait only if we control the CS pin */
	if (spi_cfg->cs.cs_is_gpio || NRF_SPIM_IS_320MHZ_SPIM(spim_reg)) {
		k_busy_wait(spi_cfg->cs.delay);
	}

	if (spi_cfg->cs.cs_is_gpio) {
		gpio_pin_set_dt(&spi_cfg->cs.gpio, 0);
	}

	/*
	 * Disable the SPIM peripheral so it no longer drives the SPI bus, clearing CS if its
	 * controlled by the SPIM peripheral.
	 */
	nrfy_spim_disable(spim_reg);
}

static void spi_config_copy(struct spi_config *des, const struct spi_config *src)
{
	memcpy(des, src, sizeof(struct spi_config));
}

static bool spi_config_equal(const struct spi_config *a, const struct spi_config *b)
{
	if (a->frequency != b->frequency ||
	    a->operation != b->operation ||
	    a->slave != b->slave ||
	    a->cs.cs_is_gpio != b->cs.cs_is_gpio ||
	    a->word_delay != b->word_delay) {
		return false;
	}

	if (a->cs.cs_is_gpio) {
		if (a->cs.gpio.port != b->cs.gpio.port ||
		    a->cs.gpio.pin != b->cs.gpio.pin ||
		    a->cs.gpio.dt_flags != b->cs.gpio.dt_flags ||
		    a->cs.delay != b->cs.delay) {
			return false;
		}
	} else {
		if (a->cs.setup_ns != b->cs.setup_ns ||
		    a->cs.hold_ns != b->cs.hold_ns) {
			return false;
		}
	}

	return true;
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
	if (frequency >= MHZ(32) && NRF_SPIM_HAS_32_MHZ_FREQ) {
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

#if SPI_NRFX_HAS_RAM_BUF

static size_t transfer_buf_chunk_limit(const struct device *dev, size_t chunk_len)
{
	struct spi_nrfx_common_data *dev_data = dev->data;
	nrfx_spim_t *spim = &dev_data->spim;

	if ((dev_data->tx_user_buf != NULL &&
	     !nrf_dma_accessible_check(&spim->p_reg, dev_data->tx_user_buf)) ||
	    (dev_data->rx_user_buf != NULL &&
	     !nrf_dma_accessible_check(&spim->p_reg, dev_data->rx_user_buf))) {
		return MIN(chunk_len, SPI_NRFX_RAM_BUF_SIZE);
	}

	return chunk_len;
}

static int transfer_buf_prepare(const struct device *dev,
				size_t chunk_len,
				const uint8_t **tx_buf,
				uint8_t **rx_buf)
{
	struct spi_nrfx_common_data *dev_data = dev->data;
	const struct spi_nrfx_common_config *dev_config = dev->config;
	nrfx_spim_t *spim = &dev_data->spim;
	const uint8_t *tx_user;
	uint8_t *rx_user;

	dev_data->rx_ram_buf = NULL;

	if (dev_data->tx_user_buf != NULL) {
		tx_user = dev_data->tx_user_buf + dev_data->user_buf_pos;

		if (!nrf_dma_accessible_check(&spim->p_reg, tx_user)) {
			memcpy(dev_config->tx_ram_buf, tx_user, chunk_len);
			*tx_buf = dev_config->tx_ram_buf;
		} else {
			*tx_buf = tx_user;
		}
	} else {
		*tx_buf = NULL;
	}

	if (dev_data->rx_user_buf != NULL) {
		rx_user = dev_data->rx_user_buf + dev_data->user_buf_pos;

		if (!nrf_dma_accessible_check(&spim->p_reg, rx_user)) {
			dev_data->rx_ram_buf = dev_config->rx_ram_buf;
			*rx_buf = dev_config->rx_ram_buf;
		} else {
			*rx_buf = rx_user;
		}
	} else {
		*rx_buf = NULL;
	}

	return 0;
}

static void transfer_buf_complete(const struct device *dev,
				  const nrfx_spim_xfer_desc_t *xfer)
{
	struct spi_nrfx_common_data *dev_data = dev->data;

	if (dev_data->rx_ram_buf != NULL) {
		memcpy(dev_data->rx_user_buf + dev_data->user_buf_pos,
		       dev_data->rx_ram_buf,
		       xfer->rx_length);
		dev_data->rx_ram_buf = NULL;
	}
}

static void transfer_buf_abort(const struct device *dev)
{
	ARG_UNUSED(dev);
}

#elif SPI_NRFX_HAS_DMM_BUF

static size_t transfer_buf_chunk_limit(const struct device *dev, size_t chunk_len)
{
	ARG_UNUSED(dev);

	return MIN(chunk_len, SPI_NRFX_DMM_BUF_SIZE);
}

static int transfer_buf_prepare(const struct device *dev,
				size_t chunk_len,
				const uint8_t **tx_buf,
				uint8_t **rx_buf)
{
	struct spi_nrfx_common_data *dev_data = dev->data;
	const struct spi_nrfx_common_config *dev_config = dev->config;
	const uint8_t *tx_user;
	uint8_t *rx_user;
	int ret;

	dev_data->tx_dmm_buf = NULL;
	dev_data->rx_dmm_buf = NULL;

	if (dev_data->tx_user_buf != NULL) {
		tx_user = dev_data->tx_user_buf + dev_data->user_buf_pos;

		ret = dmm_buffer_out_prepare(dev_config->mem_reg,
					     (void *)tx_user,
					     chunk_len,
					     &dev_data->tx_dmm_buf);
		if (ret != 0) {
			return ret;
		}

		*tx_buf = dev_data->tx_dmm_buf;
	} else {
		*tx_buf = NULL;
	}

	if (dev_data->rx_user_buf != NULL) {
		rx_user = dev_data->rx_user_buf + dev_data->user_buf_pos;

		ret = dmm_buffer_in_prepare(dev_config->mem_reg,
					    (void *)rx_user,
					    chunk_len,
					    &dev_data->rx_dmm_buf);
		if (ret != 0) {
			dmm_buffer_out_release(dev_config->mem_reg, dev_data->tx_dmm_buf);
			dev_data->tx_dmm_buf = NULL;
			return ret;
		}

		*rx_buf = dev_data->rx_dmm_buf;
	} else {
		*rx_buf = NULL;
	}

	return 0;
}

static void transfer_buf_complete(const struct device *dev,
				  const nrfx_spim_xfer_desc_t *xfer)
{
	struct spi_nrfx_common_data *dev_data = dev->data;
	const struct spi_nrfx_common_config *dev_config = dev->config;

	if (dev_data->tx_dmm_buf != NULL) {
		dmm_buffer_out_release(dev_config->mem_reg, dev_data->tx_dmm_buf);
		dev_data->tx_dmm_buf = NULL;
	}

	if (dev_data->rx_dmm_buf != NULL) {
		dmm_buffer_in_release(dev_config->mem_reg,
				      dev_data->rx_user_buf + dev_data->user_buf_pos,
				      xfer->rx_length,
				      dev_data->rx_dmm_buf);
		dev_data->rx_dmm_buf = NULL;
	}
}

static void transfer_buf_abort(const struct device *dev)
{
	struct spi_nrfx_common_data *dev_data = dev->data;
	const struct spi_nrfx_common_config *dev_config = dev->config;

	dmm_buffer_out_release(dev_config->mem_reg, dev_data->tx_dmm_buf);
	dev_data->tx_dmm_buf = NULL;
	dev_data->rx_dmm_buf = NULL;
}

#endif

static void transfer_end(const struct device *dev, int ret)
{
	const struct spi_nrfx_common_config *dev_config = dev->config;

	dev_config->evt_handler(dev, ret);
}

static void transfer_start(const struct device *dev)
{
	struct spi_nrfx_common_data *dev_data = dev->data;
	const struct spi_nrfx_common_config *dev_config = dev->config;
	size_t chunk_len;
	const uint8_t *tx_buf;
	uint8_t *rx_buf;
	nrfx_spim_xfer_desc_t xfer;
	nrfx_spim_t *spim = &dev_data->spim;
	int ret;

	chunk_len = dev_data->user_buf_len - dev_data->user_buf_pos;

	if (chunk_len == 0) {
		transfer_end(dev, dev_data->user_buf_len);
		return;
	}

	/* Limit to EasyDMA MAXCNT */
	chunk_len = MIN(chunk_len, dev_config->max_transfer_len);

#if SPI_NRFX_HAS_RAM_BUF || SPI_NRFX_HAS_DMM_BUF
	chunk_len = transfer_buf_chunk_limit(dev, chunk_len);

	ret = transfer_buf_prepare(dev, chunk_len, &tx_buf, &rx_buf);
	if (ret != 0) {
		transfer_end(dev, ret);
		return;
	}
#else
	if (dev_data->tx_user_buf != NULL) {
		tx_buf = dev_data->tx_user_buf + dev_data->user_buf_pos;
	} else {
		tx_buf = NULL;
	}

	if (dev_data->rx_user_buf != NULL) {
		rx_buf = dev_data->rx_user_buf + dev_data->user_buf_pos;
	} else {
		rx_buf = NULL;
	}
#endif

	xfer.p_tx_buffer = tx_buf;
	xfer.tx_length = tx_buf != NULL ? chunk_len : 0;
	xfer.p_rx_buffer = rx_buf;
	xfer.rx_length = rx_buf != NULL ? chunk_len : 0;

	ret = nrfx_spim_xfer(spim, &xfer, 0);
	if (ret) {
#if SPI_NRFX_HAS_RAM_BUF || SPI_NRFX_HAS_DMM_BUF
		transfer_buf_abort(dev);
#endif
		transfer_end(dev, ret);
	}
}

static void evt_handler(nrfx_spim_event_t const *evt, void *data)
{
	const struct device *dev = data;
	struct spi_nrfx_common_data *dev_data = dev->data;
	const nrfx_spim_xfer_desc_t *xfer = &evt->xfer_desc;

#if SPI_NRFX_HAS_RAM_BUF || SPI_NRFX_HAS_DMM_BUF
	transfer_buf_complete(dev, xfer);
#endif

	dev_data->user_buf_pos += MAX(xfer->tx_length, xfer->rx_length);

	transfer_start(dev);
}

void spi_nrfx_spim_common_transfer_start(const struct device *dev,
					 const uint8_t *tx_buf,
					 size_t tx_buf_len,
					 uint8_t *rx_buf,
					 size_t rx_buf_len)
{
	struct spi_nrfx_common_data *dev_data = dev->data;

	dev_data->tx_user_buf = tx_buf;
	dev_data->rx_user_buf = rx_buf;
	dev_data->user_buf_len = MAX(tx_buf_len, rx_buf_len);
	dev_data->user_buf_pos = 0;

	transfer_start(dev);
}

void spi_nrfx_spim_common_transfer_stop(const struct device *dev)
{
	struct spi_nrfx_common_data *dev_data = dev->data;
	nrfx_spim_t *spim = &dev_data->spim;

	nrfx_spim_abort(spim);
}

static void init_sck_pin(const struct device *dev, uint16_t operation)
{
	struct spi_nrfx_common_data *dev_data;
	nrfx_spim_t *spim;
	uint32_t sck_pin;
	uint32_t initial_value;

	dev_data = dev->data;
	spim = &dev_data->spim;
	sck_pin = nrfy_spim_sck_pin_get(spim->p_reg);

	if (sck_pin == NRF_SPIM_PIN_NOT_CONNECTED) {
		return;
	}

	initial_value = SPI_MODE_GET(operation) & SPI_MODE_CPOL ? 1 : 0;

	nrf_gpio_pin_write(sck_pin, initial_value);
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

	init_sck_pin(dev, spi_cfg->operation);

	spim_cfg.ss_pin = NRF_SPIM_PIN_NOT_CONNECTED;
	spim_cfg.orc = dev_config->orc;
	spim_cfg.frequency = MIN(resolve_freq(spi_cfg->frequency), dev_config->max_freq);
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

#if defined(CONFIG_NRF52_ANOMALY_58_WORKAROUND)
#if DT_NODE_HAS_STATUS(DT_NODELABEL(gpiote), okay)
	nrfx_spim_nrf52_anomaly_58_init(&dev_data->spim,
					&GPIOTE_NRFX_INST_BY_NODE(DT_NODELABEL(gpiote)));
#else
#error "GPIOTE is not enabled, anomaly 58 workaround cannot be applied for SPIM"
#endif
#endif

	spi_config_copy(&dev_data->spi_cfg, spi_cfg);
	dev_data->configured = true;
	return 0;
}

static int cs_put(const struct device *dev)
{
	const struct spi_nrfx_common_config *dev_config = dev->config;
	const struct gpio_dt_spec *cs_gpios = dev_config->cs_gpios;
	const uint8_t cs_gpios_size = dev_config->cs_gpios_size;
	int ret;

	for (size_t i = 0; i < cs_gpios_size; i++) {
		ret = pm_device_runtime_put(cs_gpios[i].port);
		if (ret) {
			return ret;
		}
	}

	return 0;
}

static int pm_suspend(const struct device *dev)
{
	struct spi_nrfx_common_data *dev_data = dev->data;
	int ret;

#if CONFIG_PINCTRL_KEEP_SLEEP_STATE
	const struct spi_nrfx_common_config *dev_config = dev->config;
#endif

	if (dev_data->configured) {
		nrfx_spim_uninit(&dev_data->spim);
		dev_data->configured = false;
	}

	ret = cs_put(dev);
	if (ret) {
		return ret;
	}

#if CONFIG_PINCTRL_KEEP_SLEEP_STATE
	ret = pinctrl_apply_state(dev_config->pcfg, PINCTRL_STATE_SLEEP);
	if (ret) {
		return ret;
	}
#endif

	return 0;
}

static int cs_get(const struct device *dev)
{
	const struct spi_nrfx_common_config *dev_config = dev->config;
	const struct gpio_dt_spec *cs_gpios = dev_config->cs_gpios;
	const uint8_t cs_gpios_size = dev_config->cs_gpios_size;
	int ret;

	for (size_t i = 0; i < cs_gpios_size; i++) {
		ret = pm_device_runtime_get(cs_gpios[i].port);
		if (ret) {
			return ret;
		}
	}

	return 0;
}

static int pm_resume(const struct device *dev)
{
	const struct spi_nrfx_common_config *dev_config = dev->config;
	int ret;

	ret = cs_get(dev);
	if (ret) {
		return ret;
	}

	ret = pinctrl_apply_state(dev_config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret) {
		return ret;
	}

	return 0;
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

static int cs_init(const struct device *dev)
{
	const struct spi_nrfx_common_config *dev_config = dev->config;
	const struct gpio_dt_spec *cs_gpios = dev_config->cs_gpios;
	const uint8_t cs_gpios_size = dev_config->cs_gpios_size;
	int ret;

	for (size_t i = 0; i < cs_gpios_size; i++) {
		ret = gpio_pin_configure_dt(&cs_gpios[i], GPIO_OUTPUT_INACTIVE);
		if (ret) {
			return ret;
		}
	}

	return 0;
}

#if SPI_NRFX_HAS_WAKE
static void wake_stop(const struct device *dev)
{
	struct spi_nrfx_common_data *dev_data = dev->data;
	const struct spi_nrfx_common_config *dev_config = dev->config;

	gpio_pin_interrupt_configure_dt(&dev_config->wake_pin, GPIO_INT_DISABLE);
	gpio_pin_configure_dt(&dev_config->wake_pin, WAKE_PIN_STOP_FLAGS);

	dev_data->wake_handler(dev_data->dev);
}

static void wake_pin_callback_handler(const struct device *port,
				      struct gpio_callback *cb,
				      uint32_t pins)
{
	struct spi_nrfx_common_data *dev_data =
		CONTAINER_OF(cb, struct spi_nrfx_common_data, wake_pin_callback);

	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	k_timer_stop(&dev_data->wake_timer);

	if (k_timer_status_get(&dev_data->wake_timer)) {
		return;
	}

	wake_stop(dev_data->dev);
}

static void wake_timer_expired_handler(struct k_timer *timer)
{
	struct spi_nrfx_common_data *dev_data =
		CONTAINER_OF(timer, struct spi_nrfx_common_data, wake_timer);

	wake_stop(dev_data->dev);
}

static int wake_init(const struct device *dev)
{
	struct spi_nrfx_common_data *dev_data = dev->data;
	const struct spi_nrfx_common_config *dev_config = dev->config;
	int ret;

	ret = gpio_pin_configure_dt(&dev_config->wake_pin, WAKE_PIN_STOP_FLAGS);
	if (ret) {
		return ret;
	}

	gpio_init_callback(&dev_data->wake_pin_callback,
			   wake_pin_callback_handler,
			   BIT(dev_config->wake_pin.pin));

	ret = gpio_add_callback(dev_config->wake_pin.port, &dev_data->wake_pin_callback);
	if (ret) {
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&dev_config->wake_pin, GPIO_INT_DISABLE);
	if (ret) {
		return ret;
	}

	k_timer_init(&dev_data->wake_timer, wake_timer_expired_handler, NULL);
	return 0;
}

#else

static int wake_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}
#endif

int spi_nrfx_spim_common_init(const struct device *dev)
{
	const struct spi_nrfx_common_config *dev_config = dev->config;
	int ret;

	dev_config->irq_connect();

	ret = cs_init(dev);
	if (ret) {
		return ret;
	}

	ret = wake_init(dev);
	if (ret) {
		return ret;
	}

	return 0;
}

#ifdef CONFIG_DEVICE_DEINIT_SUPPORT
int spi_nrfx_spim_common_deinit(const struct device *dev)
{
	return pm_device_driver_deinit(dev, spi_nrfx_spim_common_pm_action);
}
#endif
