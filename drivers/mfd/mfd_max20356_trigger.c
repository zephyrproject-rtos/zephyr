/*
 * Copyright (c) 2026 Analog Devices, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/mfd/max20356.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include "mfd_max20356.h"

LOG_MODULE_REGISTER(mfd_max20356_trig, CONFIG_MFD_LOG_LEVEL);

#define MAX20356_INT_REG_COUNT 6U

struct max20356_int_source {
	uint8_t reg_index;
	uint8_t mask;
	enum max20356_event evt;
};

/* Combined INTB source table: every Int0-5 status bit and the event group that consumes it. */
static const struct max20356_int_source max20356_int_sources[] = {
	{0, MAX20356_INT0_CHGSTATINT_MSK, MAX20356_EVT_CHARGER},
	{0, MAX20356_INT0_CC1TMOINT_MSK, MAX20356_EVT_CHARGER},
	{0, MAX20356_INT0_THMSTATINT_MSK, MAX20356_EVT_THERMAL},
	{1, MAX20356_INT1_THMSDINT_MSK, MAX20356_EVT_THERMAL},
	{1, MAX20356_INT1_CHGJEITAREGINT_MSK, MAX20356_EVT_CHARGER},
	{1, MAX20356_INT1_CHGJEITASDINT_MSK, MAX20356_EVT_CHARGER},
	{1, MAX20356_INT1_USBOKINT_MSK, MAX20356_EVT_USB},
	{1, MAX20356_INT1_USBOVPINT_MSK, MAX20356_EVT_USB},
	{1, MAX20356_INT1_ILIMINT_MSK, MAX20356_EVT_MISC},
	{1, MAX20356_INT1_CHGSYSLIMINT_MSK, MAX20356_EVT_MISC},
	{1, MAX20356_INT1_SYSBATLIMINT_MSK, MAX20356_EVT_MISC},
	{2, MAX20356_INT2_THMLDO1INT_MSK, MAX20356_EVT_THERMAL},
	{2, MAX20356_INT2_UVLOLDO1INT_MSK, MAX20356_EVT_REG_FAULT},
	{2, MAX20356_INT2_THMLDO2INT_MSK, MAX20356_EVT_THERMAL},
	{2, MAX20356_INT2_UVLOLDO2INT_MSK, MAX20356_EVT_REG_FAULT},
	{2, MAX20356_INT2_THMLDO3INT_MSK, MAX20356_EVT_THERMAL},
	{2, MAX20356_INT2_UVLOLDO3INT_MSK, MAX20356_EVT_REG_FAULT},
	{2, MAX20356_INT2_SCLDO3INT_MSK, MAX20356_EVT_REG_FAULT},
	{2, MAX20356_INT2_DRPLDO3INT_MSK, MAX20356_EVT_REG_FAULT},
	{3, MAX20356_INT3_THMBK1INT_MSK, MAX20356_EVT_THERMAL},
	{3, MAX20356_INT3_THMBK2INT_MSK, MAX20356_EVT_THERMAL},
	{3, MAX20356_INT3_THMBK3INT_MSK, MAX20356_EVT_THERMAL},
	{3, MAX20356_INT3_LSW1TMOINT_MSK, MAX20356_EVT_REG_FAULT},
	{3, MAX20356_INT3_LSW2TMOINT_MSK, MAX20356_EVT_REG_FAULT},
	{3, MAX20356_INT3_LSW3TMOINT_MSK, MAX20356_EVT_REG_FAULT},
	{3, MAX20356_INT3_THMLSWINT_MSK, MAX20356_EVT_THERMAL},
	{3, MAX20356_INT3_BBSTFAULTINT_MSK, MAX20356_EVT_REG_FAULT},
	{4, MAX20356_INT4_HRVBATCMPINT_MSK, MAX20356_EVT_MISC},
	{4, MAX20356_INT4_CHGRESTARTINT_MSK, MAX20356_EVT_CHARGER},
	{4, MAX20356_INT4_CHGVOLTMODEINT_MSK, MAX20356_EVT_CHARGER},
	{4, MAX20356_INT4_STEPCHGINT_MSK, MAX20356_EVT_MISC},
	{4, MAX20356_INT4_BATUVLOBINT_MSK, MAX20356_EVT_MISC},
	{5, MAX20356_INT5_I2CTMOINT_MSK, MAX20356_EVT_MISC},
	{5, MAX20356_INT5_WDTMR_MSK, MAX20356_EVT_WATCHDOG},
};

/* Event group dispatched for each MPC-routed interrupt source (indexed by
 * enum max20356_mpc_int_source): the three buck power-good lines share the
 * DVS/PGOOD transition-complete group, USBOK feeds the USB group.
 */
static const enum max20356_event max20356_mpc_int_evt[MAX20356_MPC_INT_COUNT] = {
	[MAX20356_MPC_INT_BUCK1_PGOOD] = MAX20356_EVT_DVS_DONE,
	[MAX20356_MPC_INT_BUCK2_PGOOD] = MAX20356_EVT_DVS_DONE,
	[MAX20356_MPC_INT_BUCK3_PGOOD] = MAX20356_EVT_DVS_DONE,
	[MAX20356_MPC_INT_USBOK] = MAX20356_EVT_USB,
};

