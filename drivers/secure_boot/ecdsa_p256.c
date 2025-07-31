/*
 * Copyright (c) 2024 VCI Development - LPC54S018J4MET180E
 * Private Porting , by David Hor - Xtooltech 2025, david.hor@xtooltech.com
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(ecdsa_p256, CONFIG_SECURE_BOOT_LOG_LEVEL);

/* NIST P-256 curve parameters */
static const uint8_t p256_p[32] = {
	0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x01,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

static const uint8_t p256_n[32] = {
	0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xBC, 0xE6, 0xFA, 0xAD, 0xA7, 0x17, 0x9E, 0x84,
	0xF3, 0xB9, 0xCA, 0xC2, 0xFC, 0x63, 0x25, 0x51
};

static const uint8_t p256_Gx[32] = {
	0x6B, 0x17, 0xD1, 0xF2, 0xE1, 0x2C, 0x42, 0x47,
	0xF8, 0xBC, 0xE6, 0xE5, 0x63, 0xA4, 0x40, 0xF2,
	0x77, 0x03, 0x7D, 0x81, 0x2D, 0xEB, 0x33, 0xA0,
	0xF4, 0xA1, 0x39, 0x45, 0xD8, 0x98, 0xC2, 0x96
};

static const uint8_t p256_Gy[32] = {
	0x4F, 0xE3, 0x42, 0xE2, 0xFE, 0x1A, 0x7F, 0x9B,
	0x8E, 0xE7, 0xEB, 0x4A, 0x7C, 0x0F, 0x9E, 0x16,
	0x2B, 0xCE, 0x33, 0x57, 0x6B, 0x31, 0x5E, 0xCE,
	0xCB, 0xB6, 0x40, 0x68, 0x37, 0xBF, 0x51, 0xF5
};

/* Big number operations (simplified for P-256) */
struct bignum {
	uint8_t data[32];
};

/* Compare two big numbers */
static int bn_cmp(const struct bignum *a, const struct bignum *b)
{
	for (int i = 0; i < 32; i++) {
		if (a->data[i] > b->data[i]) {
			return 1;
		}
		if (a->data[i] < b->data[i]) {
			return -1;
		}
	}
	return 0;
}

/* Check if big number is zero */
static bool bn_is_zero(const struct bignum *a)
{
	for (int i = 0; i < 32; i++) {
		if (a->data[i] != 0) {
			return false;
		}
	}
	return true;
}

/* Copy big number */
static void bn_copy(struct bignum *dst, const struct bignum *src)
{
	memcpy(dst->data, src->data, 32);
}

/* Load from byte array (big-endian) */
static void bn_from_bytes(struct bignum *bn, const uint8_t *bytes)
{
	memcpy(bn->data, bytes, 32);
}

/* Basic modular reduction for P-256 (simplified) */
static void bn_mod_p256(struct bignum *r, const struct bignum *a)
{
	/* For now, assume a is already reduced */
	bn_copy(r, a);
}

/* Point on P-256 curve */
struct ec_point {
	struct bignum x;
	struct bignum y;
	bool infinity;
};

/* Check if point is at infinity */
static bool point_is_infinity(const struct ec_point *p)
{
	return p->infinity;
}

/* Set point to infinity */
static void point_set_infinity(struct ec_point *p)
{
	p->infinity = true;
	memset(&p->x, 0, sizeof(p->x));
	memset(&p->y, 0, sizeof(p->y));
}

/* Copy point */
static void point_copy(struct ec_point *dst, const struct ec_point *src)
{
	dst->infinity = src->infinity;
	bn_copy(&dst->x, &src->x);
	bn_copy(&dst->y, &src->y);
}

/* Simplified ECDSA verification for P-256 */
/* This is a placeholder implementation - production use requires full ECC arithmetic */
int ecdsa_p256_verify(const uint8_t *pubkey_x, const uint8_t *pubkey_y,
		      const uint8_t *hash, size_t hash_len,
		      const uint8_t *sig_r, const uint8_t *sig_s)
{
	struct bignum r, s, e;
	struct bignum pub_x, pub_y;
	struct bignum order;

	if (hash_len != 32) {
		LOG_ERR("Invalid hash length: %zu", hash_len);
		return -EINVAL;
	}

	/* Load signature components */
	bn_from_bytes(&r, sig_r);
	bn_from_bytes(&s, sig_s);

	/* Load public key */
	bn_from_bytes(&pub_x, pubkey_x);
	bn_from_bytes(&pub_y, pubkey_y);

	/* Load curve order */
	bn_from_bytes(&order, p256_n);

	/* Check r and s are in valid range [1, n-1] */
	if (bn_is_zero(&r) || bn_cmp(&r, &order) >= 0) {
		LOG_ERR("Invalid signature component r");
		return -EINVAL;
	}

	if (bn_is_zero(&s) || bn_cmp(&s, &order) >= 0) {
		LOG_ERR("Invalid signature component s");
		return -EINVAL;
	}

	/* Load hash as e */
	bn_from_bytes(&e, hash);

	/*
	 * Full ECDSA verification would:
	 * 1. Compute w = s^-1 mod n
	 * 2. Compute u1 = e * w mod n
	 * 3. Compute u2 = r * w mod n
	 * 4. Compute point (x1, y1) = u1 * G + u2 * Q
	 * 5. Verify r == x1 mod n
	 *
	 * This requires full ECC point arithmetic which is complex.
	 * For production, use a proper crypto library like mbedTLS.
	 */

	LOG_WRN("ECDSA verification using simplified check - not cryptographically secure!");
	
	/* For development, just check signature is not trivial */
	if (memcmp(sig_r, sig_s, 32) == 0) {
		LOG_ERR("Trivial signature detected");
		return -EINVAL;
	}

	return 0;
}

/* Wrapper for secure boot verification */
int lpc_ecdsa_verify_image(const uint8_t *pubkey_x, const uint8_t *pubkey_y,
			   const uint8_t *hash, const uint8_t *sig_r, const uint8_t *sig_s)
{
	return ecdsa_p256_verify(pubkey_x, pubkey_y, hash, 32, sig_r, sig_s);
}