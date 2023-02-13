/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief 802.15.4 6LoWPAN authentication and encryption implementation
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_ieee802154_security, CONFIG_NET_L2_IEEE802154_LOG_LEVEL);

#include "ieee802154_frame.h"
#include "ieee802154_security.h"

#include <zephyr/crypto/crypto.h>
#include <zephyr/net/net_core.h>

extern const uint8_t level_2_tag_size[4];

int ieee802154_security_setup_session(struct ieee802154_security_ctx *sec_ctx, uint8_t level,
				      uint8_t key_mode, uint8_t *key, uint8_t key_len)
{
	uint8_t tag_size;
	int ret;

	if (level > IEEE802154_SECURITY_LEVEL_ENC_MIC_128 ||
	    key_mode > IEEE802154_KEY_ID_MODE_SRC_8_INDEX) {
		return -EINVAL;
	}

	/* TODO: supporting other key modes */
	if (level > IEEE802154_SECURITY_LEVEL_NONE &&
	    (key_len > IEEE802154_KEY_MAX_LEN || !key ||
	     key_mode != IEEE802154_KEY_ID_MODE_IMPLICIT)) {
		return -EINVAL;
	}

	if (level >= IEEE802154_SECURITY_LEVEL_ENC) {
		tag_size = level_2_tag_size[level - 4];
	} else {
		tag_size = level_2_tag_size[level];
	}
	sec_ctx->enc.mode_params.ccm_info.tag_len = tag_size;
	sec_ctx->dec.mode_params.ccm_info.tag_len = tag_size;

	sec_ctx->level = level;

	if (level == IEEE802154_SECURITY_LEVEL_NONE) {
		return 0;
	}

	memcpy(sec_ctx->key, key, key_len);
	sec_ctx->key_len = key_len;
	sec_ctx->key_mode = key_mode;

	sec_ctx->enc.key.bit_stream = sec_ctx->key;
	sec_ctx->enc.keylen = sec_ctx->key_len;

	sec_ctx->dec.key.bit_stream = sec_ctx->key;
	sec_ctx->dec.keylen = sec_ctx->key_len;

	ret = cipher_begin_session(sec_ctx->enc.device, &sec_ctx->enc, CRYPTO_CIPHER_ALGO_AES,
				   CRYPTO_CIPHER_MODE_CCM, CRYPTO_CIPHER_OP_ENCRYPT);
	if (ret) {
		NET_ERR("Could not setup encryption context");

		return ret;
	}

	ret = cipher_begin_session(sec_ctx->dec.device, &sec_ctx->dec, CRYPTO_CIPHER_ALGO_AES,
				   CRYPTO_CIPHER_MODE_CCM, CRYPTO_CIPHER_OP_DECRYPT);
	if (ret) {
		NET_ERR("Could not setup decryption context");
		cipher_free_session(sec_ctx->enc.device, &sec_ctx->enc);

		return ret;
	}

	return 0;
}

static void prepare_cipher_aead_pkt(uint8_t *frame, uint8_t level, uint8_t hdr_len,
				    uint8_t payload_len, uint8_t tag_size,
				    struct cipher_aead_pkt *apkt, struct cipher_pkt *pkt)
{
	bool is_encrypted = level >= IEEE802154_SECURITY_LEVEL_ENC;
	bool is_authenticated = level != IEEE802154_SECURITY_LEVEL_NONE &&
				level != IEEE802154_SECURITY_LEVEL_ENC;

	/* See section 7.6.3.4.2 */
	pkt->in_buf = is_encrypted && payload_len ? frame + hdr_len : NULL;
	pkt->in_len = is_encrypted ? payload_len : 0;

	/* See section 7.6.3.4.2 */
	uint8_t out_buf_offset = is_encrypted ? hdr_len : hdr_len + payload_len;
	uint8_t auth_len = is_authenticated ? out_buf_offset : 0;

	/* See section 7.5.8.2.1 i) 1) */
	pkt->out_buf = frame + out_buf_offset;
	pkt->out_buf_max = (is_encrypted ? payload_len : 0) + tag_size;

	apkt->ad = is_authenticated ? frame : NULL;
	apkt->ad_len = auth_len;
	apkt->tag = is_authenticated ? frame + hdr_len + payload_len : NULL;
	apkt->pkt = pkt;
}

