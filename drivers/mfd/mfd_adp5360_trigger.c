/*
 * Copyright (c) 2026 Analog Devices Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr/drivers/mfd/adp5360.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include "mfd_adp5360.h"

LOG_MODULE_REGISTER(mfd_adp5360_trig, CONFIG_MFD_LOG_LEVEL);

#define ADP5360_MFD_REG_FAULT_STATUS       0x2Eu
#define ADP5360_MFD_REG_PGOOD1_MASK        0x30u
#define ADP5360_MFD_REG_PGOOD2_MASK        0x31u
#define ADP5360_MFD_REG_INTERRUPT_ENABLE_1 0x32u
#define ADP5360_MFD_REG_INTERRUPT_ENABLE_2 0x33u

#define ADP5360_STATUS_SOC_LOW_INT_MASK                BIT(7)
#define ADP5360_STATUS_SOC_ACM_INT_MASK                BIT(6)
#define ADP5360_STATUS_ADPICHG_INT_MASK                BIT(5)
#define ADP5360_STATUS_BAT_PROTECTION_INT_MASK         BIT(4)
#define ADP5360_STATUS_TEMP_THRESHOLD_INT_MASK         BIT(3)
#define ADP5360_STATUS_BAT_VOLTAGE_THRESHOLD_INT_MASK  BIT(2)
#define ADP5360_STATUS_CHARGER_MODE_CHANGE_INT_MASK    BIT(1)
#define ADP5360_STATUS_VBUS_VOLTAGE_THRESHOLD_INT_MASK BIT(0)

#define ADP5360_STATUS_WATCHDOG_TIMEOUT_INT_MASK BIT(6)
#define ADP5360_STATUS_BUCKPGOOD_INT_MASK        BIT(5)
#define ADP5360_STATUS_BUCKBSTPGOOD_INT_MASK     BIT(4)

#define ADP5360_PGOOD1_REV_MASK             BIT(7)
#define ADP5360_PGOOD1_CHARGE_COMPLETE_MASK BIT(4)
#define ADP5360_PGOOD1_VBUS_OK_MASK         BIT(3)
#define ADP5360_PGOOD1_BATTERY_OK_MASK      BIT(2)
#define ADP5360_PGOOD1_VOUT2_OK_MASK        BIT(1)
#define ADP5360_PGOOD1_VOUT1_OK_MASK        BIT(0)

#define ADP5360_PGOOD2_REV_MASK             BIT(7)
#define ADP5360_PGOOD2_CHARGE_COMPLETE_MASK BIT(4)
#define ADP5360_PGOOD2_VBUS_OK_MASK         BIT(3)
#define ADP5360_PGOOD2_BATTERY_OK_MASK      BIT(2)
#define ADP5360_PGOOD2_VOUT2_OK_MASK        BIT(1)
#define ADP5360_PGOOD2_VOUT1_OK_MASK        BIT(0)

#define ADP5360_MFD_INTERRUPT_MASK_PG2_SHIFT 0x4u

/* Helper function to call a trigger handler */
static void mfd_adp5360_call_status_handler(const struct device *dev,
					    struct child_interrupt_callback cb, uint8_t status,
					    uint8_t mask)
{
	if ((status & mask) && cb.handler) {
		cb.handler(dev, cb.user_data);
	}
}

