/*
 * Copyright (c) 2024 Rapid Silicon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Crypto Signature APIs
 *
 * This file contains the Crypto Abstraction layer APIs.
 */
#ifndef ZEPHYR_INCLUDE_CRYPTO_SIGN_H_
#define ZEPHYR_INCLUDE_CRYPTO_SIGN_H_


/**
 * @addtogroup crypto_sign
 * @{
 */


/**
 * Hash algorithm
 */
enum sign_algo {
	CRYPTO_SIGN_ALGO_RSA2048 = 0,
	CRYPTO_SIGN_ALGO_ECDSA256 = 1
};

/* Forward declarations */
struct sign_ctx;
struct sign_pkt;


typedef int (*sign_op_t)(struct sign_ctx *ctx, struct sign_pkt *pkt,
			 bool finish);

/* Function signatures for sign verification using standard signing algorithms
 * like  RSA2048, ECDSA256.
 */
typedef int (*rsa_op_t)(struct sign_ctx *ctx, struct sign_pkt *pkt, bool finish);

typedef int (*ecdsa_op_t)(struct sign_ctx *ctx, struct sign_pkt *pkt, bool finish);			

struct sign_ops {

	enum sign_algo signing_algo;

	union {
		sign_op_t	sign_op_hndlr;
		rsa_op_t	rsa_crypt_hndlr;
		ecdsa_op_t	ecdsa_crypt_hndlr;
	};
};

/**
 * Structure encoding session parameters.
 *
 * Refer to comments for individual fields to know the contract
 * in terms of who fills what and when w.r.t begin_session() call.
 */
struct sign_ctx {

	/** Place for driver to return function pointers to be invoked per
	 * sign operation. To be populated by crypto driver on return from
	 * begin_session() based on the algo/mode chosen by the app.
	 */
	struct sign_ops ops;

	/** To be populated by the app before calling begin_session() */
	union {
		/* Cryptographic key to be used in this session */
		const uint8_t *bit_stream;
		/* For cases where  key is protected and is not
		 * available to caller
		 */
		void *handle;
	} pub_key;

	/** To be populated by the app before calling begin_session() */
	union {
		/* Signature to be used in this session */
		const uint8_t *bit_stream;
		/* For cases where signature is protected and is not
		 * available to caller
		 */
		void *handle;
	} signature;

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

	/** Cryptographic keylength in bytes. To be populated by the app
	 * before calling begin_session()
	 */
	uint16_t  pub_key_len;

	/** Signature length in bytes. To be populated by the app
	 * before calling begin_session()
	 */
	uint16_t  sign_len;

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
 * be filled up by the app before making the sign_xxx_op()
 * call.
 */
struct sign_pkt {

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

	/** To be populated by driver on return from sign_xxx_op() and
	 * holds the size of the actual result.
	 */
	int out_len;

	/** Context this packet relates to. This can be useful to get the
	 * session details, especially for async ops. Will be populated by the
	 * sign_xxx_op() API based on the ctx parameter.
	 */
	struct sign_ctx *ctx;
};

/* Prototype for the application function to be invoked by the crypto driver
 * on completion of an async request. The app may get the session context
 * via the pkt->ctx field.
 */
typedef void (*sign_completion_cb)(struct sign_pkt *completed, int status);


/**
 * @}
 */
#endif /* ZEPHYR_INCLUDE_CRYPTO_SIGN_H_ */
