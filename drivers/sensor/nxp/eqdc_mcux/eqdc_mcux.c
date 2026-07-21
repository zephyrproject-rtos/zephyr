/*
 * SPDX-FileCopyrightText: 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_mcux_eqdc

#include <errno.h>
#include <stdint.h>
#include <fsl_eqdc.h>
#include <fsl_inputmux.h>
#include <zephyr/drivers/sensor/eqdc_mcux.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(eqdc_mcux, CONFIG_SENSOR_LOG_LEVEL);

/* One INPUTMUX route (PHASEA / PHASEB) decoded from the inputmux-connections
 * phandle-array. The connection value is an inputmux_connection_t taken from
 * fsl_inputmux_connections.h.
 */
struct eqdc_mcux_inputmux_entry {
	INPUTMUX_Type *base;
	uint16_t channel;
	uint32_t connection;
};

struct eqdc_mcux_config {
	EQDC_Type *base;
	const struct pinctrl_dev_config *pincfg;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	bool reverse_direction;
	bool single_phase_mode;
	uint8_t count_multiplier;
	eqdc_revolution_count_condition_t revolution_count_mode;
	uint32_t prescaler;
	uint8_t input_filter_period;
	uint8_t input_filter_count;
	void (*irq_config_func)(const struct device *dev);
	const struct eqdc_mcux_inputmux_entry *inputmux_entries;
	uint8_t inputmux_entries_count;
};

struct eqdc_mcux_data {
	uint32_t clock_freq;
	uint32_t counts_per_revolution;
	int32_t position;
	int16_t position_diff;
	uint16_t position_diff_period;
	int16_t revolution_count;
#ifdef CONFIG_EQDC_MCUX_TRIGGER
	sensor_trigger_handler_t trigger_handler;
	const struct sensor_trigger *trigger;
	struct k_work work;
	const struct device *dev;
#endif
};

static int eqdc_mcux_check_channel(enum sensor_channel ch)
{
	if (ch != SENSOR_CHAN_ALL && ch != SENSOR_CHAN_ROTATION &&
		ch != SENSOR_CHAN_ENCODER_COUNT && ch != SENSOR_CHAN_RPM &&
		ch != SENSOR_CHAN_ENCODER_REVOLUTIONS) {
		return -ENOTSUP;
	}

	return 0;
}

static int eqdc_mcux_attr_set(const struct device *dev, enum sensor_channel ch,
			      enum sensor_attribute attr, const struct sensor_value *val)
{
	const struct eqdc_mcux_config *config = dev->config;
	struct eqdc_mcux_data *data = dev->data;

	if (eqdc_mcux_check_channel(ch) != 0) {
		return -ENOTSUP;
	}

	if ((enum sensor_attribute_eqdc_mcux)attr != SENSOR_ATTR_EQDC_COUNTS_PER_REVOLUTION) {
		return -ENOTSUP;
	}

	if (val->val1 < 1 || (uint32_t)val->val1 > (UINT32_MAX / config->count_multiplier)) {
		LOG_ERR("Counts per revolution value invalid: %d", val->val1);
		return -EINVAL;
	}

	/* Scale by the count multiplier: 4 for quadrature X4, 1 for single-phase */
	data->counts_per_revolution = (uint32_t)val->val1 * config->count_multiplier;
	EQDC_SetPositionModulusValue(config->base, data->counts_per_revolution - 1U);
	EQDC_SetEqdcLdok(config->base);

	return 0;
}

static int eqdc_mcux_attr_get(const struct device *dev, enum sensor_channel ch,
			      enum sensor_attribute attr, struct sensor_value *val)
{
	const struct eqdc_mcux_config *config = dev->config;
	struct eqdc_mcux_data *data = dev->data;

	if (eqdc_mcux_check_channel(ch) != 0) {
		return -ENOTSUP;
	}

