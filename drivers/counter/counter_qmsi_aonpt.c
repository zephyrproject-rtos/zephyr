/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <errno.h>

#include <device.h>
#include <init.h>
#include <drivers/ioapic.h>
#include <counter.h>
#include <power.h>

#include "qm_aon_counters.h"
#include "qm_isr.h"

static void aonpt_int_callback(void *user_data);

static counter_callback_t user_cb;

struct aon_data {
	struct nano_sem sem;
};

#ifdef CONFIG_AON_API_REENTRANCY
static struct aon_data aonpt_context;
#define AONPT_CONTEXT (&aonpt_context)
static const int reentrancy_protection = 1;
#else
#define AONPT_CONTEXT (NULL)
static const int reentrancy_protection;
#endif

static void aon_reentrancy_init(struct device *dev)
{
	struct aon_data *context = dev->driver_data;

	if (!reentrancy_protection) {
		return;
	}

	nano_sem_init(&context->sem);
	nano_sem_give(&context->sem);
}

static void aon_critical_region_start(struct device *dev)
{
	struct aon_data *context = dev->driver_data;

	if (!reentrancy_protection) {
		return;
	}

	nano_sem_take(&context->sem, TICKS_UNLIMITED);
}

static void aon_critical_region_end(struct device *dev)
{
	struct aon_data *context = dev->driver_data;

	if (!reentrancy_protection) {
		return;
	}

	nano_sem_give(&context->sem);
}

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

	aon_critical_region_start(dev);
	if (qm_aonpt_set_config(QM_SCSS_AON_0, &qmsi_cfg)) {
		result = -EIO;
	}
	aon_critical_region_end(dev);

	return result;
}

static int aon_timer_qmsi_stop(struct device *dev)
{
	qm_aonpt_config_t qmsi_cfg;

	qmsi_cfg.callback = NULL;
	qmsi_cfg.int_en = false;
	qmsi_cfg.count = 0;
	qmsi_cfg.callback_data = NULL;

	aon_critical_region_start(dev);
	qm_aonpt_set_config(QM_SCSS_AON_0, &qmsi_cfg);
	aon_critical_region_end(dev);

	return 0;
}

static uint32_t aon_timer_qmsi_read(void)
{
	uint32_t value;

	qm_aonpt_get_value(QM_SCSS_AON_0, &value);

	return value;
}

static int aon_timer_qmsi_set_alarm(struct device *dev,
				    counter_callback_t callback,
				    uint32_t count, void *user_data)
{
	qm_aonpt_config_t qmsi_cfg;
	int result = 0;

	/* Check if timer has been started */
	if (QM_SCSS_AON[QM_SCSS_AON_0].aonpt_cfg == 0) {
		return -ENOTSUP;
	}

	user_cb = callback;

	qmsi_cfg.callback = aonpt_int_callback;
	qmsi_cfg.int_en = true;
	qmsi_cfg.count = count;
	qmsi_cfg.callback_data = user_data;

	aon_critical_region_start(dev);
	if (qm_aonpt_set_config(QM_SCSS_AON_0, &qmsi_cfg)) {
		user_cb = NULL;
		result = -EIO;
	}
	aon_critical_region_end(dev);

	return result;
}

static struct counter_driver_api aon_timer_qmsi_api = {
	.start = aon_timer_qmsi_start,
	.stop = aon_timer_qmsi_stop,
	.read = aon_timer_qmsi_read,
	.set_alarm = aon_timer_qmsi_set_alarm,
};

static int aon_timer_init(struct device *dev)
{
	dev->driver_api = &aon_timer_qmsi_api;

	user_cb = NULL;

	IRQ_CONNECT(QM_IRQ_AONPT_0, CONFIG_AON_TIMER_IRQ_PRI,
		    qm_aonpt_isr_0, NULL, IOAPIC_EDGE | IOAPIC_HIGH);

	irq_enable(QM_IRQ_AONPT_0);

	QM_SCSS_INT->int_aon_timer_mask &= ~BIT(0);

	aon_reentrancy_init(dev);

	return 0;
}

#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
static uint32_t int_aonpt_mask_save;

static int aonpt_suspend_device(struct device *dev, int pm_policy)
{
	if (pm_policy == SYS_PM_DEEP_SLEEP) {
		int_aonpt_mask_save = QM_SCSS_INT->int_aon_timer_mask;
	}

	return 0;
}

static int aonpt_resume_device(struct device *dev, int pm_policy)
{
	if (pm_policy == SYS_PM_DEEP_SLEEP) {
		QM_SCSS_INT->int_aon_timer_mask = int_aonpt_mask_save;
	}

	return 0;
}
#endif

DEFINE_DEVICE_PM_OPS(aon_timer, aonpt_suspend_device, aonpt_resume_device);

DEVICE_AND_API_INIT_PM(aon_timer, CONFIG_AON_TIMER_QMSI_DEV_NAME,
		    aon_timer_init, DEVICE_PM_OPS_GET(aon_timer), AONPT_CONTEXT,
		    NULL, SECONDARY, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    (void *)&aon_timer_qmsi_api);

static void aonpt_int_callback(void *user_data)
{
	if (user_cb) {
		(*user_cb)(DEVICE_GET(aon_timer), user_data);
	}
}
