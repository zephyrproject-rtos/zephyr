/*
 * Copyright (c) 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_mcux_eqdc

#include <errno.h>
#include <stdint.h>
#include <fsl_eqdc.h>
#include <zephyr/drivers/sensor/mcux_eqdc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/misc/nxp_xbar/nxp_xbar.h>

LOG_MODULE_REGISTER(mcux_eqdc, CONFIG_SENSOR_LOG_LEVEL);

struct mcux_eqdc_config {
	EQDC_Type *base;
	const struct pinctrl_dev_config *pincfg;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	bool reverse_direction;
	eqdc_revolution_count_condition_t revolution_count_mode;
	uint32_t prescaler;
	uint8_t input_filter_period;
	uint8_t input_filter_count;
	void (*irq_config_func)(const struct device *dev);
#if XBAR_AVAILABLE
	const char *xbar_compat;
	uintptr_t xbar_base;  /* Unified: store as generic pointer */
	size_t xbar_maps_len;
	int xbar_maps[];
#endif
};

struct mcux_eqdc_data {
	uint32_t clock_freq;
	uint32_t counts_per_revolution;
	int32_t position;
	int16_t position_diff;
	uint16_t position_diff_period;
	uint16_t revolution_count;
#ifdef CONFIG_MCUX_EQDC_TRIGGER
	sensor_trigger_handler_t trigger_handler;
	const struct sensor_trigger *trigger;
	struct k_work work;
	const struct device *dev;
#endif
};

static int mcux_eqdc_check_channel(enum sensor_channel ch)
{
	if (ch != SENSOR_CHAN_ALL && ch != SENSOR_CHAN_ROTATION &&
		ch != SENSOR_CHAN_ENCODER_COUNT && ch != SENSOR_CHAN_RPM &&
		ch != SENSOR_CHAN_ENCODER_REVOLUTIONS) {
		return -ENOTSUP;
	}

	return 0;
}

static int mcux_eqdc_attr_set(const struct device *dev, enum sensor_channel ch,
			      enum sensor_attribute attr, const struct sensor_value *val)
{
	const struct mcux_eqdc_config *config = dev->config;
	struct mcux_eqdc_data *data = dev->data;

	if (mcux_eqdc_check_channel(ch) != 0) {
		return -ENOTSUP;
	}

	switch ((enum sensor_attribute_mcux_eqdc)attr) {
	case SENSOR_ATTR_EQDC_COUNTS_PER_REVOLUTION:
		if (!IN_RANGE(val->val1, 1, UINT32_MAX)) {
			LOG_ERR("Counts per revolution value invalid: %d", val->val1);
			return -EINVAL;
		}
		/* Transfer counts because it works on Quadrature X4 mode*/
		data->counts_per_revolution = val->val1 * 4U;
		EQDC_SetPositionModulusValue(config->base, data->counts_per_revolution - 1U);
		return 0;

	default:
		return -ENOTSUP;
	}
}

static int mcux_eqdc_attr_get(const struct device *dev, enum sensor_channel ch,
			      enum sensor_attribute attr, struct sensor_value *val)
{
	const struct mcux_eqdc_config *config = dev->config;
	struct mcux_eqdc_data *data = dev->data;

	if (mcux_eqdc_check_channel(ch) != 0) {
		return -ENOTSUP;
	}

	switch ((enum sensor_attribute_mcux_eqdc)attr) {
	case SENSOR_ATTR_EQDC_COUNTS_PER_REVOLUTION:
		val->val1 = data->counts_per_revolution / 4U;
		val->val2 = 0;
		return 0;
	case SENSOR_ATTR_EQDC_PRESCALED_FREQUENCY:
		val->val1 = data->clock_freq / (1 << config->prescaler);
		val->val2 = 0;
	default:
		return -ENOTSUP;
	}
}

