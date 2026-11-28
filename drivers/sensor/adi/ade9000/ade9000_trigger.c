/*
 * Copyright (c) 2026 A-Eberle <Daniel.Kampert@a-eberle.de>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief IRQ0 and IRQ1 handling of the ADE9000 driver.
 */

#define DT_DRV_COMPAT adi_ade9000

#include "ade9000.h"
#include "ade9000_spi.h"

/**
 * @brief          Read and clear a status register and hand the trigger to the
 *                 application.
 * @param dev      ADE9000 device
 * @param reg      ADE9000_REG_STATUS0 or ADE9000_REG_STATUS1
 * @param status   Takes the status for SENSOR_ATTR_ADE9000_STATUSx
 * @param handler  Trigger handler of the application, may be NULL
 * @param trig     Trigger the handler was installed for
 */
static void ade9000_irq_service(const struct device *dev, uint16_t reg, uint32_t *status,
				sensor_trigger_handler_t handler, const struct sensor_trigger *trig)
{
	if (ade9000_status_update(dev, reg, status) != 0) {
		return;
	}

	if (handler != NULL) {
		handler(dev, trig);
	}
}

/**
 * @brief          Mark IRQ lines as pending and wake the thread or work item
 *                 that services them. Safe to call from an ISR.
 * @param data     Device data
 * @param pending  ADE9000_PENDING_IRQ0 and/or ADE9000_PENDING_IRQ1
 */
static void ade9000_irq_raise(struct ade9000_data *data, atomic_val_t pending)
{
	atomic_or(&data->irq_pending, pending);

#if defined(CONFIG_ADE9000_TRIGGER_OWN_THREAD)
	k_sem_give(&data->irq_sem);
#elif defined(CONFIG_ADE9000_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->irq_work);
#endif
}

/**
 * @brief       Service every pending IRQ line. Runs in thread context, since
 *              SPI must not be used from the GPIO interrupt.
 * @param data  Device data
 */
static void ade9000_irq_process(struct ade9000_data *data)
{
	const struct device *dev = data->dev;
	const struct ade9000_config *cfg = dev->config;
	atomic_val_t pending = atomic_clear(&data->irq_pending);

	if (pending & ADE9000_PENDING_IRQ0) {
		ade9000_irq_service(dev, ADE9000_REG_STATUS0, &data->status0, data->irq0_handler,
				    data->irq0_trig);

		/* An event between reading and clearing keeps the line low without a new edge */
		if (gpio_pin_get_dt(&cfg->irq0) > 0) {
			ade9000_irq_raise(data, ADE9000_PENDING_IRQ0);
		}
	}

	if (pending & ADE9000_PENDING_IRQ1) {
		/* With IRQ0_ON_IRQ1 the STATUS0 events pull IRQ1 low as well */
		if (cfg->irq0_on_irq1) {
			ade9000_irq_service(dev, ADE9000_REG_STATUS0, &data->status0,
					    data->irq0_handler, data->irq0_trig);
		}

		ade9000_irq_service(dev, ADE9000_REG_STATUS1, &data->status1, data->irq1_handler,
				    data->irq1_trig);

		if (gpio_pin_get_dt(&cfg->irq1) > 0) {
			ade9000_irq_raise(data, ADE9000_PENDING_IRQ1);
		}
	}
}

/**
 * @brief        GPIO callback of the IRQ0 line, runs in interrupt context.
 * @param port   GPIO port of the line, unused
 * @param cb     Callback storage, embedded in the device data
 * @param pins   Pins that fired, unused
 */
static void ade9000_irq0_callback(const struct device *port, struct gpio_callback *cb,
				  uint32_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	ade9000_irq_raise(CONTAINER_OF(cb, struct ade9000_data, irq0_cb), ADE9000_PENDING_IRQ0);
}

/**
 * @brief        GPIO callback of the IRQ1 line, runs in interrupt context.
 * @param port   GPIO port of the line, unused
 * @param cb     Callback storage, embedded in the device data
 * @param pins   Pins that fired, unused
 */
static void ade9000_irq1_callback(const struct device *port, struct gpio_callback *cb,
				  uint32_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	ade9000_irq_raise(CONTAINER_OF(cb, struct ade9000_data, irq1_cb), ADE9000_PENDING_IRQ1);
}

