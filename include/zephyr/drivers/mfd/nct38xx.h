/*
 * Copyright (c) 2023 Google, LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MFD_NCT38XX_H_
#define ZEPHYR_INCLUDE_DRIVERS_MFD_NCT38XX_H_

#include <zephyr/device.h>
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
 * @brief Read a NCT38XX register
 *
 * @param dev NCT38XX multi-function device
 * @param reg_addr Register address
 * @param val A pointer to a buffer for the data to return
 *
 * @return 0 if successful, otherwise failed.
 */
int nct38xx_reg_read_byte(const struct device *dev, uint8_t reg_addr, uint8_t *val);

/**
 * @brief Read a sequence of NCT38XX registers
 *
 * @param dev NCT38XX multi-function device
 * @param start_addr The register start address
 * @param buf A pointer to a buffer for the data to return
 * @param num_bytes Number of data to read
 *
 * @return 0 if successful, otherwise failed.
 */
int nct38xx_reg_burst_read(const struct device *dev, uint8_t start_addr, uint8_t *buf,
			   uint32_t num_bytes);

/**
 * @brief Write a NCT38XX register
 *
 * @param dev NCT38XX device
 * @param reg_addr Register address
 * @param val Data to write
 *
 * @return 0 if successful, otherwise failed.
 */
int nct38xx_reg_write_byte(const struct device *dev, uint8_t reg_addr, uint8_t val);

/**
 * @brief Write a sequence of NCT38XX registers
 *
 * @param dev NCT38XX device
 * @param start_addr The register start address
 * @param buf A pointer to a buffer for the data to write
 * @param num_bytes Number of data to write
 *
 * @return 0 if successful, otherwise failed.
 */
int nct38xx_reg_burst_write(const struct device *dev, uint8_t start_addr, uint8_t *buf,
			    uint32_t num_bytes);

/**
 * @brief Compare data & write a NCT38XX register
 *
 * @param dev NCT38XX device
 * @param reg_addr Register address
 * @param reg_val Old register data
 * @param new_val New register data
 *
 * @return 0 if successful, otherwise failed.
 */
int nct38xx_reg_update(const struct device *dev, uint8_t reg_addr, uint8_t reg_val,
		       uint8_t new_val);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MFD_NCT38XX_H_ */