static int mcux_eqdc_sample_fetch(const struct device *dev, enum sensor_channel ch)
{
	const struct mcux_eqdc_config *config = dev->config;
	struct mcux_eqdc_data *data = dev->data;

	if (mcux_eqdc_check_channel(ch) != 0) {
		return -ENOTSUP;
	}

	/* Dummy read to update POSDH and POSDPERH register*/
	__attribute__((unused)) volatile uint16_t temp = config->base->POSD;

	data->position = (int32_t)EQDC_GetPosition(config->base);

	data->position_diff = (int16_t)EQDC_GetHoldPositionDifference(config->base);

	data->position_diff_period = EQDC_GetHoldPositionDifferencePeriod(config->base);

	temp = EQDC_GetHoldRevolution(config->base);

	/* Roll-over/roll-under detection starts immediately when the position
	 * counter (POS) crosses the boundary, Revolution Counter (REV) becomes 1
	 * immediately after the first count, hence minus 1 here.
	 */
	data->revolution_count = temp == 0 ? 0 : (temp - 1);

	LOG_DBG("Current position value: %d, position difference: %d,"
		"position difference period: %d.", data->position, data->position_diff,
		data->position_diff_period);

	return 0;
}

static int mcux_eqdc_channel_get(const struct device *dev, enum sensor_channel ch,
				 struct sensor_value *val)
{
	const struct mcux_eqdc_config *config = dev->config;
	struct mcux_eqdc_data *data = dev->data;
	uint32_t counter_value;
	uint32_t prescaled_freq;
	int32_t angle;
	uint64_t rpm_calc;

