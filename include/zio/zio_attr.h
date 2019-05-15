/*
 * Copyright (c) 2019 Thomas Burdick <thomas.burdick@gmail.com>
 * Copyright (c) 2019 Kevin Townsend
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ZIO attribute definitions.
 */

#ifndef ZEPHYR_INCLUDE_ZIO_ATTR_H_
#define ZEPHYR_INCLUDE_ZIO_ATTR_H_

#include <zephyr/types.h>
#include <device.h>
#include <zio/zio_variant.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief ZIO attribute enum and struct definitions.
 * @defgroup zio_attributes ZIO attribute definitions
 * @ingroup zio
 * @{
 */

/** @brief ZIO attribute types. */
enum zio_attr_type {
	/* samples per second */
	ZIO_SAMPLE_RATE,

	/* A count of attribute types always begins the user definable range */
	ZIO_ATTR_TYPES,

	/* enforce a specific enum size allowing for a good number of user
	 * definable values
	 */
	ZIO_ATTR_MAKE_16_BIT = 0xFFFF
};

/** Attribute record. */
struct zio_attr {
	/** Attribute identifier. */
	enum zio_attr_type type;
	/** Attribute variant. */
	struct zio_variant data;
};

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_ZIO_ATTR_H_ */
