/*
 * Copyright (c) 2025 ITE Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ite_it51xxx_vcmp

#include <zephyr/device.h>
#include <zephyr/devicetree/io-channels.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/comparator.h>
#include <zephyr/dt-bindings/comparator/it51xxx-vcmp.h>
#include <zephyr/dt-bindings/dt-util.h>

#include <soc_common.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(comparator_it51xxx_vcmp, CONFIG_COMPARATOR_LOG_LEVEL);

#define VCMP_CHANNEL_ID_REG_MASK GENMASK(2, 0)
#define VCMP_THRESHOLD           BIT(10)
#ifdef CONFIG_ADC_IT51XXX_VOL_FULL_SCALE
#define VCMP_MAX_MVOLT 3300
#else
#define VCMP_MAX_MVOLT 3000
#endif

/* 0x20, 0x28, 0x2c: Voltage Comparator n Control Register  (VCMPnCTL) (n=0 to 2) */
#define REG_VCMP_VCMP0CTL         0x20
#define REG_VCMP_VCMP1CTL         0x28
#define REG_VCMP_VCMP2CTL         0x2C
#define VCMP_CMPEN                BIT(7)
#define VCMP_CMPINTEN             BIT(6)
#define VCMP_GREATER_THRESHOLD    BIT(5)
#define VCMP_CMP_EDGE_SENSED_MODE BIT(4)
const uint8_t vcmp_ctrl_reg[VCMP_CHANNEL_CNT] = {REG_VCMP_VCMP0CTL, REG_VCMP_VCMP1CTL,
						 REG_VCMP_VCMP2CTL};

/* 0x21, 0x29, 0x2D: Voltage Comparator n Status and Control Register  (VCMPnSCTL) (n=0 to 2) */
#define REG_VCMP_VCMP0SCTL 0x21
#define REG_VCMP_VCMP1SCTL 0x29
#define REG_VCMP_VCMP2SCTL 0x2D
#define VCMP_CMPXRTIS      BIT(6)
const uint8_t vcmp_status_ctrl_reg[VCMP_CHANNEL_CNT] = {REG_VCMP_VCMP0SCTL, REG_VCMP_VCMP1SCTL,
							REG_VCMP_VCMP2SCTL};

/* 0x22, 0x2A, 0x2E: Voltage Comparator 0~2 MSB Threshold Data Buffer (10-bit resolution) */
#define REG_VCMP_CH_THRDATM 0x02

/* 0x23, 0x2B, 0x2F: Voltage Comparator 0~2 LSB Threshold Data Buffer (10-bit resolution) */
#define REG_VCMP_CH_THRDATL 0x03

/* 0x33: Voltage Comparator Scan Period 2 (VCMPSCP2) */
#define REG_VCMP_VCMPSCP2 0x33
#define SCAN_PERIOD_MASK  GENMASK(7, 4)

/* Device config */
struct vcmp_it51xxx_config {
	/* Voltage comparator channel base address */
	uintptr_t base_ch;
	/* Voltage comparator base address */
	uintptr_t reg_base;
	/* Voltage comparator module irq */
	int irq;
	/* Voltage comparator channel */
	int vcmp_ch;
	/* Comparator 0/1/2 Scan Period */
	uint8_t scan_period;
	/* Threshold assert value in mv */
	int32_t threshold_mv;
	/* Pointer of ADC device that will be performing measurement */
	const struct device *adc;
	/* Channel identifier */
	uint8_t channel_id;
};

/* Driver data */
struct vcmp_it51xxx_data {
	/* ADC channel config */
	struct adc_channel_cfg adc_ch_cfg;
	comparator_callback_t user_cb;
	void *user_cb_data;
	uint8_t interrupt_mask;
	atomic_t triggered;
	/* Pointer of voltage comparator device */
	const struct device *vcmp;
};
/*
 * Because all the three voltage comparators use the same interrupt number, the
 * 'irq_connect_dynamic' call in the driver init function sets the passed-in dev pointer in isr
 * function to the first instantiated voltage comparator.
 *
 * For example, when setting vcmp2 and vcmp3 to 'status = "okay";' in dts file, each time an
 * interrupt is triggered,the passed-in 'const struct device *dev' argument in isr function always
 * points to vcmp2 device.
 *
 * To access the 'struct vcmp_it51xxx_data' of respective voltage comparator instance, we
 * use a array to store their address.
 */
