/*
 * Copyright (c) 2018 blik GmbH
 * Copyright (c) 2018,2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_rtc

#include <zephyr/drivers/counter.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/sys_clock.h>
#include <fsl_rtc.h>
#include <zephyr/logging/log.h>

/*
 * FSL_FEATURE_* is defined with paranethesis
 * which is not acceptable when using the IS_ENABLED() macro
 */
#if (defined(FSL_FEATURE_RTC_HAS_LPO_ADJUST) && FSL_FEATURE_RTC_HAS_LPO_ADJUST)
#define NXP_RTC_HAS_LPO_ADJUST 1
#else
#define NXP_RTC_HAS_LPO_ADJUST 0
#endif

LOG_MODULE_REGISTER(mcux_rtc, CONFIG_COUNTER_LOG_LEVEL);

struct mcux_rtc_data {
	counter_alarm_callback_t alarm_callback;
	counter_top_callback_t top_callback;
	void *alarm_user_data;
	void *top_user_data;
};

struct mcux_rtc_config {
	struct counter_config_info info;
	RTC_Type *base;
	void (*irq_config_func)(const struct device *dev);
};

static int mcux_rtc_start(const struct device *dev)
{
	const struct counter_config_info *info = dev->config;
	const struct mcux_rtc_config *config =
		CONTAINER_OF(info, struct mcux_rtc_config, info);

	RTC_StartTimer(config->base);
	RTC_EnableInterrupts(config->base,
			     kRTC_AlarmInterruptEnable |
			     kRTC_TimeOverflowInterruptEnable |
			     kRTC_TimeInvalidInterruptEnable);

	return 0;
}

static int mcux_rtc_stop(const struct device *dev)
{
	const struct counter_config_info *info = dev->config;
	const struct mcux_rtc_config *config =
		CONTAINER_OF(info, struct mcux_rtc_config, info);

	RTC_DisableInterrupts(config->base,
			      kRTC_AlarmInterruptEnable |
			      kRTC_TimeOverflowInterruptEnable |
			      kRTC_TimeInvalidInterruptEnable);
	RTC_StopTimer(config->base);

	/* clear out any set alarms */
	config->base->TAR = 0;

	return 0;
}

static uint32_t mcux_rtc_read(const struct device *dev)
{
	const struct counter_config_info *info = dev->config;
	const struct mcux_rtc_config *config =
		CONTAINER_OF(info, struct mcux_rtc_config, info);

	uint32_t ticks = config->base->TSR;

	/*
	 * Read TSR seconds twice in case it glitches during an update.
	 * This can happen when a read occurs at the time the register is
	 * incrementing.
	 */
	if (config->base->TSR == ticks) {
		return ticks;
	}

	ticks = config->base->TSR;

	return ticks;
}

static int mcux_rtc_get_value(const struct device *dev, uint32_t *ticks)
{
	*ticks = mcux_rtc_read(dev);
	return 0;
}

static int mcux_rtc_set_alarm(const struct device *dev, uint8_t chan_id,
			      const struct counter_alarm_cfg *alarm_cfg)
{
	const struct counter_config_info *info = dev->config;
	const struct mcux_rtc_config *config =
		CONTAINER_OF(info, struct mcux_rtc_config, info);
	struct mcux_rtc_data *data = dev->data;

	uint32_t ticks = alarm_cfg->ticks;
	uint32_t current = mcux_rtc_read(dev);

	LOG_DBG("Current time is %d ticks", current);

	if (chan_id != 0U) {
		LOG_ERR("Invalid channel id");
		return -EINVAL;
	}

	if (data->alarm_callback != NULL) {
		return -EBUSY;
	}

	if ((alarm_cfg->flags & COUNTER_ALARM_CFG_ABSOLUTE) == 0) {
		ticks += current;
	}

	if (ticks < current) {
		LOG_ERR("Alarm cannot be earlier than current time");
		return -EINVAL;
	}

	data->alarm_callback = alarm_cfg->callback;
	data->alarm_user_data = alarm_cfg->user_data;

	config->base->TAR = ticks;
	LOG_DBG("Alarm set to %d ticks", ticks);

	return 0;
}

static int mcux_rtc_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	struct mcux_rtc_data *data = dev->data;

	if (chan_id != 0U) {
		LOG_ERR("Invalid channel id");
		return -EINVAL;
	}

	data->alarm_callback = NULL;

	return 0;
}

static int mcux_rtc_set_top_value(const struct device *dev,
				  const struct counter_top_cfg *cfg)
{
	const struct counter_config_info *info = dev->config;
	const struct mcux_rtc_config *config =
			CONTAINER_OF(info, struct mcux_rtc_config, info);
	struct mcux_rtc_data *data = dev->data;

	if (cfg->ticks != info->max_top_value) {
		LOG_ERR("Wrap can only be set to 0x%x.", info->max_top_value);
		return -ENOTSUP;
	}

	if (!(cfg->flags & COUNTER_TOP_CFG_DONT_RESET)) {
		RTC_StopTimer(config->base);
		config->base->TSR = 0;
		RTC_StartTimer(config->base);
	}

	data->top_callback = cfg->callback;
	data->top_user_data = cfg->user_data;

	return 0;
}

