/*
 * Copyright (c) 2026 Shreyas Acharya
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Data ready trigger for the BMI260.
 *
 * The interrupt arrives in interrupt context, where the bus cannot be used, so
 * the pin handler does nothing but hand the work to a thread. Which thread is
 * a build-time choice: a thread the driver owns, or the system workqueue.
 */

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "bmi260.h"
#include "bmi260_reg_map.h"

LOG_MODULE_DECLARE(bmi260, CONFIG_SENSOR_LOG_LEVEL);

/*
 * Runs in thread context. Reading the status register both tells the driver
 * what the device wanted and clears the condition, so the next edge can come.
 */
static void bmi260_handle_interrupt(const struct device *dev)
{
	struct bmi260_data *data = dev->data;
	uint8_t int_status;
	int ret;

	ret = bmi260_reg_read(dev, BMI260_REG_INT_STATUS_1, &int_status, sizeof(int_status));
	if (ret != 0) {
		LOG_ERR("Could not read the interrupt status (%d)", ret);
		return;
	}

	if ((int_status & (BMI260_INT_STATUS_1_ACC_DRDY_INT |
			   BMI260_INT_STATUS_1_GYR_DRDY_INT)) == 0U) {
		return;
	}

	if (data->drdy_handler != NULL) {
		data->drdy_handler(dev, data->drdy_trigger);
	}
}

static void bmi260_gpio_callback(const struct device *port, struct gpio_callback *cb,
				 uint32_t pins)
{
	struct bmi260_data *data = CONTAINER_OF(cb, struct bmi260_data, int1_cb);

	ARG_UNUSED(port);
	ARG_UNUSED(pins);

#if defined(CONFIG_BMI260_TRIGGER_OWN_THREAD)
	k_sem_give(&data->trig_sem);
#elif defined(CONFIG_BMI260_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->trig_work);
#endif
}

#if defined(CONFIG_BMI260_TRIGGER_OWN_THREAD)
static void bmi260_thread_main(void *p1, void *p2, void *p3)
{
	struct bmi260_data *data = p1;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (true) {
		k_sem_take(&data->trig_sem, K_FOREVER);
		bmi260_handle_interrupt(data->dev);
	}
}
#elif defined(CONFIG_BMI260_TRIGGER_GLOBAL_THREAD)
static void bmi260_work_handler(struct k_work *work)
{
	struct bmi260_data *data = CONTAINER_OF(work, struct bmi260_data, trig_work);

	bmi260_handle_interrupt(data->dev);
}
#endif

/* Routes data ready to INT1 and makes the pin drive it. */
static int bmi260_enable_drdy_interrupt(const struct device *dev, bool enable)
{
	const struct bmi260_config *cfg = dev->config;
	uint8_t io_ctrl = BMI260_INT_IO_CTRL_OUTPUT_EN;
	uint8_t map = enable ? BMI260_INT_MAP_DATA_DRDY_INT1 : 0U;
	int ret;

	if ((cfg->int1.dt_flags & GPIO_ACTIVE_LOW) == 0U) {
		io_ctrl |= BMI260_INT_IO_CTRL_LVL;
	}

	ret = bmi260_reg_write(dev, BMI260_REG_INT1_IO_CTRL, &io_ctrl, sizeof(io_ctrl));
	if (ret != 0) {
		return ret;
	}

	return bmi260_reg_write(dev, BMI260_REG_INT_MAP_DATA, &map, sizeof(map));
}

int bmi260_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
		       sensor_trigger_handler_t handler)
{
	const struct bmi260_config *cfg = dev->config;
	struct bmi260_data *data = dev->data;
	int ret;

	/* A board that does not route the pin can still poll the device. */
	if (cfg->int1.port == NULL) {
		return -ENOTSUP;
	}

	if (trig->type != SENSOR_TRIG_DATA_READY) {
		return -ENOTSUP;
	}

	/*
	 * Stop the pin before changing the handler, so an edge arriving midway
	 * cannot find a half-updated pair.
	 */
	ret = gpio_pin_interrupt_configure_dt(&cfg->int1, GPIO_INT_DISABLE);
	if (ret != 0) {
		return ret;
	}

	data->drdy_handler = handler;
	data->drdy_trigger = trig;

	ret = bmi260_enable_drdy_interrupt(dev, handler != NULL);
	if (ret != 0) {
		return ret;
	}

	if (handler == NULL) {
		return 0;
	}

	return gpio_pin_interrupt_configure_dt(&cfg->int1, GPIO_INT_EDGE_TO_ACTIVE);
}

int bmi260_trigger_mode_init(const struct device *dev)
{
	const struct bmi260_config *cfg = dev->config;
	struct bmi260_data *data = dev->data;
	int ret;

	data->dev = dev;

	/* Nothing to set up for a board that leaves the pin unrouted. */
	if (cfg->int1.port == NULL) {
		return 0;
	}

	if (!gpio_is_ready_dt(&cfg->int1)) {
		LOG_ERR("The interrupt line is not ready");
		return -ENODEV;
	}

#if defined(CONFIG_BMI260_TRIGGER_OWN_THREAD)
	k_sem_init(&data->trig_sem, 0, 1);

	k_thread_create(&data->thread, data->thread_stack, CONFIG_BMI260_THREAD_STACK_SIZE,
			bmi260_thread_main, data, NULL, NULL,
			K_PRIO_COOP(CONFIG_BMI260_THREAD_PRIORITY), 0, K_NO_WAIT);
	k_thread_name_set(&data->thread, "bmi260_trigger");
#elif defined(CONFIG_BMI260_TRIGGER_GLOBAL_THREAD)
	k_work_init(&data->trig_work, bmi260_work_handler);
#endif

	/*
	 * The pin is prepared and the callback registered before the device is
	 * told to drive it, so a line that is already asserted cannot deliver an
	 * edge into a half-built handler.
	 */
	ret = gpio_pin_configure_dt(&cfg->int1, GPIO_INPUT);
	if (ret != 0) {
		return ret;
	}

	gpio_init_callback(&data->int1_cb, bmi260_gpio_callback, BIT(cfg->int1.pin));

	ret = gpio_add_callback(cfg->int1.port, &data->int1_cb);
	if (ret != 0) {
		return ret;
	}

	/*
	 * The interrupt stays masked and unmapped until an application asks for
	 * a trigger, so a device nobody is listening to raises nothing.
	 */
	return bmi260_enable_drdy_interrupt(dev, false);
}
