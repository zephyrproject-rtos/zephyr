/**
 * @file rpa.c
 * Resolvable Private Address Generation and Resolution
 */

/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <stddef.h>
#include <errno.h>
#include <string.h>

#include "common/bt_str.h"

#include <zephyr/bluetooth/crypto.h>

#define LOG_LEVEL CONFIG_BT_RPA_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bt_rpa);

static int ah(const uint8_t irk[16], const uint8_t r[3], uint8_t out[3])
{
	uint8_t res[16];
	int err;

	LOG_DBG("irk %s", bt_hex(irk, 16));
	LOG_DBG("r %s", bt_hex(r, 3));

	/* r' = padding || r */
	memcpy(res, r, 3);
	(void)memset(res + 3, 0, 13);

	err = bt_encrypt_le(irk, res, res);
	if (err) {
		return err;
	}

	/* The output of the random address function ah is:
	 *      ah(h, r) = e(k, r') mod 2^24
	 * The output of the security function e is then truncated to 24 bits
	 * by taking the least significant 24 bits of the output of e as the
	 * result of ah.
	 */
	memcpy(out, res, 3);

	return 0;
}

#if defined(CONFIG_BT_SMP) || defined(CONFIG_BT_CTLR_PRIVACY)
bool bt_rpa_irk_matches(const uint8_t irk[16], const bt_addr_t *addr)
{
	uint8_t hash[3];
	int err;

	LOG_DBG("IRK %s bdaddr %s", bt_hex(irk, 16), bt_addr_str(addr));

	err = ah(irk, addr->val + 3, hash);
	if (err) {
		return false;
	}

	return !memcmp(addr->val, hash, 3);
}
#endif

#if defined(CONFIG_BT_PRIVACY) || defined(CONFIG_BT_CTLR_PRIVACY)
int bt_rpa_create(const uint8_t irk[16], bt_addr_t *rpa)
{
	int err;

	err = bt_rand(rpa->val + 3, 3);
	if (err) {
		return err;
	}

	BT_ADDR_SET_RPA(rpa);

	err = ah(irk, rpa->val + 3, rpa->val);
	if (err) {
		return err;
	}

	LOG_DBG("Created RPA %s", bt_addr_str((bt_addr_t *)rpa->val));

	return 0;
}
#else
int bt_rpa_create(const uint8_t irk[16], bt_addr_t *rpa)
{
	return -ENOTSUP;
}
#endif /* CONFIG_BT_PRIVACY */
