/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_s32_crypto_hse_mu

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/crypto/crypto.h>
#include <Hse_Ip.h>

#define LOG_LEVEL CONFIG_CRYPTO_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(crypto_nxp_s32_hse);

#if (CONFIG_CRYPTO_NXP_S32_HSE_AES_KEY_SIZE != 128) && \
	(CONFIG_CRYPTO_NXP_S32_HSE_AES_KEY_SIZE != 256)
#error "CRYPTO_NXP_S32_HSE_AES_KEY_SIZE must be 128 or 256"
#endif

#define CRYPTO_NXP_S32_HSE_SERVICE_TIMEOUT_TICKS 10000000
#define CRYPTO_NXP_S32_HSE_INIT_TIMEOUT_MS 10000

#define CRYPTO_NXP_S32_HSE_CIPHER_CAPS                                                             \
	(CAP_RAW_KEY | CAP_SEPARATE_IO_BUFS | CAP_SYNC_OPS | CAP_NO_IV_PREFIX)

#define CRYPTO_NXP_S32_HSE_HASH_CAPS (CAP_SEPARATE_IO_BUFS | CAP_SYNC_OPS)

#define CRYPTO_NXP_S32_HSE_MU_INSTANCE_CHECK(indx, n)                                              \
	((DT_INST_REG_ADDR(n) == IP_MU##indx##__MUB_BASE) ? indx : 0)

#define CRYPTO_NXP_S32_HSE_MU_GET_INSTANCE(n)                                                      \
	LISTIFY(__DEBRACKET HSE_IP_NUM_OF_MU_INSTANCES,                                            \
		CRYPTO_NXP_S32_HSE_MU_INSTANCE_CHECK, (|), n)

struct crypto_nxp_s32_hse_session {
	hseSrvDescriptor_t crypto_serv_desc;
	Hse_Ip_ReqType req_type;
	bool in_use;
	uint8_t channel;
	uint8_t *out_buff;
	struct k_mutex crypto_lock;
	hseKeyHandle_t key_handle;
	hseKeyInfo_t key_info;
};

struct crypto_nxp_s32_hse_data {
	struct crypto_nxp_s32_hse_session sessions[HSE_IP_NUM_OF_CHANNELS_PER_MU];
	Hse_Ip_MuStateType mu_state;
};

struct crypto_nxp_s32_hse_config {
	uint8_t mu_instance;
};

static struct k_mutex crypto_nxp_s32_lock;

static struct crypto_nxp_s32_hse_session *crypto_nxp_s32_hse_get_session(const struct device *dev)
{
	const struct crypto_nxp_s32_hse_config *config = dev->config;
	struct crypto_nxp_s32_hse_data *data = dev->data;
	struct crypto_nxp_s32_hse_session *session;
	uint8_t mu_channel;

	k_mutex_lock(&crypto_nxp_s32_lock, K_FOREVER);
	mu_channel = Hse_Ip_GetFreeChannel(config->mu_instance);

	if (mu_channel != HSE_IP_INVALID_MU_CHANNEL_U8) {
		session = &data->sessions[mu_channel - 1];
		session->in_use = true;
		k_mutex_unlock(&crypto_nxp_s32_lock);
		return session;
	}

	k_mutex_unlock(&crypto_nxp_s32_lock);
	return NULL;
}

static inline void free_session(const struct device *dev,
				struct crypto_nxp_s32_hse_session *session)
{
	const struct crypto_nxp_s32_hse_config *config = dev->config;

	k_mutex_lock(&session->crypto_lock, K_FOREVER);
	memset(&session->req_type, 0, sizeof(Hse_Ip_ReqType));
	memset(&session->crypto_serv_desc, 0, sizeof(hseSrvDescriptor_t));
	memset(&session->key_info, 0, sizeof(hseKeyInfo_t));
	Hse_Ip_ReleaseChannel(config->mu_instance, session->channel);
	session->in_use = false;
	k_mutex_unlock(&session->crypto_lock);
}

static int crypto_nxp_s32_hse_aes_ecb_encrypt(struct cipher_ctx *ctx, struct cipher_pkt *pkt)
{
	const struct crypto_nxp_s32_hse_config *config = ctx->device->config;
	struct crypto_nxp_s32_hse_session *session = ctx->drv_sessn_state;
	hseSymCipherSrv_t *cipher_serv = &(session->crypto_serv_desc.hseSrv.symCipherReq);

	__ASSERT_NO_MSG(pkt->in_len <= pkt->out_buf_max &&
			pkt->out_buf_max <= CONFIG_CRYPTO_NXP_S32_HSE_OUTPUT_BUFFER_SIZE);

	k_mutex_lock(&session->crypto_lock, K_FOREVER);

	/* Update HSE descriptor */
	cipher_serv->cipherBlockMode = HSE_CIPHER_BLOCK_MODE_ECB;
	cipher_serv->cipherDir = HSE_CIPHER_DIR_ENCRYPT;
	cipher_serv->pInput = HSE_PTR_TO_HOST_ADDR(pkt->in_buf);
	cipher_serv->inputLength = pkt->in_len;
	cipher_serv->pOutput = HSE_PTR_TO_HOST_ADDR(session->out_buff);

	if (Hse_Ip_ServiceRequest(
		    config->mu_instance, session->channel, (Hse_Ip_ReqType *)&session->req_type,
		    (hseSrvDescriptor_t *)&session->crypto_serv_desc) != HSE_SRV_RSP_OK) {
		k_mutex_unlock(&session->crypto_lock);
		return -EIO;
	}

	pkt->out_len = pkt->in_len;

	memcpy(pkt->out_buf, session->out_buff, pkt->out_buf_max);

	k_mutex_unlock(&session->crypto_lock);

	return 0;
}

static int crypto_nxp_s32_hse_aes_ecb_decrypt(struct cipher_ctx *ctx, struct cipher_pkt *pkt)
{
	const struct crypto_nxp_s32_hse_config *config = ctx->device->config;
	struct crypto_nxp_s32_hse_session *session = ctx->drv_sessn_state;
	hseSymCipherSrv_t *cipher_serv = &(session->crypto_serv_desc.hseSrv.symCipherReq);

	__ASSERT_NO_MSG(pkt->in_len <= pkt->out_buf_max &&
			pkt->out_buf_max <= CONFIG_CRYPTO_NXP_S32_HSE_OUTPUT_BUFFER_SIZE);

	k_mutex_lock(&session->crypto_lock, K_FOREVER);

	/* Update HSE descriptor */
	cipher_serv->cipherBlockMode = HSE_CIPHER_BLOCK_MODE_ECB;
	cipher_serv->cipherDir = HSE_CIPHER_DIR_DECRYPT;
	cipher_serv->pInput = HSE_PTR_TO_HOST_ADDR(pkt->in_buf);
	cipher_serv->inputLength = pkt->in_len;
	cipher_serv->pOutput = HSE_PTR_TO_HOST_ADDR(session->out_buff);

	if (Hse_Ip_ServiceRequest(
		    config->mu_instance, session->channel, (Hse_Ip_ReqType *)&session->req_type,
		    (hseSrvDescriptor_t *)&session->crypto_serv_desc) != HSE_SRV_RSP_OK) {
		k_mutex_unlock(&session->crypto_lock);
		return -EIO;
	}

	pkt->out_len = pkt->in_len;

	memcpy(pkt->out_buf, session->out_buff, pkt->out_buf_max);

	k_mutex_unlock(&session->crypto_lock);

	return 0;
}

static int crypto_nxp_s32_hse_aes_cbc_encrypt(struct cipher_ctx *ctx, struct cipher_pkt *pkt,
					      uint8_t *iv)
{
	const struct crypto_nxp_s32_hse_config *config = ctx->device->config;
	struct crypto_nxp_s32_hse_session *session = ctx->drv_sessn_state;
	hseSymCipherSrv_t *cipher_serv = &(session->crypto_serv_desc.hseSrv.symCipherReq);
	size_t iv_bytes;

	__ASSERT_NO_MSG(pkt->in_len <= pkt->out_buf_max &&
			pkt->out_buf_max <= CONFIG_CRYPTO_NXP_S32_HSE_OUTPUT_BUFFER_SIZE);

	if (ctx->flags & CAP_NO_IV_PREFIX) {
		iv_bytes = 0;
	} else {
		iv_bytes = HSE_BITS_TO_BYTES(CONFIG_CRYPTO_NXP_S32_HSE_AES_KEY_SIZE);
		memcpy(pkt->out_buf, iv, HSE_BITS_TO_BYTES(CONFIG_CRYPTO_NXP_S32_HSE_AES_KEY_SIZE));
	}

	k_mutex_lock(&session->crypto_lock, K_FOREVER);

	/* Update HSE descriptor */
	cipher_serv->cipherBlockMode = HSE_CIPHER_BLOCK_MODE_CBC;
	cipher_serv->cipherDir = HSE_CIPHER_DIR_ENCRYPT;
	cipher_serv->pIV = HSE_PTR_TO_HOST_ADDR(iv);
	cipher_serv->pInput = HSE_PTR_TO_HOST_ADDR(pkt->in_buf);
	cipher_serv->inputLength = pkt->in_len;
	cipher_serv->pOutput = HSE_PTR_TO_HOST_ADDR(session->out_buff);

	if (Hse_Ip_ServiceRequest(
		    config->mu_instance, session->channel, (Hse_Ip_ReqType *)&session->req_type,
		    (hseSrvDescriptor_t *)&session->crypto_serv_desc) != HSE_SRV_RSP_OK) {
		k_mutex_unlock(&session->crypto_lock);
		return -EIO;
	}

	pkt->out_len = pkt->in_len + iv_bytes;

	memcpy(pkt->out_buf + iv_bytes, session->out_buff, pkt->out_buf_max - iv_bytes);

	k_mutex_unlock(&session->crypto_lock);

	return 0;
}

static int crypto_nxp_s32_hse_aes_cbc_decrypt(struct cipher_ctx *ctx, struct cipher_pkt *pkt,
					      uint8_t *iv)
{
	const struct crypto_nxp_s32_hse_config *config = ctx->device->config;
	struct crypto_nxp_s32_hse_session *session = ctx->drv_sessn_state;
	hseSymCipherSrv_t *cipher_serv = &(session->crypto_serv_desc.hseSrv.symCipherReq);
	size_t iv_bytes;

	if (ctx->flags & CAP_NO_IV_PREFIX) {
		iv_bytes = 0;
	} else {
		iv_bytes = HSE_BITS_TO_BYTES(CONFIG_CRYPTO_NXP_S32_HSE_AES_KEY_SIZE);
	}

	__ASSERT_NO_MSG(pkt->in_len - iv_bytes <= pkt->out_buf_max &&
			pkt->out_buf_max <= CONFIG_CRYPTO_NXP_S32_HSE_OUTPUT_BUFFER_SIZE);

	k_mutex_lock(&session->crypto_lock, K_FOREVER);

	/* Update HSE descriptor */
	cipher_serv->cipherBlockMode = HSE_CIPHER_BLOCK_MODE_CBC;
	cipher_serv->cipherDir = HSE_CIPHER_DIR_DECRYPT;
	cipher_serv->pIV = HSE_PTR_TO_HOST_ADDR(iv);
	cipher_serv->pInput = HSE_PTR_TO_HOST_ADDR(pkt->in_buf + iv_bytes);
	cipher_serv->inputLength = pkt->in_len - iv_bytes;
	cipher_serv->pOutput = HSE_PTR_TO_HOST_ADDR(session->out_buff);

	if (Hse_Ip_ServiceRequest(
		    config->mu_instance, session->channel, (Hse_Ip_ReqType *)&session->req_type,
		    (hseSrvDescriptor_t *)&session->crypto_serv_desc) != HSE_SRV_RSP_OK) {
		k_mutex_unlock(&session->crypto_lock);
		return -EIO;
	}

	pkt->out_len = pkt->in_len;

	memcpy(pkt->out_buf, session->out_buff, pkt->out_buf_max);

	k_mutex_unlock(&session->crypto_lock);

	return 0;
}

static int crypto_nxp_s32_hse_aes_ctr_encrypt(struct cipher_ctx *ctx, struct cipher_pkt *pkt,
					      uint8_t *iv)
{
	const struct crypto_nxp_s32_hse_config *config = ctx->device->config;
	struct crypto_nxp_s32_hse_session *session = ctx->drv_sessn_state;
	hseSymCipherSrv_t *cipher_serv = &(session->crypto_serv_desc.hseSrv.symCipherReq);
	uint8_t iv_key[HSE_BITS_TO_BYTES(CONFIG_CRYPTO_NXP_S32_HSE_AES_KEY_SIZE)] = {0};
	int iv_len = ctx->keylen - HSE_BITS_TO_BYTES(ctx->mode_params.ctr_info.ctr_len);

	__ASSERT_NO_MSG(pkt->in_len <= pkt->out_buf_max &&
			pkt->out_buf_max <= CONFIG_CRYPTO_NXP_S32_HSE_OUTPUT_BUFFER_SIZE);

	/* takes the last 4 bytes of the counter parameter as the true
	 * counter start. IV forms the first 12 bytes of the split counter.
	 */
	memcpy(iv_key, iv, iv_len);

	k_mutex_lock(&session->crypto_lock, K_FOREVER);

	/* Update HSE descriptor */
	cipher_serv->cipherBlockMode = HSE_CIPHER_BLOCK_MODE_CTR;
	cipher_serv->cipherDir = HSE_CIPHER_DIR_ENCRYPT;
	cipher_serv->pIV = HSE_PTR_TO_HOST_ADDR(&iv_key);
	cipher_serv->pInput = HSE_PTR_TO_HOST_ADDR(pkt->in_buf);
	cipher_serv->inputLength = pkt->in_len;
	cipher_serv->pOutput = HSE_PTR_TO_HOST_ADDR(session->out_buff);

	if (Hse_Ip_ServiceRequest(
		    config->mu_instance, session->channel, (Hse_Ip_ReqType *)&session->req_type,
		    (hseSrvDescriptor_t *)&session->crypto_serv_desc) != HSE_SRV_RSP_OK) {
		k_mutex_unlock(&session->crypto_lock);
		return -EIO;
	}

	pkt->out_len = pkt->in_len;

	memcpy(pkt->out_buf, session->out_buff, pkt->out_buf_max);

	k_mutex_unlock(&session->crypto_lock);

	return 0;
}

static int crypto_nxp_s32_hse_aes_ctr_decrypt(struct cipher_ctx *ctx, struct cipher_pkt *pkt,
					      uint8_t *iv)
{
	const struct crypto_nxp_s32_hse_config *config = ctx->device->config;
	struct crypto_nxp_s32_hse_session *session = ctx->drv_sessn_state;
	hseSymCipherSrv_t *cipher_serv = &(session->crypto_serv_desc.hseSrv.symCipherReq);
	uint8_t iv_key[HSE_BITS_TO_BYTES(CONFIG_CRYPTO_NXP_S32_HSE_AES_KEY_SIZE)] = {0};
	int iv_len = ctx->keylen - HSE_BITS_TO_BYTES(ctx->mode_params.ctr_info.ctr_len);

	__ASSERT_NO_MSG(pkt->in_len <= pkt->out_buf_max &&
			pkt->out_buf_max <= CONFIG_CRYPTO_NXP_S32_HSE_OUTPUT_BUFFER_SIZE);

	memcpy(iv_key, iv, iv_len);

	k_mutex_lock(&session->crypto_lock, K_FOREVER);

	/* Update HSE descriptor */
	cipher_serv->cipherBlockMode = HSE_CIPHER_BLOCK_MODE_CTR;
	cipher_serv->cipherDir = HSE_CIPHER_DIR_DECRYPT;
	cipher_serv->pIV = HSE_PTR_TO_HOST_ADDR(&iv_key);
	cipher_serv->pInput = HSE_PTR_TO_HOST_ADDR(pkt->in_buf);
	cipher_serv->inputLength = pkt->in_len;
	cipher_serv->pOutput = HSE_PTR_TO_HOST_ADDR(session->out_buff);

	if (Hse_Ip_ServiceRequest(
		    config->mu_instance, session->channel, (Hse_Ip_ReqType *)&session->req_type,
		    (hseSrvDescriptor_t *)&session->crypto_serv_desc) != HSE_SRV_RSP_OK) {
		k_mutex_unlock(&session->crypto_lock);
		return -EIO;
	}

	pkt->out_len = pkt->in_len;

	memcpy(pkt->out_buf, session->out_buff, pkt->out_buf_max);

	k_mutex_unlock(&session->crypto_lock);

	return 0;
}

static int crypto_nxp_s32_hse_cipher_key_element_set(const struct device *dev,
						     struct crypto_nxp_s32_hse_session *session,
						     struct cipher_ctx *ctx)
{
	const struct crypto_nxp_s32_hse_config *config = dev->config;
	hseImportKeySrv_t *import_key_serv = &(session->crypto_serv_desc.hseSrv.importKeyReq);

	k_mutex_lock(&session->crypto_lock, K_FOREVER);

	session->req_type.eReqType = HSE_IP_REQTYPE_SYNC;
	session->req_type.u32Timeout = CRYPTO_NXP_S32_HSE_SERVICE_TIMEOUT_TICKS;

	session->crypto_serv_desc.srvId = HSE_SRV_ID_IMPORT_KEY;

	session->key_info.keyType = HSE_KEY_TYPE_AES;
	session->key_info.keyBitLen = CONFIG_CRYPTO_NXP_S32_HSE_AES_KEY_SIZE;
	session->key_info.keyFlags = HSE_KF_USAGE_ENCRYPT | HSE_KF_USAGE_DECRYPT;
	session->key_info.keyCounter = 0U;
	session->key_info.smrFlags = 0U;
	session->key_info.specific.aesBlockModeMask = HSE_KU_AES_BLOCK_MODE_ANY;
	session->key_info.specific.aesBlockModeMask = (hseAesBlockModeMask_t)0U;
	session->key_info.hseReserved[0U] = 0U;
	session->key_info.hseReserved[1U] = 0U;

	/* The key import is not encrypted, nor authenticated */
	import_key_serv->cipher.cipherKeyHandle = HSE_INVALID_KEY_HANDLE;
	import_key_serv->keyContainer.authKeyHandle = HSE_INVALID_KEY_HANDLE;
	import_key_serv->pKeyInfo = HSE_PTR_TO_HOST_ADDR(&session->key_info);
	import_key_serv->pKey[2] = HSE_PTR_TO_HOST_ADDR(ctx->key.bit_stream);
	import_key_serv->keyLen[2] = (uint16_t)ctx->keylen;
	import_key_serv->targetKeyHandle = (hseKeyHandle_t)session->key_handle;

	if (Hse_Ip_ServiceRequest(
		    config->mu_instance, session->channel, (Hse_Ip_ReqType *)&session->req_type,
		    (hseSrvDescriptor_t *)&session->crypto_serv_desc) != HSE_SRV_RSP_OK) {
		k_mutex_unlock(&session->crypto_lock);
		return -EIO;
	}

	k_mutex_unlock(&session->crypto_lock);

	return 0;
}

static int crypto_nxp_s32_hse_cipher_begin_session(const struct device *dev, struct cipher_ctx *ctx,
						   enum cipher_algo algo, enum cipher_mode mode,
						   enum cipher_op op_type)
{
	struct crypto_nxp_s32_hse_session *session;

	if (algo != CRYPTO_CIPHER_ALGO_AES) {
		LOG_ERR("Unsupported algorithm");
		return -ENOTSUP;
	}

	if (ctx->flags & ~(CRYPTO_NXP_S32_HSE_CIPHER_CAPS)) {
		LOG_ERR("Unsupported flag");
		return -ENOTSUP;
	}

	if (mode != CRYPTO_CIPHER_MODE_ECB && mode != CRYPTO_CIPHER_MODE_CBC &&
	    mode != CRYPTO_CIPHER_MODE_CTR) {
		LOG_ERR("Unsupported mode");
		return -ENOTSUP;
	}

	if (ctx->keylen != HSE_BITS_TO_BYTES(CONFIG_CRYPTO_NXP_S32_HSE_AES_KEY_SIZE)) {
		LOG_ERR("%u key size is not supported", ctx->keylen);
		return -EINVAL;
	}

	session = crypto_nxp_s32_hse_get_session(dev);

	if (session == NULL) {
		LOG_ERR("No free session");
		return -ENOSPC;
	}

	if (op_type == CRYPTO_CIPHER_OP_ENCRYPT) {
		switch (mode) {
		case CRYPTO_CIPHER_MODE_ECB:
			ctx->ops.block_crypt_hndlr = crypto_nxp_s32_hse_aes_ecb_encrypt;
			break;
		case CRYPTO_CIPHER_MODE_CBC:
			ctx->ops.cbc_crypt_hndlr = crypto_nxp_s32_hse_aes_cbc_encrypt;
			break;
		case CRYPTO_CIPHER_MODE_CTR:
			ctx->ops.ctr_crypt_hndlr = crypto_nxp_s32_hse_aes_ctr_encrypt;
			break;
		default:
			break;
		}
	} else {
		switch (mode) {
		case CRYPTO_CIPHER_MODE_ECB:
			ctx->ops.block_crypt_hndlr = crypto_nxp_s32_hse_aes_ecb_decrypt;
			break;
		case CRYPTO_CIPHER_MODE_CBC:
			ctx->ops.cbc_crypt_hndlr = crypto_nxp_s32_hse_aes_cbc_decrypt;
			break;
		case CRYPTO_CIPHER_MODE_CTR:
			ctx->ops.ctr_crypt_hndlr = crypto_nxp_s32_hse_aes_ctr_decrypt;
			break;
		default:
			break;
		}
	}

	/* Load the key in plain */
	if (crypto_nxp_s32_hse_cipher_key_element_set(dev, session, ctx)) {
		free_session(dev, session);
		LOG_ERR("Failed to import key catalog");
		return -EIO;
	}

	/* Update HSE descriptor */
	session->req_type.eReqType = HSE_IP_REQTYPE_SYNC;
	session->req_type.u32Timeout = CRYPTO_NXP_S32_HSE_SERVICE_TIMEOUT_TICKS;
	session->crypto_serv_desc.srvId = HSE_SRV_ID_SYM_CIPHER;
	session->crypto_serv_desc.hseSrv.symCipherReq.accessMode = HSE_ACCESS_MODE_ONE_PASS;
	session->crypto_serv_desc.hseSrv.symCipherReq.cipherAlgo = HSE_CIPHER_ALGO_AES;
	session->crypto_serv_desc.hseSrv.symCipherReq.keyHandle = session->key_handle;
	session->crypto_serv_desc.hseSrv.symCipherReq.sgtOption = HSE_SGT_OPTION_NONE;

	ctx->drv_sessn_state = session;
	ctx->device = dev;

	return 0;
}

static int crypto_nxp_s32_hse_cipher_free_session(const struct device *dev, struct cipher_ctx *ctx)
{
	struct crypto_nxp_s32_hse_session *session;

	session = ctx->drv_sessn_state;
	free_session(dev, session);

	return 0;
}

static int crypto_nxp_s32_hse_sha(struct hash_ctx *ctx, struct hash_pkt *pkt, bool finish)
{
	const struct crypto_nxp_s32_hse_config *config = ctx->device->config;
	struct crypto_nxp_s32_hse_session *session = ctx->drv_sessn_state;
	hseHashSrv_t *hash_serv = &(session->crypto_serv_desc.hseSrv.hashReq);

	if (!finish) {
		return -ENOTSUP;
	}

	/* Update HSE descriptor */
	hash_serv->pInput = HSE_PTR_TO_HOST_ADDR(pkt->in_buf);
	hash_serv->inputLength = pkt->in_len;

	k_mutex_lock(&session->crypto_lock, K_FOREVER);

	if (Hse_Ip_ServiceRequest(
		    config->mu_instance, session->channel, (Hse_Ip_ReqType *)&session->req_type,
		    (hseSrvDescriptor_t *)&session->crypto_serv_desc) != HSE_SRV_RSP_OK) {
		k_mutex_unlock(&session->crypto_lock);
		return -EIO;
	}

	memcpy(pkt->out_buf, session->out_buff,
	       *(uint32_t *)(session->crypto_serv_desc.hseSrv.hashReq.pHashLength));

	k_mutex_unlock(&session->crypto_lock);

	return 0;
}

static int crypto_nxp_s32_hse_hash_begin_session(const struct device *dev, struct hash_ctx *ctx,
						 enum hash_algo algo)
{
	struct crypto_nxp_s32_hse_session *session;
	static uint32_t out_len;
	hseHashSrv_t *hash_serv;

	if (ctx->flags & ~(CRYPTO_NXP_S32_HSE_HASH_CAPS)) {
		return -ENOTSUP;
	}

	session = crypto_nxp_s32_hse_get_session(dev);

	if (session == NULL) {
		return -ENOSPC;
	}

	session->req_type.eReqType = HSE_IP_REQTYPE_SYNC;
	session->req_type.u32Timeout = CRYPTO_NXP_S32_HSE_SERVICE_TIMEOUT_TICKS;

	hash_serv = &(session->crypto_serv_desc.hseSrv.hashReq);

	switch (algo) {
	case CRYPTO_HASH_ALGO_SHA224: {
		hash_serv->hashAlgo = HSE_HASH_ALGO_SHA2_224;
		out_len = HSE_BITS_TO_BYTES(224);
		break;
	}
	case CRYPTO_HASH_ALGO_SHA256: {
		hash_serv->hashAlgo = HSE_HASH_ALGO_SHA2_256;
		out_len = HSE_BITS_TO_BYTES(256);
		break;
	}
	case CRYPTO_HASH_ALGO_SHA384: {
		hash_serv->hashAlgo = HSE_HASH_ALGO_SHA2_384;
		out_len = HSE_BITS_TO_BYTES(384);
		break;
	}
	case CRYPTO_HASH_ALGO_SHA512: {
		hash_serv->hashAlgo = HSE_HASH_ALGO_SHA2_512;
		out_len = HSE_BITS_TO_BYTES(512);
		break;
	}
	default: {
		return -ENOTSUP;
	}
	}

	session->crypto_serv_desc.srvId = HSE_SRV_ID_HASH;
	hash_serv->accessMode = HSE_ACCESS_MODE_ONE_PASS;
	hash_serv->pHashLength = HSE_PTR_TO_HOST_ADDR(&out_len);
	hash_serv->pHash = HSE_PTR_TO_HOST_ADDR(session->out_buff);

	ctx->drv_sessn_state = session;
	ctx->hash_hndlr = crypto_nxp_s32_hse_sha;
	ctx->device = dev;

	return 0;
}

static int crypto_nxp_s32_hse_hash_free_session(const struct device *dev, struct hash_ctx *ctx)
{
	struct crypto_nxp_s32_hse_session *session;

	session = ctx->drv_sessn_state;
	free_session(dev, session);

	return 0;
}

static int crypto_nxp_s32_hse_query_caps(const struct device *dev)
{
	return CRYPTO_NXP_S32_HSE_HASH_CAPS | CRYPTO_NXP_S32_HSE_CIPHER_CAPS;
}

static int crypto_nxp_s32_hse_init(const struct device *dev)
{
	const struct crypto_nxp_s32_hse_config *config = dev->config;
	struct crypto_nxp_s32_hse_data *data = dev->data;
	struct crypto_nxp_s32_hse_session *session;
	hseStatus_t status;

	k_timeout_t timeout = K_MSEC(CRYPTO_NXP_S32_HSE_INIT_TIMEOUT_MS);
	int64_t start_time = k_uptime_ticks();

	do {
		status = Hse_Ip_GetHseStatus(config->mu_instance);
	} while (!(status & (HSE_STATUS_INIT_OK | HSE_STATUS_INSTALL_OK)) ||
		 (k_uptime_ticks() - start_time < timeout.ticks));

	if (!(status & HSE_STATUS_INIT_OK)) {
		LOG_ERR("HSE initialization has not been completed or "
			 "MU%d is not activated", config->mu_instance);
		return -EIO;
	}

	if (!(status & HSE_STATUS_INSTALL_OK)) {
		LOG_ERR("Key catalogs has not been formatted");
		return -EIO;
	}

	if (Hse_Ip_Init(config->mu_instance, &data->mu_state) != HSE_IP_STATUS_SUCCESS) {
		LOG_ERR("Failed to initialize MU%d", config->mu_instance);
		return -EIO;
	}

	for (size_t i = 0; i < HSE_IP_NUM_OF_CHANNELS_PER_MU; i++) {
		session = &data->sessions[i];
		k_mutex_init(&session->crypto_lock);
	}

	k_mutex_init(&crypto_nxp_s32_lock);

	return 0;
}

static DEVICE_API(crypto, crypto_nxp_s32_hse_api) = {
	.cipher_begin_session = crypto_nxp_s32_hse_cipher_begin_session,
	.cipher_free_session = crypto_nxp_s32_hse_cipher_free_session,
	.query_hw_caps = crypto_nxp_s32_hse_query_caps,
	.hash_begin_session = crypto_nxp_s32_hse_hash_begin_session,
	.hash_free_session = crypto_nxp_s32_hse_hash_free_session,
};

#define CRYPTO_NXP_S32_HSE_SESSION_CFG(indx, n)                                                    \
	{                                                                                          \
		.channel = indx + 1,                                                               \
		.out_buff = &crypto_out_buff_##n[indx][0],                                         \
		.key_handle = GET_KEY_HANDLE(HSE_KEY_CATALOG_ID_RAM,                               \
					     CONFIG_CRYPTO_NXP_S32_HSE_AES_KEY_GROUP_ID, indx),    \
	}

#define CRYPTO_NXP_S32_HSE_INIT_SESSION(n)                                                         \
	LISTIFY(__DEBRACKET HSE_IP_NUM_OF_CHANNELS_PER_MU, CRYPTO_NXP_S32_HSE_SESSION_CFG, (,), n)

#define CRYPTO_NXP_S32_HSE_INIT_DEVICE(n)                                                          \
	static __nocache uint8_t                                                                   \
		crypto_out_buff_##n[HSE_IP_NUM_OF_CHANNELS_PER_MU]                                 \
				   [CONFIG_CRYPTO_NXP_S32_HSE_OUTPUT_BUFFER_SIZE];                 \
                                                                                                   \
	static struct crypto_nxp_s32_hse_data crypto_nxp_s32_hse_data_##n = {                      \
		.sessions =                                                                        \
			{                                                                          \
				CRYPTO_NXP_S32_HSE_INIT_SESSION(n),                                \
			},                                                                         \
	};                                                                                         \
                                                                                                   \
	static const struct crypto_nxp_s32_hse_config crypto_nxp_s32_hse_config_##n = {            \
		.mu_instance = CRYPTO_NXP_S32_HSE_MU_GET_INSTANCE(n),                              \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &crypto_nxp_s32_hse_init, NULL, &crypto_nxp_s32_hse_data_##n,     \
			      &crypto_nxp_s32_hse_config_##n, POST_KERNEL,                         \
			      CONFIG_CRYPTO_INIT_PRIORITY, &crypto_nxp_s32_hse_api);

DT_INST_FOREACH_STATUS_OKAY(CRYPTO_NXP_S32_HSE_INIT_DEVICE)
