/*
 * Copyright 2024 NXP
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

#define CRYPTO_NXP_S32_HSE_TIMEOUT 1000000000

#define CRYPTO_NXP_S32_HSE_CIPHER_CAPS                                                             \
	(CAP_RAW_KEY | CAP_SEPARATE_IO_BUFS | CAP_SYNC_OPS | CAP_NO_IV_PREFIX)

#define CRYPTO_NXP_S32_HSE_HASH_CAPS (CAP_SEPARATE_IO_BUFS | CAP_SYNC_OPS)

#define CRYPTO_NXP_S32_HSE_MU_INSTANCE_CHECK(indx, n)                                              \
	((DT_INST_REG_ADDR(n) == IP_MU##indx##__MUA_BASE) ? indx : 0)

#define CRYPTO_NXP_S32_HSE_MU_GET_INSTANCE(n)                                                      \
	LISTIFY(__DEBRACKET HSE_IP_NUM_OF_MU_INSTANCES, CRYPTO_NXP_S32_HSE_MU_INSTANCE_CHECK, (|), n)

#define CRYPTO_NXP_S32_HSE_BLOCK_KEY_LEN_BYTES 16U

static __nocache uint8_t crypto_out_buff[HSE_IP_NUM_OF_CHANNELS_PER_MU]
					[CONFIG_CRYPTO_NXP_S32_HSE_OUTPUT_BUFFER_SIZE];

struct crypto_nxp_s32_hse_session {
	hseSrvDescriptor_t crypto_serv_desc;
	Hse_Ip_ReqType req_type;
	bool in_use;
	uint8_t channel;
	uint8_t *out_buff;
	struct k_mutex crypto_lock;
	hseKeyHandle_t key_handle;
	hseKeyInfo_t key_infor;
};

struct crypto_nxp_s32_hse_data {
	struct crypto_nxp_s32_hse_session sessions[HSE_IP_NUM_OF_CHANNELS_PER_MU];
	uint8_t mu_instance;
	Hse_Ip_MuStateType mu_state;
};

static struct crypto_nxp_s32_hse_session *crypto_nxp_s32_hse_get_session(const struct device *dev)
{
	struct crypto_nxp_s32_hse_data *data = dev->data;
	uint8_t mu_channel = Hse_Ip_GetFreeChannel(data->mu_instance);
	struct crypto_nxp_s32_hse_session *session;

	if (mu_channel != HSE_IP_INVALID_MU_CHANNEL_U8) {
		session = &data->sessions[mu_channel - 1];
		session->in_use = true;
		return session;
	}

	return NULL;
}

static inline void free_session(struct crypto_nxp_s32_hse_session *session)
{
	session->in_use = false;
}

static int crypto_nxp_s32_hse_aes_ecb_encrypt(struct cipher_ctx *ctx, struct cipher_pkt *pkt)
{
	struct crypto_nxp_s32_hse_session *session = ctx->drv_sessn_state;
	struct crypto_nxp_s32_hse_data *data = ctx->device->data;
	hseSymCipherSrv_t *cipher_serv = &(session->crypto_serv_desc.hseSrv.symCipherReq);

	__ASSERT_NO_MSG(pkt->in_len <= pkt->out_buf_max &&
			pkt->out_buf_max <= CONFIG_CRYPTO_NXP_S32_HSE_OUTPUT_BUFFER_SIZE);

	/* Update crypto service description input */
	cipher_serv->cipherBlockMode = HSE_CIPHER_BLOCK_MODE_ECB;
	cipher_serv->cipherDir = HSE_CIPHER_DIR_ENCRYPT;
	cipher_serv->pInput = HSE_PTR_TO_HOST_ADDR(pkt->in_buf);
	cipher_serv->inputLength = pkt->in_len;
	cipher_serv->pOutput = HSE_PTR_TO_HOST_ADDR(session->out_buff);

	k_mutex_lock(&session->crypto_lock, K_FOREVER);

	if (Hse_Ip_ServiceRequest(
		    data->mu_instance, session->channel, (Hse_Ip_ReqType *)&session->req_type,
		    (hseSrvDescriptor_t *)&session->crypto_serv_desc) != HSE_SRV_RSP_OK) {
		k_mutex_unlock(&session->crypto_lock);
		return -EIO;
	}

	memcpy(pkt->out_buf, session->out_buff, pkt->out_buf_max);

	k_mutex_unlock(&session->crypto_lock);

	return 0;
}

