/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_UWB_STATUS_H_
#define ZEPHYR_INCLUDE_DRIVERS_UWB_STATUS_H_

#include <stdint.h>

/**
 * @brief UWB subsystem Status codes
 * @defgroup uwb_status Ultra-Wideband subsystem status codes
 * @{
 */

/**
 * Error codes returned by UWB subsystem - typedefed to uint32_t.
 * UCI error codes are returned as-is
 * Errors from other components in UWB subsystem are masked with \ref UWB_COMPONENT_STATUS_BITMASK
 * and their corresponding UWB_COMPONENT_* value
 * See \ref uwb_status_code for \ref UWB_COMPONENT_GENERIC
 * See \ref uwb_tml_status_code for \ref UWB_COMPONENT_TML
 */
typedef uint32_t uwb_status_code_t;

/**
 * Bitmask for UWB software component identifier
 */
#define UWB_COMPONENT_STATUS_BITMASK  (0x7FFF0000)
/**
 * Bitshift for UWB software component identifier
 */
#define UWB_COMPONENT_STATUS_BITSHIFT (16U)

/**
 * Macro to create component specific status code based on component identifier and status value
 */
#define UWB_MAKE_COMPONENT_STATUS(GROUP, VALUE)                                                    \
	(((GROUP << UWB_COMPONENT_STATUS_BITSHIFT) & UWB_COMPONENT_STATUS_BITMASK) + (VALUE))

/**
 * UWB software component for generic status codes
 */
#define UWB_COMPONENT_GENERIC (0x01)

/**
 * Generic status codes across UWB subsystem
 */
enum uwb_status_code {
	/** Success - Match with UCI success status code */
	kUwb_StatusCode_Success = 0x00000000,
	/** Generic failure */
	kUwb_StatusCode_Failed = UWB_MAKE_COMPONENT_STATUS(UWB_COMPONENT_GENERIC, 1),
	/** Invalid input arguments */
	kUwb_StatusCode_InvalidArgument = UWB_MAKE_COMPONENT_STATUS(UWB_COMPONENT_GENERIC, 2),
	/** Buffer size allocated is insufficient to perform the required operation */
	kUwb_StatusCode_InsufficientBuffer = UWB_MAKE_COMPONENT_STATUS(UWB_COMPONENT_GENERIC, 3),
	/** Could not allocate memory */
	kUwb_StatusCode_NoSpaceLeft = UWB_MAKE_COMPONENT_STATUS(UWB_COMPONENT_GENERIC, 4),
	/** Subsystem is not initialized */
	kUwb_StatusCode_NotInitialized = UWB_MAKE_COMPONENT_STATUS(UWB_COMPONENT_GENERIC, 5),
	/** Corrupted or garbage data passed or received */
	kUwb_StatusCode_Corrupted = UWB_MAKE_COMPONENT_STATUS(UWB_COMPONENT_GENERIC, 6),
	/** Failure from vendor UWB implementation */
	kUwb_StatusCode_VendorFailed = UWB_MAKE_COMPONENT_STATUS(UWB_COMPONENT_GENERIC, 7),
};

/**
 * @}
 */

#endif /** ZEPHYR_INCLUDE_DRIVERS_UWB_STATUS_H_ */
