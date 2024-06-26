/* Copyright (c) 2022 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/sys/byteorder.h>

#if defined(CONFIG_BT_USE_PSA_API)
#include "psa/crypto.h"
#else
#include <tinycrypt/cmac_mode.h>
#include <tinycrypt/constants.h>
#endif

#include "common/bt_str.h"
#include "bt_crypto.h"

#define LOG_LEVEL CONFIG_BT_CRYPTO_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_crypto);

int bt_crypto_f4(const uint8_t *u, const uint8_t *v, const uint8_t *x, uint8_t z, uint8_t res[16])
{
	uint8_t xs[16];
	uint8_t m[65];
	int err;

	LOG_DBG("u %s", bt_hex(u, 32));
	LOG_DBG("v %s", bt_hex(v, 32));
	LOG_DBG("x %s z 0x%x", bt_hex(x, 16), z);

	/*
	 * U, V and Z are concatenated and used as input m to the function
	 * AES-CMAC and X is used as the key k.
	 *
	 * Core Spec 4.2 Vol 3 Part H 2.2.5
	 *
	 * note:
	 * bt_smp_aes_cmac uses BE data and smp_f4 accept LE so we swap
	 */
	sys_memcpy_swap(m, u, 32);
	sys_memcpy_swap(m + 32, v, 32);
	m[64] = z;

	sys_memcpy_swap(xs, x, 16);

	err = bt_crypto_aes_cmac(xs, m, sizeof(m), res);
	if (err) {
		return err;
	}

	sys_mem_swap(res, 16);

	LOG_DBG("res %s", bt_hex(res, 16));

	return err;
}

int bt_crypto_f5(const uint8_t *w, const uint8_t *n1, const uint8_t *n2, const bt_addr_le_t *a1,
		 const bt_addr_le_t *a2, uint8_t *mackey, uint8_t *ltk)
{
	static const uint8_t salt[16] = {0x6c, 0x88, 0x83, 0x91, 0xaa, 0xf5, 0xa5, 0x38,
					 0x60, 0x37, 0x0b, 0xdb, 0x5a, 0x60, 0x83, 0xbe};
	uint8_t m[53] = {0x00,						 /* counter */
			 0x62, 0x74, 0x6c, 0x65,			 /* keyID */
			 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /*n1*/
			 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /*2*/
			 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
			 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* a1 */
			 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* a2 */
			 0x01, 0x00 /* length */};
	uint8_t t[16], ws[32];
	int err;

	LOG_DBG("w %s", bt_hex(w, 32));
	LOG_DBG("n1 %s", bt_hex(n1, 16));
	LOG_DBG("n2 %s", bt_hex(n2, 16));

	sys_memcpy_swap(ws, w, 32);

	err = bt_crypto_aes_cmac(salt, ws, 32, t);
	if (err) {
		return err;
	}

	LOG_DBG("t %s", bt_hex(t, 16));

	sys_memcpy_swap(m + 5, n1, 16);
	sys_memcpy_swap(m + 21, n2, 16);
	m[37] = a1->type;
	sys_memcpy_swap(m + 38, a1->a.val, 6);
	m[44] = a2->type;
	sys_memcpy_swap(m + 45, a2->a.val, 6);

	err = bt_crypto_aes_cmac(t, m, sizeof(m), mackey);
	if (err) {
		return err;
	}

	LOG_DBG("mackey %1s", bt_hex(mackey, 16));

	sys_mem_swap(mackey, 16);

	/* counter for ltk is 1 */
	m[0] = 0x01;

	err = bt_crypto_aes_cmac(t, m, sizeof(m), ltk);
	if (err) {
		return err;
	}

	LOG_DBG("ltk %s", bt_hex(ltk, 16));

	sys_mem_swap(ltk, 16);

	return 0;
}

