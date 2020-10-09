/*
 * Copyright (c) 2020 Bose Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "sih.h"

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_SIH)
#define LOG_MODULE_NAME bt_sih
#include "common/log.h"

int sih(const uint8_t sirk[16], const uint32_t r, uint32_t *out)
{
	uint8_t res[16];
	int err;

	if ((r & BIT(23)) || ((r & BIT(22)) == 0)) {
		BT_DBG("Invalid r %0x06x", r & 0xffffff);
	}

	BT_DBG("sirk %s", bt_hex(sirk, 16));
	BT_DBG("r 0x%06x", r);

	/* r' = padding || r */
	memcpy(res, &r, 3);
	(void)memset(res + 3, 0, 13);

	BT_DBG("r' %s", bt_hex(res, 16));

	err = bt_encrypt_le(sirk, res, res);
	if (err) {
		return err;
	}

	/* The output of the function sih is:
	 *      sih(k, r) = e(k, r') mod 2^24
	 * The output of the security function e is then truncated to 24 bits
	 * by taking the least significant 24 bits of the output of e as the
	 * result of sih.
	 */

	BT_DBG("res %s", bt_hex(res, 16));
	BT_DBG("sih %s", bt_hex(res, 3));
	memcpy(out, res, 3);

	return 0;
}
