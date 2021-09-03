/*
 * Copyright (c) 2021 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_DISPLAY_EK79652_REGS_H_
#define ZEPHYR_DRIVERS_DISPLAY_EK79652_REGS_H_

#define EK79652_CMD_PSR				0x00
#define EK79652_CMD_PWR				0x01
#define EK79652_CMD_POF				0x02
#define EK79652_CMD_PFS				0x03
#define EK79652_CMD_PON				0x04
#define EK79652_CMD_PMES			0x05
#define EK79652_CMD_BTST			0x06
#define EK79652_CMD_DSLP			0x07
#define EK79652_CMD_DTM1			0x10
#define EK79652_CMD_DSP				0x11
#define EK79652_CMD_DRF				0x12
#define EK79652_CMD_DTM2			0x13
#define EK79652_CMD_PDTM1			0x14
#define EK79652_CMD_PDTM2			0x15
#define EK79652_CMD_PDRF                        0x16
#define EK79652_CMD_LUT1			0x20
#define EK79652_CMD_LUTWW			0x21
#define EK79652_CMD_LUTBW			0x22
#define EK79652_CMD_LUTWB			0x23
#define EK79652_CMD_LUTBB			0x24
#define EK79652_CMD_PLL				0x30
#define EK79652_CMD_TSC				0x40
#define EK79652_CMD_TSE				0x41
#define EK79652_CMD_TSW				0x42
#define EK79652_CMD_TSR				0x43
#define EK79652_CMD_CDI				0x50
#define EK79652_CMD_LPD				0x51
#define EK79652_CMD_TCON			0x60
#define EK79652_CMD_TRES			0x61
#define EK79652_CMD_GSST			0x62
#define EK79652_CMD_REV				0x70
#define EK79652_CMD_FLG				0x71
#define EK79652_CMD_AMV				0x80
#define EK79652_CMD_VV				0x81
#define EK79652_CMD_VDCS			0x82
#define EK79652_CMD_PD				0x90
#define EK79652_CMD_PTIN			0x91
#define EK79652_CMD_PTOUT			0x92
#define EK79652_CMD_PGM				0xA0
#define EK79652_CMD_APG				0xA1
#define EK79652_CMD_ROTP			0xA2
#define EK79652_CMD_CCSET			0xE0
#define EK79652_CMD_TSSET			0xE5
#define EK79652_CMD_PWROPT                      0xF8

#define EK79652_PDRF_VAL                        0x00

#define EK79652_PSR_RES1                        BIT(7)
#define EK79652_PSR_RES0                        BIT(6)
#define EK79652_PSR_LUT_EN			BIT(5)
#define EK79652_PSR_BW                          BIT(4)
#define EK79652_PSR_UD				BIT(3)
#define EK79652_PSR_SHL				BIT(2)
#define EK79652_PSR_SHD				BIT(1)
#define EK79652_PSR_RST				BIT(0)

#define EK79652_PWROPT_LENGTH                   2U

#define EK79652_PDT_REG_LENGTH                  8U
#define EK79652_PDT_X_IDX			0
#define EK79652_PDT_Y_IDX			2
#define EK79652_PDT_W_IDX			4
#define EK79652_PDT_H_IDX			6

/* Time constants in ms */
#define EK79652_RESET_DELAY			10U
#define EK79652_BUSY_DELAY			2U

#endif /* ZEPHYR_DRIVERS_DISPLAY_EK79652_REGS_H_ */
