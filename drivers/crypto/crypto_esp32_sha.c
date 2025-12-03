/*
 * Copyright (c) 2025 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/logging/log.h>
#include <zephyr/crypto/crypto.h>
#include <zephyr/crypto/hash.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <string.h>
#include <errno.h>

#include "hal/sha_hal.h"
#include "hal/sha_ll.h"
#include "hal/sha_types.h"
#include "soc/soc_caps.h"

#if !SOC_SHA_SUPPORT_RESUME
#include "soc/hwcrypto_reg.h"
#endif

LOG_MODULE_REGISTER(esp32_sha, CONFIG_CRYPTO_LOG_LEVEL);

#define DT_DRV_COMPAT espressif_esp32_sha

/* SHA-224 uses SHA-256 algorithm with different initial values (IV).
 * Hardware only supports SHA-256 mode, so we manually load the SHA-224 IV
 * before processing. Output is then truncated to 28 bytes.
 * These are the official SHA-224 initial hash values from FIPS 180-4.
 */
static const uint32_t sha224_init_state[8] = {0xd89e05c1U, 0x07d57c36U, 0x17dd7030U, 0x39590ef7U,
					      0x310bc0ffU, 0x11155868U, 0xa78ff964U, 0xa44ffabeU};

struct sha_params {
	esp_sha_type hal_mode;
	uint16_t block_bytes;
	uint16_t state_words;
	uint16_t out_bytes;
	uint16_t len_field_bytes;
};

struct esp_sha_config {
	const struct device *clock_dev;
	const clock_control_subsys_t clock_subsys;
};

struct esp_sha_dev_data {
	struct k_mutex sha_lock;
};

struct esp_sha_ctx {
	struct sha_params params;
	enum hash_algo algo;

	bool in_use;
	bool first_block;

	uint64_t total_len;
	size_t buf_len;

	uint32_t buf_w[32]; /* Max block size is 128 bytes = 32 words */
	uint32_t H[16];     /* Max state size for SHA-512 */
};

static K_MUTEX_DEFINE(sha_pool_lock);
static struct esp_sha_ctx sha_pool[CONFIG_CRYPTO_ESP32_SHA_SESSIONS_MAX];

static bool sha_algo_supported(enum hash_algo algo)
{
	switch (algo) {
	case CRYPTO_HASH_ALGO_SHA224:
#if SOC_SHA_SUPPORT_RESUME
		return true;
#else
		/* SHA-224 requires resume support for custom IV loading */
		/* Original ESP32 cannot properly load SHA-224 IV */
		return false;
#endif

	case CRYPTO_HASH_ALGO_SHA256:
		return true;

	case CRYPTO_HASH_ALGO_SHA384:
#if SOC_SHA_SUPPORT_SHA384
		return true;
#else
		return false;
#endif

	case CRYPTO_HASH_ALGO_SHA512:
#if SOC_SHA_SUPPORT_SHA512
		return true;
#else
		return false;
#endif

	default:
		return false;
	}
}

static int sha_get_params(enum hash_algo algo, struct sha_params *params)
{
	if (!params) {
		return -EINVAL;
	}

	if (!sha_algo_supported(algo)) {
		LOG_ERR("Algorithm %d not supported by hardware", algo);
		return -ENOTSUP;
	}

	switch (algo) {
	case CRYPTO_HASH_ALGO_SHA224:
		params->hal_mode = SHA2_256;
		params->block_bytes = 64;
		params->state_words = 8;
		params->out_bytes = 28;
		params->len_field_bytes = 8;
		break;

	case CRYPTO_HASH_ALGO_SHA256:
		params->hal_mode = SHA2_256;
		params->block_bytes = 64;
		params->state_words = 8;
		params->out_bytes = 32;
		params->len_field_bytes = 8;
		break;

#if SOC_SHA_SUPPORT_SHA384
	case CRYPTO_HASH_ALGO_SHA384:
		params->hal_mode = SHA2_384;
		params->block_bytes = 128;
		params->state_words = 12;
		params->out_bytes = 48;
		params->len_field_bytes = 16;
		break;
#endif

#if SOC_SHA_SUPPORT_SHA512
	case CRYPTO_HASH_ALGO_SHA512:
		params->hal_mode = SHA2_512;
		params->block_bytes = 128;
		params->state_words = 16;
		params->out_bytes = 64;
		params->len_field_bytes = 16;
		break;
#endif

	default:
		return -ENOTSUP;
	}

	return 0;
}

