/*
 * Copyright (c) 2026 RAKwireless Technology Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_NFC_POLLER_H_
#define ZEPHYR_INCLUDE_NFC_POLLER_H_

#include <zephyr/device.h>
#include <zephyr/drivers/nfc.h>
#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief NFC poller session
 * @defgroup nfc_poller NFC poller session
 * @ingroup nfc_subsys
 * @{
 *
 * A poller is the caller-owned handle for one NFC device used in reader mode.
 * It binds the device once, so the layers above take a poller instead of a
 * device, and it owns the session lock: a multi-frame sequence such as reading
 * an NDEF message holds the session for its whole duration, so a second thread
 * cannot interleave frames and break the target's state.
 */

/** @brief An NFC device bound for use as a poller (reader). */
struct nfc_poller {
	/** Bound NFC device. */
	const struct device *dev;

	/** @cond INTERNAL_HIDDEN */
	struct k_mutex lock;
	nfc_proto_t protos;
	uint8_t backend;
	bool started;
	/** @endcond */
};

/**
 * @brief Statically define a poller bound to an NFC device.
 *
 * @param _name Symbol name of the poller.
 * @param _dev Device pointer, for example DEVICE_DT_GET(DT_ALIAS(nfc0)).
 */
#define NFC_POLLER_DEFINE(_name, _dev)                                                             \
	static struct nfc_poller _name = {                                                         \
		.dev = (_dev),                                                                     \
		.lock = Z_MUTEX_INITIALIZER(_name.lock),                                           \
	}

/**
 * @brief Open a poller session.
 *
 * Selects the protocols the device will run and resolves how the device is
 * driven, once, for the rest of the session.
 *
 * @param poller Poller to start.
 * @param protos Protocols to run, see @ref NFC_PROTO_DEFS.
 *
 * @retval 0 on success.
 * @retval -EINVAL if @p poller or its device is NULL, or @p protos is empty.
 * @retval -ENODEV if the bound device is not ready.
 * @retval -EALREADY if the session is already open.
 * @retval -ENOTSUP if the device does not support @p protos as a poller.
 */
int nfc_poller_start(struct nfc_poller *poller, nfc_proto_t protos);

/**
 * @brief Close a poller session.
 *
 * @retval 0 on success.
 * @retval -EINVAL if @p poller is NULL.
 */
int nfc_poller_stop(struct nfc_poller *poller);

/**
 * @brief Discover one target, in whichever technology the session runs.
 *
 * Blocks until a target answers or @p timeout passes. The field is removed
 * between poll cycles, so a target that is still on the antenna is found again
 * on the next one; a reader that acts once per presentation compares
 * nfc_target_uid() against the previous result.
 *
 * The result identifies its own technology, so a caller that only reads NDEF
 * can hand it straight to nfc_tag_connect() without inspecting it.
 *
 * @param poller Open poller session.
 * @param timeout Deadline for the discovery.
 * @param out Out: activated target.
 *
 * @retval 0 on success.
 * @retval -EINVAL if @p out is NULL.
 * @retval -EPERM if the session is not open.
 * @retval -ENOTSUP if no discoverable technology is enabled for the session.
 * @retval -EAGAIN if no target answered before the deadline.
 * @retval -errno as reported by the driver on a transport failure.
 */
int nfc_discover(struct nfc_poller *poller, k_timeout_t timeout, struct nfc_target *out);

/**
 * @brief Let go of a discovered target without connecting to it.
 *
 * Leaves the target in the HALT state, so anticollision passes over it and
 * reaches the other targets in the field. A target reached through
 * nfc_tag_connect() is released by nfc_tag_close() instead, which also ends
 * the tag-type protocol.
 *
 * @param poller Open poller session the target was discovered on.
 * @param target Target from nfc_discover().
 * @param timeout Deadline for the release exchange.
 *
 * @retval 0 on success.
 * @retval -EINVAL if @p target is NULL.
 * @retval -EPERM if the session is not open.
 * @retval -ENOTSUP if the target's technology cannot be released.
 */
int nfc_target_release(struct nfc_poller *poller, const struct nfc_target *target,
		       k_timeout_t timeout);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_NFC_POLLER_H_ */
