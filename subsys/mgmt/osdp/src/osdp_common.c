/*
 * Copyright (c) 2020 Siddharth Chandrasekaran <siddharth@embedjournal.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ctype.h>

#include <zephyr/device.h>
#include <zephyr/sys/crc.h>
#include <zephyr/logging/log.h>

#ifdef CONFIG_OSDP_SC_ENABLED
#include <zephyr/crypto/crypto.h>
#include <zephyr/random/rand32.h>
#endif

#include "osdp_common.h"

LOG_MODULE_DECLARE(osdp, CONFIG_OSDP_LOG_LEVEL);

void osdp_dump(const char *head, uint8_t *buf, int len)
{
	LOG_HEXDUMP_DBG(buf, len, head);
}

uint16_t osdp_compute_crc16(const uint8_t *buf, size_t len)
{
	return crc16_itu_t(0x1D0F, buf, len);
}

int64_t osdp_millis_now(void)
{
	return (int64_t) k_uptime_get();
}

int64_t osdp_millis_since(int64_t last)
{
	int64_t tmp = last;

	return (int64_t) k_uptime_delta(&tmp);
}

void osdp_keyset_complete(struct osdp_pd *pd)
{
	cp_keyset_complete(pd);
}

#ifdef CONFIG_OSDP_SC_ENABLED

void osdp_encrypt(uint8_t *key, uint8_t *iv, uint8_t *data, int len)
{
	const struct device *dev;
	struct cipher_ctx ctx = {
		.keylen = 16,
		.key.bit_stream = key,
		.flags = CAP_NO_IV_PREFIX
	};
	struct cipher_pkt encrypt = {
		.in_buf = data,
		.in_len = len,
		.out_buf = data,
		.out_len = len
	};

	dev = device_get_binding(CONFIG_OSDP_CRYPTO_DRV_NAME);
	if (dev == NULL) {
		LOG_ERR("Failed to get crypto dev binding!");
		return;
	}

	if (iv != NULL) {
		if (cipher_begin_session(dev, &ctx,
					 CRYPTO_CIPHER_ALGO_AES,
					 CRYPTO_CIPHER_MODE_CBC,
					 CRYPTO_CIPHER_OP_ENCRYPT)) {
			LOG_ERR("Failed at cipher_begin_session");
			return;
		}
		if (cipher_cbc_op(&ctx, &encrypt, iv)) {
			LOG_ERR("CBC ENCRYPT - Failed");
		}
	} else {
		if (cipher_begin_session(dev, &ctx,
					 CRYPTO_CIPHER_ALGO_AES,
					 CRYPTO_CIPHER_MODE_ECB,
					 CRYPTO_CIPHER_OP_ENCRYPT)) {
			LOG_ERR("Failed at cipher_begin_session");
			return;
		}
		if (cipher_block_op(&ctx, &encrypt)) {
			LOG_ERR("ECB ENCRYPT - Failed");
		}
	}
	cipher_free_session(dev, &ctx);
}

void osdp_decrypt(uint8_t *key, uint8_t *iv, uint8_t *data, int len)
{
	const struct device *dev;
	struct cipher_ctx ctx = {
		.keylen = 16,
		.key.bit_stream = key,
		.flags = CAP_NO_IV_PREFIX
	};
	struct cipher_pkt decrypt = {
		.in_buf = data,
		.in_len = len,
		.out_buf = data,
		.out_len = len
	};

	dev = device_get_binding(CONFIG_OSDP_CRYPTO_DRV_NAME);
	if (dev == NULL) {
		LOG_ERR("Failed to get crypto dev binding!");
		return;
	}

	if (iv != NULL) {
		if (cipher_begin_session(dev, &ctx,
					 CRYPTO_CIPHER_ALGO_AES,
					 CRYPTO_CIPHER_MODE_CBC,
					 CRYPTO_CIPHER_OP_DECRYPT)) {
			LOG_ERR("Failed at cipher_begin_session");
			return;
		}
		if (cipher_cbc_op(&ctx, &decrypt, iv)) {
			LOG_ERR("CBC DECRYPT - Failed");
		}
	} else {
		if (cipher_begin_session(dev, &ctx,
					 CRYPTO_CIPHER_ALGO_AES,
					 CRYPTO_CIPHER_MODE_ECB,
					 CRYPTO_CIPHER_OP_DECRYPT)) {
			LOG_ERR("Failed at cipher_begin_session");
			return;
		}
		if (cipher_block_op(&ctx, &decrypt)) {
			LOG_ERR("ECB DECRYPT - Failed");
		}
	}
	cipher_free_session(dev, &ctx);
}

void osdp_fill_random(uint8_t *buf, int len)
{
	sys_csrand_get(buf, len);
}

uint32_t osdp_get_sc_status_mask(void)
{
	int i;
	uint32_t mask = 0;
	struct osdp_pd *pd;
	struct osdp *ctx = osdp_get_ctx();

	for (i = 0; i < NUM_PD(ctx); i++) {
		pd = osdp_to_pd(ctx, i);
		if (ISSET_FLAG(pd, PD_FLAG_SC_ACTIVE)) {
			mask |= 1 << i;
		}
	}
	return mask;
}

#endif /* CONFIG_OSDP_SC_ENABLED */
