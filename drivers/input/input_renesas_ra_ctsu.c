/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/irq.h>
#include <zephyr/input/input.h>
#include <zephyr/sys/math_extras.h>
#include <zephyr/sys/mem_blocks.h>
#include <soc.h>

#include <zephyr/drivers/clock_control/renesas_ra_cgc.h>
#include <zephyr/input/input_renesas_ra_ctsu.h>
#include <rm_touch.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(renesas_ra_touch, CONFIG_INPUT_LOG_LEVEL);

struct renesas_ra_ctsu_cfg {
	const struct gpio_dt_spec tscap_pin;
	const struct pinctrl_dev_config *pcfg;
	const struct device *clock;
	const struct clock_control_ra_subsys_cfg clock_subsys;
	void (*irq_config)(void);
};

struct renesas_ra_ctsu_data {
	struct k_sem scanning;
	struct k_queue scan_q;
	struct k_thread thread_data;

	K_KERNEL_STACK_MEMBER(thread_stack, CONFIG_INPUT_RENESAS_RA_CTSU_DRV_STACK_SIZE);
};

struct renesas_ra_ctsu_group_cfg {
	const struct device *ctsu_dev;
	size_t num_button;
	size_t num_slider;
	size_t num_wheel;
};

struct renesas_ra_ctsu_device_cb {
	const struct device *dev;
	void (*device_cb)(const struct device *dev, void *data);
};

struct renesas_ra_ctsu_group_data {
	const struct device *dev;
	/* FSP Touch data */
	struct st_touch_instance touch_instance;
#ifndef CONFIG_INPUT_RENESAS_RA_QE_TOUCH_CFG
	struct st_touch_instance_ctrl touch_ctrl;
	struct st_touch_cfg touch_cfg;
	/* FSP CTSU data */
	struct st_ctsu_instance ctsu_instance;
	struct st_ctsu_instance_ctrl ctsu_ctrl;
	struct st_ctsu_cfg ctsu_cfg;
#endif /* CONFIG_INPUT_RENESAS_RA_QE_TOUCH_CFG */
	/* Touch driver private data */
	struct k_work reading_work;
	struct k_timer sampling_timer;
	/* Touch driver sample result */
	uint64_t *p_button_status;
	uint16_t *p_slider_position;
	uint16_t *p_wheel_position;
	/* Touch device callback data */
	struct renesas_ra_ctsu_device_cb *p_button_cb;
	struct renesas_ra_ctsu_device_cb *p_slider_cb;
	struct renesas_ra_ctsu_device_cb *p_wheel_cb;
};

extern void ctsu_write_isr(void);
extern void ctsu_read_isr(void);
extern void ctsu_end_isr(void);

struct ctsu_device_cfg {
	const struct device *group_dev;
	uint16_t event_code;
};

struct ctsu_scan_msg {
	void *reserved; /* first word of queue data item reserved for the kernel */
	struct st_touch_instance *p_instance;
};

SYS_MEM_BLOCKS_DEFINE_STATIC(scan_msg_allocator, sizeof(struct ctsu_scan_msg),
			     CONFIG_INPUT_RENESAS_RA_CTSU_MSG_MEM_BLOCK_SIZE, sizeof(uint32_t));

static void renesas_ra_callback_adapter(touch_callback_args_t *p_args)
{
	const struct device *dev = p_args->p_context;
	const struct renesas_ra_ctsu_group_cfg *cfg = dev->config;
	const struct device *ctsu_dev = cfg->ctsu_dev;
	struct renesas_ra_ctsu_data *ctsu_data = ctsu_dev->data;
	struct renesas_ra_ctsu_group_data *data = dev->data;

	if (p_args->event == CTSU_EVENT_SCAN_COMPLETE) {
		k_work_submit(&data->reading_work);
	}

	k_sem_give(&ctsu_data->scanning);
}

#define POLLING_INTERVAL_MS K_MSEC(CONFIG_INPUT_RENESAS_RA_CTSU_POLLING_INTERVAL_MS)
#define STABILIZATION_US    K_USEC(CONFIG_INPUT_RENESAS_RA_CTSU_STABILIZATION_TIME_US)

static void renesas_ra_ctsu_drv_handler(void *arg0, void *arg1, void *arg2)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	const struct device *ctsu_dev = arg0;
	struct renesas_ra_ctsu_data *ctsu_data = ctsu_dev->data;

	while (true) {
		struct ctsu_scan_msg *msg = k_queue_get(&ctsu_data->scan_q, K_FOREVER);
		struct st_touch_instance *p_instance = msg->p_instance;
		fsp_err_t err;

		k_sem_reset(&ctsu_data->scanning);
		err = p_instance->p_api->scanStart(p_instance->p_ctrl);
		if (err == FSP_SUCCESS) {
			k_sem_take(&ctsu_data->scanning, K_FOREVER);
		}

		sys_mem_blocks_free(&scan_msg_allocator, 1, (void **)&msg);

		k_sleep(STABILIZATION_US);
	}
}