	switch ((enum sensor_attribute_eqdc_mcux)attr) {
	case SENSOR_ATTR_EQDC_COUNTS_PER_REVOLUTION:
		val->val1 = data->counts_per_revolution / config->count_multiplier;
		val->val2 = 0;
		return 0;
	case SENSOR_ATTR_EQDC_PRESCALED_FREQUENCY:
		val->val1 = data->clock_freq / (1U << config->prescaler);
		val->val2 = 0;
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int eqdc_mcux_sample_fetch(const struct device *dev, enum sensor_channel ch)
{
	const struct eqdc_mcux_config *config = dev->config;
	struct eqdc_mcux_data *data = dev->data;

	if (eqdc_mcux_check_channel(ch) != 0) {
		return -ENOTSUP;
	}

	/* Dummy read to latch the POSDH and POSDPERH hold registers */
	(void)config->base->POSD;

	data->position = (int32_t)EQDC_GetPosition(config->base);

	data->position_diff = (int16_t)EQDC_GetHoldPositionDifference(config->base);

	data->position_diff_period = EQDC_GetHoldPositionDifferencePeriod(config->base);

	/* REV is a signed 16-bit counter: it decrements for reverse rotation
	 * (e.g. with reverse-direction enabled), so read it as int16_t.
	 */
	data->revolution_count = (int16_t)EQDC_GetHoldRevolution(config->base);

	LOG_DBG("Current position value: %d, position difference: %d,"
		"position difference period: %d.", data->position, data->position_diff,
		data->position_diff_period);

	return 0;
}

static int eqdc_mcux_channel_get(const struct device *dev, enum sensor_channel ch,
				 struct sensor_value *val)
{
	const struct eqdc_mcux_config *config = dev->config;
	struct eqdc_mcux_data *data = dev->data;
	int32_t counter_value;
	uint32_t prescaled_freq;
	int32_t angle;
	uint64_t rpm_calc;

	switch (ch) {
	case SENSOR_CHAN_ROTATION:
		/* Calculate angle in degrees */
		if (data->counts_per_revolution > 0) {
			counter_value = data->position % (int32_t)data->counts_per_revolution;
			/* Normalize negative modulo result to 0-360 range */
			if (counter_value < 0) {
				counter_value += data->counts_per_revolution;
			}
			/* Calculate angle in Q26.6 fixed-point format */
			angle = (counter_value * 23040) / data->counts_per_revolution;
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
			data->position_diff_period != UINT16_MAX && data->position_diff != 0) {
			/*
			 * Convert the held position difference into RPM: the
			 * counts moved over counts-per-revolution give revolutions,
			 * the prescaled clock over the difference period gives the
			 * sampling rate, and a factor of 60 converts per-second to
			 * per-minute.
			 */
			prescaled_freq = data->clock_freq / (1U << config->prescaler);

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

#ifdef CONFIG_EQDC_MCUX_TRIGGER

static void eqdc_mcux_trigger_work_handler(struct k_work *work)
{
	struct eqdc_mcux_data *data = CONTAINER_OF(work, struct eqdc_mcux_data, work);

	if (data->trigger_handler) {
		data->trigger_handler(data->dev, data->trigger);
	}
}

static int eqdc_mcux_trigger_set(const struct device *dev,
				 const struct sensor_trigger *trig,
				 sensor_trigger_handler_t handler)
{
	const struct eqdc_mcux_config *config = dev->config;
	struct eqdc_mcux_data *data = dev->data;
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
		if (config->revolution_count_mode == kEQDC_RevolutionCountOnIndexPulse) {
			int_mask = kEQDC_IndexPresetPulseInterruptEnable;
		} else {
			int_mask = kEQDC_PositionRollOverInterruptEnable |
				   kEQDC_PositionRollUnderInterruptEnable;
		}

		/* Clear any pending flags */
		EQDC_ClearStatusFlags(config->base, kEQDC_StatusAllFlags);

		/* Enable interrupts */
		EQDC_EnableInterrupts(config->base, int_mask);
	}

	return 0;
}

static void eqdc_mcux_isr(const struct device *dev)
{
	const struct eqdc_mcux_config *config = dev->config;
	struct eqdc_mcux_data *data = dev->data;
	uint32_t flags;

	flags = EQDC_GetStatusFlags(config->base);

	/* Clear interrupt flags */
	EQDC_ClearStatusFlags(config->base, flags);

	LOG_DBG("ISR flags: 0x%08x", flags);

	if (data->trigger_handler) {
		k_work_submit(&data->work);
	}
}

#endif /* CONFIG_EQDC_MCUX_TRIGGER */

static DEVICE_API(sensor, eqdc_mcux_api) = {
	.attr_set = eqdc_mcux_attr_set,
	.attr_get = eqdc_mcux_attr_get,
	.sample_fetch = eqdc_mcux_sample_fetch,
	.channel_get = eqdc_mcux_channel_get,
#ifdef CONFIG_EQDC_MCUX_TRIGGER
	.trigger_set = eqdc_mcux_trigger_set,
#endif
};

static int eqdc_mcux_init_pins(const struct device *dev)
{
	const struct eqdc_mcux_config *config = dev->config;
	int ret;

	ret = pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret != 0 && ret != -ENOENT) {
		return ret;
	}

	/* Route the EQDC phase inputs through INPUTMUX */
	for (uint8_t i = 0; i < config->inputmux_entries_count; i++) {
		const struct eqdc_mcux_inputmux_entry *entry = &config->inputmux_entries[i];

		INPUTMUX_Init(entry->base);
		INPUTMUX_AttachSignal(entry->base, entry->channel,
				      (inputmux_connection_t)entry->connection);
		INPUTMUX_Deinit(entry->base);
	}

	return 0;
}

static int eqdc_mcux_init(const struct device *dev)
{
	const struct eqdc_mcux_config *config = dev->config;
	struct eqdc_mcux_data *data = dev->data;
	eqdc_config_t eqdc_config;
	int ret;

	LOG_DBG("Initializing %s", dev->name);

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("clock control device not ready");
		return -ENODEV;
	}

	ret = clock_control_on(config->clock_dev, config->clock_subsys);
	if (ret < 0) {
		LOG_ERR("Could not enable EQDC clock (%d)", ret);
		return ret;
	}

	if (clock_control_get_rate(config->clock_dev, config->clock_subsys,
				   &data->clock_freq) || data->clock_freq == 0U) {
		LOG_ERR("Could not get clock frequency");
		return -EINVAL;
	}

	/* Initialize pins and INPUTMUX routing */
	ret = eqdc_mcux_init_pins(dev);
	if (ret != 0) {
		LOG_ERR("Failed to configure pins/INPUTMUX (%d)", ret);
		return ret;
	}

	/* Initialize EQDC peripheral */
	EQDC_GetDefaultConfig(&eqdc_config);
	if (config->single_phase_mode) {
		eqdc_config.operateMode = kEQDC_SinglePhaseDecodeOperationMode;
		eqdc_config.countMode = kEQDC_SignedCountSingleEdge;
	}
	eqdc_config.positionModulusValue = data->counts_per_revolution - 1U;
	eqdc_config.enableReverseDirection = config->reverse_direction;
	eqdc_config.revolutionCountCondition = config->revolution_count_mode;
	eqdc_config.prescaler = config->prescaler;
	eqdc_config.filterSamplePeriod = config->input_filter_period;
	eqdc_config.filterSampleCount = config->input_filter_count;
	eqdc_config.enablePeriodMeasurement = true;
	EQDC_Init(config->base, &eqdc_config);
	EQDC_DoSoftwareLoadInitialPositionValue(config->base);

#ifdef CONFIG_EQDC_MCUX_TRIGGER
	data->dev = dev;
	k_work_init(&data->work, eqdc_mcux_trigger_work_handler);
	config->irq_config_func(dev);
#endif

	return 0;
}

#define EQDC_MCUX_INPUTMUX_ENTRY(node_id, prop, idx)					\
	{										\
		.base = (INPUTMUX_Type *)DT_REG_ADDR(DT_PHANDLE_BY_IDX(node_id, prop, idx)), \
		.channel = (uint16_t)DT_PHA_BY_IDX(node_id, prop, idx, channel),	\
		.connection = (uint32_t)DT_PHA_BY_IDX(node_id, prop, idx, connection),	\
	}

#define EQDC_MCUX_INPUTMUX_DEFINE(n)							\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, inputmux_connections),			\
		    (static const struct eqdc_mcux_inputmux_entry			\
			    eqdc_mcux_inputmux_entries_##n[] = {			\
			    DT_INST_FOREACH_PROP_ELEM_SEP(n, inputmux_connections,	\
							  EQDC_MCUX_INPUTMUX_ENTRY, (,))}; \
		    ), ())

