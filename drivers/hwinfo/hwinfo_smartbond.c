/*
 * Copyright (c) 2023 Jerzy Kasenberg.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/sys/byteorder.h>
#include <soc.h>
#include <da1469x_trimv.h>

#define PRODUCT_INFO_GPOUP	(12U)
#define CHIP_ID_GPOUP		(13U)

#define PRODUCT_INFO_LENGTH	(3U)
#define CHIP_ID_LENGTH		(1U)

ssize_t z_impl_hwinfo_get_device_id(uint8_t *buffer, size_t length)
{
	size_t len;
	uint32_t unique_id[4];
	uint8_t product_info_len;
	uint8_t chip_id_len;

	product_info_len = da1469x_trimv_group_read(PRODUCT_INFO_GPOUP, &unique_id[0],
						    PRODUCT_INFO_LENGTH);
	chip_id_len = da1469x_trimv_group_read(CHIP_ID_GPOUP, &unique_id[3],
					       CHIP_ID_LENGTH);

	if ((product_info_len != PRODUCT_INFO_LENGTH) || (chip_id_len != CHIP_ID_LENGTH)) {
		return -ENODATA;
	}

	for (uint8_t i = 0; i < (product_info_len + chip_id_len); i++) {
		unique_id[i] = BSWAP_32(unique_id[i]);
	}

	len = MIN(length, sizeof(unique_id));

	memcpy(buffer, unique_id, len);

	return len;
}

int z_impl_hwinfo_get_reset_cause(uint32_t *cause)
{
	int ret = 0;
	uint32_t reason = CRG_TOP->RESET_STAT_REG;
	uint32_t flags = 0;

	/*
	 * When POR is detected other bits are not valid.
	 */
	if (reason & CRG_TOP_RESET_STAT_REG_PORESET_STAT_Msk) {
		flags = RESET_POR;
	} else {
		if (reason & CRG_TOP_RESET_STAT_REG_HWRESET_STAT_Msk) {
			flags |= RESET_PIN;
		}
		if (reason & CRG_TOP_RESET_STAT_REG_SWRESET_STAT_Msk) {
			flags |= RESET_SOFTWARE;
		}
		if (reason & CRG_TOP_RESET_STAT_REG_WDOGRESET_STAT_Msk) {
			flags |= RESET_WATCHDOG;
		}
		if (reason & CRG_TOP_RESET_STAT_REG_CMAC_WDOGRESET_STAT_Msk) {
			flags |= RESET_WATCHDOG;
		}
		if (reason & CRG_TOP_RESET_STAT_REG_SWD_HWRESET_STAT_Msk) {
			flags |= RESET_DEBUG;
		}
	}

	*cause = flags;

	return ret;
}

int z_impl_hwinfo_clear_reset_cause(void)
{
	int ret = 0;

	CRG_TOP->RESET_STAT_REG = 0;

	return ret;
}

int z_impl_hwinfo_get_supported_reset_cause(uint32_t *supported)
{
	*supported = (RESET_PIN
		| RESET_SOFTWARE
		| RESET_POR
		| RESET_WATCHDOG
		| RESET_DEBUG);

	return 0;
}
