/*
 * Copyright (c) 2020 Oticon A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys/printk.h>
#include <device.h>
#include <drivers/audio/pdm.h>
#include <drivers/gpio.h>

#define THINGY52_MIC_PWR_PIN 9
#define MIC_PWR_ON 1
#define MAX_ITERATIONS 4
#define MAX_SAMPLES 100
#define PDM_BUFFER_SIZE CONFIG_PDM_BUFFER_SIZE
#define MEM_SLAB_BLOCKS 4

#define PDM_NODE   DT_NODELABEL(pdm)
#define PDM_LABEL  DT_LABEL(PDM_NODE)
#define GPIO_NODE  DT_NODELABEL(sx1509b)
#define GPIO_LABEL DT_LABEL(GPIO_NODE)

static struct device *pdm_dev;
static struct k_mem_slab pdm_mem_slab;
static __aligned(4) int16_t pdm_buffers[PDM_BUFFER_SIZE][MEM_SLAB_BLOCKS];
static uint16_t samples, iterations;


static void pdm_data_handler(int16_t *data, uint16_t data_size)
{
	samples++;
	if (samples == MAX_SAMPLES) {
		samples = 0;
		iterations++;
		printk("[ITERATION %u/%d] Received %d samples of size %u\n",
			iterations, MAX_ITERATIONS, MAX_SAMPLES, data_size);
		int result = pdm_stop(pdm_dev);

		if (result != 0) {
			printk("Failed to start PDM sampling, error %d\n",
				result);
			return;
		}
		if (iterations != MAX_ITERATIONS) {
			result = pdm_start(pdm_dev);
			if (result != 0) {
				printk("Failed to start PDM sampling, error %d\n",
					result);
				return;
			}
		} else {
			printk("Program finished.\n");
		}
	}
	k_mem_slab_free(&pdm_mem_slab, (void *)&data);
}

static struct pdm_config pdm_cfg = {
	.data_handler	= pdm_data_handler,
	.mem_slab	= &pdm_mem_slab,
};

int main(void)
{
	int result;

	samples = 0;
	iterations = 0;

	result = k_mem_slab_init(&pdm_mem_slab, &pdm_buffers,
				 sizeof(int16_t) * PDM_BUFFER_SIZE,
				 MEM_SLAB_BLOCKS);
	if (result != 0) {
		printk("Failed to initialize the mem slab! Error %d\n",
			result);
		return result;
	}

	pdm_dev = device_get_binding(PDM_LABEL);
	if (!pdm_dev) {
		printk("Cannot find %s!\n", PDM_LABEL);
		return -ENOENT;
	}
	printk("Successfully bound PDM driver.\n");

	struct device *gpio_dev = device_get_binding(GPIO_LABEL);

	if (!gpio_dev) {
		printk("Cannot find %s!\n", GPIO_LABEL);
		return -ENOENT;
	}
	printk("Successfully bound GPIO driver.\n");

	result = gpio_pin_configure(gpio_dev, THINGY52_MIC_PWR_PIN,
					GPIO_OUTPUT_ACTIVE);
	if (result != 0) {
		printk("Failed to configure pin %d, error %d\n",
			THINGY52_MIC_PWR_PIN, result);
		return result;
	}

	result = gpio_pin_set(gpio_dev, THINGY52_MIC_PWR_PIN, MIC_PWR_ON);
	if (result != 0) {
		printk("Failed to turn on the microphone, error %d\n", result);
		return result;
	}

	result = pdm_configure(pdm_dev, &pdm_cfg);
	if (result != 0) {
		printk("Failed to set the data handler, error %d\n", result);
		return result;
	}

	result = pdm_start(pdm_dev);
	if (result != 0) {
		printk("Failed to start PDM sampling, error %d\n", result);
		return result;
	}

	return 0;
}
