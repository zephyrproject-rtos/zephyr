/*
 * Copyright 2019-2020 Peter Bigot Consulting, LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT regulator_fixed

#include <kernel.h>
#include <drivers/regulator.h>
#include <drivers/gpio.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(regulator_fixed, CONFIG_REGULATOR_LOG_LEVEL);

#define OPTION_ALWAYS_ON_POS 0
#define OPTION_ALWAYS_ON BIT(OPTION_ALWAYS_ON_POS)
#define OPTION_BOOT_ON_POS 1
#define OPTION_BOOT_ON BIT(OPTION_BOOT_ON_POS)

struct driver_config {
	const char *regulator_name;
	const char *gpio_name;
	uint32_t startup_delay_us;
	uint32_t off_on_delay_us;
	gpio_pin_t gpio_pin;
	gpio_dt_flags_t gpio_flags;
	uint8_t options;
};

enum work_task {
	WORK_TASK_UNDEFINED,
	WORK_TASK_ENABLE,
	WORK_TASK_DISABLE,
	WORK_TASK_DELAY,
};

struct driver_data_onoff {
	const struct device *gpio;
	const struct device *dev;
	struct onoff_manager mgr;
#ifdef CONFIG_MULTITHREADING
	struct k_work_delayable dwork;
#endif /* CONFIG_MULTITHREADING */
	onoff_notify_fn notify;
	enum work_task task;
};

/* Common initialization of GPIO device and pin state.
 *
 * @param dev the regulator device, whether sync or onoff
 *
 * @param gpiop where to store the GPIO device pointer
 *
 * @return negative on error, otherwise zero.
 */
static int common_init(const struct device *dev,
		       const struct device **gpiop)
{
	const struct driver_config *cfg = dev->config;
	const struct device *gpio = device_get_binding(cfg->gpio_name);

	if (gpio == NULL) {
		LOG_ERR("no GPIO device: %s", cfg->gpio_name);
		return -ENODEV;
	}

	*gpiop = gpio;

	gpio_flags_t flags = cfg->gpio_flags;
	bool on = cfg->options & (OPTION_ALWAYS_ON | OPTION_BOOT_ON);
	uint32_t delay_us = 0;

	if (on) {
		flags |= GPIO_OUTPUT_ACTIVE;
		delay_us = cfg->startup_delay_us;
	} else {
		flags |= GPIO_OUTPUT_INACTIVE;
	}

	int rc = gpio_pin_configure(gpio, cfg->gpio_pin, flags);

	if ((rc == 0) && (delay_us > 0)) {
		/* Turned on and we have to wait until the on
		 * completes.  Since this is in the driver init we
		 * can't sleep.
		 */
		k_busy_wait(delay_us);
	}

	return rc;
}

static void finalize_transition(struct driver_data_onoff *data,
				onoff_notify_fn notify,
				uint32_t delay_us,
				int rc)
{
	const struct driver_config *cfg = data->dev->config;

	LOG_DBG("%s: finalize %d delay %u us", cfg->regulator_name, rc, delay_us);

	/* If there's no error and we have to delay, do so. */
	if ((rc >= 0) && (delay_us > 0)) {
		/* If the delay is less than a tick or we're not
		 * sleep-capable we have to busy-wait.
		 */
		if ((k_us_to_ticks_floor32(delay_us) == 0)
		    || k_is_pre_kernel()
		    || !IS_ENABLED(CONFIG_MULTITHREADING)) {
			k_busy_wait(delay_us);
#ifdef CONFIG_MULTITHREADING
		} else {
			/* Otherwise sleep in the work queue. */
			__ASSERT_NO_MSG(data->task == WORK_TASK_UNDEFINED);
			data->task = WORK_TASK_DELAY;
			data->notify = notify;
			rc = k_work_schedule(&data->dwork, K_USEC(delay_us));
			if (rc == 0) {
				return;
			}
#endif /* CONFIG_MULTITHREADING */
		}
	}

	notify(&data->mgr, rc);
}

#ifdef CONFIG_MULTITHREADING
/* The worker is used for several things:
 *
 * * If a transition occurred in a context where the GPIO state could
 *   not be changed that's done here.
 * * If a start or stop transition requires a delay that exceeds one
 *   tick the notification after the delay is performed here.
 */
