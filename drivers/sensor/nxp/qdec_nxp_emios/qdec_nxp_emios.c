/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_emios_qdec

#include <limits.h>

#include <nxp_lcu.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/mux.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <fsl_emios.h>

LOG_MODULE_REGISTER(qdec_nxp_emios, CONFIG_SENSOR_LOG_LEVEL);

/* MCB up mode counts 1..An inclusive, then returns to 1 (RM 60.6.3.15.4). */
#define QDEC_NXP_EMIOS_MODULUS UINT16_MAX

struct qdec_nxp_emios_route {
	const struct device *dev;
	const struct mux_state *state;
};

struct qdec_nxp_emios_config {
	EMIOS_Type *base;
	const struct device *clock;
	clock_control_subsys_t clock_id;
	const struct device *lcu;
	const struct pinctrl_dev_config *pinctrl;
	const struct qdec_nxp_emios_route *routes;
	size_t route_count;
	void (*irq_config)(void);
	uint32_t counts_per_rev;
	uint16_t feedback_filter;
	uint16_t pulse_filter;
	uint8_t channels[2];
	uint8_t inputs[2];
	uint8_t cell;
	bool reverse;
};

struct qdec_nxp_emios_sample {
	int64_t count;
	int64_t previous[2];
};

struct qdec_nxp_emios_data {
	struct k_spinlock lock;
	uint64_t periods[2];
	struct qdec_nxp_emios_sample sample;
	int error;
	bool fetched;
};

static int qdec_nxp_emios_extend_count(uint64_t periods, uint16_t counter, int64_t *count)
{
	uint64_t offset;

	if (counter == 0U) {
		return -EINVAL;
	}
	offset = counter - 1U;
	if (periods > ((uint64_t)INT64_MAX - offset) / QDEC_NXP_EMIOS_MODULUS) {
		return -ERANGE;
	}
	*count = (int64_t)(periods * QDEC_NXP_EMIOS_MODULUS + offset);
	return 0;
}

static int qdec_nxp_emios_update(struct qdec_nxp_emios_sample *sample,
			       int64_t cw, int64_t ccw, bool reverse)
{
	int64_t delta[2];
	int64_t movement;

	if (cw < 0 || ccw < 0) {
		return -EINVAL;
	}
	if (cw < sample->previous[0] || ccw < sample->previous[1]) {
		return -EOVERFLOW;
	}
	delta[0] = cw - sample->previous[0];
	delta[1] = ccw - sample->previous[1];
	movement = reverse ? delta[1] - delta[0] : delta[0] - delta[1];
	if ((movement > 0 && sample->count > INT64_MAX - movement) ||
	    (movement < 0 && sample->count < INT64_MIN - movement)) {
		return -ERANGE;
	}
	sample->count += movement;
	sample->previous[0] = cw;
	sample->previous[1] = ccw;
	return 0;
}

static int qdec_nxp_emios_value(int64_t count, uint32_t counts_per_rev,
			      enum sensor_channel channel, struct sensor_value *value)
{
	int64_t result;
	int64_t remainder;

	if (counts_per_rev == 0U) {
		return -EINVAL;
	}
	switch (channel) {
	case SENSOR_CHAN_ENCODER_COUNT:
		result = count;
		break;
	case SENSOR_CHAN_ENCODER_REVOLUTIONS:
		result = count / counts_per_rev;
		if (count % counts_per_rev < 0) {
			result--;
		}
		break;
	case SENSOR_CHAN_ROTATION:
		remainder = count % counts_per_rev;
		if (remainder < 0) {
			remainder += counts_per_rev;
		}
		result = (remainder * INT64_C(360000000)) / counts_per_rev;
		value->val1 = result / 1000000;
		value->val2 = result % 1000000;
		return 0;
	default:
		return -ENOTSUP;
	}
	if (result < INT32_MIN || result > INT32_MAX) {
		return -ERANGE;
	}
	value->val1 = result;
	value->val2 = 0;
	return 0;
}

static bool supported_channel(enum sensor_channel channel)
{
	return channel == SENSOR_CHAN_ALL || channel == SENSOR_CHAN_ENCODER_COUNT ||
	       channel == SENSOR_CHAN_ROTATION || channel == SENSOR_CHAN_ENCODER_REVOLUTIONS;
}

static uint16_t read_count(EMIOS_Type *base, uint8_t channel)
{
	return (uint16_t)base->UC[channel].CNT;
}

