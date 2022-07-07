/*
 * Copyright (c) 2020 Bose Corporation
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * The static functions in this file operate on Big Endian (BE) as the
 * underlying encryption library is BE as well. Furthermore, the sample data
 * in the CSIS spec is also provided as BE, and logging values as BE will make
 * it easier to compare.
 */
#include "csis_crypto.h"
#include <zephyr/bluetooth/crypto.h>
#include <tinycrypt/constants.h>
#include <tinycrypt/utils.h>
#include <tinycrypt/aes.h>
#include <tinycrypt/cmac_mode.h>
#include <tinycrypt/ccm_mode.h>
#include <zephyr/sys/byteorder.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_CSIS_CRYPTO)
#define LOG_MODULE_NAME bt_csis_crypto
#include "common/log.h"

#define BT_CSIS_CRYPTO_PADDING_SIZE 13
#define BT_CSIS_R_SIZE              3 /* r is 24 bit / 3 octet */
#define BT_CSIS_R_MASK              BIT_MASK(24) /* r is 24 bit / 3 octet */

static int aes_cmac(const uint8_t key[BT_CSIS_CRYPTO_KEY_SIZE],
		    const uint8_t *in, size_t in_len, uint8_t *out)
{
	struct tc_aes_key_sched_struct sched;
	struct tc_cmac_struct state;

	/* TODO: Copy of the aes_cmac from smp.c: Can we merge them? */

	if (tc_cmac_setup(&state, key, &sched) == TC_CRYPTO_FAIL) {
		return -EIO;
	}

	if (tc_cmac_update(&state, in, in_len) == TC_CRYPTO_FAIL) {
		return -EIO;
	}

	if (tc_cmac_final(out, &state) == TC_CRYPTO_FAIL) {
		return -EIO;
	}

	return 0;
}

static void xor_128(const uint8_t a[16], const uint8_t b[16], uint8_t out[16])
{
	size_t len = 16;
	/* TODO: Identical to the xor_128 from smp.c: Move to util */

	while (len--) {
		*out++ = *a++ ^ *b++;
	}
}

int bt_csis_sih(const uint8_t sirk[BT_CSIS_SET_SIRK_SIZE], uint32_t r,
		uint32_t *out)
{
	uint8_t res[16]; /* need to store 128 bit */
	int err;
	uint8_t sirk_tmp[BT_CSIS_SET_SIRK_SIZE];

	if ((r & BIT(23)) || ((r & BIT(22)) == 0)) {
		BT_DBG("Invalid r %0x06x", (uint32_t)(r & BT_CSIS_R_MASK));
	}

	BT_DBG("SIRK %s", bt_hex(sirk, BT_CSIS_SET_SIRK_SIZE));
	BT_DBG("r 0x%06x", r);

	/* r' = padding || r */
	(void)memset(res, 0, BT_CSIS_CRYPTO_PADDING_SIZE);
	sys_put_be24(r, res + BT_CSIS_CRYPTO_PADDING_SIZE);

	BT_DBG("BE: r' %s", bt_hex(res, sizeof(res)));

	if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) {
		/* Swap to Big Endian (BE) */
		sys_memcpy_swap(sirk_tmp, sirk, BT_CSIS_SET_SIRK_SIZE);
	} else {
		(void)memcpy(sirk_tmp, sirk, BT_CSIS_SET_SIRK_SIZE);
	}

	err = bt_encrypt_be(sirk_tmp, res, res);

	if (err != 0) {
		return err;
	}

	/* The output of the function sih is:
	 *      sih(k, r) = e(k, r') mod 2^24
	 * The output of the security function e is then truncated to 24 bits
	 * by taking the least significant 24 bits of the output of e as the
	 * result of sih.
	 */

	BT_DBG("BE: res %s", bt_hex(res, sizeof(res)));

	/* Result is the lowest 3 bytes */
	*out = sys_get_be24(res + 13);

	BT_DBG("sih 0x%06x", *out);

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
	      const uint8_t salt[BT_CSIS_CRYPTO_SALT_SIZE],
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

	BT_DBG("BE: n %s", bt_hex(n, n_size));
	BT_DBG("BE: salt %s", bt_hex(salt, BT_CSIS_CRYPTO_SALT_SIZE));
	BT_DBG("BE: p %s", bt_hex(p, p_size));

	err = aes_cmac(salt, n, n_size, t);

	BT_DBG("BE: t %s", bt_hex(t, sizeof(t)));

	if (err) {
		return err;
	}

	err = aes_cmac(t, p, p_size, out);

	BT_DBG("BE: out %s", bt_hex(out, 16));

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
	      uint8_t out[BT_CSIS_CRYPTO_SALT_SIZE])
{
	uint8_t zero[16];
	int err;

	/*
	 * s1(M) = AES-CMAC_zero(M)
	 */

	BT_DBG("BE: m %s", bt_hex(m, m_size));

	memset(zero, 0, sizeof(zero));

	err = aes_cmac(zero, m, m_size, out);

	BT_DBG("BE: out %s", bt_hex(out, 16));

	return err;
}