int mfd_adp5360_interrupt_trigger_set(const struct device *dev, enum adp5360_interrupt_type type,
				      mfd_adp5360_interrupt_callback handler, void *user_data)
{
	struct mfd_adp5360_data *data = dev->data;
	uint8_t mask;

	LOG_DBG("Setup Interrupt trigger");
	if (handler == NULL) {
		LOG_ERR("No handler set!");
		return -EINVAL;
	}

	switch (type) {
	case ADP5360_INTERRUPT_SOC_LOW:
		data->interrupt_handlers.soc_low_callback.handler = handler;
		data->interrupt_handlers.soc_low_callback.user_data = user_data;
		mask = ADP5360_STATUS_SOC_LOW_INT_MASK;
		break;
	case ADP5360_INTERRUPT_SOC_ACM:
		data->interrupt_handlers.soc_acm_callback.handler = handler;
		data->interrupt_handlers.soc_acm_callback.user_data = user_data;
		mask = ADP5360_STATUS_SOC_ACM_INT_MASK;
		break;
	case ADP5360_INTERRUPT_ADPICHG:
		data->interrupt_handlers.adpichg_callback.handler = handler;
		data->interrupt_handlers.adpichg_callback.user_data = user_data;
		mask = ADP5360_STATUS_ADPICHG_INT_MASK;
		break;
	case ADP5360_INTERRUPT_BAT_PROTECTION:
		data->interrupt_handlers.bat_protection_callback.handler = handler;
		data->interrupt_handlers.bat_protection_callback.user_data = user_data;
		mask = ADP5360_STATUS_BAT_PROTECTION_INT_MASK;
		break;
	case ADP5360_INTERRUPT_TEMP_THRESHOLD:
		data->interrupt_handlers.temp_threshold_callback.handler = handler;
		data->interrupt_handlers.temp_threshold_callback.user_data = user_data;
		mask = ADP5360_STATUS_TEMP_THRESHOLD_INT_MASK;
		break;
	case ADP5360_INTERRUPT_BAT_VOLTAGE_THRESHOLD:
		data->interrupt_handlers.bat_voltage_threshold_callback.handler = handler;
		data->interrupt_handlers.bat_voltage_threshold_callback.user_data = user_data;
		mask = ADP5360_STATUS_BAT_VOLTAGE_THRESHOLD_INT_MASK;
		break;
	case ADP5360_INTERRUPT_CHARGER_MODE_CHANGE:
		data->interrupt_handlers.charger_mode_change_callback.handler = handler;
		data->interrupt_handlers.charger_mode_change_callback.user_data = user_data;
		mask = ADP5360_STATUS_CHARGER_MODE_CHANGE_INT_MASK;
		break;
	case ADP5360_INTERRUPT_VBUS_VOLTAGE_THRESHOLD:
		data->interrupt_handlers.vbus_voltage_threshold_callback.handler = handler;
		data->interrupt_handlers.vbus_voltage_threshold_callback.user_data = user_data;
		mask = ADP5360_STATUS_VBUS_VOLTAGE_THRESHOLD_INT_MASK;
		break;
	case ADP5360_INTERRUPT_MANUAL_RESET:
		data->interrupt_handlers.manual_reset_callback.handler = handler;
		data->interrupt_handlers.manual_reset_callback.user_data = user_data;
		mask = ADP5360_STATUS_MANUAL_RESET_INT_MASK;
		break;
	case ADP5360_INTERRUPT_WATCHDOG_TIMEOUT:
		data->interrupt_handlers.watchdog_timeout_callback.handler = handler;
		data->interrupt_handlers.watchdog_timeout_callback.user_data = user_data;
		mask = ADP5360_STATUS_WATCHDOG_TIMEOUT_INT_MASK;
		break;
	case ADP5360_INTERRUPT_BUCKPGOOD:
		data->interrupt_handlers.buckpgood_callback.handler = handler;
		data->interrupt_handlers.buckpgood_callback.user_data = user_data;
		mask = ADP5360_STATUS_BUCKPGOOD_INT_MASK;
		break;
	case ADP5360_INTERRUPT_BUCKBSTPGOOD:
		data->interrupt_handlers.buckbstpgood_callback.handler = handler;
		data->interrupt_handlers.buckbstpgood_callback.user_data = user_data;
		mask = ADP5360_STATUS_BUCKBSTPGOOD_INT_MASK;
		break;
	default:
		return -EINVAL;
	}

	if (type >= ADP5360_INTERRUPT_BUCKBSTPGOOD) {
		return mfd_adp5360_reg_update(dev, ADP5360_MFD_REG_INTERRUPT_ENABLE_2, mask,
					      handler ? 1 : 0);
	} else {
		return mfd_adp5360_reg_update(dev, ADP5360_MFD_REG_INTERRUPT_ENABLE_1, mask,
					      handler ? 1 : 0);
	}
}

