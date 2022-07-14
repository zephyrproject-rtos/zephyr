/*
 * Copyright (c) 2020 PHYTEC Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_DISPLAY_GD7965_REGS_H_
#define ZEPHYR_DRIVERS_DISPLAY_GD7965_REGS_H_

#define GD7965_CMD_PSR				0x00
#define GD7965_CMD_PWR				0x01
#define GD7965_CMD_POF				0x02
#define GD7965_CMD_PFS				0x03
#define GD7965_CMD_PON				0x04
#define GD7965_CMD_PMES				0x05
#define GD7965_CMD_BTST				0x06
#define GD7965_CMD_DSLP				0x07
#define GD7965_CMD_DTM1				0x10
#define GD7965_CMD_DSP				0x11
#define GD7965_CMD_DRF				0x12
#define GD7965_CMD_DTM2				0x13
#define GD7965_CMD_DUSPI			0x15
#define GD7965_CMD_AUTO				0x17
#define GD7965_CMD_LUTOPT			0x2A
#define GD7965_CMD_KWOPT			0x2B
#define GD7965_CMD_PLL				0x30
#define GD7965_CMD_TSC				0x40
#define GD7965_CMD_TSE				0x41
#define GD7965_CMD_TSW				0x42
#define GD7965_CMD_TSR				0x43
#define GD7965_CMD_PBC				0x44
#define GD7965_CMD_CDI				0x50
#define GD7965_CMD_LPD				0x51
#define GD7965_CMD_EVS				0x52
#define GD7965_CMD_TCON				0x60
#define GD7965_CMD_TRES				0x61
#define GD7965_CMD_GSST				0x65
#define GD7965_CMD_REV				0x70
#define GD7965_CMD_FLG				0x71
#define GD7965_CMD_AMV				0x80
#define GD7965_CMD_VV				0x81
#define GD7965_CMD_VDCS				0x82
#define GD7965_CMD_PTL				0x90
#define GD7965_CMD_PTIN				0x91
#define GD7965_CMD_PTOUT			0x92
#define GD7965_CMD_PGM				0xA0
#define GD7965_CMD_APG				0xA1
#define GD7965_CMD_ROTP				0xA2
#define GD7965_CMD_CCSET			0xE0
#define GD7965_CMD_PWS				0xE3
#define GD7965_CMD_LVSEL			0xE4
#define GD7965_CMD_TSSET			0xE5
#define GD7965_CMD_TSBDRY			0xE7

#define GD7965_PSR_REG				BIT(5)
#define GD7965_PSR_KW_R				BIT(4)
#define GD7965_PSR_UD				BIT(3)
#define GD7965_PSR_SHL				BIT(2)
#define GD7965_PSR_SHD				BIT(1)
#define GD7965_PSR_RST				BIT(0)

#define GD7965_AUTO_PON_DRF_POF			0xA5
#define GD7965_AUTO_PON_DRF_POF_DSLP		0xA7

#define GD7965_CDI_REG_LENGTH			2U
#define GD7965_CDI_BDZ_DDX_IDX			0
#define GD7965_CDI_CDI_IDX			1
#define GD7965_CDI_BDZ				BIT(7)
#define GD7965_CDI_BDV1				BIT(5)
#define GD7965_CDI_BDV0				BIT(4)
#define GD7965_CDI_N2OCP			BIT(3)
#define GD7965_CDI_DDX1				BIT(1)
#define GD7965_CDI_DDX0				BIT(0)

struct gd7965_tres {
	uint16_t hres;
	uint16_t vres;
} __packed;

BUILD_ASSERT(sizeof(struct gd7965_tres) == 4);

struct gd7965_ptl {
	uint16_t hrst;
	uint16_t hred;
	uint16_t vrst;
	uint16_t vred;
	uint8_t flags;
} __packed;

BUILD_ASSERT(sizeof(struct gd7965_ptl) == 9);

#define GD7965_PTL_FLAG_PT_SCAN			BIT(0)


/* Time constants in ms */
#define GD7965_RESET_DELAY			10U
#define GD7965_PON_DELAY			100U
#define GD7965_BUSY_DELAY			1U

#endif /* ZEPHYR_DRIVERS_DISPLAY_GD7965_REGS_H_ */
