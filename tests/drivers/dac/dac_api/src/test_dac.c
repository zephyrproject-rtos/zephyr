/*
 * Copyright (c) 2020 Libre Solar Technologies GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/drivers/dac.h>
#include <zephyr/zephyr.h>
#include <ztest.h>

#if defined(CONFIG_BOARD_NUCLEO_F091RC) || \
	defined(CONFIG_BOARD_NUCLEO_F207ZG) || \
	defined(CONFIG_BOARD_STM32F3_DISCO) || \
	defined(CONFIG_BOARD_NUCLEO_F429ZI) || \
	defined(CONFIG_BOARD_NUCLEO_F746ZG) || \
	defined(CONFIG_BOARD_NUCLEO_F767ZI) || \
	defined(CONFIG_BOARD_NUCLEO_G071RB) || \
	defined(CONFIG_BOARD_NUCLEO_G474RE) || \
	defined(CONFIG_BOARD_NUCLEO_H743ZI) || \
	defined(CONFIG_BOARD_NUCLEO_L073RZ) || \
	defined(CONFIG_BOARD_NUCLEO_L152RE) || \
	defined(CONFIG_BOARD_DISCO_L475_IOT1) || \
	defined(CONFIG_BOARD_NUCLEO_L552ZE_Q) || \
	defined(CONFIG_BOARD_STM32L562E_DK) || \
	defined(CONFIG_BOARD_B_U585I_IOT02A) || \
	defined(CONFIG_BOARD_NUCLEO_WL55JC) || \
	defined(CONFIG_BOARD_RONOTH_LODEV)

#define DAC_DEVICE_NODE		DT_NODELABEL(dac1)
#define DAC_CHANNEL_ID		1
#define DAC_RESOLUTION		12

#elif defined(CONFIG_BOARD_TWR_KE18F)

#define DAC_DEVICE_NODE		DT_NODELABEL(dac0)
#define DAC_RESOLUTION		12
#define DAC_CHANNEL_ID		0

#elif defined(CONFIG_BOARD_FRDM_K64F)

#define DAC_DEVICE_NODE		DT_NODELABEL(dac0)
#define DAC_RESOLUTION		12
#define DAC_CHANNEL_ID		0

#elif defined(CONFIG_BOARD_FRDM_K22F)

#define DAC_DEVICE_NODE		DT_NODELABEL(dac0)
#define DAC_RESOLUTION		12
#define DAC_CHANNEL_ID		0

#elif defined(CONFIG_BOARD_BL652_DVK) || \
	defined(CONFIG_BOARD_BL653_DVK) || \
	defined(CONFIG_BOARD_BL654_DVK) || \
	defined(CONFIG_BOARD_BL5340_DVK_CPUAPP)
 /* Note external DAC MCP4725 is not populated on BL652_DVK, BL653_DVK and
  * BL654_DVK at factory
  */
#define DAC_DEVICE_NODE		DT_NODELABEL(dac0)
#define DAC_RESOLUTION		12
#define DAC_CHANNEL_ID		0

#else
#error "Unsupported board."
#endif

static const struct dac_channel_cfg dac_ch_cfg = {
	.channel_id  = DAC_CHANNEL_ID,
	.resolution  = DAC_RESOLUTION
};

const struct device *get_dac_device(void)
{
	return DEVICE_DT_GET(DAC_DEVICE_NODE);
}

static const struct device *init_dac(void)
{
	int ret;
	const struct device *dac_dev = DEVICE_DT_GET(DAC_DEVICE_NODE);

	zassert_true(device_is_ready(dac_dev), "DAC device is not ready");

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

	const struct device *dac_dev = init_dac();

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
