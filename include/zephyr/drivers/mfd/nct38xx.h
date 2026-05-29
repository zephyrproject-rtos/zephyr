/*
 * Copyright (c) 2023 Google, LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MFD_NCT38XX_H_
#define ZEPHYR_INCLUDE_DRIVERS_MFD_NCT38XX_H_

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get the semaphore reference for a NCT38xx instance. Callers
 * should pass the return value to k_sem_take/k_sem_give
 *
 * @param[in] dev     Pointer to device struct of the driver instance
 *
 * @return Address of the semaphore
 */
struct k_sem *mfd_nct38xx_get_lock_reference(const struct device *dev);

/**
 * @brief Get the I2C DT spec reference for a NCT38xx instance.
 *
 * @param[in] dev     Pointer to device struct of the driver instance
 *
 * @return Address of the I2C DT spec
 */
const struct i2c_dt_spec *mfd_nct38xx_get_i2c_dt_spec(const struct device *dev);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MFD_NCT38XX_H_ */
