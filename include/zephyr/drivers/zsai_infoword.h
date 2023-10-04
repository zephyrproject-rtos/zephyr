/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for ZSAI drivers
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_ZSAI_INFOWORD_H_
#define ZEPHYR_INCLUDE_DRIVERS_ZSAI_INFOWORD_H_

/**
 * @brief ZSAI internal Interface
 * @defgroup zsai_internal_interface ZSAI internal Interface
 * @ingroup io_interfaces
 * @{
 */

#include <errno.h>

#include <zephyr/types.h>
#include <stddef.h>
#include <sys/types.h>
#include <zephyr/device.h>
#include <zephyr/drivers/zsai.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @}
 */

/**
 * @brief ZSAI Interface
 * @defgroup zsai_interface ZSAI Interface
 * @ingroup io_interfaces
 * @{
 */
#define ZSAI_INFOWORD_WRITE_BLOCK_SIZE_1	0
#define ZSAI_INFOWORD_WRITE_BLOCK_SIZE_2	1
#define ZSAI_INFOWORD_WRITE_BLOCK_SIZE_4	2
#define ZSAI_INFOWORD_WRITE_BLOCK_SIZE_8	3
#define ZSAI_INFOWORD_WRITE_BLOCK_SIZE_16	4
#define ZSAI_INFOWORD_WRITE_BLOCK_SIZE_32	5
#define ZSAI_INFOWORD_WRITE_BLOCK_SIZE_64	6
#define ZSAI_INFOWORD_WRITE_BLOCK_SIZE_128	7

#define ZSAI_INFOWORD_WRITE_BLOCK_SIZE_SHIFT_CALC(size)				\
	(UTIL_CAT(ZSAI_INFOWORD_WRITE_BLOCK_SIZE_, size))

#define ZSAI_MK_INFOWORD_WRITE_BLOCK_SIZE(wbs)					\
	.write_block_size = (ZSAI_INFOWORD_WRITE_BLOCK_SIZE_SHIFT_CALC(wbs)),

/* Device supports erase although may not require it */
#if IS_ENABLED(CONFIG_ZSAI_DEVICE_HAS_ERASE)
#define ZSAI_MK_INFOWORD_ERRASE_SUPPORTED(es)	.erase_supported = (es),
#define ZSAI_MK_INFOWORD_UNIFORM_PAGE_SIZE(u)	.uniform_page_size = (u),
#define ZSAI_MK_INFOWORD_ERASE_VALUE(ev)	.erase_bit_value = ((ev) & 1),
#else
#define ZSAI_MK_INFOWORD_ERRASE_SUPPORTED(dev)
#define ZSAI_MK_INFOWORD_UNIFORM_PAGE_SIZE(u)
#define ZSAI_MK_INFOWORD_ERASE_VALUE(ev)
#endif

/* Device requires erase */
#if IS_ENABLED(CONFIG_ZSAI_DEVICE_REQUIRES_ERASE)
#define ZSAI_MK_INFOWORD_ERASE_REQUIRED(er)	.erase_required = (er),
#else
#define ZSAI_MK_INFOWORD_ERASE_REQUIRED(er)
#endif

/**
 * @brief Initialize infoword with provided values
 *
 * @param	wbs	: write block size.
 * @param	es	: hardware erase support.
 * @param	er	: erase required.
 * @param	ev	: erase value bit: 0 or 1.
 * @param	ups	: uniform page layout.
 */
#define ZSAI_MK_INFOWORD(wbs, es, er, ev, ups) {				\
		ZSAI_MK_INFOWORD_ERRASE_SUPPORTED(es)				\
		ZSAI_MK_INFOWORD_WRITE_BLOCK_SIZE(wbs)				\
		ZSAI_MK_INFOWORD_ERASE_REQUIRED(er)				\
		ZSAI_MK_INFOWORD_ERASE_VALUE(ev)				\
		ZSAI_MK_INFOWORD_UNIFORM_PAGE_SIZE(ups)				\
	}

#if IS_ENABLED(CONFIG_ZSAI_DEVICE_REQUIRES_ERASE) && !IS_ENABLED(CONFIG_ZSAI_DEVICE_HAS_NO_ERASE)
#define ZSAI_ERASE_REQUIRED(dev) (true)
#elif !IS_ENABLED(CONFIG_ZSAI_DEVICE_REQUIRES_ERASE) && IS_ENABLED(CONFIG_ZSAI_DEVICE_HAS_NO_ERASE)
#define ZSAI_ERASE_REQUIRED(dev) (false)
#else
#define ZSAI_ERASE_REQUIRED(dev) (ZSAI_DEV_INFOWORD(dev).erase_required == 1)
#endif

#if IS_ENABLED(CONFIG_ZSAI_DEVICE_ERASES_TO_0) && IS_ENABLED(CONFIG_ZSAI_DEVICE_ERASES_TO_1)
#define ZSAI_ERASE_VALUE(dev) ((uint32_t)(-(int)ZSAI_DEV_INFOWORD(dev).erase_bit_value))
#elif IS_ENABLED(CONFIG_ZSAI_DEVICE_ERASES_TO_0)
#define ZSAI_ERASE_VALUE(dev) (0x00)
#elif IS_ENABLED(CONFIG_ZSAI_DEVICE_ERASES_TO_1)
#define ZSAI_ERASE_VALUE(dev) ((uint32_t)-1)
#else
#define ZSAI_ERASE_VALUE(dev) (BUILD_ASSERT("No ZSAI device supports erase"))
#endif

#if IS_ENABLED(CONFIG_ZSAI_DEVICE_HAS_ERASE) && IS_ENABLED(CONFIG_ZSAI_DEVICE_HAS_NO_ERASE)
#define ZSAI_ERASE_SUPPORTED(dev)	(ZSAI_DEV_INFOWORD(dev).erase_supported == 1)
#elif IS_ENABLED(CONFIG_ZSAI_DEVICE_HAS_ERASE)
#define ZSAI_ERASE_SUPPORTED(dev)	(true)
#else
#define ZSAI_ERASE_SUPPORTED(dev)	(false)
#endif

#define ZSAI_WRITE_BLOCK_SIZE(dev) ((1 << ZSAI_DEV_INFOWORD(dev).write_block_size))

#ifdef __cplusplus
}
#endif

/**
 * @}
 */


#endif /* ZEPHYR_INCLUDE_DRIVERS_ZSAI_INFOWORD_H_ */
