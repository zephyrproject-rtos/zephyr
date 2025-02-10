/*
 * Copyright (c) 2024 Semtech Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>

#include <zephyr/lora_lbm/lora_lbm_transceiver.h>

#include "lr11xx_hal_context.h"

LOG_MODULE_REGISTER(lora_lr11xx, CONFIG_LORA_BASICS_MODEM_DRIVERS_LOG_LEVEL);

#define LR11XX_SPI_OPERATION (SPI_WORD_SET(8) | SPI_OP_MODE_MASTER | SPI_TRANSFER_MSB)

/**
 * @brief Event pin callback handler.
 *
 * @param dev
 * @param cb
 * @param pins
 */
static void lr11xx_board_event_callback(const struct device *dev, struct gpio_callback *cb,
					uint32_t pins)
{
	struct lr11xx_hal_context_data_t *data =
		CONTAINER_OF(cb, struct lr11xx_hal_context_data_t, event_cb);
	const struct lr11xx_hal_context_cfg_t *config = data->lr11xx_dev->config;

	if ((pins & BIT(config->event.pin)) == 0U) {
		return;
	}

	if (gpio_pin_get_dt(&config->event)) {
		/* Wait for value to drop */
		gpio_pin_interrupt_configure_dt(&config->event, GPIO_INT_EDGE_TO_INACTIVE);
		/* Call provided callback */
#ifdef CONFIG_LORA_BASICS_MODEM_DRIVERS_EVENT_TRIGGER_OWN_THREAD
		k_sem_give(&data->gpio_sem);
#elif CONFIG_LORA_BASICS_MODEM_DRIVERS_EVENT_TRIGGER_GLOBAL_THREAD
		k_work_submit(&data->work);
#endif
	} else {
		gpio_pin_interrupt_configure_dt(&config->event, GPIO_INT_EDGE_TO_ACTIVE);
	}
}

#ifdef CONFIG_LORA_BASICS_MODEM_DRIVERS_EVENT_TRIGGER_OWN_THREAD
static void lr11xx_thread(struct lr11xx_hal_context_data_t *data)
{
	while (1) {
		k_sem_take(&data->gpio_sem, K_FOREVER);
		if (data->event_interrupt_cb) {
			data->event_interrupt_cb(data->lr11xx_dev);
		}
	}
}
#endif /* CONFIG_LORA_BASICS_MODEM_DRIVERS_EVENT_TRIGGER_OWN_THREAD */

#ifdef CONFIG_LORA_BASICS_MODEM_DRIVERS_EVENT_TRIGGER_GLOBAL_THREAD
static void lr11xx_work_cb(struct k_work *work)
{
	struct lr11xx_hal_context_data_t *data =
		CONTAINER_OF(work, struct lr11xx_hal_context_data_t, work);
	if (data->event_interrupt_cb) {
		data->event_interrupt_cb(data->lr11xx_dev);
	}
}
#endif /* CONFIG_LORA_BASICS_MODEM_DRIVERS_EVENT_TRIGGER_GLOBAL_THREAD */

void lora_transceiver_board_attach_interrupt(const struct device *dev, event_cb_t cb)
{
#ifdef CONFIG_LORA_BASICS_MODEM_DRIVERS_EVENT_TRIGGER
	struct lr11xx_hal_context_data_t *data = dev->data;

	data->event_interrupt_cb = cb;
#else
	LOG_ERR("Event trigger not supported!");
#endif /* CONFIG_LORA_BASICS_MODEM_DRIVERS_EVENT_TRIGGER */
}

void lora_transceiver_board_enable_interrupt(const struct device *dev)
{
#ifdef CONFIG_LORA_BASICS_MODEM_DRIVERS_EVENT_TRIGGER
	const struct lr11xx_hal_context_cfg_t *config = dev->config;

	gpio_pin_interrupt_configure_dt(&config->event, GPIO_INT_EDGE_TO_ACTIVE);
#else
	LOG_ERR("Event trigger not supported!");
#endif /* CONFIG_LORA_BASICS_MODEM_DRIVERS_EVENT_TRIGGER */
}

