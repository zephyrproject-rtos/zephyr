/*
 * Copyright (c) 2023 PHOENIX CONTACT Electronics GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_MDIO_ADIN2111_H__
#define ZEPHYR_INCLUDE_DRIVERS_MDIO_ADIN2111_H__

#include <stdint.h>
#include <zephyr/device.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Read from MDIO Bus using Clause 45 access
 *
 * @note The caller is responsible for device lock.
 *       Shall not be called from ISR.
 *
 * @param[in] dev MDIO device.
 * @param[in] prtad Port address.
 * @param[in] devad Device address.
 * @param[in] regad Register address.
 * @param[out] data Pointer to receive read data.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 * @retval -ETIMEDOUT If transaction timedout on the bus.
 * @retval <0 Error, a negative errno code.
 */
int adin2111_mdio_c45_read(const struct device *dev, uint8_t prtad,
			   uint8_t devad, uint16_t regad, uint16_t *data);

/**
 * @brief Write to MDIO bus using Clause 45 access
 *
 * @note The caller is responsible for device lock.
 *       Shall not be called from ISR.
 *
 * @param[in] dev MDIO device.
 * @param[in] prtad Port address.
 * @param[in] devad Device address.
 * @param[in] regad Register address.
 * @param[in] data Data to write.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 * @retval -ETIMEDOUT If transaction timedout on the bus.
 * @retval <0 Error, a negative errno code.
 */
int adin2111_mdio_c45_write(const struct device *dev, uint8_t prtad,
			    uint8_t devad, uint16_t regad, uint16_t data);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_MDIO_ADIN2111_H__ */
