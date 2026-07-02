/*
 * Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __UWB_STATUS_H__
#define __UWB_STATUS_H__

#include <stdint.h>

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
enum {
	kUwb_StatusCode_Success = 0x00000000, /* Match with UCI success status code */
	kUwb_StatusCode_Failed = UWB_MAKE_COMPONENT_STATUS(UWB_COMPONENT_GENERIC, 1),
	kUwb_StatusCode_InvalidArgument = UWB_MAKE_COMPONENT_STATUS(UWB_COMPONENT_GENERIC, 2),
	kUwb_StatusCode_InsufficientBuffer = UWB_MAKE_COMPONENT_STATUS(UWB_COMPONENT_GENERIC, 3),
	kUwb_StatusCode_NoSpaceLeft = UWB_MAKE_COMPONENT_STATUS(UWB_COMPONENT_GENERIC, 4),
	kUwb_StatusCode_NotInitialized = UWB_MAKE_COMPONENT_STATUS(UWB_COMPONENT_GENERIC, 5),
	kUwb_StatusCode_Corrupted = UWB_MAKE_COMPONENT_STATUS(UWB_COMPONENT_GENERIC, 6),
	kUwb_StatusCode_VendorFailed = UWB_MAKE_COMPONENT_STATUS(UWB_COMPONENT_GENERIC, 7),
};

#endif /** __UWB_STATUS_H__ */