/* Invoke the registered callback for one event group, if any. */
static void max20356_dispatch(const struct device *dev, enum max20356_event evt)
{
	struct mfd_max20356_data *data = dev->data;

	k_mutex_lock(&data->cb_lock, K_FOREVER);
	if (data->cb[evt] != NULL) {
		data->cb[evt](dev, evt, data->cb_user[evt]);
	}
	k_mutex_unlock(&data->cb_lock);
}

/* Set (unmask) or clear (mask) every IntMask bit backing an event group. */
static int max20356_set_group_mask(const struct device *dev, enum max20356_event evt, bool unmask)
{
	for (size_t i = 0; i < ARRAY_SIZE(max20356_int_sources); i++) {
		const struct max20356_int_source *src = &max20356_int_sources[i];
		int ret;

		if (src->evt != evt) {
			continue;
		}

		ret = mfd_max20356_reg_update(dev, MAX20356_REG_INTMASK0 + src->reg_index,
					      src->mask, unmask ? src->mask : 0U);
		if (ret < 0) {
			return ret;
		}
	}

	return 0;
}

/* process each interrupt enabled and call its respective callback function */
static void max20356_process_int(const struct device *dev)
{
	struct mfd_max20356_data *data = dev->data;
	const struct mfd_max20356_config *config = dev->config;
	uint8_t status[MAX20356_INT_REG_COUNT] = {0};
	uint16_t fired = 0;
	uint8_t first = 0;
	int ret;

	/* read only INT5 if watchdog is active */
	if (data->wdt_active) {
		first = 5U;
	}

	for (uint8_t i = first; i < MAX20356_INT_REG_COUNT; i++) {
		ret = mfd_max20356_reg_read(dev, MAX20356_REG_INT0 + i, &status[i]);
		if (ret < 0) {
			LOG_ERR("Int%u read failed: %d", i, ret);
			goto rearm;
		}
	}

	for (size_t i = 0; i < ARRAY_SIZE(max20356_int_sources); i++) {
		const struct max20356_int_source *src = &max20356_int_sources[i];

		if ((status[src->reg_index] & src->mask) != 0U) {
			fired |= BIT(src->evt);
		}
	}

	for (uint8_t evt = 0; evt < MAX20356_EVT_MAX; evt++) {
		if ((fired & BIT(evt)) != 0U) {
			max20356_dispatch(dev, (enum max20356_event)evt);
		}
	}

rearm:
	ret = gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to re-arm INTB: %d", ret);
	}
}

static void max20356_gpio_callback(const struct device *port, struct gpio_callback *cb,
				   gpio_port_pins_t pins)
{
	struct mfd_max20356_data *data = CONTAINER_OF(cb, struct mfd_max20356_data, gpio_cb);
	const struct mfd_max20356_config *config = data->dev->config;

	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	(void)gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_DISABLE);

#if defined(CONFIG_MAX20356_TRIGGER_OWN_THREAD)
	k_sem_give(&data->sem);
#else
	k_work_submit(&data->work);
#endif
}

#if defined(CONFIG_MAX20356_TRIGGER_OWN_THREAD)
static void max20356_thread(void *p1, void *p2, void *p3)
{
	struct mfd_max20356_data *data = p1;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	while (true) {
		k_sem_take(&data->sem, K_FOREVER);
		max20356_process_int(data->dev);
	}
}
#else
static void max20356_work_handler(struct k_work *work)
{
	struct mfd_max20356_data *data = CONTAINER_OF(work, struct mfd_max20356_data, work);

	max20356_process_int(data->dev);
}
#endif

/* Shared GPIO callback for every MPC-routed interrupt line: disable this line's
 * interrupt and hand off to its work item; the embedded gpio_callback identifies
 * which line via CONTAINER_OF.
 */
static void max20356_mpc_int_callback(const struct device *port, struct gpio_callback *cb,
				      gpio_port_pins_t pins)
{
	struct max20356_mpc_int_ctx *ctx = CONTAINER_OF(cb, struct max20356_mpc_int_ctx, cb);
	const struct mfd_max20356_config *config = ctx->dev->config;

	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	(void)gpio_pin_interrupt_configure_dt(&config->mpc_int_gpio[ctx->src], GPIO_INT_DISABLE);

	k_work_submit(&ctx->work);
}

/* Work handler for an MPC-routed interrupt: dispatch its event group and re-arm
 * the line, mirroring the INTB disable-in-ISR / re-arm-in-worker pattern.
 */
static void max20356_mpc_int_work(struct k_work *work)
{
	struct max20356_mpc_int_ctx *ctx = CONTAINER_OF(work, struct max20356_mpc_int_ctx, work);
	const struct mfd_max20356_config *config = ctx->dev->config;
	int ret;

	max20356_dispatch(ctx->dev, max20356_mpc_int_evt[ctx->src]);

	ret = gpio_pin_interrupt_configure_dt(&config->mpc_int_gpio[ctx->src],
					      GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to re-arm MPC int %u: %d", ctx->src, ret);
	}
}