static int mfd_adp5360_interrupt_callback_init(const struct device *dev)
{
	struct mfd_adp5360_data *data = dev->data;

	/* Initialize all interrupt handlers to NULL */
	data->interrupt_handlers.soc_low_callback.handler = NULL;
	data->interrupt_handlers.soc_low_callback.user_data = NULL;
	data->interrupt_handlers.soc_acm_callback.handler = NULL;
	data->interrupt_handlers.soc_acm_callback.user_data = NULL;
	data->interrupt_handlers.adpichg_callback.handler = NULL;
	data->interrupt_handlers.adpichg_callback.user_data = NULL;
	data->interrupt_handlers.bat_protection_callback.handler = NULL;
	data->interrupt_handlers.bat_protection_callback.user_data = NULL;
	data->interrupt_handlers.temp_threshold_callback.handler = NULL;
	data->interrupt_handlers.temp_threshold_callback.user_data = NULL;
	data->interrupt_handlers.bat_voltage_threshold_callback.handler = NULL;
	data->interrupt_handlers.bat_voltage_threshold_callback.user_data = NULL;
	data->interrupt_handlers.charger_mode_change_callback.handler = NULL;
	data->interrupt_handlers.charger_mode_change_callback.user_data = NULL;
	data->interrupt_handlers.vbus_voltage_threshold_callback.handler = NULL;
	data->interrupt_handlers.vbus_voltage_threshold_callback.user_data = NULL;
	data->interrupt_handlers.manual_reset_callback.handler = NULL;
	data->interrupt_handlers.manual_reset_callback.user_data = NULL;
	data->interrupt_handlers.watchdog_timeout_callback.handler = NULL;
	data->interrupt_handlers.watchdog_timeout_callback.user_data = NULL;
	data->interrupt_handlers.buckpgood_callback.handler = NULL;
	data->interrupt_handlers.buckpgood_callback.user_data = NULL;
	data->interrupt_handlers.buckbstpgood_callback.handler = NULL;
	data->interrupt_handlers.buckbstpgood_callback.user_data = NULL;

	return 0;
}

static void mfd_adp5360_interrupt_main_cb(const struct device *dev)
{
	const struct mfd_adp5360_config *config = dev->config;
	struct mfd_adp5360_data *data = dev->data;

	int ret = 0;
	uint8_t int_status[2];
	/* Read interrupt status registers and call corresponding handlers */

	ret = mfd_adp5360_reg_burst_read(dev, ADP5360_MFD_REG_INT_STATUS1, int_status,
					 ARRAY_SIZE(int_status));
	if (ret < 0) {
		LOG_ERR("Failed to read interrupt status registers");
		return;
	}

	mfd_adp5360_call_status_handler(dev, data->interrupt_handlers.soc_low_callback,
					int_status[0], ADP5360_STATUS_SOC_LOW_INT_MASK);

	mfd_adp5360_call_status_handler(dev, data->interrupt_handlers.soc_acm_callback,
					int_status[0], ADP5360_STATUS_SOC_ACM_INT_MASK);

	mfd_adp5360_call_status_handler(dev, data->interrupt_handlers.adpichg_callback,
					int_status[0], ADP5360_STATUS_ADPICHG_INT_MASK);

	mfd_adp5360_call_status_handler(dev, data->interrupt_handlers.bat_protection_callback,
					int_status[0], ADP5360_STATUS_BAT_PROTECTION_INT_MASK);

	mfd_adp5360_call_status_handler(dev, data->interrupt_handlers.temp_threshold_callback,
					int_status[0], ADP5360_STATUS_TEMP_THRESHOLD_INT_MASK);

	mfd_adp5360_call_status_handler(
		dev, data->interrupt_handlers.bat_voltage_threshold_callback, int_status[0],
		ADP5360_STATUS_BAT_VOLTAGE_THRESHOLD_INT_MASK);

	mfd_adp5360_call_status_handler(dev, data->interrupt_handlers.charger_mode_change_callback,
					int_status[0], ADP5360_STATUS_CHARGER_MODE_CHANGE_INT_MASK);

	mfd_adp5360_call_status_handler(
		dev, data->interrupt_handlers.vbus_voltage_threshold_callback, int_status[0],
		ADP5360_STATUS_VBUS_VOLTAGE_THRESHOLD_INT_MASK);

	mfd_adp5360_call_status_handler(dev, data->interrupt_handlers.manual_reset_callback,
					int_status[1], ADP5360_STATUS_MANUAL_RESET_INT_MASK);

	mfd_adp5360_call_status_handler(dev, data->interrupt_handlers.watchdog_timeout_callback,
					int_status[1], ADP5360_STATUS_WATCHDOG_TIMEOUT_INT_MASK);

	mfd_adp5360_call_status_handler(dev, data->interrupt_handlers.buckpgood_callback,
					int_status[1], ADP5360_STATUS_BUCKPGOOD_INT_MASK);

	mfd_adp5360_call_status_handler(dev, data->interrupt_handlers.buckbstpgood_callback,
					int_status[1], ADP5360_STATUS_BUCKBSTPGOOD_INT_MASK);

	ret = gpio_pin_interrupt_configure_dt(&config->interrupt_gpio, GPIO_INT_EDGE_FALLING);
	if (ret < 0) {
		LOG_ERR("Failed to re-enable interrupt");
		return;
	}
	__ASSERT(ret == 0, "Interrupt Configuration Failed");
}

