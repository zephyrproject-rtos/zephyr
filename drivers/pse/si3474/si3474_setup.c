#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "si3474.h"
#include "si3474_reg.h"

LOG_MODULE_DECLARE(SI3474, CONFIG_PSE_LOG_LEVEL);

static void si3474_gpio_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	struct si3474_data *drv_data = CONTAINER_OF(cb, struct si3474_data, gpio_cb);
	const struct si3474_config *cfg = drv_data->dev->config;

	ARG_UNUSED(pins);

	gpio_pin_interrupt_configure_dt(&cfg->gpio_int, GPIO_INT_DISABLE);

	k_sem_give(&drv_data->gpio_sem);
}

int si3474_event_trigger_set(const struct device *dev, pse_event_trigger_handler_t handler)
{
	struct si3474_data *drv_data = dev->data;
	const struct si3474_config *cfg = dev->config;
	int res = 0;

	gpio_pin_interrupt_configure_dt(&cfg->gpio_int, GPIO_INT_DISABLE);

	drv_data->event_ready_handler = handler;

	gpio_pin_interrupt_configure_dt(&cfg->gpio_int, GPIO_INT_EDGE_TO_INACTIVE);

	return res;
}

static void si3474_thread_cb(const struct device *dev)
{
	struct si3474_data *drv_data = dev->data;
	const struct si3474_config *cfg = dev->config;

	if (drv_data->event_ready_handler != NULL) {
		drv_data->event_ready_handler(dev);
	}

	gpio_pin_interrupt_configure_dt(&cfg->gpio_int, GPIO_INT_EDGE_TO_INACTIVE);
}

static void si3474_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	struct si3474_data *drv_data = p1;

	while (1) {
		k_sem_take(&drv_data->gpio_sem, K_FOREVER);
		si3474_thread_cb(drv_data->dev);
	}
}

int si3474_init_interrupt(const struct device *dev)
{
	struct si3474_data *drv_data = dev->data;
	const struct si3474_config *cfg = dev->config;
	int result = 0;

	if (!device_is_ready(cfg->gpio_int.port)) {
		LOG_ERR("gpio_int gpio not ready");
		return -ENODEV;
	}

	drv_data->dev = dev;

	gpio_pin_configure_dt(&cfg->gpio_int, GPIO_INPUT);
	gpio_init_callback(&drv_data->gpio_cb, si3474_gpio_callback, BIT(cfg->gpio_int.pin));
	result = gpio_add_callback(cfg->gpio_int.port, &drv_data->gpio_cb);

	if (result < 0) {
		LOG_ERR("Failed to set gpio callback");
		return result;
	}

	k_sem_init(&drv_data->gpio_sem, 0, K_SEM_MAX_LIMIT);

	k_thread_create(&drv_data->thread, drv_data->thread_stack, CONFIG_SI3474_THREAD_STACK_SIZE,
			si3474_thread, drv_data, NULL, NULL,
			K_PRIO_COOP(CONFIG_SI3474_THREAD_PRIORITY), 0, K_NO_WAIT);

	gpio_pin_interrupt_configure_dt(&cfg->gpio_int, GPIO_INT_EDGE_TO_INACTIVE);
	if (result < 0) {
		LOG_ERR("Failed to set gpio callback");
		return result;
	}

	return 0;
}

int si3474_init_ports(const struct device *dev)
{
	const struct si3474_config *cfg = dev->config;
	uint16_t dev_address = cfg->i2c.addr;
	uint8_t buff;
	int res;

	if (!device_is_ready(cfg->gpio_rst.port)) {
		LOG_ERR("gpio_rst pin not ready");
		return -ENODEV;
	}
	res = gpio_pin_configure_dt(&cfg->gpio_rst, GPIO_OUTPUT_ACTIVE);
	if (res < 0) {
		LOG_ERR("Failed to configure si3474 reset pin");
		return res;
	}
	if (!device_is_ready(cfg->gpio_oss.port)) {
		LOG_ERR("gpio_oss pin not ready");
		return -ENODEV;
	}
	res = gpio_pin_configure_dt(&cfg->gpio_oss, GPIO_OUTPUT_ACTIVE);
	if (res < 0) {
		LOG_ERR("Failed to configure si3474 oss pin");
		return res;
	}

	k_msleep(1000);
	res = gpio_pin_set_dt(&cfg->gpio_rst, true);
	if (res != 0) {
		LOG_WRN("Failed to assert si3474_rst %i", res);
	}
	/* Datasheet says RESETb timing pulses must be >10 µ */
	k_msleep(10);
	res = gpio_pin_set_dt(&cfg->gpio_rst, false);
	if (res != 0) {
		LOG_WRN("Failed to deassert si3474_rst %i", res);
	}
	/* Datasheet says the device is reachable via i2c 30ms after the reset */
	k_msleep(30);

	return res;
}