static uint32_t mcux_rtc_get_pending_int(const struct device *dev)
{
	const struct counter_config_info *info = dev->config;
	const struct mcux_rtc_config *config =
		CONTAINER_OF(info, struct mcux_rtc_config, info);

	return RTC_GetStatusFlags(config->base) & RTC_SR_TAF_MASK;
}

static uint32_t mcux_rtc_get_top_value(const struct device *dev)
{
	const struct counter_config_info *info = dev->config;

	return info->max_top_value;
}

static void mcux_rtc_isr(const struct device *dev)
{
	const struct counter_config_info *info = dev->config;
	const struct mcux_rtc_config *config =
		CONTAINER_OF(info, struct mcux_rtc_config, info);
	struct mcux_rtc_data *data = dev->data;
	counter_alarm_callback_t cb;
	uint32_t current = mcux_rtc_read(dev);


	LOG_DBG("Current time is %d ticks", current);

	if ((RTC_GetStatusFlags(config->base) & RTC_SR_TAF_MASK) &&
	    (data->alarm_callback)) {
		cb = data->alarm_callback;
		data->alarm_callback = NULL;
		cb(dev, 0, current, data->alarm_user_data);
	}

	if ((RTC_GetStatusFlags(config->base) & RTC_SR_TOF_MASK) &&
	    (data->top_callback)) {
		data->top_callback(dev, data->top_user_data);
	}

	/*
	 * Clear any conditions to ack the IRQ
	 *
	 * callback may have already reset the alarm flag if a new
	 * alarm value was programmed to the TAR
	 */
	RTC_StopTimer(config->base);
	if (RTC_GetStatusFlags(config->base) & RTC_SR_TAF_MASK) {
		RTC_ClearStatusFlags(config->base, kRTC_AlarmFlag);
	} else if (RTC_GetStatusFlags(config->base) & RTC_SR_TIF_MASK) {
		RTC_ClearStatusFlags(config->base, kRTC_TimeInvalidFlag);
	} else if (RTC_GetStatusFlags(config->base) & RTC_SR_TOF_MASK) {
		RTC_ClearStatusFlags(config->base, kRTC_TimeOverflowFlag);
	}
	RTC_StartTimer(config->base);
}

#if defined(CONFIG_COUNTER_CALIBRATION)

/*
 * Calibration maps the signed parts-per-billion value onto the RTC Time
 * Compensation Register (TCR). TCR[7:0] configures the number of 32.768 kHz
 * source clock cycles per second as (32768 - value), with value a
 * two's-complement -128..+127, so a positive value shortens the second and
 * speeds the counter up, matching the counter API sign convention. TCR[15:8]
 * (CIR) holds the compensation interval in seconds minus one; it is kept at 0
 * (compensate every second), giving ppb = value * 1e9 / 32768, ~30518 ppb per
 * LSB over a range of roughly -3.9M..+3.9M ppb. The register is double
 * buffered and safe to write while the counter is running; writing 0 disables
 * compensation. Compensation counts 32.768 kHz oscillator cycles, so it is
 * not supported when the prescaler runs from the LPO clock.
 */
#define MCUX_RTC_OSC_CYCLES_PER_SEC 32768
#define MCUX_RTC_PPB_PER_HZ         1000000000LL
#define MCUX_RTC_COMP_VALUE_MIN     (-128)
#define MCUX_RTC_COMP_VALUE_MAX     (127)

static int mcux_rtc_set_calibration(const struct device *dev, int32_t calibration)
{
	const struct counter_config_info *info = dev->config;
	const struct mcux_rtc_config *config = CONTAINER_OF(info, struct mcux_rtc_config, info);
	int64_t value;

	if (DT_INST_ENUM_IDX(0, clock_source) == 1) {
		return -ENOTSUP;
	}

	/*
	 * value = round(calibration * 32768 / 1e9); get_calibration() reports
	 * the programmed value so a set()/get() round-trips at the hardware
	 * resolution.
	 */
	value = ((int64_t)calibration * MCUX_RTC_OSC_CYCLES_PER_SEC +
		 ((calibration >= 0 ? MCUX_RTC_PPB_PER_HZ : -MCUX_RTC_PPB_PER_HZ) / 2)) /
		MCUX_RTC_PPB_PER_HZ;

	if (value < MCUX_RTC_COMP_VALUE_MIN || value > MCUX_RTC_COMP_VALUE_MAX) {
		return -EINVAL;
	}

	config->base->TCR = RTC_TCR_CIR(0U) | RTC_TCR_TCR((uint8_t)value);

	return 0;
}