static void mfd_adp5360_interrupt_gpio_callback(const struct device *dev, struct gpio_callback *cb,
						uint32_t pins)
{
	struct mfd_adp5360_data *data = CONTAINER_OF(cb, struct mfd_adp5360_data, int_callback);
	const struct mfd_adp5360_config *config = data->dev->config;
	int ret = 0;

	/* Disable the interrupt to prevent multiple triggers */
	ret = gpio_pin_interrupt_configure_dt(&config->interrupt_gpio, GPIO_INT_DISABLE);
	if (ret < 0) {
		LOG_ERR("Failed to disable interrupt");
		return;
	}
#if defined(CONFIG_ADP5360_TRIGGER_OWN_THREAD)
	k_sem_give(&data->sem);
#elif defined(CONFIG_ADP5360_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->work);
#endif
}

#if defined(CONFIG_ADP5360_TRIGGER_OWN_THREAD)
static void mfd_adp5360_thread(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	struct mfd_adp5360_data *data = (struct mfd_adp5360_data *)arg1;

	while (1) {
		k_sem_take(&data->sem, K_FOREVER);
		mfd_adp5360_interrupt_main_cb(data->dev);
	}
}
#endif

#if defined(CONFIG_ADP5360_TRIGGER_GLOBAL_THREAD)
static void mfd_adp5360_work_cb(struct k_work *work)
{
	struct mfd_adp5360_data *data = CONTAINER_OF(work, struct mfd_adp5360_data, work);

	/* Handle the interrupt in workqueue context */
	mfd_adp5360_interrupt_main_cb(data->dev);
}
#endif

int mfd_adp5360_init_interrupt(const struct device *dev)
{
	const struct mfd_adp5360_config *config = dev->config;
	struct mfd_adp5360_data *data = dev->data;
	int ret;

	if (!gpio_is_ready_dt(&config->interrupt_gpio)) {
		LOG_ERR("Interrupt GPIO is not ready");
		return -ENODEV;
	}

	ret = mfd_adp5360_interrupt_callback_init(dev);
	if (ret < 0) {
		LOG_ERR("Failed to initialize interrupt callback");
		return ret;
	}

	data->dev = dev;

	/* Initialize semaphore/work BEFORE enabling interrupt to avoid race condition */
#if defined(CONFIG_ADP5360_TRIGGER_OWN_THREAD)
	k_sem_init(&data->sem, 0, K_SEM_MAX_LIMIT);
	k_thread_create(&data->thread, data->thread_stack,
			K_THREAD_STACK_SIZEOF(data->thread_stack),
			(k_thread_entry_t)mfd_adp5360_thread, data, NULL, NULL,
			K_PRIO_COOP(CONFIG_ADP5360_THREAD_PRIORITY), 0, K_NO_WAIT);
#elif defined(CONFIG_ADP5360_TRIGGER_GLOBAL_THREAD)
	k_work_init(&data->work, mfd_adp5360_work_cb);
#endif

	ret = gpio_pin_configure_dt(&config->interrupt_gpio, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Failed to configure interrupt GPIO pin");
		return ret;
	}

	/* Initialize GPIO callback */
	gpio_init_callback(&data->int_callback, mfd_adp5360_interrupt_gpio_callback,
			   BIT(config->interrupt_gpio.pin));
	ret = gpio_add_callback(config->interrupt_gpio.port, &data->int_callback);
	if (ret < 0) {
		LOG_ERR("Failed to add GPIO callback");
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&config->interrupt_gpio, GPIO_INT_EDGE_FALLING);
	if (ret < 0) {
		LOG_ERR("Failed to configure interrupt GPIO pin interrupt");
		return ret;
	}

	return 0;
}

