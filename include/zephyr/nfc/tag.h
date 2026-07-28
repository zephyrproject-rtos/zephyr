/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NFC_TAG_H_
#define ZEPHYR_INCLUDE_NFC_TAG_H_

#include <zephyr/drivers/nfc.h>
#include <zephyr/kernel.h>
#include <zephyr/nfc/iso_dep.h>
#include <zephyr/nfc/poller.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief NFC subsystem
 * @defgroup nfc_subsys NFC subsystem
 * @ingroup connectivity
 *
 * Vendor- and technology-neutral NFC protocol layers above the NFC driver API:
 * NFC-A activation, ISO-DEP, the NFC Forum tag types, and NDEF.
 */

/**
 * @brief NDEF tag access
 * @defgroup nfc_tag NDEF tag access
 * @ingroup nfc_subsys
 * @{
 *
 * The single way to read and write the NDEF message on a tag. The tag type is
 * detected during nfc_tag_connect() and every operation afterwards is the same
 * regardless of it.
 *
 * A caller that needs more than NDEF reaches the backend through an accessor:
 * nfc_tag_t2t() for raw Type 2 blocks, nfc_tag_iso_dep() for its own APDUs.
 */

/** @brief NFC Forum tag type an NDEF tag was resolved to. */
enum nfc_tag_type {
	/** Type 2 Tag, reached over raw NFC-A. */
	NFC_TAG_TYPE_T2T,
	/** Type 4 Tag on NFC-A, reached over ISO-DEP. */
	NFC_TAG_TYPE_T4T_A,
	/** Type 4 Tag on NFC-B. Reserved; NFC-B activation is not implemented. */
	NFC_TAG_TYPE_T4T_B,
};

/**
 * @brief Type 2 Tag backend state.
 *
 * Filled in by nfc_tag_connect(); reach it with nfc_tag_t2t() for the raw block
 * access in @ref nfc_t2t.
 */
struct nfc_t2t_tag {
	/** Bound poller session, captured at connect time. */
	struct nfc_poller *poller;
	struct nfc_target target;
	/** Usable data-area size in bytes, from the Capability Container. */
	uint16_t data_size;
	/** Whether the tag reports write access. */
	bool writable;
};

/**
 * @brief Type 4 Tag backend state.
 *
 * Filled in by nfc_tag_connect(). It has no raw API of its own; a caller that
 * needs to send its own APDUs uses nfc_tag_iso_dep().
 */
struct nfc_t4t_tag {
	struct nfc_iso_dep_tag iso_dep;
	/** Selected NDEF file identifier, from the Capability Container. */
	uint16_t ndef_file_id;
	/** Maximum NDEF message size the tag accepts. */
	uint16_t max_ndef_len;
	/** Whether the tag reports write access. */
	bool writable;
};

/** @brief A connected NDEF tag, of whichever type was detected. */
struct nfc_tag {
	/** Detected tag type, selects the valid union member. */
	enum nfc_tag_type type;
	union {
		struct nfc_t2t_tag t2t;
		struct nfc_t4t_tag t4t;
	};
};

/**
 * @brief Detect the tag type and connect to it.
 *
 * Resolves the tag type from the discovered target (on NFC-A, from the
 * SEL_RES/SAK) and runs the matching backend's activation.
 *
 * @param poller Open poller session the target was discovered on.
 * @param target Activated target from discovery.
 * @param tag Out: connected tag, valid until the field is lost.
 * @param timeout Overall deadline for the activation.
 *
 * @retval 0 on success.
 * @retval -EINVAL if an argument is NULL.
 * @retval -ENOTSUP if the target is not a supported NDEF tag type.
 * @retval -EBADMSG if the tag responds unexpectedly.
 */
int nfc_tag_connect(struct nfc_poller *poller, const struct nfc_target *target, struct nfc_tag *tag,
		    k_timeout_t timeout);

/**
 * @brief Read the tag's NDEF message.
 *
 * @param tag Connected tag.
 * @param buf Output buffer for the NDEF message.
 * @param len In: capacity of @p buf. Out: message length.
 * @param timeout Overall deadline for the read.
 *
 * @retval 0 on success.
 * @retval -ENOENT if the tag holds no NDEF message, including one the tag is
 *         formatted for but has never been written.
 * @retval -ENOMEM if @p buf is too small.
 */
int nfc_tag_read_ndef(struct nfc_tag *tag, uint8_t *buf, uint16_t *len, k_timeout_t timeout);

/**
 * @brief Write an NDEF message to the tag.
 *
 * @retval 0 on success.
 * @retval -EACCES if the tag is read-only.
 * @retval -ENOMEM if the message does not fit.
 */
int nfc_tag_write_ndef(struct nfc_tag *tag, const uint8_t *buf, uint16_t len, k_timeout_t timeout);

/**
 * @brief Release a connected tag.
 *
 * Ends the exchange and leaves the tag in the HALT state, so anticollision
 * passes over it and reaches the other targets in the field.
 *
 * @param tag Connected tag. Invalid afterwards.
 * @param timeout Deadline for the release exchange.
 *
 * @retval 0 on success.
 * @retval -EINVAL if @p tag is NULL.
 * @retval -EPERM if the poller session is no longer open.
 */
int nfc_tag_close(struct nfc_tag *tag, k_timeout_t timeout);

/**
 * @brief Get the Type 2 backend of a connected tag.
 *
 * @return Backend state, or NULL if @p tag is not a Type 2 Tag.
 */
static inline struct nfc_t2t_tag *nfc_tag_t2t(struct nfc_tag *tag)
{
	return tag->type == NFC_TAG_TYPE_T2T ? &tag->t2t : NULL;
}

/**
 * @brief Get the ISO-DEP connection of a connected tag.
 *
 * @return Connection, or NULL if @p tag is not reached over ISO-DEP.
 */
static inline struct nfc_iso_dep_tag *nfc_tag_iso_dep(struct nfc_tag *tag)
{
	return tag->type == NFC_TAG_TYPE_T2T ? NULL : &tag->t4t.iso_dep;
}

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NFC_TAG_H_ */
