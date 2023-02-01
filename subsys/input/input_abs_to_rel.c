#define DT_DRV_COMPAT zephyr_input_abs_to_rel

#include <zephyr/drivers/input.h>
#include <zephyr/input/input.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(input_abs_to_rel, LOG_LEVEL_INF);

struct input_abs_to_rel_config {
	const struct device *input_dev;
};

struct input_abs_to_rel_data {
	int32_t last_val;
	int32_t acc;
};

static void input_abs_to_rel_cb(const struct device *dev,
				struct input_event *evt, bool sync)
{
	struct input_abs_to_rel_data *data = dev->data;
	int rel;
	int out;

	if (evt->code != INPUT_ABS_Z) {
		return;
	}

	rel = evt->value - data->last_val;
	data->last_val = evt->value;

	data->acc += rel;

	out = data->acc / 4;
	data->acc %= 4;

	if (out) {
		input_report_rel(dev, INPUT_REL_Z, out, true, K_FOREVER);
	}
}

static int input_abs_to_rel_init(const struct device *dev)
{
	const struct input_abs_to_rel_config *cfg = dev->config;

	if (!device_is_ready(cfg->input_dev)) {
		LOG_ERR("Input device not ready");
		return -ENODEV;
	}

	return 0;
}

static const struct input_driver_api input_abs_to_rel_api = {
};

#define INPUT_ABS_TO_REL_INIT(index) \
	static void input_abs_to_rel_cb_##index(struct input_event *evt, bool sync) \
	{ \
		input_abs_to_rel_cb(DEVICE_DT_GET(DT_INST(index, DT_DRV_COMPAT)), evt, sync); \
	} \
	INPUT_LISTENER_CB_DEFINE(DEVICE_DT_GET(DT_INST_PHANDLE(index, input)), \
				 input_abs_to_rel_cb_##index); \
	static const struct input_abs_to_rel_config input_abs_to_rel_config_##index = { \
	        .input_dev = DEVICE_DT_GET(DT_INST_PHANDLE(index, input)), \
	}; \
	static struct input_abs_to_rel_data input_abs_to_rel_data_##index; \
	DEVICE_DT_INST_DEFINE(index, input_abs_to_rel_init, NULL, \
			      &input_abs_to_rel_data_##index, &input_abs_to_rel_config_##index, \
			      POST_KERNEL, CONFIG_KSCAN_INIT_PRIORITY, &input_abs_to_rel_api);

DT_INST_FOREACH_STATUS_OKAY(INPUT_ABS_TO_REL_INIT)