int mfd_adp5360_pgood_trigger_set(const struct device *dev, enum pgood_pin pin,
				  enum adp5360_pgood_status_type type,
				  mfd_adp5360_interrupt_callback handler, void *user_data)
{
	struct mfd_adp5360_data *data = dev->data;
	uint8_t mask;

	LOG_DBG("Setup PGOOD trigger");
	if (handler == NULL) {
		LOG_ERR("No handler set!");
		return -EINVAL;
	}

	switch (type) {
	case ADP5360_PGOOD_STATUS_CHARGE_COMPLETE:
		mask = ADP5360_PGOOD1_CHARGE_COMPLETE_MASK;
		data->pgood_handlers.charge_complete_callback.handler = handler;
		data->pgood_handlers.charge_complete_callback.user_data = user_data;
		break;
	case ADP5360_PGOOD_STATUS_VBUS_OK:
		mask = ADP5360_PGOOD1_VBUS_OK_MASK;
		data->pgood_handlers.vbus_ok_callback.handler = handler;
		data->pgood_handlers.vbus_ok_callback.user_data = user_data;
		break;
	case ADP5360_PGOOD_STATUS_BATTERY_OK:
		mask = ADP5360_PGOOD1_BATTERY_OK_MASK;
		data->pgood_handlers.battery_ok_callback.handler = handler;
		data->pgood_handlers.battery_ok_callback.user_data = user_data;
		break;
	case ADP5360_PGOOD_STATUS_VOUT2_OK:
		mask = ADP5360_PGOOD1_VOUT2_OK_MASK;
		data->pgood_handlers.vout2_ok_callback.handler = handler;
		data->pgood_handlers.vout2_ok_callback.user_data = user_data;
		break;
	case ADP5360_PGOOD_STATUS_VOUT1_OK:
		mask = ADP5360_PGOOD1_VOUT1_OK_MASK;
		data->pgood_handlers.vout1_ok_callback.handler = handler;
		data->pgood_handlers.vout1_ok_callback.user_data = user_data;
		break;
	default:
		return -EINVAL;
	}

	if (pin == ADP5360_PGOOD1) {
		return mfd_adp5360_reg_update(dev, ADP5360_MFD_REG_PGOOD1_MASK, mask,
					      handler ? 1 : 0);
	} else if (pin == ADP5360_PGOOD2) {
		return mfd_adp5360_reg_update(dev, ADP5360_MFD_REG_PGOOD2_MASK, mask,
					      handler ? 1 : 0);
	} else {
		return -EINVAL;
	}
}

static int mfd_adp5360_pgood_callback_init(const struct device *dev)
{
	struct mfd_adp5360_data *data = dev->data;

	/* Initialize all pgood handlers to NULL */
	data->pgood_handlers.manual_reset_pressed_callback.handler = NULL;
	data->pgood_handlers.manual_reset_pressed_callback.user_data = NULL;
	data->pgood_handlers.charge_complete_callback.handler = NULL;
	data->pgood_handlers.charge_complete_callback.user_data = NULL;
	data->pgood_handlers.vbus_ok_callback.handler = NULL;
	data->pgood_handlers.vbus_ok_callback.user_data = NULL;
	data->pgood_handlers.battery_ok_callback.handler = NULL;
	data->pgood_handlers.battery_ok_callback.user_data = NULL;
	data->pgood_handlers.vout2_ok_callback.handler = NULL;
	data->pgood_handlers.vout2_ok_callback.user_data = NULL;
	data->pgood_handlers.vout1_ok_callback.handler = NULL;
	data->pgood_handlers.vout1_ok_callback.user_data = NULL;

	return 0;
}

static void mfd_adp5360_pgood_main_cb(const struct device *dev)
{
	const struct mfd_adp5360_config *config = dev->config;
	struct mfd_adp5360_data *data = dev->data;
	int ret = 0;
	uint8_t pgood_status;

	ret = mfd_adp5360_reg_read(dev, ADP5360_MFD_REG_PGOOD_STATUS, &pgood_status);
	if (ret < 0) {
		LOG_ERR("Failed to read pgood status register");
		return;
	}
	/* Check which status asserted and run handlers if present */
	mfd_adp5360_call_status_handler(dev, data->pgood_handlers.charge_complete_callback,
					pgood_status, ADP5360_PGOOD1_CHARGE_COMPLETE_MASK);

	mfd_adp5360_call_status_handler(dev, data->pgood_handlers.vbus_ok_callback, pgood_status,
					ADP5360_PGOOD1_VBUS_OK_MASK);

	mfd_adp5360_call_status_handler(dev, data->pgood_handlers.battery_ok_callback, pgood_status,
					ADP5360_PGOOD1_BATTERY_OK_MASK);

	mfd_adp5360_call_status_handler(dev, data->pgood_handlers.vout2_ok_callback, pgood_status,
					ADP5360_PGOOD1_VOUT2_OK_MASK);

	mfd_adp5360_call_status_handler(dev, data->pgood_handlers.vout1_ok_callback, pgood_status,
					ADP5360_PGOOD1_VOUT1_OK_MASK);

	ret = gpio_pin_interrupt_configure_dt(&config->pgood1_gpio, GPIO_INT_EDGE_FALLING);
	if (ret < 0) {
		LOG_ERR("Failed to re-enable pgood1 interrupt");
		return;
	}

	__ASSERT(ret == 0, "pgood Interrupt Configuration Failed");
}