static int process_event(const struct device *dev, uint8_t index)
{
	const struct qdec_nxp_emios_config *config = dev->config;
	struct qdec_nxp_emios_data *data = dev->data;
	uint8_t channel = config->channels[index];
	uint32_t status = EMIOS_GetUCStatusFlag(config->base, channel);

	if ((status & (kEMIOS_OverrunFlag | kEMIOS_OverflowFlag)) != 0U) {
		return -EOVERFLOW;
	}
	if ((status & kEMIOS_EventFlag) == 0U) {
		return 0;
	}
	if (data->periods[index] >= (uint64_t)INT64_MAX / QDEC_NXP_EMIOS_MODULUS) {
		return -ERANGE;
	}
	data->periods[index]++;
	EMIOS_ClearUCStatusFlag(config->base, kEMIOS_EventFlag, channel);
	return 0;
}

static void latch_error(const struct device *dev, int error)
{
	const struct qdec_nxp_emios_config *config = dev->config;
	struct qdec_nxp_emios_data *data = dev->data;

	data->error = error;
	for (uint8_t i = 0U; i < 2U; i++) {
		EMIOS_DisableUCInterruptAndDMA(config->base, config->channels[i]);
	}
}

static void qdec_nxp_emios_isr(const struct device *dev)
{
	struct qdec_nxp_emios_data *data = dev->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);
	int ret = 0;

	if (data->error == 0) {
		for (uint8_t i = 0U; i < 2U; i++) {
			ret = process_event(dev, i);
			if (ret != 0) {
				latch_error(dev, ret);
				break;
			}
		}
	}
	k_spin_unlock(&data->lock, key);
	if (ret != 0) {
		LOG_ERR("Encoder interrupt accounting failed (%d); reboot to recover", ret);
	}
}

static int read_counters(const struct device *dev, int64_t counts[2])
{
	const struct qdec_nxp_emios_config *config = dev->config;
	struct qdec_nxp_emios_data *data = dev->data;

	for (uint8_t i = 0U; i < 2U; i++) {
		uint32_t status;
		uint16_t counter;
		int ret;

		do {
			ret = process_event(dev, i);
			if (ret != 0) {
				return ret;
			}
			counter = read_count(config->base, config->channels[i]);
			status = EMIOS_GetUCStatusFlag(config->base, config->channels[i]);
			/* Retry if the counter crossed a period boundary during the read. */
		} while ((status & (kEMIOS_EventFlag | kEMIOS_OverrunFlag |
				   kEMIOS_OverflowFlag)) != 0U);
		ret = qdec_nxp_emios_extend_count(data->periods[i], counter, &counts[i]);
		if (ret != 0) {
			return ret;
		}
	}
	return 0;
}

static int qdec_nxp_emios_fetch(const struct device *dev, enum sensor_channel channel)
{
	const struct qdec_nxp_emios_config *config = dev->config;
	struct qdec_nxp_emios_data *data = dev->data;
	k_spinlock_key_t key;
	int64_t counts[2];
	int ret;

	if (!supported_channel(channel)) {
		return -ENOTSUP;
	}
	key = k_spin_lock(&data->lock);
	ret = data->error;
	if (ret != 0) {
		k_spin_unlock(&data->lock, key);
		return ret;
	}
	ret = read_counters(dev, counts);
	if (ret == 0) {
		ret = qdec_nxp_emios_update(&data->sample, counts[0], counts[1],
					    config->reverse);
	}
	if (ret == 0) {
		data->fetched = true;
	} else {
		latch_error(dev, ret);
	}
	k_spin_unlock(&data->lock, key);
	if (ret != 0) {
		LOG_ERR("Encoder sampling failed (%d); reboot to recover", ret);
	}
	return ret;
}

static int qdec_nxp_emios_get(const struct device *dev, enum sensor_channel channel,
			     struct sensor_value *value)
{
	const struct qdec_nxp_emios_config *config = dev->config;
	struct qdec_nxp_emios_data *data = dev->data;
	int64_t count;
	k_spinlock_key_t key;
	int ret;

	if (!supported_channel(channel)) {
		return -ENOTSUP;
	}
	key = k_spin_lock(&data->lock);
	ret = data->error;
	if (ret == 0 && !data->fetched) {
		ret = -EAGAIN;
	}
	count = data->sample.count;
	k_spin_unlock(&data->lock, key);
	if (ret != 0) {
		return ret;
	}
	ret = qdec_nxp_emios_value(count, config->counts_per_rev, channel, value);
	if (ret == -ERANGE) {
		LOG_ERR("Encoder result exceeds Sensor value range");
	}
	return ret;
}

