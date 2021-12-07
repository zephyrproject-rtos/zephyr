/*
 * Copyright (c) 2018-2019, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* Define a simple and generic interface to access eMMC and SD-card devices. */

#include "mmc_ll.h"
#include <string.h>

#define MMC_DEFAULT_MAX_RETRIES		5
#define SEND_OP_COND_MAX_RETRIES	100

#define MULT_BY_512K_SHIFT		19

static const struct mmc_ops *ops;
static unsigned int mmc_ocr_value;
static struct mmc_csd_emmc mmc_csd;
static unsigned char mmc_ext_csd[512] __aligned(16);
static unsigned int mmc_flags;
static struct mmc_device_info *mmc_dev_info;
static unsigned int rca;
static unsigned int scr[2]__aligned(16) = { 0 };

static const unsigned char tran_speed_base[16] = {
	0, 10, 12, 13, 15, 20, 26, 30, 35, 40, 45, 52, 55, 60, 70, 80
};

static const unsigned char sd_tran_speed_base[16] = {
	0, 10, 12, 13, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 70, 80
};

static bool is_cmd23_enabled(void)
{
	return ((mmc_flags & MMC_FLAG_CMD23) != 0U);
}

static int mmc_send_cmd(unsigned int idx, unsigned int arg,
			unsigned int r_type, unsigned int *r_data)
{
	struct mmc_cmd cmd;
	int ret;

	zeromem(&cmd, sizeof(struct mmc_cmd));

	cmd.cmd_idx = idx;
	cmd.cmd_arg = arg;
	cmd.resp_type = r_type;

	ret = ops->send_cmd(&cmd);

	if ((ret == 0) && (r_data != NULL)) {
		int i;

		for (i = 0; i < 4; i++) {
			*r_data = cmd.resp_data[i];
			r_data++;
		}
	}

	if (ret != 0) {
		VERBOSE("Send command %u error: %d\n", idx, ret);
	}

	return ret;
}

static int mmc_device_state(void)
{
	int retries = MMC_DEFAULT_MAX_RETRIES;
	unsigned int resp_data[4];

	do {
		int ret;

		if (retries == 0) {
			ERROR("CMD13 failed after %d retries\n",
			      MMC_DEFAULT_MAX_RETRIES);
			return -EIO;
		}

		ret = mmc_send_cmd(MMC_CMD(13), rca << RCA_SHIFT_OFFSET,
				   MMC_RESPONSE_R1, &resp_data[0]);
		if (ret != 0) {
			retries--;
			continue;
		}

		if ((resp_data[0] & STATUS_SWITCH_ERROR) != 0U) {
			return -EIO;
		}

		retries--;
	} while ((resp_data[0] & STATUS_READY_FOR_DATA) == 0U);

	return MMC_GET_STATE(resp_data[0]);
}

static int mmc_set_ext_csd(unsigned int ext_cmd, unsigned int value)
{
	int ret;

	ret = mmc_send_cmd(MMC_CMD(6),
			   EXTCSD_WRITE_BYTES | EXTCSD_CMD(ext_cmd) |
			   EXTCSD_VALUE(value) | EXTCSD_CMD_SET_NORMAL,
			   MMC_RESPONSE_R1B, NULL);
	if (ret != 0) {
		return ret;
	}

	do {
		ret = mmc_device_state();
		if (ret < 0) {
			return ret;
		}
	} while (ret == MMC_STATE_PRG);

	return 0;
}

static int mmc_sd_switch(unsigned int bus_width)
{
	int ret;
	int retries = MMC_DEFAULT_MAX_RETRIES;
	unsigned int bus_width_arg = 0;

	ret = ops->prepare(0, (uintptr_t)&scr, sizeof(scr));
	if (ret != 0) {
		return ret;
	}

	/* CMD55: Application Specific Command */
	ret = mmc_send_cmd(MMC_CMD(55), rca << RCA_SHIFT_OFFSET,
			   MMC_RESPONSE_R5, NULL);
	if (ret != 0) {
		return ret;
	}

	/* ACMD51: SEND_SCR */
	do {
		ret = mmc_send_cmd(MMC_ACMD(51), 0, MMC_RESPONSE_R1, NULL);
		if ((ret != 0) && (retries == 0)) {
			ERROR("ACMD51 failed after %d retries (ret=%d)\n",
			      MMC_DEFAULT_MAX_RETRIES, ret);
			return ret;
		}

		retries--;
	} while (ret != 0);

	ret = ops->read(0, (uintptr_t)&scr, sizeof(scr));
	if (ret != 0) {
		return ret;
	}

	if (((scr[0] & SD_SCR_BUS_WIDTH_4) != 0U) &&
	    (bus_width == MMC_BUS_WIDTH_4)) {
		bus_width_arg = 2;
	}

	/* CMD55: Application Specific Command */
	ret = mmc_send_cmd(MMC_CMD(55), rca << RCA_SHIFT_OFFSET,
			   MMC_RESPONSE_R5, NULL);
	if (ret != 0) {
		return ret;
	}

	/* ACMD6: SET_BUS_WIDTH */
	ret = mmc_send_cmd(MMC_ACMD(6), bus_width_arg, MMC_RESPONSE_R1, NULL);
	if (ret != 0) {
		return ret;
	}

	do {
		ret = mmc_device_state();
		if (ret < 0) {
			return ret;
		}
	} while (ret == MMC_STATE_PRG);

	return 0;
}