void lora_transceiver_board_disable_interrupt(const struct device *dev)
{
#ifdef CONFIG_LORA_BASICS_MODEM_DRIVERS_EVENT_TRIGGER
	const struct lr11xx_hal_context_cfg_t *config = dev->config;

	gpio_pin_interrupt_configure_dt(&config->event, GPIO_INT_DISABLE);
#else
	LOG_ERR("Event trigger not supported!");
#endif /* CONFIG_LORA_BASICS_MODEM_DRIVERS_EVENT_TRIGGER */
}

uint32_t lora_transceiver_get_tcxo_startup_delay_ms(const struct device *dev)
{
	const struct lr11xx_hal_context_cfg_t *config = dev->config;

	return config->tcxo_cfg.wakeup_time_ms;
}

int32_t lora_transceiver_get_model(const struct device *dev)
{
	const struct lr11xx_hal_context_cfg_t *config = dev->config;

	return config->chip_type;
}

/**
 * @brief Initialise lr11xx.
 * Initialise all GPIOs and configure interrupt on event pin.
 *
 * @param dev
 * @return int
 */
static int lr11xx_init(const struct device *dev)
{
	const struct lr11xx_hal_context_cfg_t *config = dev->config;
	struct lr11xx_hal_context_data_t *data = dev->data;
	int ret;

	/* Check the SPI device */
	if (!device_is_ready(config->spi.bus)) {
		LOG_ERR("Could not find SPI device");
		return -EINVAL;
	}

	/* Busy pin */
	ret = gpio_pin_configure_dt(&config->busy, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Could not configure busy gpio");
		return ret;
	}

	/* Reset pin */
	ret = gpio_pin_configure_dt(&config->reset, GPIO_OUTPUT_INACTIVE);
	if (ret < 0) {
		LOG_ERR("Could not configure reset gpio");
		return ret;
	}

	/* Event pin */
	ret = gpio_pin_configure_dt(&config->event, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Could not configure event gpio");
		return ret;
	}

	data->lr11xx_dev = dev;
	data->radio_status = RADIO_AWAKE;
	data->tx_offset = config->tx_offset;

	/* Event pin trigger config */
#ifdef CONFIG_LORA_BASICS_MODEM_DRIVERS_EVENT_TRIGGER
#ifdef CONFIG_LORA_BASICS_MODEM_DRIVERS_EVENT_TRIGGER_GLOBAL_THREAD
	data->work.handler = lr11xx_work_cb;
#elif CONFIG_LORA_BASICS_MODEM_DRIVERS_EVENT_TRIGGER_OWN_THREAD
	k_sem_init(&data->trig_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&data->thread, data->thread_stack,
			CONFIG_LORA_BASICS_MODEM_DRIVERS_EVENT_TRIGGER_THREAD_STACK_SIZE,
			(k_thread_entry_t)lr11xx_thread, data, NULL, NULL,
			K_PRIO_COOP(CONFIG_LORA_BASICS_MODEM_DRIVERS_EVENT_TRIGGER_THREAD_PRIORITY),
			0, K_NO_WAIT);
#endif /* CONFIG_LORA_BASICS_MODEM_DRIVERS_EVENT_TRIGGER_OWN_THREAD */

	/* Init callback */
	gpio_init_callback(&data->event_cb, lr11xx_board_event_callback, BIT(config->event.pin));
	/* Add callback */
	if (gpio_add_callback(config->event.port, &data->event_cb)) {
		LOG_ERR("Could not set event pin callback");
		return -EIO;
	}
#endif /* CONFIG_LORA_BASICS_MODEM_DRIVERS_EVENT_TRIGGER */
	return ret;
}