static void sha_ctx_init_params(struct esp_sha_ctx *s, enum hash_algo algo)
{
	bool was_in_use = s->in_use;

	memset(s, 0, sizeof(*s));
	s->in_use = was_in_use;
	s->first_block = true;
	s->algo = algo;

	if (sha_get_params(algo, &s->params) != 0) {
		LOG_ERR("Failed to get parameters for algorithm %d", algo);
		return;
	}

	if (algo == CRYPTO_HASH_ALGO_SHA224) {
		memcpy(s->H, sha224_init_state, sizeof(sha224_init_state));
	}
}

static struct esp_sha_ctx *sha_pool_alloc(enum hash_algo algo)
{
	struct esp_sha_ctx *ret = NULL;

	k_mutex_lock(&sha_pool_lock, K_FOREVER);

	for (size_t i = 0; i < ARRAY_SIZE(sha_pool); i++) {
		if (!sha_pool[i].in_use) {
			sha_pool[i].in_use = true;
			sha_ctx_init_params(&sha_pool[i], algo);
			ret = &sha_pool[i];
			break;
		}
	}

	k_mutex_unlock(&sha_pool_lock);

	if (!ret) {
		LOG_WRN("No available SHA context in pool");
	}

	return ret;
}

static void sha_pool_free(struct esp_sha_ctx *s)
{
	if (!s) {
		return;
	}

	k_mutex_lock(&sha_pool_lock, K_FOREVER);
	memset(s, 0, sizeof(*s));
	k_mutex_unlock(&sha_pool_lock);
}

#if SOC_SHA_SUPPORT_RESUME
static inline void sha_hw_restore(struct esp_sha_ctx *s)
{
	if (s->first_block && s->algo != CRYPTO_HASH_ALGO_SHA224) {
		return;
	}

	sha_hal_write_digest(s->params.hal_mode, s->H);
}

#else  /* !SOC_SHA_SUPPORT_RESUME */

static inline void sha_ll_write_digest_esp32(esp_sha_type sha_type, void *digest_state,
					     size_t state_len)
{
	uint32_t *digest_state_words = (uint32_t *)digest_state;
	uint32_t *reg_addr_buf = (uint32_t *)(SHA_TEXT_BASE);

	if (sha_type == SHA2_384 || sha_type == SHA2_512) {
		for (size_t i = 0; i < state_len; i += 2) {
			reg_addr_buf[i] = digest_state_words[i + 1];
			reg_addr_buf[i + 1] = digest_state_words[i];
		}
	} else {
		for (size_t i = 0; i < state_len; i++) {
			reg_addr_buf[i] = digest_state_words[i];
		}
	}

	sha_ll_load(sha_type);
}

static inline void sha_hw_restore_esp32(struct esp_sha_ctx *s)
{
	if (s->first_block && s->algo != CRYPTO_HASH_ALGO_SHA224) {
		return;
	}

	sha_ll_write_digest_esp32(s->params.hal_mode, s->H, s->params.state_words);
}
#endif /* SOC_SHA_SUPPORT_RESUME */

static size_t sha_make_padding(const struct esp_sha_ctx *s, const uint8_t *tail, size_t tail_len,
			       uint8_t last[128], uint8_t last2[128])
{
	const size_t B = s->params.block_bytes;
	const size_t L = s->params.len_field_bytes;
	const uint64_t bit_len = s->total_len * 8ULL;

	if (tail_len >= B) {
		LOG_ERR("Invalid tail length: %zu", tail_len);
		return 0;
	}

	memset(last, 0, B);
	if (tail_len > 0) {
		memcpy(last, tail, tail_len);
	}
	last[tail_len] = 0x80;

	if (tail_len + 1 + L <= B) {
		if (L == 16) {
			memset(&last[B - 16], 0, 8);
		}

		for (int i = 0; i < 8; i++) {
			last[B - 8 + i] = (uint8_t)(bit_len >> (56 - 8 * i));
		}
		return 1;
	}

	memset(last2, 0, B);
	if (L == 16) {
		memset(&last2[B - 16], 0, 8);
	}
	for (int i = 0; i < 8; i++) {
		last2[B - 8 + i] = (uint8_t)(bit_len >> (56 - 8 * i));
	}

	return 2;
}