static void mfd_adp5360_pgood1_gpio_callback(const struct device *dev, struct gpio_callback *cb,
					     uint32_t pins)
{
	struct mfd_adp5360_data *data = CONTAINER_OF(cb, struct mfd_adp5360_data, pgood1_callback);
	const struct mfd_adp5360_config *config = data->dev->config;
	int ret;
	/* Disable the interrupt to prevent multiple triggers */
	ret = gpio_pin_interrupt_configure_dt(&config->pgood1_gpio, GPIO_INT_DISABLE);
	if (ret < 0) {
		LOG_ERR("Failed to disable pgood1 interrupt");
		return;
	}

#if defined(CONFIG_ADP5360_TRIGGER_OWN_THREAD)
	k_sem_give(&data->pgood1_sem);
#elif defined(CONFIG_ADP5360_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->pgood1_work);
#endif
}

#if defined(CONFIG_ADP5360_TRIGGER_OWN_THREAD)
static void mfd_adp5360_pgood1_thread(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	struct mfd_adp5360_data *data = (struct mfd_adp5360_data *)arg1;

	while (1) {
		/* Wait for the pgood1 event to be signaled */
		k_sem_take(&data->pgood1_sem, K_FOREVER);
		mfd_adp5360_pgood_main_cb(data->dev);
	}
}
#endif

#if defined(CONFIG_ADP5360_TRIGGER_GLOBAL_THREAD)
static void mfd_adp5360_pgood1_work_cb(struct k_work *work)
{
	struct mfd_adp5360_data *data = CONTAINER_OF(work, struct mfd_adp5360_data, pgood1_work);
	const struct device *dev = data->dev;
	/* Handle the pgood1 event in workqueue context */
	mfd_adp5360_pgood_main_cb(dev);
}
#endif

static void mfd_adp5360_pgood2_gpio_callback(const struct device *dev, struct gpio_callback *cb,
					     uint32_t pins)
{
	struct mfd_adp5360_data *data = CONTAINER_OF(cb, struct mfd_adp5360_data, pgood2_callback);
	const struct mfd_adp5360_config *config = data->dev->config;
	int ret;
	/* Disable the interrupt to prevent multiple triggers */
	ret = gpio_pin_interrupt_configure_dt(&config->pgood2_gpio, GPIO_INT_DISABLE);
	if (ret < 0) {
		LOG_ERR("Failed to disable pgood2 interrupt");
		return;
	}
#if defined(CONFIG_ADP5360_TRIGGER_OWN_THREAD)
	k_sem_give(&data->pgood2_sem);
#elif defined(CONFIG_ADP5360_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->pgood2_work);
#endif
}

#if defined(CONFIG_ADP5360_TRIGGER_OWN_THREAD)
static void mfd_adp5360_pgood2_thread(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	struct mfd_adp5360_data *data = (struct mfd_adp5360_data *)arg1;

	while (1) {
		/* Wait for the pgood2 event to be signaled */
		k_sem_take(&data->pgood2_sem, K_FOREVER);
		mfd_adp5360_pgood_main_cb(data->dev);
	}
}
#endif

#if defined(CONFIG_ADP5360_TRIGGER_GLOBAL_THREAD)
static void mfd_adp5360_pgood2_work_cb(struct k_work *work)
{
	struct mfd_adp5360_data *data = CONTAINER_OF(work, struct mfd_adp5360_data, pgood2_work);
	const struct device *dev = data->dev;
	/* Handle the pgood2 event in workqueue context */
	mfd_adp5360_pgood_main_cb(dev);
}
#endif

