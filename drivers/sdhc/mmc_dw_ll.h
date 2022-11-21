/*
 * Copyright (C) 2022 Intel Corporation
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

#ifndef MMC_DW_LL_H
#define MMC_DW_LL_H

#include <zephyr/cache.h>
#include <zephyr/kernel.h>

/* Refer to atf/include/export/lib/utils_def_exp.h*/
#define U(_x)					(_x##U)

/* Refer to atf/tools/cert_create/include/debug.h */
#define BIT_32(nr)				(U(1) << (nr))

/* MMC Peripheral Definition */
#define MMC_BLOCK_SIZE			U(512)
#define MMC_BLOCK_MASK			(MMC_BLOCK_SIZE - U(1))
#define MMC_BOOT_CLK_RATE		(400 * 1000)

#define MMC_CMD(_x)				U(_x)

#define MMC_ACMD(_x)			U(_x)

#define OCR_POWERUP				BIT(31)
#define OCR_HCS					BIT(30)
#define OCR_BYTE_MODE			(U(0) << 29)
#define OCR_SECTOR_MODE			(U(2) << 29)
#define OCR_ACCESS_MODE_MASK	(U(3) << 29)
#define OCR_3_5_3_6				BIT(23)
#define OCR_3_4_3_5				BIT(22)
#define OCR_3_3_3_4				BIT(21)
#define OCR_3_2_3_3				BIT(20)
#define OCR_3_1_3_2				BIT(19)
#define OCR_3_0_3_1				BIT(18)
#define OCR_2_9_3_0				BIT(17)
#define OCR_2_8_2_9				BIT(16)
#define OCR_2_7_2_8				BIT(15)
#define OCR_VDD_MIN_2V7			GENMASK(23, 15)
#define OCR_VDD_MIN_2V0			GENMASK(14, 8)
#define OCR_VDD_MIN_1V7			BIT(7)

#define MMC_RSP_48				BIT(0)
#define MMC_RSP_136				BIT(1)		/* 136 bit response */
#define MMC_RSP_CRC				BIT(2)		/* expect valid crc */
#define MMC_RSP_CMD_IDX			BIT(3)		/* response contains cmd idx */
#define MMC_RSP_BUSY			BIT(4)		/* device may be busy */

/* JEDEC 4.51 chapter 6.12 */
#define MMC_RESPONSE_NONE		0
#define MMC_RESPONSE_R1			(MMC_RSP_48 | MMC_RSP_CMD_IDX | MMC_RSP_CRC)
#define MMC_RESPONSE_R1B		(MMC_RESPONSE_R1 | MMC_RSP_BUSY)
#define MMC_RESPONSE_R2			(MMC_RSP_136 | MMC_RSP_CRC)
#define MMC_RESPONSE_R3			(MMC_RSP_48)

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
#define EXTCSD_CMD_SET_NORMAL	U(1)

#define CSD_TRAN_SPEED_UNIT_MASK	GENMASK(2, 0)
#define CSD_TRAN_SPEED_MULT_MASK	GENMASK(6, 3)
#define CSD_TRAN_SPEED_MULT_SHIFT	3

#define STATUS_CURRENT_STATE(x)		(((x) & 0xf) << 9)
#define STATUS_READY_FOR_DATA	BIT(8)
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

struct mmc_cmd {
	unsigned int	cmd_idx;
	unsigned int	cmd_arg;
	unsigned int	resp_type;
	unsigned int	resp_data[4];
};

struct mmc_ops {
	void (*init)(void);
	int (*busy)(void);
	int (*card_present)(void);
	int (*send_cmd)(struct mmc_cmd *cmd);
	int (*set_ios)(unsigned int clk, unsigned int width);
	int (*prepare)(int lba, uintptr_t buf, size_t size);
	int (*read)(int lba, uintptr_t buf, size_t size);
	int (*write)(int lba, const uintptr_t buf, size_t size);
};

enum mmc_dw_device_type {
	MMC_IS_EMMC,
	MMC_IS_SD,
	MMC_IS_SD_HC,
};

struct mmc_device_info {
	unsigned long long	device_size;	/* Size of device in bytes */
	unsigned int		block_size;	/* Block size in bytes */
	unsigned int		max_bus_freq;	/* Max bus freq in Hz */
	unsigned int		ocr_voltage;	/* OCR voltage */
	enum mmc_dw_device_type	mmc_dev_type;	/* Type of MMC */
};

struct dw_mmc_params {
	uintptr_t	reg_base;
	uintptr_t	desc_base;
	size_t		desc_size;
	int		clk_rate;
	int		bus_width;
	unsigned int	flags;
	enum mmc_dw_device_type	mmc_dev_type;
};

struct dw_idmac_desc {
	unsigned int	des0;
	unsigned int	des1;
	unsigned int	des2;
	unsigned int	des3;
};

void dw_mmc_init(struct dw_mmc_params *params, struct mmc_device_info *info,
	const struct mmc_ops **cb_mmc_ops);

#endif /* MMC_DW_LL_H */
