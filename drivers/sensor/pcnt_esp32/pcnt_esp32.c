/*
 * Copyright (c) 2022 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_pcnt

/* Include esp-idf headers first to avoid redefining BIT() macro */
#include <hal/pcnt_hal.h>
#include <hal/pcnt_ll.h>
#include <hal/pcnt_types.h>

#include <soc.h>
#include <errno.h>
#include <string.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control.h>
#ifdef CONFIG_PCNT_ESP32_TRIGGER
#include <zephyr/drivers/interrupt_controller/intc_esp32.h>
#endif /* CONFIG_PCNT_ESP32_TRIGGER */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pcnt_esp32, CONFIG_SENSOR_LOG_LEVEL);

#define PCNT_INTR_UNIT_0  BIT(0)
#define PCNT_INTR_UNIT_1  BIT(1)
#define PCNT_INTR_UNIT_2  BIT(2)
#define PCNT_INTR_UNIT_3  BIT(3)
#ifdef CONFIG_SOC_ESP32
#define PCNT_INTR_UNIT_4  BIT(4)
#define PCNT_INTR_UNIT_5  BIT(5)
#define PCNT_INTR_UNIT_6  BIT(6)
#define PCNT_INTR_UNIT_7  BIT(7)
#endif /* CONFIG_SOC_ESP32 */

#ifdef CONFIG_PCNT_ESP32_TRIGGER
#define PCNT_INTR_THRES_1 BIT(2)
#define PCNT_INTR_THRES_0 BIT(3)
#endif /* CONFIG_PCNT_ESP32_TRIGGER */

struct pcnt_esp32_data {
	pcnt_hal_context_t hal;
	struct k_mutex cmd_mux;
#ifdef CONFIG_PCNT_ESP32_TRIGGER
	sensor_trigger_handler_t trigger_handler;
#endif /* CONFIG_PCNT_ESP32_TRIGGER */
};

struct pcnt_esp32_channel_config {
	uint8_t sig_pos_mode;
	uint8_t sig_neg_mode;
	uint8_t ctrl_h_mode;
	uint8_t ctrl_l_mode;
};

struct pcnt_esp32_unit_config {
	const uint8_t idx;
	int16_t filter;
	int16_t count_val_acc;
	struct pcnt_esp32_channel_config channel_config[2];
	int32_t h_thr;
	int32_t l_thr;
	int32_t offset;
};

struct pcnt_esp32_config {
	const struct pinctrl_dev_config *pincfg;
	const struct device *clock_dev;
	const clock_control_subsys_t clock_subsys;
	const int irq_src;
	struct pcnt_esp32_unit_config *unit_config;
	const int unit_len;
};

static int pcnt_esp32_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct pcnt_esp32_config *config = dev->config;
	struct pcnt_esp32_data *data = (struct pcnt_esp32_data *const)(dev)->data;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_ROTATION) {
		return -ENOTSUP;
	}

	k_mutex_lock(&data->cmd_mux, K_FOREVER);

	for (uint8_t i = 0; i < config->unit_len; i++) {
		struct pcnt_esp32_unit_config *unit_config = &config->unit_config[i];

		unit_config->count_val_acc = pcnt_ll_get_count(data->hal.dev, i);
	}

	k_mutex_unlock(&data->cmd_mux);

	return 0;
}

static int pcnt_esp32_channel_get(const struct device *dev, enum sensor_channel chan,
				  struct sensor_value *val)
{
	int ret = 0;
	const struct pcnt_esp32_config *config = dev->config;
	struct pcnt_esp32_data *data = (struct pcnt_esp32_data *const)(dev)->data;

	k_mutex_lock(&data->cmd_mux, K_FOREVER);

	if (chan == SENSOR_CHAN_ROTATION) {
		val->val1 = config->unit_config[0].count_val_acc + config->unit_config[0].offset;
		val->val2 = 0;
	} else {
		ret = -ENOTSUP;
	}

	k_mutex_unlock(&data->cmd_mux);

	return ret;
}

static int pcnt_esp32_configure_pinctrl(const struct device *dev)
{
	int ret;
	struct pcnt_esp32_config *config = (struct pcnt_esp32_config *)dev->config;

	return pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
}

