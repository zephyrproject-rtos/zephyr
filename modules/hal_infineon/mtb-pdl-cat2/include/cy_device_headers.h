/*
 * Copyright (c) 2025 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef HAL_INFINEON_MTB_PDL_CAT2_CY_DEVICE_HEADERS_H_
#define HAL_INFINEON_MTB_PDL_CAT2_CY_DEVICE_HEADERS_H_

/* This wrapper ensures infineon_kconfig.h is included before
 * the PDL cy_device_headers.h, providing the part number macros
 * required by the PDL headers. This header is placed in the HAL
 * include path before the PDL include path, so it shadows the
 * PDL header and ensures the kconfig mapping is always available.
 */
#include <infineon_kconfig.h>

/* Include the actual PDL header using #include_next to bypass this wrapper
 * and get the real cy_device_headers.h from the devices/include directory.
 */
#include_next <cy_device_headers.h>

#endif /* HAL_INFINEON_MTB_PDL_CAT2_CY_DEVICE_HEADERS_H_ */
