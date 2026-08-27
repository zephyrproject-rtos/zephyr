/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for MXCRYPTO MFD driver
 * @ingroup mfd_interface_infineon_mxcrypto
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MFD_INFINEON_MXCRYPTO_H_
#define ZEPHYR_INCLUDE_DRIVERS_MFD_INFINEON_MXCRYPTO_H_

#include <zephyr/device.h>
#include <zephyr/kernel.h>

#include "cy_device.h"
#include "cy_device_headers.h"

/**
 * @defgroup mfd_interface_infineon_mxcrypto MFD Infineon MXCRYPTO interface
 * @ingroup mfd_interfaces
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get the MXCRYPTO register base address.
 *
 * The base address is shared by the TRNG and crypto (AES / SHA) child
 * functions of the MXCRYPTO block.
 *
 * @param[in] dev Pointer to the parent MFD device.
 *
 * @return Pointer to the CRYPTO register block.
 */
CRYPTO_Type *mfd_infineon_mxcrypto_get_base(const struct device *dev);

/**
 * @brief Acquire the shared MXCRYPTO access mutex.
 *
 * The TRNG and crypto child functions share a single hardware block, so a
 * child must hold this lock for the duration of any multi-step hardware
 * operation that must not be interleaved with the sibling function.
 *
 * @param[in] dev     Pointer to the parent MFD device.
 * @param[in] timeout Lock acquisition timeout.
 *
 * @retval 0 on success.
 * @retval -EAGAIN if the lock could not be acquired within @p timeout.
 */
int mfd_infineon_mxcrypto_lock(const struct device *dev, k_timeout_t timeout);

/**
 * @brief Release the shared MXCRYPTO access mutex.
 *
 * @param[in] dev Pointer to the parent MFD device.
 */
void mfd_infineon_mxcrypto_unlock(const struct device *dev);

#ifdef __cplusplus
}
#endif

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_MFD_INFINEON_MXCRYPTO_H_ */