/* Configure the optional host GPIO watching one MPC-routed interrupt line. A
 * line with no port in devicetree is skipped.
 */
static int max20356_mpc_int_init(const struct device *dev, uint8_t src)
{
	struct mfd_max20356_data *data = dev->data;
	const struct mfd_max20356_config *config = dev->config;
	const struct gpio_dt_spec *gpio = &config->mpc_int_gpio[src];
	struct max20356_mpc_int_ctx *ctx = &data->mpc_int[src];
	int ret;

	if (gpio->port == NULL) {
		return 0;
	}

	if (!gpio_is_ready_dt(gpio)) {
		LOG_ERR("MPC int %u GPIO not ready", src);
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(gpio, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("MPC int %u GPIO configure failed: %d", src, ret);
		return ret;
	}

	ctx->dev = dev;
	ctx->src = src;
	k_work_init(&ctx->work, max20356_mpc_int_work);

	gpio_init_callback(&ctx->cb, max20356_mpc_int_callback, BIT(gpio->pin));

	ret = gpio_add_callback_dt(gpio, &ctx->cb);
	if (ret < 0) {
		LOG_ERR("Failed to add MPC int %u callback: %d", src, ret);
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to arm MPC int %u: %d", src, ret);
		return ret;
	}

	return 0;
}

int mfd_max20356_add_callback(const struct device *dev, enum max20356_event evt, max20356_cb_t cb,
			      void *user)
{
	struct mfd_max20356_data *data = dev->data;
	int ret;

	if (evt >= MAX20356_EVT_MAX || cb == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&data->cb_lock, K_FOREVER);
	/* refuse registration of new callbacks while watchdog is active */
	if (data->wdt_active) {
		k_mutex_unlock(&data->cb_lock);
		return -EBUSY;
	}
	data->cb[evt] = cb;
	data->cb_user[evt] = user;
	ret = max20356_set_group_mask(dev, evt, true);
	k_mutex_unlock(&data->cb_lock);

	return ret;
}

int mfd_max20356_remove_callback(const struct device *dev, enum max20356_event evt,
				 max20356_cb_t cb)
{
	struct mfd_max20356_data *data = dev->data;
	int ret;

	if (evt >= MAX20356_EVT_MAX) {
		return -EINVAL;
	}

	k_mutex_lock(&data->cb_lock, K_FOREVER);
	if (data->cb[evt] != cb) {
		k_mutex_unlock(&data->cb_lock);
		return -EINVAL;
	}
	data->cb[evt] = NULL;
	data->cb_user[evt] = NULL;
	ret = max20356_set_group_mask(dev, evt, false);
	k_mutex_unlock(&data->cb_lock);

	return ret;
}

int mfd_max20356_trigger_init(const struct device *dev)
{
	struct mfd_max20356_data *data = dev->data;
	const struct mfd_max20356_config *config = dev->config;
	uint8_t scratch = 0;
	int ret;

	data->dev = dev;
	k_mutex_init(&data->cb_lock);

#if defined(CONFIG_MAX20356_TRIGGER_OWN_THREAD)
	k_sem_init(&data->sem, 0, K_SEM_MAX_LIMIT);
	k_thread_create(&data->thread, data->thread_stack,
			K_THREAD_STACK_SIZEOF(data->thread_stack), max20356_thread, data, NULL,
			NULL, K_PRIO_COOP(CONFIG_MAX20356_THREAD_PRIORITY), 0, K_NO_WAIT);
	k_thread_name_set(&data->thread, "max20356_trig");
#else
	k_work_init(&data->work, max20356_work_handler);
#endif

	/* MPC-routed interrupt lines are independent of INTB: set them up first so
	 * they work even on a poll-only device with no int-gpios.
	 */
	for (uint8_t src = 0; src < MAX20356_MPC_INT_COUNT; src++) {
		ret = max20356_mpc_int_init(dev, src);
		if (ret < 0) {
			return ret;
		}
	}

	/* int-gpios is optional: without it the driver operates poll-only. */
	if (config->int_gpio.port == NULL) {
		LOG_INF("No int-gpios; INTB trigger disabled (poll-only)");
		return 0;
	}

	if (!gpio_is_ready_dt(&config->int_gpio)) {
		LOG_ERR("INTB GPIO not ready");
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(&config->int_gpio, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("INTB GPIO configure failed: %d", ret);
		return ret;
	}

	gpio_init_callback(&data->gpio_cb, max20356_gpio_callback, BIT(config->int_gpio.pin));

	ret = gpio_add_callback_dt(&config->int_gpio, &data->gpio_cb);
	if (ret < 0) {
		LOG_ERR("Failed to add INTB callback: %d", ret);
		return ret;
	}

	/* clear int status of int0-5 while interrupt is not configured */
	for (uint8_t i = 0; i < MAX20356_INT_REG_COUNT; i++) {
		(void)mfd_max20356_reg_read(dev, MAX20356_REG_INT0 + i, &scratch);
	}

	ret = gpio_pin_interrupt_configure_dt(&config->int_gpio, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret < 0) {
		LOG_ERR("Failed to arm INTB: %d", ret);
		return ret;
	}

	return 0;
}
