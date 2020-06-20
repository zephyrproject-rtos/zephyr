/* ecc.h - ECDH helpers */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*  @brief Container for public key callback */
struct bt_pub_key_cb {
	/** @brief Callback type for Public Key generation.
	 *
	 *  Used to notify of the local public key or that the local key is not
	 *  available (either because of a failure to read it or because it is
	 *  being regenerated).
	 *
	 *  @param key The local public key, or NULL in case of no key.
	 */
	void (*func)(const uint8_t key[64]);

	struct bt_pub_key_cb *_next;
};

/*  @brief Generate a new Public Key.
 *
 *  Generate a new ECC Public Key. The callback will persist even after the
 *  key has been generated, and will be used to notify of new generation
 *  processes (NULL as key).
 *
 *  @param cb Callback to notify the new key, or NULL to request an update
 *            without registering any new callback.
 *
 *  @return Zero on success or negative error code otherwise
 */
int bt_pub_key_gen(struct bt_pub_key_cb *cb);

/*  @brief Get the current Public Key.
 *
 *  Get the current ECC Public Key.
 *
 *  @return Current key, or NULL if not available.
 */
const uint8_t *bt_pub_key_get(void);

/*  @typedef bt_dh_key_cb_t
 *  @brief Callback type for DH Key calculation.
 *
 *  Used to notify of the calculated DH Key.
 *
 *  @param key The DH Key, or NULL in case of failure.
 */
typedef void (*bt_dh_key_cb_t)(const uint8_t key[32]);

/*  @brief Calculate a DH Key from a remote Public Key.
 *
 *  Calculate a DH Key from the remote Public Key.
 *
 *  @param remote_pk Remote Public Key.
 *  @param cb Callback to notify the calculated key.
 *
 *  @return Zero on success or negative error code otherwise
 */
int bt_dh_key_gen(const uint8_t remote_pk[64], bt_dh_key_cb_t cb);