static int mmc_set_ios(unsigned int clk, unsigned int bus_width)
{
	int ret;
	unsigned int width = bus_width;

	if (mmc_dev_info->mmc_dev_type != MMC_IS_EMMC) {
		if (width == MMC_BUS_WIDTH_8) {
			WARN("Wrong bus config for SD-card, force to 4\n");
			width = MMC_BUS_WIDTH_4;
		}
		ret = mmc_sd_switch(width);
		if (ret != 0) {
			return ret;
		}
	} else if (mmc_csd.spec_vers == 4U) {
		ret = mmc_set_ext_csd(CMD_EXTCSD_BUS_WIDTH,
				      (unsigned int)width);
		if (ret != 0) {
			return ret;
		}
	} else {
		VERBOSE("Wrong MMC type or spec version\n");
	}

	return ops->set_ios(clk, width);
}

static int mmc_fill_device_info(void)
{
	unsigned long long c_size;
	unsigned int speed_idx;
	unsigned int nb_blocks;
	unsigned int freq_unit;
	int ret = 0;
	struct mmc_csd_sd_v2 *csd_sd_v2;

	switch (mmc_dev_info->mmc_dev_type) {
	case MMC_IS_EMMC:
		mmc_dev_info->block_size = MMC_BLOCK_SIZE;

		ret = ops->prepare(0, (uintptr_t)&mmc_ext_csd,
				   sizeof(mmc_ext_csd));
		if (ret != 0) {
			return ret;
		}

		/* MMC CMD8: SEND_EXT_CSD */
		ret = mmc_send_cmd(MMC_CMD(8), 0, MMC_RESPONSE_R1, NULL);
		if (ret != 0) {
			return ret;
		}

		ret = ops->read(0, (uintptr_t)&mmc_ext_csd,
				sizeof(mmc_ext_csd));
		if (ret != 0) {
			return ret;
		}

		do {
			ret = mmc_device_state();
			if (ret < 0) {
				return ret;
			}
		} while (ret != MMC_STATE_TRAN);

		nb_blocks = (mmc_ext_csd[CMD_EXTCSD_SEC_CNT] << 0) |
			    (mmc_ext_csd[CMD_EXTCSD_SEC_CNT + 1] << 8) |
			    (mmc_ext_csd[CMD_EXTCSD_SEC_CNT + 2] << 16) |
			    (mmc_ext_csd[CMD_EXTCSD_SEC_CNT + 3] << 24);

		mmc_dev_info->device_size = (unsigned long long)nb_blocks *
			mmc_dev_info->block_size;

		break;

	case MMC_IS_SD:
		/*
		 * Use the same mmc_csd struct, as required fields here
		 * (READ_BL_LEN, C_SIZE, CSIZE_MULT) are common with eMMC.
		 */
		mmc_dev_info->block_size = BIT_32(mmc_csd.read_bl_len);

		c_size = ((unsigned long long)mmc_csd.c_size_high << 2U) |
			 (unsigned long long)mmc_csd.c_size_low;
		assert(c_size != 0xFFFU);

		mmc_dev_info->device_size = (c_size + 1U) *
					    BIT_64(mmc_csd.c_size_mult + 2U) *
					    mmc_dev_info->block_size;

		break;

	case MMC_IS_SD_HC:
		assert(mmc_csd.csd_structure == 1U);

		mmc_dev_info->block_size = MMC_BLOCK_SIZE;

		/* Need to use mmc_csd_sd_v2 struct */
		csd_sd_v2 = (struct mmc_csd_sd_v2 *)&mmc_csd;
		c_size = ((unsigned long long)csd_sd_v2->c_size_high << 16) |
			 (unsigned long long)csd_sd_v2->c_size_low;

		mmc_dev_info->device_size = (c_size + 1U) << MULT_BY_512K_SHIFT;

		break;

	default:
		ret = -EINVAL;
		break;
	}

	if (ret < 0) {
		return ret;
	}

	speed_idx = (mmc_csd.tran_speed & CSD_TRAN_SPEED_MULT_MASK) >>
			 CSD_TRAN_SPEED_MULT_SHIFT;

	assert(speed_idx > 0U);

	if (mmc_dev_info->mmc_dev_type == MMC_IS_EMMC) {
		mmc_dev_info->max_bus_freq = tran_speed_base[speed_idx];
	} else {
		mmc_dev_info->max_bus_freq = sd_tran_speed_base[speed_idx];
	}

	freq_unit = mmc_csd.tran_speed & CSD_TRAN_SPEED_UNIT_MASK;
	while (freq_unit != 0U) {
		mmc_dev_info->max_bus_freq *= 10U;
		--freq_unit;
	}

	mmc_dev_info->max_bus_freq *= 10000U;

	return 0;
}