static void renesas_ra_ctsu_group_sampling_handler(struct k_timer *timer)
{
	struct renesas_ra_ctsu_group_data *data =
		CONTAINER_OF(timer, struct renesas_ra_ctsu_group_data, sampling_timer);
	const struct device *ctsu_dev = k_timer_user_data_get(timer);
	struct renesas_ra_ctsu_data *ctsu_data = ctsu_dev->data;
	struct ctsu_scan_msg *msg;

	if (sys_mem_blocks_alloc(&scan_msg_allocator, 1, (void **)&msg) != 0) {
		return;
	}

	msg->p_instance = (void *)&data->touch_instance;
	k_queue_append(&ctsu_data->scan_q, msg);
}

static void renesas_ra_ctsu_group_buttons_read(const struct device *dev)
{
#if !DT_HAS_COMPAT_STATUS_OKAY(renesas_ra_ctsu_button)
	ARG_UNUSED(dev);
#else
	const struct renesas_ra_ctsu_group_cfg *cfg = dev->config;
	struct renesas_ra_ctsu_group_data *data = dev->data;

	if (cfg->num_button == 0) {
		return;
	}

	if (*data->p_button_status != 0) {
		uint64_t tmp_status = *data->p_button_status;

		while (tmp_status != 0) {
			int num = u64_count_trailing_zeros(tmp_status);
			struct renesas_ra_ctsu_device_cb *p_button_cb = &data->p_button_cb[num];

			p_button_cb->device_cb(p_button_cb->dev, NULL);
			WRITE_BIT(tmp_status, num, 0);
		}
	}
#endif /* DT_HAS_COMPAT_STATUS_OKAY(renesas_ra_ctsu_button) */
}

static void renesas_ra_ctsu_group_sliders_read(const struct device *dev)
{
#if !DT_HAS_COMPAT_STATUS_OKAY(renesas_ra_ctsu_slider)
	ARG_UNUSED(dev);
#else
	const struct renesas_ra_ctsu_group_cfg *cfg = dev->config;
	struct renesas_ra_ctsu_group_data *data = dev->data;

	if (cfg->num_slider == 0) {
		return;
	}

	for (int i = 0; i < cfg->num_slider; i++) {
		uint16_t slider_position = data->p_slider_position[i];

		if (slider_position != UINT16_MAX) {
			struct renesas_ra_ctsu_device_cb *p_slider_cb = &data->p_slider_cb[i];

			p_slider_cb->device_cb(p_slider_cb->dev, &slider_position);
		}
	}
#endif /* DT_HAS_COMPAT_STATUS_OKAY(renesas_ra_ctsu_slider) */
}

static void renesas_ra_ctsu_group_wheels_read(const struct device *dev)
{
#if !DT_HAS_COMPAT_STATUS_OKAY(renesas_ra_ctsu_wheel)
	ARG_UNUSED(dev);
#else
	const struct renesas_ra_ctsu_group_cfg *cfg = dev->config;
	struct renesas_ra_ctsu_group_data *data = dev->data;

	if (cfg->num_wheel == 0) {
		return;
	}

	for (int i = 0; i < cfg->num_wheel; i++) {
		uint16_t wheel_position = data->p_wheel_position[i];

		if (wheel_position != UINT16_MAX) {
			struct renesas_ra_ctsu_device_cb *p_wheel_cb = &data->p_wheel_cb[i];

			p_wheel_cb->device_cb(p_wheel_cb->dev, &wheel_position);
		}
	}
#endif /* DT_HAS_COMPAT_STATUS_OKAY(renesas_ra_ctsu_wheel) */
}

static void renesas_ra_ctsu_group_reading_handler(struct k_work *work)
{
	struct renesas_ra_ctsu_group_data *data =
		CONTAINER_OF(work, struct renesas_ra_ctsu_group_data, reading_work);
	const struct device *dev = data->dev;
	const struct st_touch_instance *p_instance = &data->touch_instance;
	fsp_err_t err;

	err = p_instance->p_api->dataGet(p_instance->p_ctrl, data->p_button_status,
					 data->p_slider_position, data->p_wheel_position);
	if (err != FSP_SUCCESS) {
		return;
	}

	renesas_ra_ctsu_group_buttons_read(dev);
	renesas_ra_ctsu_group_sliders_read(dev);
	renesas_ra_ctsu_group_wheels_read(dev);
}