int mfd_adp5360_init_pgood_interrupt(const struct device *dev)
{
	const struct mfd_adp5360_config *config = dev->config;
	struct mfd_adp5360_data *data = dev->data;
	int ret;

	data->dev = dev;

	if (config->pgood1_gpio.port) {
		if (!gpio_is_ready_dt(&config->pgood1_gpio)) {
			LOG_ERR("PGOOD1 GPIO is not ready");
			return -ENODEV;
		}

		/* Initialize semaphore/work BEFORE enabling interrupt to avoid race condition */
#if defined(CONFIG_ADP5360_TRIGGER_OWN_THREAD)
		k_sem_init(&data->pgood1_sem, 0, K_SEM_MAX_LIMIT);
		k_thread_create(&data->pgood1_thread, data->pgood1_thread_stack,
				K_THREAD_STACK_SIZEOF(data->pgood1_thread_stack),
				(k_thread_entry_t)mfd_adp5360_pgood1_thread, data, NULL, NULL,
				K_PRIO_COOP(CONFIG_ADP5360_THREAD_PRIORITY), 0, K_NO_WAIT);
#elif defined(CONFIG_ADP5360_TRIGGER_GLOBAL_THREAD)
		k_work_init(&data->pgood1_work, mfd_adp5360_pgood1_work_cb);
#endif

		ret = gpio_pin_configure_dt(&config->pgood1_gpio, GPIO_INPUT);
		if (ret < 0) {
			LOG_ERR("Failed to configure pgood1 GPIO pin");
			return ret;
		}

		gpio_init_callback(&data->pgood1_callback, mfd_adp5360_pgood1_gpio_callback,
				   BIT(config->pgood1_gpio.pin));
		ret = gpio_add_callback(config->pgood1_gpio.port, &data->pgood1_callback);
		if (ret < 0) {
			LOG_ERR("Failed to add pgood1 GPIO callback");
			return ret;
		}

		ret = gpio_pin_interrupt_configure_dt(&config->pgood1_gpio, GPIO_INT_EDGE_FALLING);
		if (ret < 0) {
			LOG_ERR("Failed to configure pgood1 GPIO pin interrupt");
			return ret;
		}

		ret = mfd_adp5360_reg_update(dev, ADP5360_MFD_REG_PGOOD1_MASK,
					     ADP5360_PGOOD1_REV_MASK, 1);
		if (ret < 0) {
			LOG_ERR("Failed to set pgood1 into active low");
			return ret;
		}
	}

	if (config->pgood2_gpio.port) {
		if (!gpio_is_ready_dt(&config->pgood2_gpio)) {
			LOG_ERR("PGOOD2 GPIO is not ready");
			return -ENODEV;
		}

		/* Initialize semaphore/work BEFORE enabling interrupt to avoid race condition */
#if defined(CONFIG_ADP5360_TRIGGER_OWN_THREAD)
		k_sem_init(&data->pgood2_sem, 0, K_SEM_MAX_LIMIT);
		k_thread_create(&data->pgood2_thread, data->pgood2_thread_stack,
				K_THREAD_STACK_SIZEOF(data->pgood2_thread_stack),
				(k_thread_entry_t)mfd_adp5360_pgood2_thread, data, NULL, NULL,
				K_PRIO_COOP(CONFIG_ADP5360_THREAD_PRIORITY), 0, K_NO_WAIT);
#elif defined(CONFIG_ADP5360_TRIGGER_GLOBAL_THREAD)
		k_work_init(&data->pgood2_work, mfd_adp5360_pgood2_work_cb);
#endif

		ret = gpio_pin_configure_dt(&config->pgood2_gpio, GPIO_INPUT);
		if (ret < 0) {
			LOG_ERR("Failed to configure pgood2 GPIO pin");
			return ret;
		}

		gpio_init_callback(&data->pgood2_callback, mfd_adp5360_pgood2_gpio_callback,
				   BIT(config->pgood2_gpio.pin));
		ret = gpio_add_callback(config->pgood2_gpio.port, &data->pgood2_callback);
		if (ret < 0) {
			LOG_ERR("Failed to add pgood2 GPIO callback");
			return ret;
		}

		ret = gpio_pin_interrupt_configure_dt(&config->pgood2_gpio, GPIO_INT_EDGE_FALLING);
		if (ret < 0) {
			LOG_ERR("Failed to configure pgood2 GPIO pin interrupt");
			return ret;
		}

		ret = mfd_adp5360_reg_update(dev, ADP5360_MFD_REG_PGOOD2_MASK,
					     ADP5360_PGOOD2_REV_MASK, 1);
		if (ret < 0) {
			LOG_ERR("Failed to set pgood2 into active low");
			return ret;
		}
	}

	/* Initialize pgood handlers to NULL */
	ret = mfd_adp5360_pgood_callback_init(dev);
	if (ret < 0) {
		LOG_ERR("Failed to initialize pgood callback");
		return ret;
	}

	return 0;
}

int mfd_adp5360_reset_trigger_set(const struct device *dev, mfd_adp5360_interrupt_callback handler,
				  void *user_data)
{
	struct mfd_adp5360_data *data = dev->data;

	LOG_DBG("Setup Reset trigger");
	if (handler == NULL) {
		LOG_ERR("No handler set!");
		return -EINVAL;
	}

	data->reset_status_handler.handler = handler;
	data->reset_status_handler.user_data = user_data;
	return 0;
}

static int mfd_adp5360_reset_callback_init(const struct device *dev)
{
	struct mfd_adp5360_data *data = dev->data;

	/* Initialize reset callback to NULL */
	data->reset_status_handler.handler = NULL;
	data->reset_status_handler.user_data = NULL;

	return 0;
}

