/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rx_ctsu

#include <zephyr/device.h>
#include <zephyr/input/input.h>
#include <zephyr/drivers/pinctrl.h>
#include <r_ctsu_qe_pinset.h>
#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <soc.h>
#include <r_ctsu_qe_if.h>
#include <rm_touch_qe_if.h>
#include <zephyr/input/input_renesas_rx_ctsu.h>
#include <zephyr/dt-bindings/input/input-event-codes.h>

LOG_MODULE_REGISTER(renesas_rx_ctsu, CONFIG_INPUT_LOG_LEVEL);

#define BUTTON_TYPE 0
#define SLIDER_TYPE 1
#define WHEEL_TYPE  2

#define MAX_TUNING_LOOP_COUNT 1024

typedef enum {
	INITIALIZING = 0,
	TUNING = 1,
	SCANNING = 2,
} working_phase_t;

enum touch_event {
	RELEASE = 0, /* state change from TOUCHING to UNTOUCH */
	PRESS = 1,   /* state change from UNTOUCH to TOUCHING */
};

/** Configuration of each TS channel */
typedef struct st_touch_channel_config {
	uint8_t channel_num;
	ctsu_element_cfg_t config;
} touch_channel_cfg_t;

/** Component context */
typedef struct st_buttons_context {
	/** TS channel of button */
	uint8_t element;
	/** Configuration for each button */
	touch_button_cfg_t config;
	/** Event for that will be reported to higher layer */
	uint16_t event;
} touch_button_context_t;

typedef struct st_sliders_context {
	/** Array of TS channels used in slider */
	uint8_t *p_elements;
	/** Configuration for each slider */
	touch_slider_cfg_t config;
	/** Event for that will be reported to higher layer */
	uint16_t event;
} touch_slider_context_t;

typedef struct st_wheels_context {
	/** Array of TS channels used in wheel */
	uint8_t *p_elements;
	/** Configuration for each wheel */
	touch_wheel_cfg_t config;
	/** Event that will be reported to higher layer */
	uint16_t event;
} touch_wheel_context_t;

struct renesas_rx_ctsu_config {
	const struct pinctrl_dev_config *pcfg;
	/** CTSU channels config */
	touch_channel_cfg_t *channel_cfgs;
	uint8_t *channels_index_map;
	uint8_t *button_position_index;
	/** Touch components */
	touch_button_context_t *buttons;
	touch_slider_context_t *sliders;
	touch_wheel_context_t *wheels;
};

struct renesas_rx_ctsu_data {
	const struct device *dev;
	/** Data processing */
	struct k_work data_process_work;
	struct k_work scan_work;
	struct k_timer scan_timer;
	struct k_sem tune_scan_end;
	working_phase_t work_phase;
	/** Touch instances */
	touch_instance_t touch_instance;
#ifndef CONFIG_INPUT_RENESAS_RX_QE_TOUCH_CFG
	touch_instance_ctrl_t touch_ctrl;
	touch_cfg_t touch_cfg;
	/** CTSU instances */
	ctsu_instance_t ctsu_instance;
	ctsu_instance_ctrl_t ctsu_ctrl;
	ctsu_cfg_t ctsu_cfg;
#endif /* CONFIG_INPUT_RENESAS_RX_QE_TOUCH_CFG */
	/** Touch driver output data */
	uint64_t curr_buttons_data;
	uint64_t prev_buttons_data;
	uint16_t *curr_sliders_position;
	uint16_t *prev_sliders_position;
	uint16_t *curr_wheels_position;
	uint16_t *prev_wheels_position;
};

void ctsu_ctsuend_isr(void);
void ctsu_ctsuwr_isr(void);
void ctsu_ctsurd_isr(void);

static void ctsuwr_isr(const struct device *dev)
{
	ARG_UNUSED(dev);
	ctsu_ctsuwr_isr();
}

static void ctsurd_isr(const struct device *dev)
{
	ARG_UNUSED(dev);
	ctsu_ctsurd_isr();
}

static void ctsufn_isr(const struct device *dev)
{
	ARG_UNUSED(dev);
	ctsu_ctsuend_isr();
}