static struct vcmp_it51xxx_data *vcmp_data[VCMP_CHANNEL_CNT];

static void vcmp_irq_handler(const struct device *dev)
{
	struct vcmp_it51xxx_data *const data = dev->data;

	if (data->user_cb == NULL) {
		atomic_set_bit(&data->triggered, 0);
		return;
	}

	data->user_cb(dev, data->user_cb_data);
	atomic_clear_bit(&data->triggered, 0);
}

static void clear_vcmp_status(const struct device *dev, int channel)
{
	const struct vcmp_it51xxx_config *const cfg = dev->config;
	const uintptr_t reg_base = cfg->reg_base;

	sys_write8(sys_read8(reg_base + vcmp_status_ctrl_reg[channel]),
		   reg_base + vcmp_status_ctrl_reg[channel]);
}

static int vcmp_set_threshold(const struct device *dev, int32_t threshold_mv)
{
	const struct vcmp_it51xxx_config *const cfg = dev->config;
	const uintptr_t base_ch = cfg->base_ch;
	int32_t reg_val;

	/*
	 * Tranfrom threshold from mv to raw
	 * NOTE: CMPXTHRDAT[9:0] = threshold(mv) * 1024 / 3000(mv)
	 */
	reg_val = (threshold_mv * VCMP_THRESHOLD / VCMP_MAX_MVOLT);

	if (reg_val >= VCMP_THRESHOLD) {
		LOG_ERR("Vcmp%d threshold only support 10-bits", cfg->vcmp_ch);
		return -ENOTSUP;
	}

	/* Set threshold raw value */
	sys_write8(reg_val & 0xff, base_ch + REG_VCMP_CH_THRDATL);
	sys_write8((reg_val >> 8) & 0xff, base_ch + REG_VCMP_CH_THRDATM);

	return 0;
}

static void vcmp_set_attr(const struct device *dev, enum comparator_trigger trigger)
{
	const struct vcmp_it51xxx_config *const cfg = dev->config;
	const uintptr_t base_ch = cfg->base_ch;

	/* Set lower or higher threshold */
	if (trigger == COMPARATOR_TRIGGER_RISING_EDGE) {
		sys_write8(sys_read8(base_ch) | VCMP_GREATER_THRESHOLD, base_ch);
	} else {
		sys_write8(sys_read8(base_ch) & ~VCMP_GREATER_THRESHOLD, base_ch);
	}
}

static void vcmp_enable(const struct device *dev, bool enable)
{
	const struct vcmp_it51xxx_config *const cfg = dev->config;
	const uintptr_t base_ch = cfg->base_ch;

	if (enable) {
		/* Enable voltage comparator interrupt */
		sys_write8(sys_read8(base_ch) | VCMP_CMPINTEN, base_ch);
		/* Start voltage comparator */
		sys_write8(sys_read8(base_ch) | VCMP_CMPEN, base_ch);
	} else {
		/* Disable voltage comparator interrupt */
		sys_write8(sys_read8(base_ch) & ~VCMP_CMPINTEN, base_ch);
		/* Stop voltage comparator */
		sys_write8(sys_read8(base_ch) & ~VCMP_CMPEN, base_ch);
	}
}

static int it51xxx_vcmp_get_output(const struct device *dev)
{
	ARG_UNUSED(dev);

	LOG_ERR("Unsupported function: %s", __func__);

	return -ENOTSUP;
}

static int it51xxx_vcmp_set_trigger(const struct device *dev, enum comparator_trigger trigger)
{
	const struct vcmp_it51xxx_config *cfg = dev->config;
	struct vcmp_it51xxx_data *data = dev->data;

	/* Disable VCMP interrupt */
	vcmp_enable(dev, false);

	/* W/C voltage comparator specific channel interrupt status */
	clear_vcmp_status(dev, cfg->vcmp_ch);

	switch (trigger) {
	case COMPARATOR_TRIGGER_BOTH_EDGES:
		LOG_ERR("Unsupported trigger: COMPARATOR_TRIGGER_BOTH_EDGES");
		return -ENOTSUP;
	case COMPARATOR_TRIGGER_RISING_EDGE:
		data->interrupt_mask = COMPARATOR_TRIGGER_RISING_EDGE;
		vcmp_set_attr(dev, COMPARATOR_TRIGGER_RISING_EDGE);
		break;
	case COMPARATOR_TRIGGER_FALLING_EDGE:
		data->interrupt_mask = COMPARATOR_TRIGGER_FALLING_EDGE;
		vcmp_set_attr(dev, COMPARATOR_TRIGGER_FALLING_EDGE);
		break;
	case COMPARATOR_TRIGGER_NONE:
		data->interrupt_mask = 0;
		break;
	default:
		return -EINVAL;
	}

	if (data->interrupt_mask) {
		vcmp_enable(dev, true);
	}

	return 0;
}