static int crypto_nxp_s32_hse_aes_ecb_decrypt(struct cipher_ctx *ctx, struct cipher_pkt *pkt)
{
	struct crypto_nxp_s32_hse_session *session = ctx->drv_sessn_state;
	struct crypto_nxp_s32_hse_data *data = ctx->device->data;
	hseSymCipherSrv_t *cipher_serv = &(session->crypto_serv_desc.hseSrv.symCipherReq);

	__ASSERT_NO_MSG(pkt->in_len <= pkt->out_buf_max &&
			pkt->out_buf_max <= CONFIG_CRYPTO_NXP_S32_HSE_OUTPUT_BUFFER_SIZE);

	/* Update crypto service description information */
	cipher_serv->cipherBlockMode = HSE_CIPHER_BLOCK_MODE_ECB;
	cipher_serv->cipherDir = HSE_CIPHER_DIR_DECRYPT;
	cipher_serv->pInput = HSE_PTR_TO_HOST_ADDR(pkt->in_buf);
	cipher_serv->inputLength = pkt->in_len;
	cipher_serv->pOutput = HSE_PTR_TO_HOST_ADDR(session->out_buff);

	k_mutex_lock(&session->crypto_lock, K_FOREVER);

	if (Hse_Ip_ServiceRequest(
		    data->mu_instance, session->channel, (Hse_Ip_ReqType *)&session->req_type,
		    (hseSrvDescriptor_t *)&session->crypto_serv_desc) != HSE_SRV_RSP_OK) {
		k_mutex_unlock(&session->crypto_lock);
		return -EIO;
	}

	memcpy(pkt->out_buf, session->out_buff, pkt->out_buf_max);

	k_mutex_unlock(&session->crypto_lock);

	return 0;
}

static int crypto_nxp_s32_hse_aes_cbc_encrypt(struct cipher_ctx *ctx, struct cipher_pkt *pkt,
					      uint8_t *iv)
{
	struct crypto_nxp_s32_hse_session *session = ctx->drv_sessn_state;
	struct crypto_nxp_s32_hse_data *data = ctx->device->data;
	hseSymCipherSrv_t *cipher_serv = &(session->crypto_serv_desc.hseSrv.symCipherReq);
	size_t iv_bytes;

	__ASSERT_NO_MSG(pkt->in_len <= pkt->out_buf_max &&
			pkt->out_buf_max <= CONFIG_CRYPTO_NXP_S32_HSE_OUTPUT_BUFFER_SIZE);

	if (ctx->flags & CAP_NO_IV_PREFIX) {
		iv_bytes = 0;
	} else {
		iv_bytes = CRYPTO_NXP_S32_HSE_BLOCK_KEY_LEN_BYTES;
		memcpy(pkt->out_buf, iv, CRYPTO_NXP_S32_HSE_BLOCK_KEY_LEN_BYTES);
	}

	/* Update crypto service description input */
	cipher_serv->cipherBlockMode = HSE_CIPHER_BLOCK_MODE_CBC;
	cipher_serv->cipherDir = HSE_CIPHER_DIR_ENCRYPT;
	cipher_serv->pIV = HSE_PTR_TO_HOST_ADDR(iv);
	cipher_serv->pInput = HSE_PTR_TO_HOST_ADDR(pkt->in_buf);
	cipher_serv->inputLength = pkt->in_len;
	cipher_serv->pOutput = HSE_PTR_TO_HOST_ADDR(session->out_buff);

	pkt->out_len = pkt->in_len + iv_bytes;

	k_mutex_lock(&session->crypto_lock, K_FOREVER);

	if (Hse_Ip_ServiceRequest(
		    data->mu_instance, session->channel, (Hse_Ip_ReqType *)&session->req_type,
		    (hseSrvDescriptor_t *)&session->crypto_serv_desc) != HSE_SRV_RSP_OK) {
		k_mutex_unlock(&session->crypto_lock);
		return -EIO;
	}

	memcpy(pkt->out_buf + iv_bytes, session->out_buff, pkt->out_buf_max - iv_bytes);

	k_mutex_unlock(&session->crypto_lock);

	return 0;
}

