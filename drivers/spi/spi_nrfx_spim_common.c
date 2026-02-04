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

	if (spi_cfg->cs.cs_is_gpio == false && NRF_SPIM_IS_320MHZ_SPIM(spim_reg) == false) {
		return;
	}

	if (spi_cfg->cs.cs_is_gpio) {
		gpio_pin_set_dt(&spi_cfg->cs.gpio, 1);
	} else {
		nrfy_spim_enable(spim_reg);
	}

	k_busy_wait(spi_cfg->cs.delay);
}

void spi_nrfx_spim_common_cs_clear(const struct device *dev, const struct spi_config *spi_cfg)
{
	struct spi_nrfx_common_data *dev_data = dev->data;
	NRF_SPIM_Type *spim_reg = dev_data->spim.p_reg;

	if (spi_cfg->cs.cs_is_gpio == false && NRF_SPIM_IS_320MHZ_SPIM(spim_reg) == false) {
		return;
	}

	k_busy_wait(spi_cfg->cs.delay);

	if (spi_cfg->cs.cs_is_gpio) {
		gpio_pin_set_dt(&spi_cfg->cs.gpio, 0);
	} else {
		nrfy_spim_disable(spim_reg);
	}
}

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

#if SPI_NRFX_HAS_RAM_BUF || CONFIG_HAS_NORDIC_DMM
#if SPI_NRFX_HAS_RAM_BUF
static int prepare_tx_ram_buf(const struct device *dev,
			      const uint8_t **tx_buf,
			      size_t tx_buf_len)
{
	struct spi_nrfx_common_data *dev_data = dev->data;
	const struct spi_nrfx_common_config *dev_config = dev->config;
	NRF_SPIM_Type *spim_reg = dev_data->spim.p_reg;

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
	struct spi_nrfx_common_data *dev_data = dev->data;
	const struct spi_nrfx_common_config *dev_config = dev->config;
	NRF_SPIM_Type *spim_reg = dev_data->spim.p_reg;

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

	if (rx_buf != dev_config->rx_ram_buf) {
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
			      uint8_t **rx_buf,
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

	return dmm_buffer_out_prepare(dev_config->mem_reg,
				      (void *)*tx_buf,
				      tx_buf_len,
				      (void **)tx_buf);
}

static int prepare_rx_dmm_buf(const struct device *dev,
			      uint8_t **rx_buf,
			      size_t rx_buf_len)
{
	const struct spi_nrfx_common_config *dev_config = dev->config;

	if (*rx_buf == NULL || rx_buf_len == 0) {
		return 0;
	}

	return dmm_buffer_in_prepare(dev_config->mem_reg,
				     *rx_buf,
				     rx_buf_len,
				     (void **)rx_buf);
}

static void release_tx_dmm_buf(const struct device *dev, const uint8_t *tx_buf)
{
	const struct spi_nrfx_common_config *dev_config = dev->config;

	dmm_buffer_out_release(dev_config->mem_reg, (void *)tx_buf);
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

static void release_rx_dmm_buf(const struct device *dev, const uint8_t *rx_buf)
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

	dev_data->tx_user_buf = *tx_buf;

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

	dev_data->rx_user_buf = *rx_buf;
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
	const struct spi_nrfx_common_config *dev_config = dev->config;
	nrfx_spim_t *spim = &dev_data->spim;
	struct nrfy_spim_xfer_desc_t xfer;
	int ret;

	if (tx_buf_len > dev_config->max_transfer_len ||
	    rx_buf_len > dev_config->max_transfer_len) {
		return -EINVAL;
	}

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

	ret = nrfx_spim_xfer(spim, &xfer, 0);
	if (ret) {
		release_tx_buf(dev, tx_buf);
		release_rx_buf(dev, rx_buf);
		return ret;
	}

	return 0;
}

void spi_nrfx_spim_common_transfer_stop(const struct device *dev)
{
	struct spi_nrfx_common_data *dev_data = dev->data;
	nrfx_spim_t *spim = &dev_data->spim;

	nrfx_spim_abort(spim);
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
	const struct spi_nrfx_common_config *dev_config = dev->config;
	int ret;

	if (dev_data->configured) {
		nrfx_spim_uninit(&dev_data->spim);
		dev_data->configured = false;
	}

	ret = cs_put(dev);
	if (ret) {
		return ret;
	}

	ret = pinctrl_apply_state(dev_config->pcfg, PINCTRL_STATE_SLEEP);
	if (ret) {
		return ret;
	}

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

	ret = pinctrl_apply_state(dev_config->pcfg, PINCTRL_STATE_SLEEP);
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