#if defined(CONFIG_ADE9000_TRIGGER_OWN_THREAD)
/**
 * @brief      Own interrupt thread, services the IRQ lines whenever the
 *             semaphore is given.
 * @param p1   Device data
 * @param p2   Unused
 * @param p3   Unused
 */
static void ade9000_thread(void *p1, void *p2, void *p3)
{
	struct ade9000_data *data = p1;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (true) {
		k_sem_take(&data->irq_sem, K_FOREVER);
		ade9000_irq_process(data);
	}
}
#elif defined(CONFIG_ADE9000_TRIGGER_GLOBAL_THREAD)
/**
 * @brief        Work item on the system work queue, services the IRQ lines.
 * @param work   Work item, embedded in the device data
 */
static void ade9000_work_handler(struct k_work *work)
{
	ade9000_irq_process(CONTAINER_OF(work, struct ade9000_data, irq_work));
}
#endif

/**
 * @brief           Configure one IRQ line and attach its callback. A line that
 *                  is not in the devicetree is skipped.
 * @param gpio      IRQ line
 * @param cb        Callback storage in the device data
 * @param handler   GPIO callback for this line
 * @return          0 on success, negative errno otherwise
 */
static int ade9000_irq_line_init(const struct gpio_dt_spec *gpio, struct gpio_callback *cb,
				 gpio_callback_handler_t handler)
{
	int ret;

	if (gpio->port == NULL) {
		return 0;
	}

	if (gpio_is_ready_dt(gpio) == false) {
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(gpio, GPIO_INPUT);
	if (ret) {
		return ret;
	}

	gpio_init_callback(cb, handler, BIT(gpio->pin));

	ret = gpio_add_callback_dt(gpio, cb);
	if (ret) {
		return ret;
	}

	return gpio_pin_interrupt_configure_dt(gpio, GPIO_INT_EDGE_TO_ACTIVE);
}

int ade9000_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
			sensor_trigger_handler_t handler)
{
	const struct ade9000_config *cfg = dev->config;
	struct ade9000_data *data = dev->data;

	switch ((int)trig->type) {
	case SENSOR_TRIG_ADE9000_IRQ0: {
		if ((cfg->irq0.port == NULL) &&
		    ((cfg->irq0_on_irq1 == false) || (cfg->irq1.port == NULL))) {
			return -ENOTSUP;
		}

		data->irq0_trig = trig;
		data->irq0_handler = handler;

		return 0;
	}
	case SENSOR_TRIG_ADE9000_IRQ1: {
		if (cfg->irq1.port == NULL) {
			return -ENOTSUP;
		}

		data->irq1_trig = trig;
		data->irq1_handler = handler;

		return 0;
	}
	default: {
		return -ENOTSUP;
	}
	}
}

int ade9000_init_interrupt(const struct device *dev)
{
	const struct ade9000_config *cfg = dev->config;
	struct ade9000_data *data = dev->data;
	int ret;

	data->dev = dev;

#if defined(CONFIG_ADE9000_TRIGGER_OWN_THREAD)
	k_sem_init(&data->irq_sem, 0, K_SEM_MAX_LIMIT);
	k_thread_create(&data->thread, data->thread_stack,
			K_KERNEL_STACK_SIZEOF(data->thread_stack), ade9000_thread, data, NULL, NULL,
			K_PRIO_COOP(CONFIG_ADE9000_THREAD_PRIORITY), 0, K_NO_WAIT);
	k_thread_name_set(&data->thread, dev->name);
#elif defined(CONFIG_ADE9000_TRIGGER_GLOBAL_THREAD)
	k_work_init(&data->irq_work, ade9000_work_handler);
#endif

	ret = ade9000_irq_line_init(&cfg->irq0, &data->irq0_cb, ade9000_irq0_callback);
	if (ret) {
		return ret;
	}

	ret = ade9000_irq_line_init(&cfg->irq1, &data->irq1_cb, ade9000_irq1_callback);
	if (ret) {
		return ret;
	}

	/* An edge interrupt misses a line that is already low, so service it once */
	if ((cfg->irq0.port != NULL) && (gpio_pin_get_dt(&cfg->irq0) > 0)) {
		ade9000_irq_raise(data, ADE9000_PENDING_IRQ0);
	}

	if ((cfg->irq1.port != NULL) && (gpio_pin_get_dt(&cfg->irq1) > 0)) {
		ade9000_irq_raise(data, ADE9000_PENDING_IRQ1);
	}

	return 0;
}
