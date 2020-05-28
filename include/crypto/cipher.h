/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Crypto Cipher APIs
 *
 * This file contains the Crypto Abstraction layer APIs.
 *
 * [Experimental] Users should note that the APIs can change
 * as a part of ongoing development.
 */

/**
 * @brief Crypto APIs
 * @defgroup crypto Crypto
 * @{
 * @}
 */


/**
 * @brief Crypto Cipher APIs
 * @defgroup crypto_cipher Cipher
 * @ingroup crypto
 * @{
 */

#ifndef ZEPHYR_INCLUDE_CRYPTO_CIPHER_H_
#define ZEPHYR_INCLUDE_CRYPTO_CIPHER_H_

#include <device.h>
#include <errno.h>
#include <sys/util.h>
#include <sys/__assert.h>
#include "cipher_structs.h"

/* The API a crypto driver should implement */
__subsystem struct crypto_driver_api {
	int (*query_hw_caps)(struct device *dev);

	/* Setup a crypto session */
	int (*begin_session)(struct device *dev, struct cipher_ctx *ctx,
			enum cipher_algo algo, enum cipher_mode mode,
			enum cipher_op op_type);

	/* Tear down an established session */
	int (*free_session)(struct device *dev, struct cipher_ctx *ctx);

	/* Register async crypto op completion callback with the driver */
	int (*crypto_async_callback_set)(struct device *dev,
					 crypto_completion_cb cb);
};

/* Following are the public API a user app may call.
 * The first two relate to crypto "session" setup / teardown. Further we
 * have four cipher mode specific (CTR, CCM, CBC ...) calls to perform the
 * actual crypto operation in the context of a session. Also we have an
 * API to provide the callback for async operations.
 */

/**
 * @brief Query the crypto hardware capabilities
 *
 * This API is used by the app to query the capabilities supported by the
 * crypto device. Based on this the app can specify a subset of the supported
 * options to be honored for a session during cipher_begin_session().
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @return bitmask of supported options.
 */
static inline int cipher_query_hwcaps(struct device *dev)
{
	struct crypto_driver_api *api;
	int tmp;

	api = (struct crypto_driver_api *) dev->api;

	tmp = api->query_hw_caps(dev);

	__ASSERT((tmp & (CAP_OPAQUE_KEY_HNDL | CAP_RAW_KEY)) != 0,
		 "Driver should support at least one key type: RAW/Opaque");

	__ASSERT((tmp & (CAP_INPLACE_OPS | CAP_SEPARATE_IO_BUFS)) != 0,
	     "Driver should support at least one IO buf type: Inplace/separate");

	__ASSERT((tmp & (CAP_SYNC_OPS | CAP_ASYNC_OPS)) != 0,
		"Driver should support at least one op-type: sync/async");
	return tmp;

}

/**
 * @brief Setup a crypto session
 *
 * Initializes one time parameters, like the session key, algorithm and cipher
 * mode which may remain constant for all operations in the session. The state
 * may be cached in hardware and/or driver data state variables.
 *
 * @param  dev      Pointer to the device structure for the driver instance.
 * @param  ctx      Pointer to the context structure. Various one time
 *			parameters like key, keylength, etc. are supplied via
 *			this structure. The structure documentation specifies
 *			which fields are to be populated by the app before
 *			making this call.
 * @param  algo     The crypto algorithm to be used in this session. e.g AES
 * @param  mode     The cipher mode to be used in this session. e.g CBC, CTR
 * @param  optype   Whether we should encrypt or decrypt in this session
 *
 * @return 0 on success, negative errno code on fail.
 */
static inline int cipher_begin_session(struct device *dev,
				       struct cipher_ctx *ctx,
				       enum cipher_algo algo,
				       enum cipher_mode  mode,
				       enum cipher_op optype)
{
	struct crypto_driver_api *api;
	uint32_t flags;

	api = (struct crypto_driver_api *) dev->api;
	ctx->device = dev;
	ctx->ops.cipher_mode = mode;

	flags = (ctx->flags & (CAP_OPAQUE_KEY_HNDL | CAP_RAW_KEY));
	__ASSERT(flags != 0U, "Keytype missing: RAW Key or OPAQUE handle");
	__ASSERT(flags != (CAP_OPAQUE_KEY_HNDL | CAP_RAW_KEY),
			 "conflicting options for keytype");

	flags = (ctx->flags & (CAP_INPLACE_OPS | CAP_SEPARATE_IO_BUFS));
	__ASSERT(flags != 0U, "IO buffer type missing");
	__ASSERT(flags != (CAP_INPLACE_OPS | CAP_SEPARATE_IO_BUFS),
			"conflicting options for IO buffer type");

	flags = (ctx->flags & (CAP_SYNC_OPS | CAP_ASYNC_OPS));
	__ASSERT(flags != 0U, "sync/async type missing");
	__ASSERT(flags != (CAP_SYNC_OPS |  CAP_ASYNC_OPS),
			"conflicting options for sync/async");

	return api->begin_session(dev, ctx, algo, mode, optype);
}

/**
 * @brief Cleanup a crypto session
 *
 * Clears the hardware and/or driver state of a previous session.
 *
 * @param  dev      Pointer to the device structure for the driver instance.
 * @param  ctx      Pointer to the crypto context structure of the session
 *			to be freed.
 *
 * @return 0 on success, negative errno code on fail.
 */
static inline int cipher_free_session(struct device *dev,
				      struct cipher_ctx *ctx)
{
	struct crypto_driver_api *api;

	api = (struct crypto_driver_api *) dev->api;

	return api->free_session(dev, ctx);
}

