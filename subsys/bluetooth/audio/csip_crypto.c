/*
 * Copyright (c) 2020 Bose Corporation
 * Copyright (c) 2021-2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * The static functions in this file operate on Big Endian (BE) as the
 * underlying encryption library is BE as well. Furthermore, the sample data
 * in the CSIS spec is also provided as BE, and logging values as BE will make
 * it easier to compare.
 */
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/autoconf.h>
#include <zephyr/bluetooth/audio/csip.h>
#include <zephyr/bluetooth/crypto.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>

#include "crypto/bt_crypto.h"

#include "common/bt_str.h"

#include "csip_crypto.h"

LOG_MODULE_REGISTER(bt_csip_crypto, CONFIG_BT_CSIP_SET_MEMBER_CRYPTO_LOG_LEVEL);

#define BT_CSIP_CRYPTO_PADDING_SIZE 13
#define BT_CSIP_PADDED_RAND_SIZE    (BT_CSIP_CRYPTO_PADDING_SIZE + BT_CSIP_CRYPTO_PRAND_SIZE)
#define BT_CSIP_R_MASK              BIT_MASK(24) /* r is 24 bit / 3 octet */

int bt_csip_sih(const uint8_t sirk[BT_CSIP_SIRK_SIZE], uint8_t r[BT_CSIP_CRYPTO_PRAND_SIZE],
		uint8_t out[BT_CSIP_CRYPTO_HASH_SIZE])
{
	uint8_t res[BT_CSIP_PADDED_RAND_SIZE]; /* need to store 128 bit */
	int err;

	if ((r[BT_CSIP_CRYPTO_PRAND_SIZE - 1] & BIT(7)) ||
	   ((r[BT_CSIP_CRYPTO_PRAND_SIZE - 1] & BIT(6)) == 0)) {
		LOG_DBG("Invalid r %s", bt_hex(r, BT_CSIP_CRYPTO_PRAND_SIZE));
	}

	LOG_DBG("SIRK %s", bt_hex(sirk, BT_CSIP_SIRK_SIZE));
	LOG_DBG("r %s", bt_hex(r, BT_CSIP_CRYPTO_PRAND_SIZE));

	/* r' = padding || r */
	(void)memset(res + BT_CSIP_CRYPTO_PRAND_SIZE, 0, BT_CSIP_CRYPTO_PADDING_SIZE);
	memcpy(res, r, BT_CSIP_CRYPTO_PRAND_SIZE);

	LOG_DBG("r' %s", bt_hex(res, sizeof(res)));

	err = bt_encrypt_le(sirk, res, res);

	if (err != 0) {
		return err;
	}

	/* The output of the function sih is:
	 *      sih(k, r) = e(k, r') mod 2^24
	 * The output of the security function e is then truncated to 24 bits
	 * by taking the least significant 24 bits of the output of e as the
	 * result of sih.
	 */

	LOG_DBG("res %s", bt_hex(res, sizeof(res)));

	/* Result is the lowest 3 bytes */
	memcpy(out, res, BT_CSIP_CRYPTO_HASH_SIZE);

	LOG_DBG("sih %s", bt_hex(out, BT_CSIP_CRYPTO_HASH_SIZE));

	return 0;
}

/**
 * @brief k1 derivation function
 *
 * The key derivation function k1 is used to derive a key. The derived key is
 * used to encrypt and decrypt the value of the Set Identity Resolving Key
 * characteristic.
 *
 * @param n      n is 0 or more bytes.
 * @param n_size Number of bytes in @p n.
 * @param salt   A 16-byte salt.
 * @param p      p is 0 or more bytes.
 * @param p_size Number of bytes in @p p.
 * @param out    A 16-byte output buffer.
 * @return int 0 on success, any other value indicates a failure.
 */
