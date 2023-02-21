/*
 * Copyright (C) 2023 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/cache.h>
#include "socfpga_system_manager.h"
#include <zephyr/drivers/sdhc.h>

/* HRS09 */
#define SDHC_PHY_SW_RESET		BIT(0)
#define SDHC_PHY_INIT_COMPLETE		BIT(1)
#define SDHC_EXTENDED_RD_MODE(x)	((x) << 2)
#define EXTENDED_WR_MODE		3
#define SDHC_EXTENDED_WR_MODE(x)	((x) << 3)
#define RDCMD_EN			15
#define SDHC_RDCMD_EN(x)		((x) << 15)
#define SDHC_RDDATA_EN(x)		((x) << 16)

/* CMD_DATA_OUTPUT */
#define SDHC_CDNS_HRS16			0x40

/* SRS09 */
#define CI				BIT(16)
#define STATUS_DATA_BUSY		BIT(2)

/* SRS10 */
#define DTW				1
#define EDTW				5
#define BP				8
#define BVS				9
#define BIT1				0 << 1
#define BIT4				1 << 1
#define BIT8				1 << 5

/* SRS11 */
#define ICE				BIT(0)
#define ICS				BIT(1)
#define SDCE				BIT(2)
#define USDCLKFS			6
#define SDCLKFS				8
#define DTCV				16
#define SRFA				BIT(24)
#define SRCMD				BIT(25)
#define SRDAT				BIT(26)

/* This value determines the interval by which DAT line timeouts are detected */
/* The interval can be computed as below: */
/* • 1111b - Reserved */
/* • 1110b - t_sdmclk*2(27+2) */
/* • 1101b - t_sdmclk*2(26+2) */
#define READ_CLK			0xa << 16
#define WRITE_CLK			0xe << 16
#define DTC_VAL				0xE

/* SRS12 */
#define CC				0
#define TC				1
#define EINT				15

/* SRS01 */
#define BLOCK_SIZE			0
#define SDMA_BUF			7 << 12
#define BLK_COUNT_CT			16

/* SRS14 */
#define CC_IE				0
#define TC_IE				1
#define DMAINT_IE			3

/* SRS15 Registers */
#define SDR12_MODE			0 << 16
#define SDR25_MODE			1 << 16
#define SDR50_MODE			2 << 16
#define SDR104_MODE			3 << 16
#define DDR50_MODE			4 << 16
/* V18SE is 0 for DS and HS, 1 for UHS-I */
#define V18SE				BIT(19)
#define CMD23_EN			BIT(27)
/* HC4E is 0 means version 3.0 and 1 means v 4.0 */
#define HV4E				BIT(28)
#define BIT_AD_32			0 << 29
#define BIT_AD_64			1 << 29
#define PVE				BIT(31)

/* Combo PHY */
#define PHY_DQ_TIMING_REG		0x0
#define PHY_DQS_TIMING_REG		0x04
#define PHY_GATE_LPBK_CTRL_REG		0x08
#define PHY_DLL_MASTER_CTRL_REG		0x0C
#define PHY_DLL_SLAVE_CTRL_REG		0x10
#define PHY_CTRL_REG			0x80

#define PHY_REG_ADDR_MASK		0xFFFF
#define PHY_REG_DATA_MASK		0xFFFFFFFF
#define PERIPHERAL_SDMMC_MASK		0x60
#define PERIPHERAL_SDMMC_OFFSET		6
#define DFI_INTF_MASK			0x1

/* PHY_DQS_TIMING_REG */
#define CP_USE_EXT_LPBK_DQS(x)		(x << 22)
#define CP_USE_LPBK_DQS(x)		(x << 21)
#define CP_USE_PHONY_DQS(x)		(x << 20)
#define CP_USE_PHONY_DQS_CMD(x)		(x << 19)

/* PHY_GATE_LPBK_CTRL_REG */
#define CP_SYNC_METHOD(x)		((x) << 31)
#define CP_SW_HALF_CYCLE_SHIFT(x)	((x) << 28)
#define CP_RD_DEL_SEL(x)		((x) << 19)
#define CP_UNDERRUN_SUPPRESS(x)		((x) << 18)
#define CP_GATE_CFG_ALWAYS_ON(x)	((x) << 6)

/* PHY_DLL_MASTER_CTRL_REG */
#define CP_DLL_BYPASS_MODE(x)		((x) << 23)
#define CP_DLL_START_POINT(x)		((x) << 0)

