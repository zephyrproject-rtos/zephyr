/** @file
 *  @brief NFC subsystem core APIs.
 */

/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __NFC_H
#define __NFC_H

/**
 * @brief NFC Tag APIs
 * @defgroup nfc_tag NFC Tag APIs
 * @{
 */

#include <stddef.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief NDEF message structure.
 */
struct nfc_tag_msg {
	const u8_t *data;
	size_t len;
};

/** @brief NFC callback structure.
 */
struct nfc_tag_cb {
	/** @brief External NFC field has been detected.
	 *
	 *  NFC tag has detected external NFC field and was selected by an NFC
	 *  polling device.
	 */
	void (*field_detected)(void);

	/** @brief The NDEF message erasing request completed.
	 *
	 * @param[in] msg NDEF message.
	 */
	void (*ndef_data_erased)(const struct nfc_tag_msg *msg);

	/** @brief The NDEF message writing request completed.
	 *
	 * @param[in] msg NDEF message.
	 */
	void (*ndef_data_written)(const struct nfc_tag_msg *msg);

	/** @brief The NDEF message reading request completed.
	 *
	 * @param[in] msg NDEF message.
	 */
	void (*ndef_data_read)(const struct nfc_tag_msg *msg);

	/** @brief External NFC field has been lost.
	 *
	 *  NFC tag has lost external NFC field.
	 */
	void (*field_lost)(void);

	struct nfc_tag_cb *_next;
};

/** @brief NFC Tag operations.
 */
enum nfc_tag_op {
	/* NFC Tag has been enabled. See @ref nfc_tag_enable */
	NFC_TAG_ENABLED,
	/* NFC Tag has been disabled. See @ref nfc_tag_disable */
	NFC_TAG_DISABLED,
	/* NDEF message has been set. See @ref nfc_tag_ndef_msg_set */
	NFC_TAG_NDEF_MSG_SET
};

/**
 * @typedef nfc_tag_op_completed_cb_t
 * @brief Callback for notifying that the requested NFC operation has been
 *        completed.
 *
 * @param op type of operation
 * @param err zero on success or (negative) error code otherwise.
 */
typedef void (*nfc_tag_op_completed_cb_t)(enum nfc_tag_op op, int err);

/** @brief Set up NFC Tag.
 *
 *  Set up NFC Tag subsystem. Load NDEF message data.
 *
 *  @note This function can only be called when the NFC subsystem is disabled.
 *
 *  @param[in] msg NDEF message data.
 *
 *  @return 0 on success or a negative error code on failure.
 */
int nfc_tag_setup(const struct nfc_tag_msg *msg);

/** @brief Register NFC callbacks.
 *
 *  Register callbacks that will be called by NFC Tag subsystem to notify the
 *  application of relevant events.
 *
 *  @param[in] cb Callbacks.
 *
 *  @return 0 on success or a negative error code on failure.
 */
int nfc_tag_cb_register(struct nfc_tag_cb *cb);

/** @brief Start NFC Tag emulation.
 *
 *  After receiving a callback notification, events are posted to the
 *  application callbacks.
 *
 *  @param[in] cb Callback to notify completion.
 *
 *  @return 0 on success or a negative error code on failure.
 */
int nfc_tag_enable(nfc_tag_op_completed_cb_t cb);

/** @brief Stop NFC Tag emulation.
 *
 *  After receiving a callback notification, no more events will be posted to
 *  the application callbacks.
 *
 *  @param[in] cb Callback to notify completion.
 *
 *  @return 0 on success or a negative error code on failure.
 */
int nfc_tag_disable(nfc_tag_op_completed_cb_t cb);

/** @brief Set a new NDEF message in the enabled state.
 *
 *  After receiving a callback notification, the new NDEF message is set.
 *
 *  @param[in] msg NDEF message to be updated.
 *  @param[in] cb Callback to notify completion.
 *
 *  @return 0 on success or a negative error code on failure.
 */
int nfc_tag_ndef_msg_set(const struct nfc_tag_msg *msg,
			 nfc_tag_op_completed_cb_t cb);

/**
 * @}
 */

#ifdef __cplusplus
}
#endif
/**
 * @}
 */

#endif /* __NFC_H */
