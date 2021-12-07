/*
 * Copyright (c) 2018, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#ifndef MMC_LL_H
#define MMC_LL_H

#include <cache.h>
#include <zephyr.h>

/*
 * Intel SoC re-use Arm Trusted Firmware (ATF) driver code in Zephyr.
 * The migrated ATF driver code uses mmio_X macro to access the register.
 * The following macros map mmio_X to Zephyr compatible function for
 * register access. This allow Zephyr to re-use the ATF driver codes
 * without massive changes.
 */
#define mmio_write_32(addr, data)	sys_write32((data), (addr))
#define mmio_read_32(addr)		sys_read32((addr))
#define mmio_setbits_32(addr, mask)	sys_set_bits((addr), (mask))
#define mmio_clrbits_32(addr, mask)	sys_clear_bits((addr), (mask))

#define udelay(usec)			k_busy_wait(usec)
#define mdelay(msec)			k_busy_wait(msec * 1000U)

#define flush_dcache_range(buf, size)	arch_dcache_range((void *)buf, size,\
					K_CACHE_WB)

#define inv_dcache_range(buf, size)	arch_dcache_range((void *)buf, size,\
					K_CACHE_INVD)

#define panic()				k_panic()
#define zeromem(addr, size)		memset((void *)addr, 0, size)
#define BIT_64				BIT64

/* Refer to  atf/include/lib/libc/assert.h */
#define assert(e)			__ASSERT_NO_MSG(e);

/* Refer to atf/include/export/lib/utils_def_exp.h*/
#define	U_(_x)				(_x##U)
#define	U(_x)				U_(_x)

/* Refer to atf/tools/cert_create/include/debug.h */
#define ERROR(...)			printk("ERROR: "__VA_ARGS__)
#define VERBOSE(...)			printk("VERBOSE: " __VA_ARGS__)
#define WARN(...)			printk("WARNING: " __VA_ARGS__)
#define BIT_32(nr)			(U(1) << (nr))

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
#define MMC_RSP_136			BIT(1)		/* 136 bit response */
#define MMC_RSP_CRC			BIT(2)		/* expect valid crc */
#define MMC_RSP_CMD_IDX			BIT(3)		/* response contains cmd idx */
#define MMC_RSP_BUSY			BIT(4)		/* device may be busy */

/* JEDEC 4.51 chapter 6.12 */
#define MMC_RESPONSE_R1			(MMC_RSP_48 | MMC_RSP_CMD_IDX | MMC_RSP_CRC)
#define MMC_RESPONSE_R1B		(MMC_RESPONSE_R1 | MMC_RSP_BUSY)
#define MMC_RESPONSE_R2			(MMC_RSP_48 | MMC_RSP_136 | MMC_RSP_CRC)
#define MMC_RESPONSE_R3			(MMC_RSP_48)
#define MMC_RESPONSE_R4			(MMC_RSP_48)
#define MMC_RESPONSE_R5			(MMC_RSP_48 | MMC_RSP_CRC | MMC_RSP_CMD_IDX)
#define MMC_RESPONSE_R6			(MMC_RSP_48 | MMC_RSP_CRC | MMC_RSP_CMD_IDX)
#define MMC_RESPONSE_R7			(MMC_RSP_48 | MMC_RSP_CRC | MMC_RSP_CMD_IDX)

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

struct mmc_cmd {
	unsigned int	cmd_idx;
	unsigned int	cmd_arg;
	unsigned int	resp_type;
	unsigned int	resp_data[4];
};

struct mmc_ops {
	void (*init)(void);
	int (*send_cmd)(struct mmc_cmd *cmd);
	int (*set_ios)(unsigned int clk, unsigned int width);
	int (*prepare)(int lba, uintptr_t buf, size_t size);
	int (*read)(int lba, uintptr_t buf, size_t size);
	int (*write)(int lba, const uintptr_t buf, size_t size);
};

struct mmc_csd_emmc {
	unsigned int		not_used:		1;
	unsigned int		crc:			7;
	unsigned int		ecc:			2;
	unsigned int		file_format:		2;
	unsigned int		tmp_write_protect:	1;
	unsigned int		perm_write_protect:	1;
	unsigned int		copy:			1;
	unsigned int		file_format_grp:	1;

