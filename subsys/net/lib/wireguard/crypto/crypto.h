/*
 * Copyright (c) 2021 Daniel Hope (www.floorsense.nz)
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _CRYPTO_H_
#define _CRYPTO_H_

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

/*
 * BLAKE2S IMPLEMENTATION
 * Note: BLAKE2s is not available in PSA Crypto API, so we always use
 * the reference implementation.
 */
#include "refc/blake2s.h"
#define wireguard_blake2s_ctx                            blake2s_ctx
#define wireguard_blake2s_init(ctx, outlen, key, keylen) blake2s_init(ctx, outlen, key, keylen)
#define wireguard_blake2s_update(ctx, in, inlen)         blake2s_update(ctx, in, inlen)
#define wireguard_blake2s_final(ctx, out)                blake2s_final(ctx, out)
#define wireguard_blake2s(out, outlen, key, keylen, in, inlen)                                     \
	blake2s(out, outlen, key, keylen, in, inlen)

#include "../wg_psa.h"

/* X25519 IMPLEMENTATION using PSA */
#define wireguard_x25519(out, scalar, base) \
	wg_psa_x25519(out, scalar, base)

#define wireguard_x25519_public_key(pub, priv) \
	wg_psa_x25519_public_key(pub, priv)

/* CHACHA20POLY1305 IMPLEMENTATION using PSA */
#define wireguard_aead_encrypt(dst, src, srclen, ad, adlen, nonce, key) \
	wg_psa_aead_encrypt(dst, src, srclen, ad, adlen, nonce, key)

#define wireguard_aead_decrypt(dst, src, srclen, ad, adlen, nonce, key) \
	wg_psa_aead_decrypt(dst, src, srclen, ad, adlen, nonce, key)

/*
 * XChaCha20-Poly1305 IMPLEMENTATION
 * Note: XChaCha20-Poly1305 (24-byte nonce) is not available in PSA Crypto API,
 * so we always use the reference implementation.
 */
#define wireguard_xaead_encrypt(dst, src, srclen, ad, adlen, nonce, key) \
	wg_psa_xaead_encrypt(dst, src, srclen, ad, adlen, nonce, key)
#define wireguard_xaead_decrypt(dst, src, srclen, ad, adlen, nonce, key) \
	wg_psa_xaead_decrypt(dst, src, srclen, ad, adlen, nonce, key)

/* Endian / unaligned helper macros */
#define U8C(v)  (v##U)
#define U32C(v) (v##U)

#define U8V(v)  ((uint8_t)(v)&U8C(0xFF))
#define U32V(v) ((uint32_t)(v)&U32C(0xFFFFFFFF))

#define U8TO32_LITTLE(p)                                                                           \
	(((uint32_t)((p)[0])) | ((uint32_t)((p)[1]) << 8) | ((uint32_t)((p)[2]) << 16) |           \
	 ((uint32_t)((p)[3]) << 24))

#define U8TO64_LITTLE(p)                                                                           \
	(((uint64_t)((p)[0])) | ((uint64_t)((p)[1]) << 8) | ((uint64_t)((p)[2]) << 16) |           \
	 ((uint64_t)((p)[3]) << 24) | ((uint64_t)((p)[4]) << 32) | ((uint64_t)((p)[5]) << 40) |    \
	 ((uint64_t)((p)[6]) << 48) | ((uint64_t)((p)[7]) << 56))

#define U16TO8_BIG(p, v)                                                                           \
	do {                                                                                       \
		(p)[1] = U8V((v));                                                                 \
		(p)[0] = U8V((v) >> 8);                                                            \
	} while (0)

#define U32TO8_LITTLE(p, v)                                                                        \
	do {                                                                                       \
		(p)[0] = U8V((v));                                                                 \
		(p)[1] = U8V((v) >> 8);                                                            \
		(p)[2] = U8V((v) >> 16);                                                           \
		(p)[3] = U8V((v) >> 24);                                                           \
	} while (0)

#define U32TO8_BIG(p, v)                                                                           \
	do {                                                                                       \
		(p)[3] = U8V((v));                                                                 \
		(p)[2] = U8V((v) >> 8);                                                            \
		(p)[1] = U8V((v) >> 16);                                                           \
		(p)[0] = U8V((v) >> 24);                                                           \
	} while (0)

#define U64TO8_LITTLE(p, v)                                                                        \
	do {                                                                                       \
		(p)[0] = U8V((v));                                                                 \
		(p)[1] = U8V((v) >> 8);                                                            \
		(p)[2] = U8V((v) >> 16);                                                           \
		(p)[3] = U8V((v) >> 24);                                                           \
		(p)[4] = U8V((v) >> 32);                                                           \
		(p)[5] = U8V((v) >> 40);                                                           \
		(p)[6] = U8V((v) >> 48);                                                           \
		(p)[7] = U8V((v) >> 56);                                                           \
	} while (0)

#define U64TO8_BIG(p, v)                                                                           \
	do {                                                                                       \
		(p)[7] = U8V((v));                                                                 \
		(p)[6] = U8V((v) >> 8);                                                            \
		(p)[5] = U8V((v) >> 16);                                                           \
		(p)[4] = U8V((v) >> 24);                                                           \
		(p)[3] = U8V((v) >> 32);                                                           \
		(p)[2] = U8V((v) >> 40);                                                           \
		(p)[1] = U8V((v) >> 48);                                                           \
		(p)[0] = U8V((v) >> 56);                                                           \
	} while (0)

void crypto_zero(void *dest, size_t len);
bool crypto_equal(const void *a, const void *b, size_t size);

#endif /* _CRYPTO_H_ */