static int input_renesas_ra_ctsu_group_configure(const struct device *dev,
						 const struct renesas_ra_ctsu_touch_cfg *cfg)
{
	const struct st_touch_instance *p_instance = &cfg->touch_instance;
	const struct renesas_ra_ctsu_group_cfg *config = dev->config;
	struct renesas_ra_ctsu_group_data *data = dev->data;
	fsp_err_t err;

	err = p_instance->p_api->open(p_instance->p_ctrl, p_instance->p_cfg);
	if (err != FSP_SUCCESS) {
		return -EIO;
	}

	err = p_instance->p_api->callbackSet(p_instance->p_ctrl, renesas_ra_callback_adapter,
					     (void *)dev, NULL);
	if (err != FSP_SUCCESS) {
		p_instance->p_api->close(p_instance->p_ctrl);
		return -EIO;
	}

#ifdef CONFIG_INPUT_RENESAS_RA_QE_TOUCH_CFG
	data->touch_instance = *p_instance;
#endif /* CONFIG_INPUT_RENESAS_RA_QE_TOUCH_CFG */

	k_work_init(&data->reading_work, renesas_ra_ctsu_group_reading_handler);
	k_timer_init(&data->sampling_timer, renesas_ra_ctsu_group_sampling_handler, NULL);
	k_timer_user_data_set(&data->sampling_timer, (void *)config->ctsu_dev);
	k_timer_start(&data->sampling_timer, POLLING_INTERVAL_MS, POLLING_INTERVAL_MS);

	return 0;
}

int z_impl_renesas_ra_ctsu_group_configure(const struct device *dev,
					   const struct renesas_ra_ctsu_touch_cfg *cfg)
{
#ifndef CONFIG_INPUT_RENESAS_RA_QE_TOUCH_CFG
	ARG_UNUSED(dev);
	ARG_UNUSED(cfg);
	return -ENOSYS;
#else
	return input_renesas_ra_ctsu_group_configure(dev, cfg);
#endif /* CONFIG_INPUT_RENESAS_RA_QE_TOUCH_CFG */
}

static int renesas_ra_ctsu_group_init(const struct device *dev)
{
	const struct renesas_ra_ctsu_group_cfg *cfg = dev->config;
#ifndef CONFIG_INPUT_RENESAS_RA_QE_TOUCH_CFG
	struct renesas_ra_ctsu_group_data *data = dev->data;
	const struct renesas_ra_ctsu_touch_cfg *touch_cfg =
		(const struct renesas_ra_ctsu_touch_cfg *)&data->touch_instance;
#endif /* CONFIG_INPUT_RENESAS_RA_QE_TOUCH_CFG */

	if (!device_is_ready(cfg->ctsu_dev)) {
		return -ENODEV;
	}

#ifndef CONFIG_INPUT_RENESAS_RA_QE_TOUCH_CFG
	return input_renesas_ra_ctsu_group_configure(dev, touch_cfg);
#else
	return 0;
#endif /* CONFIG_INPUT_RENESAS_RA_QE_TOUCH_CFG */
}

static void renesas_ra_ctsu_write_isr(void *arg)
{
	ARG_UNUSED(arg);
	ctsu_write_isr();
}

static void renesas_ra_ctsu_read_isr(void *arg)
{
	ARG_UNUSED(arg);
	ctsu_read_isr();
}

static void renesas_ra_ctsu_end_isr(void *arg)
{
	ARG_UNUSED(arg);
	ctsu_end_isr();
}

__maybe_unused static void ctsu_renesas_ra_button_cb(const struct device *dev, void *data)
{
	ARG_UNUSED(data);
	const struct ctsu_device_cfg *cfg = dev->config;

	input_report_key(dev, cfg->event_code, 0, false, K_NO_WAIT);
}

__maybe_unused static void ctsu_renesas_ra_slider_cb(const struct device *dev, void *data)
{
	const struct ctsu_device_cfg *cfg = dev->config;
	uint16_t *p_data = data;

	if (p_data == NULL) {
		return;
	}

	input_report_abs(dev, cfg->event_code, *p_data, false, K_NO_WAIT);
}

#define ctsu_renesas_ra_wheel_cb ctsu_renesas_ra_slider_cb

#define FOREACH_CHILD_CB(node_id, fn, compat)                                                      \
	IF_ENABLED(DT_NODE_HAS_COMPAT(node_id, compat), (fn(node_id)))

#define FOREACH_CHILD(node_id, fn, compat)                                                         \
	DT_FOREACH_CHILD_STATUS_OKAY_VARGS(node_id, FOREACH_CHILD_CB, fn, compat)

/* CTSU instance define */
#define DT_DRV_COMPAT renesas_ra_ctsu

