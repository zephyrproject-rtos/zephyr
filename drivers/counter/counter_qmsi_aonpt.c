/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <device.h>
#include <init.h>
#include <drivers/ioapic.h>
#include <counter.h>
#include <power.h>
#include <soc.h>
#include <misc/util.h>

#include "qm_aon_counters.h"
#include "qm_isr.h"

static void aonpt_int_callback(void *user_data);

static counter_callback_t user_cb;

struct aon_data {
#ifdef CONFIG_AON_API_REENTRANCY
	struct k_sem sem;
#endif
#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
	u32_t device_power_state;
#endif
};

#define AONPT_HAS_CONTEXT_DATA \
	(CONFIG_AON_API_REENTRANCY || CONFIG_DEVICE_POWER_MANAGEMENT)

#if AONPT_HAS_CONTEXT_DATA
static struct aon_data aonpt_context;
#define AONPT_CONTEXT (&aonpt_context)
#else
#define AONPT_CONTEXT (NULL)
#endif

#ifdef CONFIG_AON_API_REENTRANCY
#define RP_GET(dev) (&((struct aon_data *)(dev->driver_data))->sem)
#else
#define RP_GET(dev) (NULL)
#endif

static int aon_timer_qmsi_start(struct device *dev)
{
	qm_aonpt_config_t qmsi_cfg;
	int result = 0;

	user_cb = NULL;

	qmsi_cfg.callback = NULL;
	qmsi_cfg.int_en = false;
	/* AONPT is a countdown timer. So, set the initial value to
	 * the maximum value.
	 */
	qmsi_cfg.count = 0xffffffff;
	qmsi_cfg.callback_data = NULL;

	if (IS_ENABLED(CONFIG_AON_API_REENTRANCY)) {
		k_sem_take(RP_GET(dev), K_FOREVER);
	}

	if (qm_aonpt_set_config(QM_AONC_0, &qmsi_cfg)) {
		result = -EIO;
	}

	if (IS_ENABLED(CONFIG_AON_API_REENTRANCY)) {
		k_sem_give(RP_GET(dev));
	}

	return result;
}

static int aon_timer_qmsi_stop(struct device *dev)
{
	qm_aonpt_config_t qmsi_cfg;

	qmsi_cfg.callback = NULL;
	qmsi_cfg.int_en = false;
	qmsi_cfg.count = 0;
	qmsi_cfg.callback_data = NULL;

	if (IS_ENABLED(CONFIG_AON_API_REENTRANCY)) {
		k_sem_take(RP_GET(dev), K_FOREVER);
	}

	qm_aonpt_set_config(QM_AONC_0, &qmsi_cfg);

	if (IS_ENABLED(CONFIG_AON_API_REENTRANCY)) {
		k_sem_give(RP_GET(dev));
	}

	return 0;
}

static u32_t aon_timer_qmsi_read(struct device *dev)
{
	u32_t value;

	qm_aonpt_get_value(QM_AONC_0, (uint32_t *)&value);

	return value;
}

static int aon_timer_qmsi_set_alarm(struct device *dev,
				    counter_callback_t callback,
				    u32_t count, void *user_data)
{
	qm_aonpt_config_t qmsi_cfg;
	int result = 0;

	/* Check if timer has been started */
	if (QM_AONC[QM_AONC_0]->aonpt_cfg == 0) {
		return -ENOTSUP;
	}

	user_cb = callback;

	qmsi_cfg.callback = aonpt_int_callback;
	qmsi_cfg.int_en = true;
	qmsi_cfg.count = count;
	qmsi_cfg.callback_data = user_data;

	if (IS_ENABLED(CONFIG_AON_API_REENTRANCY)) {
		k_sem_take(RP_GET(dev), K_FOREVER);
	}

	if (qm_aonpt_set_config(QM_AONC_0, &qmsi_cfg)) {
		user_cb = NULL;
		result = -EIO;
	}

	if (IS_ENABLED(CONFIG_AON_API_REENTRANCY)) {
		k_sem_give(RP_GET(dev));
	}

	return result;
}

