/*
 * Copyright (c) 2024 Chen Xingyu <hi@xingrz.me>
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT adc_keys

#include <stdlib.h>
#include <stdbool.h>

#include <zephyr/device.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(adc_keys, CONFIG_INPUT_LOG_LEVEL);

struct adc_keys_code_config {
	int32_t press_mv;
	uint8_t key_index;
};

struct adc_keys_key_state {
	bool last_state;
	bool curr_state;
};

struct adc_keys_config {
	struct adc_dt_spec channel;
	uint32_t sample_period_ms;
	int32_t keyup_mv;
	const struct adc_keys_code_config *code_cfg;
	const uint16_t *key_code;
	struct adc_keys_key_state *key_state;
	uint8_t code_cnt;
	uint8_t key_cnt;
};

struct adc_keys_data {
	const struct device *self;
	struct k_work_delayable dwork;
	struct adc_sequence seq;
};

static inline int32_t adc_keys_read(const struct device *dev)
{
	const struct adc_keys_config *cfg = dev->config;
	struct adc_keys_data *data = dev->data;
	uint16_t sample_raw;
	int32_t sample_mv;
	int ret;

	data->seq.buffer = &sample_raw;
	data->seq.buffer_size = sizeof(sample_raw);

	ret = adc_read(cfg->channel.dev, &data->seq);
	if (ret) {
		LOG_ERR("ADC read failed %d", ret);
		return cfg->keyup_mv;
	}

	sample_mv = (int32_t)sample_raw;
	adc_raw_to_millivolts_dt(&cfg->channel, &sample_mv);

	return sample_mv;
}

static inline void adc_keys_process(const struct device *dev)
{
	const struct adc_keys_config *cfg = dev->config;
	int32_t sample_mv, closest_mv;
	uint32_t diff, closest_diff = UINT32_MAX;
	const struct adc_keys_code_config *code_cfg;
	struct adc_keys_key_state *key_state;
	uint16_t key_code;

	sample_mv = adc_keys_read(dev);

	/*
	 * Find the closest key press threshold to the sample value.
	 */

	for (uint8_t i = 0; i < cfg->code_cnt; i++) {
		diff = abs(sample_mv - cfg->code_cfg[i].press_mv);
		if (diff < closest_diff) {
			closest_diff = diff;
			closest_mv = cfg->code_cfg[i].press_mv;
		}
	}

	diff = abs(sample_mv - cfg->keyup_mv);
	if (diff < closest_diff) {
		closest_diff = diff;
		closest_mv = cfg->keyup_mv;
	}

	LOG_DBG("sample=%d mV, closest=%d mV, diff=%d mV", sample_mv, closest_mv, closest_diff);

	/*
	 * Update cached key states according to the closest key press threshold.
	 *
	 * Note that multiple keys may have the same press threshold, which is
	 * the mixed voltage that these keys are simultaneously pressed.
	 */

	for (uint8_t i = 0; i < cfg->code_cnt; i++) {
		code_cfg = &cfg->code_cfg[i];
		key_state = &cfg->key_state[code_cfg->key_index];

		/*
		 * Only update curr_state if the key is pressed to prevent
		 * being overwritten by another threshold configuration.
		 */
		if (closest_mv == code_cfg->press_mv) {
			key_state->curr_state = true;
		}
	}

	/*
	 * Report the key event if the key state has changed.
	 */

	for (uint8_t i = 0; i < cfg->key_cnt; i++) {
		key_state = &cfg->key_state[i];
		key_code = cfg->key_code[i];

		if (key_state->last_state != key_state->curr_state) {
			LOG_DBG("Report event %s %d, code=%d", dev->name, key_state->curr_state,
				key_code);
			input_report_key(dev, key_code, key_state->curr_state, true, K_FOREVER);
			key_state->last_state = key_state->curr_state;
		}

		/*
		 * Reset the state so that it can be updated in the next
		 * iteration.
		 */
		key_state->curr_state = false;
	}
}

