/*
 * Copyright (c) 2017 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Shim layer for mbedTLS, crypto API compliant.
 */

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_CRYPTO_LEVEL
#include <logging/sys_log.h>

#include <crypto/cipher.h>
#include <device.h>
#include <entropy.h>
#include <string.h>
#include <errno.h>
#include <init.h>
#include <kernel.h>

#if !defined(CONFIG_MBEDTLS_CFG_FILE)
#include "mbedtls/config.h"
#else
#include CONFIG_MBEDTLS_CFG_FILE
#endif /* CONFIG_MBEDTLS_CFG_FILE */

#if defined(MBEDTLS_MEMORY_BUFFER_ALLOC_C)
#include "mbedtls/memory_buffer_alloc.h"
#else
#error "You need to define MBEDTLS_MEMORY_BUFFER_ALLOC_C"
#endif /* MBEDTLS_MEMORY_BUFFER_ALLOC_C */

#include <mbedtls/aes.h>
#include <mbedtls/ccm.h>

#if defined(CONFIG_MBEDTLS_ENABLE_HMAC_DRBG)
#include <mbedtls/hmac_drbg.h>
#endif

#if defined(CONFIG_MBEDTLS_ENABLE_CMAC)
#include <mbedtls/cmac.h>
#include <mbedtls/md.h>
#endif

#if defined(CONFIG_MBEDTLS_ENABLE_ECC_DH)
#include <mbedtls/ecp.h>
#include <mbedtls/bignum.h>
#include <mbedtls/ecdh.h>
#endif

#define MTLS_SUPPORT (CAP_RAW_KEY | CAP_SEPARATE_IO_BUFS | CAP_SYNC_OPS)

struct mtls_shim_session {
	enum cipher_algo algo;
	enum cipher_mode mode;
	union {
#if defined(CONFIG_CRYPTO_ENABLE_AES_CCM)
		mbedtls_ccm_context mtls;
#endif
#if defined(CONFIG_CRYPTO_ENABLE_AES_CBC) || \
		defined(CONFIG_CRYPTO_ENABLE_AES_CTR)
		mbedtls_aes_context aes;
#endif
#if defined(CONFIG_MBEDTLS_ENABLE_HMAC_DRBG)
		mbedtls_hmac_drbg_context hmac_drbg;
#endif
#if defined(CONFIG_MBEDTLS_ENABLE_CMAC)
		mbedtls_cipher_context_t cipher;
#endif
	};
};

K_MEM_POOL_DEFINE(mtls_shim_session_pool,
	(sizeof(struct mtls_shim_session) + sizeof(struct k_mem_block_id)),
	(sizeof(struct mtls_shim_session) + sizeof(struct k_mem_block_id)),
	CONFIG_CRYPTO_MAX_SESSION,
	__alignof__(struct mtls_shim_session));

#if defined(CONFIG_MBEDTLS_ENABLE_HMAC_DRBG) || \
	defined(CONFIG_MBEDTLS_ENABLE_ECC_DH)

static struct device *hwrng;

static int mtls_get_entropy_from_device(void *data,
					unsigned char *output,
					size_t output_len)
{
	unsigned char *orig_output = output;
	size_t orig_len = output_len;

	ARG_UNUSED(data);

	while (output_len) {
		u16_t to_read = min(output_len, 0xffff);
		int r;

		r = entropy_get_entropy(hwrng, output, to_read);
		if (!r) {
			output_len -= to_read;
			output += to_read;
		} else if (r < 0) {
			explicit_bzero(orig_output, orig_len);

			return r;
		}
	}

	return 0;
}
#endif /* CONFIG_MBEDTLS_ENABLE_HMAC_DRBG || CONFIG_MBEDTLS_ENABLE_ECC_DH */

