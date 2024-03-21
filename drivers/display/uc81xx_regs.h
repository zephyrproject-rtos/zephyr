/*
 * Copyright (c) 2020 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_DISPLAY_UC81XX_REGS_H_
#define ZEPHYR_DRIVERS_DISPLAY_UC81XX_REGS_H_

/* UC8176/UC8179 */
#define UC81XX_CMD_PSR				0x00
#define UC81XX_CMD_PWR				0x01
#define UC81XX_CMD_POF				0x02
#define UC81XX_CMD_PFS				0x03
#define UC81XX_CMD_PON				0x04
#define UC81XX_CMD_PMES				0x05
#define UC81XX_CMD_BTST				0x06
#define UC81XX_CMD_DSLP				0x07
#define UC81XX_CMD_DTM1				0x10
#define UC81XX_CMD_DSP				0x11
#define UC81XX_CMD_DRF				0x12

/* UC8179 only */
#define UC81XX_CMD_DTM2				0x13
#define UC81XX_CMD_DUSPI			0x15
#define UC81XX_CMD_AUTO				0x17
#define UC81XX_CMD_LUTOPT			0x2A
#define UC81XX_CMD_KWOPT			0x2B

#define UC81XX_CMD_LUTC				0x20
#define UC81XX_CMD_LUTWW			0x21
#define UC81XX_CMD_LUTKW			0x22
#define UC81XX_CMD_LUTWK			0x23
#define UC81XX_CMD_LUTKK			0x24
#define UC81XX_CMD_LUTBD			0x25

/* UC8176/UC8179 */
#define UC81XX_CMD_PLL				0x30
#define UC81XX_CMD_TSC				0x40
#define UC81XX_CMD_TSE				0x41
#define UC81XX_CMD_TSW				0x42
#define UC81XX_CMD_TSR				0x43

/* UC8179 */
#define UC81XX_CMD_PBC				0x44

/* UC8176/UC8179 - different register layouts */
#define UC81XX_CMD_CDI				0x50
/* UC8176/UC8179 */
#define UC81XX_CMD_LPD				0x51

/* UC8179 */
#define UC81XX_CMD_EVS				0x52

/* UC8176/UC8179 */
#define UC81XX_CMD_TCON				0x60
#define UC81XX_CMD_TRES				0x61
#define UC81XX_CMD_GSST				0x65
#define UC81XX_CMD_REV				0x70
#define UC81XX_CMD_FLG				0x71
#define UC81XX_CMD_AMV				0x80
#define UC81XX_CMD_VV				0x81
#define UC81XX_CMD_VDCS				0x82
#define UC81XX_CMD_PTL				0x90
#define UC81XX_CMD_PTIN				0x91
#define UC81XX_CMD_PTOUT			0x92
#define UC81XX_CMD_PGM				0xA0
#define UC81XX_CMD_APG				0xA1
#define UC81XX_CMD_ROTP				0xA2
#define UC81XX_CMD_CCSET			0xE0
#define UC81XX_CMD_PWS				0xE3

/* UC8179 */
#define UC81XX_CMD_LVSEL			0xE4

/* UC8176/UC8179 */
#define UC81XX_CMD_TSSET			0xE5

/* UC8179 */
#define UC81XX_CMD_TSBDRY			0xE7

#define UC81XX_PSR_REG				BIT(5)
#define UC81XX_PSR_KW_R				BIT(4)
#define UC81XX_PSR_UD				BIT(3)
#define UC81XX_PSR_SHL				BIT(2)
#define UC81XX_PSR_SHD				BIT(1)
#define UC81XX_PSR_RST				BIT(0)

#define UC81XX_AUTO_PON_DRF_POF			0xA5
#define UC81XX_AUTO_PON_DRF_POF_DSLP		0xA7

#define UC8176_CDI_VBD_MASK			0xc0
#define UC8176_CDI_VBD0				BIT(6)
#define UC8176_CDI_VBD1				BIT(7)
#define UC8176_CDI_DDX1				BIT(5)
#define UC8176_CDI_DDX0				BIT(4)
#define UC8176_CDI_CDI_MASK			0x0f

#define UC8179_CDI_REG_LENGTH			2U
#define UC8179_CDI_BDZ_DDX_IDX			0
#define UC8179_CDI_CDI_IDX			1
#define UC8179_CDI_BDZ				BIT(7)
#define UC8179_CDI_BDV1				BIT(5)
#define UC8179_CDI_BDV0				BIT(4)
#define UC8179_CDI_N2OCP			BIT(3)
#define UC8179_CDI_DDX1				BIT(1)
#define UC8179_CDI_DDX0				BIT(0)

struct uc81xx_tres8 {
	uint8_t hres;
	uint8_t vres;
} __packed;

BUILD_ASSERT(sizeof(struct uc81xx_tres8) == 2);

struct uc81xx_ptl8 {
	uint8_t hrst;
	uint8_t hred;
	uint8_t vrst;
	uint8_t vred;
	uint8_t flags;
} __packed;

BUILD_ASSERT(sizeof(struct uc81xx_ptl8) == 5);

struct uc81xx_tres16 {
	uint16_t hres;
	uint16_t vres;
} __packed;

BUILD_ASSERT(sizeof(struct uc81xx_tres16) == 4);

struct uc81xx_ptl16 {
	uint16_t hrst;
	uint16_t hred;
	uint16_t vrst;
	uint16_t vred;
	uint8_t flags;
} __packed;

BUILD_ASSERT(sizeof(struct uc81xx_ptl16) == 9);

#define UC81XX_PTL_FLAG_PT_SCAN			BIT(0)

/* Time constants in ms */
#define UC81XX_RESET_DELAY			10U
#define UC81XX_PON_DELAY			100U
#define UC81XX_BUSY_DELAY			1U

#endif /* ZEPHYR_DRIVERS_DISPLAY_UC81XX_REGS_H_ */
