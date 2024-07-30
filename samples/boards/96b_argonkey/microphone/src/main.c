/*
 * Copyright (c) 2018 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/led.h>

#include <zephyr/audio/dmic.h>

/* uncomment if you want PCM output in ascii */
/*#define PCM_OUTPUT_IN_ASCII		1  */

#define AUDIO_FREQ		16000
#define CHAN_SIZE		16
#define PCM_BLK_SIZE_MS		((AUDIO_FREQ/1000) * sizeof(int16_t))

#define NUM_MS		5000

K_MEM_SLAB_DEFINE(rx_mem_slab, PCM_BLK_SIZE_MS, NUM_MS, 1);

struct pcm_stream_cfg mic_streams = {
	.pcm_rate = AUDIO_FREQ,
	.pcm_width = CHAN_SIZE,
	.block_size = PCM_BLK_SIZE_MS,
	.mem_slab = &rx_mem_slab,
};

struct dmic_cfg cfg = {
	.io = {
		/* requesting a pdm freq around 2MHz */
		.min_pdm_clk_freq = 1800000,
		.max_pdm_clk_freq = 2500000,
	},
	.streams = &mic_streams,
	.channel = {
		.req_num_chan = 1,
	},
};

#define NUM_LEDS 12
#define DELAY_TIME K_MSEC(25)

static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);

void signal_sampling_started(void)
{
	gpio_pin_configure_dt(&led0, GPIO_OUTPUT_ACTIVE);
	gpio_pin_configure_dt(&led1, GPIO_OUTPUT_INACTIVE);
}

void signal_sampling_stopped(void)
{
	gpio_pin_configure_dt(&led0, GPIO_OUTPUT_ACTIVE);
	gpio_pin_configure_dt(&led1, GPIO_OUTPUT_ACTIVE);
}

void signal_print_stopped(void)
{
	gpio_pin_configure_dt(&led0, GPIO_OUTPUT_INACTIVE);
	gpio_pin_configure_dt(&led1, GPIO_OUTPUT_ACTIVE);
}

void *rx_block[NUM_MS];
size_t rx_size = PCM_BLK_SIZE_MS;

int main(void)
{
	int i;
	uint32_t ms;

	if (!gpio_is_ready_dt(&led0)) {
		printk("LED0 GPIO controller device is not ready\n");
		return 0;
	}

	if (!gpio_is_ready_dt(&led1)) {
		printk("LED1 GPIO controller device is not ready\n");
		return 0;
	}

#ifdef CONFIG_LP3943
	static const struct device *const ledc = DEVICE_DT_GET_ONE(ti_lp3943);

	if (!device_is_ready(ledc)) {
		printk("Device %s is not ready\n", ledc->name);
		return 0;
	}

	/* turn all leds on */
	for (i = 0; i < NUM_LEDS; i++) {
		led_on(ledc, i);
		k_sleep(DELAY_TIME);
	}

	/* turn all leds off */
	for (i = 0; i < NUM_LEDS; i++) {
		led_off(ledc, i);
		k_sleep(DELAY_TIME);
	}

#endif

	printk("ArgonKey test!!\n");

	int ret;

	const struct device *const mic_dev = DEVICE_DT_GET_ONE(st_mpxxdtyy);

	if (!device_is_ready(mic_dev)) {
		printk("Device %s is not ready\n", mic_dev->name);
		return 0;
	}

	ret = dmic_configure(mic_dev, &cfg);
	if (ret < 0) {
		printk("microphone configuration error\n");
		return 0;
	}

	ret = dmic_trigger(mic_dev, DMIC_TRIGGER_START);
	if (ret < 0) {
		printk("microphone start trigger error\n");
		return 0;
	}

	signal_sampling_started();

	/* Acquire microphone audio */
	for (ms = 0U; ms < NUM_MS; ms++) {
		ret = dmic_read(mic_dev, 0, &rx_block[ms], &rx_size, 2000);
		if (ret < 0) {
			printk("microphone audio read error\n");
			return 0;
		}
	}

	signal_sampling_stopped();

	ret = dmic_trigger(mic_dev, DMIC_TRIGGER_STOP);
	if (ret < 0) {
		printk("microphone stop trigger error\n");
		return 0;
	}

	/* print PCM stream */
#ifdef PCM_OUTPUT_IN_ASCII
	printk("-- start\n");
	int j;

	for (i = 0; i < NUM_MS; i++) {
		uint16_t *pcm_out = rx_block[i];

		for (j = 0; j < rx_size/2; j++) {
			printk("0x%04x,\n", pcm_out[j]);
		}
	}
	printk("-- end\n");
#else
	unsigned char pcm_l, pcm_h;
	int j;

	for (i = 0; i < NUM_MS; i++) {
		uint16_t *pcm_out = rx_block[i];

		for (j = 0; j < rx_size/2; j++) {
			pcm_l = (char)(pcm_out[j] & 0xFF);
			pcm_h = (char)((pcm_out[j] >> 8) & 0xFF);

			z_impl_k_str_out(&pcm_l, 1);
			z_impl_k_str_out(&pcm_h, 1);
		}
	}
#endif

	signal_print_stopped();
	return 0;
}
