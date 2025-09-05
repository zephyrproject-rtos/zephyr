/*
 * Copyright (c) 2025 Embeint Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/pm/device.h>

#include "ral.h"

#include "ralf_sx127x.h"
#include "ral_sx127x_bsp.h"
#include "sx127x_hal.h"
#include "sx127x.h"

#include "lbm_common.h"

struct lbm_sx127x_config {
	struct lbm_lora_config_common lbm_common;
	struct spi_dt_spec spi;
	struct gpio_dt_spec reset;
	struct gpio_dt_spec ant_enable;
	struct gpio_dt_spec rfi_enable;
	struct gpio_dt_spec rfo_enable;
	struct gpio_dt_spec pa_boost_enable;
	struct gpio_dt_spec tcxo_power;
	const struct gpio_dt_spec *dios;
	uint8_t num_dios;
};

struct lbm_sx127x_dio_package {
	uint8_t idx;
	struct gpio_callback callback;
	struct k_work worker;
};

struct lbm_sx127x_data {
	struct lbm_lora_data_common lbm_common;
	sx127x_t radio;
	const struct device *dev;
	struct lbm_sx127x_dio_package dio_packages[3];
	struct k_timer timer;
	void (*timer_cb)(void *context);
	bool asleep;
};

LOG_MODULE_DECLARE(lbm_driver, CONFIG_LORA_LOG_LEVEL);

static int sx127x_transceive(const struct device *dev, uint8_t reg, bool write, void *data,
			     size_t length)
{
	const struct lbm_sx127x_config *config = dev->config;
	const struct spi_buf buf[2] = {
		{
			.buf = &reg,
			.len = sizeof(reg),
		},
		{
			.buf = data,
			.len = length,
		},
	};
	struct spi_buf_set tx = {
		.buffers = buf,
		.count = ARRAY_SIZE(buf),
	};

	if (!write) {
		const struct spi_buf_set rx = {.buffers = buf, .count = ARRAY_SIZE(buf)};

		return spi_transceive_dt(&config->spi, &tx, &rx);
	}

	return spi_write_dt(&config->spi, &tx);
}

static int sx127x_write(const struct device *dev, uint8_t reg_addr, uint8_t *data, uint8_t len)
{
	return sx127x_transceive(dev, reg_addr | BIT(7), true, data, len);
}

static int sx127x_read(const struct device *dev, uint8_t reg_addr, uint8_t *data, uint8_t len)
{
	return sx127x_transceive(dev, reg_addr, false, data, len);
}

sx127x_radio_id_t sx127x_hal_get_radio_id(const sx127x_t *radio)
{
#if defined(SX1272)
	return SX127X_RADIO_ID_SX1272;
#elif defined(SX1276)
	return SX127X_RADIO_ID_SX1276;
#else
#error "Please define the radio to be used"
#endif
}

sx127x_hal_status_t sx127x_hal_write(const sx127x_t *radio, const uint16_t address,
				     const uint8_t *data, const uint16_t data_len)
{
	const struct device *dev = radio->hal_context;
	int ret;

	LOG_DBG("ADDR=0x%02x DATA_LEN=%d", address, data_len);

	/* Only 7 bit addresses make any sense */
	__ASSERT_NO_MSG(address < BIT(7));

	ret = sx127x_write(dev, address, (uint8_t *)data, data_len);
	return ret == 0 ? SX127X_HAL_STATUS_OK : SX127X_HAL_STATUS_ERROR;
}

sx127x_hal_status_t sx127x_hal_read(const sx127x_t *radio, const uint16_t address, uint8_t *data,
				    const uint16_t data_len)
{
	const struct device *dev = radio->hal_context;
	int ret;

	LOG_DBG("ADDR=0x%02x DATA_LEN=%d", address, data_len);

	/* Only 7 bit addresses make any sense */
	__ASSERT_NO_MSG(address < BIT(7));

	ret = sx127x_read(dev, address, data, data_len);
	return ret == 0 ? SX127X_HAL_STATUS_OK : SX127X_HAL_STATUS_ERROR;
}

void sx127x_hal_reset(const sx127x_t *radio)
{
	const struct device *dev = radio->hal_context;
	const struct lbm_sx127x_config *config = dev->config;

	LOG_DBG("");

	/* Assert reset pin for >= 100 us */
	gpio_pin_set_dt(&config->reset, 1);
	k_sleep(K_MSEC(1));
	gpio_pin_set_dt(&config->reset, 0);

	/* Wait >= 5ms for modem to be ready again */
	k_sleep(K_MSEC(50));
}

