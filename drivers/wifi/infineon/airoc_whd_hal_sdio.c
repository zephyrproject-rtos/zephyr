/*
 * Copyright (c) 2023 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "airoc_wifi.h"
#include "airoc_whd_hal_common.h"

#include <bus_protocols/whd_bus_sdio_protocol.h>
#include <bus_protocols/whd_bus.h>
#include <bus_protocols/whd_sdio.h>
#include <zephyr/sd/sd.h>

LOG_MODULE_DECLARE(infineon_airoc_wifi, CONFIG_WIFI_LOG_LEVEL);

#ifdef __cplusplus
extern "C" {
#endif

struct whd_bus_priv {
	whd_sdio_config_t sdio_config;
	whd_bus_stats_t whd_bus_stats;
	whd_sdio_t sdio_obj;
};

static whd_init_config_t init_config_default = {.thread_stack_size = CY_WIFI_THREAD_STACK_SIZE,
						.thread_stack_start = NULL,
						.thread_priority =
							(uint32_t)CY_WIFI_THREAD_PRIORITY,
						.country = CY_WIFI_COUNTRY};

/******************************************************
 *                 Function
 ******************************************************/

int airoc_wifi_init_primary(const struct device *dev, whd_interface_t *interface,
			    whd_netif_funcs_t *netif_funcs, whd_buffer_funcs_t *buffer_if)
{
	int ret;
	struct airoc_wifi_data *data = dev->data;
	const struct airoc_wifi_config *config = dev->config;

	whd_sdio_config_t whd_sdio_config = {
		.sdio_1bit_mode = WHD_FALSE,
		.high_speed_sdio_clock = WHD_FALSE,
	};

#if DT_INST_NODE_HAS_PROP(0, wifi_host_wake_gpios)
	whd_oob_config_t oob_config = {
		.host_oob_pin = (void *)&config->wifi_host_wake_gpio,
		.dev_gpio_sel = DEFAULT_OOB_PIN,
		.is_falling_edge =
			(CY_WIFI_HOST_WAKE_IRQ_EVENT == GPIO_INT_TRIG_LOW) ? WHD_TRUE : WHD_FALSE,
		.intr_priority = CY_WIFI_OOB_INTR_PRIORITY};
	whd_sdio_config.oob_config = oob_config;
#endif

	if (airoc_wifi_power_on(dev)) {
		LOG_ERR("airoc_wifi_power_on retuens fail");
		return -ENODEV;
	}

	if (!device_is_ready(config->bus_dev.bus_sdio)) {
		LOG_ERR("SDHC device is not ready");
		return -ENODEV;
	}

	ret = sd_init(config->bus_dev.bus_sdio, &data->card);
	if (ret) {
		return ret;
	}

	/* Init SDIO functions */
	ret = sdio_init_func(&data->card, &data->sdio_func1, BACKPLANE_FUNCTION);
	if (ret) {
		LOG_ERR("sdio_enable_func BACKPLANE_FUNCTION, error: %x", ret);
		return ret;
	}
	ret = sdio_init_func(&data->card, &data->sdio_func2, WLAN_FUNCTION);
	if (ret) {
		LOG_ERR("sdio_enable_func WLAN_FUNCTION, error: %x", ret);
		return ret;
	}
	ret = sdio_set_block_size(&data->sdio_func1, SDIO_64B_BLOCK);
	if (ret) {
		LOG_ERR("Can't set block size for BACKPLANE_FUNCTION, error: %x", ret);
		return ret;
	}
	ret = sdio_set_block_size(&data->sdio_func2, SDIO_64B_BLOCK);
	if (ret) {
		LOG_ERR("Can't set block size for WLAN_FUNCTION, error: %x", ret);
		return ret;
	}

	/* Init wifi host driver (whd) */
	cy_rslt_t whd_ret = whd_init(&data->whd_drv, &init_config_default, &resource_ops, buffer_if,
				     netif_funcs);
	if (whd_ret == CY_RSLT_SUCCESS) {
		whd_ret = whd_bus_sdio_attach(data->whd_drv, &whd_sdio_config,
					      (whd_sdio_t)&data->card);

		if (whd_ret == CY_RSLT_SUCCESS) {
			whd_ret = whd_wifi_on(data->whd_drv, interface);
		}

		if (whd_ret != CY_RSLT_SUCCESS) {
			whd_deinit(*interface);
			return -ENODEV;
		}
	}
	return 0;
}

/*
 * Implement SDIO CMD52/53 wrappers
 */

static struct sdio_func *airoc_wifi_get_sdio_func(struct sd_card *sd, whd_bus_function_t function)
{
	struct airoc_wifi_data *data = CONTAINER_OF(sd, struct airoc_wifi_data, card);
	struct sdio_func *func[] = {&sd->func0, &data->sdio_func1, &data->sdio_func2};