#if IS_ENABLED(CONFIG_PM_DEVICE)
/**
 * @brief Power management action define.
 * Not implemented yet.
 *
 * @param dev
 * @param action
 * @return int
 */
static int lr11xx_pm_action(const struct device *dev, enum pm_device_action action)
{
	int ret = 0;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		/* Put the lr11xx into normal operation mode */
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		/* Put the lr11xx into sleep mode */
		break;
	default:
		return -ENOTSUP;
	}

	return ret;
}
#endif /* IS_ENABLED(CONFIG_PM_DEVICE) */

/*
 * Device creation macro.
 */

#define DT_PROP_BY_IDX_U8(node_id, prop, idx) ((uint8_t)DT_PROP_BY_IDX(node_id, prop, idx))

#define DT_TABLE_U8(node_id, prop)                                                                 \
	{DT_FOREACH_PROP_ELEM_SEP(node_id, prop, DT_PROP_BY_IDX_U8, (, ))}

#define CONFIGURE_GPIO_IF_IN_DT(node_id, name, dt_prop)                                            \
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, dt_prop),                                            \
		    (.name = GPIO_DT_SPEC_GET(node_id, dt_prop),), ())

#define LR11XX_CHIP_TYPE(node_id)                                                                  \
	COND_CODE_1(DT_NODE_HAS_COMPAT(node_id, semtech_lr1110),                                   \
		    (LR11XX_SYSTEM_VERSION_TYPE_LR1110),                                           \
		    (COND_CODE_1(DT_NODE_HAS_COMPAT(node_id, semtech_lr1120),                      \
				 (LR11XX_SYSTEM_VERSION_TYPE_LR1120),                              \
				 (COND_CODE_1(DT_NODE_HAS_COMPAT(node_id, semtech_lr1121),         \
					      (LR11XX_SYSTEM_VERSION_TYPE_LR1121), (/**/))))))

#define LR11XX_XOSC_CFG(node_id)                                                                   \
	COND_CODE_1(DT_PROP(node_id, tcxo_wakeup_time) == 0,                                       \
		    (RAL_XOSC_CFG_XTAL), (RAL_XOSC_CFG_TCXO_RADIO_CTRL))

/* LR11xx does not support external TCXO control (by the MCU) */
#define LR11XX_CFG_TCXO(node_id)                                                                   \
	.tcxo_cfg = {                                                                              \
		.xosc_cfg = LR11XX_XOSC_CFG(node_id),                                              \
		.voltage = DT_PROP(node_id, tcxo_voltage),                                         \
		.wakeup_time_ms = DT_PROP(node_id, tcxo_wakeup_time),                              \
	}

#define LR11XX_CFG_LF_CLCK(node_id)                                                                \
	.lf_clck_cfg = {                                                                           \
		.lf_clk_cfg = DT_PROP(node_id, lf_clk),                                            \
		.wait_32k_ready = true,                                                            \
	}

#define LR11XX_CFG_RF_SW(node_id)                                                                  \
	.rf_switch_cfg = {                                                                         \
		.enable = DT_PROP_OR(node_id, rf_sw_enable, 0),                                    \
		.standby = DT_PROP_OR(node_id, rf_sw_standby_mode, 0),                             \
		.rx = DT_PROP_OR(node_id, rf_sw_rx_mode, 0),                                       \
		.tx = DT_PROP_OR(node_id, rf_sw_tx_mode, 0),                                       \
		.tx_hp = DT_PROP_OR(node_id, rf_sw_tx_hp_mode, 0),                                 \
		.tx_hf = DT_PROP_OR(node_id, rf_sw_tx_hf_mode, 0),                                 \
		.wifi = DT_PROP_OR(node_id, rf_sw_wifi_mode, 0),                                   \
		.gnss = DT_PROP_OR(node_id, rf_sw_gnss_mode, 0),                                   \
	}

