/*
 * Copyright (c) 2025 Tenstorrent AI ULC
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/mspi.h>
#include "flash_mspi_nor.h"

const struct flash_mspi_nor_vendor *vendors[] = {
#if defined(CONFIG_FLASH_MSPI_NOR_MICRON)
	&micron_vendor,
#endif
};

const uint32_t vendor_count = ARRAY_SIZE(vendors);
