/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Microchip PMECC G1 driver interface.
 */

#ifndef _ZEPHYR_DRIVERS_FLASH_FLASH_MCHP_PMECC_G1_H_
#define _ZEPHYR_DRIVERS_FLASH_FLASH_MCHP_PMECC_G1_H_

#include "flash_mchp_nand_g1.h"

/**
 * @brief Initialize the PMECC user interface.
 *
 * @param[in] dev  PMECC device instance.
 * @param[in] chip NAND chip instance.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int ecc_init_user(const struct device *dev, struct nand_chip *chip);

/**
 * @brief Enable PMECC operation.
 *
 * @param[in] dev      PMECC device instance.
 * @param[in] is_write Operation direction.
 *
 * A nonzero value indicates a write operation. A value of zero indicates
 * a read operation.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int ecc_enable(const struct device *dev, uint32_t is_write);

/**
 * @brief Get the calculated ECC bytes.
 *
 * @param[in]  dev PMECC device instance.
 * @param[out] buf Buffer used to store the ECC bytes.
 *
 * @return 0 on success, or a negative error code on failure.
 */
int ecc_get_eccbytes(const struct device *dev, uint8_t *buf);

/**
 * @brief Check the integrity of data read from NAND flash.
 *
 * @param[in]  dev  PMECC device instance.
 * @param[in]  data Page data buffer read from NAND flash.
 * @param[out] oob  OOB data containing the stored ECC bytes.
 *
 * @return 0 on success, no flipped bit found.
 * @return >0 positive value indicating the number of corrected bit flips,
 *         or the maximum corrected bit flips in an ecc sector of the page.
 * @return -EBADMSG indicating ECC error cannot be corrected.
 * @return <0 negative values for other errors.
 */
int ecc_process(const struct device *dev, uint8_t *data, uint8_t *oob);

#endif /* _ZEPHYR_DRIVERS_FLASH_FLASH_MCHP_PMECC_G1_H_ */