	if (function > WLAN_FUNCTION) {
		return NULL;
	}

	return func[function];
}

whd_result_t whd_bus_sdio_cmd52(whd_driver_t whd_driver, whd_bus_transfer_direction_t direction,
				whd_bus_function_t function, uint32_t address, uint8_t value,
				sdio_response_needed_t response_expected, uint8_t *response)
{
	int ret;
	struct sd_card *sd = whd_driver->bus_priv->sdio_obj;
	struct sdio_func *func = airoc_wifi_get_sdio_func(sd, function);

	WHD_BUS_STATS_INCREMENT_VARIABLE(whd_driver->bus_priv, cmd52);

	if (direction == BUS_WRITE) {
		ret = sdio_rw_byte(func, address, value, response);
	} else {
		ret = sdio_read_byte(func, address, response);
	}
	WHD_BUS_STATS_CONDITIONAL_INCREMENT_VARIABLE(whd_driver->bus_priv, (ret != WHD_SUCCESS),
						     cmd52_fail);

	/* Possibly device might not respond to this cmd. So, don't check return value here */
	if ((ret != WHD_SUCCESS) && (address == SDIO_SLEEP_CSR)) {
		return ret;
	}

	CHECK_RETURN(ret);
	return WHD_SUCCESS;
}

whd_result_t whd_bus_sdio_cmd53(whd_driver_t whd_driver, whd_bus_transfer_direction_t direction,
				whd_bus_function_t function, sdio_transfer_mode_t mode,
				uint32_t address, uint16_t data_size, uint8_t *data,
				sdio_response_needed_t response_expected, uint32_t *response)
{
	whd_result_t ret;
	struct sd_card *sd = whd_driver->bus_priv->sdio_obj;
	struct sdio_func *func = airoc_wifi_get_sdio_func(sd, function);

	if (direction == BUS_WRITE) {
		WHD_BUS_STATS_INCREMENT_VARIABLE(whd_driver->bus_priv, cmd53_write);
		ret = sdio_write_addr(func, address, data, data_size);
	} else {
		WHD_BUS_STATS_INCREMENT_VARIABLE(whd_driver->bus_priv, cmd53_read);
		ret = sdio_read_addr(func, address, data, data_size);
	}

	WHD_BUS_STATS_CONDITIONAL_INCREMENT_VARIABLE(
		whd_driver->bus_priv, ((ret != WHD_SUCCESS) && (direction == BUS_READ)),
		cmd53_read_fail);
	WHD_BUS_STATS_CONDITIONAL_INCREMENT_VARIABLE(
		whd_driver->bus_priv, ((ret != WHD_SUCCESS) && (direction == BUS_WRITE)),
		cmd53_write_fail);

	CHECK_RETURN(ret);
	return WHD_SUCCESS;
}

/*
 * Implement SDIO Card interrupt
 */

void whd_bus_sdio_irq_handler(const struct device *dev, int reason, const void *user_data)
{
	if (reason == SDHC_INT_SDIO) {
		whd_driver_t whd_driver = (whd_driver_t)user_data;

		WHD_BUS_STATS_INCREMENT_VARIABLE(whd_driver->bus_priv, sdio_intrs);

		/* call thread notify to wake up WHD thread */
		whd_thread_notify_irq(whd_driver);
	}
}

whd_result_t whd_bus_sdio_irq_register(whd_driver_t whd_driver)
{
	/* Nothing to do here, all handles by whd_bus_sdio_irq_enable function */
	return WHD_SUCCESS;
}

whd_result_t whd_bus_sdio_irq_enable(whd_driver_t whd_driver, whd_bool_t enable)
{
	int ret;
	struct sd_card *sd = whd_driver->bus_priv->sdio_obj;

	/* Enable/disable SDIO Card interrupts */
	if (enable) {
		ret = sdhc_enable_interrupt(sd->sdhc, whd_bus_sdio_irq_handler, SDHC_INT_SDIO,
					    whd_driver);
	} else {
		ret = sdhc_disable_interrupt(sd->sdhc, SDHC_INT_SDIO);
	}
	return ret;
}

/*
 * Implement OOB functionality
 */

