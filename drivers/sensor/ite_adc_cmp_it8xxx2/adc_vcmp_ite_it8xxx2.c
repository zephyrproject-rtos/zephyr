#include <drivers/sensor.h>
#include <drivers/adc/adc_vcmp_ite_it8xxx2.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(adc_vcmp_itxxx82, LOG_LEVEL_INF);

struct adc_vcmp_ite_it8xxx2_data {
	/* Work queue to be notified when threshold assertion happens */
	struct k_work work;
	/* Sensor trigger hanlder to notify user of assetion */
	sensor_trigger_handler_t handler;
	/* ADC driver reference */
	const struct device *dev;
};

struct adc_vcmp_ite_it8xxx2_config {
	/*
	 * Pointer of ADC device that will be performing measurement, this
	 * must be provied by device tree.
	 */
	const struct device *adc;
	/*
	 * ADC channel that will be used to measure signal, this must be
	 * provided by device tree.
	 */
	uint8_t csel;
	/* Threshold selection number assigned during initialization */
	uint8_t vcmp;
	/* Threshold assert value in millivolts */
	uint32_t thr_mv;
	/*
	 * Condition to be met between measured signal and threshold assert
	 * value that will trigger event
	 */
	enum adc_vcmp_ite_it3xxx2_trigger_mode trig_mode;
};

#define DT_DRV_COMPAT ite_it8xxx2_adc_cmp

#define ADC_VCMP_ITE_IT8xxx2_UNDEFINED		(-1)
static void adc_vcmp_npcx_trigger_work_handler(struct k_work *item)
{
	struct adc_vcmp_ite_it8xxx2_data *data =
		CONTAINER_OF(item, struct adc_vcmp_ite_it8xxx2_data, work);
	struct sensor_trigger trigger = {
		.type = SENSOR_TRIG_THRESHOLD,
		.chan = SENSOR_CHAN_VOLTAGE
	};

	if (data->handler) {
		data->handler(data->dev, &trigger);
	}
}

#define ADC_VCMP_IT8XXX2_INIT_ADC_INST(node_id)                               \
	{DEVICE_DT_GET(DT_PROP(node_id, adc)),                                \
	DT_STRING_TOKEN(node_id, scan_period)},

/**/
static int adc_vcmp_init_adc(const struct device *dev)
{
	int i;
	const struct adc_vcmp_init_adc_t {
		const struct device *dev;
		enum vcmp_scan_period scan_period;
	} adc_nodes[] = {
		DT_FOREACH_STATUS_OKAY(ite_it8xxx2_adc_cmp,
			ADC_VCMP_IT8XXX2_INIT_ADC_INST)
	};

	for(i = 0; i < ARRAY_SIZE(adc_nodes); i++) {
		adc_vcmp_it8xxx2_set_scan_period(adc_nodes->dev,
						adc_nodes->scan_period);
	}
	return 0;
}
SYS_INIT(adc_vcmp_init_adc, PRE_KERNEL_1, CONFIG_SENSOR_INIT_PRIORITY);

