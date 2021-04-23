/*
 * Copyright (c) 2017-2021 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for flash map
 */

/*
 * The file contains functions that will be deprecated and removed from flash area API
 */
#ifndef ZEPHYR_INCLUDE_STORAGE_FLASH_MAP_PHASE_OUT_H_
#define ZEPHYR_INCLUDE_STORAGE_FLASH_MAP_PHASE_OUT_H_

#ifdef __cplusplus
extern "C" {
#endif

struct flash_area;

/**
 * @brief Retrieve partitions flash area from the flash_map.
 *
 * NOTE: This is probably not the function you want to use, rather use FLASH_AREA macro to obtain
 *	 flash area object directly.
 *
 * Function Retrieves flash_area from flash_map for given partition.
 *
 * @param[in]  id ID of the flash partition;
 * @param[out] fa Pointer which has to reference flash_area; will be set to NULL if @p id could
 *	       not be found.
 *
 * @return  0 on success, -EACCES if the flash_map is not available, -ENOENT if @p id is unknown.
 */
int flash_area_open(uint8_t id, const struct flash_area **fa);

/**
 * @brief Close flash_area
 *
 * Reserved for future usage and external projects compatibility reason.
 * Currently is NOP.
 *
 * @param[in] fa Flash area to be closed.
 */
static inline void flash_area_close(const struct flash_area *fa)
{
	/* Nothing here */
}

/**
 * Check whether given flash area has supporting flash driver
 * in the system.
 *
 * Deprecated and will be removed. The driver is not assigned at compile time, to check if it
 * has been correctly assigned flash_area.fa_dev may be checked for NULL value, although
 * such situation should never happen or compilation should fail.
 *
 * @param[in] fa Flash area.
 *
 * @return 1 On success. -ENODEV if no driver match.
 */
__deprecated
int flash_area_has_driver(const struct flash_area *fa);

/**
 * Get driver for given flash area.
 *
 * Deprecated and will be removed. To get device use flash_area.fa_dev member.
 *
 * @param fa Flash area.
 *
 * @return device driver.
 */
__deprecated
const struct device *flash_area_get_device(const struct flash_area *fa);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_STORAGE_FLASH_MAP_PHASE_OUT_H_ */