uint32_t sx127x_hal_get_dio_1_pin_state(const sx127x_t *radio)
{
	const struct device *dev = radio->hal_context;
	const struct lbm_sx127x_config *config = dev->config;

	return gpio_pin_get_dt(&config->dios[0]);
}

static void sx127x_timer_expiry(struct k_timer *timer)
{
	struct lbm_sx127x_data *data = CONTAINER_OF(timer, struct lbm_sx127x_data, timer);

	LOG_DBG("");

	/* Run the provided callback */
	data->timer_cb((void *)&data->radio);
}

sx127x_hal_status_t sx127x_hal_timer_start(const sx127x_t *radio, const uint32_t time_in_ms,
					   void (*callback)(void *context))
{
	const struct device *dev = radio->hal_context;
	struct lbm_sx127x_data *data = dev->data;

	if (callback == NULL) {
		return SX127X_HAL_STATUS_ERROR;
	}

	LOG_DBG("Starting %d ms timer", time_in_ms);

	/* Update internal state */
	data->timer_cb = callback;

	/* Start the timer */
	k_timer_start(&data->timer, K_MSEC(time_in_ms), K_FOREVER);
	return SX127X_HAL_STATUS_OK;
}

sx127x_hal_status_t sx127x_hal_timer_stop(const sx127x_t *radio)
{
	const struct device *dev = radio->hal_context;
	struct lbm_sx127x_data *data = dev->data;

	LOG_DBG("");

	k_timer_stop(&data->timer);
	return SX127X_HAL_STATUS_OK;
}

bool sx127x_hal_timer_is_started(const sx127x_t *radio)
{
	const struct device *dev = radio->hal_context;
	struct lbm_sx127x_data *data = dev->data;

	return k_timer_remaining_get(&data->timer) > 0;
}

void ral_sx127x_bsp_get_tx_cfg(const void *context,
			       const ral_sx127x_bsp_tx_cfg_input_params_t *input_params,
			       ral_sx127x_bsp_tx_cfg_output_params_t *output_params)

{
	int16_t power = input_params->system_output_pwr_in_dbm;
#if defined(SX1272)
	output_params->pa_cfg.pa_select = SX127X_PA_SELECT_RFO;
	output_params->pa_cfg.is_20_dbm_output_on = false;
#elif defined(SX1276)
	if (input_params->freq_in_hz > RF_FREQUENCY_MID_BAND_THRESHOLD) {
		output_params->pa_cfg.pa_select = SX127X_PA_SELECT_BOOST;
		output_params->pa_cfg.is_20_dbm_output_on = true;
	} else {
		output_params->pa_cfg.pa_select = SX127X_PA_SELECT_RFO;
		output_params->pa_cfg.is_20_dbm_output_on = false;
	}
#endif

	if (output_params->pa_cfg.pa_select == SX127X_PA_SELECT_BOOST) {
		if (output_params->pa_cfg.is_20_dbm_output_on == true) {
			power = MAX(power, 5);
			power = MIN(power, 20);
		} else {
			power = MAX(power, 2);
			power = MIN(power, 17);
		}
	} else {
#if defined(SX1272)
		power = MAX(power, -1);
		power = MIN(power, 14);
#elif defined(SX1276)
		power = MAX(power, -4);
		power = MIN(power, 15);
#endif
	}

	output_params->chip_output_pwr_in_dbm_configured = power;
	output_params->chip_output_pwr_in_dbm_expected = power;
	output_params->pa_ramp_time = SX127X_RAMP_40_US;
}

void ral_sx127x_bsp_get_ocp_value(const void *context, uint8_t *ocp_trim_value)
{
	/* Do nothing, let the driver choose the default values */
}

ral_status_t ral_sx127x_bsp_get_instantaneous_tx_power_consumption(
	const void *context,
	const ral_sx127x_bsp_tx_cfg_output_params_t *tx_cfg_output_params_local,
	uint32_t *pwr_consumption_in_ua)
{
	/* Not yet implemented, not relevant for LoRa */
	return RAL_STATUS_UNSUPPORTED_FEATURE;
}

ral_status_t
ral_sx127x_bsp_get_instantaneous_gfsk_rx_power_consumption(const void *context, bool rx_boosted,
							   uint32_t *pwr_consumption_in_ua)
{
	/* Not yet implemented, not relevant for LoRa */
	return RAL_STATUS_UNSUPPORTED_FEATURE;
}