static void ctsu_scan_callback(ctsu_callback_args_t *p_arg)
{
	const struct device *dev = p_arg->p_context;
	struct renesas_rx_ctsu_data *data = dev->data;

	if (data->work_phase == TUNING) {
		k_sem_give(&data->tune_scan_end);
		return;
	}

	if (data->work_phase != SCANNING || p_arg->event != CTSU_EVENT_SCAN_COMPLETE) {
		return;
	}

	k_work_submit(&data->data_process_work);
}

static void process_data(struct k_work *work)
{
	struct renesas_rx_ctsu_data *data =
		CONTAINER_OF(work, struct renesas_rx_ctsu_data, data_process_work);
	const struct device *dev = data->dev;
	const struct renesas_rx_ctsu_config *config = dev->config;
	fsp_err_t ret;

	ret = RM_TOUCH_DataGet(data->touch_instance.p_ctrl, &data->curr_buttons_data,
			       data->curr_sliders_position, data->curr_wheels_position);
	if (ret != FSP_SUCCESS) {
		LOG_ERR("CTSU: Failed to get data %d", ret);
		return;
	}

	/** Buttons */
	int changed_buttons = data->curr_buttons_data ^ data->prev_buttons_data;
	int button_position = 0;

	while (changed_buttons != 0) {
		if (changed_buttons & BIT(0)) {
			int index = config->button_position_index[button_position];

			input_report_key(dev, config->buttons[index].event,
					 data->curr_buttons_data & BIT(button_position), true,
					 K_FOREVER);
		}
		button_position++;
		changed_buttons = changed_buttons >> 1;
	}
	data->prev_buttons_data = data->curr_buttons_data;

	/** Sliders */
	for (int i = 0; i < data->touch_instance.p_cfg->num_sliders; i++) {
		if (data->curr_sliders_position[i] != data->prev_sliders_position[i]) {
			input_report_abs(dev, config->sliders[i].event,
					 data->curr_sliders_position[i], true, K_FOREVER);
		}
		data->prev_sliders_position[i] = data->curr_sliders_position[i];
	}

	/** Wheels */
	for (int i = 0; i < data->touch_instance.p_cfg->num_wheels; i++) {
		if (data->curr_wheels_position[i] != data->prev_wheels_position[i]) {
			input_report_abs(dev, config->wheels[i].event,
					 data->curr_wheels_position[i], true, K_FOREVER);
		}
		data->prev_wheels_position[i] = data->curr_wheels_position[i];
	}
}

static void timer_callback(struct k_timer *timer)
{
	struct renesas_rx_ctsu_data *data =
		CONTAINER_OF(timer, struct renesas_rx_ctsu_data, scan_timer);
	k_work_submit(&data->scan_work);
}

static void trigger_scan(struct k_work *work)
{
	struct renesas_rx_ctsu_data *data =
		CONTAINER_OF(work, struct renesas_rx_ctsu_data, scan_work);

	/** Start next scan */
	RM_TOUCH_ScanStart(data->touch_instance.p_ctrl);
}

#ifndef CONFIG_INPUT_RENESAS_RX_QE_TOUCH_CFG
static inline int set_scan_channel(const struct device *dev)
{
	struct renesas_rx_ctsu_data *data = dev->data;
	const struct renesas_rx_ctsu_config *config = dev->config;
	touch_channel_cfg_t *channel_cfgs = config->channel_cfgs;

	for (int i = 0; i < data->ctsu_cfg.num_rx; i++) {
		int cha_reg = channel_cfgs[i].channel_num / 8;
		int cha_pos = channel_cfgs[i].channel_num % 8;

		switch (cha_reg) {
		case 0:
			data->ctsu_cfg.ctsuchac0 |= (1 << cha_pos);
			break;
		case 1:
			data->ctsu_cfg.ctsuchac1 |= (1 << cha_pos);
			break;
		case 2:
			data->ctsu_cfg.ctsuchac2 |= (1 << cha_pos);
			break;
		case 3:
			data->ctsu_cfg.ctsuchac3 |= (1 << cha_pos);
			break;
		case 4:
			data->ctsu_cfg.ctsuchac4 |= (1 << cha_pos);
			break;
		default:
			LOG_ERR("Invalid TS channel");
			return -EINVAL;
		}
	}

	return 0;
}
#endif /* !CONFIG_INPUT_RENESAS_RX_QE_TOUCH_CFG */