#define EQDC_MCUX_INPUTMUX_INIT(n)							\
	COND_CODE_1(DT_INST_NODE_HAS_PROP(n, inputmux_connections),			\
		    (.inputmux_entries = eqdc_mcux_inputmux_entries_##n,		\
		     .inputmux_entries_count = DT_INST_PROP_LEN(n, inputmux_connections),), \
		    ())

#ifdef CONFIG_EQDC_MCUX_TRIGGER
#define EQDC_MCUX_IRQ_CONFIG(inst)						\
	static void eqdc_mcux_irq_config_##inst(const struct device *dev)	\
	{									\
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(inst, index, irq),		\
			    DT_INST_IRQ_BY_NAME(inst, index, priority),		\
			    eqdc_mcux_isr, DEVICE_DT_INST_GET(inst), 0);	\
		irq_enable(DT_INST_IRQ_BY_NAME(inst, index, irq));		\
	}

#define EQDC_MCUX_IRQ_CONFIG_FUNC(inst) .irq_config_func = eqdc_mcux_irq_config_##inst,
#else
#define EQDC_MCUX_IRQ_CONFIG(inst)
#define EQDC_MCUX_IRQ_CONFIG_FUNC(inst)
#endif /* CONFIG_EQDC_MCUX_TRIGGER */