#if defined(CONFIG_CRYPTO_ENABLE_AES_CCM)
static int mtls_ccm_encrypt_auth(struct cipher_ctx *ctx,
				 struct cipher_aead_pkt *apkt,
				 u8_t *nonce)
{
	struct mtls_shim_session *session = ctx->drv_sessn_state;
	int ret;

	ret = mbedtls_ccm_encrypt_and_tag(&session->mtls, apkt->pkt->in_len,
					  nonce,
					  ctx->mode_params.ccm_info.nonce_len,
					  apkt->ad, apkt->ad_len,
					  apkt->pkt->in_buf,
					  apkt->pkt->out_buf, apkt->tag,
					  ctx->mode_params.ccm_info.tag_len);
	if (ret) {
		SYS_LOG_ERR("Could non encrypt/auth (%d)", ret);

		/*ToDo: try to return relevant code depending on ret? */
		return -EINVAL;
	}

	/* This is equivalent to what the TinyCrypt shim does in
	 * do_ccm_encrypt_mac().
	 */
	apkt->pkt->out_len = apkt->pkt->in_len;
	apkt->pkt->out_len += ctx->mode_params.ccm_info.tag_len;

	return 0;
}

static int mtls_ccm_decrypt_auth(struct cipher_ctx *ctx,
				 struct cipher_aead_pkt *apkt,
				 u8_t *nonce)
{
	struct mtls_shim_session *session = ctx->drv_sessn_state;
	int ret;

	ret = mbedtls_ccm_auth_decrypt(&session->mtls, apkt->pkt->in_len,
				       nonce,
				       ctx->mode_params.ccm_info.nonce_len,
				       apkt->ad, apkt->ad_len,
				       apkt->pkt->in_buf,
				       apkt->pkt->out_buf, apkt->tag,
				       ctx->mode_params.ccm_info.tag_len);
	if (ret) {
		SYS_LOG_ERR("Could non decrypt/auth (%d)", ret);

		/*ToDo: try to return relevant code depending on ret? */
		return -EINVAL;
	}

	apkt->pkt->out_len = apkt->pkt->in_len;
	apkt->pkt->out_len += ctx->mode_params.ccm_info.tag_len;

	return 0;
}
#endif

static struct mtls_shim_session *mtls_alloc_session(enum cipher_algo algo,
						    enum cipher_mode mode)
{
	struct mtls_shim_session *ret;

	ret = k_mem_pool_malloc(&mtls_shim_session_pool, sizeof(*ret));
	if (ret == NULL) {
		SYS_LOG_ERR("No free session for now");
	} else {
		ret->algo = algo;
		ret->mode = mode;
	}

	return ret;
}

static void mtls_free_session(struct mtls_shim_session *session)
{
	switch (session->algo) {
	case CRYPTO_CIPHER_ALGO_AES:
#if defined(CONFIG_CRYPTO_ENABLE_AES_CCM)
		if (session->mode == CRYPTO_CIPHER_MODE_CCM)
			mbedtls_ccm_free(&session->mtls);
#endif
#if defined(CONFIG_CRYPTO_ENABLE_AES_CBC) || \
		defined(CONFIG_CRYPTO_ENABLE_AES_CTR)
		if ((session->mode == CRYPTO_CIPHER_MODE_CBC) ||
		     (session->mode == CRYPTO_CIPHER_MODE_CTR))
			mbedtls_aes_free(&session->aes);
#endif
		break;

	case CRYPTO_CIPHER_ALGO_PRNG:
#if defined(CONFIG_MBEDTLS_ENABLE_HMAC_DRBG)
		mbedtls_hmac_drbg_free(&session->hmac_drbg);
#endif
		break;

	case CRYPTO_CIPHER_ALGO_MAC:
#if defined(CONFIG_MBEDTLS_ENABLE_CMAC)
		mbedtls_cipher_free(&session->cipher);
#endif
		break;

	case CRYPTO_CIPHER_ALGO_ECC:
		/* Do nothing. */
		break;
	}

	memset(session, 0, sizeof(*session));
	k_free(session);
}

#if defined(CONFIG_CRYPTO_ENABLE_AES_CBC)
static int mtls_aes_cbc_decrypt(struct cipher_ctx *ctx, struct cipher_pkt *op,
				u8_t *iv)
{
	int ret;
	struct mtls_shim_session *session = ctx->drv_sessn_state;

	ret = mbedtls_aes_crypt_cbc(&session->aes, MBEDTLS_AES_DECRYPT,
				    op->in_len, iv,
				    op->in_buf,
				    op->out_buf);
	if (ret) {
		SYS_LOG_ERR("Could non decrypt/auth (%d)", ret);

		/*TODO: try to return relevant code depending on ret? */
		return -EINVAL;
	}

	op->out_len = op->in_len;
	return 0;
}

