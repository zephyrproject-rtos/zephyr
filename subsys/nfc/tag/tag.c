/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/nfc/tag.h>
#include <zephyr/sys/util.h>

#include "common/poller.h"
#include "protocol/iso14443a.h"
#include "tag/tag_internal.h"

LOG_MODULE_REGISTER(nfc_tag, CONFIG_NFC_LOG_LEVEL);

/* NFC-A SEL_RES (SAK) bits, ISO/IEC 14443-3. */
#define NFCA_SAK_MIFARE_CLASSIC BIT(3)
#define NFCA_SAK_ISO_DEP        BIT(5)

int z_nfc_tag_detect(const struct nfc_target *target, enum nfc_tag_type *out)
{
	uint8_t sak;

	if (target->tech != NFC_TECH_A) {
		return -ENOTSUP;
	}

	sak = target->a.sak;

	if ((sak & NFCA_SAK_MIFARE_CLASSIC) != 0U && (sak & NFCA_SAK_ISO_DEP) == 0U) {
		LOG_DBG("SAK %02x is a MIFARE Classic, not an NFC Forum tag", sak);
		return -ENOTSUP;
	}

	*out = (sak & NFCA_SAK_ISO_DEP) != 0U ? NFC_TAG_TYPE_T4T_A : NFC_TAG_TYPE_T2T;

	LOG_DBG("SAK %02x resolved to type %d", sak, (int)*out);

	return 0;
}

static struct nfc_poller *tag_poller(struct nfc_tag *tag)
{
	return tag->type == NFC_TAG_TYPE_T2T ? tag->t2t.poller : tag->t4t.iso_dep.poller;
}

/* A chain, not a switch: an excluded backend's call goes with its branch. */
static inline bool tag_is_t2t(const struct nfc_tag *tag)
{
	return IS_ENABLED(CONFIG_NFC_T2T) && tag->type == NFC_TAG_TYPE_T2T;
}

static inline bool tag_is_t4t(const struct nfc_tag *tag)
{
	return IS_ENABLED(CONFIG_NFC_T4T) &&
	       (tag->type == NFC_TAG_TYPE_T4T_A || tag->type == NFC_TAG_TYPE_T4T_B);
}

int nfc_tag_connect(struct nfc_poller *poller, const struct nfc_target *target, struct nfc_tag *tag,
		    k_timeout_t timeout)
{
	int ret;

	if (target == NULL || tag == NULL) {
		return -EINVAL;
	}

	if (!z_nfc_poller_ready(poller)) {
		return -EPERM;
	}

	memset(tag, 0, sizeof(*tag));

	ret = z_nfc_tag_detect(target, &tag->type);
	if (ret < 0) {
		return ret;
	}

	z_nfc_poller_lock(poller);

	if (tag_is_t2t(tag)) {
		ret = z_nfc_t2t_connect(poller, target, &tag->t2t, timeout);
	} else if (tag_is_t4t(tag)) {
		ret = z_nfc_t4t_connect(poller, target, &tag->t4t, timeout);
	} else {
		ret = -ENOTSUP;
	}

	z_nfc_poller_unlock(poller);

	return ret;
}

int nfc_tag_read_ndef(struct nfc_tag *tag, uint8_t *buf, uint16_t *len, k_timeout_t timeout)
{
	struct nfc_poller *poller;
	int ret;

	if (tag == NULL) {
		return -EINVAL;
	}

	poller = tag_poller(tag);
	if (!z_nfc_poller_ready(poller)) {
		return -EPERM;
	}

	z_nfc_poller_lock(poller);

	if (tag_is_t2t(tag)) {
		ret = z_nfc_t2t_read_ndef(&tag->t2t, buf, len, timeout);
	} else if (tag_is_t4t(tag)) {
		ret = z_nfc_t4t_read_ndef(&tag->t4t, buf, len, timeout);
	} else {
		ret = -EINVAL;
	}

	z_nfc_poller_unlock(poller);

	return ret;
}

static const struct nfc_target *tag_target(const struct nfc_tag *tag)
{
	if (tag_is_t2t(tag)) {
		return &tag->t2t.target;
	}
	if (tag_is_t4t(tag)) {
		return &tag->t4t.iso_dep.target;
	}

	return NULL;
}

static int tag_close_frontend(struct nfc_tag *tag, struct nfc_poller *poller, k_timeout_t timeout)
{
	if (tag_is_t2t(tag)) {
		return z_nfca_halt(poller, &tag->t2t.target, timeout);
	}
	if (tag_is_t4t(tag)) {
		/* DESELECT ends layer 4 and halts the target in one exchange. */
		return z_nfc_iso_dep_deselect(&tag->t4t.iso_dep, timeout);
	}

	return -EINVAL;
}

static int tag_close_offload(struct nfc_tag *tag, struct nfc_poller *poller)
{
	const struct nfc_target *target = tag_target(tag);

	if (target == NULL) {
		return -EINVAL;
	}

	return nfc_offload_release(poller->dev, target);
}

int nfc_tag_close(struct nfc_tag *tag, k_timeout_t timeout)
{
	struct nfc_poller *poller;
	int ret;

	if (tag == NULL) {
		return -EINVAL;
	}

	poller = tag_poller(tag);
	if (!z_nfc_poller_ready(poller)) {
		return -EPERM;
	}

	z_nfc_poller_lock(poller);

	if (z_nfc_poller_backend(poller) == Z_NFC_BACKEND_OFFLOAD) {
		ret = tag_close_offload(tag, poller);
	} else {
		ret = tag_close_frontend(tag, poller, timeout);
	}

	z_nfc_poller_unlock(poller);

	memset(tag, 0, sizeof(*tag));

	return ret;
}

int nfc_tag_write_ndef(struct nfc_tag *tag, const uint8_t *buf, uint16_t len, k_timeout_t timeout)
{
	struct nfc_poller *poller;
	int ret;

	if (tag == NULL) {
		return -EINVAL;
	}

	poller = tag_poller(tag);
	if (!z_nfc_poller_ready(poller)) {
		return -EPERM;
	}

	z_nfc_poller_lock(poller);

	if (tag_is_t2t(tag)) {
		ret = z_nfc_t2t_write_ndef(&tag->t2t, buf, len, timeout);
	} else if (tag_is_t4t(tag)) {
		ret = z_nfc_t4t_write_ndef(&tag->t4t, buf, len, timeout);
	} else {
		ret = -EINVAL;
	}

	z_nfc_poller_unlock(poller);

	return ret;
}
