/*
 * Copyright (c) 2020 Libre Solar Technologies GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/drivers/dac.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#if defined(CONFIG_BOARD_NUCLEO_F091RC) || \
	defined(CONFIG_BOARD_NUCLEO_F207ZG) || \
	defined(CONFIG_BOARD_STM32F3_DISCO) || \
	defined(CONFIG_BOARD_NUCLEO_F429ZI) || \
	defined(CONFIG_BOARD_NUCLEO_F746ZG) || \
	defined(CONFIG_BOARD_NUCLEO_F767ZI) || \
	defined(CONFIG_BOARD_NUCLEO_G071RB) || \
	defined(CONFIG_BOARD_NUCLEO_G431RB) || \
	defined(CONFIG_BOARD_NUCLEO_G474RE) || \
	defined(CONFIG_BOARD_NUCLEO_H743ZI) || \
	defined(CONFIG_BOARD_NUCLEO_L073RZ) || \
	defined(CONFIG_BOARD_NUCLEO_L152RE) || \
	defined(CONFIG_BOARD_DISCO_L475_IOT1) || \
	defined(CONFIG_BOARD_NUCLEO_L552ZE_Q) || \
	defined(CONFIG_BOARD_STM32L562E_DK) || \
	defined(CONFIG_BOARD_STM32H573I_DK) || \
	defined(CONFIG_BOARD_B_U585I_IOT02A) || \
	defined(CONFIG_BOARD_NUCLEO_U575ZI_Q) || \
	defined(CONFIG_BOARD_NUCLEO_U5A5ZJ_Q) || \
	defined(CONFIG_BOARD_NUCLEO_WL55JC) || \
	defined(CONFIG_BOARD_RONOTH_LODEV)

#define DAC_DEVICE_NODE		DT_NODELABEL(dac1)
#define DAC_CHANNEL_ID		1
#define DAC_RESOLUTION		12

#elif defined(CONFIG_BOARD_NUCLEO_H563ZI)

#define DAC_DEVICE_NODE		DT_NODELABEL(dac1)
#define DAC_CHANNEL_ID		2
#define DAC_RESOLUTION		12

#elif defined(CONFIG_BOARD_TWR_KE18F) || \
	defined(CONFIG_BOARD_FRDM_K64F) || \
	defined(CONFIG_BOARD_FRDM_K22F) || \
	defined(CONFIG_BOARD_SEEEDUINO_XIAO) || \
	defined(CONFIG_BOARD_ARDUINO_MKRZERO) || \
	defined(CONFIG_BOARD_ARDUINO_ZERO) || \
	defined(CONFIG_BOARD_LPCXPRESSO55S36)

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

#elif defined(CONFIG_BOARD_ESP32_DEVKITC_WROOM) || \
	defined(CONFIG_BOARD_ESP32_DEVKITC_WROVER) || \
	defined(CONFIG_BOARD_ESP_WROVER_KIT) || \
	defined(CONFIG_BOARD_ESP32S2_SAOLA) || \
	defined(CONFIG_BOARD_GD32A503V_EVAL) || \
	defined(CONFIG_BOARD_GD32E103V_EVAL) || \
	defined(CONFIG_BOARD_GD32F450I_EVAL) || \
	defined(CONFIG_BOARD_GD32F450Z_EVAL) || \
	defined(CONFIG_BOARD_GD32F470I_EVAL) || \
	defined(CONFIG_BOARD_YD_ESP32)

#define DAC_DEVICE_NODE		DT_NODELABEL(dac)
#define DAC_RESOLUTION		12
#define DAC_CHANNEL_ID		0

#elif defined(CONFIG_BOARD_SAM_E70_XPLAINED) || \
	defined(CONFIG_BOARD_SAM_V71_XULT)

#define DAC_DEVICE_NODE		DT_NODELABEL(dacc)
#define DAC_RESOLUTION		12
#define DAC_CHANNEL_ID		0

#else
#error "Unsupported board."
#endif

static const struct dac_channel_cfg dac_ch_cfg = {
	.channel_id  = DAC_CHANNEL_ID,
	.resolution  = DAC_RESOLUTION,
	.buffered = true
};

const struct device *get_dac_device(void)
{
	return DEVICE_DT_GET(DAC_DEVICE_NODE);
}

static const struct device *init_dac(void)
{
	int ret;
	const struct device *const dac_dev = DEVICE_DT_GET(DAC_DEVICE_NODE);

	zassert_true(device_is_ready(dac_dev), "DAC device is not ready");

	ret = dac_channel_setup(dac_dev, &dac_ch_cfg);
	zassert_equal(ret, 0,
		"Setting up of the first channel failed with code %d", ret);

	return dac_dev;
}

/*
 * test_dac_write_value
 */
ZTEST(dac, test_task_write_value)
{
	int ret;

	const struct device *dac_dev = init_dac();

	/* write a value of half the full scale resolution */
	ret = dac_write_value(dac_dev, DAC_CHANNEL_ID,
						(1U << DAC_RESOLUTION) / 2);
	zassert_equal(ret, 0, "dac_write_value() failed with code %d", ret);
}

static void *dac_setup(void)
{
	k_object_access_grant(get_dac_device(), k_current_get());

	return NULL;
}

ZTEST_SUITE(dac, NULL, dac_setup, NULL, NULL, NULL);
