/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_NFC_COMMON_POLLER_H_
#define ZEPHYR_SUBSYS_NFC_COMMON_POLLER_H_

#include <zephyr/drivers/nfc.h>
#include <zephyr/kernel.h>
#include <zephyr/nfc/poller.h>

#include "common/backend.h"

/*
 * Taken by the outermost public entry point only, so that a whole protocol
 * sequence is one critical section. The mutex is recursive for the owning
 * thread, so an inner entry point reached from an outer one is harmless. The
 * driver claim is held for the same span, which also excludes a caller using
 * the driver API directly.
 */
static inline void z_nfc_poller_lock(struct nfc_poller *poller)
{
	k_mutex_lock(&poller->lock, K_FOREVER);
	(void)nfc_claim(poller->dev);
}

static inline void z_nfc_poller_unlock(struct nfc_poller *poller)
{
	(void)nfc_release(poller->dev);
	k_mutex_unlock(&poller->lock);
}

/** @brief Backend resolved once by nfc_poller_start(). */
static inline enum z_nfc_backend_kind z_nfc_poller_backend(const struct nfc_poller *poller)
{
	return (enum z_nfc_backend_kind)poller->backend;
}

static inline bool z_nfc_poller_ready(const struct nfc_poller *poller)
{
	return poller != NULL && poller->started;
}

#endif /* ZEPHYR_SUBSYS_NFC_COMMON_POLLER_H_ */