/* PHY_DLL_SLAVE_CTRL_REG */
#define CP_READ_DQS_CMD_DELAY(x)	((x) << 24)
#define CP_CLK_WRDQS_DELAY(x)		((x) << 16)
#define CP_CLK_WR_DELAY(x)		((x) << 8)
#define CP_READ_DQS_DELAY(x)		((x) << 0)

/* PHY_DQ_TIMING_REG */
#define CP_IO_MASK_ALWAYS_ON(x)		((x) << 31)
#define CP_IO_MASK_END(x)		((x) << 27)
#define CP_IO_MASK_START(x)		((x) << 24)
#define CP_DATA_SELECT_OE_END(x)	((x) << 0)

/* SW RESET REG*/
#define SDHC_CDNS_HRS00			(0x00)
#define SDHC_CDNS_HRS00_SWR		BIT(0)

/* PHY access port */
#define SDHC_CDNS_HRS04			0x10
#define SDHC_CDNS_HRS04_ADDR		GENMASK(5, 0)

/* PHY data access port */
#define SDHC_CDNS_HRS05			0x14

/* eMMC control registers */
#define SDHC_CDNS_HRS06			0x18

/* PHY_CTRL_REG */
#define CP_PHONY_DQS_TIMING_MASK	0x3F
#define CP_PHONY_DQS_TIMING_SHIFT	4

/* SRS */
#define SDHC_CDNS_SRS_BASE		0x200
#define SDHC_CDNS_SRS00			0x200
#define SDHC_CDNS_SRS01			0x204
#define SDHC_CDNS_SRS02			0x208
#define SDHC_CDNS_SRS03			0x20c
#define SDHC_CDNS_SRS04			0x210
#define SDHC_CDNS_SRS05			0x214
#define SDHC_CDNS_SRS06			0x218
#define SDHC_CDNS_SRS07			0x21C
#define SDHC_CDNS_SRS08			0x220
#define SDHC_CDNS_SRS09			0x224
#define SDHC_CDNS_SRS09_CI		BIT(16)
#define SDHC_CDNS_SRS10			0x228
#define SDHC_CDNS_SRS11			0x22C
#define SDHC_CDNS_SRS12			0x230
#define SDHC_CDNS_SRS13			0x234
#define SDHC_CDNS_SRS14			0x238
#define SDHC_CDNS_SRS15			0x23c
#define SDHC_CDNS_SRS21			0x254
#define SDHC_CDNS_SRS22			0x258
#define SDHC_CDNS_SRS23			0x25c

/* SRS00 */
#define SAAR				1

/* SRS03 */
#define CMD_START			(U(1) << 31)
#define CMD_USE_HOLD_REG		(1 << 29)
#define CMD_UPDATE_CLK_ONLY		(1 << 21)
#define CMD_SEND_INIT			(1 << 15)
#define CMD_STOP_ABORT_CMD		(4 << 22)
#define CMD_RESUME_CMD			(2 << 22)
#define CMD_SUSPEND_CMD			(1 << 22)
#define DATA_PRESENT			(1 << 21)
#define CMD_IDX_CHK_ENABLE		(1 << 20)
#define CMD_WRITE			(0 << 4)
#define CMD_READ			(1 << 4)
#define MULTI_BLK_READ			(1 << 5)
#define RESP_ERR			(1 << 7)
#define CMD_CHECK_RESP_CRC		(1 << 19)
#define RES_TYPE_SEL_48			(2 << 16)
#define RES_TYPE_SEL_136		(1 << 16)
#define RES_TYPE_SEL_48_B		(3 << 16)
#define RES_TYPE_SEL_NO			(0 << 16)
#define DMA_ENABLED			(1 << 0)
#define BLK_CNT_EN			(1 << 1)
#define AUTO_CMD_EN			(2 << 2)
#define COM_IDX				24
#define ERROR_INT			BIT(15)
#define INT_SBE				(1 << 13)
#define INT_HLE				(1 << 12)
#define INT_FRUN			(1 << 11)
#define INT_DRT				(1 << 9)
#define INT_RTO				(1 << 8)
#define INT_DCRC			(1 << 7)
#define INT_RCRC			(1 << 6)
#define INT_RXDR			(1 << 5)
#define INT_TXDR			(1 << 4)
#define INT_DTO				(1 << 3)
#define INT_CMD_DONE			(1 << 0)
#define TRAN_COMP			(1 << 1)

/* HRS07 */
#define SDHC_CDNS_HRS07			0x1c
#define SDHC_IDELAY_VAL(x)		((x) << 0)
#define SDHC_RW_COMPENSATE(x)		((x) << 16)

