/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NFC_ISO_DEP_H_
#define ZEPHYR_INCLUDE_NFC_ISO_DEP_H_

#include <zephyr/drivers/nfc.h>
#include <zephyr/kernel.h>
#include <zephyr/nfc/poller.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief ISO-DEP (ISO/IEC 14443-4)
 * @defgroup nfc_iso_dep ISO-DEP (ISO/IEC 14443-4)
 * @ingroup nfc_subsys
 * @{
 */

/**
 * @brief Active ISO-DEP (ISO/IEC 14443-4) connection.
 *
 * The state is technology-neutral (ATS/ATTRIB-derived), but only NFC-A
 * activation (RATS) is implemented; NFC-B (ATTRIB) reports -ENOTSUP.
 */
struct nfc_iso_dep_tag {
	/** Bound poller session, captured by nfc_iso_dep_connect(). */
	struct nfc_poller *poller;
	struct nfc_target target;
	nfc_mode_t modes;
	uint8_t fsci;
	uint8_t fwi;
	uint8_t sfgi;
	uint8_t cid;
	bool cid_supported;
	bool nad_supported;
	uint8_t historical[32];
	uint8_t historical_len;
	/** Internal I-block number toggle; do not modify. */
	uint8_t block_num;
};

/** @brief ISO-DEP connect parameters. */
struct nfc_iso_dep_connect_param {
	uint8_t cid;
	k_timeout_t timeout;
};

/**
 * @brief Establish an ISO-DEP connection with an activated target.
 *
 * @param poller Open poller session the target was discovered on.
 * @param target Activated target from discovery.
 * @param param Connect parameters, or NULL for the defaults.
 * @param iso_dep Out: established connection.
 *
 * @retval 0 on success.
 * @retval -EINVAL if @p target or @p iso_dep is NULL.
 * @retval -EPERM if the poller session is not open.
 * @retval -ENOTSUP if @p target is not NFC-A, or on an offloading controller.
 * @retval -EAGAIN if the deadline has already passed.
 * @retval -EBADMSG if the target's ATS is malformed.
 */
int nfc_iso_dep_connect(struct nfc_poller *poller, const struct nfc_target *target,
			const struct nfc_iso_dep_connect_param *param,
			struct nfc_iso_dep_tag *iso_dep);

/**
 * @brief Exchange one application block, with chaining and WTX handling.
 *
 * @param iso_dep Established connection.
 * @param tx_data Application block to send.
 * @param tx_len Length of @p tx_data.
 * @param rx_data Buffer for the answer.
 * @param rx_len In: capacity of @p rx_data. Out: bytes received.
 * @param nad Node address, or 0 when the target reports no NAD support.
 * @param timeout Deadline for the exchange.
 *
 * @retval 0 on success.
 * @retval -EPERM if the poller session is not open.
 * @retval -EBADMSG if the target answers with an unexpected block.
 * @retval -EIO if the exchange fails after the protocol's retries.
 */
int nfc_iso_dep_transceive(struct nfc_iso_dep_tag *iso_dep, const uint8_t *tx_data, uint16_t tx_len,
			   uint8_t *rx_data, uint16_t *rx_len, uint8_t nad, k_timeout_t timeout);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NFC_ISO_DEP_H_ */
