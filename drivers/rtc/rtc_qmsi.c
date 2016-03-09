/*
 * Copyright (c) 2015 Intel Corporation.
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

#include <device.h>
#include <drivers/ioapic.h>
#include <init.h>
#include <nanokernel.h>
#include <rtc.h>

#include "qm_rtc.h"

static struct device *rtc_qmsi_dev;

static void (*user_callback)(struct device *dev);

static void rtc_qmsi_enable(struct device *dev)
{
	clk_periph_enable(CLK_PERIPH_RTC_REGISTER | CLK_PERIPH_CLK);
}

static void rtc_qmsi_disable(struct device *dev)
{
	clk_periph_disable(CLK_PERIPH_RTC_REGISTER);
}

static void rtc_callback(void)
{
	if (user_callback)
		user_callback(rtc_qmsi_dev);
}

static int rtc_qmsi_set_config(struct device *dev, struct rtc_config *cfg)
{
	qm_rtc_config_t qm_cfg;

	qm_cfg.init_val = cfg->init_val;
	qm_cfg.alarm_en = cfg->alarm_enable;
	qm_cfg.alarm_val = cfg->alarm_val;
	qm_cfg.callback = rtc_callback;

	user_callback = cfg->cb_fn;

	if (qm_rtc_set_config(QM_RTC_0, &qm_cfg) != QM_RC_OK)
		return DEV_FAIL;

	return 0;
}

static int rtc_qmsi_set_alarm(struct device *dev, const uint32_t alarm_val)
{
	return qm_rtc_set_alarm(QM_RTC_0, alarm_val) == QM_RC_OK ? 0 : DEV_FAIL;
}

static uint32_t rtc_qmsi_read(struct device *dev)
{
	return QM_RTC[QM_RTC_0].rtc_ccvr;
}

static struct rtc_driver_api api = {
	.enable = rtc_qmsi_enable,
	.disable = rtc_qmsi_disable,
	.read = rtc_qmsi_read,
	.set_config = rtc_qmsi_set_config,
	.set_alarm = rtc_qmsi_set_alarm,
};

static int rtc_qmsi_init(struct device *dev)
{
	IRQ_CONNECT(CONFIG_RTC_IRQ, CONFIG_RTC_IRQ_PRI, qm_rtc_isr_0, 0,
		    IOAPIC_EDGE | IOAPIC_HIGH);

	/* Unmask RTC interrupt */
	irq_enable(CONFIG_RTC_IRQ);

	/* Route RTC interrupt to Lakemont */
	QM_SCSS_INT->int_rtc_mask &= ~BIT(0);

	dev->driver_api = &api;
	return 0;
}

DEVICE_INIT(rtc, CONFIG_RTC_DRV_NAME, &rtc_qmsi_init,
			NULL, NULL,
			SECONDARY, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

static struct device *rtc_qmsi_dev = DEVICE_GET(rtc);