static void onoff_worker(struct k_work *work)
{
	struct k_work_delayable *dwork
		= k_work_delayable_from_work(work);
	struct driver_data_onoff *data
		= CONTAINER_OF(dwork, struct driver_data_onoff,
			       dwork);
	onoff_notify_fn notify = data->notify;
	const struct driver_config *cfg = data->dev->config;
	uint32_t delay_us = 0;
	int rc = 0;

	if (data->task == WORK_TASK_ENABLE) {
		rc = gpio_pin_set(data->gpio, cfg->gpio_pin, true);
		LOG_DBG("%s: work enable: %d", cfg->regulator_name, rc);
		delay_us = cfg->startup_delay_us;
	} else if (data->task == WORK_TASK_DISABLE) {
		rc = gpio_pin_set(data->gpio, cfg->gpio_pin, false);
		LOG_DBG("%s: work disable: %d", cfg->regulator_name, rc);
		delay_us = cfg->off_on_delay_us;
	} else if (data->task == WORK_TASK_DELAY) {
		LOG_DBG("%s: work delay complete", cfg->regulator_name);
	}

	data->notify = NULL;
	data->task = WORK_TASK_UNDEFINED;
	finalize_transition(data, notify, delay_us, rc);
}
#endif /* CONFIG_MULTITHREADING */

static void start(struct onoff_manager *mgr,
		  onoff_notify_fn notify)
{
	struct driver_data_onoff *data =
		CONTAINER_OF(mgr, struct driver_data_onoff, mgr);
	const struct driver_config *cfg = data->dev->config;
	uint32_t delay_us = cfg->startup_delay_us;
	int rc = 0;

	LOG_DBG("%s: start", cfg->regulator_name);

	if ((cfg->options & OPTION_ALWAYS_ON) != 0) {
		delay_us = 0;
		goto finalize;
	}

	rc = gpio_pin_set(data->gpio, cfg->gpio_pin, true);

#ifdef CONFIG_MULTITHREADING
	if (rc == -EWOULDBLOCK) {
		/* Perform the enable and finalization in a work item.
		 */
		LOG_DBG("%s: start deferred", cfg->regulator_name);
		__ASSERT_NO_MSG(data->task == WORK_TASK_UNDEFINED);
		data->task = WORK_TASK_ENABLE;
		data->notify = notify;
		k_work_schedule(&data->dwork, K_NO_WAIT);
		return;
	}
#endif /* CONFIG_MULTITHREADING */

finalize:
	finalize_transition(data, notify, delay_us, rc);

	return;
}

static void stop(struct onoff_manager *mgr,
		 onoff_notify_fn notify)
{
	struct driver_data_onoff *data =
		CONTAINER_OF(mgr, struct driver_data_onoff, mgr);
	const struct driver_config *cfg = data->dev->config;
	uint32_t delay_us = cfg->off_on_delay_us;
	int rc = 0;

	LOG_DBG("%s: stop", cfg->regulator_name);

	if ((cfg->options & OPTION_ALWAYS_ON) != 0) {
		delay_us = 0;
		goto finalize;
	}

	rc = gpio_pin_set(data->gpio, cfg->gpio_pin, false);

#ifdef CONFIG_MULTITHREADING
	if (rc == -EWOULDBLOCK) {
		/* Perform the disable and finalization in a work
		 * item.
		 */
		LOG_DBG("%s: stop deferred", cfg->regulator_name);
		__ASSERT_NO_MSG(data->task == WORK_TASK_UNDEFINED);
		data->task = WORK_TASK_DISABLE;
		data->notify = notify;
		k_work_schedule(&data->dwork, K_NO_WAIT);
		return;
	}
#endif /* CONFIG_MULTITHREADING */

finalize:
	finalize_transition(data, notify, delay_us, rc);

	return;
}

static int enable_onoff(const struct device *dev, struct onoff_client *cli)
{
	struct driver_data_onoff *data = dev->data;

	return onoff_request(&data->mgr, cli);
}

static int disable_onoff(const struct device *dev)
{
	struct driver_data_onoff *data = dev->data;

	return onoff_release(&data->mgr);
}

static const struct onoff_transitions transitions =
	ONOFF_TRANSITIONS_INITIALIZER(start, stop, NULL);

static const struct regulator_driver_api api_onoff = {
	.enable = enable_onoff,
	.disable = disable_onoff,
};

static int regulator_fixed_init_onoff(const struct device *dev)
{
	struct driver_data_onoff *data = dev->data;
	int rc;

	data->dev = dev;
	rc = onoff_manager_init(&data->mgr, &transitions);
	__ASSERT_NO_MSG(rc == 0);

#ifdef CONFIG_MULTITHREADING
	k_work_init_delayable(&data->dwork, onoff_worker);
#endif /* CONFIG_MULTITHREADING */

	rc = common_init(dev, &data->gpio);
	if (rc >= 0) {
		rc = 0;
	}

	LOG_INF("%s onoff: %d", dev->name, rc);

	return rc;
}

