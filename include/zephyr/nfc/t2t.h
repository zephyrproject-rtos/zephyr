/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NFC_T2T_H_
#define ZEPHYR_INCLUDE_NFC_T2T_H_

#include <zephyr/kernel.h>
#include <zephyr/nfc/tag.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief NFC Forum Type 2 Tag raw access
 * @defgroup nfc_t2t NFC Forum Type 2 Tag raw access
 * @ingroup nfc_subsys
 * @{
 *
 * Block-level access for what NDEF does not cover, such as a manufacturer page
 * or a vendor-specific memory layout. Connect with nfc_tag_connect() and reach
 * the backend with nfc_tag_t2t().
 */

/**
 * @brief Read 16 bytes (4 blocks) starting at @p block.
 *
 * @retval 0 on success.
 * @retval -EIO if the tag does not answer.
 */
int nfc_t2t_read_block(struct nfc_t2t_tag *t2t, uint8_t block, uint8_t out[16],
		       k_timeout_t timeout);

/**
 * @brief Write one 4-byte block.
 *
 * @retval 0 on success.
 * @retval -EIO if the tag does not acknowledge.
 */
int nfc_t2t_write_block(struct nfc_t2t_tag *t2t, uint8_t block, const uint8_t in[4],
			k_timeout_t timeout);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NFC_T2T_H_ */