/* CTSU group instance define */
#define CTSU_ELEMENT_CFG_GET_BY_IDX(idx, id)                                                       \
	{                                                                                          \
		.ssdiv = DT_ENUM_IDX_BY_IDX(id, ssdiv, idx),                                       \
		.so = DT_PROP_BY_IDX(id, so, idx),                                                 \
		.snum = DT_PROP_BY_IDX(id, snum, idx),                                             \
		.sdpa = DT_PROP_BY_IDX(id, sdpa, idx),                                             \
	}

#define RENESAS_RA_CTSU_ELEM_GET(idx, node_id) DT_PROP_BY_IDX(node_id, elements, idx)

#define CTSU_ELEM_IDX_DEFINE(node_id)                                                              \
	static uint8_t CONCAT(DT_NODE_FULL_NAME_TOKEN(node_id), _elem_index)[] = {                 \
		LISTIFY(DT_PROP_LEN(node_id, elements), RENESAS_RA_CTSU_ELEM_GET, (,), node_id)};

#define CTSU_DEVICE_BUTTON_CALLBACK_DEFINE(node_id)                                                \
	{                                                                                          \
		.dev = DEVICE_DT_GET(node_id),                                                     \
		.device_cb = ctsu_renesas_ra_button_cb,                                            \
	},

#define CTSU_DEVICE_SLIDER_CALLBACK_DEFINE(node_id)                                                \
	{                                                                                          \
		.dev = DEVICE_DT_GET(node_id),                                                     \
		.device_cb = ctsu_renesas_ra_slider_cb,                                            \
	},

#define CTSU_DEVICE_WHEEL_CALLBACK_DEFINE(node_id)                                                 \
	{                                                                                          \
		.dev = DEVICE_DT_GET(node_id),                                                     \
		.device_cb = ctsu_renesas_ra_wheel_cb,                                             \
	},

#define CTSU_BUTTON_DT_SPEC_GET(node_id)                                                           \
	{                                                                                          \
		.elem_index = DT_PROP(node_id, elements),                                          \
		.threshold = DT_PROP(node_id, threshold),                                          \
		.hysteresis = DT_PROP(node_id, hysteresis),                                        \
	},

#define CTSU_SLIDER_DT_SPEC_GET(node_id)                                                           \
	{                                                                                          \
		.p_elem_index = CONCAT(DT_NODE_FULL_NAME_TOKEN(node_id), _elem_index),             \
		.num_elements = ARRAY_SIZE(CONCAT(DT_NODE_FULL_NAME_TOKEN(node_id), _elem_index)), \
		.threshold = DT_PROP(node_id, threshold),                                          \
	},

#define CTSU_WHEEL_DT_SPEC_GET(node_id) CTSU_SLIDER_DT_SPEC_GET(node_id)

#define CTSU_GROUP_VAR_NAME(node_id, surfix)                                                       \
	CONCAT(renesas_ra_ctsu_, DT_NODE_FULL_NAME_TOKEN(node_id), surfix)

#define CTSU_ELEMENTS_DEFINE(node_id)                                                              \
	{LISTIFY(DT_PROP_LEN(node_id, ssdiv), CTSU_ELEMENT_CFG_GET_BY_IDX, (,), node_id)}