int pcnt_esp32_init(const struct device *dev)
{
	int ret;
	const struct pcnt_esp32_config *config = dev->config;
	struct pcnt_esp32_data *data = (struct pcnt_esp32_data *const)(dev)->data;

	ret = clock_control_on(config->clock_dev, config->clock_subsys);
	if (ret < 0) {
		LOG_ERR("Could not initialize clock (%d)", ret);
		return ret;
	}

	ret = pcnt_esp32_configure_pinctrl(dev);
	if (ret < 0) {
		LOG_ERR("PWM pinctrl setup failed (%d)", ret);
		return ret;
	}

	pcnt_hal_init(&data->hal, 0);

	for (uint8_t i = 0; i < config->unit_len; i++) {
		struct pcnt_esp32_unit_config *unit_config = &config->unit_config[i];

		unit_config->h_thr = 0;
		unit_config->l_thr = 0;
		unit_config->offset = 0;

		pcnt_ll_enable_thres_event(data->hal.dev, i, 0, false);
		pcnt_ll_enable_thres_event(data->hal.dev, i, 1, false);
		pcnt_ll_enable_low_limit_event(data->hal.dev, i, false);
		pcnt_ll_enable_high_limit_event(data->hal.dev, i, false);
		pcnt_ll_enable_zero_cross_event(data->hal.dev, i, false);
		pcnt_ll_set_edge_action(data->hal.dev, i, 0,
					unit_config->channel_config[0].sig_pos_mode,
					unit_config->channel_config[0].sig_neg_mode);
		pcnt_ll_set_edge_action(data->hal.dev, i, 1,
					unit_config->channel_config[1].sig_pos_mode,
					unit_config->channel_config[1].sig_neg_mode);
		pcnt_ll_set_level_action(data->hal.dev, i, 0,
					 unit_config->channel_config[0].ctrl_h_mode,
					 unit_config->channel_config[0].ctrl_l_mode);
		pcnt_ll_set_level_action(data->hal.dev, i, 1,
					 unit_config->channel_config[1].ctrl_h_mode,
					 unit_config->channel_config[1].ctrl_l_mode);
		pcnt_ll_clear_count(data->hal.dev, i);

		pcnt_ll_set_glitch_filter_thres(data->hal.dev, i, unit_config->filter);
		pcnt_ll_enable_glitch_filter(data->hal.dev, i, (bool)unit_config->filter);

		pcnt_ll_start_count(data->hal.dev, i);
	}

	return 0;
}

static int pcnt_esp32_attr_set_thresh(const struct device *dev, enum sensor_attribute attr,
				      const struct sensor_value *val)
{
	const struct pcnt_esp32_config *config = dev->config;
	struct pcnt_esp32_data *data = (struct pcnt_esp32_data *const)(dev)->data;

	for (uint8_t i = 0; i < config->unit_len; i++) {
		struct pcnt_esp32_unit_config *unit_config = &config->unit_config[i];

		switch (attr) {
		case SENSOR_ATTR_LOWER_THRESH:
			unit_config->l_thr = val->val1;
			pcnt_ll_set_thres_value(data->hal.dev, i, 0, unit_config->l_thr);
			pcnt_ll_enable_thres_event(data->hal.dev, i, 0, true);
			break;
		case SENSOR_ATTR_UPPER_THRESH:
			unit_config->h_thr = val->val1;
			pcnt_ll_set_thres_value(data->hal.dev, i, 1, unit_config->h_thr);
			pcnt_ll_enable_thres_event(data->hal.dev, i, 1, true);
			break;
		default:
			return -ENOTSUP;
		}
		pcnt_ll_stop_count(data->hal.dev, i);
		pcnt_ll_clear_count(data->hal.dev, i);
		pcnt_ll_start_count(data->hal.dev, i);
	}

	return 0;
}

static int pcnt_esp32_attr_set_offset(const struct device *dev, const struct sensor_value *val)
{
	const struct pcnt_esp32_config *config = dev->config;

	for (uint8_t i = 0; i < config->unit_len; i++) {
		struct pcnt_esp32_unit_config *unit_config = &config->unit_config[i];

		unit_config->offset = val->val1;
	}

	return 0;
}

