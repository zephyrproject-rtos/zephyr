/*
 * Copyright (c) 2023 Texas Instruments Incorporated
 * Copyright (c) 2023 L Lakshmanan
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_RAT_H_
#define ZEPHYR_INCLUDE_RAT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
 * @brief Enum's to represent different possible region size for the address translate module
 */
enum address_trans_region_size {
	address_trans_region_size_1 = 0x0,
	address_trans_region_size_2,
	address_trans_region_size_4,
	address_trans_region_size_8,
	address_trans_region_size_16,
	address_trans_region_size_32,
	address_trans_region_size_64,
	address_trans_region_size_128,
	address_trans_region_size_256,
	address_trans_region_size_512,
	address_trans_region_size_1K,
	address_trans_region_size_2K,
	address_trans_region_size_4K,
	address_trans_region_size_8K,
	address_trans_region_size_16K,
	address_trans_region_size_32K,
	address_trans_region_size_64K,
	address_trans_region_size_128K,
	address_trans_region_size_256K,
	address_trans_region_size_512K,
	address_trans_region_size_1M,
	address_trans_region_size_2M,
	address_trans_region_size_4M,
	address_trans_region_size_8M,
	address_trans_region_size_16M,
	address_trans_region_size_32M,
	address_trans_region_size_64M,
	address_trans_region_size_128M,
	address_trans_region_size_256M,
	address_trans_region_size_512M,
	address_trans_region_size_1G,
	address_trans_region_size_2G,
	address_trans_region_size_4G
};

/**
 * @brief Region config structure
 */
struct address_trans_region_config {
	uint64_t system_addr;
	uint32_t local_addr;
	uint32_t size;
};

void sys_mm_drv_ti_rat_init(struct address_trans_region_config *region_config,
			    uint64_t rat_base_addr, uint8_t translate_regions);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_RAT_H_ */