void whd_bus_sdio_oob_irq_handler(const struct device *port, struct gpio_callback *cb,
				  gpio_port_pins_t pins)
{
#if DT_INST_NODE_HAS_PROP(0, wifi_host_wake_gpios)
	struct airoc_wifi_data *data = CONTAINER_OF(cb, struct airoc_wifi_data, host_oob_pin_cb);

	/* Get OOB pin info */
	const whd_oob_config_t *oob_config = &data->whd_drv->bus_priv->sdio_config.oob_config;
	const struct gpio_dt_spec *host_oob_pin = oob_config->host_oob_pin;

	/* Check OOB state is correct */
	int expected_event = (oob_config->is_falling_edge == WHD_TRUE) ? 0 : 1;

	if (!(pins & BIT(host_oob_pin->pin)) || (gpio_pin_get_dt(host_oob_pin) != expected_event)) {
		WPRINT_WHD_ERROR(("Unexpected interrupt event %d\n", expected_event));
		WHD_BUS_STATS_INCREMENT_VARIABLE(data->whd_drv->bus_priv, error_intrs);
		return;
	}

	WHD_BUS_STATS_INCREMENT_VARIABLE(data->whd_drv->bus_priv, oob_intrs);

	/* Call thread notify to wake up WHD thread */
	whd_thread_notify_irq(data->whd_drv);

#endif /* DT_INST_NODE_HAS_PROP(0, wifi-host-wake-gpios) */
}

whd_result_t whd_bus_sdio_register_oob_intr(whd_driver_t whd_driver)
{
#if DT_INST_NODE_HAS_PROP(0, wifi_host_wake_gpios)
	int ret;
	const struct device *dev = DEVICE_DT_GET(DT_DRV_INST(0));
	struct airoc_wifi_data *data = dev->data;

	/* Get OOB pin info */
	const whd_oob_config_t *oob_config = &whd_driver->bus_priv->sdio_config.oob_config;
	const struct gpio_dt_spec *host_oob_pin = oob_config->host_oob_pin;

	/* Check if OOB pin is ready */
	if (!gpio_is_ready_dt(host_oob_pin)) {
		WPRINT_WHD_ERROR(("%s: Failed at gpio_is_ready_dt for host_oob_pin\n", __func__));
		return WHD_HAL_ERROR;
	}

	/* Configure OOB pin as output */
	ret = gpio_pin_configure_dt(host_oob_pin, GPIO_INPUT);
	if (ret != 0) {
		WPRINT_WHD_ERROR((
			" %s: Failed at gpio_pin_configure_dt for host_oob_pin, result code = %d\n",
			__func__, ret));
		return WHD_HAL_ERROR;
	}

	/* Initialize/add OOB pin callback */
	gpio_init_callback(&data->host_oob_pin_cb, whd_bus_sdio_oob_irq_handler,
			   BIT(host_oob_pin->pin));

	ret = gpio_add_callback_dt(host_oob_pin, &data->host_oob_pin_cb);
	if (ret != 0) {
		WPRINT_WHD_ERROR(
			("%s: Failed at gpio_add_callback_dt for host_oob_pin, result code = %d\n",
			 __func__, ret));
		return WHD_HAL_ERROR;
	}
#endif /* DT_INST_NODE_HAS_PROP(0, wifi_host_wake_gpios) */

	return WHD_SUCCESS;
}

whd_result_t whd_bus_sdio_unregister_oob_intr(whd_driver_t whd_driver)
{
#if DT_INST_NODE_HAS_PROP(0, wifi_host_wake_gpios)
	int ret;
	const whd_oob_config_t *oob_config = &whd_driver->bus_priv->sdio_config.oob_config;

	/* Disable OOB pin interrupts */
	ret = gpio_pin_interrupt_configure_dt(oob_config->host_oob_pin, GPIO_INT_DISABLE);
	if (ret != 0) {
		WPRINT_WHD_ERROR(("%s: Failed at gpio_pin_interrupt_configure_dt for host_oob_pin, "
				  "result code = %d\n",
				  __func__, ret));
		return WHD_HAL_ERROR;
	}
#endif /* DT_INST_NODE_HAS_PROP(0, wifi_host_wake_gpios) */
	return WHD_SUCCESS;
}

whd_result_t whd_bus_sdio_enable_oob_intr(whd_driver_t whd_driver, whd_bool_t enable)
{
#if DT_INST_NODE_HAS_PROP(0, wifi_host_wake_gpios)
	int ret;
	const whd_oob_config_t *oob_config = &whd_driver->bus_priv->sdio_config.oob_config;
	uint32_t trig_conf =
		(oob_config->is_falling_edge == WHD_TRUE) ? GPIO_INT_TRIG_LOW : GPIO_INT_TRIG_HIGH;

	/* Enable OOB pin interrupts */
	ret = gpio_pin_interrupt_configure_dt(oob_config->host_oob_pin,
					      GPIO_INT_ENABLE | GPIO_INT_EDGE | trig_conf);
	if (ret != 0) {
		WPRINT_WHD_ERROR(("%s: Failed at gpio_pin_interrupt_configure_dt for host_oob_pin, "
				  "result code = %d\n",
				  __func__, ret));
		return WHD_HAL_ERROR;
	}
#endif /* DT_INST_NODE_HAS_PROP(0, wifi_host_wake_gpios) */
	return WHD_SUCCESS;
}

#ifdef __cplusplus
} /* extern "C" */
#endif
