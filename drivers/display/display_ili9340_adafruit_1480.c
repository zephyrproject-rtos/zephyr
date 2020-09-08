/*
 * Copyright (c) 2017 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "display_ili9340.h"

int ili9340_lcd_init(const struct device *dev)
{
	int r;
	uint8_t tx_data[15];

	tx_data[0] = 0x23;
	r = ili9340_transmit(dev, ILI9340_CMD_POWER_CTRL_1, tx_data, 1);
	if (r < 0) {
		return r;
	}

	tx_data[0] = 0x10;
	r = ili9340_transmit(dev, ILI9340_CMD_POWER_CTRL_2, tx_data, 1);
	if (r < 0) {
		return r;
	}

	tx_data[0] = 0x3e;
	tx_data[1] = 0x28;
	r = ili9340_transmit(dev, ILI9340_CMD_VCOM_CTRL_1, tx_data, 2);
	if (r < 0) {
		return r;
	}

	tx_data[0] = 0x86;
	r = ili9340_transmit(dev, ILI9340_CMD_VCOM_CTRL_2, tx_data, 1);
	if (r < 0) {
		return r;
	}

	tx_data[0] =
	    ILI9340_DATA_MEM_ACCESS_CTRL_MV | ILI9340_DATA_MEM_ACCESS_CTRL_BGR;
	r = ili9340_transmit(dev, ILI9340_CMD_MEM_ACCESS_CTRL, tx_data, 1);
	if (r < 0) {
		return r;
	}

	tx_data[0] = 0x00;
	tx_data[1] = 0x18;
	r = ili9340_transmit(dev, ILI9340_CMD_FRAME_CTRL_NORMAL_MODE, tx_data, 2);
	if (r < 0) {
		return r;
	}

	tx_data[0] = 0x08;
	tx_data[1] = 0x82;
	tx_data[2] = 0x27;
	r = ili9340_transmit(dev, ILI9340_CMD_DISPLAY_FUNCTION_CTRL, tx_data, 3);
	if (r < 0) {
		return r;
	}

	tx_data[0] = 0x01;
	r = ili9340_transmit(dev, ILI9340_CMD_GAMMA_SET, tx_data, 1);
	if (r < 0) {
		return r;
	}

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
	r = ili9340_transmit(dev, ILI9340_CMD_POSITIVE_GAMMA_CORRECTION, tx_data,
			     15);
	if (r < 0) {
		return r;
	}

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
	r = ili9340_transmit(dev, ILI9340_CMD_NEGATIVE_GAMMA_CORRECTION, tx_data,
			     15);
	if (r < 0) {
		return r;
	}

	return 0;
}
