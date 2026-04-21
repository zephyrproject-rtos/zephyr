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
#endif

#ifdef CONFIG_SENSOR_ASYNC_API
#include <zephyr/rtio/rtio.h>
#include <zephyr/drivers/sensor_clock.h>
#endif

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pcnt_esp32, CONFIG_SENSOR_LOG_LEVEL);

#ifdef CONFIG_SOC_SERIES_ESP32
#define PCNT_ESP32_MAX_UNITS 8
#else
#define PCNT_ESP32_MAX_UNITS 4
#endif

#ifdef CONFIG_PCNT_ESP32_TRIGGER
#define PCNT_INTR_THRES_1 BIT(2)
#define PCNT_INTR_THRES_0 BIT(3)
#define PCNT_INTR_L_LIM   BIT(4)
#define PCNT_INTR_H_LIM   BIT(5)
#define PCNT_INTR_ZERO    BIT(6)
#endif

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
	uint32_t counts_per_rev;
	struct pcnt_esp32_channel_config channel_config[2];
	int32_t h_thr;
	int32_t l_thr;
	int32_t offset;
	int32_t h_limit;
	int32_t l_limit;
	bool h_limit_set;
	bool l_limit_set;
	bool zero_cross_en;
#ifdef CONFIG_PCNT_ESP32_TRIGGER
	sensor_trigger_handler_t trigger_handler;
	const struct sensor_trigger *trigger;
#endif
};

struct pcnt_esp32_config {
	const struct pinctrl_dev_config *pincfg;
	const struct device *clock_dev;
	const clock_control_subsys_t clock_subsys;
	const int irq_source;
	const int irq_priority;
	const int irq_flags;
	struct pcnt_esp32_unit_config *unit_config;
	const int unit_len;
};

struct pcnt_esp32_data {
	pcnt_hal_context_t hal;
	struct k_mutex cmd_mux;
};

static inline struct pcnt_esp32_unit_config *
pcnt_esp32_find_unit(const struct pcnt_esp32_config *config, uint16_t chan_idx)
{
	for (uint8_t i = 0; i < config->unit_len; i++) {
		if (config->unit_config[i].idx == chan_idx) {
			return &config->unit_config[i];
		}
	}
	return NULL;
}

static int pcnt_esp32_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	const struct pcnt_esp32_config *config = dev->config;
	struct pcnt_esp32_data *data = dev->data;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_ROTATION &&
	    chan != SENSOR_CHAN_ENCODER_COUNT) {
		return -ENOTSUP;
	}

	k_mutex_lock(&data->cmd_mux, K_FOREVER);

	for (uint8_t i = 0; i < config->unit_len; i++) {
		struct pcnt_esp32_unit_config *unit_config = &config->unit_config[i];

		unit_config->count_val_acc = pcnt_ll_get_count(data->hal.dev, unit_config->idx);
	}

	k_mutex_unlock(&data->cmd_mux);

	return 0;
}

static void pcnt_esp32_count_to_value(const struct pcnt_esp32_unit_config *unit_config,
				      enum sensor_channel chan, struct sensor_value *val)
{
	int32_t raw = unit_config->count_val_acc + unit_config->offset;

	if (chan == SENSOR_CHAN_ROTATION && unit_config->counts_per_rev > 0) {
		int32_t cpr = (int32_t)unit_config->counts_per_rev;
		int32_t wrapped = raw % cpr;
		int64_t deg_micro;

		if (wrapped < 0) {
			wrapped += cpr;
		}
		deg_micro = ((int64_t)wrapped * 360 * 1000000) / cpr;

		val->val1 = (int32_t)(deg_micro / 1000000);
		val->val2 = (int32_t)(deg_micro % 1000000);
	} else {
		val->val1 = raw;
		val->val2 = 0;
	}
}

static int pcnt_esp32_channel_get(const struct device *dev, enum sensor_channel chan,
				  struct sensor_value *val)
{
	int ret = 0;
	const struct pcnt_esp32_config *config = dev->config;
	struct pcnt_esp32_data *data = dev->data;

	k_mutex_lock(&data->cmd_mux, K_FOREVER);

	if (chan == SENSOR_CHAN_ROTATION || chan == SENSOR_CHAN_ENCODER_COUNT) {
		pcnt_esp32_count_to_value(&config->unit_config[0], chan, val);
	} else {
		ret = -ENOTSUP;
	}

	k_mutex_unlock(&data->cmd_mux);

	return ret;
}