#ifndef CONFIG_INPUT_RENESAS_RA_QE_TOUCH_CFG
#define RENESAS_RA_CTSU_GROUP_DEFINE(id)                                                           \
	static const ctsu_element_cfg_t CTSU_GROUP_VAR_NAME(id, _elements_cfg)[] =                 \
		CTSU_ELEMENTS_DEFINE(id);                                                          \
                                                                                                   \
	FOREACH_CHILD(id, CTSU_ELEM_IDX_DEFINE, renesas_ra_ctsu_slider);                           \
	FOREACH_CHILD(id, CTSU_ELEM_IDX_DEFINE, renesas_ra_ctsu_wheel);                            \
                                                                                                   \
	static touch_button_cfg_t CTSU_GROUP_VAR_NAME(id, _button_cfg)[] = {                       \
		FOREACH_CHILD(id, CTSU_BUTTON_DT_SPEC_GET, renesas_ra_ctsu_button)};               \
                                                                                                   \
	static touch_slider_cfg_t CTSU_GROUP_VAR_NAME(id, _slider_cfg)[] = {                       \
		FOREACH_CHILD(id, CTSU_SLIDER_DT_SPEC_GET, renesas_ra_ctsu_slider)};               \
                                                                                                   \
	static touch_wheel_cfg_t CTSU_GROUP_VAR_NAME(id, _wheel_cfg)[] = {                         \
		FOREACH_CHILD(id, CTSU_WHEEL_DT_SPEC_GET, renesas_ra_ctsu_wheel)};                 \
                                                                                                   \
	struct renesas_ra_ctsu_device_cb CTSU_GROUP_VAR_NAME(id, _button_cb)[] = {                 \
		FOREACH_CHILD(id, CTSU_DEVICE_BUTTON_CALLBACK_DEFINE, renesas_ra_ctsu_button)};    \
	struct renesas_ra_ctsu_device_cb CTSU_GROUP_VAR_NAME(id, _slider_cb)[] = {                 \
		FOREACH_CHILD(id, CTSU_DEVICE_SLIDER_CALLBACK_DEFINE, renesas_ra_ctsu_slider)};    \
	struct renesas_ra_ctsu_device_cb CTSU_GROUP_VAR_NAME(id, _wheel_cb)[] = {                  \
		FOREACH_CHILD(id, CTSU_DEVICE_WHEEL_CALLBACK_DEFINE, renesas_ra_ctsu_wheel)};      \
                                                                                                   \
	struct renesas_ra_ctsu_group_cfg CONCAT(renesas_ra_ctsu_, DT_NODE_FULL_NAME_TOKEN(id),     \
						_cfg) = {                                          \
		.ctsu_dev = DEVICE_DT_GET(DT_PARENT(id)),                                          \
		.num_button = ARRAY_SIZE(CTSU_GROUP_VAR_NAME(id, _button_cfg)),                    \
		.num_slider = ARRAY_SIZE(CTSU_GROUP_VAR_NAME(id, _slider_cfg)),                    \
		.num_wheel = ARRAY_SIZE(CTSU_GROUP_VAR_NAME(id, _wheel_cfg)),                      \
	};                                                                                         \
                                                                                                   \
	static uint64_t CTSU_GROUP_VAR_NAME(id, _button_data);                                     \
	static uint16_t CTSU_GROUP_VAR_NAME(                                                       \
		id, _slider_data)[ARRAY_SIZE(CTSU_GROUP_VAR_NAME(id, _slider_cfg))];               \
	static uint16_t CTSU_GROUP_VAR_NAME(                                                       \
		id, _wheel_data)[ARRAY_SIZE(CTSU_GROUP_VAR_NAME(id, _wheel_cfg))];                 \
                                                                                                   \
	static struct renesas_ra_ctsu_group_data CTSU_GROUP_VAR_NAME(id, _data) = {                \
		.dev = DEVICE_DT_GET(id),                                                          \
		.touch_instance =                                                                  \
			{                                                                          \
				.p_ctrl = &CTSU_GROUP_VAR_NAME(id, _data).touch_ctrl,              \
				.p_cfg = &CTSU_GROUP_VAR_NAME(id, _data).touch_cfg,                \
				.p_api = &g_touch_on_ctsu,                                         \
			},                                                                         \
		.ctsu_instance =                                                                   \
			{                                                                          \
				.p_ctrl = &CTSU_GROUP_VAR_NAME(id, _data).ctsu_ctrl,               \
				.p_cfg = &CTSU_GROUP_VAR_NAME(id, _data).ctsu_cfg,                 \
				.p_api = &g_ctsu_on_ctsu,                                          \
			},                                                                         \
		.touch_cfg =                                                                       \
			{                                                                          \
				.on_freq = DT_PROP(id, on_freq),                                   \
				.off_freq = DT_PROP(id, off_freq),                                 \
				.drift_freq = DT_PROP(id, drift_freq),                             \
				.cancel_freq = DT_PROP(id, cancel_freq),                           \
				.p_ctsu_instance = &CTSU_GROUP_VAR_NAME(id, _data).ctsu_instance,  \
				.p_buttons = CTSU_GROUP_VAR_NAME(id, _button_cfg),                 \
				.p_sliders = CTSU_GROUP_VAR_NAME(id, _slider_cfg),                 \
				.p_wheels = CTSU_GROUP_VAR_NAME(id, _wheel_cfg),                   \
				.num_sliders = ARRAY_SIZE(CTSU_GROUP_VAR_NAME(id, _slider_cfg)),   \
				.num_wheels = ARRAY_SIZE(CTSU_GROUP_VAR_NAME(id, _wheel_cfg)),     \
				.num_buttons = ARRAY_SIZE(CTSU_GROUP_VAR_NAME(id, _button_cfg)),   \
			},                                                                         \
		.ctsu_cfg =                                                                        \
			{                                                                          \
				.cap = CTSU_CAP_SOFTWARE,                                          \
				.txvsel = DT_ENUM_IDX(DT_PARENT(id), pwr_supply_sel),              \
				.txvsel2 = DT_ENUM_IDX(DT_PARENT(id), pwr_supply_sel2),            \
				.atune1 = DT_ENUM_IDX(DT_PARENT(id), atune1),                      \
				.atune12 = DT_ENUM_IDX(DT_PARENT(id), atune12),                    \
				.md = CONCAT(CTSU_MODE_,                                           \
					     DT_STRING_UPPER_TOKEN(DT_PARENT(id), measure_mode)),  \
				.posel = DT_ENUM_IDX(DT_PARENT(id), po_sel),                       \
				.ctsuchac0 = DT_PROP_BY_IDX(id, ctsuchac, 0),                      \
				.ctsuchac1 = DT_PROP_BY_IDX(id, ctsuchac, 1),                      \
				.ctsuchac2 = DT_PROP_BY_IDX(id, ctsuchac, 2),                      \
				.ctsuchac3 = DT_PROP_BY_IDX(id, ctsuchac, 3),                      \
				.ctsuchac4 = DT_PROP_BY_IDX(id, ctsuchac, 4),                      \
				.ctsuchtrc0 = DT_PROP_BY_IDX(id, ctsuchtrc, 0),                    \
				.ctsuchtrc1 = DT_PROP_BY_IDX(id, ctsuchtrc, 1),                    \
				.ctsuchtrc2 = DT_PROP_BY_IDX(id, ctsuchtrc, 2),                    \
				.ctsuchtrc3 = DT_PROP_BY_IDX(id, ctsuchtrc, 3),                    \
				.ctsuchtrc4 = DT_PROP_BY_IDX(id, ctsuchtrc, 4),                    \
				.num_rx = DT_PROP(id, rx_count),                                   \
				.num_tx = DT_PROP(id, tx_count),                                   \
				.num_moving_average = DT_PROP(id, num_moving_avg),                 \
				.p_elements = CTSU_GROUP_VAR_NAME(id, _elements_cfg),              \
				.write_irq = DT_IRQ_BY_NAME(DT_PARENT(id), ctsuwr, irq),           \
				.read_irq = DT_IRQ_BY_NAME(DT_PARENT(id), ctsurd, irq),            \
				.end_irq = DT_IRQ_BY_NAME(DT_PARENT(id), ctsufn, irq),             \
			},                                                                         \
		.p_button_status = &CTSU_GROUP_VAR_NAME(id, _button_data),                         \
		.p_slider_position = CTSU_GROUP_VAR_NAME(id, _slider_data),                        \
		.p_wheel_position = CTSU_GROUP_VAR_NAME(id, _wheel_data),                          \
		.p_button_cb = CTSU_GROUP_VAR_NAME(id, _button_cb),                                \
		.p_slider_cb = CTSU_GROUP_VAR_NAME(id, _slider_cb),                                \
		.p_wheel_cb = CTSU_GROUP_VAR_NAME(id, _wheel_cb),                                  \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_DEFINE(id, renesas_ra_ctsu_group_init, NULL, &CTSU_GROUP_VAR_NAME(id, _data),    \
			 &CTSU_GROUP_VAR_NAME(id, _cfg), POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY,  \
			 NULL);
#else
#define RENESAS_RA_CTSU_GROUP_DEFINE(id)                                                           \
	struct renesas_ra_ctsu_device_cb CTSU_GROUP_VAR_NAME(id, _button_cb)[] = {                 \
		FOREACH_CHILD(id, CTSU_DEVICE_BUTTON_CALLBACK_DEFINE, renesas_ra_ctsu_button)};    \
	struct renesas_ra_ctsu_device_cb CTSU_GROUP_VAR_NAME(id, _slider_cb)[] = {                 \
		FOREACH_CHILD(id, CTSU_DEVICE_SLIDER_CALLBACK_DEFINE, renesas_ra_ctsu_slider)};    \
	struct renesas_ra_ctsu_device_cb CTSU_GROUP_VAR_NAME(id, _wheel_cb)[] = {                  \
		FOREACH_CHILD(id, CTSU_DEVICE_WHEEL_CALLBACK_DEFINE, renesas_ra_ctsu_wheel)};      \
                                                                                                   \
	struct renesas_ra_ctsu_group_cfg CTSU_GROUP_VAR_NAME(id, _cfg) = {                         \
		.ctsu_dev = DEVICE_DT_GET(DT_PARENT(id)),                                          \
		.num_button = ARRAY_SIZE(CTSU_GROUP_VAR_NAME(id, _button_cb)),                     \
		.num_slider = ARRAY_SIZE(CTSU_GROUP_VAR_NAME(id, _slider_cb)),                     \
		.num_wheel = ARRAY_SIZE(CTSU_GROUP_VAR_NAME(id, _wheel_cb)),                       \
	};                                                                                         \
                                                                                                   \
	static uint64_t CTSU_GROUP_VAR_NAME(id, _button_data);                                     \
	static uint16_t CTSU_GROUP_VAR_NAME(                                                       \
		id, _slider_data)[ARRAY_SIZE(CTSU_GROUP_VAR_NAME(id, _slider_cb))];                \
	static uint16_t CTSU_GROUP_VAR_NAME(                                                       \
		id, _wheel_data)[ARRAY_SIZE(CTSU_GROUP_VAR_NAME(id, _wheel_cb))];                  \
                                                                                                   \
	static struct renesas_ra_ctsu_group_data CTSU_GROUP_VAR_NAME(id, _data) = {                \
		.dev = DEVICE_DT_GET(id),                                                          \
		.p_button_status = &CTSU_GROUP_VAR_NAME(id, _button_data),                         \
		.p_slider_position = CTSU_GROUP_VAR_NAME(id, _slider_data),                        \
		.p_wheel_position = CTSU_GROUP_VAR_NAME(id, _wheel_data),                          \
		.p_button_cb = CTSU_GROUP_VAR_NAME(id, _button_cb),                                \
		.p_slider_cb = CTSU_GROUP_VAR_NAME(id, _slider_cb),                                \
		.p_wheel_cb = CTSU_GROUP_VAR_NAME(id, _wheel_cb),                                  \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_DEFINE(id, renesas_ra_ctsu_group_init, NULL, &CTSU_GROUP_VAR_NAME(id, _data),    \
			 &CTSU_GROUP_VAR_NAME(id, _cfg), POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY,  \
			 NULL);
#endif

static int renesas_ra_ctsu_init(const struct device *dev)
{
	const struct renesas_ra_ctsu_cfg *ctsu_cfg = dev->config;
	struct renesas_ra_ctsu_data *data = dev->data;
	k_tid_t tid;
	int ret;

	if (!device_is_ready(ctsu_cfg->clock)) {
		return -ENODEV;
	}

	/* Perform discharge process for the TSCAP pin */
	ret = gpio_pin_configure_dt(&ctsu_cfg->tscap_pin, GPIO_OUTPUT_LOW);
	if (ret != 0) {
		return ret;
	}

	/* Wait 10 usec for discharge to complete before switching to the CTSU pin function */
	k_busy_wait(10);

	ret = pinctrl_apply_state(ctsu_cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret != 0) {
		return ret;
	}

	ret = clock_control_on(ctsu_cfg->clock, (clock_control_subsys_t)&ctsu_cfg->clock_subsys);
	if (ret != 0) {
		return ret;
	}

	k_sem_init(&data->scanning, 0, 1);
	k_queue_init(&data->scan_q);

	tid = k_thread_create(
		&data->thread_data, data->thread_stack, K_THREAD_STACK_SIZEOF(data->thread_stack),
		renesas_ra_ctsu_drv_handler, (void *)dev, NULL, NULL,
		K_PRIO_COOP(CONFIG_INPUT_RENESAS_RA_CTSU_DRV_PRIORITY), K_ESSENTIAL, K_NO_WAIT);
	if (tid == NULL) {
		LOG_ERR("thread creation failed");
		return -ENODEV;
	}

	k_thread_name_set(&data->thread_data, dev->name);

	ctsu_cfg->irq_config();

	return 0;
}

#define RENESAS_RA_CTSU_DEFINE(inst)                                                               \
	PINCTRL_DT_INST_DEFINE(inst);                                                              \
                                                                                                   \
	static void renesas_ra_ctsu_irq_config##inst(void)                                         \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(inst, ctsuwr, irq),                                \
			    DT_INST_IRQ_BY_NAME(inst, ctsuwr, priority),                           \
			    renesas_ra_ctsu_write_isr, NULL, 0);                                   \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(inst, ctsurd, irq),                                \
			    DT_INST_IRQ_BY_NAME(inst, ctsurd, priority), renesas_ra_ctsu_read_isr, \
			    NULL, 0);                                                              \
		IRQ_CONNECT(DT_INST_IRQ_BY_NAME(inst, ctsufn, irq),                                \
			    DT_INST_IRQ_BY_NAME(inst, ctsufn, priority), renesas_ra_ctsu_end_isr,  \
			    NULL, 0);                                                              \
                                                                                                   \
		R_ICU->IELSR[DT_INST_IRQ_BY_NAME(inst, ctsuwr, irq)] =                             \
			BSP_PRV_IELS_ENUM(EVENT_CTSU_WRITE);                                       \
		R_ICU->IELSR[DT_INST_IRQ_BY_NAME(inst, ctsurd, irq)] =                             \
			BSP_PRV_IELS_ENUM(EVENT_CTSU_READ);                                        \
		R_ICU->IELSR[DT_INST_IRQ_BY_NAME(inst, ctsufn, irq)] =                             \
			BSP_PRV_IELS_ENUM(EVENT_CTSU_END);                                         \
                                                                                                   \
		irq_enable(DT_INST_IRQ_BY_NAME(inst, ctsuwr, irq));                                \
		irq_enable(DT_INST_IRQ_BY_NAME(inst, ctsurd, irq));                                \
		irq_enable(DT_INST_IRQ_BY_NAME(inst, ctsufn, irq));                                \
	}                                                                                          \
                                                                                                   \
	static const struct renesas_ra_ctsu_cfg renesas_ra_ctsu_cfg##inst = {                      \
		.tscap_pin = GPIO_DT_SPEC_INST_GET(inst, tscap_gpios),                             \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(inst),                                      \
		.clock = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),                                 \
		.clock_subsys =                                                                    \
			{                                                                          \
				.mstp = DT_INST_CLOCKS_CELL(inst, mstp),                           \
				.stop_bit = DT_INST_CLOCKS_CELL(inst, stop_bit),                   \
			},                                                                         \
		.irq_config = renesas_ra_ctsu_irq_config##inst,                                    \
	};                                                                                         \
                                                                                                   \
	static struct renesas_ra_ctsu_data renesas_ra_ctsu_data##inst;                             \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, renesas_ra_ctsu_init, NULL, &renesas_ra_ctsu_data##inst,       \
			      &renesas_ra_ctsu_cfg##inst, POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY, \
			      NULL);                                                               \
                                                                                                   \
	DT_INST_FOREACH_CHILD_STATUS_OKAY(inst, RENESAS_RA_CTSU_GROUP_DEFINE)

