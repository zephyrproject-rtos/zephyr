/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file mchp_xec_espi.h
 * @brief Microchip eSPI Driver Header
 *
 * @details
 * This header provides conditional inclusion of Microchip eSPI controller
 * extension to types from espi.h
 */

#ifndef INCLUDE_ZEPHYR_DRIVERS_ESPI_MCHP_XEC_ESPI_H_
#define INCLUDE_ZEPHYR_DRIVERS_ESPI_MCHP_XEC_ESPI_H_

#include <zephyr/drivers/espi.h>

#ifdef CONFIG_ESPI_PERIPHERAL_MAILBOX

#define MCHP_XEC_MAX_MAILBOX_INDEX 32

enum mchp_xec_espi_pc_mbox_cmd_id {
	MCHP_XEC_ESPI_PC_MBOX_CMD_ID_HOST_TO_EC = 0,
	MCHP_XEC_ESPI_PC_MBOX_CMD_ID_EC_TO_HOST,
	MCHP_XEC_ESPI_PC_MBOX_CMD_ID_SMI_SRC,
	MCHP_XEC_ESPI_PC_MBOX_CMD_ID_SMI_MASK,
	MCHP_XEC_ESPI_PC_MBOX_CMD_ID_MAX,
};

enum mchp_xec_espi_pc_mbox_isrc {
	MCHP_XEC_ESPI_PC_MBOX_ISRC_EC = 0,
	MCHP_XEC_ESPI_PC_MBOX_ISRC_SIRQ,
	MCHP_XEC_ESPI_PC_MBOX_ISRC_SIRQ_SMI,
	MCHP_XEC_ESPI_PC_MBOX_ISRC_MAX,
};

/** @brief Read peripheral channel mailbox data
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param mbox_idx
 * @param num_mboxes
 * @param dest
 *
 * @retval 0 success
 * @retval -EINVAL if dev is NULL or mbox_idx > 32 or dest is NULL
 */
int mchp_xec_espi_pc_mailbox_get(const struct device *dev, uint8_t mbox_idx, uint8_t num_mboxes,
				 uint8_t *dest);

/** @brief Write peripheral channel mailbox data
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param mbox_idx
 * @param num_mboxes
 * @param src
 *
 * @retval 0 success
 * @retval -EINVAL if dev is NULL or mbox_idx > 32 or dest is NULL
 */
int mchp_xec_espi_pc_mailbox_set(const struct device *dev, uint8_t mbox_idx, uint8_t num_mboxes,
				 const uint8_t *src);

/** @brief Read all 32-byte of the peripheral channel mailbox data
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param dest pointer to >= 32-bit aligned buffer at least 32 bytes in size
 *
 * @retval 0 success
 * @retval -EINVAL if dev or dest is NULL
 */
int mchp_xec_espi_pc_mailbox_get_all(const struct device *dev, uint32_t *dest);

/** @brief Write all 32 bytes of the peripheral channel mailbox data
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param src pointer to 32-bit aligned constant buffer of 32 bytes.
 *
 * @retval 0 success
 * @retval -EINVAL if dev or src are NULL
 */
int mchp_xec_espi_pc_mailbox_set_all(const struct device *dev, const uint32_t *src);

/** @brief Write command to specified mailbox box command register
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param cmd_id enum mchp_xec_espi_pc_mbox_cmd_id specifies the mailbox command register
 *               to write.
 * @param cmd
 *
 * @retval 0 success
 * @retval -EINVAL if dev is NULL or cmd_id is invalid
 */
int mchp_xec_espi_pc_mailbox_cmd(const struct device *dev, enum mchp_xec_espi_pc_mbox_cmd_id cmd_id,
				 uint8_t cmd);

/** @brief Write command to specified mailbox box command register
 *
 * @param dev Pointer to the device structure for the driver instance
 * @param id enum mchp_xec_espi_pc_mbox_isrc specifying the interrupt (internal EC or serial IRQ)
 * @param enable a zero value is disable else enable
 *
 * @retval 0 success
 * @retval -EINVAL if dev is NULL or id is invalid
 */
int mchp_xec_espi_pc_mailbox_ictrl(const struct device *dev, enum mchp_xec_espi_pc_mbox_isrc id,
				   uint8_t enable);

#endif /* CONFIG_ESPI_PERIPHERAL_MAILBOX */

#endif /* INCLUDE_ZEPHYR_DRIVERS_ESPI_MCHP_XEC_ESPI_H_ */