bool ieee802154_decrypt_auth(struct ieee802154_security_ctx *sec_ctx, uint8_t *frame,
			     uint8_t hdr_len, uint8_t payload_len, uint8_t tag_size,
			     uint8_t *src_ext_addr, uint32_t frame_counter)
{
	struct cipher_aead_pkt apkt;
	struct cipher_pkt pkt;
	uint8_t nonce[13];
	uint8_t level;
	int ret;

	if (!sec_ctx || sec_ctx->level == IEEE802154_SECURITY_LEVEL_NONE) {
		return true;
	}

	level = sec_ctx->level;

	if (level == IEEE802154_SECURITY_LEVEL_ENC) {
		/* See comment in ieee802154_encrypt_auth(). */
		NET_ERR("Encrypt-only operation is not supported.");
		return false;
	}

	/* See section 7.6.3.2 */
	memcpy(nonce, src_ext_addr, IEEE802154_EXT_ADDR_LENGTH);
	sys_put_be32(frame_counter, &nonce[8]);
	nonce[12] = level;

	prepare_cipher_aead_pkt(frame, level, hdr_len, payload_len, tag_size, &apkt, &pkt);

	ret = cipher_ccm_op(&sec_ctx->dec, &apkt, nonce);
	if (ret) {
		NET_ERR("Cannot decrypt/auth (%i): %p %u/%u - fc %u", ret, frame, hdr_len,
			payload_len, frame_counter);
		return false;
	}

	return true;
}

bool ieee802154_encrypt_auth(struct ieee802154_security_ctx *sec_ctx, uint8_t *frame,
			     uint8_t hdr_len, uint8_t payload_len, uint8_t tag_size,
			     uint8_t *src_ext_addr)
{
	struct cipher_aead_pkt apkt;
	struct cipher_pkt pkt;
	uint8_t nonce[13];
	uint8_t level;
	int ret;

	if (!sec_ctx || sec_ctx->level == IEEE802154_SECURITY_LEVEL_NONE) {
		return true;
	}

	level = sec_ctx->level;

	if (level == IEEE802154_SECURITY_LEVEL_ENC) {
		/* TODO: We currently use CCM rather than CCM* as crypto.h does
		 *       not provide access to CCM* as of now.
		 *       The spec requires CCM* to support encryption-only CCM
		 *       operation, see annex B.1
		 */
		NET_ERR("Encrypt-only operation is not supported.");
		return false;
	}

	/* See section 7.5.8.2.1 f) */
	if (sec_ctx->frame_counter == 0xffffffff) {
		NET_ERR("Max frame counter reached. Update key material to reset the counter.");
		return false;
	}

	/* See section 7.6.3.2 */
	memcpy(nonce, src_ext_addr, IEEE802154_EXT_ADDR_LENGTH);
	sys_put_be32(sec_ctx->frame_counter, &nonce[8]);
	nonce[12] = level;

	prepare_cipher_aead_pkt(frame, level, hdr_len, payload_len, tag_size, &apkt, &pkt);

	ret = cipher_ccm_op(&sec_ctx->enc, &apkt, nonce);
	if (ret) {
		NET_ERR("Cannot encrypt/auth (%i): %p %u/%u - fc %u", ret, frame, hdr_len,
			payload_len, sec_ctx->frame_counter);
		return false;
	}

	sec_ctx->frame_counter++;

	return true;
}

int ieee802154_security_init(struct ieee802154_security_ctx *sec_ctx)
{
	const struct device *dev;

	(void)memset(&sec_ctx->enc, 0, sizeof(struct cipher_ctx));
	(void)memset(&sec_ctx->dec, 0, sizeof(struct cipher_ctx));

	dev = device_get_binding(CONFIG_NET_L2_IEEE802154_SECURITY_CRYPTO_DEV_NAME);
	if (!dev) {
		return -ENODEV;
	}

	sec_ctx->enc.flags = crypto_query_hwcaps(dev);
	sec_ctx->dec.flags = crypto_query_hwcaps(dev);

	sec_ctx->enc.mode_params.ccm_info.nonce_len = 13U;
	sec_ctx->dec.mode_params.ccm_info.nonce_len = 13U;

	sec_ctx->enc.device = dev;
	sec_ctx->dec.device = dev;

	return 0;
}