/* PHY reset port */
#define SDHC_CDNS_HRS09			0x24

/* HRS10 */
/* PHY reset port */
#define SDHC_CDNS_HRS10			0x28

/* HCSDCLKADJ DATA; DDR Mode */
#define SDHC_HCSDCLKADJ(x)		((x) << 16)

/* Pinmux headers will reomove after ATF driver implementation */
#define PINMUX_SDMMC_SEL		0x0
#define PIN0SEL				0x00
#define PIN1SEL				0x04
#define PIN2SEL				0x08
#define PIN3SEL				0x0C
#define PIN4SEL				0x10
#define PIN5SEL				0x14
#define PIN6SEL				0x18
#define PIN7SEL				0x1C
#define PIN8SEL				0x20
#define PIN9SEL				0x24
#define PIN10SEL			0x28

/* HRS16 */
#define SDHC_WRCMD0_DLY(x)		((x) << 0)
#define SDHC_WRCMD1_DLY(x)		((x) << 4)
#define SDHC_WRDATA0_DLY(x)		((x) << 8)
#define SDHC_WRDATA1_DLY(x)		((x) << 12)
#define SDHC_WRCMD0_SDCLK_DLY(x)	((x) << 16)
#define SDHC_WRCMD1_SDCLK_DLY(x)	((x) << 20)
#define SDHC_WRDATA0_SDCLK_DLY(x)	((x) << 24)
#define SDHC_WRDATA1_SDCLK_DLY(x)	((x) << 28)

