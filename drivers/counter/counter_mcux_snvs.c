/*
 * Copyright (c) 2021 Basalte bv
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_imx_snvs_rtc
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(mcux_snvs, CONFIG_COUNTER_LOG_LEVEL);

#if CONFIG_COUNTER_MCUX_SNVS_SRTC
#define MCUX_SNVS_SRTC
#define MCUX_SNVS_NUM_CHANNELS 2
#else
#define MCUX_SNVS_NUM_CHANNELS 1
#endif

#include <zephyr/drivers/counter.h>
#include <zephyr/irq.h>
#include <fsl_snvs_hp.h>

#ifdef MCUX_SNVS_SRTC
#include <fsl_snvs_lp.h>
#endif

struct mcux_snvs_config {
	/* info must be first element */
	struct counter_config_info info;
	SNVS_Type *base;
	void (*irq_config_func)(const struct device *dev);
};

struct mcux_snvs_data {
	counter_alarm_callback_t alarm_hp_rtc_callback;
	void *alarm_hp_rtc_user_data;
#ifdef MCUX_SNVS_SRTC
	counter_alarm_callback_t alarm_lp_srtc_callback;
	void *alarm_lp_srtc_user_data;
#endif
};

static int mcux_snvs_start(const struct device *dev)
{
	ARG_UNUSED(dev);

	return -EALREADY;
}

static int mcux_snvs_stop(const struct device *dev)
{
	ARG_UNUSED(dev);

	return -ENOTSUP;
}

static int mcux_snvs_get_value(const struct device *dev, uint32_t *ticks)
{
	const struct mcux_snvs_config *config = dev->config;
	uint32_t tmp = 0;

	do {
		*ticks = tmp;
		tmp = (config->base->HPRTCMR << 17U);
		tmp |= (config->base->HPRTCLR >> 15U);
	} while (*ticks != tmp);

	return 0;
}

static int mcux_snvs_set_alarm(const struct device *dev,
			      uint8_t chan_id,
			      const struct counter_alarm_cfg *alarm_cfg)
{
	const struct mcux_snvs_config *config = dev->config;
	struct mcux_snvs_data *data = dev->data;

	uint32_t current, ticks;

	mcux_snvs_get_value(dev, &current);
	ticks = alarm_cfg->ticks;

	if ((alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) == 0) {
		ticks += current;
	}

	if (ticks < current) {
		LOG_ERR("Invalid alarm ticks");
		return -EINVAL;
	}

	if (chan_id == 0) {
		if (data->alarm_hp_rtc_callback) {
			return -EBUSY;
		}
		data->alarm_hp_rtc_callback = alarm_cfg->callback;
		data->alarm_hp_rtc_user_data = alarm_cfg->user_data;

		/* disable RTC alarm interrupt */
		config->base->HPCR &= ~SNVS_HPCR_HPTA_EN_MASK;
		while ((config->base->HPCR & SNVS_HPCR_HPTA_EN_MASK) != 0U) {
		}

		/* Set alarm in seconds*/
		config->base->HPTAMR = (uint32_t)(ticks >> 17U);
		config->base->HPTALR = (uint32_t)(ticks << 15U);

		/* enable RTC alarm interrupt */
		config->base->HPCR |= SNVS_HPCR_HPTA_EN_MASK;
#ifdef MCUX_SNVS_SRTC
	} else if (chan_id == 1) {
		if (data->alarm_lp_srtc_callback) {
			return -EBUSY;
		}
		data->alarm_lp_srtc_callback = alarm_cfg->callback;
		data->alarm_lp_srtc_user_data = alarm_cfg->user_data;

		/* disable SRTC alarm interrupt */
		config->base->LPCR &= ~SNVS_LPCR_LPTA_EN_MASK;
		while ((config->base->LPCR & SNVS_LPCR_LPTA_EN_MASK) != 0U) {
		}

		/* Set alarm in seconds*/
		config->base->LPTAR = ticks;

		/* enable SRTC alarm interrupt */
		config->base->LPCR |= SNVS_LPCR_LPTA_EN_MASK;
#endif
	} else {
		LOG_ERR("Invalid channel id");
		return -EINVAL;
	}

	return 0;
}