static void mfd_adp5360_reset_cb(const struct device *dev)
{
	const struct mfd_adp5360_config *config = dev->config;
	struct mfd_adp5360_data *data = dev->data;
	int ret;

	if (data->reset_status_handler.handler) {
		data->reset_status_handler.handler(dev, data->reset_status_handler.user_data);
	} else {
		LOG_WRN("Reset status interrupt occurred but no handler registered");
	}

	ret = gpio_pin_interrupt_configure_dt(&config->reset_status_gpio, GPIO_INT_EDGE_FALLING);
	if (ret < 0) {
		LOG_ERR("Failed to re-enable interrupt");
		return;
	}
}

static void mfd_adp5360_reset_gpio_callback(const struct device *dev, struct gpio_callback *cb,
					    uint32_t pins)
{
	struct mfd_adp5360_data *data =
		CONTAINER_OF(cb, struct mfd_adp5360_data, reset_status_callback);
	const struct mfd_adp5360_config *config = data->dev->config;
	int ret;
	/* Disable the interrupt to prevent multiple triggers */
	ret = gpio_pin_interrupt_configure_dt(&config->reset_status_gpio, GPIO_INT_DISABLE);
	if (ret < 0) {
		LOG_ERR("Failed to disable reset status GPIO interrupt");
		return;
	}
#if defined(CONFIG_ADP5360_TRIGGER_OWN_THREAD)
	k_sem_give(&data->reset_status_sem);
#elif defined(CONFIG_ADP5360_TRIGGER_GLOBAL_THREAD)
	k_work_submit(&data->reset_status_work);
#endif
}

#if defined(CONFIG_ADP5360_TRIGGER_OWN_THREAD)
static void mfd_adp5360_reset_status_thread(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	struct mfd_adp5360_data *data = (struct mfd_adp5360_data *)arg1;

	while (1) {
		k_sem_take(&data->reset_status_sem, K_FOREVER);
		mfd_adp5360_reset_cb(data->dev);
	}
}
#endif

#if defined(CONFIG_ADP5360_TRIGGER_GLOBAL_THREAD)
static void mfd_adp5360_reset_status_work_cb(struct k_work *work)
{
	struct mfd_adp5360_data *data =
		CONTAINER_OF(work, struct mfd_adp5360_data, reset_status_work);
	const struct device *dev = data->dev;

	mfd_adp5360_reset_cb(dev);
}
#endif

int mfd_adp5360_init_reset_status_interrupt(const struct device *dev)
{
	const struct mfd_adp5360_config *config = dev->config;
	struct mfd_adp5360_data *data = dev->data;
	int ret;

	if (!gpio_is_ready_dt(&config->reset_status_gpio)) {
		LOG_ERR("Reset status GPIO is not ready");
		return -ENODEV;
	}

	ret = mfd_adp5360_reset_callback_init(dev);
	if (ret < 0) {
		LOG_ERR("Failed to initialize reset status callback");
		return ret;
	}

	data->dev = dev;

	/* Initialize semaphore/work BEFORE enabling interrupt to avoid race condition */
#if defined(CONFIG_ADP5360_TRIGGER_OWN_THREAD)
	k_sem_init(&data->reset_status_sem, 0, K_SEM_MAX_LIMIT);
	k_thread_create(&data->reset_thread, data->reset_thread_stack,
			K_THREAD_STACK_SIZEOF(data->reset_thread_stack),
			(k_thread_entry_t)mfd_adp5360_reset_status_thread, data, NULL, NULL,
			K_PRIO_COOP(CONFIG_ADP5360_THREAD_PRIORITY), 0, K_NO_WAIT);
#elif defined(CONFIG_ADP5360_TRIGGER_GLOBAL_THREAD)
	k_work_init(&data->reset_status_work, mfd_adp5360_reset_status_work_cb);
#endif

	ret = gpio_pin_configure_dt(&config->reset_status_gpio, GPIO_INPUT);
	if (ret < 0) {
		LOG_ERR("Failed to configure reset status GPIO pin");
		return ret;
	}

	gpio_init_callback(&data->reset_status_callback, mfd_adp5360_reset_gpio_callback,
			   BIT(config->reset_status_gpio.pin));
	ret = gpio_add_callback(config->reset_status_gpio.port, &data->reset_status_callback);
	if (ret < 0) {
		LOG_ERR("Failed to add pgood1 GPIO callback");
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(&config->reset_status_gpio, GPIO_INT_EDGE_RISING);
	if (ret < 0) {
		LOG_ERR("Failed to configure reset status GPIO pin interrupt");
		return ret;
	}

	return 0;
}