/* Shared Macros */
#define SDMMC_CDN(_reg)			(SDMMC_CDN_REG_BASE + \
						(SDMMC_CDN_##_reg))

/* Refer to atf/include/export/lib/utils_def_exp.h*/
#define U(_x)				(_x##U)

/* Refer to atf/tools/cert_create/include/debug.h */
#define BIT_32(nr)			(U(1) << (nr))

/* MMC Peripheral Definition */
#define MMC_BLOCK_SIZE			U(512)
#define MMC_BLOCK_MASK			(MMC_BLOCK_SIZE - U(1))
#define MMC_BOOT_CLK_RATE		(400 * 1000)

#define MMC_CMD(_x)			U(_x)

#define MMC_ACMD(_x)			U(_x)

#define OCR_POWERUP			BIT(31)
#define OCR_HCS				BIT(30)
#define OCR_BYTE_MODE			(U(0) << 29)
#define OCR_SECTOR_MODE			(U(2) << 29)
#define OCR_ACCESS_MODE_MASK		(U(3) << 29)
#define OCR_3_5_3_6			BIT(23)
#define OCR_3_4_3_5			BIT(22)
#define OCR_3_3_3_4			BIT(21)
#define OCR_3_2_3_3			BIT(20)
#define OCR_3_1_3_2			BIT(19)
#define OCR_3_0_3_1			BIT(18)
#define OCR_2_9_3_0			BIT(17)
#define OCR_2_8_2_9			BIT(16)
#define OCR_2_7_2_8			BIT(15)
#define OCR_VDD_MIN_2V7			GENMASK(23, 15)
#define OCR_VDD_MIN_2V0			GENMASK(14, 8)
#define OCR_VDD_MIN_1V7			BIT(7)

#define MMC_RSP_48			BIT(0)
#define MMC_RSP_136			BIT(1)	/* 136 bit response */
#define MMC_RSP_CRC			BIT(2)	/* expect valid crc */
#define MMC_RSP_CMD_IDX			BIT(3)	/* response contains cmd idx */
#define MMC_RSP_BUSY			BIT(4)	/* device may be busy */

/* JEDEC 4.51 chapter 6.12 */
#define MMC_RESPONSE_R1			(MMC_RSP_48 | MMC_RSP_CMD_IDX | MMC_RSP_CRC)
#define MMC_RESPONSE_R1B		(MMC_RESPONSE_R1 | MMC_RSP_BUSY)
#define MMC_RESPONSE_R2			(MMC_RSP_48 | MMC_RSP_136 | MMC_RSP_CRC)
#define MMC_RESPONSE_R3			(MMC_RSP_48)
#define MMC_RESPONSE_R4			(MMC_RSP_48)
#define MMC_RESPONSE_R5			(MMC_RSP_48 | MMC_RSP_CRC | MMC_RSP_CMD_IDX)
#define MMC_RESPONSE_R6			(MMC_RSP_CRC | MMC_RSP_CMD_IDX)
#define MMC_RESPONSE_R7			(MMC_RSP_48 | MMC_RSP_CRC)
#define MMC_RESPONSE_NONE		0

/* Value randomly chosen for eMMC RCA, it should be > 1 */
#define MMC_FIX_RCA			6
#define RCA_SHIFT_OFFSET		16

#define CMD_EXTCSD_PARTITION_CONFIG	179
#define CMD_EXTCSD_BUS_WIDTH		183
#define CMD_EXTCSD_HS_TIMING		185
#define CMD_EXTCSD_SEC_CNT		212

#define PART_CFG_BOOT_PARTITION1_ENABLE	(U(1) << 3)
#define PART_CFG_PARTITION1_ACCESS	(U(1) << 0)

/* Values in EXT CSD register */
#define MMC_BUS_WIDTH_1			U(0)
#define MMC_BUS_WIDTH_4			U(1)
#define MMC_BUS_WIDTH_8			U(2)
#define MMC_BUS_WIDTH_DDR_4		U(5)
#define MMC_BUS_WIDTH_DDR_8		U(6)
#define MMC_BOOT_MODE_BACKWARD		(U(0) << 3)
#define MMC_BOOT_MODE_HS_TIMING		(U(1) << 3)
#define MMC_BOOT_MODE_DDR		(U(2) << 3)

#define EXTCSD_SET_CMD			(U(0) << 24)
#define EXTCSD_SET_BITS			(U(1) << 24)
#define EXTCSD_CLR_BITS			(U(2) << 24)
#define EXTCSD_WRITE_BYTES		(U(3) << 24)
#define EXTCSD_CMD(x)			(((x) & 0xff) << 16)
#define EXTCSD_VALUE(x)			(((x) & 0xff) << 8)
#define EXTCSD_CMD_SET_NORMAL		U(1)

#define CSD_TRAN_SPEED_UNIT_MASK	GENMASK(2, 0)
#define CSD_TRAN_SPEED_MULT_MASK	GENMASK(6, 3)
#define CSD_TRAN_SPEED_MULT_SHIFT	3

#define STATUS_CURRENT_STATE(x)		(((x) & 0xf) << 9)
#define STATUS_READY_FOR_DATA		BIT(8)
#define STATUS_SWITCH_ERROR		BIT(7)
#define MMC_GET_STATE(x)		(((x) >> 9) & 0xf)
#define MMC_STATE_IDLE			0
#define MMC_STATE_READY			1
#define MMC_STATE_IDENT			2
#define MMC_STATE_STBY			3
#define MMC_STATE_TRAN			4
#define MMC_STATE_DATA			5
#define MMC_STATE_RCV			6
#define MMC_STATE_PRG			7
#define MMC_STATE_DIS			8
#define MMC_STATE_BTST			9
#define MMC_STATE_SLP			10

#define MMC_FLAG_CMD23			(U(1) << 0)

#define CMD8_CHECK_PATTERN		U(0xAA)
#define VHS_2_7_3_6_V			BIT(8)

#define SD_SCR_BUS_WIDTH_1		BIT(8)
#define SD_SCR_BUS_WIDTH_4		BIT(10)

/*ADMA table component*/
#define ADMA_DESC_ATTR_VALID		BIT(0)
#define ADMA_DESC_ATTR_END		BIT(1)
#define ADMA_DESC_ATTR_INT		BIT(2)
#define ADMA_DESC_ATTR_ACT1		BIT(4)
#define ADMA_DESC_ATTR_ACT2		BIT(5)
#define ADMA_DESC_TRANSFER_DATA		ADMA_DESC_ATTR_ACT2

/*SRS10 Register*/
#define LEDC				BIT(0)
#define DT_WIDTH			BIT(1)
#define HS_EN				BIT(2)
/* Conf depends on SRS15.HV4E */
#define SDMA				0 << 3
#define ADMA2_32			2 << 3
#define ADMA2_64			3 << 3
/* here 0 defines the 64 Kb size */
#define MAX_64KB_PAGE			0

struct sdmmc_cmd {
	unsigned int	cmd_idx;
	unsigned int	cmd_arg;
	unsigned int	resp_type;
	unsigned int	resp_data[4];
};

struct sdmmc_ops {
	int (*init)(void);
	int (*busy)(void);
	int (*card_present)(void);
	int (*reset)(void);
	int (*send_cmd)(struct sdmmc_cmd *cmd, struct sdhc_data *data);
	int (*set_ios)(unsigned int clk, unsigned int width);
	int (*prepare)(uint32_t lba, uintptr_t buf, struct sdhc_data *data);
	int (*cache_invd)(int lba, uintptr_t buf, size_t size);
};

struct sdmmc_combo_phy {
	uint32_t	cp_clk_wr_delay;
	uint32_t	cp_clk_wrdqs_delay;
	uint32_t	cp_data_select_oe_end;
	uint32_t	cp_dll_bypass_mode;
	uint32_t	cp_dll_locked_mode;
	uint32_t	cp_dll_start_point;
	uint32_t	cp_gate_cfg_always_on;
	uint32_t	cp_io_mask_always_on;
	uint32_t	cp_io_mask_end;
	uint32_t	cp_io_mask_start;
	uint32_t	cp_rd_del_sel;
	uint32_t	cp_read_dqs_cmd_delay;
	uint32_t	cp_read_dqs_delay;
	uint32_t	cp_sw_half_cycle_shift;
	uint32_t	cp_sync_method;
	uint32_t	cp_underrun_suppress;
	uint32_t	cp_use_ext_lpbk_dqs;
	uint32_t	cp_use_lpbk_dqs;
	uint32_t	cp_use_phony_dqs;
	uint32_t	cp_use_phony_dqs_cmd;
};

struct sdmmc_sdhc {
	uint32_t	sdhc_extended_rd_mode;
	uint32_t	sdhc_extended_wr_mode;
	uint32_t	sdhc_hcsdclkadj;
	uint32_t	sdhc_idelay_val;
	uint32_t	sdhc_rdcmd_en;
	uint32_t	sdhc_rddata_en;
	uint32_t	sdhc_rw_compensate;
	uint32_t	sdhc_sdcfsh;
	uint32_t	sdhc_sdcfsl;
	uint32_t	sdhc_wrcmd0_dly;
	uint32_t	sdhc_wrcmd0_sdclk_dly;
	uint32_t	sdhc_wrcmd1_dly;
	uint32_t	sdhc_wrcmd1_sdclk_dly;
	uint32_t	sdhc_wrdata0_dly;
	uint32_t	sdhc_wrdata0_sdclk_dly;
	uint32_t	sdhc_wrdata1_dly;
	uint32_t	sdhc_wrdata1_sdclk_dly;
};

enum sdmmc_device_type {
	SD_DS_ID, /* Identification */
	SD_DS, /* Default speed */
	SD_HS, /* High speed */
	SD_UHS_SDR12, /* Ultra high speed SDR12 */
	SD_UHS_SDR25, /* Ultra high speed SDR25 */
	SD_UHS_SDR50, /* Ultra high speed SDR`50 */
	SD_UHS_SDR104, /* Ultra high speed SDR104 */
	SD_UHS_DDR50, /* Ultra high speed DDR50 */
	EMMC_SDR_BC, /* SDR backward compatible */
	EMMC_SDR, /* SDR */
	EMMC_DDR, /* DDR */
	EMMC_HS200, /* High speed 200Mhz in SDR */
	EMMC_HS400, /* High speed 200Mhz in DDR */
	EMMC_HS400es, /* High speed 200Mhz in SDR with enhanced strobe*/
};

struct cdns_sdmmc_params {
	uintptr_t	reg_base;
	uintptr_t	reg_pinmux;
	uintptr_t	reg_phy;
	uintptr_t	desc_base;
	size_t		desc_size;
	int		clk_rate;
	int		bus_width;
	unsigned int	flags;
	enum sdmmc_device_type	cdn_sdmmc_dev_type;
	uint32_t	combophy;
};

struct sdmmc_device_info {
	unsigned long long	device_size;	/* Size of device in bytes */
	unsigned int		block_size;	/* Block size in bytes */
	unsigned int		max_bus_freq;	/* Max bus freq in Hz */
	unsigned int		ocr_voltage;	/* OCR voltage */
	enum sdmmc_device_type	cdn_sdmmc_dev_type;	/* Type of MMC */
};

/* read and write API */
size_t sdmmc_read_blocks(int lba, uintptr_t buf, size_t size);
size_t sdmmc_write_blocks(int lba, const uintptr_t buf, size_t size);

struct cdns_idmac_desc {
	uint8_t attr;
	uint8_t reserved;
	uint16_t len;
	uint32_t addr_lo;
	uint32_t addr_hi;
} __packed;

void cdns_sdmmc_init(struct cdns_sdmmc_params *params, struct sdmmc_device_info *info,
	const struct sdmmc_ops **cb_sdmmc_ops);