static int pcnt_esp32_attr_set(const struct device *dev, enum sensor_channel chan,
			       enum sensor_attribute attr, const struct sensor_value *val)
{
	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_ROTATION) {
		return -ENOTSUP;
	}

	switch (attr) {
	case SENSOR_ATTR_LOWER_THRESH:
	case SENSOR_ATTR_UPPER_THRESH:
		return pcnt_esp32_attr_set_thresh(dev, attr, val);
	case SENSOR_ATTR_OFFSET:
		return pcnt_esp32_attr_set_offset(dev, val);
	default:
		return -ENOTSUP;
	}

	return 0;
}

static int pcnt_esp32_attr_get(const struct device *dev, enum sensor_channel chan,
			       enum sensor_attribute attr, struct sensor_value *val)
{
	const struct pcnt_esp32_config *config = dev->config;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_ROTATION) {
		return -ENOTSUP;
	}

	switch (attr) {
	case SENSOR_ATTR_LOWER_THRESH:
		val->val1 = config->unit_config[0].l_thr;
		break;
	case SENSOR_ATTR_UPPER_THRESH:
		val->val1 = config->unit_config[0].h_thr;
		break;
	case SENSOR_ATTR_OFFSET:
		val->val1 = config->unit_config[0].offset;
		break;
	default:
		return -ENOTSUP;
	}

	val->val2 = 0;

	return 0;
}

#ifdef CONFIG_PCNT_ESP32_TRIGGER
static void IRAM_ATTR pcnt_esp32_isr(const struct device *dev)
{
	struct pcnt_esp32_data *data = (struct pcnt_esp32_data *const)(dev)->data;

	uint32_t pcnt_intr_status;
	uint32_t pcnt_unit_status;
	struct sensor_trigger trigger;

	pcnt_intr_status = pcnt_ll_get_intr_status(data->hal.dev);
	pcnt_ll_clear_intr_status(data->hal.dev, pcnt_intr_status);

	if (pcnt_intr_status & PCNT_INTR_UNIT_0) {
		pcnt_unit_status = pcnt_ll_get_unit_status(data->hal.dev, 0);
	} else if (pcnt_intr_status & PCNT_INTR_UNIT_1) {
		pcnt_unit_status = pcnt_ll_get_unit_status(data->hal.dev, 1);
	} else if (pcnt_intr_status & PCNT_INTR_UNIT_2) {
		pcnt_unit_status = pcnt_ll_get_unit_status(data->hal.dev, 2);
	} else if (pcnt_intr_status & PCNT_INTR_UNIT_3) {
		pcnt_unit_status = pcnt_ll_get_unit_status(data->hal.dev, 3);
#ifdef CONFIG_SOC_ESP32
	} else if (pcnt_intr_status & PCNT_INTR_UNIT_4) {
		pcnt_unit_status = pcnt_ll_get_unit_status(data->hal.dev, 4);
	} else if (pcnt_intr_status & PCNT_INTR_UNIT_5) {
		pcnt_unit_status = pcnt_ll_get_unit_status(data->hal.dev, 5);
	} else if (pcnt_intr_status & PCNT_INTR_UNIT_6) {
		pcnt_unit_status = pcnt_ll_get_unit_status(data->hal.dev, 6);
	} else if (pcnt_intr_status & PCNT_INTR_UNIT_7) {
		pcnt_unit_status = pcnt_ll_get_unit_status(data->hal.dev, 7);
#endif /* CONFIG_SOC_ESP32 */
	} else {
		return;
	}

	if (!(pcnt_unit_status & PCNT_INTR_THRES_0) && !(pcnt_unit_status & PCNT_INTR_THRES_1)) {
		return;
	}

	if (!data->trigger_handler) {
		return;
	}

	trigger.type = SENSOR_TRIG_THRESHOLD;
	trigger.chan = SENSOR_CHAN_ROTATION;
	data->trigger_handler(dev, &trigger);
}