static int adc_vcmp_ite_it8xxx2_init(const struct device *dev)
{
	const struct adc_vcmp_ite_it8xxx2_config *const config = dev->config;
	struct adc_vcmp_ite_it8xxx2_data *data = dev->data;
	struct adc_vcmp_it8xxx2_vcmp_control_t control;
	int ret;

	LOG_DBG("Initialize ADC CMP threshold selection (%d)", config->vcmp);
	/* Data must keep device reference for worker handler*/
	data->dev = dev;

	/* Set ADC channel selection */
	control.param = ADC_VCMP_ITE_IT8XXX2_PARAM_CSELL;
	control.val = (uint32_t)config->csel;
	ret = adc_vcmp_it8xxx2_ctrl_set_param(config->adc, config->vcmp,
						&control);
	if (ret) {
		goto init_error;
	}

	/* Init and set Worker queue to enable notifications */
	k_work_init(&data->work, adc_vcmp_npcx_trigger_work_handler);
	control.param = ADC_VCMP_ITE_IT8XXX2_PARAM_WORK;
	control.val = (uint32_t)&data->work;
	ret = adc_vcmp_it8xxx2_ctrl_set_param(config->adc, config->vcmp,
						&control);
	if (ret) {
		goto init_error;
	}

	/* Set threshold value if set on device tree */
	if (config->thr_mv != ADC_VCMP_ITE_IT8xxx2_UNDEFINED) {
		control.param = ADC_VCMP_ITE_IT8XXX2_PARAM_THRDAT;
#if 0
		/* TODO: add function to convert mv to raw */
		/* Convert from millivolts to ADC raw register value */
		ret = adc_npcx_threshold_mv_to_thrval(config->thr_mv,
						&control.val);
		if (ret) {
			goto init_error;
		}
#endif
		ret = adc_vcmp_it8xxx2_ctrl_set_param(config->adc, config->vcmp,
						&control);
		if (ret) {
			goto init_error;
		}
	}

	/* Set threshold comparison if set on device tree */
	if (config->trig_mode == VCMP_TRIGGER_MODE_LESS_OR_EQUAL ||
	    config->trig_mode == VCMP_TRIGGER_MODE_GREATER) {
		control.param = ADC_VCMP_ITE_IT8XXX2_PARAM_TMOD;
		control.val = (uint32_t)config->trig_mode;
		ret = adc_vcmp_it8xxx2_ctrl_set_param(config->adc,
				config->vcmp, &control);
	}

init_error:
	if (ret) {
		LOG_ERR("Error setting parameter %d - value %d",
			(uint32_t)control.param, control.val);
	}

	return ret;
}

static int adc_vcmp_ite_it8xxx2_attr_set(const struct device *dev,
					 enum sensor_channel chan,
					 enum sensor_attribute attr,
					 const struct sensor_value *val)
{
	const struct adc_vcmp_ite_it8xxx2_config *const config = dev->config;
	struct adc_vcmp_it8xxx2_vcmp_control_t control;
	int ret;

	if (chan != SENSOR_CHAN_VOLTAGE) {
		return -ENOTSUP;
	}

	switch ((uint16_t)attr) {
	case SENSOR_ATTR_LOWER_THRESH:
	case SENSOR_ATTR_UPPER_THRESH:
		/* Set threshold value first */
		control.param = ADC_VCMP_ITE_IT8XXX2_PARAM_THRDAT;
		control.val = val->val1;
		ret = adc_vcmp_it8xxx2_ctrl_set_param(config->adc,
						config->vcmp, &control);
		if (ret) {
			break;
		}

		/* Then set lower or higher threshold */
		control.param = ADC_VCMP_ITE_IT8XXX2_PARAM_TMOD;
		control.val = attr == SENSOR_ATTR_UPPER_THRESH ?
			VCMP_TRIGGER_MODE_GREATER :
			VCMP_TRIGGER_MODE_LESS_OR_EQUAL;
		ret = adc_vcmp_it8xxx2_ctrl_set_param(config->adc,
						config->vcmp, &control);
		break;
#if 0
		/* TODO: add function to convert mv to raw */
	case SENSOR_ATTR_LOWER_VOLTAGE_THRESH:
	case SENSOR_ATTR_UPPER_VOLTAGE_THRESH:
		/* Set threshold value first */
		control.param = ADC_NPCX_THRESHOLD_PARAM_THVAL;
		/* Convert from millivolts to ADC raw register value */
		ret = adc_npcx_threshold_mv_to_thrval(val->val1,
						&control.val);
		if (ret) {
			break;
		}
		ret = adc_vcmp_it8xxx2_ctrl_set_param(config->adc,
						config->vcmp, &control);
		if (ret) {
			break;
		}

		/* Then set lower or higher threshold */
		control.param = ADC_NPCX_THRESHOLD_PARAM_L_H;
		control.val =
			(uint16_t)attr == SENSOR_ATTR_UPPER_VOLTAGE_THRESH ?
			ADC_NPCX_THRESHOLD_PARAM_L_H_HIGHER :
			ADC_NPCX_THRESHOLD_PARAM_L_H_LOWER;

		ret = adc_vcmp_it8xxx2_ctrl_set_param(config->adc,
						config->vcmp, &control);
		break;
#endif
	case SENSOR_ATTR_ALERT:
		control.val = val->val1;
		ret = adc_vcmp_it8xxx2_ctrl_enable(config->adc,
						config->vcmp, !!control.val);
		break;
	default:
		ret = -ENOTSUP;
	}
	return ret;
}

