/* AVIA Semiconductor HX711 24-Bit ADC with PGA for registor bridges
 * Copyright (c) 2024 Vinicius Miguel
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/adc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/dt-bindings/adc/hx711_adc.h>
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#define ADC_CONTEXT_USES_KERNEL_TIMER 1
#define ADC_CONTEXT_WAIT_FOR_COMPLETION_TIMEOUT                                                    \
	K_MSEC(CONFIG_ADC_HX711_WAIT_FOR_COMPLETION_TIMEOUT_MS)
#include "adc_context.h"

LOG_MODULE_REGISTER(hx711, CONFIG_ADC_LOG_LEVEL);

// Not applicable since the HX711 uses a resistor bridge with a current source.
#define HX711_REF_INTERNAL 0

#define HX711_CHANNEL_A_GAIN_128 128
#define HX711_CHANNEL_A_GAIN_64  64
#define HX711_CHANNEL_B_GAIN_32	 32

struct hx711_config {
	const struct gpio_dt_spec gpio_sck;
	const struct gpio_dt_spec gpio_dout;
	const struct gpio_dt_spec gpio_rate;
	enum adc_gain gain;
	uint8_t rate;
};

struct hx711_data {
	const struct device *dev;
	struct adc_context ctx;
	k_timeout_t ready_time;
	struct k_sem acq_sem;
	unsigned int *buffer;
	unsigned int *repeat_buffer;
	struct k_thread thread;
	bool differential;

	K_KERNEL_STACK_MEMBER(stack, CONFIG_ADC_HX711_ACQUISITION_THREAD_STACK_SIZE);
};

static bool hx711_adc_is_ready(const struct device *dev)
{
	const struct hx711_config *config = dev->config;
	int state;

	gpio_pin_get_dt(&config->gpio_dout, &state);

	return state == 0;
}

static void hx711_adc_power_down(const struct device *dev)
{
	const struct hx711_config *config = dev->config;

	gpio_pin_set_dt(&config->gpio_sck, 1);

	k_sleep(K_USEC(64));
}

static void hx711_adc_power_up(const struct device *dev)
{
	const struct hx711_config *config = dev->config;

	gpio_pin_set_dt(&config->gpio_sck, 0);
}

static int hx711_adc_channel_setup(const struct device *dev,
				 const struct adc_channel_cfg *channel_cfg)
{
	/*The HX711 does not have multiple channels on a conventional sense.
	* The channel selection is set by the gain setting. 
	* Gains 128 and 64 are channel A, while gain 32 is channel B.
	* The selection happens automatically when the gain is set during 
	* the acquisition process.
	*/
	return 0;
}

static void adc_context_update_buffer_pointer(struct adc_context *ctx, bool repeat_sampling)
{
	struct hx711_data *data = CONTAINER_OF(ctx, struct hx711_data, ctx);

	if (repeat_sampling) {
		data->buffer = data->repeat_buffer;
	}
}

static void adc_context_start_sampling(struct adc_context *ctx)
{
	struct hx711_data *data = CONTAINER_OF(ctx, struct hx711_data, ctx);
	int ret;

	data->repeat_buffer = data->buffer;

	ret = hx711_start_conversion(data->dev);
	if (ret != 0) {
		adc_context_complete(ctx, ret);
		return;
	}
	k_sem_give(&data->acq_sem);
}

static int hx711_adc_start_read(const struct device *dev, const struct adc_sequence *sequence)
{
	int rc;
	struct hx711_data *data = dev->data;

	rc = hx711_validate_sequence(dev, sequence);
	if (rc != 0) {
		return rc;
	}

	data->buffer = sequence->buffer;

	adc_context_start_read(&data->ctx, sequence);

	return adc_context_wait_for_completion(&data->ctx);
}

static int hx711_adc_read_async(const struct device *dev, const struct adc_sequence *sequence,
				  struct k_poll_signal *async)
{
	int rc;
	struct hx711_data *data = dev->data;

	adc_context_lock(&data->ctx, async ? true : false, async);
	rc = hx711_adc_start_read(dev, sequence);
	adc_context_release(&data->ctx, rc);

	return rc;
}

static uint8_t hx711_adc_read_byte(const struct device *dev)
{
  	struct hx711_data *data = dev->data;
	const struct hx711_config *config = dev->config;
	uint8_t value = 0;
	uint8_t mask  = 0x80;
	int state = 0;
	while (mask > 0)
	{
		gpio_pin_set_dt(&config->gpio_sck, 1);
		k_sleep(K_USEC(1));
		gpio_pin_get_dt(&config->gpio_dout, &state);
		if (state)
			value |= mask;

		gpio_pin_set_dt(&config->gpio_sck, 0);
		k_sleep(K_USEC(1));
		mask >>= 1;
	}
	return value;
}