static int mtls_aes_cbc_encrypt(struct cipher_ctx *ctx, struct cipher_pkt *op,
				u8_t *iv)
{
	int ret;
	u8_t iv_copy[16];
	struct mtls_shim_session *session = ctx->drv_sessn_state;

	(void)memcpy(iv_copy, iv, sizeof(iv_copy));

	ret = mbedtls_aes_crypt_cbc(&session->aes, MBEDTLS_AES_ENCRYPT,
				    op->in_len, iv_copy,
				    op->in_buf,
				    op->out_buf);
	if (ret) {
		SYS_LOG_ERR("Could non decrypt/auth (%d)", ret);

		/*ToDo: try to return relevant code depending on ret? */
		return -EINVAL;
	}

	op->out_len = op->in_len;

	return 0;
}

static int mtls_session_setup_aes_cbc(struct cipher_ctx *ctx,
				      enum cipher_algo algo,
				      enum cipher_mode mode,
				      enum cipher_op op_type)
{
	int ret;
	struct mtls_shim_session *session;

	session = mtls_alloc_session(CRYPTO_CIPHER_ALGO_AES, mode);
	if (!session) {
		return -ENOSPC;
	}

	mbedtls_aes_init(&session->aes);

	if (op_type == CRYPTO_CIPHER_OP_ENCRYPT) {
		ctx->ops.cbc_crypt_hndlr = mtls_aes_cbc_encrypt;
		ret = mbedtls_aes_setkey_enc(&session->aes,
					     ctx->key.bit_stream,
					     ctx->keylen * 8);
	} else {
		ctx->ops.cbc_crypt_hndlr = mtls_aes_cbc_decrypt;
		ret = mbedtls_aes_setkey_dec(&session->aes,
					     ctx->key.bit_stream,
					     ctx->keylen * 8);
	}

	if (ret) {
		SYS_LOG_ERR("Could not setup the key (%d)", ret);
		mtls_free_session(session);
		return -EINVAL;
	}

	ctx->drv_sessn_state = session;
	return ret;
}
#endif

#if defined(CONFIG_CRYPTO_ENABLE_AES_CTR)
static int mtls_aes_ctr_crypt(struct cipher_ctx *ctx, struct cipher_pkt *op,
				u8_t *iv)
{
	int ret;
	size_t out_len = 0;
	u8_t nonce_counter[16] = {0};
	u8_t stream_block[16] = {0};
	struct mtls_shim_session *session = ctx->drv_sessn_state;
	int ivlen = ctx->keylen - (ctx->mode_params.ctr_info.ctr_len >> 3);

	(void)memcpy(nonce_counter, iv, ivlen);

	ret = mbedtls_aes_crypt_ctr(&session->aes, op->in_len, &out_len,
				    nonce_counter, stream_block,
				    op->in_buf,
				    op->out_buf);
	if (ret) {
		SYS_LOG_ERR("Could non decrypt/encrypt (%d)", ret);
		return -EINVAL;
	}
	op->out_len = out_len;

	return 0;
}

static int mtls_session_setup_aes_ctr(struct cipher_ctx *ctx,
				      enum cipher_algo algo,
				      enum cipher_mode mode,
				      enum cipher_op op_type)
{
	int ret;
	struct mtls_shim_session *session;

	session = mtls_alloc_session(CRYPTO_CIPHER_ALGO_AES,
				     CRYPTO_CIPHER_MODE_CTR);
	if (!session) {
		return -ENOSPC;
	}

	mbedtls_aes_init(&session->aes);

	ctx->ops.ctr_crypt_hndlr = mtls_aes_ctr_crypt;
	ret = mbedtls_aes_setkey_enc(&session->aes,
				     ctx->key.bit_stream, ctx->keylen * 8);

	if (ret) {
		SYS_LOG_ERR("Could not setup the key (%d)", ret);
		mtls_free_session(session);
		return -EINVAL;
	}

	ctx->drv_sessn_state = session;
	return ret;
}
#endif