static int adc_vcmp_ite_it8xxx2_attr_get(const struct device *dev,
					 enum sensor_channel chan,
					 enum sensor_attribute attr,
					 struct sensor_value *val)
{
	return -ENOTSUP;
}

static int adc_vcmp_ite_it8xxx2_trigger_set(const struct device *dev,
					    const struct sensor_trigger *trig,
					    sensor_trigger_handler_t handler)
{
	const struct adc_vcmp_ite_it8xxx2_config *const config = dev->config;
	struct adc_vcmp_ite_it8xxx2_data *data = dev->data;
	struct adc_vcmp_it8xxx2_vcmp_control_t control;

	if (trig == NULL || handler == NULL) {
		return -EINVAL;
	}

	if (trig->type != SENSOR_TRIG_THRESHOLD ||
	    trig->chan != SENSOR_CHAN_VOLTAGE) {
		return -ENOTSUP;
	}

	data->handler = handler;

	control.param = ADC_VCMP_ITE_IT8XXX2_PARAM_WORK;
	control.val = (uint32_t)&data->work;
	return adc_vcmp_it8xxx2_ctrl_set_param(config->adc, config->vcmp,
						&control);
}

static int adc_vcmp_it8xxx2_channel_get(const struct device *dev,
					enum sensor_channel chan,
					struct sensor_value *val)
{
	const struct adc_vcmp_ite_it8xxx2_config *const config = dev->config;

	if (chan != SENSOR_CHAN_VOLTAGE) {
		return -ENOTSUP;
	}

	if (val == NULL) {
		return -EINVAL;
	}

	val->val1 = (uint32_t)&config->adc;
	return 0;
}

static const struct sensor_driver_api adc_vcmp_ite_it8xxx2_api = {
	.attr_set = adc_vcmp_ite_it8xxx2_attr_set,
	.attr_get = adc_vcmp_ite_it8xxx2_attr_get,
	.trigger_set = adc_vcmp_ite_it8xxx2_trigger_set,
	.channel_get = adc_vcmp_it8xxx2_channel_get,
};

#define ADC_VCMP_IT8XXX2_INST_INIT(inst)                                      \
	static struct adc_vcmp_ite_it8xxx2_data                               \
		adc_vcmp_ite_it8xxx2_data_##inst;                             \
	static const struct adc_vcmp_ite_it8xxx2_config                       \
		adc_vcmp_ite_it8xxx2_config_##inst = {                        \
		.adc = DEVICE_DT_GET(DT_PROP(DT_PARENT(inst), adc)),          \
		.csel = DT_PROP(inst, adc_channel),                           \
		.vcmp = (uint32_t)inst,                                       \
		.thr_mv = DT_PROP_OR(inst, threshold_mv,                      \
			ADC_VCMP_ITE_IT8xxx2_UNDEFINED),                      \
		.trig_mode = DT_STRING_TOKEN_OR(inst, trigger_mode,           \
			ADC_VCMP_ITE_IT8xxx2_UNDEFINED)                       \
	};                                                                    \
	DEVICE_DT_DEFINE(inst, adc_vcmp_ite_it8xxx2_init, NULL,               \
			 &adc_vcmp_ite_it8xxx2_data_##inst,                   \
			 &adc_vcmp_ite_it8xxx2_config_##inst,                 \
			 PRE_KERNEL_2,                                        \
			 CONFIG_SENSOR_INIT_PRIORITY,                         \
			 &adc_vcmp_ite_it8xxx2_api);

#define ADC_VCMP_IT8XXX2_ID_WITH_COMMA(inst )	inst,

#define ADC_VCMP_IT8XXX2_NODE_INIT(node_id)                                   \
	enum UTIL_CAT(adc_vcmp_ite_it8xxx2_node_, node_id) {                  \
		DT_FOREACH_CHILD_STATUS_OKAY(node_id,                         \
			ADC_VCMP_IT8XXX2_ID_WITH_COMMA)                       \
	};                                                                    \
	DT_FOREACH_CHILD_STATUS_OKAY(node_id, ADC_VCMP_IT8XXX2_INST_INIT)

DT_FOREACH_STATUS_OKAY(ite_it8xxx2_adc_cmp, ADC_VCMP_IT8XXX2_NODE_INIT)
