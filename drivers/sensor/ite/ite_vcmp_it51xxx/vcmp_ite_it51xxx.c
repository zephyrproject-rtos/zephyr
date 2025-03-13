/*
 * Copyright (c) 2025 ITE Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ite_it51xxx_vcmp

#include <zephyr/drivers/adc.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/devicetree/io-channels.h>
#include <zephyr/drivers/sensor/it51xxx_vcmp.h>
#include <zephyr/dt-bindings/dt-util.h>
#include <zephyr/dt-bindings/sensor/it51xxx_vcmp.h>

#include <soc_common.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(vcmp_ite_it51xxx, CONFIG_SENSOR_LOG_LEVEL);

#define VCMP_CHANNEL_ID_REG_MASK 0x7
#define VCMP_RESOLUTION          BIT(10)
#ifdef CONFIG_ADC_IT51XXX_VOL_FULL_SCALE
#define VCMP_MAX_MVOLT 3300
#else
#define VCMP_MAX_MVOLT 3000
#endif

/* 0x046, 0x049, 0x04c, 0x06e, 0x071, 0x074: Voltage comparator x control */
#define IT51XXX_VCMP_CMPEN             BIT(7)
#define IT51XXX_VCMP_CMPINTEN          BIT(6)
#define IT51XXX_VCMP_GREATER_THRESHOLD BIT(5)

/* Device config */
struct vcmp_it51xxx_config {
	/* Voltage comparator x control register */
	volatile uint8_t *reg_vcmpxctl;
	/* Voltage comparator scan period register */
	volatile uint8_t *reg_vcmpscp;
	/* Voltage comparator x threshold data buffer MSB register */
	volatile uint8_t *reg_vcmpxthrdatm;
	/* Voltage comparator x threshold data buffer LSB register */
	volatile uint8_t *reg_vcmpxthrdatl;
	/* Voltage comparator status register */
	volatile uint8_t *reg_vcmpsts;
	/* Voltage comparator module irq */
	int irq;
	/* Voltage comparator channel */
	int vcmp_ch;
	/* Scan period for "all voltage comparator channel" */
	int scan_period;
	/*
	 * Determines the condition between ADC data and threshold_mv
	 * that will trigger voltage comparator interrupt.
	 */
	int comparison;
	/* Threshold assert value in mv */
	int threshold_mv;
	/* Pointer of ADC device that will be performing measurement */
	const struct device *adc;
};

/* Driver data */
struct vcmp_it51xxx_data {
	/* ADC channel config */
	struct adc_channel_cfg adc_ch_cfg;
	/* Work queue to be notified when threshold assertion happens */
	struct k_work work;
	/* Sensor trigger handler to notify user of assertion */
	sensor_trigger_handler_t handler;
	const struct sensor_trigger *trig;
	/* Pointer of voltage comparator device */
	const struct device *vcmp;
};

/* Voltage comparator work queue address */
static uint32_t vcmp_work_addr[VCMP_CHANNEL_CNT];
#ifdef CONFIG_VCMP_IT51XXX_WORKQUEUE
/*
 * Pointer of work queue thread to be notified when threshold assertion
 * occurs.
 */
struct k_work_q *work_q;
#endif

static void clear_vcmp_status(const struct device *dev, int vcmp_ch)
{
	const struct vcmp_it51xxx_config *const config = dev->config;
	volatile uint8_t *reg_vcmpsts = config->reg_vcmpsts;

	/* W/C voltage comparator specific channel interrupt status */
	*reg_vcmpsts = BIT(vcmp_ch);
}

static void vcmp_enable(const struct device *dev, int enable)
{
	const struct vcmp_it51xxx_config *const config = dev->config;
	volatile uint8_t *reg_vcmpxctl = config->reg_vcmpxctl;

	if (enable) {
		/* Enable voltage comparator specific channel interrupt */
		*reg_vcmpxctl |= IT51XXX_VCMP_CMPINTEN;
		/* Start voltage comparator specific channel */
		*reg_vcmpxctl |= IT51XXX_VCMP_CMPEN;
	} else {
		/* Disable voltage comparator specific channel interrupt */
		*reg_vcmpxctl &= ~IT51XXX_VCMP_CMPINTEN;
		/* Stop voltage comparator specific channel */
		*reg_vcmpxctl &= ~IT51XXX_VCMP_CMPEN;
	}
}