static inline void sha_compress_block(const struct esp_sha_ctx *s, const uint8_t *block, bool first)
{
	const int words = s->params.block_bytes / 4;
	uint32_t w[32];

	for (int i = 0; i < words; i++) {
		w[i] = sys_get_le32(&block[i * 4]);
	}

	bool first_param = (s->algo == CRYPTO_HASH_ALGO_SHA224) ? false : first;

	sha_hal_hash_block(s->params.hal_mode, w, words, first_param);
	sha_hal_wait_idle();
}

static int sha_update_stream(struct esp_sha_ctx *s, const uint8_t *in, size_t len)
{
	if (len == 0) {
		return 0;
	}

	if (!in) {
		return -EINVAL;
	}

	s->total_len += len;
	uint8_t *buf = (uint8_t *)s->buf_w;

	if (s->buf_len > 0) {
		size_t take = MIN(len, s->params.block_bytes - s->buf_len);

		memcpy(&buf[s->buf_len], in, take);
		s->buf_len += take;
		in += take;
		len -= take;

		if (s->buf_len == s->params.block_bytes) {
			sha_compress_block(s, buf, s->first_block);
			s->first_block = false;
			s->buf_len = 0;
		}
	}

	while (len >= s->params.block_bytes) {
		sha_compress_block(s, in, s->first_block);
		s->first_block = false;
		in += s->params.block_bytes;
		len -= s->params.block_bytes;
	}

	if (len > 0) {
		memcpy(buf, in, len);
		s->buf_len = len;
	}

	return 0;
}

static int sha_query_hw_caps(const struct device *dev)
{
	ARG_UNUSED(dev);

	return (CAP_SEPARATE_IO_BUFS | CAP_SYNC_OPS);
}

static int sha_handler(struct hash_ctx *hctx, struct hash_pkt *pkt, bool fin)
{
	struct esp_sha_ctx *s = hctx->drv_sessn_state;
	struct esp_sha_dev_data *data = hctx->device->data;
	int ret = 0;

	if (!s) {
		LOG_ERR("Invalid session state");
		return -EINVAL;
	}

	if ((pkt->in_len > 0 && !pkt->in_buf) || (fin && !pkt->out_buf)) {
		LOG_ERR("Invalid buffer pointers");
		return -EINVAL;
	}

#if !SOC_SHA_SUPPORT_RESUME
	if (!s->first_block) {
		LOG_ERR("Multi-part hash not supported on this chip (no resume support)");
		return -ENOTSUP;
	}

	if (!fin) {
		LOG_ERR("Non-final operations not supported on original ESP32");
		return -ENOTSUP;
	}
#endif

	k_mutex_lock(&data->sha_lock, K_FOREVER);

	sha_hal_wait_idle();

#if SOC_SHA_SUPPORT_RESUME
	sha_hw_restore(s);
#else
	sha_hw_restore_esp32(s);
#endif

	if (pkt->in_len > 0) {
		ret = sha_update_stream(s, pkt->in_buf, pkt->in_len);
		if (ret != 0) {
			LOG_ERR("Failed to update stream: %d", ret);
			goto unlock;
		}
	}

	if (!fin) {
#if SOC_SHA_SUPPORT_RESUME
		sha_hal_wait_idle();
		sha_hal_read_digest(s->params.hal_mode, s->H);
#endif
		goto unlock;
	}

	uint8_t last[128], last2[128];
	uint8_t *tail = (uint8_t *)s->buf_w;
	size_t nfinal = sha_make_padding(s, tail, s->buf_len, last, last2);

	if (nfinal == 0) {
		LOG_ERR("Failed to create padding");
		ret = -EINVAL;
		goto unlock;
	}

	sha_compress_block(s, last, s->first_block);
	if (nfinal == 2) {
		sha_compress_block(s, last2, false);
	}

	sha_hal_wait_idle();
	sha_hal_read_digest(s->params.hal_mode, s->H);

	int words = s->params.out_bytes / 4;

#if SOC_SHA_SUPPORT_RESUME
	/* ESP32-S3 and newer: Use little-endian output, no word swapping */
	for (int j = 0; j < words; j++) {
		sys_put_le32(s->H[j], &pkt->out_buf[j * 4]);
	}
#else
	/* ESP32: Use big-endian output with word swapping for SHA-384/512 */
	if (s->algo == CRYPTO_HASH_ALGO_SHA384 || s->algo == CRYPTO_HASH_ALGO_SHA512) {
		for (int j = 0; j < words; j += 2) {
			sys_put_be32(s->H[j + 1], &pkt->out_buf[j * 4]);
			sys_put_be32(s->H[j], &pkt->out_buf[(j + 1) * 4]);
		}
	} else {
		for (int j = 0; j < words; j++) {
			sys_put_be32(s->H[j], &pkt->out_buf[j * 4]);
		}
	}
#endif

	ret = 0;

	sha_ctx_init_params(s, s->algo);

unlock:
	k_mutex_unlock(&data->sha_lock);

	return ret;
}

