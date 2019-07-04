/*
 * Copyright (c) 2019 Marc Reilly
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "display_st7789v.h"
#include <zephyr.h>
#include <stddef.h>

void st7789v_lcd_init(struct st7789v_data *p_st7789v)
{
	/* the LCD itself is smaller than the area of the ram on
	 * the controller. In its 'natural' orientation the LCD is
	 * 170 columns and 320 rows which is centered in the controller
	 * RAM area (so the LCD actually starts at column 35).
	 * We switch the rows/column addressing around so that the
	 * display is 320x170, and so the effective LCD is offset
	 * by 35 "rows".
	 */
	st7789v_set_lcd_margins(p_st7789v, 0, 35);

	u8_t cmd;
	u8_t data[14];

	cmd = ST7789V_CMD_PORCTRL;
	data[0] = 0x0c;
	data[1] = 0x0c;
	data[2] = 0x00;
	data[3] = 0x33;
	data[4] = 0x33;
	st7789v_transmit(p_st7789v, cmd, data, 5);

	cmd = ST7789V_CMD_CMD2EN;
	data[0] = 0x5a;
	data[1] = 0x69;
	data[2] = 0x02;
	data[3] = 0x01;
	st7789v_transmit(p_st7789v, cmd, data, 4);

	cmd = ST7789V_CMD_DGMEN;
	data[0] = 0x00;
	st7789v_transmit(p_st7789v, cmd, data, 1);

	cmd = ST7789V_CMD_GCTRL;
	data[0] = 0x35;
	st7789v_transmit(p_st7789v, cmd, data, 1);

	cmd = ST7789V_CMD_VCOMS;
	data[0] = 0x2b;
	st7789v_transmit(p_st7789v, cmd, data, 1);

#if CONFIG_ST7789V_REVERSE_LCD_RGB_ORDER
	cmd = ST7789V_CMD_LCMCTRL;
	data[0] = ST7789V_LCMCTRL_XBGR;
	st7789v_transmit(p_st7789v, cmd, data, 1);
#endif

	cmd = ST7789V_CMD_VDVVRHEN;
	data[0] = 0x01;
	st7789v_transmit(p_st7789v, cmd, data, 1);

	cmd = ST7789V_CMD_VRH;
	data[0] = 0x0f;
	st7789v_transmit(p_st7789v, cmd, data, 1);

	cmd = ST7789V_CMD_VDS;
	data[0] = 0x20;
	st7789v_transmit(p_st7789v, cmd, data, 1);

	cmd = ST7789V_CMD_FRCTRL2;
	data[0] = 0x0f;
	st7789v_transmit(p_st7789v, cmd, data, 1);

	cmd = ST7789V_CMD_PWCTRL1;
	data[0] = 0x52;
	data[1] = (0x2 << 6) | (0x2 << 4) | 0x01;
	st7789v_transmit(p_st7789v, cmd, data, 2);

	cmd = ST7789V_CMD_MADCTL;
	data[0] = ST7789V_MADCTL_MV_REVERSE_MODE;
#ifdef CONFIG_ST7789V_REVERSE_X
	data[0] |= ST7789V_MADCTL_MX_RIGHT_TO_LEFT;
#endif
#ifdef CONFIG_ST7789V_REVERSE_Y
	data[0] |= ST7789V_MADCTL_MY_BOTTOM_TO_TOP;
#endif
	st7789v_transmit(p_st7789v, cmd, data, 1);

	cmd = ST7789V_CMD_COLMOD;
#ifdef CONFIG_ST7789V_RGB565
	data[0] = ST7789V_COLMOD_RGB_65K
		| ST7789V_COLMOD_FMT_16bit;
#else
	data[0] = ST7789V_COLMOD_RGB_262K
		| ST7789V_COLMOD_FMT_18bit;
#endif
	st7789v_transmit(p_st7789v, cmd, data, 1);

	cmd = ST7789V_CMD_INV_ON;
	st7789v_transmit(p_st7789v, cmd, NULL, 0);

	cmd = ST7789V_CMD_GAMSET;
	data[0] = 0x01U;
	st7789v_transmit(p_st7789v, cmd, data, 1);

	cmd = ST7789V_CMD_PVGAMCTRL;
	data[0] = 0xD0 | 0x00;		/* v63 | V0 */
	data[1] = 0x00;			/* v1 */
	data[2] = 0x02;			/* v2 */
	data[3] = 0x07;			/* v4 */
	data[4] = 0x0B;			/* V6 */
	data[5] = (0x01 << 4) | 0x0a;	/* J0 | V13 */
	data[6] = 0x31;			/* V2 */
	data[7] = (0x5 << 4) | 0x4;	/* V36 | V27 */
	data[8] = 0x40;			/* V43 */
	data[9] = (0x02 << 4) | 0x09;	/* J1 | V50 */
	data[10] = 0x12;		/* V57 */
	data[11] = 0x12;		/* V59 */
	data[12] = 0x12;		/* V61 */
	data[13] = 0x17;		/* V62 */
	st7789v_transmit(p_st7789v, cmd, data, 14);

	cmd = ST7789V_CMD_NVGAMCTRL;
	data[0] = (0xd << 4) | 0x00;	/* V63 | V0 */
	data[1] = 0x00;			/* v1 */
	data[2] = 0x02;			/* v2 */
	data[3] = 0x07;			/* v4 */
	data[4] = 0x05;			/* V6 */
	data[5] = (0x01 << 4) | 0x05;	/* J0 | V13 */
	data[6] = 0x2D;			/* V20 */
	data[7] = (0x4 << 4) | 0x4;	/* V36 | V27 */
	data[8] = 0x44;			/* V43 */
	data[9] = (0x01 << 4) | 0x0c;	/* J1 | V50 */
	data[10] = 0x18;		/* V57 */
	data[11] = 0x16;		/* V59 */
	data[12] = 0x1c;		/* V61 */
	data[13] = 0x1d;		/* V62 */
	st7789v_transmit(p_st7789v, cmd, data, 14);

	cmd = ST7789V_CMD_RAMCTRL;
	data[0] = 0x00;
	data[1] = (0x3 << 6) | (0x3u << 4);
#if CONFIG_ST7789V_SWAP_PIXEL_LCD_ENDIANNESS
	data[1] |= 0x08;
#endif
	st7789v_transmit(p_st7789v, cmd, data, 2);

	cmd = ST7789V_CMD_RGBCTRL;
	data[0] = 0x80 | (0x2 << 5) | 0xd;
	data[1] = 0x08;
	data[2] = 0x14;
	st7789v_transmit(p_st7789v, cmd, data, 3);
}