#define LR11XX_RSSI_CFG(node_id, range)                                                            \
	{                                                                                          \
		.gain_offset = DT_PROP(node_id, DT_CAT3(rssi_calibration_, range, _offset)),       \
		.gain_tune = DT_TABLE_U8(node_id, DT_CAT3(rssi_calibration_, range, _tune)),       \
	}

#define LR11XX_CONFIG(node_id)                                                                     \
	{                                                                                          \
		.spi = SPI_DT_SPEC_GET(node_id, LR11XX_SPI_OPERATION, 0),                          \
		.reset = GPIO_DT_SPEC_GET(node_id, reset_gpios),                                   \
		.busy = GPIO_DT_SPEC_GET(node_id, busy_gpios),                                     \
		.event = GPIO_DT_SPEC_GET(node_id, event_gpios),                                   \
		.lf_tx_path_options = DT_PROP(node_id, lf_tx_path),                                \
		.chip_type = LR11XX_CHIP_TYPE(node_id),                                            \
		LR11XX_CFG_TCXO(node_id),                                                          \
		LR11XX_CFG_LF_CLCK(node_id),                                                       \
		LR11XX_CFG_RF_SW(node_id),                                                         \
		.reg_mode = DT_PROP(node_id, reg_mode),                                            \
		.rx_boosted = DT_PROP(node_id, rx_boosted),                                        \
		.tx_offset = DT_PROP_OR(node_id, tx_offset, 0),                                    \
		.pa_lf_lp_cfg_table = (lr11xx_pa_pwr_cfg_t *)DT_CAT(pa_lf_lp_cfg_table_, node_id), \
		.pa_lf_hp_cfg_table = (lr11xx_pa_pwr_cfg_t *)DT_CAT(pa_lf_hp_cfg_table_, node_id), \
		.pa_hf_cfg_table = (lr11xx_pa_pwr_cfg_t *)DT_CAT(pa_hf_cfg_table_, node_id),       \
		.rssi_calibration_table_below_600mhz = LR11XX_RSSI_CFG(node_id, lf),               \
		.rssi_calibration_table_from_600mhz_to_2ghz = LR11XX_RSSI_CFG(node_id, mf),        \
		.rssi_calibration_table_above_2ghz = LR11XX_RSSI_CFG(node_id, hf),                 \
	}

#define LR11XX_DEVICE_INIT(node_id)                                                                \
	DEVICE_DT_DEFINE(node_id, lr11xx_init, PM_DEVICE_DT_GET(node_id), &lr11xx_data_##node_id,  \
			 &lr11xx_config_##node_id, POST_KERNEL,                                    \
			 CONFIG_LORA_BASICS_MODEM_DRIVERS_INIT_PRIORITY, NULL);

#define LR11XX_DEFINE(node_id)                                                                     \
	static struct lr11xx_hal_context_data_t lr11xx_data_##node_id;                             \
	static uint8_t pa_lf_lp_cfg_table_##node_id[] = DT_TABLE_U8(node_id, tx_power_cfg_lf_lp);  \
	static uint8_t pa_lf_hp_cfg_table_##node_id[] = DT_TABLE_U8(node_id, tx_power_cfg_lf_hp);  \
	static uint8_t pa_hf_cfg_table_##node_id[] = DT_TABLE_U8(node_id, tx_power_cfg_hf);        \
	static const struct lr11xx_hal_context_cfg_t lr11xx_config_##node_id =                     \
		LR11XX_CONFIG(node_id);                                                            \
	PM_DEVICE_DT_DEFINE(node_id, lr11xx_pm_action);                                            \
	LR11XX_DEVICE_INIT(node_id)

DT_FOREACH_STATUS_OKAY(semtech_lr1110, LR11XX_DEFINE)
DT_FOREACH_STATUS_OKAY(semtech_lr1120, LR11XX_DEFINE)
DT_FOREACH_STATUS_OKAY(semtech_lr1121, LR11XX_DEFINE)
