/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <soc.h>
#include <zephyr/devicetree.h>
#include <zephyr/toolchain.h>

#include <ironside_client/image.h>
#include <ironside_client/layout.h>

#define ASSERT_PARTITION(label, addr, size)                                                 \
	BUILD_ASSERT(DT_REG_SIZE(DT_NODELABEL(label)) % 4096 == 0,                           \
		     #label " is not a whole number of 4 KB blocks");                        \
	BUILD_ASSERT(DT_PARTITION_ADDR(DT_NODELABEL(label)) == (addr) &&                     \
		     DT_REG_SIZE(DT_NODELABEL(label)) == (size),                             \
		     #label " is not where IronSide TE places it for these partition sizes")

ASSERT_PARTITION(slot0_partition, IRONSIDE_NVM_APP_ADDR, IRONSIDE_NVM_APP_SIZE);
ASSERT_PARTITION(dfubank_partition, IRONSIDE_NVM_DFUBANK_ADDR, IRONSIDE_NVM_DFUBANK_SIZE);
ASSERT_PARTITION(storage_partition, IRONSIDE_NVM_APPSTORAGE_ADDR, IRONSIDE_NVM_APPSTORAGE_SIZE);
ASSERT_PARTITION(urotstorage_partition, IRONSIDE_NVM_UROTSTORAGE_ADDR,
		 IRONSIDE_NVM_UROTSTORAGE_SIZE);
ASSERT_PARTITION(dfuimage_partition, IRONSIDE_NVM_DFUIMAGE_ADDR, IRONSIDE_NVM_DFUIMAGE_SIZE);

BUILD_ASSERT(DT_REG_ADDR(DT_NODELABEL(sram_ns)) + DT_REG_SIZE(DT_NODELABEL(sram_ns)) ==
		     IRONSIDE_RAM_NS_DATA_END,
	     "sram_ns must end where the APP-XRAM window starts");

/* sign.py takes the header gap to be IRONSIDE_IMAGE_ENTRY_OFFSET, whatever the image links. */
BUILD_ASSERT(CONFIG_ROM_START_OFFSET == IRONSIDE_IMAGE_ENTRY_OFFSET,
	     "IronSide TE boots the vector table IRONSIDE_IMAGE_ENTRY_OFFSET into the slot");
