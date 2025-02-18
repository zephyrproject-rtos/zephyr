/*
 * Copyright (c) 2025 Arduino SA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_DISPLAY_DISPLAY_ST7701_H_
#define ZEPHYR_DRIVERS_DISPLAY_DISPLAY_ST7701_H_

/**
 * @name General parameters.
 * @{
 */

/* Command2 BKx selection command */
#define DSI_CMD2BKX_SEL      0xFF
#define DSI_CMD2BK1_SEL      0x11
#define DSI_CMD2BK0_SEL      0x10
#define DSI_CMD2BKX_SEL_NONE 0x00

/* Command2, BK0 commands */
#define DSI_CMD2_BK0_PVGAMCTRL 0xB0 /* Positive Voltage Gamma Control */
#define DSI_CMD2_BK0_NVGAMCTRL 0xB1 /* Negative Voltage Gamma Control */
#define DSI_CMD2_BK0_LNESET    0xC0 /* Display Line setting */
#define DSI_CMD2_BK0_PORCTRL   0xC1 /* Porch control */
#define DSI_CMD2_BK0_INVSEL    0xC2 /* Inversion selection, Frame Rate Control */

/* Command2, BK1 commands */
#define DSI_CMD2_BK1_VRHS     0xB0 /* Vop amplitude setting */
#define DSI_CMD2_BK1_VCOM     0xB1 /* VCOM amplitude setting */
#define DSI_CMD2_BK1_VGHSS    0xB2 /* VGH Voltage setting */
#define DSI_CMD2_BK1_TESTCMD  0xB3 /* TEST Command Setting */
#define DSI_CMD2_BK1_VGLS     0xB5 /* VGL Voltage setting */
#define DSI_CMD2_BK1_PWCTLR1  0xB7 /* Power Control 1 */
#define DSI_CMD2_BK1_PWCTLR2  0xB8 /* Power Control 2 */
#define DSI_CMD2_BK1_SPD1     0xC1 /* Source pre_drive timing set1 */
#define DSI_CMD2_BK1_SPD2     0xC2 /* Source EQ2 Setting */
#define DSI_CMD2_BK1_MIPISET1 0xD0 /* MIPI Setting 1 */

#define ST7701_CMD_ID1 0xDA
#define ST7701_ID      0xFF

/**
 * @name MIPI DCS Write Control Display fields.
 * @{
 */

/** Write Control Display: brightness control. */
#define ST7701_WRCTRLD_BCTRL BIT(5)
/** Write Control Display: display dimming. */
#define ST7701_WRCTRLD_DD    BIT(3)
/** Write Control Display: backlight. */
#define ST7701_WRCTRLD_BL    BIT(2)

/** Adaptive Brightness Control: off. */
#define ST7701_WRCABC_OFF 0x00U
/** Adaptibe Brightness Control: user interface. */
#define ST7701_WRCABC_UI  0x01U
/** Adaptibe Brightness Control: still picture. */
#define ST7701_WRCABC_ST  0x02U
/** Adaptibe Brightness Control: moving image. */
#define ST7701_WRCABC_MV  0x03U
/** @} */

#endif /* ZEPHYR_DRIVERS_DISPLAY_DISPLAY_ST7701_H_ */
