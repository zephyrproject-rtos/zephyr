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

void ili9340_lcd_init(struct ili9340_data *p_ili9340)
{
	uint8_t cmd;
	uint8_t data[15];

	/* Software reset */
	cmd = ILI9340_CMD_SOFTWARE_RESET;
	ili9340_transmit(p_ili9340, cmd, NULL, 0);

	k_sleep(K_MSEC(5));

	cmd = ILI9341_CMD_POWER_CTRL_B;
	data[0] = 0x00U;
	data[1] = 0x8BU;
	data[2] = 0x30U;
	ili9340_transmit(p_ili9340, cmd, data, 3);

	cmd = ILI9341_CMD_POWER_ON_SEQ_CTRL;
	data[0] = 0x67U;
	data[1] = 0x03U;
	data[2] = 0x12U;
	data[3] = 0x81U;
	ili9340_transmit(p_ili9340, cmd, data, 4);

	cmd = ILI9341_CMD_DRVR_TIMING_CTRL_A_I;
	data[0] = 0x85U;
	data[1] = 0x10U;
	data[2] = 0x7AU;
	ili9340_transmit(p_ili9340, cmd, data, 3);

	cmd = ILI9341_CMD_POWER_CTRL_A;
	data[0] = 0x39U;
	data[1] = 0x2CU;
	data[2] = 0x00U;
	data[3] = 0x34U;
	data[4] = 0x02U;
	ili9340_transmit(p_ili9340, cmd, data, 5);

	cmd = ILI9341_CMD_PUMP_RATIO_CTRL;
	data[0] = 0x20U;
	ili9340_transmit(p_ili9340, cmd, data, 1);

	cmd = ILI9341_CMD_DRVR_TIMING_CTRL_B;
	data[0] = 0x00U;
	data[1] = 0x00U;
	ili9340_transmit(p_ili9340, cmd, data, 2);

	/* Power control */
	/* VRH[5:0] */
	cmd = ILI9340_CMD_POWER_CTRL_1;
	data[0] = 0x1BU;
	ili9340_transmit(p_ili9340, cmd, data, 1);

	/* Power control */
	/* SAP[2:0];BT[3:0] */
	cmd = ILI9340_CMD_POWER_CTRL_2;
	data[0] = 0x10U;
	ili9340_transmit(p_ili9340, cmd, data, 1);

	/* VCM control */
	cmd = ILI9340_CMD_VCOM_CTRL_1;
	data[0] = 0x3FU;
	data[1] = 0x3CU;
	ili9340_transmit(p_ili9340, cmd, data, 2);

	/* VCM control2 */
	cmd = ILI9340_CMD_VCOM_CTRL_2;
	data[0] = 0xB7U;
	ili9340_transmit(p_ili9340, cmd, data, 1);

	/* Memory Access Control */
	cmd = ILI9340_CMD_MEM_ACCESS_CTRL;
	data[0] = ILI9340_DATA_MEM_ACCESS_CTRL_MY |
		  ILI9340_DATA_MEM_ACCESS_CTRL_MV |
		  ILI9340_DATA_MEM_ACCESS_CTRL_ML |
		  ILI9340_DATA_MEM_ACCESS_CTRL_BGR;
	ili9340_transmit(p_ili9340, cmd, data, 1);

	/* Pixel Format Set */
	cmd = ILI9340_CMD_PIXEL_FORMAT_SET;
#ifdef CONFIG_ILI9340_RGB565
	data[0] = ILI9340_DATA_PIXEL_FORMAT_MCU_16_BIT |
		  ILI9340_DATA_PIXEL_FORMAT_RGB_16_BIT;
#else
	data[0] = ILI9340_DATA_PIXEL_FORMAT_MCU_18_BIT |
		  ILI9340_DATA_PIXEL_FORMAT_RGB_18_BIT;
#endif
	ili9340_transmit(p_ili9340, cmd, data, 1);

	/* Frame Rate */
	cmd = ILI9340_CMD_FRAME_CTRL_NORMAL_MODE;
	data[0] = 0x00U;
	data[1] = 0x1BU;
	ili9340_transmit(p_ili9340, cmd, data, 2);

	/* Display Function Control */
	cmd = ILI9340_CMD_DISPLAY_FUNCTION_CTRL;
	data[0] = 0x0AU;
	data[1] = 0xA2U;
	ili9340_transmit(p_ili9340, cmd, data, 2);

	/* 3Gamma Function Disable */
	cmd = ILI9341_CMD_ENABLE_3G;
	data[0] = 0x00U;
	ili9340_transmit(p_ili9340, cmd, data, 1);

	/* Gamma curve selected */
	cmd = ILI9340_CMD_GAMMA_SET;
	data[0] = 0x01U;
	ili9340_transmit(p_ili9340, cmd, data, 1);

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
	ili9340_transmit(p_ili9340, cmd, data, 15);

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
	ili9340_transmit(p_ili9340, cmd, data, 15);

	/* Sleep Out */
	cmd = ILI9340_CMD_EXIT_SLEEP;
	ili9340_transmit(p_ili9340, cmd, NULL, 0);

	k_sleep(K_MSEC(120));

	/* Display Off */
	cmd = ILI9340_CMD_DISPLAY_OFF;
	ili9340_transmit(p_ili9340, cmd, NULL, 0);
}