static int pcnt_esp32_configure_pinctrl(const struct device *dev)
{
	struct pcnt_esp32_config *config = (struct pcnt_esp32_config *)dev->config;

	return pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT);
}

#ifdef CONFIG_PCNT_ESP32_TRIGGER
static void IRAM_ATTR pcnt_esp32_isr(const struct device *dev);
#endif

int pcnt_esp32_init(const struct device *dev)
{
	int ret;
	const struct pcnt_esp32_config *config = dev->config;
	struct pcnt_esp32_data *data = dev->data;

	ret = clock_control_on(config->clock_dev, config->clock_subsys);
	if (ret < 0) {
		LOG_ERR("Could not initialize clock (%d)", ret);
		return ret;
	}

	ret = pcnt_esp32_configure_pinctrl(dev);
	if (ret < 0) {
		LOG_ERR("pinctrl setup failed (%d)", ret);
		return ret;
	}

	pcnt_hal_init(&data->hal, 0);

	for (uint8_t i = 0; i < config->unit_len; i++) {
		struct pcnt_esp32_unit_config *unit_config = &config->unit_config[i];
		uint8_t u = unit_config->idx;

		unit_config->h_thr = 0;
		unit_config->l_thr = 0;
		unit_config->offset = 0;

		pcnt_ll_enable_thres_event(data->hal.dev, u, 0, false);
		pcnt_ll_enable_thres_event(data->hal.dev, u, 1, false);
		pcnt_ll_enable_low_limit_event(data->hal.dev, u, false);
		pcnt_ll_enable_high_limit_event(data->hal.dev, u, false);
		pcnt_ll_enable_zero_cross_event(data->hal.dev, u, false);

		/*
		 * Always program the h/l limit values even when the limit events
		 * are disabled: PCNT clamps the counter to these limits whenever
		 * the comparator is active, and leaving them at their default
		 * (typically 0) makes the counter saturate at zero.
		 */
		pcnt_ll_set_high_limit_value(data->hal.dev, u,
					     unit_config->h_limit_set ? unit_config->h_limit
								      : INT16_MAX);
		pcnt_ll_set_low_limit_value(data->hal.dev, u,
					    unit_config->l_limit_set ? unit_config->l_limit
								     : INT16_MIN);

		if (unit_config->h_limit_set) {
			pcnt_ll_enable_high_limit_event(data->hal.dev, u, true);
		}
		if (unit_config->l_limit_set) {
			pcnt_ll_enable_low_limit_event(data->hal.dev, u, true);
		}
		if (unit_config->zero_cross_en) {
			pcnt_ll_enable_zero_cross_event(data->hal.dev, u, true);
		}

		pcnt_ll_set_edge_action(data->hal.dev, u, 0,
					unit_config->channel_config[0].sig_pos_mode,
					unit_config->channel_config[0].sig_neg_mode);
		pcnt_ll_set_edge_action(data->hal.dev, u, 1,
					unit_config->channel_config[1].sig_pos_mode,
					unit_config->channel_config[1].sig_neg_mode);
		pcnt_ll_set_level_action(data->hal.dev, u, 0,
					 unit_config->channel_config[0].ctrl_h_mode,
					 unit_config->channel_config[0].ctrl_l_mode);
		pcnt_ll_set_level_action(data->hal.dev, u, 1,
					 unit_config->channel_config[1].ctrl_h_mode,
					 unit_config->channel_config[1].ctrl_l_mode);
		pcnt_ll_clear_count(data->hal.dev, u);

		pcnt_ll_set_glitch_filter_thres(data->hal.dev, u, unit_config->filter);
		pcnt_ll_enable_glitch_filter(data->hal.dev, u, (bool)unit_config->filter);

		pcnt_ll_start_count(data->hal.dev, u);
	}

#ifdef CONFIG_PCNT_ESP32_TRIGGER
	ret = esp_intr_alloc(config->irq_source,
			     ESP_PRIO_TO_FLAGS(config->irq_priority) |
				     ESP_INT_FLAGS_CHECK(config->irq_flags) | ESP_INTR_FLAG_IRAM,
			     (intr_handler_t)pcnt_esp32_isr, (void *)dev, NULL);
	if (ret != 0) {
		LOG_ERR("pcnt isr alloc failed (%d)", ret);
		return ret;
	}
#endif

	return 0;
}