int bt_crypto_f6(const uint8_t *w, const uint8_t *n1, const uint8_t *n2, const uint8_t *r,
		 const uint8_t *iocap, const bt_addr_le_t *a1, const bt_addr_le_t *a2,
		 uint8_t *check)
{
	uint8_t ws[16];
	uint8_t m[65];
	int err;

	LOG_DBG("w %s", bt_hex(w, 16));
	LOG_DBG("n1 %s", bt_hex(n1, 16));
	LOG_DBG("n2 %s", bt_hex(n2, 16));
	LOG_DBG("r %s", bt_hex(r, 16));
	LOG_DBG("io_cap %s", bt_hex(iocap, 3));
	LOG_DBG("a1 %s", bt_hex(a1, 7));
	LOG_DBG("a2 %s", bt_hex(a2, 7));

	sys_memcpy_swap(m, n1, 16);
	sys_memcpy_swap(m + 16, n2, 16);
	sys_memcpy_swap(m + 32, r, 16);
	sys_memcpy_swap(m + 48, iocap, 3);

	m[51] = a1->type;
	memcpy(m + 52, a1->a.val, 6);
	sys_memcpy_swap(m + 52, a1->a.val, 6);

	m[58] = a2->type;
	memcpy(m + 59, a2->a.val, 6);
	sys_memcpy_swap(m + 59, a2->a.val, 6);

	sys_memcpy_swap(ws, w, 16);

	err = bt_crypto_aes_cmac(ws, m, sizeof(m), check);
	if (err) {
		return err;
	}

	LOG_DBG("res %s", bt_hex(check, 16));

	sys_mem_swap(check, 16);

	return 0;
}

int bt_crypto_g2(const uint8_t u[32], const uint8_t v[32], const uint8_t x[16], const uint8_t y[16],
		 uint32_t *passkey)
{
	uint8_t m[80], xs[16];
	int err;

	LOG_DBG("u %s", bt_hex(u, 32));
	LOG_DBG("v %s", bt_hex(v, 32));
	LOG_DBG("x %s", bt_hex(x, 16));
	LOG_DBG("y %s", bt_hex(y, 16));

	sys_memcpy_swap(m, u, 32);
	sys_memcpy_swap(m + 32, v, 32);
	sys_memcpy_swap(m + 64, y, 16);

	sys_memcpy_swap(xs, x, 16);

	/* reuse xs (key) as buffer for result */
	err = bt_crypto_aes_cmac(xs, m, sizeof(m), xs);
	if (err) {
		return err;
	}
	LOG_DBG("res %s", bt_hex(xs, 16));

	memcpy(passkey, xs + 12, 4);
	*passkey = sys_be32_to_cpu(*passkey) % 1000000;

	LOG_DBG("passkey %u", *passkey);

	return 0;
}

int bt_crypto_h6(const uint8_t w[16], const uint8_t key_id[4], uint8_t res[16])
{
	uint8_t ws[16];
	uint8_t key_id_s[4];
	int err;

	LOG_DBG("w %s", bt_hex(w, 16));
	LOG_DBG("key_id %s", bt_hex(key_id, 4));

	sys_memcpy_swap(ws, w, 16);
	sys_memcpy_swap(key_id_s, key_id, 4);

	err = bt_crypto_aes_cmac(ws, key_id_s, 4, res);
	if (err) {
		return err;
	}

	LOG_DBG("res %s", bt_hex(res, 16));

	sys_mem_swap(res, 16);

	return 0;
}

int bt_crypto_h7(const uint8_t salt[16], const uint8_t w[16], uint8_t res[16])
{
	uint8_t ws[16];
	uint8_t salt_s[16];
	int err;

	LOG_DBG("w %s", bt_hex(w, 16));
	LOG_DBG("salt %s", bt_hex(salt, 16));

	sys_memcpy_swap(ws, w, 16);
	sys_memcpy_swap(salt_s, salt, 16);

	err = bt_crypto_aes_cmac(salt_s, ws, 16, res);
	if (err) {
		return err;
	}

	LOG_DBG("res %s", bt_hex(res, 16));

	sys_mem_swap(res, 16);

	return 0;
}

int bt_crypto_h8(const uint8_t k[16], const uint8_t s[16], const uint8_t key_id[4], uint8_t res[16])
{
	uint8_t key_id_s[4];
	uint8_t iks[16];
	uint8_t ks[16];
	uint8_t ss[16];
	int err;

	LOG_DBG("k %s", bt_hex(k, 16));
	LOG_DBG("s %s", bt_hex(s, 16));
	LOG_DBG("key_id %s", bt_hex(key_id, 4));

	sys_memcpy_swap(ks, k, 16);
	sys_memcpy_swap(ss, s, 16);

	err = bt_crypto_aes_cmac(ss, ks, 16, iks);
	if (err) {
		return err;
	}

	sys_memcpy_swap(key_id_s, key_id, 4);

	err = bt_crypto_aes_cmac(iks, key_id_s, 4, res);
	if (err) {
		return err;
	}

	LOG_DBG("res %s", bt_hex(res, 16));

	sys_mem_swap(res, 16);

	return 0;
}