static int input_renesas_rx_ctsu_configure(const struct device *dev,
					   const struct renesas_rx_ctsu_touch_cfg *cfg)
{
	struct renesas_rx_ctsu_data *data = dev->data;
	fsp_err_t ret;
	int tuning_loop_count = 0;

	data->touch_instance = cfg->touch_instance;

	k_sem_init(&data->tune_scan_end, 0, 1);

	/** Set initial states */
	for (int i = 0; i < data->touch_instance.p_cfg->num_sliders; i++) {
		data->prev_sliders_position[i] = TOUCH_OFF_VALUE;
		data->curr_sliders_position[i] = TOUCH_OFF_VALUE;
	}
	for (int i = 0; i < data->touch_instance.p_cfg->num_wheels; i++) {
		data->prev_wheels_position[i] = TOUCH_OFF_VALUE;
		data->curr_wheels_position[i] = TOUCH_OFF_VALUE;
	}

	data->work_phase = INITIALIZING;
	ret = RM_TOUCH_Open(data->touch_instance.p_ctrl, data->touch_instance.p_cfg);
	if (ret != FSP_SUCCESS) {
		LOG_ERR("CTSU Open failed");
		return -EIO;
	}

	ret = RM_TOUCH_CallbackSet(data->touch_instance.p_ctrl, ctsu_scan_callback, (void *)dev,
				   NULL);
	if (ret != FSP_SUCCESS) {
		LOG_ERR("CTSU Failed to set callback");
		return -EIO;
	}

	data->work_phase = TUNING;

	do {
		ret = RM_TOUCH_ScanStart(data->touch_instance.p_ctrl);
		if (ret != FSP_SUCCESS) {
			LOG_ERR("CTSU: Failed to start scan");
			return -EIO;
		}
		k_sem_take(&data->tune_scan_end, K_FOREVER);
		ret = RM_TOUCH_DataGet(data->touch_instance.p_ctrl, &data->curr_buttons_data,
				       data->curr_sliders_position, data->curr_wheels_position);
		tuning_loop_count++;
	} while (ret != FSP_SUCCESS && tuning_loop_count < MAX_TUNING_LOOP_COUNT);

	if (tuning_loop_count >= MAX_TUNING_LOOP_COUNT) {
		LOG_ERR("CTSU: Failed to tune the touch sensor");
		return -EIO;
	}

	data->dev = dev;

	/* Processing data handler */
	k_work_init(&data->data_process_work, process_data);

	/* Scanning trigger */
	k_work_init(&data->scan_work, trigger_scan);

	/* Timer for to set scanning work */
	k_timer_init(&data->scan_timer, timer_callback, NULL);

	/* Start first scan to ensure the scanning can run normally */
	data->work_phase = SCANNING;
	ret = RM_TOUCH_ScanStart(data->touch_instance.p_ctrl);
	if (ret != FSP_SUCCESS) {
		LOG_ERR("CTSU: Failed to start scan");
		return -EIO;
	}

	/* Start timer to periodically run scanning work */
	k_timer_start(&data->scan_timer, K_MSEC(CONFIG_INPUT_RENESAS_RX_CTSU_SCAN_INTERVAL_MS),
		      K_MSEC(CONFIG_INPUT_RENESAS_RX_CTSU_SCAN_INTERVAL_MS));

	return 0;
}

int z_impl_renesas_rx_ctsu_group_configure(const struct device *dev,
					   const struct renesas_rx_ctsu_touch_cfg *cfg)
{
#ifndef CONFIG_INPUT_RENESAS_RX_QE_TOUCH_CFG
	ARG_UNUSED(dev);
	ARG_UNUSED(cfg);
	return -ENOSYS;
#else
	return input_renesas_rx_ctsu_configure(dev, cfg);
#endif /* CONFIG_INPUT_RENESAS_RX_QE_TOUCH_CFG */
}

