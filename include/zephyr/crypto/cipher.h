/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Crypto Cipher structure definitions
 *
 * This file contains the Crypto Abstraction layer structures.
 *
 * [Experimental] Users should note that the Structures can change
 * as a part of ongoing development.
 */

#ifndef ZEPHYR_INCLUDE_CRYPTO_CIPHER_H_
#define ZEPHYR_INCLUDE_CRYPTO_CIPHER_H_

#include <zephyr/device.h>
#include <zephyr/sys/util.h>
/**
 * @addtogroup crypto_cipher
 * @{
 */


/** Cipher Algorithm */
enum cipher_algo {
	CRYPTO_CIPHER_ALGO_AES = 1,
};

/** Cipher Operation */
enum cipher_op {
	CRYPTO_CIPHER_OP_DECRYPT = 0,
	CRYPTO_CIPHER_OP_ENCRYPT = 1,
};

/**
 * Possible cipher mode options.
 *
 * More to be added as required.
 */
enum cipher_mode {
	CRYPTO_CIPHER_MODE_ECB = 1,
	CRYPTO_CIPHER_MODE_CBC = 2,
	CRYPTO_CIPHER_MODE_CTR = 3,
	CRYPTO_CIPHER_MODE_CCM = 4,
	CRYPTO_CIPHER_MODE_GCM = 5,
};

/* Forward declarations */
struct cipher_aead_pkt;
struct cipher_ctx;
struct cipher_pkt;

typedef int (*block_op_t)(struct cipher_ctx *ctx, struct cipher_pkt *pkt);

/* Function signatures for encryption/ decryption using standard cipher modes
 * like  CBC, CTR, CCM.
 */
typedef int (*cbc_op_t)(struct cipher_ctx *ctx, struct cipher_pkt *pkt,
			uint8_t *iv);

typedef int (*ctr_op_t)(struct cipher_ctx *ctx, struct cipher_pkt *pkt,
			uint8_t *ctr);

typedef int (*ccm_op_t)(struct cipher_ctx *ctx, struct cipher_aead_pkt *pkt,
			 uint8_t *nonce);

typedef int (*gcm_op_t)(struct cipher_ctx *ctx, struct cipher_aead_pkt *pkt,
			 uint8_t *nonce);

struct cipher_ops {

	enum cipher_mode cipher_mode;

	union {
		block_op_t	block_crypt_hndlr;
		cbc_op_t	cbc_crypt_hndlr;
		ctr_op_t	ctr_crypt_hndlr;
		ccm_op_t	ccm_crypt_hndlr;
		gcm_op_t	gcm_crypt_hndlr;
	};
};

struct ccm_params {
	uint16_t tag_len;
	uint16_t nonce_len;
};

struct ctr_params {
	/* CTR mode counter is a split counter composed of iv and counter
	 * such that ivlen + ctr_len = keylen
	 */
	uint32_t ctr_len;
};

struct gcm_params {
	uint16_t tag_len;
	uint16_t nonce_len;
};

/**
 * Structure encoding session parameters.
 *
 * Refer to comments for individual fields to know the contract
 * in terms of who fills what and when w.r.t begin_session() call.
 */
struct cipher_ctx {

	/** Place for driver to return function pointers to be invoked per
	 * cipher operation. To be populated by crypto driver on return from
	 * begin_session() based on the algo/mode chosen by the app.
	 */
	struct cipher_ops ops;

	/** To be populated by the app before calling begin_session() */
	union {
		/* Cryptographic key to be used in this session */
		uint8_t *bit_stream;
		/* For cases where  key is protected and is not
		 * available to caller
		 */
		void *handle;
	} key;

	/** The device driver instance this crypto context relates to. Will be
	 * populated by the begin_session() API.
	 */
	const struct device *device;

	/** If the driver supports multiple simultaneously crypto sessions, this
	 * will identify the specific driver state this crypto session relates
	 * to. Since dynamic memory allocation is not possible, it is
	 * suggested that at build time drivers allocate space for the
	 * max simultaneous sessions they intend to support. To be populated
	 * by the driver on return from begin_session().
	 */
	void *drv_sessn_state;

	/** Place for the user app to put info relevant stuff for resuming when
	 * completion callback happens for async ops. Totally managed by the
	 * app.
	 */
	void *app_sessn_state;

	/** Cypher mode parameters, which remain constant for all ops
	 * in a session. To be populated by the app before calling
	 * begin_session().
	 */
	union {
		struct ccm_params ccm_info;
		struct ctr_params ctr_info;
		struct gcm_params gcm_info;
	} mode_params;

	/** Cryptographic keylength in bytes. To be populated by the app
	 * before calling begin_session()
	 */
	uint16_t  keylen;

	/** How certain fields are to be interpreted for this session.
	 * (A bitmask of CAP_* below.)
	 * To be populated by the app before calling begin_session().
	 * An app can obtain the capability flags supported by a hw/driver
	 * by calling crypto_query_hwcaps().
	 */
	uint16_t flags;
};

/**
 * Structure encoding IO parameters of one cryptographic
 * operation like encrypt/decrypt.
 *
 * The fields which has not been explicitly called out has to
 * be filled up by the app before making the cipher_xxx_op()
 * call.
 */
struct cipher_pkt {

	/** Start address of input buffer */
	uint8_t *in_buf;

	/** Bytes to be operated upon */
	int  in_len;

	/** Start of the output buffer, to be allocated by
	 * the application. Can be NULL for in-place ops. To be populated
	 * with contents by the driver on return from op / async callback.
	 */
	uint8_t *out_buf;

	/** Size of the out_buf area allocated by the application. Drivers
	 * should not write past the size of output buffer.
	 */
	int out_buf_max;

	/** To be populated by driver on return from cipher_xxx_op() and
	 * holds the size of the actual result.
	 */
	int out_len;

	/** Context this packet relates to. This can be useful to get the
	 * session details, especially for async ops. Will be populated by the
	 * cipher_xxx_op() API based on the ctx parameter.
	 */
	struct cipher_ctx *ctx;
};

/**
 * Structure encoding IO parameters in AEAD (Authenticated Encryption
 * with Associated Data) scenario like in CCM.
 *
 * App has to furnish valid contents prior to making cipher_ccm_op() call.
 */
struct cipher_aead_pkt {
	/* IO buffers for encryption. This has to be supplied by the app. */
	struct cipher_pkt *pkt;

	/**
	 * Start address for Associated Data. This has to be supplied by app.
	 */
	uint8_t *ad;

	/** Size of  Associated Data. This has to be supplied by the app. */
	uint32_t ad_len;

	/** Start address for the auth hash. For an encryption op this will
	 * be populated by the driver when it returns from cipher_ccm_op call.
	 * For a decryption op this has to be supplied by the app.
	 */
	uint8_t *tag;
};

/* Prototype for the application function to be invoked by the crypto driver
 * on completion of an async request. The app may get the session context
 * via the pkt->ctx field. For CCM ops the encompassing AEAD packet may be
 * accessed via container_of(). The type of a packet can be determined via
 * pkt->ctx.ops.mode .
 */
typedef void (*cipher_completion_cb)(struct cipher_pkt *completed, int status);

/**
 * @}
 */
#endif /* ZEPHYR_INCLUDE_CRYPTO_CIPHER_H_ */