static int crypto_nxp_s32_hse_aes_cbc_decrypt(struct cipher_ctx *ctx, struct cipher_pkt *pkt,
					      uint8_t *iv)
{
	struct crypto_nxp_s32_hse_session *session = ctx->drv_sessn_state;
	struct crypto_nxp_s32_hse_data *data = ctx->device->data;
	hseSymCipherSrv_t *cipher_serv = &(session->crypto_serv_desc.hseSrv.symCipherReq);
	size_t iv_bytes;

	if (ctx->flags & CAP_NO_IV_PREFIX) {
		iv_bytes = 0;
	} else {
		iv_bytes = CRYPTO_NXP_S32_HSE_BLOCK_KEY_LEN_BYTES;
	}

	__ASSERT_NO_MSG(pkt->in_len - iv_bytes <= pkt->out_buf_max &&
			pkt->out_buf_max <= CONFIG_CRYPTO_NXP_S32_HSE_OUTPUT_BUFFER_SIZE);

	/* Update crypto service description information */
	cipher_serv->cipherBlockMode = HSE_CIPHER_BLOCK_MODE_CBC;
	cipher_serv->cipherDir = HSE_CIPHER_DIR_DECRYPT;
	cipher_serv->pIV = HSE_PTR_TO_HOST_ADDR(iv);
	cipher_serv->pInput = HSE_PTR_TO_HOST_ADDR(pkt->in_buf + iv_bytes);
	cipher_serv->inputLength = pkt->in_len - iv_bytes;
	cipher_serv->pOutput = HSE_PTR_TO_HOST_ADDR(session->out_buff);

	k_mutex_lock(&session->crypto_lock, K_FOREVER);

	if (Hse_Ip_ServiceRequest(
		    data->mu_instance, session->channel, (Hse_Ip_ReqType *)&session->req_type,
		    (hseSrvDescriptor_t *)&session->crypto_serv_desc) != HSE_SRV_RSP_OK) {
		k_mutex_unlock(&session->crypto_lock);
		return -EIO;
	}

	memcpy(pkt->out_buf, session->out_buff, pkt->out_buf_max);

	k_mutex_unlock(&session->crypto_lock);

	return 0;
}

static int crypto_nxp_s32_hse_aes_ctr_encrypt(struct cipher_ctx *ctx, struct cipher_pkt *pkt,
					      uint8_t *iv)
{
	struct crypto_nxp_s32_hse_session *session = ctx->drv_sessn_state;
	struct crypto_nxp_s32_hse_data *data = ctx->device->data;
	hseSymCipherSrv_t *cipher_serv = &(session->crypto_serv_desc.hseSrv.symCipherReq);
	uint8_t iv_key[CRYPTO_NXP_S32_HSE_BLOCK_KEY_LEN_BYTES] = {0};
	int iv_len = ctx->keylen - HSE_BITS_TO_BYTES(ctx->mode_params.ctr_info.ctr_len);

	__ASSERT_NO_MSG(pkt->in_len <= pkt->out_buf_max &&
			pkt->out_buf_max <= CONFIG_CRYPTO_NXP_S32_HSE_OUTPUT_BUFFER_SIZE);

	/* takes the last 4 bytes of the counter parameter as the true
	 * counter start. IV forms the first 12 bytes of the split counter.
	 */
	memcpy(iv_key, iv, iv_len);

	/* Update crypto service description input */
	cipher_serv->cipherBlockMode = HSE_CIPHER_BLOCK_MODE_CTR;
	cipher_serv->cipherDir = HSE_CIPHER_DIR_ENCRYPT;
	cipher_serv->pIV = HSE_PTR_TO_HOST_ADDR(&iv_key);
	cipher_serv->pInput = HSE_PTR_TO_HOST_ADDR(pkt->in_buf);
	cipher_serv->inputLength = pkt->in_len;
	cipher_serv->pOutput = HSE_PTR_TO_HOST_ADDR(session->out_buff);

	k_mutex_lock(&session->crypto_lock, K_FOREVER);

	if (Hse_Ip_ServiceRequest(
		    data->mu_instance, session->channel, (Hse_Ip_ReqType *)&session->req_type,
		    (hseSrvDescriptor_t *)&session->crypto_serv_desc) != HSE_SRV_RSP_OK) {
		k_mutex_unlock(&session->crypto_lock);
		return -EIO;
	}

	memcpy(pkt->out_buf, session->out_buff, pkt->out_buf_max);

	k_mutex_unlock(&session->crypto_lock);

	return 0;
}

