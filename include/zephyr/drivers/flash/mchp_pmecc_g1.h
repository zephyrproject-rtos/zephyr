/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ZEPHYR_INCLUDE_DRIVERS_FLASH_MCHP_PMECC_G1_H__
#define __ZEPHYR_INCLUDE_DRIVERS_FLASH_MCHP_PMECC_G1_H__

int ecc_init_user(const struct device *dev, struct nand_chip *chip);
int ecc_enable(const struct device *dev, uint32_t is_write);
int ecc_get_eccbytes(const struct device *dev, uint8_t *buf);
int ecc_process(const struct device *dev, uint8_t *data, uint8_t *oob);

#endif /* __ZEPHYR_INCLUDE_DRIVERS_FLASH_MCHP_PMECC_G1_H__ */