static int pcnt_esp32_attr_set_thresh(const struct device *dev, uint16_t chan_idx,
				      enum sensor_attribute attr, const struct sensor_value *val)
{
	const struct pcnt_esp32_config *config = dev->config;
	struct pcnt_esp32_data *data = dev->data;
	struct pcnt_esp32_unit_config *unit_config;
	uint8_t u;

	unit_config = pcnt_esp32_find_unit(config, chan_idx);
	if (unit_config == NULL) {
		return -EINVAL;
	}

	u = unit_config->idx;

	k_mutex_lock(&data->cmd_mux, K_FOREVER);

	switch (attr) {
	case SENSOR_ATTR_LOWER_THRESH:
		unit_config->l_thr = val->val1;
		pcnt_ll_stop_count(data->hal.dev, u);
		pcnt_ll_set_thres_value(data->hal.dev, u, 0, unit_config->l_thr);
		pcnt_ll_enable_thres_event(data->hal.dev, u, 0, true);
		pcnt_ll_start_count(data->hal.dev, u);
		break;
	case SENSOR_ATTR_UPPER_THRESH:
		unit_config->h_thr = val->val1;
		pcnt_ll_stop_count(data->hal.dev, u);
		pcnt_ll_set_thres_value(data->hal.dev, u, 1, unit_config->h_thr);
		pcnt_ll_enable_thres_event(data->hal.dev, u, 1, true);
		pcnt_ll_start_count(data->hal.dev, u);
		break;
	default:
		k_mutex_unlock(&data->cmd_mux);
		return -ENOTSUP;
	}

	k_mutex_unlock(&data->cmd_mux);
	return 0;
}