ral_status_t
ral_sx127x_bsp_get_instantaneous_lora_rx_power_consumption(const void *context, bool rx_boosted,
							   uint32_t *pwr_consumption_in_ua)

{
	/* Not yet implemented, not relevant for LoRa */
	return RAL_STATUS_UNSUPPORTED_FEATURE;
}

void lbm_driver_antenna_configure(const struct device *dev, enum lbm_modem_mode mode)
{
	const struct lbm_sx127x_config *config = dev->config;

	/* Antenna / RF switch control */
	switch (mode) {
	case MODE_SLEEP:
		lbm_optional_gpio_set_dt(&config->pa_boost_enable, 0);
		lbm_optional_gpio_set_dt(&config->ant_enable, 0);
		lbm_optional_gpio_set_dt(&config->rfi_enable, 0);
		lbm_optional_gpio_set_dt(&config->rfo_enable, 0);
		break;
	case MODE_TX:
	case MODE_CW:
		lbm_optional_gpio_set_dt(&config->rfi_enable, 0);
		lbm_optional_gpio_set_dt(&config->pa_boost_enable, 1);
		lbm_optional_gpio_set_dt(&config->rfo_enable, 1);
		lbm_optional_gpio_set_dt(&config->ant_enable, 1);
		break;
	case MODE_RX:
	case MODE_RX_ASYNC:
	case MODE_CAD:
		lbm_optional_gpio_set_dt(&config->pa_boost_enable, 0);
		lbm_optional_gpio_set_dt(&config->rfo_enable, 0);
		lbm_optional_gpio_set_dt(&config->rfi_enable, 1);
		lbm_optional_gpio_set_dt(&config->ant_enable, 1);
		break;
	}
}

static void sx127x_dio_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	/* Get DIO container to find the DIO index */
	struct lbm_sx127x_dio_package *dio_package =
		CONTAINER_OF(cb, struct lbm_sx127x_dio_package, callback);
	uint8_t dio = dio_package->idx;
	/* Get the parent data structure */
	struct lbm_sx127x_data *data =
		CONTAINER_OF(dio_package, struct lbm_sx127x_data, dio_packages[dio]);

	LOG_DBG("%d", dio);

	__ASSERT_NO_MSG(dio <= 3);
	/* Submit work to process the interrupt immediately */
	k_work_submit(&data->dio_packages[dio].worker);
}

static void dio_work_function(struct k_work *work)
{
	/* Get DIO container to find the DIO index */
	struct lbm_sx127x_dio_package *dio_package =
		CONTAINER_OF(work, struct lbm_sx127x_dio_package, worker);
	uint8_t dio = dio_package->idx;
	/* Get the parent data structure */
	struct lbm_sx127x_data *data =
		CONTAINER_OF(dio_package, struct lbm_sx127x_data, dio_packages[dio]);

	switch (dio) {
	case 0:
		data->radio.dio_0_irq_handler(&data->radio);
		break;
	case 1:
		data->radio.dio_1_irq_handler(&data->radio);
		break;
	case 2:
		data->radio.dio_2_irq_handler(&data->radio);
		break;
	default:
		LOG_WRN("Unknown DIO %d", dio);
	}
}

void sx127x_hal_dio_irq_attach(const sx127x_t *radio)
{
	/* Nothing to do here */
}

static void sx127x_irq_handler(void *irq_context)
{
	const struct device *dev = irq_context;
	struct lbm_sx127x_data *data = dev->data;

	/* Finish the current task from the common worker */
	k_work_reschedule(&data->lbm_common.op_done_work, K_NO_WAIT);
}