static int configure_lcu(const struct device *dev)
{
	const struct qdec_nxp_emios_config *config = dev->config;
	struct nxp_lcu_cell_config cell = {
		.sources = {
			NXP_LCU_SOURCE_INPUT(config->inputs[0]),
			NXP_LCU_SOURCE_INPUT(config->inputs[1]),
			NXP_LCU_SOURCE_OUTPUT(config->cell * 4U),
			NXP_LCU_SOURCE_OUTPUT(config->cell * 4U + 1U),
		},
		.outputs = {
			{.lut = 0xAAAAU, .rise_filter = config->feedback_filter,
			 .fall_filter = config->feedback_filter},
			{.lut = 0xCCCCU, .rise_filter = config->feedback_filter,
			 .fall_filter = config->feedback_filter},
			{.lut = 0x4182U, .rise_filter = config->pulse_filter,
			 .fall_filter = config->pulse_filter},
			{.lut = 0x2814U, .rise_filter = config->pulse_filter,
			 .fall_filter = config->pulse_filter},
		},
	};
	int ret;

	ret = nxp_lcu_configure(config->lcu, config->cell, dev, &cell);
	if (ret != 0) {
		return ret;
	}
	return nxp_lcu_set_outputs(config->lcu, config->cell, dev, BIT_MASK(4));
}

static int configure_counters(const struct qdec_nxp_emios_config *config)
{
	emios_config_t emios;
	emios_uc_mc_config_t counter = {
		.period = QDEC_NXP_EMIOS_MODULUS,
		.ucMode = kEMIOS_MCB_UpCount,
		.clockSource = kEMIOS_ClkSrcExternal,
		.edgeSelect = kEMIOS_IC_SingleEdge,
		.edgePolarity = kEMIOS_IC_RisingEdge,
	};
	int ret;

	ret = clock_control_on(config->clock, config->clock_id);
	if (ret != 0) {
		LOG_ERR("Cannot enable encoder clock: %d", ret);
		return ret;
	}
	/* EMIOS_Init resets every UC; this driver owns the entire instance. */
	EMIOS_GetDefaultConfig(&emios);
	emios.useGlobalTimeBase = true;
	EMIOS_Init(config->base, &emios);
	for (uint8_t i = 0U; i < 2U; i++) {
		EMIOS_ConfigModulusCounter(config->base, &counter, config->channels[i]);
		EMIOS_DisableUCInterruptAndDMA(config->base, config->channels[i]);
		EMIOS_ClearUCStatusFlag(config->base, kEMIOS_EventFlag | kEMIOS_OverrunFlag |
				       kEMIOS_OverflowFlag, config->channels[i]);
	}
	EMIOS_EnableGlobalPrescaler(config->base);
	return 0;
}

static int configure_inputs(const struct qdec_nxp_emios_config *config)
{
	int ret;

	for (size_t index = 0U; index < config->route_count; index++) {
		const struct qdec_nxp_emios_route *route = &config->routes[index];

		if (!device_is_ready(route->dev)) {
			LOG_ERR("Encoder route controller is not ready");
			return -ENODEV;
		}
		ret = mux_state_apply(route->dev, route->state);
		if (ret != 0) {
			LOG_ERR("Cannot configure encoder route %zu: %d", index, ret);
			return ret;
		}
	}
	return pinctrl_apply_state(config->pinctrl, PINCTRL_STATE_DEFAULT);
}

static int qdec_nxp_emios_init(const struct device *dev)
{
	const struct qdec_nxp_emios_config *config = dev->config;
	struct qdec_nxp_emios_data *data = dev->data;
	int ret;

	if (!device_is_ready(config->clock) || !device_is_ready(config->lcu)) {
		LOG_ERR("Encoder clock or LCU controller is not ready");
		return -ENODEV;
	}
	ret = nxp_lcu_request_cell(config->lcu, config->cell, dev);
	if (ret != 0) {
		return ret;
	}
	ret = configure_lcu(dev);
	if (ret != 0) {
		goto release_lcu;
	}
	ret = configure_counters(config);
	if (ret != 0) {
		goto release_lcu;
	}
	ret = configure_inputs(config);
	if (ret != 0) {
		goto deinit_emios;
	}
	ret = read_counters(dev, data->sample.previous);
	if (ret != 0) {
		LOG_ERR("Cannot read initial encoder counts (%d)", ret);
		goto deinit_emios;
	}
	config->irq_config();
	for (uint8_t i = 0U; i < 2U; i++) {
		EMIOS_EnableUCInterrupt(config->base, config->channels[i]);
	}
	return 0;

deinit_emios:
	EMIOS_Deinit(config->base);
release_lcu:
	if (nxp_lcu_release(config->lcu, config->cell, dev) != 0) {
		LOG_ERR("Cannot release encoder LCU cell");
	}
	return ret;
}

static DEVICE_API(sensor, qdec_nxp_emios_api) = {
	.sample_fetch = qdec_nxp_emios_fetch,
	.channel_get = qdec_nxp_emios_get,
};

#define QDEC_ROUTE(node_id, prop, idx) \
	{ \
		.dev = MUX_STATE_DT_DEV_GET_BY_IDX(node_id, idx), \
		.state = MUX_STATE_DT_GET_BY_IDX(node_id, idx), \
	},