struct driver_data_sync {
	const struct device *gpio;
	struct onoff_sync_service srv;
};

#if DT_HAS_COMPAT_STATUS_OKAY(regulator_fixed_sync) - 0

static int enable_sync(const struct device *dev, struct onoff_client *cli)
{
	struct driver_data_sync *data = dev->data;
	const struct driver_config *cfg = dev->config;
	k_spinlock_key_t key;
	int rc = onoff_sync_lock(&data->srv, &key);

	if ((rc == 0)
	    && ((cfg->options & OPTION_ALWAYS_ON) == 0)) {
		rc = gpio_pin_set(data->gpio, cfg->gpio_pin, true);
	}

	return onoff_sync_finalize(&data->srv, key, cli, rc, true);
}

static int disable_sync(const struct device *dev)
{
	struct driver_data_sync *data = dev->data;
	const struct driver_config *cfg = dev->config;
	k_spinlock_key_t key;
	int rc = onoff_sync_lock(&data->srv, &key);

	if  ((cfg->options & OPTION_ALWAYS_ON) != 0) {
		rc = 0;
	} else if (rc == 1) {
		rc = gpio_pin_set(data->gpio, cfg->gpio_pin, false);
	} else if (rc == 0) {
		rc = -EINVAL;
	} /* else rc > 0, leave it on */

	return onoff_sync_finalize(&data->srv, key, NULL, rc, false);
}

static const struct regulator_driver_api api_sync = {
	.enable = enable_sync,
	.disable = disable_sync,
};

static int regulator_fixed_init_sync(const struct device *dev)
{
	struct driver_data_sync *data = dev->data;
	const struct driver_config *cfg = dev->config;
	int rc = common_init(dev, &data->gpio);

	(void)regulator_fixed_init_onoff;
	(void)api_onoff;
	(void)cfg;

	__ASSERT(cfg->startup_delay_us == 0,
		 "sync not valid with startup delay");
	__ASSERT(cfg->off_on_delay_us == 0,
		 "sync not valid with shutdown delay");

	LOG_INF("%s sync: %d", dev->name, rc);

	return rc;
}

#endif /* DT_HAS_COMPAT_STATUS_OK(regulator_fixed_sync) */

/* This should also check:
 *  && DT_INST_PROP(id, startup_delay_us) == 0
 *  && DT_INST_PROP(id, off_on_delay_us) == 0
 * but the preprocessor magic doesn't seem able to do that so we'll assert
 * in init instead.
 */
#define REG_IS_SYNC(id)						\
	DT_NODE_HAS_COMPAT(DT_DRV_INST(id), regulator_fixed_sync)

#define REG_DATA_TAG(id) COND_CODE_1(REG_IS_SYNC(id),	\
		(driver_data_sync),			\
		(driver_data_onoff))
#define REG_API(id) COND_CODE_1(REG_IS_SYNC(id),	\
		(api_sync),				\
		(api_onoff))
#define REG_INIT(id) COND_CODE_1(REG_IS_SYNC(id),	\
		(regulator_fixed_init_sync),		\
		(regulator_fixed_init_onoff))

#define REGULATOR_DEVICE(id) \
static const struct driver_config regulator_##id##_cfg = { \
	.regulator_name = DT_INST_PROP(id, regulator_name), \
	.gpio_name = DT_INST_GPIO_LABEL(id, enable_gpios), \
	.startup_delay_us = DT_INST_PROP(id, startup_delay_us), \
	.off_on_delay_us = DT_INST_PROP(id, off_on_delay_us), \
	.gpio_pin = DT_INST_GPIO_PIN(id, enable_gpios),	\
	.gpio_flags = DT_INST_GPIO_FLAGS(id, enable_gpios), \
	.options = (DT_INST_PROP(id, regulator_boot_on)	\
		    << OPTION_BOOT_ON_POS) \
		  | (DT_INST_PROP(id, regulator_always_on) \
		     << OPTION_ALWAYS_ON_POS), \
}; \
\
static struct REG_DATA_TAG(id) regulator_##id##_data; \
\
DEVICE_DT_INST_DEFINE(id, REG_INIT(id), device_pm_control_nop, \
		 &regulator_##id##_data, &regulator_##id##_cfg,	       \
		 POST_KERNEL, CONFIG_REGULATOR_FIXED_INIT_PRIORITY,    \
		 &REG_API(id));

DT_INST_FOREACH_STATUS_OKAY(REGULATOR_DEVICE)