static int renesas_rx_ctsu_init(const struct device *dev)
{
#ifndef CONFIG_INPUT_RENESAS_RX_QE_TOUCH_CFG
	struct renesas_rx_ctsu_data *data = dev->data;
#endif /* !CONFIG_INPUT_RENESAS_RX_QE_TOUCH_CFG */
	const struct renesas_rx_ctsu_config *config = dev->config;
	int err;

	err = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (err < 0) {
		LOG_ERR("CTSU: Failed to set pinctrl");
		return err;
	}

#ifndef CONFIG_INPUT_RENESAS_RX_QE_TOUCH_CFG

	err = set_scan_channel(dev);
	if (err < 0) {
		LOG_ERR("CTSU: Failed to set scan channel");
		return err;
	}

	data->ctsu_instance.p_ctrl = &data->ctsu_ctrl;
	data->ctsu_instance.p_cfg = &data->ctsu_cfg;
	data->ctsu_instance.p_api = &g_ctsu_on_ctsu;

	data->touch_cfg.p_ctsu_instance = &data->ctsu_instance;
	data->touch_instance.p_ctrl = &data->touch_ctrl;
	data->touch_instance.p_cfg = &data->touch_cfg;
	data->touch_instance.p_api = &g_touch_on_ctsu;

	return input_renesas_rx_ctsu_configure(
		dev, (const struct renesas_rx_ctsu_touch_cfg *)&data->touch_instance);
#else
	return 0;
#endif /* !CONFIG_INPUT_RENESAS_RX_QE_TOUCH_CFG */
}

#define CHANNEL_GET_CONFIG(node_id, prop, idx)                                                     \
	{                                                                                          \
		.channel_num = DT_PROP_BY_IDX(node_id, prop, idx),                                 \
		.config =                                                                          \
			{                                                                          \
				.ssdiv = DT_PROP(node_id, ssdiv),                                  \
				.so = DT_PROP(node_id, so),                                        \
				.snum = DT_PROP(node_id, snum),                                    \
				.sdpa = DT_PROP(node_id, sdpa),                                    \
			},                                                                         \
	},

#define CTSU_CHANNEL_CFG_INIT(node_id)                                                             \
	DT_FOREACH_PROP_ELEM(node_id, channels_num, CHANNEL_GET_CONFIG)

#define CTSU_GET_CHANNELS_COUNT(node_id) DT_PROP_LEN(node_id, channels_num)

#define BUTTON_GET_CONTEXT(node_id)                                                                \
	IF_ENABLED(IS_EQ(DT_ENUM_IDX(node_id, component_type), BUTTON_TYPE),                       \
	({                                                                                         \
		.element = DT_PROP_BY_IDX(node_id, channels_num, 0),                               \
		.config = {                                                                        \
			.elem_index = 0,                                                           \
			.threshold = DT_PROP(node_id, touch_count_threshold),                      \
			.hysteresis = DT_PROP(node_id, threshold_range),                           \
		},                                                                                 \
		.event = DT_PROP(node_id, zephyr_code),                                            \
	},))

#define SLIDER_GET_CONTEXT(node_id)                                                                \
	IF_ENABLED(IS_EQ(DT_ENUM_IDX(node_id, component_type), SLIDER_TYPE),                       \
	({                                                                                         \
		.p_elements = (uint8_t[])DT_PROP(node_id, channels_num),                           \
		.config = {                                                                        \
			.p_elem_index = NULL,                                                      \
			.num_elements = DT_PROP_LEN(node_id, channels_num),                        \
			.threshold = DT_PROP(node_id, touch_count_threshold),                      \
		},                                                                                 \
		.event = DT_PROP(node_id, zephyr_code),                                            \
	},))

#define WHEEL_GET_CONTEXT(node_id)                                                                 \
	IF_ENABLED(IS_EQ(DT_ENUM_IDX(node_id, component_type), WHEEL_TYPE),                        \
	({                                                                                         \
		.p_elements = (uint8_t[])DT_PROP(node_id, channels_num),                           \
		.config = {                                                                        \
			.p_elem_index = NULL,                                                      \
			.num_elements = DT_PROP_LEN(node_id, channels_num),                        \
			.threshold = DT_PROP(node_id, touch_count_threshold),                      \
		},                                                                                 \
		.event = DT_PROP(node_id, zephyr_code),                                            \
	},))

#define NUM_ELEMENTS(idx) DT_INST_FOREACH_CHILD_STATUS_OKAY_SEP(idx, CTSU_GET_CHANNELS_COUNT, (+))

#define COMPONENT_COUNT(node_id, type) IS_EQ(DT_ENUM_IDX(node_id, component_type), type)

#define COMPONENT_GET_COUNT_BY_TYPE(idx, type)                                                     \
	DT_INST_FOREACH_CHILD_STATUS_OKAY_SEP_VARGS(idx, COMPONENT_COUNT, (+), type)