static int k1(const uint8_t *n, size_t n_size,
	      const uint8_t salt[BT_CSIP_CRYPTO_SALT_SIZE],
	      const uint8_t *p, size_t p_size, uint8_t out[16])
{
	/* TODO: This is basically a duplicate of bt_mesh_k1 - Perhaps they can
	 * be merged
	 */
	uint8_t t[16];
	int err;

	/*
	 * T = AES_CMAC_SALT(N)
	 *
	 * k1(N, SALT, P) = AES-CMAC_T(P)
	 */

	LOG_DBG("BE: n %s", bt_hex(n, n_size));
	LOG_DBG("BE: salt %s", bt_hex(salt, BT_CSIP_CRYPTO_SALT_SIZE));
	LOG_DBG("BE: p %s", bt_hex(p, p_size));

	err = bt_crypto_aes_cmac(salt, n, n_size, t);

	LOG_DBG("BE: t %s", bt_hex(t, sizeof(t)));

	if (err) {
		return err;
	}

	err = bt_crypto_aes_cmac(t, p, p_size, out);

	LOG_DBG("BE: out %s", bt_hex(out, 16));

	return err;
}

/**
 * @brief s1 SALT generation function
 *
 * @param m      A non-zero length octet array or ASCII encoded string
 * @param m_size Size of @p m.
 * @param out    16-byte output buffer.
 * @return int 0 on success, any other value indicates a failure.
 */
static int s1(const uint8_t *m, size_t m_size,
	      uint8_t out[BT_CSIP_CRYPTO_SALT_SIZE])
{
	uint8_t zero[16];
	int err;

	/*
	 * s1(M) = AES-CMAC_zero(M)
	 */

	LOG_DBG("BE: m %s", bt_hex(m, m_size));

	memset(zero, 0, sizeof(zero));

	err = bt_crypto_aes_cmac(zero, m, m_size, out);

	LOG_DBG("BE: out %s", bt_hex(out, 16));

	return err;
}

int bt_csip_sef(const uint8_t k[BT_CSIP_CRYPTO_KEY_SIZE], const uint8_t sirk[BT_CSIP_SIRK_SIZE],
		uint8_t out_sirk[BT_CSIP_SIRK_SIZE])
{
	const uint8_t m[] = {'S', 'I', 'R', 'K', 'e', 'n', 'c'};
	const uint8_t p[] = {'c', 's', 'i', 's'};
	uint8_t s1_out[BT_CSIP_CRYPTO_SALT_SIZE];
	uint8_t k1_out[BT_CSIP_CRYPTO_KEY_SIZE];
	uint8_t k1_tmp[BT_CSIP_CRYPTO_KEY_SIZE];
	int err;

	/*
	 * sef(K, SIRK) = k1(K, s1("SIRKenc"), "csis") ^ SIRK
	 */

	LOG_DBG("SIRK %s", bt_hex(sirk, BT_CSIP_SIRK_SIZE));

	/* Swap because aes_cmac is big endian and k is little endian */
	sys_memcpy_swap(k1_tmp, k, sizeof(k1_tmp));
	LOG_DBG("BE: k %s", bt_hex(k1_tmp, sizeof(k1_tmp)));

	err = s1(m, sizeof(m), s1_out);
	if (err) {
		return err;
	}

	LOG_DBG("BE: s1 result %s", bt_hex(s1_out, sizeof(s1_out)));

	err = k1(k1_tmp, sizeof(k1_tmp), s1_out, p, sizeof(p), k1_out);
	if (err) {
		return err;
	}

	LOG_DBG("BE: k1 result %s", bt_hex(k1_out, sizeof(k1_out)));

	/* Get result back to little endian. */
	sys_mem_swap(k1_out, sizeof(k1_out));

	mem_xor_128(out_sirk, k1_out, sirk);
	LOG_DBG("out %s", bt_hex(out_sirk, BT_CSIP_SIRK_SIZE));

	return 0;
}

int bt_csip_sdf(const uint8_t k[BT_CSIP_CRYPTO_KEY_SIZE], const uint8_t enc_sirk[BT_CSIP_SIRK_SIZE],
		uint8_t out_sirk[BT_CSIP_SIRK_SIZE])
{
	/* SIRK encryption is currently symmetric, which means that we can
	 * simply apply the sef function to decrypt it.
	 */

	/*
	 * sdf(K, EncSIRK) = k1(K, s1("SIRKenc"), "csis") ^ EncSIRK
	 */

	LOG_DBG("Running SDF as SEF");
	return bt_csip_sef(k, enc_sirk, out_sirk);
}
