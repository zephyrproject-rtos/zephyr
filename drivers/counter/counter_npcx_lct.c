/*
 * Copyright (c) 2025 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/counter.h>
#include <zephyr/kernel.h>
#include <zephyr/sys_clock.h>

#include "soc_miwu.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(counter_npcx_lct, CONFIG_COUNTER_LOG_LEVEL);

#if DT_HAS_COMPAT_STATUS_OKAY(nuvoton_npcx_lct_v1)
#define DT_DRV_COMPAT    nuvoton_npcx_lct_v1
#elif DT_HAS_COMPAT_STATUS_OKAY(nuvoton_npcx_lct_v2)
#define DT_DRV_COMPAT    nuvoton_npcx_lct_v2
#elif DT_HAS_COMPAT_STATUS_OKAY(nuvoton_npck_lct)
#define DT_DRV_COMPAT    nuvoton_npck_lct
#endif
BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1, "Invalid number of NPCX LCT peripherals");

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(nuvoton_npcx_lct_v1) +
	     DT_NUM_INST_STATUS_OKAY(nuvoton_npcx_lct_v2) +
	     DT_NUM_INST_STATUS_OKAY(nuvoton_npck_lct) <= 1,
	     "Multiple npcx LCT instances in DT");

#define COUNTER_NPCX_LCT_MAX_SECOND        DT_INST_PROP(0, maximum_cnt_in_sec)
#define COUNTER_NPCX_LCT_CHECK_TIMEOUT_US  200

#define DAY_PER_WEEK 7U
#define SEC_PER_WEEK ((DAY_PER_WEEK) * (SEC_PER_DAY))

#define NPCX_LCT_PWR_VCC1 0
#define NPCX_LCT_PWR_VSBY 1

#define NPCX_LCT_STAT_EV_MASK 0x01

struct counter_npcx_lct_config {
	struct counter_config_info info;
	struct lct_reg *reg_base;
	bool pwr_by_vsby;
#if defined(CONFIG_COUNTER_NPCX_NPCXN_V1) || defined(CONFIG_COUNTER_NPCX_NPCXN_V2)
	const struct npcx_wui lct_wui;
#endif
#if defined(CONFIG_COUNTER_NPCX_NPCKN)
	int irq;
#endif
};

struct counter_npcx_lct_data {
	struct k_sem lock;
#if defined(CONFIG_COUNTER_NPCX_NPCXN_V1) || defined(CONFIG_COUNTER_NPCX_NPCXN_V2)
	struct miwu_callback lct_wui_cb;
#endif
	counter_alarm_callback_t alarm_cb;
	void *user_data;
};

static void npcx_lct_handle_isr(const struct device *dev)
{
	const struct counter_npcx_lct_config *const config = dev->config;
	struct counter_npcx_lct_data *data = dev->data;
	struct lct_reg *const reg_base = config->reg_base;
	counter_alarm_callback_t alarm_cb = data->alarm_cb;

	/* The counter needs some time to stop. Wait for the LCT enable bit to clear, or the event
	 * status won’t clear properly.
	 */
	if (WAIT_FOR(!IS_BIT_SET(reg_base->LCTCONT, NPCX_LCTCONT_LCTEN),
		     COUNTER_NPCX_LCT_CHECK_TIMEOUT_US, NULL) == false) {
		LOG_ERR("The LCT function is still working");
	}

	reg_base->LCTCONT &= ~BIT(NPCX_LCTCONT_LCTEVEN);
	reg_base->LCTSTAT |= BIT(NPCX_LCTSTAT_LCTEVST);

	if (data->alarm_cb) {
		data->alarm_cb = NULL;
		alarm_cb(dev, 0, 0, data->user_data);
	}
}

#if defined(CONFIG_COUNTER_NPCX_NPCKN)
static void counter_npcx_lct_isr(const struct device *dev)
{
	npcx_lct_handle_isr(dev);
}
#endif
#if defined(CONFIG_COUNTER_NPCX_NPCXN_V1) || defined(CONFIG_COUNTER_NPCX_NPCXN_V2)
static void counter_npcx_lct_isr(const struct device *dev, struct npcx_wui *wui)
{
	npcx_lct_handle_isr(dev);
}
#endif

#if defined(CONFIG_COUNTER_NPCX_NPCXN_V2)
#define LCT_REG_WEEKM(base) (base->LCTWEEKM)
#else
#define LCT_REG_WEEKM(base) 0U
#endif

static int npcx_lct_enable(struct lct_reg *const reg_base, bool enable)
{
	if (enable) {
		reg_base->LCTCONT |= BIT(NPCX_LCTCONT_LCTEN);
	} else {
		reg_base->LCTCONT &= ~BIT(NPCX_LCTCONT_LCTEN);
	}

	/* The counter takes time to start and stop. Wait until the LCT enable bit is in the correct
	 * state, otherwise the hardware behavior may be incorrect.
	 */
	if (WAIT_FOR(IS_BIT_SET(reg_base->LCTCONT, NPCX_LCTCONT_LCTEN) == enable,
		     COUNTER_NPCX_LCT_CHECK_TIMEOUT_US, NULL) == false) {
		LOG_ERR("LCT enable/disable timeout");
		return -ETIMEDOUT;
	}

	return 0;
}

