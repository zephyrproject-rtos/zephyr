#define DT_DRV_COMPAT zephyr_input_longpress

#include <zephyr/drivers/input.h>
#include <zephyr/input/input.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(input_longpress, LOG_LEVEL_INF);

struct longpress_config {
	const struct device *input_dev;
	uint16_t in_code;
	uint8_t steps;
	uint16_t short_code;
	const uint16_t *long_codes;
	const int *long_delays_ms;
};

struct longpress_data {
	const struct device *dev;
	struct k_work_delayable work;
	uint8_t step;
};

static void longpress_deferred(struct k_work *work)
{
	struct longpress_data *data = CONTAINER_OF(work, struct longpress_data, work);
	const struct device *dev = data->dev;
	const struct longpress_config *cfg = dev->config;
	int next_delay_ms;
	uint16_t code;

	code = cfg->long_codes[data->step];
	input_report_key(dev, code, 1, true, K_FOREVER);

	data->step++;

	if (data->step >= cfg->steps) {
		return;
	}

	next_delay_ms = cfg->long_delays_ms[data->step];

	k_work_schedule(&data->work, K_MSEC(next_delay_ms));
}

static void longpress_cb(const struct device *dev, struct input_event *evt,
			 bool sync)
{
	const struct longpress_config *cfg = dev->config;
	struct longpress_data *data = dev->data;

	if (evt->code != cfg->in_code) {
		return;
	}

	if (evt->value) {
		data->step = 0;
		k_work_schedule(&data->work, K_MSEC(cfg->long_delays_ms[0]));
	} else {
		if (data->step == 0) {
			input_report_key(dev, cfg->short_code, 1, true, K_FOREVER);
		}
		k_work_cancel_delayable(&data->work);
	}
}

static int longpress_init(const struct device *dev)
{
	const struct longpress_config *cfg = dev->config;
	struct longpress_data *data = dev->data;

	if (!device_is_ready(cfg->input_dev)) {
		LOG_ERR("Input device not ready");
		return -ENODEV;
	}

	data->dev = dev;

	k_work_init_delayable(&data->work, longpress_deferred);

	return 0;
}

static const struct input_driver_api input_longpress_api = {
};

#define INPUT_LONGPRESS_INIT(index) \
	static void longpress_cb_##index(struct input_event *evt, bool sync) \
	{ \
		longpress_cb(DEVICE_DT_GET(DT_INST(index, DT_DRV_COMPAT)), evt, sync); \
	} \
	INPUT_LISTENER_CB_DEFINE(DEVICE_DT_GET(DT_INST_PHANDLE(index, input)), \
				 longpress_cb_##index); \
	static const uint16_t input_longpress_codes_##index[] = DT_INST_PROP(index, long_codes); \
	static const int input_longpress_delays_ms_##index[] = DT_INST_PROP(index, long_delays_ms); \
	static const struct longpress_config longpress_config_##index = { \
		.input_dev = DEVICE_DT_GET(DT_INST_PHANDLE(index, input)), \
		.in_code = DT_INST_PROP(index, input_code), \
		.steps = DT_INST_PROP_LEN(index, long_codes), \
		.short_code = DT_INST_PROP(index, short_code), \
		.long_codes = input_longpress_codes_##index, \
		.long_delays_ms = input_longpress_delays_ms_##index, \
	}; \
	static struct longpress_data longpress_data_##index; \
	DEVICE_DT_INST_DEFINE(index, longpress_init, NULL, \
			      &longpress_data_##index, &longpress_config_##index, \
			      POST_KERNEL, CONFIG_KSCAN_INIT_PRIORITY, &input_longpress_api);

DT_INST_FOREACH_STATUS_OKAY(INPUT_LONGPRESS_INIT)