static int mcux_snvs_cancel_alarm(const struct device *dev,
				 uint8_t chan_id)
{
	const struct mcux_snvs_config *config = dev->config;
	struct mcux_snvs_data *data = dev->data;

	if (chan_id == 0) {
		/* disable RTC alarm interrupt */
		config->base->HPCR &= ~SNVS_HPCR_HPTA_EN_MASK;
		while ((config->base->HPCR & SNVS_HPCR_HPTA_EN_MASK) != 0U) {
		}

		/* clear callback */
		data->alarm_hp_rtc_callback = NULL;

#ifdef MCUX_SNVS_SRTC
	} else if (chan_id == 1) {
		/* disable SRTC alarm interrupt */
		config->base->LPCR &= ~SNVS_LPCR_LPTA_EN_MASK;
		while ((config->base->LPCR & SNVS_LPCR_LPTA_EN_MASK) != 0U) {
		}

		/* clear callback */
		data->alarm_lp_srtc_callback = NULL;
#endif
	} else {
		LOG_ERR("Invalid channel id");
		return -EINVAL;
	}

	return 0;
}

static int mcux_snvs_set_top_value(const struct device *dev,
				  const struct counter_top_cfg *cfg)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cfg);

	return -ENOTSUP;
}

static uint32_t mcux_snvs_get_pending_int(const struct device *dev)
{
	const struct mcux_snvs_config *config = dev->config;
	uint32_t flags;

	flags = SNVS_HP_RTC_GetStatusFlags(config->base) & kSNVS_RTC_AlarmInterruptFlag;

#ifdef MCUX_SNVS_SRTC
	flags |= SNVS_LP_SRTC_GetStatusFlags(config->base) & kSNVS_SRTC_AlarmInterruptFlag;
#endif

	return flags;
}

static uint32_t mcux_snvs_get_top_value(const struct device *dev)
{
	ARG_UNUSED(dev);

	return UINT32_MAX;
}

void mcux_snvs_isr(const struct device *dev)
{
	const struct mcux_snvs_config *config = dev->config;
	struct mcux_snvs_data *data = dev->data;

	uint32_t current;

	mcux_snvs_get_value(dev, &current);

	if (SNVS_HP_RTC_GetStatusFlags(config->base) & kSNVS_RTC_AlarmInterruptFlag) {
		/* Clear alarm flag */
		SNVS_HP_RTC_ClearStatusFlags(config->base, kSNVS_RTC_AlarmInterruptFlag);

		if (data->alarm_hp_rtc_callback) {
			data->alarm_hp_rtc_callback(dev, 0, current, data->alarm_hp_rtc_user_data);

			mcux_snvs_cancel_alarm(dev, 0);
		}
	}

#ifdef MCUX_SNVS_SRTC
	if (SNVS_LP_SRTC_GetStatusFlags(config->base) & kSNVS_SRTC_AlarmInterruptFlag) {
		/* Clear alarm flag */
		SNVS_LP_SRTC_ClearStatusFlags(config->base, kSNVS_SRTC_AlarmInterruptFlag);

		if (data->alarm_lp_srtc_callback) {
			data->alarm_lp_srtc_callback(dev, 1, current,
						     data->alarm_lp_srtc_user_data);
			mcux_snvs_cancel_alarm(dev, 1);
		}
	}
#endif
}