static int sd_send_op_cond(void)
{
	int n;
	unsigned int resp_data[4];

	for (n = 0; n < SEND_OP_COND_MAX_RETRIES; n++) {
		int ret;

		/* CMD55: Application Specific Command */
		ret = mmc_send_cmd(MMC_CMD(55), 0, MMC_RESPONSE_R1, NULL);
		if (ret != 0) {
			return ret;
		}

		/* ACMD41: SD_SEND_OP_COND */
		ret = mmc_send_cmd(MMC_ACMD(41), OCR_HCS |
			mmc_dev_info->ocr_voltage, MMC_RESPONSE_R3,
			&resp_data[0]);
		if (ret != 0) {
			return ret;
		}

		if ((resp_data[0] & OCR_POWERUP) != 0U) {
			mmc_ocr_value = resp_data[0];

			if ((mmc_ocr_value & OCR_HCS) != 0U) {
				mmc_dev_info->mmc_dev_type = MMC_IS_SD_HC;
			} else {
				mmc_dev_info->mmc_dev_type = MMC_IS_SD;
			}

			return 0;
		}

		mdelay(10);
	}

	ERROR("ACMD41 failed after %d retries\n", SEND_OP_COND_MAX_RETRIES);

	return -EIO;
}

static int mmc_reset_to_idle(void)
{
	int ret;

	/* CMD0: reset to IDLE */
	ret = mmc_send_cmd(MMC_CMD(0), 0, 0, NULL);
	if (ret != 0) {
		return ret;
	}

	mdelay(2);

	return 0;
}

static int mmc_send_op_cond(void)
{
	int ret, n;
	unsigned int resp_data[4];

	ret = mmc_reset_to_idle();
	if (ret != 0) {
		return ret;
	}

	for (n = 0; n < SEND_OP_COND_MAX_RETRIES; n++) {
		ret = mmc_send_cmd(MMC_CMD(1), OCR_SECTOR_MODE |
				   OCR_VDD_MIN_2V7 | OCR_VDD_MIN_1V7,
				   MMC_RESPONSE_R3, &resp_data[0]);
		if (ret != 0) {
			return ret;
		}

		if ((resp_data[0] & OCR_POWERUP) != 0U) {
			mmc_ocr_value = resp_data[0];
			return 0;
		}

		mdelay(10);
	}

	ERROR("CMD1 failed after %d retries\n", SEND_OP_COND_MAX_RETRIES);

	return -EIO;
}