static int pcnt_esp32_attr_set(const struct device *dev, enum sensor_channel chan,
			       enum sensor_attribute attr, const struct sensor_value *val)
{
	const struct pcnt_esp32_config *config = dev->config;
	struct pcnt_esp32_data *data = dev->data;
	struct pcnt_esp32_unit_config *unit_config;
	uint16_t chan_idx;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_ROTATION &&
	    chan != SENSOR_CHAN_ENCODER_COUNT &&
	    ((int)chan < (int)SENSOR_CHAN_PRIV_START ||
	     (int)chan >= (int)SENSOR_CHAN_PRIV_START + PCNT_ESP32_MAX_UNITS)) {
		return -ENOTSUP;
	}

	if ((int)chan >= (int)SENSOR_CHAN_PRIV_START) {
		chan_idx = (uint16_t)((int)chan - (int)SENSOR_CHAN_PRIV_START);
	} else {
		chan_idx = 0;
	}

	unit_config = pcnt_esp32_find_unit(config, chan_idx);
	if (unit_config == NULL) {
		return -EINVAL;
	}

	switch (attr) {
	case SENSOR_ATTR_LOWER_THRESH:
	case SENSOR_ATTR_UPPER_THRESH:
		return pcnt_esp32_attr_set_thresh(dev, unit_config->idx, attr, val);
	case SENSOR_ATTR_OFFSET:
		unit_config->offset = val->val1;
		return 0;
	case SENSOR_ATTR_PRIV_START: /* SENSOR_ATTR_PCNT_ESP32_RESET */
		k_mutex_lock(&data->cmd_mux, K_FOREVER);
		pcnt_ll_clear_count(data->hal.dev, unit_config->idx);
		unit_config->count_val_acc = 0;
		k_mutex_unlock(&data->cmd_mux);
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int pcnt_esp32_attr_get(const struct device *dev, enum sensor_channel chan,
			       enum sensor_attribute attr, struct sensor_value *val)
{
	const struct pcnt_esp32_config *config = dev->config;
	struct pcnt_esp32_unit_config *unit_config;
	uint16_t chan_idx;

	if (chan != SENSOR_CHAN_ALL && chan != SENSOR_CHAN_ROTATION &&
	    chan != SENSOR_CHAN_ENCODER_COUNT &&
	    ((int)chan < (int)SENSOR_CHAN_PRIV_START ||
	     (int)chan >= (int)SENSOR_CHAN_PRIV_START + PCNT_ESP32_MAX_UNITS)) {
		return -ENOTSUP;
	}

	if ((int)chan >= (int)SENSOR_CHAN_PRIV_START) {
		chan_idx = (uint16_t)((int)chan - (int)SENSOR_CHAN_PRIV_START);
	} else {
		chan_idx = 0;
	}

	unit_config = pcnt_esp32_find_unit(config, chan_idx);
	if (unit_config == NULL) {
		return -EINVAL;
	}

	switch (attr) {
	case SENSOR_ATTR_LOWER_THRESH:
		val->val1 = unit_config->l_thr;
		break;
	case SENSOR_ATTR_UPPER_THRESH:
		val->val1 = unit_config->h_thr;
		break;
	case SENSOR_ATTR_OFFSET:
		val->val1 = unit_config->offset;
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
	const struct pcnt_esp32_config *config = dev->config;
	struct pcnt_esp32_data *data = dev->data;
	uint32_t intr_status;

	intr_status = pcnt_ll_get_intr_status(data->hal.dev);
	pcnt_ll_clear_intr_status(data->hal.dev, intr_status);

	for (uint8_t i = 0; i < config->unit_len; i++) {
		struct pcnt_esp32_unit_config *unit_config = &config->unit_config[i];
		uint8_t u = unit_config->idx;
		uint32_t unit_status;
		uint32_t event_mask = PCNT_INTR_THRES_0 | PCNT_INTR_THRES_1 | PCNT_INTR_L_LIM |
				      PCNT_INTR_H_LIM | PCNT_INTR_ZERO;

		if (!(intr_status & BIT(u))) {
			continue;
		}

		unit_status = pcnt_ll_get_unit_status(data->hal.dev, u);

		if (!(unit_status & event_mask)) {
			continue;
		}

		if (unit_config->trigger_handler != NULL) {
			unit_config->trigger_handler(dev, unit_config->trigger);
		}
	}
}

static int pcnt_esp32_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
				  sensor_trigger_handler_t handler)
{
	const struct pcnt_esp32_config *config = dev->config;
	struct pcnt_esp32_data *data = dev->data;
	struct pcnt_esp32_unit_config *unit_config;
	uint16_t chan_idx;

	if (trig->type != SENSOR_TRIG_THRESHOLD) {
		return -ENOTSUP;
	}

	if (trig->chan != SENSOR_CHAN_ALL && trig->chan != SENSOR_CHAN_ROTATION &&
	    trig->chan != SENSOR_CHAN_ENCODER_COUNT &&
	    ((int)trig->chan < (int)SENSOR_CHAN_PRIV_START ||
	     (int)trig->chan >= (int)SENSOR_CHAN_PRIV_START + PCNT_ESP32_MAX_UNITS)) {
		return -ENOTSUP;
	}

	if (handler == NULL) {
		return -EINVAL;
	}

	if ((int)trig->chan >= (int)SENSOR_CHAN_PRIV_START) {
		chan_idx = (uint16_t)((int)trig->chan - (int)SENSOR_CHAN_PRIV_START);
	} else {
		chan_idx = 0;
	}

	unit_config = pcnt_esp32_find_unit(config, chan_idx);
	if (unit_config == NULL) {
		return -EINVAL;
	}

	unit_config->trigger_handler = handler;
	unit_config->trigger = trig;

	/*
	 * Ensure any stale event latched in the raw status is cleared, then arm
	 * the CPU-side interrupt for this unit. The watchpoint (threshold / h-lim
	 * / l-lim / zero-cross) must already have been configured via
	 * attr_set or a DT property; this call only connects the existing event
	 * chain to the interrupt controller.
	 */
	pcnt_ll_stop_count(data->hal.dev, unit_config->idx);
	pcnt_ll_clear_count(data->hal.dev, unit_config->idx);
	pcnt_ll_clear_intr_status(data->hal.dev, BIT(unit_config->idx));
	pcnt_ll_enable_intr(data->hal.dev, BIT(unit_config->idx), true);
	pcnt_ll_start_count(data->hal.dev, unit_config->idx);
	return 0;
}
#endif /* CONFIG_PCNT_ESP32_TRIGGER */

#ifdef CONFIG_SENSOR_ASYNC_API
struct pcnt_esp32_encoded_data {
	uint64_t timestamp_ns;
	uint8_t num_units;
	int32_t counts[PCNT_ESP32_MAX_UNITS];
	uint32_t counts_per_rev[PCNT_ESP32_MAX_UNITS];
	uint8_t unit_idx[PCNT_ESP32_MAX_UNITS];
};

static int pcnt_esp32_decoder_get_frame_count(const uint8_t *buffer,
					      struct sensor_chan_spec chan_spec,
					      uint16_t *frame_count)
{
	const struct pcnt_esp32_encoded_data *edata =
		(const struct pcnt_esp32_encoded_data *)buffer;

	if (chan_spec.chan_type != SENSOR_CHAN_ROTATION &&
	    chan_spec.chan_type != SENSOR_CHAN_ENCODER_COUNT) {
		return -ENOTSUP;
	}

	for (uint8_t i = 0; i < edata->num_units; i++) {
		if (edata->unit_idx[i] == chan_spec.chan_idx) {
			*frame_count = 1;
			return 0;
		}
	}

	return -EINVAL;
}

static int pcnt_esp32_decoder_get_size_info(struct sensor_chan_spec chan_spec, size_t *base_size,
					    size_t *frame_size)
{
	switch (chan_spec.chan_type) {
	case SENSOR_CHAN_ROTATION:
	case SENSOR_CHAN_ENCODER_COUNT:
		*base_size = sizeof(struct sensor_q31_data);
		*frame_size = sizeof(struct sensor_q31_sample_data);
		return 0;
	default:
		return -ENOTSUP;
	}
}

static int pcnt_esp32_decoder_decode(const uint8_t *buffer, struct sensor_chan_spec chan_spec,
				     uint32_t *fit, uint16_t max_count, void *data_out)
{
	const struct pcnt_esp32_encoded_data *edata =
		(const struct pcnt_esp32_encoded_data *)buffer;
	struct sensor_q31_data *out = data_out;
	int32_t raw = 0;
	uint32_t cpr = 0;
	bool found = false;

	if (*fit != 0 || max_count < 1) {
		return 0;
	}

	if (chan_spec.chan_type != SENSOR_CHAN_ROTATION &&
	    chan_spec.chan_type != SENSOR_CHAN_ENCODER_COUNT) {
		return -ENOTSUP;
	}

	for (uint8_t i = 0; i < edata->num_units; i++) {
		if (edata->unit_idx[i] == chan_spec.chan_idx) {
			raw = edata->counts[i];
			cpr = edata->counts_per_rev[i];
			found = true;
			break;
		}
	}

	if (!found) {
		return -EINVAL;
	}

	out->header.base_timestamp_ns = edata->timestamp_ns;
	out->header.reading_count = 1;

	if (chan_spec.chan_type == SENSOR_CHAN_ROTATION && cpr > 0) {
		int32_t cpr_i = (int32_t)cpr;
		int32_t wrapped = raw % cpr_i;
		int64_t deg_q31;

		if (wrapped < 0) {
			wrapped += cpr_i;
		}
		/* shift=9 keeps the q31 value in the [0, 360) degree range */
		deg_q31 = ((int64_t)wrapped * 360 * ((int64_t)INT32_MAX + 1)) / cpr_i;
		out->shift = 9;
		out->readings[0].value = (q31_t)(deg_q31 >> out->shift);
	} else {
		out->shift = 0;
		out->readings[0].value = (q31_t)raw;
	}

	*fit = 1;
	return 1;
}

SENSOR_DECODER_API_DT_DEFINE() = {
	.get_frame_count = pcnt_esp32_decoder_get_frame_count,
	.get_size_info = pcnt_esp32_decoder_get_size_info,
	.decode = pcnt_esp32_decoder_decode,
};

static int pcnt_esp32_get_decoder(const struct device *dev,
				  const struct sensor_decoder_api **decoder)
{
	ARG_UNUSED(dev);
	*decoder = &SENSOR_DECODER_NAME();
	return 0;
}

static void pcnt_esp32_submit(const struct device *dev, struct rtio_iodev_sqe *iodev_sqe)
{
	const struct sensor_read_config *cfg = iodev_sqe->sqe.iodev->data;
	const struct pcnt_esp32_config *config = dev->config;
	struct pcnt_esp32_data *data = dev->data;
	const struct sensor_chan_spec *channels = cfg->channels;
	const size_t num_channels = cfg->count;
	struct pcnt_esp32_encoded_data *edata;
	uint8_t *buf;
	uint32_t buf_len;
	uint64_t cycles;
	int rc;

	for (size_t i = 0; i < num_channels; i++) {
		if (channels[i].chan_type != SENSOR_CHAN_ROTATION &&
		    channels[i].chan_type != SENSOR_CHAN_ENCODER_COUNT) {
			rtio_iodev_sqe_err(iodev_sqe, -ENOTSUP);
			return;
		}
	}

	rc = rtio_sqe_rx_buf(iodev_sqe, sizeof(*edata), sizeof(*edata), &buf, &buf_len);
	if (rc != 0) {
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return;
	}

	edata = (struct pcnt_esp32_encoded_data *)buf;
	memset(edata, 0, sizeof(*edata));

	rc = sensor_clock_get_cycles(&cycles);
	if (rc != 0) {
		rtio_iodev_sqe_err(iodev_sqe, rc);
		return;
	}

	edata->timestamp_ns = sensor_clock_cycles_to_ns(cycles);
	edata->num_units = config->unit_len;

	k_mutex_lock(&data->cmd_mux, K_FOREVER);

	for (uint8_t i = 0; i < config->unit_len; i++) {
		struct pcnt_esp32_unit_config *unit_config = &config->unit_config[i];
		int32_t raw = pcnt_ll_get_count(data->hal.dev, unit_config->idx);

		unit_config->count_val_acc = raw;
		edata->counts[i] = raw + unit_config->offset;
		edata->counts_per_rev[i] = unit_config->counts_per_rev;
		edata->unit_idx[i] = unit_config->idx;
	}

	k_mutex_unlock(&data->cmd_mux);

	rtio_iodev_sqe_ok(iodev_sqe, 0);
}
#endif /* CONFIG_SENSOR_ASYNC_API */

static DEVICE_API(sensor, pcnt_esp32_api) = {
	.sample_fetch = pcnt_esp32_sample_fetch,
	.channel_get = pcnt_esp32_channel_get,
	.attr_set = pcnt_esp32_attr_set,
	.attr_get = pcnt_esp32_attr_get,
#ifdef CONFIG_PCNT_ESP32_TRIGGER
	.trigger_set = pcnt_esp32_trigger_set,
#endif
#ifdef CONFIG_SENSOR_ASYNC_API
	.submit = pcnt_esp32_submit,
	.get_decoder = pcnt_esp32_get_decoder,
#endif
};

PINCTRL_DT_INST_DEFINE(0);

#define UNIT_CONFIG(node_id)                                                                       \
	{                                                                                          \
		.idx = DT_REG_ADDR(node_id),                                                       \
		.filter = DT_PROP_OR(node_id, filter, 0) > 1023 ? 1023                             \
								: DT_PROP_OR(node_id, filter, 0),  \
		.counts_per_rev = DT_PROP_OR(node_id, counts_per_revolution, 0),                   \
		.h_limit = DT_PROP_OR(node_id, high_limit, 0),                                     \
		.l_limit = DT_PROP_OR(node_id, low_limit, 0),                                      \
		.h_limit_set = DT_NODE_HAS_PROP(node_id, high_limit),                              \
		.l_limit_set = DT_NODE_HAS_PROP(node_id, low_limit),                               \
		.zero_cross_en = DT_PROP(node_id, zero_cross_event),                               \
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
				.sig_pos_mode = DT_PROP_OR(DT_CHILD(node_id, channelb_1),          \
							   sig_pos_mode, 0),                       \
				.sig_neg_mode = DT_PROP_OR(DT_CHILD(node_id, channelb_1),          \
							   sig_neg_mode, 0),                       \
				.ctrl_l_mode =                                                     \
					DT_PROP_OR(DT_CHILD(node_id, channelb_1), ctrl_l_mode, 0), \
				.ctrl_h_mode =                                                     \
					DT_PROP_OR(DT_CHILD(node_id, channelb_1), ctrl_h_mode, 0), \
			},                                                                         \
	},

static struct pcnt_esp32_unit_config unit_config[] = {DT_INST_FOREACH_CHILD(0, UNIT_CONFIG)};

static struct pcnt_esp32_config pcnt_esp32_config = {
	.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
	.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(0)),
	.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(0, offset),
	.irq_source = DT_INST_IRQ_BY_IDX(0, 0, irq),
	.irq_priority = DT_INST_IRQ_BY_IDX(0, 0, priority),
	.irq_flags = DT_INST_IRQ_BY_IDX(0, 0, flags),
	.unit_config = unit_config,
	.unit_len = ARRAY_SIZE(unit_config),
};

static struct pcnt_esp32_data pcnt_esp32_data = {
	.hal = {
		.dev = (pcnt_dev_t *)DT_INST_REG_ADDR(0),
	},
	.cmd_mux = Z_MUTEX_INITIALIZER(pcnt_esp32_data.cmd_mux),
};

SENSOR_DEVICE_DT_INST_DEFINE(0, &pcnt_esp32_init, NULL, &pcnt_esp32_data, &pcnt_esp32_config,
			     POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &pcnt_esp32_api);
