/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Crypto Cipher structure definitions
 * @ingroup crypto_cipher
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

/** Cipher algorithms. */
enum cipher_algo {
	CRYPTO_CIPHER_ALGO_AES = 1, /**< Advanced Encryption Standard. */
};

/** Cipher operation types. */
enum cipher_op {
	CRYPTO_CIPHER_OP_DECRYPT = 0, /**< Decrypt input data. */
	CRYPTO_CIPHER_OP_ENCRYPT = 1, /**< Encrypt input data. */
};

/**
 * Possible cipher mode options.
 *
 * More to be added as required.
 */
enum cipher_mode {
	CRYPTO_CIPHER_MODE_ECB = 1, /**< Electronic Codebook mode. */
	CRYPTO_CIPHER_MODE_CBC = 2, /**< Cipher Block Chaining mode. */
	CRYPTO_CIPHER_MODE_CTR = 3, /**< Counter mode. */
	CRYPTO_CIPHER_MODE_CCM = 4, /**< Counter with CBC-MAC mode. */
	CRYPTO_CIPHER_MODE_GCM = 5, /**< Galois/Counter mode. */
};

/* Forward declarations */
struct cipher_aead_pkt;
struct cipher_ctx;
struct cipher_pkt;

/**
 * @brief Perform an ECB block cipher operation.
 *
 * @param ctx Cipher session context.
 * @param pkt Packet containing input and output buffers.
 *
 * @retval 0 Operation completed successfully.
 * @retval -errno Negative errno code on failure.
 */
typedef int (*block_op_t)(struct cipher_ctx *ctx, struct cipher_pkt *pkt);

/**
 * @brief Perform a CBC cipher operation.
 *
 * @param ctx Cipher session context.
 * @param pkt Packet containing input and output buffers.
 * @param iv Initialization vector for this operation. The buffer must
 * remain valid for the duration of the operation.
 *
 * @retval 0 Operation completed successfully.
 * @retval -errno Negative errno code on failure.
 */
typedef int (*cbc_op_t)(struct cipher_ctx *ctx, struct cipher_pkt *pkt,
			uint8_t *iv);

/**
 * @brief Perform a CTR cipher operation.
 *
 * @param ctx Cipher session context.
 * @param pkt Packet containing input and output buffers.
 * @param ctr Initial counter bytes for this operation. For split-counter
 * sessions, this is the IV portion supplied by the application.
 *
 * @retval 0 Operation completed successfully.
 * @retval -errno Negative errno code on failure.
 */
typedef int (*ctr_op_t)(struct cipher_ctx *ctx, struct cipher_pkt *pkt,
			uint8_t *ctr);

/**
 * @brief Perform a CCM authenticated cipher operation.
 *
 * @param ctx Cipher session context.
 * @param pkt Packet containing input, output, associated data, and
 * authentication tag buffers.
 * @param nonce Nonce for this operation. The buffer must remain valid for
 * the duration of the operation.
 *
 * @retval 0 Operation completed successfully.
 * @retval -errno Negative errno code on failure.
 */
typedef int (*ccm_op_t)(struct cipher_ctx *ctx, struct cipher_aead_pkt *pkt,
			 uint8_t *nonce);

/**
 * @brief Perform a GCM authenticated cipher operation.
 *
 * @param ctx Cipher session context.
 * @param pkt Packet containing input, output, associated data, and
 * authentication tag buffers.
 * @param nonce Nonce for this operation. The buffer must remain valid for
 * the duration of the operation.
 *
 * @retval 0 Operation completed successfully.
 * @retval -errno Negative errno code on failure.
 */
typedef int (*gcm_op_t)(struct cipher_ctx *ctx, struct cipher_aead_pkt *pkt,
			 uint8_t *nonce);

/**
 * Cipher operation handlers selected for a session.
 *
 * The crypto driver populates this structure during cipher_begin_session()
 * according to the selected algorithm, mode, and operation type.
 */
struct cipher_ops {

