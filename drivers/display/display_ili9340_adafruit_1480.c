/*
 * Copyright (c) 2017 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "display_ili9340.h"

void ili9340_lcd_init(struct ili9340_data *data)
{
	uint8_t tx_data[15];

	tx_data[0] = 0x23;
	ili9340_transmit(data, ILI9340_CMD_POWER_CTRL_1, tx_data, 1);

	tx_data[0] = 0x10;
	ili9340_transmit(data, ILI9340_CMD_POWER_CTRL_2, tx_data, 1);

	tx_data[0] = 0x3e;
	tx_data[1] = 0x28;
	ili9340_transmit(data, ILI9340_CMD_VCOM_CTRL_1, tx_data, 2);

	tx_data[0] = 0x86;
	ili9340_transmit(data, ILI9340_CMD_VCOM_CTRL_2, tx_data, 1);

	tx_data[0] =
	    ILI9340_DATA_MEM_ACCESS_CTRL_MV | ILI9340_DATA_MEM_ACCESS_CTRL_BGR;
	ili9340_transmit(data, ILI9340_CMD_MEM_ACCESS_CTRL, tx_data, 1);

#ifdef CONFIG_ILI9340_RGB565
	tx_data[0] = ILI9340_DATA_PIXEL_FORMAT_MCU_16_BIT |
		     ILI9340_DATA_PIXEL_FORMAT_RGB_16_BIT;
#else
	tx_data[0] = ILI9340_DATA_PIXEL_FORMAT_MCU_18_BIT |
		     ILI9340_DATA_PIXEL_FORMAT_RGB_18_BIT;
#endif
	ili9340_transmit(data, ILI9340_CMD_PIXEL_FORMAT_SET, tx_data, 1);

	tx_data[0] = 0x00;
	tx_data[1] = 0x18;
	ili9340_transmit(data, ILI9340_CMD_FRAME_CTRL_NORMAL_MODE, tx_data, 2);

	tx_data[0] = 0x08;
	tx_data[1] = 0x82;
	tx_data[2] = 0x27;
	ili9340_transmit(data, ILI9340_CMD_DISPLAY_FUNCTION_CTRL, tx_data, 3);

	tx_data[0] = 0x01;
	ili9340_transmit(data, ILI9340_CMD_GAMMA_SET, tx_data, 1);

	tx_data[0] = 0x0F;
	tx_data[1] = 0x31;
	tx_data[2] = 0x2B;
	tx_data[3] = 0x0C;
	tx_data[4] = 0x0E;
	tx_data[5] = 0x08;
	tx_data[6] = 0x4E;
	tx_data[7] = 0xF1;
	tx_data[8] = 0x37;
	tx_data[9] = 0x07;
	tx_data[10] = 0x10;
	tx_data[11] = 0x03;
	tx_data[12] = 0x0E;
	tx_data[13] = 0x09;
	tx_data[14] = 0x00;
	ili9340_transmit(data, ILI9340_CMD_POSITIVE_GAMMA_CORRECTION, tx_data,
			 15);

	tx_data[0] = 0x00;
	tx_data[1] = 0x0E;
	tx_data[2] = 0x14;
	tx_data[3] = 0x03;
	tx_data[4] = 0x11;
	tx_data[5] = 0x07;
	tx_data[6] = 0x31;
	tx_data[7] = 0xC1;
	tx_data[8] = 0x48;
	tx_data[9] = 0x08;
	tx_data[10] = 0x0F;
	tx_data[11] = 0x0C;
	tx_data[12] = 0x31;
	tx_data[13] = 0x36;
	tx_data[14] = 0x0F;
	ili9340_transmit(data, ILI9340_CMD_NEGATIVE_GAMMA_CORRECTION, tx_data,
			 15);
}
