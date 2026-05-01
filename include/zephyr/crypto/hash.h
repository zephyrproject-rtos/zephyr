/*
 * Copyright (c) 2022 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Crypto Hash APIs
 * @ingroup crypto_hash
 *
 * This file contains the Crypto Abstraction layer APIs.
 */
#ifndef ZEPHYR_INCLUDE_CRYPTO_HASH_H_
#define ZEPHYR_INCLUDE_CRYPTO_HASH_H_


/**
 * @addtogroup crypto_hash
 * @{
 */


/** Hash algorithms. */
enum hash_algo {
	CRYPTO_HASH_ALGO_SHA224 = 1, /**< SHA-224 algorithm. */
	CRYPTO_HASH_ALGO_SHA256 = 2, /**< SHA-256 algorithm. */
	CRYPTO_HASH_ALGO_SHA384 = 3, /**< SHA-384 algorithm. */
	CRYPTO_HASH_ALGO_SHA512 = 4, /**< SHA-512 algorithm. */
};

/* Forward declarations */
struct hash_ctx;
struct hash_pkt;


/**
 * @brief Perform a hash operation.
 *
 * @param ctx Hash session context.
 * @param pkt Packet containing input and output buffers.
 * @param finish Finalize the hash operation when true. Continue a multipart
 * hash operation when false.
 *
 * @retval 0 Operation completed successfully.
 * @retval -errno Negative errno code on failure.
 */
typedef int (*hash_op_t)(struct hash_ctx *ctx, struct hash_pkt *pkt,
			 bool finish);

/**
 * Structure encoding session parameters.
 *
 * Refer to comments for individual fields to know the contract
 * in terms of who fills what and when w.r.t begin_session() call.
 */
struct hash_ctx {
	/**
	 * Device driver instance this crypto context relates to.
	 *
	 * This is populated by hash_begin_session().
	 */
	const struct device *device;

	/**
	 * Driver-owned session state.
	 *
	 * If the driver supports multiple simultaneous crypto sessions, this
	 * identifies the specific driver state for this crypto session. Since
	 * dynamic memory allocation is not possible, drivers should allocate
	 * space at build time for the maximum number of simultaneous sessions
	 * they support. This is populated by the driver during
	 * hash_begin_session().
	 */
	void *drv_sessn_state;

	/**
	 * Hash operation handler selected for this session.
	 *
	 * This is populated by the driver during hash_begin_session().
	 */
	hash_op_t hash_hndlr;

	/**
	 * Multipart hash operation state.
	 *
	 * This is true after a multipart hash operation has started.
	 */
	bool started;

	/**
	 * Flags describing how certain fields are interpreted for this session.
	 *
	 * (A bitmask of CAP_* below.)
	 * To be populated by the app before calling hash_begin_session().
	 * An app can obtain the capability flags supported by a hw/driver
	 * by calling crypto_query_hwcaps().
	 */
	uint16_t flags;
};

/**
 * Structure encoding IO parameters of a hash
 * operation.
 *
 * The fields which has not been explicitly called out has to
 * be filled up by the app before calling hash_compute().
 */
struct hash_pkt {

	/**
	 * Start address of the input buffer. The buffer is allocated by the
	 * application and must remain valid for the duration of the operation.
	 */
	const uint8_t *in_buf;

	/** Number of input bytes to process. */
	size_t  in_len;

	/**
	 * Start address of the output buffer.
	 *
	 * The buffer is allocated by the application and must remain valid for
	 * the duration of the operation. This can be NULL for in-place
	 * operations, in which case the driver writes the result to @p in_buf.
	 */
	uint8_t *out_buf;

	/**
	 * Context this packet relates to.
	 *
	 * This can be useful to get the session details, especially for async
	 * ops. This is populated by hash_compute() or hash_update().
	 */
	struct hash_ctx *ctx;
};

/**
 * @brief Handle completion of an asynchronous hash request.
 *
 * The application can get the session context from the completed packet's
 * @c ctx field.
 *
 * @param completed Completed hash packet.
 * @param status Completion status. A value of 0 indicates success and a
 * negative errno code indicates failure.
 */
typedef void (*hash_completion_cb)(struct hash_pkt *completed, int status);


/**
 * @}
 */
#endif /* ZEPHYR_INCLUDE_CRYPTO_HASH_H_ */