#ifndef CONFIG_INPUT_RENESAS_RX_QE_TOUCH_CFG
#define CTSU_HAL_CONFIG_DEFINE(idx)                                                                \
	.ctsu_cfg =                                                                                \
		{                                                                                  \
			.cap = CTSU_CAP_SOFTWARE,                                                  \
			.md = CTSU_MODE_SELF_MULTI_SCAN,                                           \
			.num_rx = NUM_ELEMENTS(idx),                                               \
			.num_moving_average = CONFIG_INPUT_RENESAS_RX_CTSU_NUM_MOVING_AVERAGE,     \
			.atune1 = CONFIG_INPUT_RENESAS_RX_CTSU_POWER_SUPPLY_CAPACITY,              \
			.txvsel = CONFIG_INPUT_RENESAS_RX_CTSU_TRANSMISSION_POWER_SUPPLY,          \
			.ctsuchac0 = 0,                                                            \
			.ctsuchac1 = 0,                                                            \
			.ctsuchac2 = 0,                                                            \
			.ctsuchac3 = 0,                                                            \
			.ctsuchac4 = 0,                                                            \
			.ctsuchtrc0 = 0,                                                           \
			.ctsuchtrc1 = 0,                                                           \
			.ctsuchtrc2 = 0,                                                           \
			.ctsuchtrc3 = 0,                                                           \
			.ctsuchtrc4 = 0,                                                           \
			.tuning_enable = true,                                                     \
			.p_elements = ctsu_element_cfgs_##idx,                                     \
			.p_callback = ctsu_scan_callback,                                          \
			.p_context = NULL,                                                         \
	},                                                                                         \
	.touch_cfg = {                                                                             \
		.p_buttons = button_cfgs_##idx,                                                    \
		.num_buttons = COMPONENT_GET_COUNT_BY_TYPE(idx, BUTTON_TYPE),                      \
		.p_sliders = slider_cfgs_##idx,                                                    \
		.num_sliders = COMPONENT_GET_COUNT_BY_TYPE(idx, SLIDER_TYPE),                      \
		.p_wheels = wheel_cfgs_##idx,                                                      \
		.num_wheels = COMPONENT_GET_COUNT_BY_TYPE(idx, WHEEL_TYPE),                        \
		.on_freq = CONFIG_INPUT_RENESAS_RX_CTSU_ON_FREQ,                                   \
		.off_freq = CONFIG_INPUT_RENESAS_RX_CTSU_OFF_FREQ,                                 \
		.drift_freq = CONFIG_INPUT_RENESAS_RX_CTSU_DRIFT_FREQ,                             \
		.cancel_freq = CONFIG_INPUT_RENESAS_RX_CTSU_CANCEL_FREQ,                           \
	},
#else
#define CTSU_HAL_CONFIG_DEFINE(idx)
#endif /* !CONFIG_INPUT_RENESAS_RX_QE_TOUCH_CFG */