static int vcmp_set_threshold(const struct device *dev, enum sensor_attribute attr, int32_t reg_val)
{
	const struct vcmp_it51xxx_config *const config = dev->config;
	volatile uint8_t *reg_vcmpxthrdatm = config->reg_vcmpxthrdatm;
	volatile uint8_t *reg_vcmpxthrdatl = config->reg_vcmpxthrdatl;
	volatile uint8_t *reg_vcmpxctl = config->reg_vcmpxctl;

	if (reg_val >= VCMP_RESOLUTION) {
		LOG_ERR("Vcmp%d threshold only support 10-bits", config->vcmp_ch);
		return -ENOTSUP;
	}

	/* Set threshold raw value */
	*reg_vcmpxthrdatl = (uint8_t)(reg_val & 0xff);
	*reg_vcmpxthrdatm = (uint8_t)((reg_val >> 8) & 0xff);

	/* Set lower or higher threshold */
	if ((attr == SENSOR_ATTR_UPPER_THRESH) ||
	    (attr == (uint16_t)SENSOR_ATTR_UPPER_VOLTAGE_THRESH)) {
		*reg_vcmpxctl |= IT51XXX_VCMP_GREATER_THRESHOLD;
	} else {
		*reg_vcmpxctl &= ~IT51XXX_VCMP_GREATER_THRESHOLD;
	}

	return 0;
}

static void it51xxx_vcmp_trigger_work_handler(struct k_work *item)
{
	struct vcmp_it51xxx_data *data = CONTAINER_OF(item, struct vcmp_it51xxx_data, work);

	if (data->handler) {
		data->handler(data->vcmp, data->trig);
	}
}

static int vcmp_ite_it51xxx_attr_set(const struct device *dev, enum sensor_channel chan,
				     enum sensor_attribute attr, const struct sensor_value *val)
{
	const struct vcmp_it51xxx_config *const config = dev->config;
	int32_t reg_val, ret = 0;

	if (chan != SENSOR_CHAN_VOLTAGE) {
		return -ENOTSUP;
	}

	switch ((uint16_t)attr) {
	case SENSOR_ATTR_LOWER_THRESH:
	case SENSOR_ATTR_UPPER_THRESH:
		ret = vcmp_set_threshold(dev, attr, val->val1);
		break;
	case SENSOR_ATTR_LOWER_VOLTAGE_THRESH:
	case SENSOR_ATTR_UPPER_VOLTAGE_THRESH:
		/*
		 * Tranfrom threshold from mv to raw
		 * NOTE: CMPXTHRDAT[9:0] = threshold(mv) * 1024 / 3000(mv)
		 */
		reg_val = (val->val1 * VCMP_RESOLUTION / VCMP_MAX_MVOLT);
		ret = vcmp_set_threshold(dev, attr, reg_val);
		break;
	case SENSOR_ATTR_ALERT:
		if (!!val->val1) {
			clear_vcmp_status(dev, config->vcmp_ch);
			vcmp_enable(dev, 1);
		} else {
			vcmp_enable(dev, 0);
			clear_vcmp_status(dev, config->vcmp_ch);
		}

		break;
	default:
		ret = -ENOTSUP;
	}

	return ret;
}

static int vcmp_ite_it51xxx_trigger_set(const struct device *dev, const struct sensor_trigger *trig,
					sensor_trigger_handler_t handler)
{
	const struct vcmp_it51xxx_config *const config = dev->config;
	struct vcmp_it51xxx_data *const data = dev->data;

	if (trig->type != SENSOR_TRIG_THRESHOLD || trig->chan != SENSOR_CHAN_VOLTAGE) {
		return -ENOTSUP;
	}

	data->handler = handler;
	data->trig = trig;

	vcmp_work_addr[config->vcmp_ch] = (uint32_t)&data->work;

	return 0;
}