static void npcx_lct_set_alarm_time(struct lct_reg *const reg_base, int32_t seconds)
{
	uint32_t weeks;

	weeks = seconds / SEC_PER_WEEK;
#if defined(CONFIG_COUNTER_NPCX_NPCXN_V2)
	reg_base->LCTWEEKM = (weeks >> 8) & 0xf;
#endif
	reg_base->LCTWEEK = weeks & 0xff;
	seconds %= SEC_PER_WEEK;
	reg_base->LCTDAY = seconds / SEC_PER_DAY;
	seconds %= SEC_PER_DAY;
	reg_base->LCTHOUR = seconds / SEC_PER_HOUR;
	seconds %= SEC_PER_HOUR;
	reg_base->LCTMINUTE = seconds / SEC_PER_MIN;
	reg_base->LCTSECOND = seconds % SEC_PER_MIN;
}

static int counter_npcx_lct_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	const struct counter_npcx_lct_config *const config = dev->config;
	struct counter_npcx_lct_data *data = dev->data;
	struct lct_reg *const reg_base = config->reg_base;
	int ret = 0;

	if (chan_id != 0) {
		LOG_ERR("Invalid channel id %u", chan_id);
		return -ENOTSUP;
	}

	k_sem_take(&data->lock, K_FOREVER);

	ret = npcx_lct_enable(reg_base, false);
	if (ret != 0) {
		LOG_ERR("disable LCT failed");
		ret = -EBUSY;
		goto return_exit;
	}
	reg_base->LCTCONT &= ~BIT(NPCX_LCTCONT_LCTEVEN);

#if defined(CONFIG_COUNTER_NPCX_NPCXN_V2)
	reg_base->LCTWEEKM = 0;
#endif
	reg_base->LCTWEEK = 0;
	reg_base->LCTDAY = 0;
	reg_base->LCTHOUR = 0;
	reg_base->LCTMINUTE = 0;
	reg_base->LCTSECOND = 0;

	data->alarm_cb = NULL;
	data->user_data = NULL;

return_exit:
	k_sem_give(&data->lock);

	return ret;
}

static int counter_npcx_lct_start(const struct device *dev)
{

	const struct counter_npcx_lct_config *const config = dev->config;
	struct counter_npcx_lct_data *data = dev->data;
	struct lct_reg *const reg_base = config->reg_base;
	int ret = 0;

	k_sem_take(&data->lock, K_FOREVER);

	if (IS_BIT_SET(reg_base->LCTCONT, NPCX_LCTCONT_LCTEN)) {
		ret = -EALREADY;
		goto return_exit;
	}

	ret = npcx_lct_enable(reg_base, true);
	if (ret != 0) {
		LOG_ERR("enable LCT failed");
		ret = -EBUSY;
		goto return_exit;
	}

return_exit:
	k_sem_give(&data->lock);

	return ret;
}
static int counter_npcx_lct_stop(const struct device *dev)
{
	const struct counter_npcx_lct_config *const config = dev->config;
	struct counter_npcx_lct_data *data = dev->data;
	struct lct_reg *const reg_base = config->reg_base;
	int ret = 0;

	k_sem_take(&data->lock, K_FOREVER);

	if (!IS_BIT_SET(reg_base->LCTCONT, NPCX_LCTCONT_LCTEN)) {
		/* Already stop, nothing to do. */
		goto return_exit;
	}

	ret = npcx_lct_enable(reg_base, false);
	if (ret != 0) {
		LOG_ERR("disable LCT failed");
		ret = -EBUSY;
		goto return_exit;
	}

return_exit:
	k_sem_give(&data->lock);

	return ret;
}
static int counter_npcx_lct_get_value(const struct device *dev, uint32_t *ticks)
{
	const struct counter_npcx_lct_config *const config = dev->config;
	struct lct_reg *const reg_base = config->reg_base;
	struct counter_npcx_lct_data *data = dev->data;
	uint32_t second;
	uint8_t week, day, hour, minute;
	uint16_t weekm;

	k_sem_take(&data->lock, K_FOREVER);

	do {
		weekm = LCT_REG_WEEKM(reg_base);
		week = reg_base->LCTWEEK;
		day = reg_base->LCTDAY;
		hour = reg_base->LCTHOUR;
		minute = reg_base->LCTMINUTE;
		second = reg_base->LCTSECOND;
	} while (weekm != LCT_REG_WEEKM(reg_base) || week != reg_base->LCTWEEK ||
		 day != reg_base->LCTDAY || hour != reg_base->LCTHOUR ||
		 minute != reg_base->LCTMINUTE || second != reg_base->LCTSECOND);

	weekm = (weekm << 8) + week;

	*ticks = weekm * SEC_PER_WEEK + day * SEC_PER_DAY + hour * SEC_PER_HOUR +
		 minute * SEC_PER_MIN + second;

	k_sem_give(&data->lock);

	return 0;
}

