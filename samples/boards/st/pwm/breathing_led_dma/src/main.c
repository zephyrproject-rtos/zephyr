/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/sys/printk.h>

static const struct device *const dma_dev = DEVICE_DT_GET(DT_ALIAS(dmaa));
static const struct device *const pwm_led_dev = DEVICE_DT_GET(DT_ALIAS(pribreath));
static const struct device *const pwm_sec_out_dev = DEVICE_DT_GET(DT_ALIAS(secbreath));


uint32_t ledPeriod = 20000000; // 20 ms 
uint32_t pwmTableLed[] = {
	1000000, 1600000, 2200000, 2800000, 3400000, 4000000, 4600000, 5200000,
	5800000, 6400000, 7000000, 7600000, 8200000, 8800000, 9400000, 10000000,
	10600000, 11200000, 11800000, 12400000, 13000000, 13600000, 14200000, 14800000,
	15400000, 16000000, 16600000, 17200000, 17800000, 18400000, 19000000, 18400000,
	17800000, 17200000, 16600000, 16000000, 15400000, 14800000, 14200000, 13600000,
	13000000, 12400000, 11800000, 11200000, 10600000, 10000000, 9400000, 8800000,
	8200000, 7600000, 7000000, 6400000, 5800000, 5200000, 4600000, 4000000,
	3400000, 2800000, 2200000, 1600000
};

uint32_t pwmTableOut[] = {
	1000000, 2200000, 3400000, 4600000, 5800000, 7000000, 8200000, 9400000,
	10600000, 11800000, 13000000, 14200000, 15400000, 16600000, 17800000, 19000000,
	17800000, 16600000, 15400000, 14200000, 13000000, 11800000, 10600000, 9400000,
	8200000, 7000000, 5800000, 4600000, 3400000, 2200000, 1600000
};

int main(void)
{
	if (!device_is_ready(pwm_led_dev)) {
		printk("PWM device is not ready\n");
		return 0;
	}

	if (!device_is_ready(pwm_sec_out_dev)) {
		printk("PWM device is not ready\n");
		return 0;
	}

	if (!device_is_ready(dma_dev)) {
		printk("DMA device is not ready\n");
		return 0;
	}

	pwm_set_dma(pwm_led_dev, CONFIG_PRIMARY_BREATH_CHANNEL, ledPeriod, pwmTableLed, sizeof(pwmTableLed) / sizeof(uint32_t), PWM_POLARITY_NORMAL);
	pwm_set_dma(pwm_sec_out_dev, CONFIG_SECONDARY_BREATH_CHANNEL, ledPeriod, pwmTableOut, sizeof(pwmTableOut) / sizeof(uint32_t), PWM_POLARITY_NORMAL);

	while (1) {
		k_msleep(500);
	}

}