int bt_csis_sef(const uint8_t k[BT_CSIS_CRYPTO_KEY_SIZE],
		const uint8_t sirk[BT_CSIS_SET_SIRK_SIZE],
		uint8_t out_sirk[BT_CSIS_SET_SIRK_SIZE])
{
	const uint8_t m[] = {'S', 'I', 'R', 'K', 'e', 'n', 'c'};
	const uint8_t p[] = {'c', 's', 'i', 's'};
	uint8_t s1_out[BT_CSIS_CRYPTO_SALT_SIZE];
	uint8_t k1_out[BT_CSIS_CRYPTO_KEY_SIZE];
	uint8_t k1_tmp[BT_CSIS_CRYPTO_KEY_SIZE];
	int err;

	/*
	 * sef(K, SIRK) = k1(K, s1("SIRKenc"), "csis") ^ SIRK
	 */

	BT_DBG("SIRK %s", bt_hex(sirk, BT_CSIS_SET_SIRK_SIZE));

	if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) {
		/* Swap because aes_cmac is big endian
		 * and we are little endian
		 */
		sys_memcpy_swap(k1_tmp, k, sizeof(k1_tmp));
	} else {
		(void)memcpy(k1_tmp, k, sizeof(k1_tmp));
	}
	BT_DBG("BE: k %s", bt_hex(k1_tmp, sizeof(k1_tmp)));

	err = s1(m, sizeof(m), s1_out);
	if (err) {
		return err;
	}

	BT_DBG("BE: s1 result %s", bt_hex(s1_out, sizeof(s1_out)));

	err = k1(k1_tmp, sizeof(k1_tmp), s1_out, p, sizeof(p), k1_out);
	if (err) {
		return err;
	}

	BT_DBG("BE: k1 result %s", bt_hex(k1_out, sizeof(k1_out)));

	if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__) {
		/* Swap result back to little endian */
		sys_mem_swap(k1_out, sizeof(k1_out));
	}

	xor_128(k1_out, sirk, out_sirk);
	BT_DBG("out %s", bt_hex(out_sirk, BT_CSIS_SET_SIRK_SIZE));

	return 0;
}

int bt_csis_sdf(const uint8_t k[BT_CSIS_CRYPTO_KEY_SIZE],
		const uint8_t enc_sirk[BT_CSIS_SET_SIRK_SIZE],
		uint8_t out_sirk[BT_CSIS_SET_SIRK_SIZE])
{
	/* SIRK encryption is currently symmetric, which means that we can
	 * simply apply the sef function to decrypt it.
	 */

	/*
	 * sdf(K, EncSIRK) = k1(K, s1("SIRKenc"), "csis") ^ EncSIRK
	 */

	BT_DBG("Running SDF as SEF");
	return bt_csis_sef(k, enc_sirk, out_sirk);
}