	unsigned int		reserved_1:		5;
	unsigned int		write_bl_partial:	1;
	unsigned int		write_bl_len:		4;
	unsigned int		r2w_factor:		3;
	unsigned int		default_ecc:		2;
	unsigned int		wp_grp_enable:		1;

	unsigned int		wp_grp_size:		5;
	unsigned int		erase_grp_mult:		5;
	unsigned int		erase_grp_size:		5;
	unsigned int		c_size_mult:		3;
	unsigned int		vdd_w_curr_max:		3;
	unsigned int		vdd_w_curr_min:		3;
	unsigned int		vdd_r_curr_max:		3;
	unsigned int		vdd_r_curr_min:		3;
	unsigned int		c_size_low:		2;

	unsigned int		c_size_high:		10;
	unsigned int		reserved_2:		2;
	unsigned int		dsr_imp:		1;
	unsigned int		read_blk_misalign:	1;
	unsigned int		write_blk_misalign:	1;
	unsigned int		read_bl_partial:	1;
	unsigned int		read_bl_len:		4;
	unsigned int		ccc:			12;

	unsigned int		tran_speed:		8;
	unsigned int		nsac:			8;
	unsigned int		taac:			8;
	unsigned int		reserved_3:		2;
	unsigned int		spec_vers:		4;
	unsigned int		csd_structure:		2;
};

struct mmc_csd_sd_v2 {
	unsigned int		not_used:		1;
	unsigned int		crc:			7;
	unsigned int		reserved_1:		2;
	unsigned int		file_format:		2;
	unsigned int		tmp_write_protect:	1;
	unsigned int		perm_write_protect:	1;
	unsigned int		copy:			1;
	unsigned int		file_format_grp:	1;

	unsigned int		reserved_2:		5;
	unsigned int		write_bl_partial:	1;
	unsigned int		write_bl_len:		4;
	unsigned int		r2w_factor:		3;
	unsigned int		reserved_3:		2;
	unsigned int		wp_grp_enable:		1;

	unsigned int		wp_grp_size:		7;
	unsigned int		sector_size:		7;
	unsigned int		erase_block_en:		1;
	unsigned int		reserved_4:		1;
	unsigned int		c_size_low:		16;

	unsigned int		c_size_high:		6;
	unsigned int		reserved_5:		6;
	unsigned int		dsr_imp:		1;
	unsigned int		read_blk_misalign:	1;
	unsigned int		write_blk_misalign:	1;
	unsigned int		read_bl_partial:	1;
	unsigned int		read_bl_len:		4;
	unsigned int		ccc:			12;

	unsigned int		tran_speed:		8;
	unsigned int		nsac:			8;
	unsigned int		taac:			8;
	unsigned int		reserved_6:		6;
	unsigned int		csd_structure:		2;
};

enum mmc_device_type {
	MMC_IS_EMMC,
	MMC_IS_SD,
	MMC_IS_SD_HC,
};

struct mmc_device_info {
	unsigned long long	device_size;	/* Size of device in bytes */
	unsigned int		block_size;	/* Block size in bytes */
	unsigned int		max_bus_freq;	/* Max bus freq in Hz */
	unsigned int		ocr_voltage;	/* OCR voltage */
	enum mmc_device_type	mmc_dev_type;	/* Type of MMC */
};

size_t mmc_read_blocks(int lba, uintptr_t buf, size_t size);
size_t mmc_write_blocks(int lba, const uintptr_t buf, size_t size);
size_t mmc_erase_blocks(int lba, size_t size);
size_t mmc_rpmb_read_blocks(int lba, uintptr_t buf, size_t size);
size_t mmc_rpmb_write_blocks(int lba, const uintptr_t buf, size_t size);
size_t mmc_rpmb_erase_blocks(int lba, size_t size);
int mmc_init(const struct mmc_ops *ops_ptr, unsigned int clk,
	     unsigned int width, unsigned int flags,
	     struct mmc_device_info *device_info);

#endif /* MMC_LL_H */