static int mcux_rtc_get_calibration(const struct device *dev, int32_t *calibration)
{
	const struct counter_config_info *info = dev->config;
	const struct mcux_rtc_config *config = CONTAINER_OF(info, struct mcux_rtc_config, info);
	uint32_t reg;
	int8_t value;
	uint8_t interval;

	if (DT_INST_ENUM_IDX(0, clock_source) == 1) {
		return -ENOTSUP;
	}

	reg = config->base->TCR;

	/* Read back the programmed TCR/CIR fields, not the read-only TCV/CIC. */
	value = (int8_t)((reg & RTC_TCR_TCR_MASK) >> RTC_TCR_TCR_SHIFT);
	interval = (uint8_t)((reg & RTC_TCR_CIR_MASK) >> RTC_TCR_CIR_SHIFT);

	*calibration = (int32_t)(((int64_t)value * MCUX_RTC_PPB_PER_HZ) /
				 ((int64_t)MCUX_RTC_OSC_CYCLES_PER_SEC * ((int64_t)interval + 1)));

	return 0;
}

#endif /* CONFIG_COUNTER_CALIBRATION */

static int mcux_rtc_init(const struct device *dev)
{
	const struct counter_config_info *info = dev->config;
	const struct mcux_rtc_config *config =
		CONTAINER_OF(info, struct mcux_rtc_config, info);
	rtc_config_t rtc_config;

	RTC_GetDefaultConfig(&rtc_config);
	RTC_Init(config->base, &rtc_config);

	/* DT_ENUM_IDX(DT_NODELABEL(rtc), clock_source):
	 * "RTC": 0
	 * "LPO": 1
	 */
	BUILD_ASSERT((((DT_INST_ENUM_IDX(0, clock_source) == 1) &&
		NXP_RTC_HAS_LPO_ADJUST) ||
		DT_INST_ENUM_IDX(0, clock_source) == 0),
		"Cannot choose the LPO clock for that instance of the RTC");

#if (defined(FSL_FEATURE_RTC_HAS_LPO_ADJUST) && FSL_FEATURE_RTC_HAS_LPO_ADJUST)
	/* The RTC prescaler increments using the LPO 1 kHz clock
	 * instead of the RTC clock
	 */
	RTC_EnableLPOClock(config->base, DT_INST_ENUM_IDX(0, clock_source));
#endif

#if !(defined(FSL_FEATURE_RTC_HAS_NO_CR_OSCE) && FSL_FEATURE_RTC_HAS_NO_CR_OSCE)
	/* Enable 32kHz oscillator and wait for 1ms to settle */
	RTC_SetClockSource(config->base);
	k_busy_wait(USEC_PER_MSEC);
#endif /* !FSL_FEATURE_RTC_HAS_NO_CR_OSCE */

	config->irq_config_func(dev);

	return 0;
}

static DEVICE_API(counter, mcux_rtc_driver_api) = {
	.start = mcux_rtc_start,
	.stop = mcux_rtc_stop,
	.get_value = mcux_rtc_get_value,
	.set_alarm = mcux_rtc_set_alarm,
	.cancel_alarm = mcux_rtc_cancel_alarm,
	.set_top_value = mcux_rtc_set_top_value,
	.get_pending_int = mcux_rtc_get_pending_int,
	.get_top_value = mcux_rtc_get_top_value,
#if defined(CONFIG_COUNTER_CALIBRATION)
	.set_calibration = mcux_rtc_set_calibration,
	.get_calibration = mcux_rtc_get_calibration,
#endif /* CONFIG_COUNTER_CALIBRATION */
};

static struct mcux_rtc_data mcux_rtc_data_0;

static void mcux_rtc_irq_config_0(const struct device *dev);

static struct mcux_rtc_config mcux_rtc_config_0 = {
	.base = (RTC_Type *)DT_INST_REG_ADDR(0),
	.irq_config_func = mcux_rtc_irq_config_0,
	.info = {
		.max_top_value = UINT32_MAX,
		.freq = DT_INST_PROP(0, clock_frequency) /
				DT_INST_PROP(0, prescaler),
		.flags = COUNTER_CONFIG_INFO_COUNT_UP,
		.channels = 1,
	},
};

DEVICE_DT_INST_DEFINE(0, &mcux_rtc_init, NULL,
		    &mcux_rtc_data_0, &mcux_rtc_config_0.info,
		    POST_KERNEL, CONFIG_COUNTER_INIT_PRIORITY,
		    &mcux_rtc_driver_api);

static void mcux_rtc_irq_config_0(const struct device *dev)
{
	IRQ_CONNECT(DT_INST_IRQN(0),
		    DT_INST_IRQ(0, priority),
		    mcux_rtc_isr, DEVICE_DT_INST_GET(0), 0);
	irq_enable(DT_INST_IRQN(0));
}