#if defined(CONFIG_CRYPTO_ENABLE_AES_CCM)
static int mtls_session_setup_aes_ccm(struct cipher_ctx *ctx,
				      enum cipher_algo algo,
				      enum cipher_mode mode,
				      enum cipher_op op_type)
{
	int ret;
	struct mtls_shim_session *session;

	session = mtls_alloc_session(CRYPTO_CIPHER_ALGO_AES, mode);
	if (!session) {
		return -ENOSPC;
	}

	if (op_type == CRYPTO_CIPHER_OP_ENCRYPT) {
		ctx->ops.ccm_crypt_hndlr = mtls_ccm_encrypt_auth;
	} else {
		ctx->ops.ccm_crypt_hndlr = mtls_ccm_decrypt_auth;
	}

	mbedtls_ccm_init(&session->mtls);

	ret = mbedtls_ccm_setkey(&session->mtls, MBEDTLS_CIPHER_ID_AES,
				 ctx->key.bit_stream, ctx->keylen * 8);
	if (ret) {
		SYS_LOG_ERR("Could not setup the key (%d)", ret);
		mtls_free_session(session);
		return -EINVAL;
	}

	ctx->drv_sessn_state = session;
	return 0;
}
#endif

static int mtls_session_setup_aes(struct cipher_ctx *ctx,
				  enum cipher_algo algo,
				  enum cipher_mode mode,
				  enum cipher_op op_type)
{
	int ret = 0;

	/* TODO: It does not seem necessary, mbedTLS supports bigger keys.
	 * keeping this size due TinyCrypt compatibility.
	 */
	if (ctx->keylen != 16) {
		SYS_LOG_ERR("%u key size is not supported", ctx->keylen);
		return -EINVAL;
	}

	switch (mode) {
#if defined(CONFIG_CRYPTO_ENABLE_AES_CBC)
	case CRYPTO_CIPHER_MODE_CBC:
		ret = mtls_session_setup_aes_cbc(ctx, algo, mode, op_type);
		break;
#endif
#if defined(CONFIG_CRYPTO_ENABLE_AES_CTR)
	case CRYPTO_CIPHER_MODE_CTR:
		ret = mtls_session_setup_aes_ctr(ctx, algo, mode, op_type);
		break;
#endif
#if defined(CONFIG_CRYPTO_ENABLE_AES_CCM)
	case CRYPTO_CIPHER_MODE_CCM:
		ret = mtls_session_setup_aes_ccm(ctx, algo, mode, op_type);
		break;
#endif
	default:
		SYS_LOG_ERR("Unsupported mode for AES algo: %d", mode);
		return -EINVAL;
	}

	return ret;
}

#if defined(CONFIG_MBEDTLS_ENABLE_HMAC_DRBG)
static int mtls_hmac_prng_op(struct cipher_ctx *ctx,
			     struct cipher_prng_pkt *pkt)
{
	struct mtls_shim_session *session = ctx->drv_sessn_state;
	int r;

	if (pkt->reseed) {
		r = mbedtls_hmac_drbg_reseed(&session->hmac_drbg,
					     pkt->additional_input,
					     pkt->additional_input_len);
	} else {
		r = mbedtls_hmac_drbg_random(&session->hmac_drbg,
					     pkt->data,
					     pkt->data_len);
	}

	switch (r) {
	case 0:
		return 0;
	case MBEDTLS_ERR_HMAC_DRBG_REQUEST_TOO_BIG:
		return -EMSGSIZE;
	case MBEDTLS_ERR_HMAC_DRBG_ENTROPY_SOURCE_FAILED:
	default:
		return -EIO;
	}
}

static int mtls_session_setup_prng(struct cipher_ctx *ctx,
				   enum cipher_mode mode)
{
	struct mtls_shim_session *session;
	const mbedtls_md_info_t *md;
	int r;

	if (mode != CRYPTO_CIPHER_MODE_PRNG_HMAC) {
		SYS_LOG_ERR("Only HMAC supported for PRNG algo");
		return -EINVAL;
	}

	session = mtls_alloc_session(CRYPTO_CIPHER_ALGO_PRNG, mode);
	if (!session) {
		return -ENOSPC;
	}