	/** Cipher mode associated with the active handler. */
	enum cipher_mode cipher_mode;

	/** Handler selected for the active cipher mode. */
	union {
		/** Handler for ECB block operations. */
		block_op_t	block_crypt_hndlr;
		/** Handler for CBC operations. */
		cbc_op_t	cbc_crypt_hndlr;
		/** Handler for CTR operations. */
		ctr_op_t	ctr_crypt_hndlr;
		/** Handler for CCM authenticated operations. */
		ccm_op_t	ccm_crypt_hndlr;
		/** Handler for GCM authenticated operations. */
		gcm_op_t	gcm_crypt_hndlr;
	};
};

/** CCM mode session parameters. */
struct ccm_params {
	/** Authentication tag length, in bytes. */
	uint16_t tag_len;
	/** Nonce length, in bytes. */
	uint16_t nonce_len;
};

/** CTR mode session parameters. */
struct ctr_params {
	/**
	 * Counter length, in bytes.
	 *
	 * CTR mode uses a split counter composed of an IV and counter such
	 * that IV length + counter length = key length.
	 */
	uint32_t ctr_len;
};

/** GCM mode session parameters. */
struct gcm_params {
	/** Authentication tag length, in bytes. */
	uint16_t tag_len;
	/** Nonce length, in bytes. */
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

	/**
	 * Key material for the session. The application populates the selected
	 * member before calling cipher_begin_session().
	 */
	union {
		/**
		 * Raw cryptographic key bytes. The buffer must remain valid while
		 * the session is active.
		 */
		const uint8_t *bit_stream;
		/**
		 * Driver-specific key handle for protected keys that are not
		 * directly available to the application.
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

	/** Cipher mode parameters, which remain constant for all ops
	 * in a session. To be populated by the app before calling
	 * begin_session().
	 */
	union {
		/** CCM parameters for CCM sessions. */
		struct ccm_params ccm_info;
		/** CTR parameters for CTR sessions. */
		struct ctr_params ctr_info;
		/** GCM parameters for GCM sessions. */
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

	/**
	 * Start address of the input buffer. The buffer is allocated by the
	 * application and must remain valid for the duration of the operation.
	 */
	uint8_t *in_buf;

	/** Number of input bytes to process. */
	int  in_len;

	/**
	 * Start address of the output buffer.
	 *
	 * The buffer is allocated by the application and must remain valid for
	 * the duration of the operation. This can be NULL for in-place
	 * operations, in which case the driver writes the result to @p in_buf.
	 */
	uint8_t *out_buf;

	/**
	 * Size of the output buffer, in bytes. Drivers must not write past this
	 * buffer size.
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
	/**
	 * Packet containing input and output buffers. This is supplied by the
	 * application and must remain valid for the duration of the operation.
	 */
	struct cipher_pkt *pkt;

	/**
	 * Start address of associated data. The buffer is supplied by the
	 * application and must remain valid for the duration of the operation.
	 */
	uint8_t *ad;

	/** Size of associated data, in bytes. */
	uint32_t ad_len;

	/**
	 * Start address of the authentication tag buffer.
	 *
	 * For encryption, the driver writes the tag to this application
	 * allocated buffer before the operation completes. For decryption, the
	 * application supplies the expected tag in this buffer.
	 */
	uint8_t *tag;
};

/**
 * @brief Handle completion of an asynchronous cipher request.
 *
 * The application can get the session context from the completed packet's
 * @c ctx field. For AEAD operations, the encompassing AEAD packet can be
 * accessed with container_of(). The packet type can be determined from the
 * session cipher mode.
 *
 * @param completed Completed cipher packet.
 * @param status Completion status. A value of 0 indicates success and a
 * negative errno code indicates failure.
 */
typedef void (*cipher_completion_cb)(struct cipher_pkt *completed, int status);

/**
 * @}
 */
#endif /* ZEPHYR_INCLUDE_CRYPTO_CIPHER_H_ */