static int vcmp_it51xxx_channel_get(const struct device *dev, enum sensor_channel chan,
				    struct sensor_value *val)
{
	const struct vcmp_it51xxx_config *const config = dev->config;

	if (chan != SENSOR_CHAN_VOLTAGE) {
		return -ENOTSUP;
	}

	/*
	 * it51xxx adc and comparator module read automatically, according to
	 * {ADCCTS1, ADCCTS2} and VCMPSCP register setting.
	 */
	val->val1 = config->vcmp_ch;

	return 0;
}

/*
 * All voltage comparator channels share one irq interrupt, so we
 * need to handle all channels, when the interrupt fired.
 */
static void vcmp_it51xxx_isr(const struct device *dev)
{
	const struct vcmp_it51xxx_config *const config = dev->config;
	volatile uint8_t *reg_vcmpsts = config->reg_vcmpsts;
	int idx, status;

	/* Find out which voltage comparator triggered */
	status = *reg_vcmpsts & VCMP_CHANNEL_ID_REG_MASK;

	for (idx = VCMP_CHANNEL_0; idx < VCMP_CHANNEL_CNT; idx++) {
		if (status & BIT(idx)) {
			/* Call triggered channel callback function in work queue */
			if (vcmp_work_addr[idx]) {
#ifdef CONFIG_VCMP_IT51XXX_WORKQUEUE
				k_work_submit_to_queue(work_q,
						       (struct k_work *)vcmp_work_addr[idx]);
#else
				k_work_submit((struct k_work *)vcmp_work_addr[idx]);
#endif
			}
			/* W/C voltage comparator specific channel interrupt status */
			clear_vcmp_status(dev, idx);
		}
	}

	/* W/C voltage comparator irq interrupt status */
	ite_intc_isr_clear(config->irq);
}

static int vcmp_it51xxx_init(const struct device *dev)
{
	const struct vcmp_it51xxx_config *const config = dev->config;
	struct vcmp_it51xxx_data *const data = dev->data;
	volatile uint8_t *reg_vcmpxctl = config->reg_vcmpxctl;
	volatile uint8_t *reg_vcmpscp = config->reg_vcmpscp;

	/* Disable voltage comparator specific channel before init */
	vcmp_enable(dev, 0);

	/*
	 * ADC channel signal output to voltage comparator,
	 * so we need to set ADC channel to alternate mode first.
	 */
	if (!device_is_ready(config->adc)) {
		LOG_ERR("ADC device not ready");
		return -ENODEV;
	}
	adc_channel_setup(config->adc, &data->adc_ch_cfg);

	/* Select which ADC channel output voltage into comparator */
	*reg_vcmpxctl |= data->adc_ch_cfg.channel_id & VCMP_CHANNEL_ID_REG_MASK;

	/* Set minimum scan period for "all voltage comparator channel" */
	if (*reg_vcmpscp > config->scan_period) {
		*reg_vcmpscp = config->scan_period;
	}

	/* Data must keep device reference for worker handler */
	data->vcmp = dev;

	/* Init and set work item to enable notifications */
	k_work_init(&data->work, it51xxx_vcmp_trigger_work_handler);
	vcmp_work_addr[config->vcmp_ch] = (uint32_t)&data->work;

	/* Set threshold and comparison if set on device tree */
	if ((config->threshold_mv != IT51XXX_VCMP_UNDEFINED) &&
	    (config->comparison != IT51XXX_VCMP_UNDEFINED)) {
		enum sensor_attribute attr;
		struct sensor_value val;

		if (config->comparison == IT51XXX_VCMP_LESS_OR_EQUAL) {
			attr = SENSOR_ATTR_LOWER_VOLTAGE_THRESH;
		} else {
			attr = SENSOR_ATTR_UPPER_VOLTAGE_THRESH;
		}

		val.val1 = config->threshold_mv;
		val.val2 = 0;

		vcmp_ite_it51xxx_attr_set(dev, SENSOR_CHAN_VOLTAGE, attr, &val);
	}

	/*
	 * All voltage comparator channels share one irq interrupt,
	 * so if the irq is enabled before, we needn't to enable again.
	 * And we will figure out the triggered channel in vcmp_it51xxx_isr().
	 */
	if (!irq_is_enabled(config->irq)) {
		ite_intc_isr_clear(config->irq);

		irq_connect_dynamic(config->irq, 0, (void (*)(const void *))vcmp_it51xxx_isr,
				    (const void *)dev, 0);
		irq_enable(config->irq);
	}

	return 0;
}