	md = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
	if (!md) {
		goto out;
	}

	mbedtls_hmac_drbg_init(&session->hmac_drbg);
	r = mbedtls_hmac_drbg_seed(&session->hmac_drbg, md,
				   mtls_get_entropy_from_device, NULL,
				   (const u8_t *)ctx->key.personalization.data,
				   ctx->key.personalization.len);
	if (r != 0) {
		goto out;
	}

	ctx->ops.prng_crypt_hndlr = mtls_hmac_prng_op;
	ctx->drv_sessn_state = session;

	return 0;

out:
	mtls_free_session(session);
	return -EINVAL;
}
#endif /* CONFIG_MBEDTLS_ENABLE_HMAC_DRBG */

#if defined(CONFIG_MBEDTLS_ENABLE_CMAC)
static int mtls_mac_op(struct cipher_ctx *ctx, struct cipher_mac_pkt *pkt)
{
	struct mtls_shim_session *session = ctx->drv_sessn_state;
	int r;

	if (pkt->finalize) {
		r = mbedtls_cipher_cmac_finish(&session->cipher,
					       pkt->data);
	} else {
		r = mbedtls_cipher_cmac_update(&session->cipher,
					       pkt->data, pkt->data_len);
	}

	switch (r) {
	case 0:
		return 0;
	case MBEDTLS_ERR_MD_BAD_INPUT_DATA:
		return -EINVAL;
	default:
		return -EIO;
	}
}

static int mtls_session_setup_mac(struct cipher_ctx *ctx,
				  enum cipher_mode mode,
				  enum cipher_op op_type)
{
	struct mtls_shim_session *session;
	const mbedtls_cipher_info_t *ci;
	mbedtls_cipher_type_t type;
	mbedtls_operation_t op;
	int r;

	if (!(mode & CRYPTO_CIPHER_MODE_CMAC)) {
		SYS_LOG_ERR("Only CMAC supported for MAC algo");
		return -EINVAL;
	}

	if (mode & CRYPTO_CIPHER_MODE_ECB) {
		type = MBEDTLS_CIPHER_AES_128_ECB;
	} else if (mode & CRYPTO_CIPHER_MODE_CBC) {
		type = MBEDTLS_CIPHER_AES_128_CBC;
	} else if (mode & CRYPTO_CIPHER_MODE_CTR) {
		type = MBEDTLS_CIPHER_AES_128_CTR;
	} else if (mode & CRYPTO_CIPHER_MODE_CCM) {
		type = MBEDTLS_CIPHER_AES_128_CCM;
	} else {
		/* Defaults to AES, 128-bit keys, ECB mode. */
		type = MBEDTLS_CIPHER_AES_128_ECB;
	}
	ci = mbedtls_cipher_info_from_type(type);
	if (!ci) {
		return -EINVAL;
	}

	session = mtls_alloc_session(CRYPTO_CIPHER_ALGO_MAC, mode);
	if (!session) {
		return -ENOSPC;
	}

	mbedtls_cipher_init(&session->cipher);

	r = mbedtls_cipher_setup(&session->cipher, ci);
	if (r != 0) {
		mtls_free_session(session);
		return -EINVAL;
	}

	if (op_type == CRYPTO_CIPHER_OP_ENCRYPT) {
		op = MBEDTLS_ENCRYPT;
	} else {
		op = MBEDTLS_DECRYPT;
	}

	r = mbedtls_cipher_cmac_starts(&session->cipher,
				  ctx->key.bit_stream, 128);
	if (r != 0) {
		mtls_free_session(session);
		return -EINVAL;
	}

	ctx->ops.mac_crypt_hndlr = mtls_mac_op;
	ctx->ops.cipher_mode = mode;
	ctx->drv_sessn_state = session;

	return 0;
}
#endif /* CONFIG_MBEDTLS_ENABLE_CMAC */

#if defined(CONFIG_MBEDTLS_ENABLE_ECC_DH)
static int mtls_ecc_gen_key(struct cipher_ctx *ctx,
			    struct cipher_ecc_pkt *pkt)
{
	mbedtls_ecp_keypair keypair;
	u8_t buffer[pkt->public_key_len + 1];
	size_t olen;
	int r;

