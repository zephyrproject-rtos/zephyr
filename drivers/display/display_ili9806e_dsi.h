/*
 * SPDX-FileCopyrightText: 2024 - 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_DISPLAY_ILITEK_ILI9806E_H_
#define ZEPHYR_DRIVERS_DISPLAY_ILITEK_ILI9806E_H_

#include <zephyr/device.h>

/*
 * ILI9806E specific register definitions. Names follow the command mnemonics
 * in the ILI9806E datasheet. Registers shared with other ILI98xx controllers
 * are defined in display_ili98xx_dsi.h.
 */

/* Page 1 commands */
#define ILI9806E_IFMODE_1   0x08 /* Interface Mode Control 1 */
#define ILI9806E_DISCTRL1   0x20 /* Display Function Control 1 */
#define ILI9806E_DISCTRL2   0x21 /* Display Function Control 2 */
#define ILI9806E_RESCTRL    0x30 /* Resolution Control */
#define ILI9806E_PWCTRL_1   0x40 /* Power Control 1 */
#define ILI9806E_PWCTRL_2   0x41 /* Power Control 2 */
#define ILI9806E_PWCTRL_3   0x42 /* Power Control 3 */
#define ILI9806E_PWCTRL_4   0x43 /* Power Control 4 */
#define ILI9806E_PWCTRL_5   0x44 /* Power Control 5 */

/* Page 5 commands */
#define ILI9806E_BLCTRL8 0x07 /* Backlight Control 8 */
#define ILI9806E_BLCTRL9 0x09 /* Backlight Control 9 */

/* Page 6: GIPCTRL1 (GIP Control 1), 00h~1Dh */
#define ILI9806E_GIPCTRL1_00 0x00
#define ILI9806E_GIPCTRL1_01 0x01
#define ILI9806E_GIPCTRL1_02 0x02
#define ILI9806E_GIPCTRL1_03 0x03
#define ILI9806E_GIPCTRL1_04 0x04
#define ILI9806E_GIPCTRL1_05 0x05
#define ILI9806E_GIPCTRL1_06 0x06
#define ILI9806E_GIPCTRL1_07 0x07
#define ILI9806E_GIPCTRL1_08 0x08
#define ILI9806E_GIPCTRL1_09 0x09
#define ILI9806E_GIPCTRL1_0A 0x0A
#define ILI9806E_GIPCTRL1_0B 0x0B
#define ILI9806E_GIPCTRL1_0C 0x0C
#define ILI9806E_GIPCTRL1_0D 0x0D
#define ILI9806E_GIPCTRL1_0E 0x0E
#define ILI9806E_GIPCTRL1_0F 0x0F
#define ILI9806E_GIPCTRL1_10 0x10
#define ILI9806E_GIPCTRL1_11 0x11
#define ILI9806E_GIPCTRL1_12 0x12
#define ILI9806E_GIPCTRL1_13 0x13
#define ILI9806E_GIPCTRL1_14 0x14
#define ILI9806E_GIPCTRL1_15 0x15
#define ILI9806E_GIPCTRL1_16 0x16
#define ILI9806E_GIPCTRL1_17 0x17
#define ILI9806E_GIPCTRL1_18 0x18
#define ILI9806E_GIPCTRL1_19 0x19
#define ILI9806E_GIPCTRL1_1A 0x1A
#define ILI9806E_GIPCTRL1_1B 0x1B
#define ILI9806E_GIPCTRL1_1C 0x1C
#define ILI9806E_GIPCTRL1_1D 0x1D

/* Page 6: GIPCTRL2 (GIP Control 2), 20h~27h */
#define ILI9806E_GIPCTRL2_20 0x20
#define ILI9806E_GIPCTRL2_21 0x21
#define ILI9806E_GIPCTRL2_22 0x22
#define ILI9806E_GIPCTRL2_23 0x23
#define ILI9806E_GIPCTRL2_24 0x24
#define ILI9806E_GIPCTRL2_25 0x25
#define ILI9806E_GIPCTRL2_26 0x26
#define ILI9806E_GIPCTRL2_27 0x27

/* Page 6: GIPCTRL3 (GIP Control 3), 30h~40h */
#define ILI9806E_GIPCTRL3_30 0x30
#define ILI9806E_GIPCTRL3_31 0x31
#define ILI9806E_GIPCTRL3_32 0x32
#define ILI9806E_GIPCTRL3_33 0x33
#define ILI9806E_GIPCTRL3_34 0x34
#define ILI9806E_GIPCTRL3_35 0x35
#define ILI9806E_GIPCTRL3_36 0x36
#define ILI9806E_GIPCTRL3_37 0x37
#define ILI9806E_GIPCTRL3_38 0x38
#define ILI9806E_GIPCTRL3_39 0x39
#define ILI9806E_GIPCTRL3_3A 0x3A
#define ILI9806E_GIPCTRL3_3B 0x3B
#define ILI9806E_GIPCTRL3_3C 0x3C
#define ILI9806E_GIPCTRL3_3D 0x3D
#define ILI9806E_GIPCTRL3_3E 0x3E
#define ILI9806E_GIPCTRL3_3F 0x3F
#define ILI9806E_GIPCTRL3_40 0x40

/* Page 6 commands */
#define ILI9806E_GVLOCTRL 0x53 /* GOUT_VGLO Control */
#define ILI9806E_GVHOCTRL 0x54 /* GOUT_VGHO Control */

/* Page 7 commands */
#define ILI9806E_VGLREGEN 0x17 /* VGL_REG EN */
#define ILI9806E_VREG12EN 0x18 /* VREG1/2OUT ENABLE */
/* Page 7 registers used by the vendor init sequence but not documented in the datasheet */
#define ILI9806E_P7_REG_02 0x02
#define ILI9806E_P7_REG_26 0x26
#define ILI9806E_P7_REG_E1 0xE1

#endif /* ZEPHYR_DRIVERS_DISPLAY_ILITEK_ILI9806E_H_ */