static DEVICE_API(sensor, vcmp_ite_it51xxx_api) = {
	.attr_set = vcmp_ite_it51xxx_attr_set,
	.trigger_set = vcmp_ite_it51xxx_trigger_set,
	.channel_get = vcmp_it51xxx_channel_get,
};

#define VCMP_IT51XXX_INIT(inst)                                                                    \
	static const struct vcmp_it51xxx_config vcmp_it51xxx_cfg_##inst = {                        \
		.reg_vcmpxctl = (uint8_t *)DT_INST_REG_ADDR_BY_IDX(inst, 0),                       \
		.reg_vcmpscp = (uint8_t *)DT_INST_REG_ADDR_BY_IDX(inst, 1),                        \
		.reg_vcmpxthrdatm = (uint8_t *)DT_INST_REG_ADDR_BY_IDX(inst, 2),                   \
		.reg_vcmpxthrdatl = (uint8_t *)DT_INST_REG_ADDR_BY_IDX(inst, 3),                   \
		.reg_vcmpsts = (uint8_t *)DT_INST_REG_ADDR_BY_IDX(inst, 4),                        \
		.irq = DT_INST_IRQN(inst),                                                         \
		.vcmp_ch = DT_INST_PROP(inst, vcmp_ch),                                            \
		.scan_period = DT_INST_PROP(inst, scan_period),                                    \
		.comparison = DT_INST_PROP(inst, comparison),                                      \
		.threshold_mv = DT_INST_PROP(inst, threshold_mv),                                  \
		.adc = DEVICE_DT_GET(DT_INST_IO_CHANNELS_CTLR(inst)),                              \
	};                                                                                         \
                                                                                                   \
	static struct vcmp_it51xxx_data vcmp_it51xxx_data_##inst = {                               \
		.adc_ch_cfg.gain = ADC_GAIN_1,                                                     \
		.adc_ch_cfg.reference = ADC_REF_INTERNAL,                                          \
		.adc_ch_cfg.acquisition_time = ADC_ACQ_TIME_DEFAULT,                               \
		.adc_ch_cfg.channel_id = (uint8_t)DT_INST_IO_CHANNELS_INPUT(inst),                 \
	};                                                                                         \
                                                                                                   \
	SENSOR_DEVICE_DT_INST_DEFINE(inst, vcmp_it51xxx_init, NULL, &vcmp_it51xxx_data_##inst,     \
				     &vcmp_it51xxx_cfg_##inst, POST_KERNEL,                        \
				     CONFIG_VCMP_IT51XXX_INIT_PRIORITY, &vcmp_ite_it51xxx_api);

DT_INST_FOREACH_STATUS_OKAY(VCMP_IT51XXX_INIT)
#ifdef CONFIG_VCMP_IT51XXX_WORKQUEUE
BUILD_ASSERT(CONFIG_SENSOR_INIT_PRIORITY < CONFIG_VCMP_IT51XXX_INIT_PRIORITY,
	     "CONFIG_SENSOR_INIT_PRIORITY must be less than CONFIG_VCMP_IT51XXX_INIT_PRIORITY");
#endif