	ARG_UNUSED(ctx);

	if (pkt->curve != CIPHER_ECC_CURVE_SECP256R1) {
		return -EINVAL;
	}

	mbedtls_ecp_keypair_init(&keypair);
	r = mbedtls_ecp_gen_key(MBEDTLS_ECP_DP_SECP256R1, &keypair,
				mtls_get_entropy_from_device, NULL);
	if (r != 0) {
		goto free_keypair;
	}

	r = mbedtls_mpi_write_binary(&keypair.d,
				     pkt->private_key,
				     pkt->private_key_len);
	if (r != 0) {
		goto free_keypair;
	}

	r = mbedtls_ecp_point_write_binary(&keypair.grp,
					   &keypair.Q,
					   MBEDTLS_ECP_PF_UNCOMPRESSED,
					   &olen,
					   buffer, sizeof(buffer));
	(void)memcpy(pkt->public_key, buffer + 1, pkt->public_key_len);

free_keypair:
	mbedtls_ecp_keypair_free(&keypair);

	return r == 0 ? 0 : -EIO;
}

static int mtls_ecc_validate_pubkey(struct cipher_ctx *ctx,
				    struct cipher_ecc_pkt *pkt)
{
	mbedtls_ecp_group grp = { };
	mbedtls_ecp_point pt = { };
	u8_t buffer[pkt->public_key_len + 1];
	int r;

	ARG_UNUSED(ctx);

	if (pkt->curve != CIPHER_ECC_CURVE_SECP256R1) {
		return -EINVAL;
	}

	buffer[0] = 0x04;
	(void)memcpy(buffer + 1, pkt->public_key, pkt->public_key_len);

	r = mbedtls_ecp_group_load(&grp, MBEDTLS_ECP_DP_SECP256R1);
	if (r != 0) {
		return -EINVAL;
	}

	mbedtls_ecp_point_init(&pt);
	r = mbedtls_ecp_point_read_binary(&grp, &pt, buffer, sizeof(buffer));
	if (r == 0) {
		r = mbedtls_ecp_check_pubkey(&grp, &pt);
	}

	mbedtls_ecp_point_free(&pt);
	mbedtls_ecp_group_free(&grp);

	return r == 0 ? 0 : -EINVAL;
}

static int mtls_ecc_gen_shared_key(struct cipher_ctx *ctx,
				   struct cipher_ecc_pkt *pkt)
{
	mbedtls_ecp_keypair keypair;
	u8_t buffer[pkt->public_key_len + 1];
	mbedtls_ecp_point Q;
	mbedtls_mpi z;
	mbedtls_mpi d;
	int r;

	ARG_UNUSED(ctx);

	if (pkt->curve != CIPHER_ECC_CURVE_SECP256R1) {
		return -EINVAL;
	}

	mbedtls_ecp_keypair_init(&keypair);
	mbedtls_ecp_group_free(&keypair.grp);
	r = mbedtls_ecp_group_load(&keypair.grp, MBEDTLS_ECP_DP_SECP256R1);
	if (r != 0) {
		goto free_keypair;
	}

	mbedtls_ecp_point_init(&Q);
	buffer[0] = 0x04;
	(void)memcpy(buffer + 1, pkt->public_key, pkt->public_key_len);

	r = mbedtls_ecp_point_read_binary(&keypair.grp, &Q, buffer,
					  sizeof(buffer));
	if (r != 0) {
		goto free_Q;
	}

	mbedtls_mpi_init(&z);
	r = mbedtls_mpi_read_binary(&z, pkt->private_key, pkt->private_key_len);
	if (r != 0) {
		goto free_z;
	}

	mbedtls_mpi_init(&d);

	r = mbedtls_ecdh_compute_shared(&keypair.grp,
					&d, &Q, &z,
					mtls_get_entropy_from_device, NULL);
	if (r != 0) {
		goto free_d;
	}

	r = mbedtls_mpi_write_binary(&d, pkt->shared_secret,
				     pkt->shared_secret_len);

free_d:
	mbedtls_mpi_free(&d);
free_z:
	mbedtls_mpi_free(&z);
free_Q:
	mbedtls_ecp_point_free(&Q);
free_keypair:
	mbedtls_ecp_keypair_free(&keypair);