static int crypto_nxp_s32_hse_aes_ctr_decrypt(struct cipher_ctx *ctx, struct cipher_pkt *pkt,
					      uint8_t *iv)
{
	struct crypto_nxp_s32_hse_session *session = ctx->drv_sessn_state;
	struct crypto_nxp_s32_hse_data *data = ctx->device->data;
	hseSymCipherSrv_t *cipher_serv = &(session->crypto_serv_desc.hseSrv.symCipherReq);
	uint8_t iv_key[CRYPTO_NXP_S32_HSE_BLOCK_KEY_LEN_BYTES] = {0};
	int iv_len = ctx->keylen - HSE_BITS_TO_BYTES(ctx->mode_params.ctr_info.ctr_len);

	__ASSERT_NO_MSG(pkt->in_len <= pkt->out_buf_max &&
			pkt->out_buf_max <= CONFIG_CRYPTO_NXP_S32_HSE_OUTPUT_BUFFER_SIZE);

	memcpy(iv_key, iv, iv_len);

	/* Update crypto service description information */
	cipher_serv->cipherBlockMode = HSE_CIPHER_BLOCK_MODE_CTR;
	cipher_serv->cipherDir = HSE_CIPHER_DIR_DECRYPT;
	cipher_serv->pIV = HSE_PTR_TO_HOST_ADDR(&iv_key);
	cipher_serv->pInput = HSE_PTR_TO_HOST_ADDR(pkt->in_buf);
	cipher_serv->inputLength = pkt->in_len;
	cipher_serv->pOutput = HSE_PTR_TO_HOST_ADDR(session->out_buff);

	k_mutex_lock(&session->crypto_lock, K_FOREVER);

	if (Hse_Ip_ServiceRequest(
		    data->mu_instance, session->channel, (Hse_Ip_ReqType *)&session->req_type,
		    (hseSrvDescriptor_t *)&session->crypto_serv_desc) != HSE_SRV_RSP_OK) {
		k_mutex_unlock(&session->crypto_lock);
		return -EIO;
	}

	memcpy(pkt->out_buf, session->out_buff, pkt->out_buf_max);

	k_mutex_unlock(&session->crypto_lock);

	return 0;
}

static int crypto_nxp_s32_hse_cipher_key_element_set(const struct device *dev,
						     struct crypto_nxp_s32_hse_session *session,
						     struct cipher_ctx *ctx)
{
	struct crypto_nxp_s32_hse_data *data = dev->data;
	hseImportKeySrv_t *import_key_serv = &(session->crypto_serv_desc.hseSrv.importKeyReq);

	session->req_type.eReqType = HSE_IP_REQTYPE_SYNC;
	session->req_type.u32Timeout = CRYPTO_NXP_S32_HSE_TIMEOUT;

	session->crypto_serv_desc.srvId = HSE_SRV_ID_IMPORT_KEY;

	session->key_infor.keyType = HSE_KEY_TYPE_AES;
	session->key_infor.keyBitLen = HSE_KEY128_BITS;
	session->key_infor.keyFlags = HSE_KF_USAGE_ENCRYPT | HSE_KF_USAGE_DECRYPT |
				      HSE_KF_USAGE_SIGN | HSE_KF_USAGE_VERIFY;
	session->key_infor.keyCounter = 0U;
	session->key_infor.smrFlags = 0U;
	session->key_infor.specific.aesBlockModeMask = HSE_KU_AES_BLOCK_MODE_ANY;
	session->key_infor.specific.aesBlockModeMask = (hseAesBlockModeMask_t)0U;
	session->key_infor.hseReserved[0U] = 0U;
	session->key_infor.hseReserved[1U] = 0U;

	/* The key import is not encrypted, nor authenticated */
	import_key_serv->cipher.cipherKeyHandle = HSE_INVALID_KEY_HANDLE;
	import_key_serv->keyContainer.authKeyHandle = HSE_INVALID_KEY_HANDLE;
	import_key_serv->pKeyInfo = HSE_PTR_TO_HOST_ADDR(&session->key_infor);
	import_key_serv->pKey[2] = HSE_PTR_TO_HOST_ADDR(ctx->key.bit_stream);
	import_key_serv->keyLen[2] = (uint16_t)ctx->keylen;
	import_key_serv->targetKeyHandle = (hseKeyHandle_t)session->key_handle;