static u32_t aon_timer_qmsi_get_pending_int(struct device *dev)
{
	return QM_AONC[QM_AONC_0]->aonpt_stat;
}

static const struct counter_driver_api aon_timer_qmsi_api = {
	.start = aon_timer_qmsi_start,
	.stop = aon_timer_qmsi_stop,
	.read = aon_timer_qmsi_read,
	.set_alarm = aon_timer_qmsi_set_alarm,
	.get_pending_int = aon_timer_qmsi_get_pending_int,
};

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
static qm_aonc_context_t aonc_ctx;

static void aonpt_qmsi_set_power_state(struct device *dev, u32_t power_state)
{
	struct aon_data *context = dev->driver_data;

	context->device_power_state = power_state;
}

static u32_t aonpt_qmsi_get_power_state(struct device *dev)
{
	struct aon_data *context = dev->driver_data;

	return context->device_power_state;
}

static int aonpt_suspend_device(struct device *dev)
{
	qm_aonpt_save_context(QM_AONC_0, &aonc_ctx);

	aonpt_qmsi_set_power_state(dev, DEVICE_PM_SUSPEND_STATE);

	return 0;
}

static int aonpt_resume_device_from_suspend(struct device *dev)
{
	qm_aonpt_restore_context(QM_AONC_0, &aonc_ctx);

	aonpt_qmsi_set_power_state(dev, DEVICE_PM_ACTIVE_STATE);

	return 0;
}

/*
* Implements the driver control management functionality
* the *context may include IN data or/and OUT data
*/
static int aonpt_qmsi_device_ctrl(struct device *dev, u32_t ctrl_command,
				  void *context)
{
	if (ctrl_command == DEVICE_PM_SET_POWER_STATE) {
		if (*((u32_t *)context) == DEVICE_PM_SUSPEND_STATE) {
			return aonpt_suspend_device(dev);
		} else if (*((u32_t *)context) == DEVICE_PM_ACTIVE_STATE) {
			return aonpt_resume_device_from_suspend(dev);
		}
	} else if (ctrl_command == DEVICE_PM_GET_POWER_STATE) {
		*((u32_t *)context) = aonpt_qmsi_get_power_state(dev);
		return 0;
	}

	return 0;
}
#else
#define aonpt_qmsi_set_power_state(...)
#endif

static int aon_timer_init(struct device *dev)
{
	dev->driver_api = &aon_timer_qmsi_api;

	user_cb = NULL;

	IRQ_CONNECT(IRQ_GET_NUMBER(QM_IRQ_AONPT_0_INT),
		    CONFIG_AON_TIMER_IRQ_PRI, qm_aonpt_0_isr, NULL,
		    IOAPIC_EDGE | IOAPIC_HIGH);

	irq_enable(IRQ_GET_NUMBER(QM_IRQ_AONPT_0_INT));

	QM_IR_UNMASK_INTERRUPTS(QM_INTERRUPT_ROUTER->aonpt_0_int_mask);

	if (IS_ENABLED(CONFIG_AON_API_REENTRANCY)) {
		k_sem_init(RP_GET(dev), 1, UINT_MAX);
	}

	aonpt_qmsi_set_power_state(dev, DEVICE_PM_ACTIVE_STATE);

	return 0;
}


DEVICE_DEFINE(aon_timer, CONFIG_AON_TIMER_QMSI_DEV_NAME, aon_timer_init,
	      aonpt_qmsi_device_ctrl, AONPT_CONTEXT, NULL, POST_KERNEL,
	      CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
	      &aon_timer_qmsi_api);

static void aonpt_int_callback(void *user_data)
{
	if (user_cb) {
		(*user_cb)(DEVICE_GET(aon_timer), user_data);
	}
}