static int sx127x_driver_init(const struct device *dev)
{
	const struct lbm_sx127x_config *config = dev->config;
	struct lbm_sx127x_data *data = dev->data;
	ral_status_t status;

	data->radio.hal_context = dev;
	data->radio.irq_handler_context = (void *)dev;
	data->radio.irq_handler = sx127x_irq_handler;
	k_timer_init(&data->timer, sx127x_timer_expiry, NULL);

	/* Validate hardware is ready */
	if (!spi_is_ready_dt(&config->spi)) {
		LOG_ERR("SPI bus %s not ready", config->spi.bus->name);
		return -ENODEV;
	}

	/* Setup GPIOs */
	if (gpio_pin_configure_dt(&config->reset, GPIO_OUTPUT_INACTIVE)) {
		LOG_ERR("GPIO configuration failed.");
		return -EIO;
	}
	if (config->ant_enable.port) {
		gpio_pin_configure_dt(&config->ant_enable, GPIO_OUTPUT_INACTIVE);
	}
	if (config->rfi_enable.port) {
		gpio_pin_configure_dt(&config->rfi_enable, GPIO_OUTPUT_INACTIVE);
	}
	if (config->rfo_enable.port) {
		gpio_pin_configure_dt(&config->rfo_enable, GPIO_OUTPUT_INACTIVE);
	}
	if (config->pa_boost_enable.port) {
		gpio_pin_configure_dt(&config->pa_boost_enable, GPIO_OUTPUT_INACTIVE);
	}
	if (config->tcxo_power.port) {
		gpio_pin_configure_dt(&config->tcxo_power, GPIO_OUTPUT_INACTIVE);
	}

	/* Configure interrupts */
	for (int i = 0; i < MIN(config->num_dios, 3); i++) {
		data->dio_packages[i].idx = i;
		k_work_init(&data->dio_packages[i].worker, dio_work_function);

		gpio_pin_configure_dt(&config->dios[i], GPIO_INPUT);
		gpio_init_callback(&data->dio_packages[i].callback, sx127x_dio_callback,
				   BIT(config->dios[i].pin));
		if (gpio_add_callback(config->dios[i].port, &data->dio_packages[i].callback) < 0) {
			LOG_ERR("Could not set GPIO callback for DIO%d interrupt.", i);
			return -EIO;
		}
		gpio_pin_interrupt_configure_dt(&config->dios[i], GPIO_INT_EDGE_RISING);
	}

	/* Reset chip on boot */
	status = ral_reset(&config->lbm_common.ralf.ral);
	if (status != RAL_STATUS_OK) {
		LOG_ERR("Reset failure (%d)", status);
		return -EIO;
	}

	/* Common structure init */
	return lbm_lora_common_init(dev);
}

#define SX127X_DIO_GPIO_ELEM(idx, node_id) GPIO_DT_SPEC_GET_BY_IDX(node_id, dio_gpios, idx)

#define SX127X_DIO_GPIO_INIT(node_id)                                                              \
	LISTIFY(DT_PROP_LEN(node_id, dio_gpios), SX127X_DIO_GPIO_ELEM, (,), node_id)

#define SX127X_DEFINE(node_id)                                                                     \
	static const struct gpio_dt_spec sx127x_dios_##node_id[] = {                               \
		SX127X_DIO_GPIO_INIT(node_id)};                                                    \
	BUILD_ASSERT(ARRAY_SIZE(sx127x_dios_##node_id) >= 1);                                      \
	static struct lbm_sx127x_data data_##node_id;                                              \
	static const struct lbm_sx127x_config config_##node_id = {                                 \
		.lbm_common.ralf = RALF_SX127X_INSTANTIATE(&data_##node_id.radio),                 \
		.spi = SPI_DT_SPEC_GET(                                                            \
			node_id, SPI_WORD_SET(8) | SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB, 0),      \
		.reset = GPIO_DT_SPEC_GET(node_id, reset_gpios),                                   \
		.ant_enable = GPIO_DT_SPEC_GET_OR(node_id, antenna_enable_gpios, {0}),             \
		.rfi_enable = GPIO_DT_SPEC_GET_OR(node_id, rfi_enable_gpios, {0}),                 \
		.rfo_enable = GPIO_DT_SPEC_GET_OR(node_id, rfo_enable_gpios, {0}),                 \
		.pa_boost_enable = GPIO_DT_SPEC_GET_OR(node_id, pa_boost_enable_gpios, {0}),       \
		.tcxo_power = GPIO_DT_SPEC_GET_OR(node_id, tcxo_power_gpios, {0}),                 \
		.dios = sx127x_dios_##node_id,                                                     \
		.num_dios = ARRAY_SIZE(sx127x_dios_##node_id),                                     \
	};                                                                                         \
	DEVICE_DT_DEFINE(node_id, sx127x_driver_init, NULL, &data_##node_id, &config_##node_id,    \
			 POST_KERNEL, CONFIG_LORA_INIT_PRIORITY, &lbm_lora_api)

DT_FOREACH_STATUS_OKAY(semtech_sx1272, SX127X_DEFINE);
DT_FOREACH_STATUS_OKAY(semtech_sx1276, SX127X_DEFINE);