static int it51xxx_vcmp_set_trigger_callback(const struct device *dev,
					     comparator_callback_t callback, void *user_data)
{
	struct vcmp_it51xxx_data *data = dev->data;

	/* Disable voltage comparator and interrupt */
	vcmp_enable(dev, false);

	data->user_cb = callback;
	data->user_cb_data = user_data;

	if (callback != NULL && atomic_test_and_clear_bit(&data->triggered, 0)) {
		callback(dev, user_data);
	}

	/* Re-enable currently set VCMP interrupt */
	if (data->interrupt_mask) {
		vcmp_enable(dev, true);
	}

	return 0;
}

static int it51xxx_vcmp_trigger_is_pending(const struct device *dev)
{
	struct vcmp_it51xxx_data *data = dev->data;

	return atomic_test_and_clear_bit(&data->triggered, 0);
}

/*
 * All voltage comparator channels share one irq interrupt, so we
 * need to handle all channels, when the interrupt fired.
 */
static void vcmp_it51xxx_isr(const struct device *dev)
{
	const struct vcmp_it51xxx_config *const cfg = dev->config;
	const uintptr_t reg_base = cfg->reg_base;
	/*
	 * Comparator n Trigger Mode (CMPnTMOD)
	 * false (0): Less than or equal to CMPnTHRDAT [9:0]
	 * true  (1): Greater than CMPnTHRDAT [9:0]
	 */
	bool comparator_mode;

	/* Find out which voltage comparator triggered */
	for (int idx = VCMP_CHANNEL_0; idx < VCMP_CHANNEL_CNT; idx++) {
		if (sys_read8(reg_base + vcmp_status_ctrl_reg[idx]) & VCMP_CMPXRTIS) {
			struct vcmp_it51xxx_data *data = vcmp_data[idx];

			comparator_mode =
				sys_read8(reg_base + vcmp_ctrl_reg[idx]) & VCMP_GREATER_THRESHOLD;

			if ((comparator_mode && data &&
			     data->interrupt_mask == COMPARATOR_TRIGGER_RISING_EDGE) ||
			    (!comparator_mode && data &&
			     data->interrupt_mask == COMPARATOR_TRIGGER_FALLING_EDGE)) {

				vcmp_irq_handler(data->vcmp);
			}

			if (comparator_mode) {
				sys_write8(sys_read8(reg_base + vcmp_ctrl_reg[idx]) &
						   ~VCMP_GREATER_THRESHOLD,
					   reg_base + vcmp_ctrl_reg[idx]);
			} else {
				sys_write8(sys_read8(reg_base + vcmp_ctrl_reg[idx]) |
						   VCMP_GREATER_THRESHOLD,
					   reg_base + vcmp_ctrl_reg[idx]);
			}

			/* W/C voltage comparator specific channel interrupt status */
			clear_vcmp_status(dev, idx);
		}
	}

	/* W/C voltage comparator irq interrupt status */
	ite_intc_isr_clear(cfg->irq);
}