static int counter_npcx_lct_set_alarm(const struct device *dev, uint8_t chan_id,
				      const struct counter_alarm_cfg *alarm_cfg)
{
	const struct counter_npcx_lct_config *const config = dev->config;
	struct counter_npcx_lct_data *data = dev->data;
	struct lct_reg *const reg_base = config->reg_base;
	int ret;

	if (chan_id != 0) {
		LOG_ERR("Invalid channel id %u", chan_id);
		ret = -ENOTSUP;
		goto return_exit;
	}

	/*
	 * Interrupts are only triggered when the counter reaches 0.
	 * So only relative alarms are supported.
	 */
	if (alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) {
		LOG_ERR("Invalid flags %x", alarm_cfg->flags);
		ret = -ENOTSUP;
		goto return_exit;
	}

	if (data->alarm_cb != NULL) {
		ret = -EBUSY;
		goto return_exit;
	}

	if (alarm_cfg->ticks > COUNTER_NPCX_LCT_MAX_SECOND) {
		ret = -EINVAL;
		goto return_exit;
	}

	k_sem_take(&data->lock, K_FOREVER);

	data->alarm_cb = alarm_cfg->callback;
	data->user_data = alarm_cfg->user_data;

	npcx_lct_set_alarm_time(reg_base, alarm_cfg->ticks);
	/* Clear pending LCT event */
	reg_base->LCTSTAT |= BIT(NPCX_LCTSTAT_LCTEVST);

	reg_base->LCTCONT |= BIT(NPCX_LCTCONT_LCTEVEN);

	if (config->pwr_by_vsby == true) {
		reg_base->LCTCONT |= BIT(NPCX_LCTCONT_LCTPSLEN);
	}

	ret = npcx_lct_enable(reg_base, true);

return_exit:
	k_sem_give(&data->lock);

	return ret;
}

static uint32_t counter_npcx_get_pending_int(const struct device *dev)
{
	const struct counter_npcx_lct_config *const config = dev->config;
	struct lct_reg *const reg_base = config->reg_base;

	return reg_base->LCTSTAT & NPCX_LCT_STAT_EV_MASK;
}

static const struct counter_driver_api counter_npcx_lct_api = {
	.start = counter_npcx_lct_start,
	.stop = counter_npcx_lct_stop,
	.get_value = counter_npcx_lct_get_value,
	.set_alarm = counter_npcx_lct_set_alarm,
	.cancel_alarm = counter_npcx_lct_cancel_alarm,
	.get_pending_int = counter_npcx_get_pending_int,
};

static int counter_npcx_lct_init(const struct device *dev)
{
	const struct counter_npcx_lct_config *const config = dev->config;
	struct counter_npcx_lct_data *const data = dev->data;
	struct lct_reg *const reg_base = config->reg_base;

#if defined(CONFIG_COUNTER_NPCX_NPCXN_V1) || defined(CONFIG_COUNTER_NPCX_NPCXN_V2)
	npcx_miwu_init_dev_callback(&data->lct_wui_cb, &config->lct_wui, counter_npcx_lct_isr, dev);
	npcx_miwu_manage_callback(&data->lct_wui_cb, true);
	npcx_miwu_interrupt_configure(&config->lct_wui, NPCX_MIWU_MODE_EDGE, NPCX_MIWU_TRIG_HIGH);
	/* Enable irq of t0-out expired event */
	npcx_miwu_irq_enable(&config->lct_wui);

	reg_base->LCTCONT |= BIT(NPCX_LCTCONT_LCT_CLK_EN);

#endif
#if defined(CONFIG_COUNTER_NPCX_NPCKN)
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority), counter_npcx_lct_isr,
		    DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));
#endif

	k_sem_init(&data->lock, 1, 1);

	if (config->pwr_by_vsby == true) {
		reg_base->LCTCONT |= BIT(NPCX_LCTCONT_LCT_VSBY_PWR);

	} else {
		reg_base->LCTCONT &= ~BIT(NPCX_LCTCONT_LCT_VSBY_PWR);
	}

	return 0;
}

static const struct counter_npcx_lct_config npcx_lct_cfg = {
	.info = {
			.channels = 1,
		},
	.reg_base = (struct lct_reg *)DT_INST_REG_ADDR(0),
	.pwr_by_vsby = DT_INST_PROP(0, pwr_by_vsby),
#if defined(CONFIG_COUNTER_NPCX_NPCKN)
	.irq = DT_INST_IRQN(0),
#endif
#if defined(CONFIG_COUNTER_NPCX_NPCXN_V1) || defined(CONFIG_COUNTER_NPCX_NPCXN_V2)
	.lct_wui = NPCX_DT_WUI_ITEM_BY_NAME(0, lct_wui),
#endif
};

static struct counter_npcx_lct_data npcx_lct_data;

DEVICE_DT_INST_DEFINE(0, counter_npcx_lct_init, NULL, &npcx_lct_data, &npcx_lct_cfg, POST_KERNEL,
		      CONFIG_COUNTER_INIT_PRIORITY, &counter_npcx_lct_api);
