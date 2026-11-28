/*
 * SPDX-FileCopyrightText: FoBE Studio
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_NET_TAILSCALE_DISCO_BOX_H_
#define ZEPHYR_NET_TAILSCALE_DISCO_BOX_H_

#include <stddef.h>
#include <stdint.h>

#define DISCO_BOX_KEY_SIZE   32
#define DISCO_BOX_NONCE_SIZE 24
#define DISCO_BOX_TAG_SIZE   16

/* Derive the NaCl box key from the already validated X25519 shared secret. */
void disco_box_precompute(uint8_t key[DISCO_BOX_KEY_SIZE],
			  const uint8_t shared[DISCO_BOX_KEY_SIZE]);

/*
 * The box contains a 16-byte tag followed by ciphertext. Callers supply
 * nonoverlapping buffers and space for box_len - DISCO_BOX_TAG_SIZE bytes of
 * plaintext. Returns zero on success or negative errno without releasing
 * plaintext.
 */
int disco_box_open(const uint8_t key[DISCO_BOX_KEY_SIZE],
		   const uint8_t nonce[DISCO_BOX_NONCE_SIZE], const uint8_t *box, size_t box_len,
		   uint8_t *plaintext);

/*
 * Callers supply nonoverlapping buffers and space for len + DISCO_BOX_TAG_SIZE
 * bytes of output. A nonce must be used at most once with each key. Returns
 * zero on success or negative errno.
 */
int disco_box_seal(const uint8_t key[DISCO_BOX_KEY_SIZE],
		   const uint8_t nonce[DISCO_BOX_NONCE_SIZE], const uint8_t *plaintext, size_t len,
		   uint8_t *box);

#endif /* ZEPHYR_NET_TAILSCALE_DISCO_BOX_H_ */