static int vcmp_it51xxx_init(const struct device *dev)
{
	const struct vcmp_it51xxx_config *const cfg = dev->config;
	struct vcmp_it51xxx_data *const data = dev->data;
	uintptr_t base_ch = cfg->base_ch;
	uintptr_t reg_vcmpscp2 = cfg->reg_base + REG_VCMP_VCMPSCP2;
	uint8_t reg_value;
	int err;

	data->adc_ch_cfg.gain = ADC_GAIN_1;
	data->adc_ch_cfg.reference = ADC_REF_INTERNAL;
	data->adc_ch_cfg.acquisition_time = ADC_ACQ_TIME_DEFAULT;
	data->adc_ch_cfg.channel_id = cfg->channel_id;

	/* Disable voltage comparator and interrupt */
	vcmp_enable(dev, false);

	/*
	 * ADC channel signal output to voltage comparator,
	 * so we need to set ADC channel to alternate mode first.
	 */
	if (!device_is_ready(cfg->adc)) {
		LOG_ERR("ADC device not ready");
		return -ENODEV;
	}

	err = adc_channel_setup(cfg->adc, &data->adc_ch_cfg);
	if (err) {
		return err;
	}

	/* Select which ADC channel output voltage into comparator */
	reg_value = FIELD_PREP(GENMASK(7, 3), sys_read8(base_ch));
	reg_value |= data->adc_ch_cfg.channel_id & VCMP_CHANNEL_ID_REG_MASK;
	sys_write8(reg_value, base_ch);

	/* Set VCMP to Edge Sense Mode */
	sys_write8(sys_read8(base_ch) | VCMP_CMP_EDGE_SENSED_MODE, base_ch);

	/* Store the address of driver data  for later access in ISR function*/
	if (cfg->vcmp_ch >= VCMP_CHANNEL_CNT) {
		LOG_ERR("invalid volt comparator channel setting(%d)", cfg->vcmp_ch);
		return -EINVAL;
	}
	vcmp_data[cfg->vcmp_ch] = dev->data;

	/*
	 * Set minimum scan period for "all" voltage comparator
	 * Three voltage comparators share a scan period setting and use the fastest one
	 */
	if (cfg->scan_period < FIELD_GET(SCAN_PERIOD_MASK, sys_read8(reg_vcmpscp2))) {
		sys_write8(FIELD_PREP(SCAN_PERIOD_MASK, cfg->scan_period), reg_vcmpscp2);
	}

	/* Data must keep device reference for later access in ISR function */
	data->vcmp = dev;

	err = vcmp_set_threshold(dev, cfg->threshold_mv);
	if (err) {
		return err;
	}

	/*
	 * All voltage comparator channels share one irq interrupt,
	 * so if the irq is enabled before, we needn't to enable again.
	 * And we will figure out the triggered channel in vcmp_it51xxx_isr().
	 */
	if (!irq_is_enabled(cfg->irq)) {
		ite_intc_isr_clear(cfg->irq);
		irq_connect_dynamic(cfg->irq, 0, (void (*)(const void *))vcmp_it51xxx_isr,
				    (const void *)dev, 0);
		irq_enable(cfg->irq);
	}

	return 0;
}

static DEVICE_API(comparator, it51xxx_vcmp_api) = {
	.get_output = it51xxx_vcmp_get_output,
	.set_trigger = it51xxx_vcmp_set_trigger,
	.set_trigger_callback = it51xxx_vcmp_set_trigger_callback,
	.trigger_is_pending = it51xxx_vcmp_trigger_is_pending,
};

#define VCMP_IT51XXX_INIT(inst)                                                                    \
	static const struct vcmp_it51xxx_config vcmp_it51xxx_cfg_##inst = {                        \
		.base_ch = DT_INST_REG_ADDR_BY_IDX(inst, 0),                                       \
		.reg_base = DT_INST_REG_ADDR_BY_IDX(inst, 1),                                      \
		.irq = DT_INST_IRQN(inst),                                                         \
		.vcmp_ch = DT_INST_PROP(inst, vcmp_ch),                                            \
		.scan_period = DT_INST_PROP(inst, scan_period),                                    \
		.threshold_mv = DT_INST_PROP(inst, threshold_mv),                                  \
		.adc = DEVICE_DT_GET(DT_INST_IO_CHANNELS_CTLR(inst)),                              \
		.channel_id = (uint8_t)DT_INST_IO_CHANNELS_INPUT(inst),                            \
	};                                                                                         \
                                                                                                   \
	static struct vcmp_it51xxx_data vcmp_it51xxx_data_##inst;                                  \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, vcmp_it51xxx_init, NULL, &vcmp_it51xxx_data_##inst,            \
			      &vcmp_it51xxx_cfg_##inst, POST_KERNEL,                               \
			      CONFIG_COMPARATOR_INIT_PRIORITY, &it51xxx_vcmp_api);

DT_INST_FOREACH_STATUS_OKAY(VCMP_IT51XXX_INIT)