static int sha_begin_session(const struct device *dev, struct hash_ctx *hctx, enum hash_algo algo)
{
	if (!dev || !hctx) {
		return -EINVAL;
	}

	if (!sha_algo_supported(algo)) {
		return -ENOTSUP;
	}

	struct esp_sha_ctx *s = sha_pool_alloc(algo);

	if (!s) {
		LOG_ERR("No available SHA sessions");
		return -ENOMEM;
	}

	hctx->device = dev;
	hctx->drv_sessn_state = s;
	hctx->hash_hndlr = sha_handler;
	hctx->started = false;

	return 0;
}

static int sha_free_session(const struct device *dev, struct hash_ctx *hctx)
{
	ARG_UNUSED(dev);

	if (!hctx) {
		return -EINVAL;
	}

	if (hctx->drv_sessn_state) {
		struct esp_sha_ctx *s = hctx->drv_sessn_state;

		sha_pool_free(s);
		hctx->drv_sessn_state = NULL;
	}

	return 0;
}

static int sha_hash_async_cb_set(const struct device *dev, hash_completion_cb cb)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cb);
	return -ENOTSUP;
}

static int sha_init(const struct device *dev)
{
	struct esp_sha_dev_data *data = dev->data;
	const struct esp_sha_config *cfg = dev->config;

	if (!cfg->clock_dev) {
		LOG_ERR("Clock device is NULL");
		return -EINVAL;
	}

	if (!device_is_ready(cfg->clock_dev)) {
		LOG_ERR("Clock device not ready");
		return -ENODEV;
	}

	if (clock_control_on(cfg->clock_dev, cfg->clock_subsys) != 0) {
		LOG_ERR("Failed to enable clock");
		return -EIO;
	}

	k_mutex_init(&data->sha_lock);

	return 0;
}

static const struct crypto_driver_api sha_crypto_api = {
	.query_hw_caps = sha_query_hw_caps,
	.hash_begin_session = sha_begin_session,
	.hash_free_session = sha_free_session,
	.hash_async_callback_set = sha_hash_async_cb_set,
};

#define ESP_SHA_DEVICE_INIT(inst)                                                                  \
	static struct esp_sha_dev_data sha_data_##inst;                                            \
	static const struct esp_sha_config sha_cfg_##inst = {                                      \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),                             \
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(inst, offset)};        \
	DEVICE_DT_INST_DEFINE(inst, sha_init, NULL, &sha_data_##inst, &sha_cfg_##inst,             \
			      POST_KERNEL, CONFIG_CRYPTO_INIT_PRIORITY, &sha_crypto_api);

DT_INST_FOREACH_STATUS_OKAY(ESP_SHA_DEVICE_INIT)
