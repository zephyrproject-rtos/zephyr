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

#include <errno.h>

#include <device.h>
#include <drivers/ioapic.h>
#include <init.h>
#include <nanokernel.h>
#include <rtc.h>

#include "qm_isr.h"
#include "qm_rtc.h"


static void rtc_qmsi_enable(struct device *dev)
{
	clk_periph_enable(CLK_PERIPH_RTC_REGISTER | CLK_PERIPH_CLK);
}

static void rtc_qmsi_disable(struct device *dev)
{
	clk_periph_disable(CLK_PERIPH_RTC_REGISTER);
}


static int rtc_qmsi_set_config(struct device *dev, struct rtc_config *cfg)
{
	qm_rtc_config_t qm_cfg;

	qm_cfg.init_val = cfg->init_val;
	qm_cfg.alarm_en = cfg->alarm_enable;
	qm_cfg.alarm_val = cfg->alarm_val;
	/* Casting callback type due different input parameter from QMSI
	 * compared aganst the Zephyr callback from void cb(struct device *dev)
	 * to void cb(void *)
	 */
	qm_cfg.callback = (void *) cfg->cb_fn;
	qm_cfg.callback_data = dev;

	if (qm_rtc_set_config(QM_RTC_0, &qm_cfg))
		return -EIO;

	return 0;
}

static int rtc_qmsi_set_alarm(struct device *dev, const uint32_t alarm_val)
{
	return qm_rtc_set_alarm(QM_RTC_0, alarm_val);
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
	IRQ_CONNECT(QM_IRQ_RTC_0, CONFIG_RTC_0_IRQ_PRI, qm_rtc_isr_0, 0,
		    IOAPIC_EDGE | IOAPIC_HIGH);

	/* Unmask RTC interrupt */
	irq_enable(QM_IRQ_RTC_0);

	/* Route RTC interrupt to Lakemont */
	QM_SCSS_INT->int_rtc_mask &= ~BIT(0);

	return 0;
}

DEVICE_AND_API_INIT(rtc, CONFIG_RTC_0_NAME, &rtc_qmsi_init, NULL, NULL,
		    SECONDARY, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    (void *)&api);