static int pcnt_esp32_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
				  sensor_trigger_handler_t handler)
{
	int ret;
	const struct pcnt_esp32_config *config = dev->config;
	struct pcnt_esp32_data *data = (struct pcnt_esp32_data *const)(dev)->data;

	if (trig->type != SENSOR_TRIG_THRESHOLD) {
		return -ENOTSUP;
	}

	if ((trig->chan != SENSOR_CHAN_ALL) && (trig->chan != SENSOR_CHAN_ROTATION)) {
		return -ENOTSUP;
	}

	if (!handler) {
		return -EINVAL;
	}

	data->trigger_handler = handler;

	ret = esp_intr_alloc(config->irq_src, 0, (intr_handler_t)pcnt_esp32_isr, (void *)dev, NULL);
	if (ret != 0) {
		LOG_ERR("pcnt isr registration failed (%d)", ret);
		return ret;
	}

	pcnt_ll_enable_intr(data->hal.dev, 1, true);

	return 0;
}
#endif /* CONFIG_PCNT_ESP32_TRIGGER */

static const struct sensor_driver_api pcnt_esp32_api = {
	.sample_fetch = pcnt_esp32_sample_fetch,
	.channel_get = pcnt_esp32_channel_get,
	.attr_set = pcnt_esp32_attr_set,
	.attr_get = pcnt_esp32_attr_get,
#ifdef CONFIG_PCNT_ESP32_TRIGGER
	.trigger_set = pcnt_esp32_trigger_set,
#endif /* CONFIG_PCNT_ESP32_TRIGGER */
};

PINCTRL_DT_INST_DEFINE(0);

#define UNIT_CONFIG(node_id)                                                                       \
	{                                                                                          \
		.idx = DT_REG_ADDR(node_id),                                                       \
		.filter = DT_PROP_OR(node_id, filter, 0) > 1024 ? 1024                             \
								: DT_PROP_OR(node_id, filter, 0),  \
		.channel_config[0] =                                                               \
			{                                                                          \
				.sig_pos_mode = DT_PROP_OR(DT_CHILD(node_id, channela_0),          \
							   sig_pos_mode, 0),                       \
				.sig_neg_mode = DT_PROP_OR(DT_CHILD(node_id, channela_0),          \
							   sig_neg_mode, 0),                       \
				.ctrl_l_mode =                                                     \
					DT_PROP_OR(DT_CHILD(node_id, channela_0), ctrl_l_mode, 0), \
				.ctrl_h_mode =                                                     \
					DT_PROP_OR(DT_CHILD(node_id, channela_0), ctrl_h_mode, 0), \
			},                                                                         \
		.channel_config[1] =                                                               \
			{                                                                          \
				.sig_pos_mode = DT_PROP_OR(DT_CHILD(node_id, channelb_0),          \
							   sig_pos_mode, 0),                       \
				.sig_neg_mode = DT_PROP_OR(DT_CHILD(node_id, channelb_0),          \
							   sig_neg_mode, 0),                       \
				.ctrl_l_mode =                                                     \
					DT_PROP_OR(DT_CHILD(node_id, channelb_0), ctrl_l_mode, 0), \
				.ctrl_h_mode =                                                     \
					DT_PROP_OR(DT_CHILD(node_id, channelb_0), ctrl_h_mode, 0), \
			},                                                                         \
	},

static struct pcnt_esp32_unit_config unit_config[] = {DT_INST_FOREACH_CHILD(0, UNIT_CONFIG)};

static struct pcnt_esp32_config pcnt_esp32_config = {
	.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
	.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(0)),
	.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(0, offset),
	.irq_src = DT_INST_IRQN(0),
	.unit_config = unit_config,
	.unit_len = ARRAY_SIZE(unit_config),
};

static struct pcnt_esp32_data pcnt_esp32_data = {
	.hal = {
		.dev = (pcnt_dev_t *)DT_INST_REG_ADDR(0),
	},
	.cmd_mux = Z_MUTEX_INITIALIZER(pcnt_esp32_data.cmd_mux),
#ifdef CONFIG_PCNT_ESP32_TRIGGER
	.trigger_handler = NULL,
#endif /* CONFIG_PCNT_ESP32_TRIGGER */
};

DEVICE_DT_INST_DEFINE(0, &pcnt_esp32_init, NULL,
			&pcnt_esp32_data,
			&pcnt_esp32_config,
			POST_KERNEL,
			CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
			&pcnt_esp32_api);