	return r == 0 ? 0 : -EIO;
}

static int mtls_session_setup_ecc(struct cipher_ctx *ctx,
				  enum cipher_mode mode,
				  enum cipher_op op)
{
	struct mtls_shim_session *data;

	if (mode != CRYPTO_CIPHER_MODE_NONE) {
		SYS_LOG_ERR("Only NONE mode supported for ECC algo");
		return -EINVAL;
	}

	switch (op) {
	case CRYPTO_CIPHER_OP_ECC_GEN_KEY:
		ctx->ops.ecc_crypt_hndlr = mtls_ecc_gen_key;
		break;
	case CRYPTO_CIPHER_OP_ECC_VALIDATE_PUBKEY:
		ctx->ops.ecc_crypt_hndlr = mtls_ecc_validate_pubkey;
		break;
	case CRYPTO_CIPHER_OP_ECC_GEN_SHARED_KEY:
		ctx->ops.ecc_crypt_hndlr = mtls_ecc_gen_shared_key;
		break;
	default:
		return -EINVAL;
	}

	data = mtls_alloc_session(CRYPTO_CIPHER_ALGO_ECC, mode);
	if (!data) {
		return -ENOSPC;
	}

	ctx->ops.cipher_mode = mode;
	ctx->drv_sessn_state = data;

	return 0;
}
#endif /* CONFIG_MBEDTLS_ENABLE_ECC_DH */

static int mtls_session_setup(struct device *dev,
			      struct cipher_ctx *ctx,
			      enum cipher_algo algo,
			      enum cipher_mode mode,
			      enum cipher_op op_type)
{
	ARG_UNUSED(dev);

	if (ctx->flags & ~(MTLS_SUPPORT)) {
		SYS_LOG_ERR("Unsupported flag");
		return -EINVAL;
	}

	switch (algo) {
	case CRYPTO_CIPHER_ALGO_AES:
		return mtls_session_setup_aes(ctx, algo, mode, op_type);
#if defined(CONFIG_MBEDTLS_ENABLE_ECC_DH)
	case CRYPTO_CIPHER_ALGO_ECC:
		return mtls_session_setup_ecc(ctx, mode, op_type);
#endif
#if defined(CONFIG_MBEDTLS_ENABLE_HMAC_DRBG)
	case CRYPTO_CIPHER_ALGO_PRNG:
		return mtls_session_setup_prng(ctx, mode);
#endif
#if defined(CONFIG_MBEDTLS_ENABLE_CMAC)
	case CRYPTO_CIPHER_ALGO_MAC:
		return mtls_session_setup_mac(ctx, mode, op_type);
#endif
	default:
		SYS_LOG_ERR("Unsupported algo for mbedTLS shim: %d", algo);
		return -ENOTSUP;
	}
}

static int mtls_session_dev_free(struct device *dev, struct cipher_ctx *ctx)
{
	struct mtls_shim_session *session = ctx->drv_sessn_state;

	ARG_UNUSED(dev);

	mtls_free_session(session);

	return 0;
}

static int mtls_query_caps(struct device *dev)
{
	return MTLS_SUPPORT;
}

static int mtls_shim_init(struct device *dev)
{
#if defined(CONFIG_MBEDTLS_ENABLE_HMAC_DRBG) || \
	defined(CONFIG_MBEDTLS_ENABLE_ECC_DH)
	hwrng = device_get_binding(CONFIG_ENTROPY_NAME);
	if (!hwrng) {
		return -ENODEV;
	}
#endif

	return 0;
}

static struct crypto_driver_api mtls_crypto_funcs = {
	.begin_session = mtls_session_setup,
	.free_session = mtls_session_dev_free,
	.crypto_async_callback_set = NULL,
	.query_hw_caps = mtls_query_caps,
};

DEVICE_AND_API_INIT(crypto_mtls, CONFIG_CRYPTO_NAME,
		    &mtls_shim_init, NULL, NULL,
		    POST_KERNEL, CONFIG_CRYPTO_INIT_PRIORITY,
		    (void *)&mtls_crypto_funcs);