/**
 * @brief Registers an async crypto op completion callback with the driver
 *
 * The application can register an async crypto op completion callback handler
 * to be invoked by the driver, on completion of a prior request submitted via
 * crypto_do_op(). Based on crypto device hardware semantics, this is likely to
 * be invoked from an ISR context.
 *
 * @param  dev   Pointer to the device structure for the driver instance.
 * @param  cb    Pointer to application callback to be called by the driver.
 *
 * @return 0 on success, -ENOTSUP if the driver does not support async op,
 *			  negative errno code on other error.
 */
static inline int cipher_callback_set(struct device *dev,
				      crypto_completion_cb cb)
{
	struct crypto_driver_api *api;

	api = (struct crypto_driver_api *) dev->api;

	if (api->crypto_async_callback_set) {
		return api->crypto_async_callback_set(dev, cb);
	}

	return -ENOTSUP;

}

/**
 * @brief Perform single-block crypto operation (ECB cipher mode). This
 * should not be overloaded to operate on multiple blocks for security reasons.
 *
 * @param  ctx       Pointer to the crypto context of this op.
 * @param  pkt   Structure holding the input/output buffer pointers.
 *
 * @return 0 on success, negative errno code on fail.
 */
static inline int cipher_block_op(struct cipher_ctx *ctx,
				     struct cipher_pkt *pkt)
{
	__ASSERT(ctx->ops.cipher_mode == CRYPTO_CIPHER_MODE_ECB, "ECB mode "
		 "session invoking a different mode handler");

	pkt->ctx = ctx;
	return ctx->ops.block_crypt_hndlr(ctx, pkt);
}

/**
 * @brief Perform Cipher Block Chaining (CBC) crypto operation.
 *
 * @param  ctx       Pointer to the crypto context of this op.
 * @param  pkt   Structure holding the input/output buffer pointers.
 * @param  iv        Initialization Vector (IV) for the operation. Same
 *			 IV value should not be reused across multiple
 *			 operations (within a session context) for security.
 *
 * @return 0 on success, negative errno code on fail.
 */
static inline int cipher_cbc_op(struct cipher_ctx *ctx,
				struct cipher_pkt *pkt, uint8_t *iv)
{
	__ASSERT(ctx->ops.cipher_mode == CRYPTO_CIPHER_MODE_CBC, "CBC mode "
		 "session invoking a different mode handler");

	pkt->ctx = ctx;
	return ctx->ops.cbc_crypt_hndlr(ctx, pkt, iv);
}

/**
 * @brief Perform Counter (CTR) mode crypto operation.
 *
 * @param  ctx       Pointer to the crypto context of this op.
 * @param  pkt   Structure holding the input/output buffer pointers.
 * @param  iv        Initialization Vector (IV) for the operation. We use a
 *			 split counter formed by appending IV and ctr.
 *			 Consequently  ivlen = keylen - ctrlen. 'ctrlen' is
 *			 specified during session setup through the
 *			 'ctx.mode_params.ctr_params.ctr_len' parameter. IV
 *			 should not be reused across multiple operations
 *			 (within a session context) for security. The non-IV
 *			 part of the split counter is transparent to the caller
 *			 and is fully managed by the crypto provider.
 *
 * @return 0 on success, negative errno code on fail.
 */
static inline int cipher_ctr_op(struct cipher_ctx *ctx,
				struct cipher_pkt *pkt, uint8_t *iv)
{
	__ASSERT(ctx->ops.cipher_mode == CRYPTO_CIPHER_MODE_CTR, "CTR mode "
		 "session invoking a different mode handler");

	pkt->ctx = ctx;
	return ctx->ops.ctr_crypt_hndlr(ctx, pkt, iv);
}

/**
 * @brief Perform Counter with CBC-MAC (CCM) mode crypto operation
 *
 * @param  ctx       Pointer to the crypto context of this op.
 * @param  pkt   Structure holding the input/output, Assosciated
 *			 Data (AD) and auth tag buffer pointers.
 * @param  nonce     Nonce for the operation. Same nonce value should not
 *			 be reused across multiple operations (within a
 *			 session context) for security.
 *
 * @return 0 on success, negative errno code on fail.
 */
static inline int cipher_ccm_op(struct cipher_ctx *ctx,
				struct cipher_aead_pkt *pkt, uint8_t *nonce)
{
	__ASSERT(ctx->ops.cipher_mode == CRYPTO_CIPHER_MODE_CCM, "CCM mode "
		 "session invoking a different mode handler");

	pkt->pkt->ctx = ctx;
	return ctx->ops.ccm_crypt_hndlr(ctx, pkt, nonce);
}

/**
 * @brief Perform Galois/Counter Mode (GCM) crypto operation
 *
 * @param  ctx       Pointer to the crypto context of this op.
 * @param  pkt   Structure holding the input/output, Associated
 *			 Data (AD) and auth tag buffer pointers.
 * @param  nonce     Nonce for the operation. Same nonce value should not
 *			 be reused across multiple operations (within a
 *			 session context) for security.
 *
 * @return 0 on success, negative errno code on fail.
 */
static inline int cipher_gcm_op(struct cipher_ctx *ctx,
				struct cipher_aead_pkt *pkt, uint8_t *nonce)
{
	__ASSERT(ctx->ops.cipher_mode == CRYPTO_CIPHER_MODE_GCM, "GCM mode "
		 "session invoking a different mode handler");

	pkt->pkt->ctx = ctx;
	return ctx->ops.gcm_crypt_hndlr(ctx, pkt, nonce);
}

/**
 * @}
 */

#endif /* ZEPHYR_INCLUDE_CRYPTO_CIPHER_H_ */
