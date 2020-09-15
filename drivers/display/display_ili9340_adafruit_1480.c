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

	return 0;
}
