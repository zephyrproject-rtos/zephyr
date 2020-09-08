/*
 * Copyright (c) 2018 - 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <drivers/display.h>
#include "display_ili9340.h"

/*
 * Derived from Seeed 2.8 inch TFT Touch Shield v2.0 sample code.
 *
 * https://github.com/Seeed-Studio/TFT_Touch_Shield_V2
 */

int ili9340_lcd_init(const struct device *dev)
{
	int r;
	uint8_t cmd;
	uint8_t data[15];

	/* Software reset */
	cmd = ILI9340_CMD_SOFTWARE_RESET;
	r = ili9340_transmit(dev, cmd, NULL, 0);
	if (r < 0) {
		return r;
	}

	k_sleep(K_MSEC(5));

	cmd = ILI9341_CMD_POWER_CTRL_B;
	data[0] = 0x00U;
	data[1] = 0x8BU;
	data[2] = 0x30U;
	r = ili9340_transmit(dev, cmd, data, 3);
	if (r < 0) {
		return r;
	}

	cmd = ILI9341_CMD_POWER_ON_SEQ_CTRL;
	data[0] = 0x67U;
	data[1] = 0x03U;
	data[2] = 0x12U;
	data[3] = 0x81U;
	r = ili9340_transmit(dev, cmd, data, 4);
	if (r < 0) {
		return r;
	}

	cmd = ILI9341_CMD_DRVR_TIMING_CTRL_A_I;
	data[0] = 0x85U;
	data[1] = 0x10U;
	data[2] = 0x7AU;
	r = ili9340_transmit(dev, cmd, data, 3);
	if (r < 0) {
		return r;
	}

	cmd = ILI9341_CMD_POWER_CTRL_A;
	data[0] = 0x39U;
	data[1] = 0x2CU;
	data[2] = 0x00U;
	data[3] = 0x34U;
	data[4] = 0x02U;
	r = ili9340_transmit(dev, cmd, data, 5);
	if (r < 0) {
		return r;
	}

	cmd = ILI9341_CMD_PUMP_RATIO_CTRL;
	data[0] = 0x20U;
	r = ili9340_transmit(dev, cmd, data, 1);
	if (r < 0) {
		return r;
	}

	cmd = ILI9341_CMD_DRVR_TIMING_CTRL_B;
	data[0] = 0x00U;
	data[1] = 0x00U;
	r = ili9340_transmit(dev, cmd, data, 2);
	if (r < 0) {
		return r;
	}

	/* Power control */
	/* VRH[5:0] */
	cmd = ILI9340_CMD_POWER_CTRL_1;
	data[0] = 0x1BU;
	r = ili9340_transmit(dev, cmd, data, 1);
	if (r < 0) {
		return r;
	}

	/* Power control */
	/* SAP[2:0];BT[3:0] */
	cmd = ILI9340_CMD_POWER_CTRL_2;
	data[0] = 0x10U;
	r = ili9340_transmit(dev, cmd, data, 1);
	if (r < 0) {
		return r;
	}

	/* VCM control */
	cmd = ILI9340_CMD_VCOM_CTRL_1;
	data[0] = 0x3FU;
	data[1] = 0x3CU;
	r = ili9340_transmit(dev, cmd, data, 2);
	if (r < 0) {
		return r;
	}

	/* VCM control2 */
	cmd = ILI9340_CMD_VCOM_CTRL_2;
	data[0] = 0xB7U;
	r = ili9340_transmit(dev, cmd, data, 1);
	if (r < 0) {
		return r;
	}

	/* Memory Access Control */
	cmd = ILI9340_CMD_MEM_ACCESS_CTRL;
	data[0] = ILI9340_DATA_MEM_ACCESS_CTRL_MY |
		  ILI9340_DATA_MEM_ACCESS_CTRL_MV |
		  ILI9340_DATA_MEM_ACCESS_CTRL_ML |
		  ILI9340_DATA_MEM_ACCESS_CTRL_BGR;
	r = ili9340_transmit(dev, cmd, data, 1);
	if (r < 0) {
		return r;
	}

	/* Frame Rate */
	cmd = ILI9340_CMD_FRAME_CTRL_NORMAL_MODE;
	data[0] = 0x00U;
	data[1] = 0x1BU;
	r = ili9340_transmit(dev, cmd, data, 2);
	if (r < 0) {
		return r;
	}

	/* Display Function Control */
	cmd = ILI9340_CMD_DISPLAY_FUNCTION_CTRL;
	data[0] = 0x0AU;
	data[1] = 0xA2U;
	r = ili9340_transmit(dev, cmd, data, 2);
	if (r < 0) {
		return r;
	}

	/* 3Gamma Function Disable */
	cmd = ILI9341_CMD_ENABLE_3G;
	data[0] = 0x00U;
	r = ili9340_transmit(dev, cmd, data, 1);
	if (r < 0) {
		return r;
	}

	/* Gamma curve selected */
	cmd = ILI9340_CMD_GAMMA_SET;
	data[0] = 0x01U;
	r = ili9340_transmit(dev, cmd, data, 1);
	if (r < 0) {
		return r;
	}

	/* Positive Gamma Correction */
	cmd = ILI9340_CMD_POSITIVE_GAMMA_CORRECTION;
	data[0] = 0x0FU;
	data[1] = 0x2AU;
	data[2] = 0x28U;
	data[3] = 0x08U;
	data[4] = 0x0EU;
	data[5] = 0x08U;
	data[6] = 0x54U;
	data[7] = 0xA9U;
	data[8] = 0x43U;
	data[9] = 0x0AU;
	data[10] = 0x0FU;
	data[11] = 0x00U;
	data[12] = 0x00U;
	data[13] = 0x00U;
	data[14] = 0x00U;
	r = ili9340_transmit(dev, cmd, data, 15);
	if (r < 0) {
		return r;
	}

	/* Negative Gamma Correction */
	cmd = ILI9340_CMD_NEGATIVE_GAMMA_CORRECTION;
	data[0] = 0x00U;
	data[1] = 0x15U;
	data[2] = 0x17U;
	data[3] = 0x07U;
	data[4] = 0x11U;
	data[5] = 0x06U;
	data[6] = 0x2BU;
	data[7] = 0x56U;
	data[8] = 0x3CU;
	data[9] = 0x05U;
	data[10] = 0x10U;
	data[11] = 0x0FU;
	data[12] = 0x3FU;
	data[13] = 0x3FU;
	data[14] = 0x0FU;
	r = ili9340_transmit(dev, cmd, data, 15);
	if (r < 0) {
		return r;
	}

	return 0;
}
