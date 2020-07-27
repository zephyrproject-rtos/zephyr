/*
 * Copyright (c) 2020 Libre Solar Technologies GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <drivers/dac.h>
#include <zephyr.h>
#include <ztest.h>

#if defined(CONFIG_BOARD_NUCLEO_L073RZ) || \
	defined(CONFIG_BOARD_NUCLEO_L152RE)

#define DAC_DEVICE_NAME		DT_LABEL(DT_NODELABEL(dac1))
#define DAC_CHANNEL_ID		1
#define DAC_RESOLUTION		12

#elif defined(CONFIG_BOARD_TWR_KE18F)

#define DAC_DEVICE_NAME		DT_LABEL(DT_NODELABEL(dac0))
#define DAC_RESOLUTION		12
#define DAC_CHANNEL_ID		0

#elif defined(CONFIG_BOARD_FRDM_K64F)

#define DAC_DEVICE_NAME		DT_LABEL(DT_NODELABEL(dac0))
#define DAC_RESOLUTION		12
#define DAC_CHANNEL_ID		0

#else
#error "Unsupported board."
#endif

static const struct dac_channel_cfg dac_ch_cfg = {
	.channel_id  = DAC_CHANNEL_ID,
	.resolution  = DAC_RESOLUTION
};

struct device *get_dac_device(void)
{
	return device_get_binding(DAC_DEVICE_NAME);
}

static struct device *init_dac(void)
{
	int ret;
	struct device *dac_dev = device_get_binding(DAC_DEVICE_NAME);

	zassert_not_null(dac_dev, "Cannot get DAC device");

	ret = dac_channel_setup(dac_dev, &dac_ch_cfg);
	zassert_equal(ret, 0,
		"Setting up of the first channel failed with code %d", ret);

	return dac_dev;
}

/*
 * test_dac_write_value
 */
static int test_task_write_value(void)
{
	int ret;

	struct device *dac_dev = init_dac();

	if (!dac_dev) {
		return TC_FAIL;
	}

	/* write a value of half the full scale resolution */
	ret = dac_write_value(dac_dev, DAC_CHANNEL_ID,
						(1U << DAC_RESOLUTION) / 2);
	zassert_equal(ret, 0, "dac_write_value() failed with code %d", ret);

	return TC_PASS;
}

void test_dac_write_value(void)
{
	zassert_true(test_task_write_value() == TC_PASS, NULL);
}