#define QDEC_SAME_EMIOS(node_id, other) \
	+ (DT_SAME_NODE(DT_PHANDLE(node_id, emios), DT_PHANDLE(other, emios)) ? 1 : 0)

#define QDEC_IRQ_CONFIG(idx, n) \
	do { \
		IRQ_CONNECT(DT_IRQ_BY_IDX(DT_INST_PHANDLE(n, emios), idx, irq), \
			    DT_IRQ_BY_IDX(DT_INST_PHANDLE(n, emios), idx, priority), \
			    qdec_nxp_emios_isr, DEVICE_DT_INST_GET(n), 0); \
		irq_enable(DT_IRQ_BY_IDX(DT_INST_PHANDLE(n, emios), idx, irq)); \
	} while (false)

#define QDEC_DEFINE(n) \
	BUILD_ASSERT(DT_NODE_HAS_COMPAT(DT_INST_PHANDLE(n, lcu), nxp_lcu), \
		     "An NXP LCU controller is required"); \
	BUILD_ASSERT(DT_NODE_HAS_STATUS_OKAY(DT_INST_PHANDLE(n, emios)), \
		     "The QDEC eMIOS instance must be enabled"); \
	BUILD_ASSERT(DT_INST_PROP_BY_IDX(n, lcu_inputs, 0) != \
		     DT_INST_PROP_BY_IDX(n, lcu_inputs, 1), "A and B inputs must differ"); \
	BUILD_ASSERT(DT_INST_PROP(n, pulse_filter) < DT_INST_PROP(n, feedback_filter), \
		     "Pulse filter must pass the delayed feedback edge pulses"); \
	BUILD_ASSERT(!DT_NODE_HAS_STATUS_OKAY(DT_CHILD(DT_INST_PHANDLE(n, emios), pwm)), \
		     "QDEC and PWM must use different eMIOS instances"); \
	BUILD_ASSERT((0 DT_FOREACH_STATUS_OKAY_VARGS(nxp_emios_qdec, QDEC_SAME_EMIOS, \
						    DT_DRV_INST(n))) == 1, \
		     "Each QDEC must exclusively own its eMIOS instance"); \
	BUILD_ASSERT(DT_INST_PROP_BY_IDX(n, emios_channels, 0) != \
		     DT_INST_PROP_BY_IDX(n, emios_channels, 1), "QDEC channels must differ"); \
	PINCTRL_DT_INST_DEFINE(n); \
	MUX_STATE_DT_INST_SPEC_DEFINE_ALL(n); \
	static const struct qdec_nxp_emios_route qdec_routes_##n[] = { \
		DT_INST_FOREACH_PROP_ELEM(n, mux_states, QDEC_ROUTE) \
	}; \
	static void qdec_nxp_emios_irq_config_##n(void) \
	{ \
		LISTIFY(DT_NUM_IRQS(DT_INST_PHANDLE(n, emios)), QDEC_IRQ_CONFIG, (;), n); \
	} \
	static const struct qdec_nxp_emios_config qdec_config_##n = { \
		.base = (EMIOS_Type *)DT_REG_ADDR(DT_INST_PHANDLE(n, emios)), \
		.clock = DEVICE_DT_GET(DT_CLOCKS_CTLR(DT_INST_PHANDLE(n, emios))), \
		.clock_id = (clock_control_subsys_t) \
			DT_CLOCKS_CELL(DT_INST_PHANDLE(n, emios), name), \
		.lcu = DEVICE_DT_GET(DT_INST_PHANDLE(n, lcu)), \
		.pinctrl = PINCTRL_DT_INST_DEV_CONFIG_GET(n), \
		.routes = qdec_routes_##n, .route_count = ARRAY_SIZE(qdec_routes_##n), \
		.irq_config = qdec_nxp_emios_irq_config_##n, \
		.counts_per_rev = DT_INST_PROP(n, counts_per_revolution), \
		.feedback_filter = DT_INST_PROP(n, feedback_filter), \
		.pulse_filter = DT_INST_PROP(n, pulse_filter), \
		.channels = DT_INST_PROP(n, emios_channels), \
		.inputs = DT_INST_PROP(n, lcu_inputs), \
		.cell = DT_INST_PROP(n, lcu_cell), \
		.reverse = DT_INST_PROP(n, reverse_direction), \
	}; \
	static struct qdec_nxp_emios_data qdec_data_##n; \
	SENSOR_DEVICE_DT_INST_DEFINE(n, qdec_nxp_emios_init, NULL, \
				     &qdec_data_##n, &qdec_config_##n, \
				     POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &qdec_nxp_emios_api);

DT_INST_FOREACH_STATUS_OKAY(QDEC_DEFINE)