static void adc_keys_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct adc_keys_data *data = CONTAINER_OF(dwork, struct adc_keys_data, dwork);
	const struct device *dev = data->self;
	const struct adc_keys_config *cfg = dev->config;

	adc_keys_process(dev);

	k_work_schedule(&data->dwork, K_MSEC(cfg->sample_period_ms));
}

static int adc_keys_init(const struct device *dev)
{
	const struct adc_keys_config *cfg = dev->config;
	struct adc_keys_data *data = dev->data;
	int ret;

	if (!adc_is_ready_dt(&cfg->channel)) {
		LOG_ERR("ADC controller device %s not ready", cfg->channel.dev->name);
		return -ENODEV;
	}

	ret = adc_channel_setup_dt(&cfg->channel);
	if (ret) {
		LOG_ERR("ADC channel setup failed %d", ret);
		return ret;
	}

	ret = adc_sequence_init_dt(&cfg->channel, &data->seq);
	if (ret) {
		LOG_ERR("ADC sequence init failed %d", ret);
		return ret;
	}

	data->self = dev;
	k_work_init_delayable(&data->dwork, adc_keys_work_handler);

	if (IS_ENABLED(CONFIG_INPUT_LOG_LEVEL_DBG)) {
		for (uint8_t i = 0; i < cfg->code_cnt; i++) {
			LOG_DBG("* code %d: key_index=%d threshold=%d mV code=%d", i,
				cfg->code_cfg[i].key_index, cfg->code_cfg[i].press_mv,
				cfg->key_code[cfg->code_cfg[i].key_index]);
		}
	}

	k_work_schedule(&data->dwork, K_MSEC(cfg->sample_period_ms));

	return 0;
}

#define ADC_KEYS_CODE_CFG_ITEM(node_id, prop, idx)                                                 \
	{                                                                                          \
		.key_index = DT_NODE_CHILD_IDX(node_id) /* include disabled nodes */,              \
		.press_mv = DT_PROP_BY_IDX(node_id, prop, idx),                                    \
	}

#define ADC_KEYS_CODE_CFG(node_id)                                                                 \
	DT_FOREACH_PROP_ELEM_SEP(node_id, press_thresholds_mv, ADC_KEYS_CODE_CFG_ITEM, (,))

#define ADC_KEYS_KEY_CODE(node_id) DT_PROP(node_id, zephyr_code)

#define ADC_KEYS_INST(n)                                                                           \
	static struct adc_keys_data adc_keys_data_##n;                                             \
                                                                                                   \
	static const struct adc_keys_code_config adc_keys_code_cfg_##n[] = {                       \
		DT_INST_FOREACH_CHILD_STATUS_OKAY_SEP(n, ADC_KEYS_CODE_CFG, (,))};                 \
                                                                                                   \
	static const uint16_t adc_keys_key_code_##n[] = {                                          \
		DT_INST_FOREACH_CHILD_SEP(n, ADC_KEYS_KEY_CODE, (,))};                             \
                                                                                                   \
	static struct adc_keys_key_state                                                           \
		adc_keys_key_state_##n[ARRAY_SIZE(adc_keys_key_code_##n)];                         \
                                                                                                   \
	static const struct adc_keys_config adc_keys_cfg_##n = {                                   \
		.channel = ADC_DT_SPEC_INST_GET(n),                                                \
		.sample_period_ms = DT_INST_PROP(n, sample_period_ms),                             \
		.keyup_mv = DT_INST_PROP(n, keyup_threshold_mv),                                   \
		.code_cfg = adc_keys_code_cfg_##n,                                                 \
		.key_code = adc_keys_key_code_##n,                                                 \
		.key_state = adc_keys_key_state_##n,                                               \
		.code_cnt = ARRAY_SIZE(adc_keys_code_cfg_##n),                                     \
		.key_cnt = ARRAY_SIZE(adc_keys_key_code_##n),                                      \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, adc_keys_init, NULL, &adc_keys_data_##n, &adc_keys_cfg_##n,       \
			      POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(ADC_KEYS_INST)
