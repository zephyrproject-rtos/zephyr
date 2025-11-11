/*
 * Copyright (c) 2025 Vogl Electronic GmbH
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MFD_MAXQ10XX_H_
#define ZEPHYR_INCLUDE_DRIVERS_MFD_MAXQ10XX_H_

#include <zephyr/device.h>
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get the semaphore reference for a MAXQ1xx instance. Callers
 * should pass the return value to k_sem_take/k_sem_give
 *
 * @param[in] dev     Pointer to device struct of the driver instance
 *
 * @return Address of the semaphore
 */
struct k_sem *mfd_maxq10xx_get_lock(const struct device *dev);


#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MFD_MAXQ10XX_H_ */
