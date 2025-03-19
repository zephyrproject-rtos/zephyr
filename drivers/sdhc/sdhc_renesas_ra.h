/*
 * Copyright (c) 2024 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define SDHI_PRV_ACCESS_TIMEOUT_US       100000U
#define SDHI_PRV_SD_OPTION_DEFAULT       0x40E0U
#define SDHI_PRV_SD_OPTION_WIDTH8_BIT    13
#define SDHI_PRV_BYTES_PER_KILOBYTE      1024
#define SDHI_PRV_SECTOR_COUNT_IN_EXT_CSD 0xFFFU
#define SDHI_TIME_OUT_MAX                0xFFFFFFFF
#define SDHI_PRV_RESPONSE_BIT            0

struct sdmmc_ra_event {
	volatile bool transfer_completed;
	struct k_sem transfer_sem;
};

struct sdmmc_ra_command {
	uint32_t opcode;
	uint32_t arg;
	void *data;
	unsigned int sector_count;
	unsigned int sector_size;
	int timeout_ms;
};

static ALWAYS_INLINE int err_fsp2zep(int fsp_err)
{
	int ret;

	switch (fsp_err) {
	/* Treating the error codes most relevant to be individuated */
	case FSP_SUCCESS:
		ret = 0;
		break;
	case FSP_ERR_TIMEOUT:
		ret = -ETIMEDOUT;
		break;
	case FSP_ERR_NOT_FOUND:
		ret = -ENODEV; /* SD card not inserted (requires CD signal) */
		break;
	case FSP_ERR_INVALID_STATE:
		ret = -EACCES; /* SD card write-protected (requires WP sinal) */
		break;
	case FSP_ERR_RESPONSE:
	default:
		ret = -EIO;
		break;
	}

	return ret;
}