	switch (ch) {
	case SENSOR_CHAN_ROTATION:
		/* Calculate angle in degrees */
		if (data->counts_per_revolution > 0) {
			counter_value = data->position % data->counts_per_revolution;
			/* Normalize negative modulo result to 0-360° range */
			if (counter_value < 0) {
				counter_value += data->counts_per_revolution;
			}
			/* Calculte angle in Q26.6 fixed-point format */
			angle =  (counter_value * 23040) / data->counts_per_revolution;
			val->val1 = angle >> 6;
			val->val2 = (angle & 0x3F) * 15625;
		} else {
			val->val1 = 0;
			val->val2 = 0;
		}
		break;
	case SENSOR_CHAN_RPM:
		/* Calculate revolutions per minute, in RPM */
		if (data->counts_per_revolution > 0 && data->position_diff_period > 0 &&
			data->position_diff_period != 0xFFFF && data->position_diff != 0) {
			/*
			 * RPM = (position_diff / counts_per_revolution) *
			 *       (prescaled_freq / position_diff_period) * 60
			 */
			prescaled_freq = data->clock_freq / (1 << config->prescaler);

			/* Calculate RPM using 64-bit arithmetic to avoid overflow */
			rpm_calc = ((uint64_t)abs(data->position_diff)) * prescaled_freq * 60;
			rpm_calc = rpm_calc / ((uint64_t)data->position_diff_period *
								data->counts_per_revolution);

			/* Handle direction (negative position_diff means reverse) */
			val->val1 = (data->position_diff < 0) ? -(int32_t)rpm_calc :
				(int32_t)rpm_calc;
			val->val2 = 0;
		} else {
			val->val1 = 0;
			val->val2 = 0;
		}
		break;
	case SENSOR_CHAN_ENCODER_REVOLUTIONS:
		val->val1 = data->revolution_count;
		val->val2 = 0;
		break;
	case SENSOR_CHAN_ENCODER_COUNT:
		val->val1 = data->position;
		val->val2 = 0;
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}

#ifdef CONFIG_MCUX_EQDC_TRIGGER

static void mcux_eqdc_trigger_work_handler(struct k_work *work)
{
	struct mcux_eqdc_data *data = CONTAINER_OF(work, struct mcux_eqdc_data, work);

	if (data->trigger_handler) {
		data->trigger_handler(data->dev, data->trigger);
	}
}

static int mcux_eqdc_trigger_set(const struct device *dev,
				 const struct sensor_trigger *trig,
				 sensor_trigger_handler_t handler)
{
	const struct mcux_eqdc_config *config = dev->config;
	struct mcux_eqdc_data *data = dev->data;
	uint32_t int_mask = 0;

	if (trig->type != SENSOR_TRIG_OVERFLOW) {
		return -ENOTSUP;
	}

	if (trig->chan != SENSOR_CHAN_ENCODER_REVOLUTIONS && trig->chan != SENSOR_CHAN_ALL) {
		return -ENOTSUP;
	}

	/* Disable interrupts while configuring */
	EQDC_DisableInterrupts(config->base, kEQDC_AllInterruptEnable);

	data->trigger_handler = handler;
	data->trigger = trig;

	if (handler) {
		/* Enable desired interrupts - example: position compare or roll over/under */
		int_mask = kEQDC_PositionRollOverInterruptEnable |
			   kEQDC_PositionRollUnderInterruptEnable;

		/* Clear any pending flags */
		EQDC_ClearStatusFlags(config->base, kEQDC_StatusAllFlags);

		/* Enable interrupts */
		EQDC_EnableInterrupts(config->base, int_mask);
	}

	return 0;
}

static void mcux_eqdc_isr(const struct device *dev)
{
	const struct mcux_eqdc_config *config = dev->config;
	struct mcux_eqdc_data *data = dev->data;
	uint32_t flags;

	flags = EQDC_GetStatusFlags(config->base);

	/* Clear interrupt flags */
	EQDC_ClearStatusFlags(config->base, flags);

	LOG_DBG("ISR flags: 0x%08x", flags);

	if (data->trigger_handler) {
		k_work_submit(&data->work);
	}
}

#endif /* CONFIG_MCUX_EQDC_TRIGGER */

static const struct DEVICE_API mcux_eqdc_api = {
	.attr_set = mcux_eqdc_attr_set,
	.attr_get = mcux_eqdc_attr_get,
	.sample_fetch = mcux_eqdc_sample_fetch,
	.channel_get = mcux_eqdc_channel_get,
#ifdef CONFIG_MCUX_EQDC_TRIGGER
	.trigger_set = mcux_eqdc_trigger_set,
#endif
};

static void mcux_eqdc_init_pins(const struct device *dev)
{
	const struct mcux_eqdc_config *config = dev->config;
	int ret;

	ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	__ASSERT(ret == 0, "Failed to apply pinctrl state");

#if XBAR_AVAILABLE
	if (config->xbar_base != 0) {
		/* Initialize XBAR using unified API */
		XBAR_INIT(config->xbar_compat, config->xbar_base);

		/* Connect signals in pairs: [input, output, input, output, ...] */
		for (size_t i = 0; i < config->xbar_maps_len; i += 2) {
			XBAR_CONNECT(config->xbar_compat, config->xbar_base,
				config->xbar_maps[i], config->xbar_maps[i + 1]);
		}
	}
#endif
}

static int mcux_eqdc_init(const struct device *dev)
{
	const struct mcux_eqdc_config *config = dev->config;
	struct mcux_eqdc_data *data = dev->data;
	eqdc_config_t eqdc_config;

	LOG_DBG("Initializing %s", dev->name);

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}
	if (clock_control_get_rate(config->clock_dev, config->clock_subsys,
				   &data->clock_freq)) {
		LOG_ERR("Could not get clock frequency");
		return -EINVAL;
	}

	/* Initialize pins and crossbar */
	mcux_eqdc_init_pins(dev);

	/* Initialize EQDC peripheral */
	EQDC_GetDefaultConfig(&eqdc_config);
	eqdc_config.positionModulusValue = data->counts_per_revolution - 1U;
	eqdc_config.enableReverseDirection = config->reverse_direction;
	eqdc_config.revolutionCountCondition = config->revolution_count_mode;
	eqdc_config.prescaler = config->prescaler;
	eqdc_config.filterSamplePeriod = config->input_filter_period;
	eqdc_config.filterSampleCount = config->input_filter_count;
	eqdc_config.enablePeriodMeasurement = true;
	EQDC_Init(config->base, &eqdc_config);

