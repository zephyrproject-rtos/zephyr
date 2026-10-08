/* Copyright (c) 2026 BayLibre SAS
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef SECURE_STORAGE_PS_COMMON_H
#define SECURE_STORAGE_PS_COMMON_H

/** @file zephyr/secure_storage/ps/common.h
 * @brief Common definitions of the secure storage subsystem's PS APIs.
 */
#include "../common.h"
#include <zephyr/toolchain.h>
#include <psa/storage_common.h>

#ifdef CONFIG_SECURE_STORAGE_PS_TRANSFORM_MODULE

/** The maximum size, in bytes, of an entry's data after it has been transformed for storage. */
enum { SECURE_STORAGE_PS_TRANSFORM_MAX_STORED_DATA_SIZE
	= CONFIG_SECURE_STORAGE_PS_MAX_DATA_SIZE
	  + CONFIG_SECURE_STORAGE_PS_TRANSFORM_OUTPUT_OVERHEAD };

/** The size, in bytes, of an entry's data given its size once transformed for storage. */
#define SECURE_STORAGE_PS_TRANSFORM_DATA_SIZE(transformed_data_size) \
	(transformed_data_size - (SECURE_STORAGE_PS_TRANSFORM_MAX_STORED_DATA_SIZE \
				  - CONFIG_SECURE_STORAGE_PS_MAX_DATA_SIZE))

#endif /* CONFIG_SECURE_STORAGE_PS_TRANSFORM_MODULE */

#endif