int mcux_snvs_rtc_set(const struct device *dev, uint32_t ticks)
{
	const struct mcux_snvs_config *config = dev->config;

#ifdef MCUX_SNVS_SRTC
	SNVS_LP_SRTC_StopTimer(config->base);

	config->base->LPSRTCMR = (uint32_t)(ticks >> 17U);
	config->base->LPSRTCLR = (uint32_t)(ticks << 15U);

	SNVS_LP_SRTC_StartTimer(config->base);
	/* Sync to our high power RTC */
	SNVS_HP_RTC_TimeSynchronize(config->base);
#else
	SNVS_HP_RTC_StopTimer(config->base);

	config->base->HPRTCMR = (uint32_t)(ticks >> 17U);
	config->base->HPRTCLR = (uint32_t)(ticks << 15U);

	SNVS_HP_RTC_StartTimer(config->base);
#endif

	return 0;
}

static int mcux_snvs_init(const struct device *dev)
{
	const struct mcux_snvs_config *config = dev->config;

	snvs_hp_rtc_config_t hp_rtc_config;

#ifdef MCUX_SNVS_SRTC
	snvs_lp_srtc_config_t lp_srtc_config;
#endif

	SNVS_HP_RTC_GetDefaultConfig(&hp_rtc_config);
	SNVS_HP_RTC_Init(config->base, &hp_rtc_config);

#ifdef MCUX_SNVS_SRTC
	/* Reset power glitch detector */
	SNVS_LP_Init(config->base);
	/* Init SRTC to default config */
	SNVS_LP_SRTC_GetDefaultConfig(&lp_srtc_config);
	SNVS_LP_SRTC_Init(config->base, &lp_srtc_config);

#if CONFIG_COUNTER_MCUX_SNVS_SRTC_WAKE
	config->base->LPCR |= SNVS_LPCR_LPWUI_EN_MASK;
#endif

	/* RTC should always run */
	SNVS_LP_SRTC_StartTimer(config->base);
	SNVS_HP_RTC_TimeSynchronize(config->base);
#endif

	/* RTC should always run */
	SNVS_HP_RTC_StartTimer(config->base);

	config->irq_config_func(dev);

	return 0;
}

static const struct counter_driver_api mcux_snvs_driver_api = {
	.start = mcux_snvs_start,
	.stop = mcux_snvs_stop,
	.get_value = mcux_snvs_get_value,
	.set_alarm = mcux_snvs_set_alarm,
	.cancel_alarm = mcux_snvs_cancel_alarm,
	.set_top_value = mcux_snvs_set_top_value,
	.get_pending_int = mcux_snvs_get_pending_int,
	.get_top_value = mcux_snvs_get_top_value,
};

/*
 * This driver is single-instance. If the devicetree contains multiple
 * instances, this will fail and the driver needs to be revisited.
 */
BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) <= 1,
	     "unsupported snvs instance");

#if DT_NODE_HAS_STATUS_OKAY(DT_DRV_INST(0))
static struct mcux_snvs_data mcux_snvs_data_0;

static void mcux_snvs_irq_config_0(const struct device *dev);

static struct mcux_snvs_config mcux_snvs_config_0 = {
	.info = {
		.max_top_value = 0,
		.freq = 1,
		.channels = MCUX_SNVS_NUM_CHANNELS,
		.flags = COUNTER_CONFIG_INFO_COUNT_UP,
	},
	.base = (SNVS_Type *)DT_REG_ADDR(DT_INST_PARENT(0)),
	.irq_config_func = mcux_snvs_irq_config_0,
};

DEVICE_DT_INST_DEFINE(0, &mcux_snvs_init, NULL,
		      &mcux_snvs_data_0,
		      &mcux_snvs_config_0,
		      POST_KERNEL, CONFIG_COUNTER_INIT_PRIORITY,
		      &mcux_snvs_driver_api);

static void mcux_snvs_irq_config_0(const struct device *dev)
{
	IRQ_CONNECT(DT_INST_IRQN(0), DT_INST_IRQ(0, priority),
		    mcux_snvs_isr, DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));
}
#endif  /* DT_NODE_HAS_STATUS_OKAY(DT_DRV_INST(0)) */
