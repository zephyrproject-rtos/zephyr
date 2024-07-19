/*
 * Copyright (c) 2023 bytes at work AG
 * Copyright (c) 2020 Teslabs Engineering S.L.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_DISPLAY_DISPLAY_OTM8009A_H_
#define ZEPHYR_DRIVERS_DISPLAY_DISPLAY_OTM8009A_H_

/**
 * @name General parameters.
 * @{
 */

/** ID1 */
#define OTM8009A_ID1 0x40U
/** Read ID1 command */
#define OTM8009A_CMD_ID1 0xDA

/** Reset pulse time (ms), ref. Table 6.3.4.1 */
#define OTM8009A_RESET_TIME 10U
/** Wake up time after reset pulse, ref. Table 6.3.4.1 */
#define OTM8009A_WAKE_TIME 20U
/** Time to wait after exiting sleep mode (ms), ref. 5.2.11. */
#define OTM8009A_EXIT_SLEEP_MODE_WAIT_TIME 5U

/** @} */

/**
 * @name Display timings (ref. table 6.4.2.1)
 * @{
 */

/** Horizontal low pulse width */
#define OTM8009A_HSYNC 2U
/** Horizontal front porch. */
#define OTM8009A_HFP 34U
/** Horizontal back porch. */
#define OTM8009A_HBP 34U
/** Vertical low pulse width. */
#define OTM8009A_VSYNC 1U
/** Vertical front porch. */
#define OTM8009A_VFP 16U
/** Vertical back porch. */
#define OTM8009A_VBP 15U

/** @} */

/**
 * @name Register fields.
 * @{
 */

/**
 * @name MIPI DCS Write Control Display fields.
 * @{
 */

/** Write Control Display: brightness control. */
#define OTM8009A_WRCTRLD_BCTRL	BIT(5)
/** Write Control Display: display dimming. */
#define OTM8009A_WRCTRLD_DD	BIT(3)
/** Write Control Display: backlight. */
#define OTM8009A_WRCTRLD_BL	BIT(2)

/** Adaptibe Brightness Control: off. */
#define OTM8009A_WRCABC_OFF	0x00U
/** Adaptibe Brightness Control: user interface. */
#define OTM8009A_WRCABC_UI	0x01U
/** Adaptibe Brightness Control: still picture. */
#define OTM8009A_WRCABC_ST	0x02U
/** Adaptibe Brightness Control: moving image. */
#define OTM8009A_WRCABC_MV	0x03U

/** @} */

/**
 * @name MIPI MCS (Manufacturer Command Set).
 * @{
 */

