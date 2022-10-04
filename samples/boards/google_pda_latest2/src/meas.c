#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/adc.h>

#define VCC1_MEAS_NODE DT_ALIAS(vcc1)
#define VCC2_MEAS_NODE DT_ALIAS(vcc2)
#define VBUS_MEAS_NODE DT_ALIAS(vbus)
#define CBUS_MEAS_NODE DT_ALIAS(cbus)
#define CCON_MEAS_NODE DT_ALIAS(ccon)

static const struct adc_dt_spec adc_vcc1 = ADC_DT_SPEC_GET(VCC1_MEAS_NODE);
static const struct adc_dt_spec adc_vcc2 = ADC_DT_SPEC_GET(VCC2_MEAS_NODE);
static const struct adc_dt_spec adc_vbus = ADC_DT_SPEC_GET(VBUS_MEAS_NODE);
static const struct adc_dt_spec adc_cbus = ADC_DT_SPEC_GET(CBUS_MEAS_NODE);
static const struct adc_dt_spec adc_ccon = ADC_DT_SPEC_GET(CCON_MEAS_NODE);

/* Common settings supported by most ADCs */
#define ADC_RESOLUTION          12
#define ADC_GAIN                ADC_GAIN_1
#define ADC_REFERENCE           ADC_REF_INTERNAL
#define ADC_ACQUISITION_TIME    ADC_ACQ_TIME_DEFAULT
#define ADC_REF_MV              3300

static int32_t sample_buffer;

struct adc_sequence sequence = {
	.buffer = &sample_buffer,
	/* buffer size in bytes, not number of samples */
	.buffer_size = sizeof(sample_buffer),
	.resolution = ADC_RESOLUTION,
};

void meas_vbus(int32_t *v)
{
	int ret;

	sequence.channels = BIT(adc_vbus.channel_id);
	ret = adc_read(adc_vbus.dev, &sequence);
	if (ret != 0) {
		printk("vbus voltage reading failed with error %d.\n", ret);
		return;
	}

	*v = sample_buffer;
	ret = adc_raw_to_millivolts(ADC_REF_MV, ADC_GAIN, ADC_RESOLUTION, v);
	if (ret != 0) {
		printk("Scaling ADC voltage failed with error %d.\n", ret);
		return;
	}

	*v = *v * DT_PROP(VBUS_MEAS_NODE, full_ohms) / DT_PROP(VBUS_MEAS_NODE, output_ohms);

	return;
}

void meas_cbus(int32_t *c)
{
	int ret;

	sequence.channels = BIT(adc_cbus.channel_id);
	ret = adc_read(adc_cbus.dev, &sequence);
	if (ret != 0) {
		printk("vbus current reading failed with error %d.\n", ret);
		return;
	}

	*c = sample_buffer;
	ret = adc_raw_to_millivolts(ADC_REF_MV, ADC_GAIN, ADC_RESOLUTION, c);
	if (ret != 0) {
		printk("Scaling ADC current failed with error %d.\n", ret);
		return;
	}

	*c = (*c - ADC_REF_MV / 2) * 10 / 3;

	return;
}

void meas_vcc1(int32_t *v)
{
	int ret;

	sequence.channels = BIT(adc_vcc1.channel_id);
	ret = adc_read(adc_vcc1.dev, &sequence);
	if (ret != 0) {
		printk("ADC voltage reading failed with error %d.\n", ret);
		return;
	}

	*v = sample_buffer;
	ret = adc_raw_to_millivolts(ADC_REF_MV, ADC_GAIN, ADC_RESOLUTION, v);
	if (ret != 0) {
		printk("Scaling ADC voltage failed with error %d.\n", ret);
		return;
	}

	return;
}

void meas_vcc2(int32_t *v)
{
	int ret;

	sequence.channels = BIT(adc_vcc2.channel_id);
	ret = adc_read(adc_vcc2.dev, &sequence);
	if (ret != 0) {
		printk("vbus voltage reading failed with error %d.\n", ret);
		return;
	}

	*v = sample_buffer;
	ret = adc_raw_to_millivolts(ADC_REF_MV, ADC_GAIN, ADC_RESOLUTION, v);
	if (ret != 0) {
		printk("Scaling ADC voltage failed with error %d.\n", ret);
		return;
	}

	return;
}

void meas_ccon(int32_t *c)
{
	int ret;

	sequence.channels = BIT(adc_ccon.channel_id);
	ret = adc_read(adc_ccon.dev, &sequence);
	if (ret != 0) {
		printk("vbus current reading failed with error %d.\n", ret);
		return;
	}

	*c = sample_buffer;
	ret = adc_raw_to_millivolts(ADC_REF_MV, ADC_GAIN, ADC_RESOLUTION, c);
	if (ret != 0) {
		printk("Scaling ADC current failed with error %d.\n", ret);
		return;
	}

	*c *= 4;

	return;
}

int meas_init(void)
{
	int ret;

	ret = adc_channel_setup_dt(&adc_vcc1);
	if (ret != 0) {
		return ret;
	}

	ret = adc_channel_setup_dt(&adc_vcc2);
	if (ret != 0) {
		return ret;
	}

	ret = adc_channel_setup_dt(&adc_vbus);
	if (ret != 0) {
		return ret;
	}

	ret = adc_channel_setup_dt(&adc_cbus);
	if (ret != 0) {
		return ret;
	}

	ret = adc_channel_setup_dt(&adc_ccon);
	if (ret != 0) {
		return ret;
	}

	return 0;

}
