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

#include "qm_aon_counters.h"

struct aon_timer_data {
	void *callback_user_data;
	counter_callback_t timer_callback;
};

static void aonpt_int_callback(void);

static struct aon_timer_data aonpt_driver_data;

static int aon_timer_qmsi_start(struct device *dev)
{
	struct aon_timer_data *driver_data = dev->driver_data;
	qm_aonpt_config_t qmsi_cfg;

	driver_data->timer_callback = NULL;

	qmsi_cfg.callback = NULL;
	qmsi_cfg.int_en = false;
	/* AONPT is a countdown timer. So, set the initial value to
	 * the maximum value.
	 */
	qmsi_cfg.count = 0xffffffff;

	if (qm_aonpt_set_config(QM_SCSS_AON_0, &qmsi_cfg) !=
	    QM_RC_OK) {
		return -EIO;
	}

	return 0;
}

static int aon_timer_qmsi_stop(struct device *dev)
{
	qm_aonpt_config_t qmsi_cfg;

	qmsi_cfg.callback = NULL;
	qmsi_cfg.int_en = false;
	qmsi_cfg.count = 0;

	qm_aonpt_set_config(QM_SCSS_AON_0, &qmsi_cfg);

	return 0;
}

static uint32_t aon_timer_qmsi_read(void)
{
	return qm_aonpt_get_value(QM_SCSS_AON_0);
}

static int aon_timer_qmsi_set_alarm(struct device *dev,
				    counter_callback_t callback,
				    uint32_t count, void *user_data)
{
	struct aon_timer_data *driver_data = dev->driver_data;
	qm_aonpt_config_t qmsi_cfg;

	qm_aonpt_get_config(QM_SCSS_AON_0, &qmsi_cfg);

	/* Check if timer has been started */
	if (qmsi_cfg.count == 0) {
		return -ENOTSUP;
	}

	driver_data->timer_callback = callback;
	driver_data->callback_user_data = user_data;

	qmsi_cfg.callback = aonpt_int_callback;
	qmsi_cfg.int_en = true;
	qmsi_cfg.count = count;

	if (qm_aonpt_set_config(QM_SCSS_AON_0, &qmsi_cfg) !=
	    QM_RC_OK) {
		driver_data->timer_callback = NULL;
		driver_data->callback_user_data = NULL;
		return -EIO;
	}

	return 0;
}

static struct counter_driver_api aon_timer_qmsi_api = {
	.start = aon_timer_qmsi_start,
	.stop = aon_timer_qmsi_stop,
	.read = aon_timer_qmsi_read,
	.set_alarm = aon_timer_qmsi_set_alarm,
};

static int aon_timer_init(struct device *dev)
{
	struct aon_timer_data *driver_data = dev->driver_data;

	dev->driver_api = &aon_timer_qmsi_api;

	driver_data->callback_user_data = NULL;
	driver_data->timer_callback = NULL;

	IRQ_CONNECT(CONFIG_AON_TIMER_IRQ,
		    CONFIG_AON_TIMER_IRQ_PRI, qm_aonpt_isr_0,
		    NULL, IOAPIC_EDGE | IOAPIC_HIGH);

	irq_enable(CONFIG_AON_TIMER_IRQ);

	QM_SCSS_INT->int_aon_timer_mask &= ~BIT(0);

	return 0;
}

DEVICE_INIT(aon_timer, CONFIG_AON_TIMER_QMSI_DEV_NAME,
	    aon_timer_init, &aonpt_driver_data, NULL, SECONDARY,
	    CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

static void aonpt_int_callback(void)
{
	struct device *dev = DEVICE_GET(aon_timer);
	struct aon_timer_data *driver_data = dev->driver_data;

	if (driver_data->timer_callback) {
		(*driver_data->timer_callback)(dev,
		driver_data->callback_user_data);
	}
}