/** Address Shift Function */
#define OTM8009A_MCS_ADRSFT		0x0000U
/** Panel Type Setting */
#define OTM8009A_MCS_PANSET		0xB3A6U
/* Source Driver Timing Setting */
#define OTM8009A_MCS_SD_CTRL		0xC0A2U
/** Panel Driving Mode */
#define OTM8009A_MCS_P_DRV_M		0xC0B4U
/** Oscillator Adjustment for Idle/Normal mode */
#define OTM8009A_MCS_OSC_ADJ		0xC181U
/** RGB Video Mode Setting */
#define OTM8009A_MCS_RGB_VID_SET	0xC1A1U
/** Source Driver Precharge Control */
#define OTM8009A_MCS_SD_PCH_CTRL	0xC480U
/** Command not documented */
#define OTM8009A_MCS_NO_DOC1		0xC48AU
/** Power Control Setting 1 */
#define OTM8009A_MCS_PWR_CTRL1		0xC580U
/** Power Control Setting 2 for Normal Mode */
#define OTM8009A_MCS_PWR_CTRL2		0xC590U
/** Power Control Setting 4 for DC Voltage */
#define OTM8009A_MCS_PWR_CTRL4		0xC5B0U
/** PWM Parameter 1 */
#define OTM8009A_MCS_PWM_PARA1		0xC680U
/** PWM Parameter 2 */
#define OTM8009A_MCS_PWM_PARA2		0xC6B0U
/** PWM Parameter 3 */
#define OTM8009A_MCS_PWM_PARA3		0xC6B1U
/** PWM Parameter 4 */
#define OTM8009A_MCS_PWM_PARA4		0xC6B3U
/** PWM Parameter 5 */
#define OTM8009A_MCS_PWM_PARA5		0xC6B4U
/** PWM Parameter 6 */
#define OTM8009A_MCS_PWM_PARA6		0xC6B5U
/** Panel Control Setting 1 */
#define OTM8009A_MCS_PANCTRLSET1	0xCB80U
/** Panel Control Setting 2 */
#define OTM8009A_MCS_PANCTRLSET2	0xCB90U
/** Panel Control Setting 3 */
#define OTM8009A_MCS_PANCTRLSET3	0xCBA0U
/** Panel Control Setting 4 */
#define OTM8009A_MCS_PANCTRLSET4	0xCBB0U
/** Panel Control Setting 5 */
#define OTM8009A_MCS_PANCTRLSET5	0xCBC0U
/** Panel Control Setting 6 */
#define OTM8009A_MCS_PANCTRLSET6	0xCBD0U
/** Panel Control Setting 7 */
#define OTM8009A_MCS_PANCTRLSET7	0xCBE0U
/** Panel Control Setting 8 */
#define OTM8009A_MCS_PANCTRLSET8	0xCBF0U
/** Panel U2D Setting 1 */
#define OTM8009A_MCS_PANU2D1		0xCC80U
/** Panel U2D Setting 2 */
#define OTM8009A_MCS_PANU2D2		0xCC90U
/** Panel U2D Setting 3 */
#define OTM8009A_MCS_PANU2D3		0xCCA0U
/** Panel D2U Setting 1 */
#define OTM8009A_MCS_PAND2U1		0xCCB0U
/** Panel D2U Setting 2 */
#define OTM8009A_MCS_PAND2U2		0xCCC0U
/** Panel D2U Setting 3 */
#define OTM8009A_MCS_PAND2U3		0xCCD0U
/** GOA VST Setting */
#define OTM8009A_MCS_GOAVST		0xCE80U
/** GOA CLKA1 Setting */
#define OTM8009A_MCS_GOACLKA1		0xCEA0U
/** GOA CLKA2 Setting */
#define OTM8009A_MCS_GOACLKA2		0xCEA7U
/** GOA CLKA3 Setting */
#define OTM8009A_MCS_GOACLKA3		0xCEB0U
/** GOA CLKA4 Setting */
#define OTM8009A_MCS_GOACLKA4		0xCEB7U
/** GOA ECLK Setting */
#define OTM8009A_MCS_GOAECLK		0xCFC0U
/** GOA Other Options 1 */
#define OTM8009A_MCS_GOAPT1		0xCFC6U
/** GOA Signal Toggle Option Setting */
#define OTM8009A_MCS_GOATGOPT		0xCFC7U
/** Command not documented */
#define OTM8009A_MCS_NO_DOC2		0xCFD0U
/** GVDD/NGVDD */
#define OTM8009A_MCS_GVDDSET		0xD800U
/** VCOM Voltage Setting */
#define OTM8009A_MCS_VCOMDC		0xD900U
/** Gamma Correction 2.2+ Setting */
#define OTM8009A_MCS_GMCT2_2P		0xE100U
/** Gamma Correction 2.2- Setting */
#define OTM8009A_MCS_GMCT2_2N		0xE200U
/** Command not documented */
#define OTM8009A_MCS_NO_DOC3		0xF5B6U
/** Enable Access Command2 "CMD2" */
#define OTM8009A_MCS_CMD2_ENA1		0xFF00U
/** Enable Access Orise Command2 */
#define OTM8009A_MCS_CMD2_ENA2		0xFF80U

/** @} */

#endif /* ZEPHYR_DRIVERS_DISPLAY_DISPLAY_OTM8009A_H_ */