static int hx711_adc_perform_read(const struct device *dev)
{
	int rc;
	struct hx711_data *data = dev->data;
	const struct hx711_config *config = dev->config;
	uint8_t m = 1;

	union
	{
		unsigned int value = 0;
		uint8_t fragments[4];
	} result;

	while (hx711_adc_is_ready(dev) == false)
		k_sleep(K_MSEC(1)); //12.5ms in 80Hz 100ms in 10Hz is the worst case.

	result.fragments[2] = hx711_adc_read_byte(dev);
	result.fragments[1] = hx711_adc_read_byte(dev);
	result.fragments[0] = hx711_adc_read_byte(dev);

	//The HX711 procotol specifies that after the acquisition is finished
	//the gain must be set for the next acquisition. This is done by pulsing
	//the SCK pin 1 to 3 times depending on the gain setting.
	if      (config->gain == HX711_CHANNEL_A_GAIN_128) m = 1;
	else if (config->gain == HX711_CHANNEL_A_GAIN_64)  m = 3;
	else if (config->gain == HX711_CHANNEL_B_GAIN_32)  m = 2;

	for (; m > 0; m--) {
		rc = gpio_pin_set_dt(&config->gpio_sck, 1);
		if(rc != 0)
			break;
		
		k_sleep(K_USEC(1));
		rc = gpio_pin_set_dt(&config->gpio_sck, 0);
		if(rc != 0)
			break;
		
		k_sleep(K_USEC(1));
	}

	if (rc != 0) {
		adc_context_complete(&data->ctx, rc);
		return rc;
	}
	//Sign extend
	if (result.fragments[2] & 0x80) result.fragments[3] = 0xFF;

	*data->buffer = result.value;

	adc_context_on_sampling_done(&data->ctx, dev);

	return rc;
}

static int hx711_read(const struct device *dev, const struct adc_sequence *sequence)
{
	return ads1x1x_adc_read_async(dev, sequence, NULL);
}

static void hx711_acquisition_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	const struct device *dev = p1;
	struct hx711_data *data = dev->data;
	int rc;

	while (true) {
		k_sem_take(&data->acq_sem, K_FOREVER);

		rc = hx711_wait_data_ready(dev);
		if (rc != 0) {
			LOG_ERR("failed to get ready status (err %d)", rc);
			adc_context_complete(&data->ctx, rc);
			continue;
		}

		hx711_adc_perform_read(dev);
	}
}

static int hx711_init(const struct device *dev)
{
	const struct ads1x1x_config *config = dev->config;
	struct ads1x1x_data *data = dev->data;
	data->dev = dev;

	k_sem_init(&data->acq_sem, 0, 1);

	if (!device_is_ready(config->gpio_sck)) {
		LOG_ERR("GPIO for SCK %s not ready", config->gpio_sck.name);
		return -ENODEV;
	}

	if (!device_is_ready(config->gpio_dout)) {
		LOG_ERR("GPIO for DOUT %s not ready", config->gpio_sck.name);
		return -ENODEV;
	}

	gpio_pin_configure_dt(&config->gpio_sck, GPIO_OUTPUT);
	gpio_pin_configure_dt(&config->gpio_dout, GPIO_INPUT);

	hx711_adc_power_down(dev);

	if (device_is_ready(config->gpio_rate)) {
		gpio_pin_configure_dt(&config->gpio_rate, GPIO_OUTPUT);
		gpio_pin_set_dt(&config->gpio_rate, 1);
	} else {
		LOG_INF("GPIO for RATE %s not ready", config->gpio_sck.name);
	}

	hx711_adc_power_up(dev);

	k_tid_t tid =
		k_thread_create(&data->thread, data->stack, K_THREAD_STACK_SIZEOF(data->stack),
				hx711_acquisition_thread, (void *)dev, NULL,
				NULL, CONFIG_ADC_HX711_ACQUISITION_THREAD_PRIO, 0, K_NO_WAIT);
	k_thread_name_set(tid, "adc_hx711");

	adc_context_unlock_unconditionally(&data->ctx);

	return 0;
}

static const struct adc_driver_api hx711_api = {
	.channel_setup = hx711_channel_setup,
	.read = hx711_read,
	.ref_internal = HX711_REF_INTERNAL,
#ifdef CONFIG_ADC_ASYNC
	.read_async = hx711_adc_read_async,
#endif
};

#define DT_INST_HX711(inst, t) DT_INST(inst, avia_ads##t)

#define HX711_INIT(t, n, odr_delay_us, res, mux, pgab)                                           \
	static const struct hx711_config ads##t##_config_##n = {                                 \
		.odr_delay = odr_delay_us,                                                         \
		.resolution = res,                                                                 \
		.multiplexer = mux,                                                                \
		.pga = pgab,                                                                       \
	};                                                                                         \
	static struct hx711_data ads##t##_data_##n = {                                           \
		ADC_CONTEXT_INIT_LOCK(ads##t##_data_##n, ctx),                                     \
		ADC_CONTEXT_INIT_TIMER(ads##t##_data_##n, ctx),                                    \
		ADC_CONTEXT_INIT_SYNC(ads##t##_data_##n, ctx),                                     \
	};                                                                                         \
	DEVICE_DT_DEFINE(DT_INST_HX711(n, t), hx711_init, NULL, &ads##t##_data_##n,            \
			 &ads##t##_config_##n, POST_KERNEL, CONFIG_ADC_HX711_INIT_PRIORITY,      \
			 &hx711_api);

DT_INST_FOREACH_STATUS_OKAY(HX711_INIT)