#define RENESAS_RX_CTSU_INIT(idx)                                                                  \
	PINCTRL_DT_INST_DEFINE(idx);                                                               \
	static void ctsu_irq_config_func_##idx(void)                                               \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(idx, ctsuwr, irq),                                 \
			    DT_INST_IRQ_BY_NAME(idx, ctsuwr, priority), ctsuwr_isr,                \
			    DEVICE_DT_INST_GET(idx), 0);                                           \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(idx, ctsurd, irq),                                 \
			    DT_INST_IRQ_BY_NAME(idx, ctsurd, priority), ctsurd_isr,                \
			    DEVICE_DT_INST_GET(idx), 0);                                           \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(idx, ctsufn, irq),                                 \
			    DT_INST_IRQ_BY_NAME(idx, ctsufn, priority), ctsufn_isr,                \
			    DEVICE_DT_INST_GET(idx), 0);                                           \
		irq_enable(DT_INST_IRQ_BY_NAME(idx, ctsuwr, irq));                                 \
		irq_enable(DT_INST_IRQ_BY_NAME(idx, ctsurd, irq));                                 \
		irq_enable(DT_INST_IRQ_BY_NAME(idx, ctsufn, irq));                                 \
	}                                                                                          \
                                                                                                   \
	static touch_channel_cfg_t ctsu_channel_cfgs_##idx[] = {                                   \
		DT_INST_FOREACH_CHILD_STATUS_OKAY(idx, CTSU_CHANNEL_CFG_INIT)};                    \
	static uint8_t channels_index_map_##idx[DT_PROP(DT_DRV_INST(idx), max_num_sensors)];       \
                                                                                                   \
	static ctsu_element_cfg_t ctsu_element_cfgs_##idx[NUM_ELEMENTS(idx)];                      \
                                                                                                   \
	static void sort_configs_by_channel_num##idx(void)                                         \
	{                                                                                          \
		memset(channels_index_map_##idx, 0xff, sizeof(channels_index_map_##idx));          \
		for (int i = 0; i < NUM_ELEMENTS(idx); i++) {                                      \
			int min_idx = i;                                                           \
			for (int j = i + 1; j < NUM_ELEMENTS(idx); j++) {                          \
				if (ctsu_channel_cfgs_##idx[j].channel_num <                       \
				    ctsu_channel_cfgs_##idx[min_idx].channel_num) {                \
					min_idx = j;                                               \
				}                                                                  \
			}                                                                          \
			if (min_idx != i) {                                                        \
				touch_channel_cfg_t tmp = ctsu_channel_cfgs_##idx[i];              \
				ctsu_channel_cfgs_##idx[i] = ctsu_channel_cfgs_##idx[min_idx];     \
				ctsu_channel_cfgs_##idx[min_idx] = tmp;                            \
			}                                                                          \
			ctsu_element_cfgs_##idx[i] = ctsu_channel_cfgs_##idx[i].config;            \
			channels_index_map_##idx[ctsu_channel_cfgs_##idx[i].channel_num] = i;      \
		}                                                                                  \
	}                                                                                          \
                                                                                                   \
	static touch_button_context_t buttons_##idx[] = {                                          \
		DT_INST_FOREACH_CHILD_STATUS_OKAY(idx, BUTTON_GET_CONTEXT)};                       \
	static touch_slider_context_t sliders_##idx[] = {                                          \
		DT_INST_FOREACH_CHILD_STATUS_OKAY(idx, SLIDER_GET_CONTEXT)};                       \
	static touch_wheel_context_t wheels_##idx[] = {                                            \
		DT_INST_FOREACH_CHILD_STATUS_OKAY(idx, WHEEL_GET_CONTEXT)};                        \
                                                                                                   \
	static touch_button_cfg_t                                                                  \
		button_cfgs_##idx[COMPONENT_GET_COUNT_BY_TYPE(idx, BUTTON_TYPE)];                  \
	static touch_slider_cfg_t                                                                  \
		slider_cfgs_##idx[COMPONENT_GET_COUNT_BY_TYPE(idx, SLIDER_TYPE)];                  \
	static touch_wheel_cfg_t wheel_cfgs_##idx[COMPONENT_GET_COUNT_BY_TYPE(idx, WHEEL_TYPE)];   \
                                                                                                   \
	static uint8_t sliders_element_index[COMPONENT_GET_COUNT_BY_TYPE(idx, SLIDER_TYPE)]        \
					    [DT_PROP(DT_DRV_INST(idx), max_num_sensors)];          \
	static uint8_t wheels_element_index[COMPONENT_GET_COUNT_BY_TYPE(idx, WHEEL_TYPE)]          \
					   [DT_PROP(DT_DRV_INST(idx), max_num_sensors)];           \
                                                                                                   \
	static uint8_t button_position_to_cfg_index##idx[COMPONENT_GET_COUNT_BY_TYPE(              \
		idx, BUTTON_TYPE)] = {0};                                                          \
                                                                                                   \
	static void map_component_cfgs##idx(void)                                                  \
	{                                                                                          \
		uint8_t temp[COMPONENT_GET_COUNT_BY_TYPE(idx, BUTTON_TYPE)];                       \
		for (int i = 0; i < COMPONENT_GET_COUNT_BY_TYPE(idx, BUTTON_TYPE); i++) {          \
			button_cfgs_##idx[i] = buttons_##idx[i].config;                            \
			button_cfgs_##idx[i].elem_index =                                          \
				channels_index_map_##idx[buttons_##idx[i].element];                \
			temp[i] = button_cfgs_##idx[i].elem_index;                                 \
			button_position_to_cfg_index##idx[i] = i;                                  \
		}                                                                                  \
		for (int i = 0; i < COMPONENT_GET_COUNT_BY_TYPE(idx, BUTTON_TYPE) - 1; i++) {      \
			for (int j = i + 1; j < COMPONENT_GET_COUNT_BY_TYPE(idx, BUTTON_TYPE);     \
			     j++) {                                                                \
				if (temp[i] > temp[j]) {                                           \
					int tmp = temp[i];                                         \
					temp[i] = temp[j];                                         \
					temp[j] = tmp;                                             \
					tmp = button_position_to_cfg_index##idx[i];                \
					button_position_to_cfg_index##idx[i] =                     \
						button_position_to_cfg_index##idx[j];              \
					button_position_to_cfg_index##idx[j] = tmp;                \
				}                                                                  \
			}                                                                          \
		}                                                                                  \
		for (int i = 0; i < COMPONENT_GET_COUNT_BY_TYPE(idx, SLIDER_TYPE); i++) {          \
			slider_cfgs_##idx[i] = sliders_##idx[i].config;                            \
			for (int j = 0; j < slider_cfgs_##idx[i].num_elements; j++) {              \
				sliders_element_index[i][j] =                                      \
					channels_index_map_##idx[sliders_##idx[i].p_elements[j]];  \
			}                                                                          \
			slider_cfgs_##idx[i].p_elem_index = sliders_element_index[i];              \
		}                                                                                  \
		for (int i = 0; i < COMPONENT_GET_COUNT_BY_TYPE(idx, WHEEL_TYPE); i++) {           \
			wheel_cfgs_##idx[i] = wheels_##idx[i].config;                              \
			for (int j = 0; j < wheel_cfgs_##idx[i].num_elements; j++) {               \
				wheels_element_index[i][j] =                                       \
					channels_index_map_##idx[wheels_##idx[i].p_elements[j]];   \
			}                                                                          \
			wheel_cfgs_##idx[i].p_elem_index = wheels_element_index[i];                \
		}                                                                                  \
	}                                                                                          \
                                                                                                   \
	static const struct renesas_rx_ctsu_config renesas_rx_ctsu_config_##idx = {                \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(idx),                                       \
		.channel_cfgs = ctsu_channel_cfgs_##idx,                                           \
		.channels_index_map = channels_index_map_##idx,                                    \
		.button_position_index = button_position_to_cfg_index##idx,                        \
		.buttons = buttons_##idx,                                                          \
		.sliders = sliders_##idx,                                                          \
		.wheels = wheels_##idx,                                                            \
	};                                                                                         \
                                                                                                   \
	static uint16_t slider_prev_position_##idx[COMPONENT_GET_COUNT_BY_TYPE(idx, SLIDER_TYPE)]; \
	static uint16_t slider_curr_position_##idx[COMPONENT_GET_COUNT_BY_TYPE(idx, SLIDER_TYPE)]; \
	static uint16_t prev_wheels_position_##idx[COMPONENT_GET_COUNT_BY_TYPE(idx, WHEEL_TYPE)];  \
	static uint16_t curr_wheels_position_##idx[COMPONENT_GET_COUNT_BY_TYPE(idx, WHEEL_TYPE)];  \
                                                                                                   \
	static struct renesas_rx_ctsu_data renesas_rx_ctsu_data_##idx = {                          \
		.prev_buttons_data = 0,                                                            \
		.curr_buttons_data = 0,                                                            \
		.prev_sliders_position = slider_prev_position_##idx,                               \
		.curr_sliders_position = slider_curr_position_##idx,                               \
		.prev_wheels_position = prev_wheels_position_##idx,                                \
		.curr_wheels_position = curr_wheels_position_##idx,                                \
		.work_phase = INITIALIZING,                                                        \
		CTSU_HAL_CONFIG_DEFINE(idx)};                                                      \
                                                                                                   \
	static int renesas_rx_ctsu_init_##idx(const struct device *dev)                            \
	{                                                                                          \
		sort_configs_by_channel_num##idx();                                                \
		map_component_cfgs##idx();                                                         \
		ctsu_irq_config_func_##idx();                                                      \
		return renesas_rx_ctsu_init(dev);                                                  \
	}                                                                                          \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(idx, &renesas_rx_ctsu_init_##idx, NULL, &renesas_rx_ctsu_data_##idx, \
			      &renesas_rx_ctsu_config_##idx, POST_KERNEL,                          \
			      CONFIG_INPUT_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(RENESAS_RX_CTSU_INIT)