static int mmc_enumerate(unsigned int clk, unsigned int bus_width)
{
	int ret;
	unsigned int resp_data[4];

	ops->init();

	ret = mmc_reset_to_idle();
	if (ret != 0) {
		return ret;
	}

	if (mmc_dev_info->mmc_dev_type == MMC_IS_EMMC) {
		ret = mmc_send_op_cond();
	} else {
		/* CMD8: Send Interface Condition Command */
		ret = mmc_send_cmd(MMC_CMD(8), VHS_2_7_3_6_V | CMD8_CHECK_PATTERN,
				   MMC_RESPONSE_R5, &resp_data[0]);

		if ((ret == 0) && ((resp_data[0] & 0xffU) == CMD8_CHECK_PATTERN)) {
			ret = sd_send_op_cond();
		}
	}
	if (ret != 0) {
		return ret;
	}

	/* CMD2: Card Identification */
	ret = mmc_send_cmd(MMC_CMD(2), 0, MMC_RESPONSE_R2, NULL);
	if (ret != 0) {
		return ret;
	}

	/* CMD3: Set Relative Address */
	if (mmc_dev_info->mmc_dev_type == MMC_IS_EMMC) {
		rca = MMC_FIX_RCA;
		ret = mmc_send_cmd(MMC_CMD(3), rca << RCA_SHIFT_OFFSET,
				   MMC_RESPONSE_R1, NULL);
		if (ret != 0) {
			return ret;
		}
	} else {
		ret = mmc_send_cmd(MMC_CMD(3), 0,
				   MMC_RESPONSE_R6, &resp_data[0]);
		if (ret != 0) {
			return ret;
		}

		rca = (resp_data[0] & 0xFFFF0000U) >> 16;
	}

	/* CMD9: CSD Register */
	ret = mmc_send_cmd(MMC_CMD(9), rca << RCA_SHIFT_OFFSET,
			   MMC_RESPONSE_R2, &resp_data[0]);
	if (ret != 0) {
		return ret;
	}

	memcpy(&mmc_csd, &resp_data, sizeof(resp_data));

	/* CMD7: Select Card */
	ret = mmc_send_cmd(MMC_CMD(7), rca << RCA_SHIFT_OFFSET,
			   MMC_RESPONSE_R1, NULL);
	if (ret != 0) {
		return ret;
	}

	do {
		ret = mmc_device_state();
		if (ret < 0) {
			return ret;
		}
	} while (ret != MMC_STATE_TRAN);

	ret = mmc_set_ios(clk, bus_width);
	if (ret != 0) {
		return ret;
	}

	return mmc_fill_device_info();
}

size_t mmc_read_blocks(int lba, uintptr_t buf, size_t size)
{
	int ret;
	unsigned int cmd_idx, cmd_arg;

	assert((ops != NULL) &&
	       (ops->read != NULL) &&
	       (size != 0U) &&
	       ((size & MMC_BLOCK_MASK) == 0U));

	ret = ops->prepare(lba, buf, size);
	if (ret != 0) {
		return 0;
	}

	if (is_cmd23_enabled()) {
		/* Set block count */
		ret = mmc_send_cmd(MMC_CMD(23), size / MMC_BLOCK_SIZE,
				   MMC_RESPONSE_R1, NULL);
		if (ret != 0) {
			return 0;
		}

		cmd_idx = MMC_CMD(18);
	} else {
		if (size > MMC_BLOCK_SIZE) {
			cmd_idx = MMC_CMD(18);
		} else {
			cmd_idx = MMC_CMD(17);
		}
	}

	if (((mmc_ocr_value & OCR_ACCESS_MODE_MASK) == OCR_BYTE_MODE) &&
	    (mmc_dev_info->mmc_dev_type != MMC_IS_SD_HC)) {
		cmd_arg = lba * MMC_BLOCK_SIZE;
	} else {
		cmd_arg = lba;
	}

	ret = mmc_send_cmd(cmd_idx, cmd_arg, MMC_RESPONSE_R1, NULL);
	if (ret != 0) {
		return 0;
	}

	ret = ops->read(lba, buf, size);
	if (ret != 0) {
		return 0;
	}

	/* Wait buffer empty */
	do {
		ret = mmc_device_state();
		if (ret < 0) {
			return 0;
		}
	} while ((ret != MMC_STATE_TRAN) && (ret != MMC_STATE_DATA));

	if (!is_cmd23_enabled() && (size > MMC_BLOCK_SIZE)) {
		ret = mmc_send_cmd(MMC_CMD(12), 0, MMC_RESPONSE_R1B, NULL);
		if (ret != 0) {
			return 0;
		}
	}

	return size;
}