	EQDC_SetOperateMode(config->base, kEQDC_QuadratureDecodeOperationMode);
	EQDC_DoSoftwareLoadInitialPositionValue(config->base);

#ifdef CONFIG_MCUX_EQDC_TRIGGER
	data->dev = dev;
	k_work_init(&data->work, mcux_eqdc_trigger_work_handler);
	config->irq_config_func(dev);
#endif

	return 0;
}

/* Devicetree helper macros */

#if XBAR_AVAILABLE
#define EQDC_XBAR_INIT(inst) \
	.xbar_compat = XBAR_COMPAT_STR(inst, xbar), \
	.xbar_base = XBAR_BASE(inst, xbar), \
	.xbar_maps = XBAR_MAPS(inst, xbar), \
	.xbar_maps_len = XBAR_MAPS_LEN(inst, xbar),

#define CHECK_XBAR_MAP_LENGTH(inst)	\
	BUILD_ASSERT((XBAR_MAPS_LEN(inst, xbar) > 0) &&	\
		((XBAR_MAPS_LEN(inst, xbar) % 2) == 0),	\
			"xbar_maps length must be an even number");
#else
#define EQDC_XBAR_INIT(inst)
#define CHECK_XBAR_MAP_LENGTH(n)
#endif /* XBAR_AVAILABLE */

#ifdef CONFIG_MCUX_EQDC_TRIGGER
#define MCUX_EQDC_IRQ_CONFIG(inst)						\
	static void mcux_eqdc_irq_config_##inst(const struct device *dev)	\
	{									\
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority),	\
			    mcux_eqdc_isr, DEVICE_DT_INST_GET(inst), 0);	\
		irq_enable(DT_INST_IRQN(inst));					\
	}

#define MCUX_EQDC_IRQ_CONFIG_FUNC(inst) .irq_config_func = mcux_eqdc_irq_config_##inst,
#else
#define MCUX_EQDC_IRQ_CONFIG(inst)
#define MCUX_EQDC_IRQ_CONFIG_FUNC(inst)
#endif /* CONFIG_MCUX_EQDC_TRIGGER */

#define MCUX_EQDC_DEVICE_INIT(inst)						\
										\
	CHECK_XBAR_MAP_LENGTH(n)			\
	MCUX_EQDC_IRQ_CONFIG(inst)						\
										\
	static struct mcux_eqdc_data mcux_eqdc_data_##inst = {			\
							\
		.counts_per_revolution = DT_INST_PROP(inst, counts_per_revolution) * 4U,\
	};									\
										\
	PINCTRL_DT_INST_DEFINE(inst);						\
										\
	static const struct mcux_eqdc_config mcux_eqdc_config_##inst = {	\
		.base = (EQDC_Type *)DT_INST_REG_ADDR(inst),			\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),			\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),		\
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(inst, name),\
		.reverse_direction = DT_INST_PROP(inst, reverse_direction),	\
		.revolution_count_mode = (eqdc_revolution_count_condition_t)DT_INST_PROP_OR(	\
			inst, revolution_count_mode, 0),	\
		.prescaler = DT_INST_PROP_OR(inst, prescaler, 0),	\
		.input_filter_period = DT_INST_PROP_OR(inst, input_filter_period, 0),	\
		.input_filter_count = DT_INST_PROP_OR(inst, input_filter_count, 0),	\
		EQDC_XBAR_INIT(inst)						\
		MCUX_EQDC_IRQ_CONFIG_FUNC(inst)					\
	};									\
										\
	SENSOR_DEVICE_DT_INST_DEFINE(inst,					\
				     mcux_eqdc_init,				\
				     NULL,					\
				     &mcux_eqdc_data_##inst,			\
				     &mcux_eqdc_config_##inst,			\
				     POST_KERNEL,				\
				     CONFIG_SENSOR_INIT_PRIORITY,		\
				     &mcux_eqdc_api);

DT_INST_FOREACH_STATUS_OKAY(MCUX_EQDC_DEVICE_INIT)