	k_mutex_lock(&session->crypto_lock, K_FOREVER);

	if (Hse_Ip_ServiceRequest(
		    data->mu_instance, session->channel, (Hse_Ip_ReqType *)&session->req_type,
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

	if (ctx->keylen != CRYPTO_NXP_S32_HSE_BLOCK_KEY_LEN_BYTES) {
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
		free_session(session);
		LOG_ERR("Failed to import key catalog");
		return -EIO;
	}

	/* Update crypto crypto description input */
	session->req_type.eReqType = HSE_IP_REQTYPE_SYNC;
	session->req_type.u32Timeout = CRYPTO_NXP_S32_HSE_TIMEOUT;
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

	ARG_UNUSED(dev);

	session = ctx->drv_sessn_state;
	k_mutex_lock(&session->crypto_lock, K_FOREVER);
	memset(&session->req_type, 0, sizeof(Hse_Ip_ReqType));
	memset(&session->crypto_serv_desc, 0, sizeof(hseSrvDescriptor_t));
	free_session(session);
	k_mutex_unlock(&session->crypto_lock);

	return 0;
}

static int crypto_nxp_s32_hse_sha(struct hash_ctx *ctx, struct hash_pkt *pkt, bool finish)
{
	struct crypto_nxp_s32_hse_session *session = ctx->drv_sessn_state;
	struct crypto_nxp_s32_hse_data *data = ctx->device->data;
	hseHashSrv_t *hash_serv = &(session->crypto_serv_desc.hseSrv.hashReq);

	if (!finish) {
		return 0;
	}

	/* Update crypto crypto description input */
	hash_serv->pInput = HSE_PTR_TO_HOST_ADDR(pkt->in_buf);
	hash_serv->inputLength = pkt->in_len;

	k_mutex_lock(&session->crypto_lock, K_FOREVER);

	if (Hse_Ip_ServiceRequest(
		    data->mu_instance, session->channel, (Hse_Ip_ReqType *)&session->req_type,
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
	session->req_type.u32Timeout = CRYPTO_NXP_S32_HSE_TIMEOUT;

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

	ARG_UNUSED(dev);

	session = ctx->drv_sessn_state;
	k_mutex_lock(&session->crypto_lock, K_FOREVER);
	memset(&session->req_type, 0, sizeof(Hse_Ip_ReqType));
	memset(&session->crypto_serv_desc, 0, sizeof(hseSrvDescriptor_t));
	free_session(session);
	k_mutex_unlock(&session->crypto_lock);

	return 0;
}

static int crypto_nxp_s32_hse_query_caps(const struct device *dev)
{
	return CRYPTO_NXP_S32_HSE_HASH_CAPS | CRYPTO_NXP_S32_HSE_CIPHER_CAPS;
}

#ifdef CONFIG_CRYPTO_NXP_S32_HSE_FORMAT_KEY_CATALOG
static int crypto_nxp_s32_hse_format_key_element(const struct device *dev)
{
	struct crypto_nxp_s32_hse_data *data = dev->data;
	hseSrvDescriptor_t crypto_serv_desc;
	Hse_Ip_ReqType crypto_req_type;
	hseFormatKeyCatalogsSrv_t *format_key_serv =
		&(crypto_serv_desc.hseSrv.formatKeyCatalogsReq);
	uint8_t channel = Hse_Ip_GetFreeChannel(data->mu_instance);
	hseKeyGroupCfgEntry_t crypto_ram_key_catalog[] = {{HSE_MU0_MASK,
							   HSE_KEY_OWNER_ANY,
							   HSE_KEY_TYPE_AES,
							   HSE_MAX_RAM_KEYS,
							   HSE_KEY128_BITS,
							   {0U, 0U}},
							  {0U, 0U, 0U, 0U, 0U, {0U, 0U}}};

	hseKeyGroupCfgEntry_t crypto_nvm_key_catalog[] = {{HSE_MU0_MASK,
							   HSE_KEY_OWNER_CUST,
							   HSE_KEY_TYPE_AES,
							   HSE_MAX_NVM_SYM_KEYS,
							   HSE_KEY128_BITS,
							   {0U, 0U}},
							  {0U, 0U, 0U, 0U, 0U, {0U, 0U}}};

	if (channel == HSE_IP_INVALID_MU_CHANNEL_U8) {
		return -EIO;
	}

	crypto_req_type.eReqType = HSE_IP_REQTYPE_SYNC;
	crypto_req_type.u32Timeout = CRYPTO_NXP_S32_HSE_TIMEOUT;

	crypto_serv_desc.srvId = HSE_SRV_ID_FORMAT_KEY_CATALOGS;
	format_key_serv->pNvmKeyCatalogCfg = HSE_PTR_TO_HOST_ADDR(crypto_nvm_key_catalog);
	format_key_serv->pRamKeyCatalogCfg = HSE_PTR_TO_HOST_ADDR(crypto_ram_key_catalog);

	if (Hse_Ip_ServiceRequest(data->mu_instance, channel, &crypto_req_type,
				  &crypto_serv_desc) != HSE_SRV_RSP_OK) {
		return -EIO;
	}

	return 0;
}
#endif

static int crypto_nxp_s32_hse_init(const struct device *dev)
{
	struct crypto_nxp_s32_hse_data *data = dev->data;
	struct crypto_nxp_s32_hse_session *session;

	if (Hse_Ip_Init(data->mu_instance, &data->mu_state) != HSE_IP_STATUS_SUCCESS) {
		LOG_ERR("Failed to initialize MU instance %d", data->mu_instance);
		return -EAGAIN;
	}

#ifdef CONFIG_CRYPTO_NXP_S32_HSE_FORMAT_KEY_CATALOG
	if (crypto_nxp_s32_hse_format_key_element(dev)) {
		LOG_ERR("Failed to format key catalog");
		return -EAGAIN;
	}
#endif

	for (size_t i = 0; i < HSE_IP_NUM_OF_CHANNELS_PER_MU; i++) {
		session = &data->sessions[i];
		k_mutex_init(&session->crypto_lock);
	}

	return 0;
}

static struct crypto_driver_api crypto_nxp_s32_hse_api = {
	.cipher_begin_session = crypto_nxp_s32_hse_cipher_begin_session,
	.cipher_free_session = crypto_nxp_s32_hse_cipher_free_session,
	.query_hw_caps = crypto_nxp_s32_hse_query_caps,
	.hash_begin_session = crypto_nxp_s32_hse_hash_begin_session,
	.hash_free_session = crypto_nxp_s32_hse_hash_free_session,
};

#define CRYPTO_NXP_S32_HSE_SESSION_CFG(indx, _)                                                    \
	{                                                                                          \
		.channel = indx + 1, .out_buff = &crypto_out_buff[indx][0],                        \
		.key_handle = GET_KEY_HANDLE(HSE_KEY_CATALOG_ID_RAM,                               \
					     CONFIG_CRYPTO_NXP_S32_HSE_AES_KEY_GROUP_ID, indx),    \
	}

#define CRYPTO_NXP_S32_HSE_INIT_SESSION()                                                          \
	LISTIFY(__DEBRACKET HSE_IP_NUM_OF_CHANNELS_PER_MU, CRYPTO_NXP_S32_HSE_SESSION_CFG, (,))

#define CRYPTO_NXP_S32_HSE_INIT_DEVICE(n)                                                          \
	static struct crypto_nxp_s32_hse_data crypto_nxp_s32_hse_data_##n = {                      \
		.sessions =                                                                        \
			{                                                                          \
				CRYPTO_NXP_S32_HSE_INIT_SESSION(),                                 \
			},                                                                         \
		.mu_instance = CRYPTO_NXP_S32_HSE_MU_GET_INSTANCE(n),                              \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &crypto_nxp_s32_hse_init, NULL, &crypto_nxp_s32_hse_data_##n,     \
			      NULL, POST_KERNEL, CONFIG_CRYPTO_INIT_PRIORITY,                      \
			      &crypto_nxp_s32_hse_api);

DT_INST_FOREACH_STATUS_OKAY(CRYPTO_NXP_S32_HSE_INIT_DEVICE)