DT_INST_FOREACH_STATUS_OKAY(RENESAS_RA_CTSU_DEFINE)

static int ctsu_device_init(const struct device *dev)
{
	const struct ctsu_device_cfg *cfg = dev->config;

	return device_is_ready(cfg->group_dev) ? 0 : -ENODEV;
}

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT renesas_ra_ctsu_button

#define RENESAS_RA_CTSU_BUTTON_DEFINE(inst)                                                        \
	IF_ENABLED(DT_NODE_HAS_STATUS_OKAY(DT_INST_PARENT(inst)), (                                \
		const struct ctsu_device_cfg ctsu_button_cfg##inst = {                             \
			.group_dev = DEVICE_DT_GET(DT_INST_PARENT(inst)),                          \
			.event_code = DT_INST_PROP(inst, event_code),                              \
		};                                                                                 \
												   \
		DEVICE_DT_INST_DEFINE(inst, ctsu_device_init, NULL, NULL, &ctsu_button_cfg##inst,  \
				POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY, NULL);                    \
	))

DT_INST_FOREACH_STATUS_OKAY(RENESAS_RA_CTSU_BUTTON_DEFINE)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT renesas_ra_ctsu_slider

#define RENESAS_RA_CTSU_SLIDER_DEFINE(inst)                                                        \
	IF_ENABLED(DT_NODE_HAS_STATUS_OKAY(DT_INST_PARENT(inst)), (                                \
		const struct ctsu_device_cfg ctsu_slider_cfg##inst = {                             \
			.group_dev = DEVICE_DT_GET(DT_INST_PARENT(inst)),                          \
			.event_code = DT_INST_PROP(inst, event_code),                              \
		};                                                                                 \
												   \
		DEVICE_DT_INST_DEFINE(inst, ctsu_device_init, NULL, NULL, &ctsu_slider_cfg##inst,  \
				POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY, NULL);                    \
	))

DT_INST_FOREACH_STATUS_OKAY(RENESAS_RA_CTSU_SLIDER_DEFINE)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT renesas_ra_ctsu_wheel

#define RENESAS_RA_CTSU_WHEEL_DEFINE(inst)                                                         \
	IF_ENABLED(DT_NODE_HAS_STATUS_OKAY(DT_INST_PARENT(inst)), (                                \
		const struct ctsu_device_cfg ctsu_wheel_cfg##inst = {                              \
			.group_dev = DEVICE_DT_GET(DT_INST_PARENT(inst)),                          \
			.event_code = DT_INST_PROP(inst, event_code),                              \
		};                                                                                 \
												   \
		DEVICE_DT_INST_DEFINE(inst, ctsu_device_init, NULL, NULL, &ctsu_wheel_cfg##inst,   \
				POST_KERNEL, CONFIG_INPUT_INIT_PRIORITY, NULL);                    \
	))

DT_INST_FOREACH_STATUS_OKAY(RENESAS_RA_CTSU_WHEEL_DEFINE)
