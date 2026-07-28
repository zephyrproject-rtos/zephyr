/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/nfc.h>
#include <zephyr/logging/log.h>
#include <zephyr/nfc/poller.h>

#include "common/poller.h"
#include "protocol/iso14443a.h"

LOG_MODULE_REGISTER(nfc_poller, CONFIG_NFC_LOG_LEVEL);

static int poller_resolve(struct nfc_poller *poller, nfc_proto_t protos,
			  enum z_nfc_backend_kind *backend)
{
	if (poller == NULL || poller->dev == NULL || protos == 0U) {
		return -EINVAL;
	}

	if (!device_is_ready(poller->dev)) {
		return -ENODEV;
	}

	if ((protos & nfc_supported_protocols(poller->dev)) != protos) {
		return -ENOTSUP;
	}

	*backend = z_nfc_backend_kind_get(poller->dev, protos);
	if (*backend != Z_NFC_BACKEND_OFFLOAD && *backend != Z_NFC_BACKEND_FRONTEND_INITIATOR) {
		return -ENOTSUP;
	}

	return 0;
}

int nfc_poller_start(struct nfc_poller *poller, nfc_proto_t protos)
{
	enum z_nfc_backend_kind backend;
	int ret;

	ret = poller_resolve(poller, protos, &backend);
	if (ret < 0) {
		return ret;
	}

	k_mutex_lock(&poller->lock, K_FOREVER);

	if (poller->started) {
		k_mutex_unlock(&poller->lock);
		return -EALREADY;
	}

	(void)nfc_claim(poller->dev);

	/*
	 * An offloading controller configures the technology itself, so it has
	 * no protocol to load.
	 */
	ret = nfc_load_protocol(poller->dev, protos, NFC_MODE_INITIATOR);
	if (ret == -ENOSYS) {
		ret = 0;
	}

	if (ret == 0) {
		poller->protos = protos;
		poller->backend = (uint8_t)backend;
		poller->started = true;
	} else {
		LOG_ERR("%s: cannot load protocol 0x%08x: %d", poller->dev->name, protos, ret);
	}

	(void)nfc_release(poller->dev);
	k_mutex_unlock(&poller->lock);

	return ret;
}

int nfc_poller_stop(struct nfc_poller *poller)
{
	if (poller == NULL) {
		return -EINVAL;
	}

	k_mutex_lock(&poller->lock, K_FOREVER);
	poller->started = false;
	k_mutex_unlock(&poller->lock);

	return 0;
}

int nfc_discover(struct nfc_poller *poller, k_timeout_t timeout, struct nfc_target *out)
{
	if (out == NULL) {
		return -EINVAL;
	}

	if (!z_nfc_poller_ready(poller)) {
		return -EPERM;
	}

	if (IS_ENABLED(CONFIG_NFC_NFCA) && (poller->protos & NFC_PROTO_ISO14443A) != 0U) {
		return z_nfca_discover(poller, timeout, out);
	}

	return -ENOTSUP;
}

int nfc_target_release(struct nfc_poller *poller, const struct nfc_target *target,
		       k_timeout_t timeout)
{
	int ret;

	if (target == NULL) {
		return -EINVAL;
	}

	if (!z_nfc_poller_ready(poller)) {
		return -EPERM;
	}

	if (!IS_ENABLED(CONFIG_NFC_NFCA) || target->tech != NFC_TECH_A) {
		return -ENOTSUP;
	}

	z_nfc_poller_lock(poller);
	if (z_nfc_poller_backend(poller) == Z_NFC_BACKEND_OFFLOAD) {
		ret = nfc_offload_release(poller->dev, target);
	} else {
		ret = z_nfca_halt(poller, target, timeout);
	}
	z_nfc_poller_unlock(poller);

	return ret;
}