/* Counts scaling: quadrature decode counts 4 edges per line (X4); single-phase
 * decode counts 1 edge per pulse. Keep this in sync with the count mode set in
 * eqdc_mcux_init().
 */
#define EQDC_MCUX_MULT(inst) (DT_INST_PROP(inst, single_phase_mode) ? 1U : 4U)

#define EQDC_MCUX_DEVICE_INIT(inst)						\
										\
	EQDC_MCUX_IRQ_CONFIG(inst)						\
										\
	EQDC_MCUX_INPUTMUX_DEFINE(inst)						\
										\
	static struct eqdc_mcux_data eqdc_mcux_data_##inst = {			\
		.counts_per_revolution = DT_INST_PROP(inst, counts_per_revolution)	\
					 * EQDC_MCUX_MULT(inst),		\
	};									\
										\
	PINCTRL_DT_INST_DEFINE(inst);						\
										\
	static const struct eqdc_mcux_config eqdc_mcux_config_##inst = {	\
		.base = (EQDC_Type *)DT_INST_REG_ADDR(inst),			\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),			\
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),		\
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(inst, name),\
		.reverse_direction = DT_INST_PROP(inst, reverse_direction),	\
		.single_phase_mode = DT_INST_PROP(inst, single_phase_mode),	\
		.count_multiplier = EQDC_MCUX_MULT(inst),			\
		.revolution_count_mode = (eqdc_revolution_count_condition_t)DT_INST_PROP(	\
			inst, revolution_count_mode),	\
		.prescaler = DT_INST_PROP_OR(inst, prescaler, 0),	\
		.input_filter_period = DT_INST_PROP(inst, input_filter_period),	\
		.input_filter_count = DT_INST_PROP(inst, input_filter_count),	\
		EQDC_MCUX_INPUTMUX_INIT(inst)					\
		EQDC_MCUX_IRQ_CONFIG_FUNC(inst)					\
	};									\
										\
	SENSOR_DEVICE_DT_INST_DEFINE(inst,					\
				     eqdc_mcux_init,				\
				     NULL,					\
				     &eqdc_mcux_data_##inst,			\
				     &eqdc_mcux_config_##inst,			\
				     POST_KERNEL,				\
				     CONFIG_SENSOR_INIT_PRIORITY,		\
				     &eqdc_mcux_api);

DT_INST_FOREACH_STATUS_OKAY(EQDC_MCUX_DEVICE_INIT)