size_t mmc_write_blocks(int lba, const uintptr_t buf, size_t size)
{
	int ret;
	unsigned int cmd_idx, cmd_arg;

	assert((ops != NULL) &&
	       (ops->write != NULL) &&
	       (size != 0U) &&
	       ((buf & MMC_BLOCK_MASK) == 0U) &&
	       ((size & MMC_BLOCK_MASK) == 0U));

	ret = ops->prepare(lba, buf, size);
	if (ret != 0) {
		return 0;
	}

	if (is_cmd23_enabled()) {
		/* Set block count */
		ret = mmc_send_cmd(MMC_CMD(23), size / MMC_BLOCK_SIZE,
				   MMC_RESPONSE_R1, NULL);
		if (ret != 0) {
			return 0;
		}

		cmd_idx = MMC_CMD(25);
	} else {
		if (size > MMC_BLOCK_SIZE) {
			cmd_idx = MMC_CMD(25);
		} else {
			cmd_idx = MMC_CMD(24);
		}
	}

	if ((mmc_ocr_value & OCR_ACCESS_MODE_MASK) == OCR_BYTE_MODE) {
		cmd_arg = lba * MMC_BLOCK_SIZE;
	} else {
		cmd_arg = lba;
	}

	ret = mmc_send_cmd(cmd_idx, cmd_arg, MMC_RESPONSE_R1, NULL);
	if (ret != 0) {
		return 0;
	}

	ret = ops->write(lba, buf, size);
	if (ret != 0) {
		return 0;
	}

	/* Wait buffer empty */
	do {
		ret = mmc_device_state();
		if (ret < 0) {
			return 0;
		}
	} while ((ret != MMC_STATE_TRAN) && (ret != MMC_STATE_RCV));

	if (!is_cmd23_enabled() && (size > MMC_BLOCK_SIZE)) {
		ret = mmc_send_cmd(MMC_CMD(12), 0, MMC_RESPONSE_R1B, NULL);
		if (ret != 0) {
			return 0;
		}
	}

	return size;
}

size_t mmc_erase_blocks(int lba, size_t size)
{
	int ret;

	assert(ops != NULL);
	assert((size != 0U) && ((size & MMC_BLOCK_MASK) == 0U));

	ret = mmc_send_cmd(MMC_CMD(35), lba, MMC_RESPONSE_R1, NULL);
	if (ret != 0) {
		return 0;
	}

	ret = mmc_send_cmd(MMC_CMD(36), lba + (size / MMC_BLOCK_SIZE) - 1U,
			   MMC_RESPONSE_R1, NULL);
	if (ret != 0) {
		return 0;
	}

	ret = mmc_send_cmd(MMC_CMD(38), lba, MMC_RESPONSE_R1B, NULL);
	if (ret != 0) {
		return 0;
	}

	do {
		ret = mmc_device_state();
		if (ret < 0) {
			return 0;
		}
	} while (ret != MMC_STATE_TRAN);

	return size;
}

static inline void mmc_rpmb_enable(void)
{
	mmc_set_ext_csd(CMD_EXTCSD_PARTITION_CONFIG,
			PART_CFG_BOOT_PARTITION1_ENABLE |
			PART_CFG_PARTITION1_ACCESS);
}

static inline void mmc_rpmb_disable(void)
{
	mmc_set_ext_csd(CMD_EXTCSD_PARTITION_CONFIG,
			PART_CFG_BOOT_PARTITION1_ENABLE);
}

size_t mmc_rpmb_read_blocks(int lba, uintptr_t buf, size_t size)
{
	size_t size_read;

	mmc_rpmb_enable();
	size_read = mmc_read_blocks(lba, buf, size);
	mmc_rpmb_disable();

	return size_read;
}

size_t mmc_rpmb_write_blocks(int lba, const uintptr_t buf, size_t size)
{
	size_t size_written;

	mmc_rpmb_enable();
	size_written = mmc_write_blocks(lba, buf, size);
	mmc_rpmb_disable();

	return size_written;
}

size_t mmc_rpmb_erase_blocks(int lba, size_t size)
{
	size_t size_erased;

	mmc_rpmb_enable();
	size_erased = mmc_erase_blocks(lba, size);
	mmc_rpmb_disable();

	return size_erased;
}

int mmc_init(const struct mmc_ops *ops_ptr, unsigned int clk,
	     unsigned int width, unsigned int flags,
	     struct mmc_device_info *device_info)
{
	assert((ops_ptr != NULL) &&
	       (ops_ptr->init != NULL) &&
	       (ops_ptr->send_cmd != NULL) &&
	       (ops_ptr->set_ios != NULL) &&
	       (ops_ptr->prepare != NULL) &&
	       (ops_ptr->read != NULL) &&
	       (ops_ptr->write != NULL) &&
	       (device_info != NULL) &&
	       (clk != 0) &&
	       ((width == MMC_BUS_WIDTH_1) ||
		(width == MMC_BUS_WIDTH_4) ||
		(width == MMC_BUS_WIDTH_8) ||
		(width == MMC_BUS_WIDTH_DDR_4) ||
		(width == MMC_BUS_WIDTH_DDR_8)));

	ops = ops_ptr;
	mmc_flags = flags;
	mmc_dev_info = device_info;

	return mmc_enumerate(clk, width);
}
