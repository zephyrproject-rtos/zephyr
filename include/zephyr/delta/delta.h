/*
 * Copyright (c) 2026 Clovis Corde
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file delta.h
 * @brief Generic binary delta patching API.
 */
#ifndef ZEPHYR_INCLUDE_DELTA_DELTA_H_
#define ZEPHYR_INCLUDE_DELTA_DELTA_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Generic binary delta patching API.
 * @defgroup delta_api Delta patching
 * @ingroup os_services
 * @{
 */

/**
 * @brief I/O callbacks for the source, patch and target streams.
 *
 * Each returns 0 on success or a negative errno.
 */
struct delta_io {
	/** Read from the source. */
	int (*read_source)(void *ctx, size_t offset, void *buf, size_t len);
	/** Read from the patch. */
	int (*read_patch)(void *ctx, size_t offset, void *buf, size_t len);
	/** Write to the target. */
	int (*write_target)(void *ctx, size_t offset, const void *buf, size_t len);
	/** Called once after the whole target has been written. May be NULL. */
	int (*finalize)(void *ctx);
	/** Pointer passed unchanged to every callback. */
	void *ctx;
};

/** @brief Caller-supplied configuration. */
struct delta_config {
	/** Backend scratch buffer; required size is backend-specific. */
	uint8_t *work_buf;
	/** Length of @c work_buf, in bytes. */
	size_t work_sz;
	/** Size of the source stream, in bytes. */
	size_t source_size;
	/** Size of the patch, in bytes. */
	size_t patch_size;
	/** Largest target the caller accepts; bigger patches are rejected. */
	size_t max_target_size;
};

/** @brief A delta backend: one patch format. */
struct delta_backend {
	/** Apply the patch; returns 0 on success or a negative errno. */
	int (*apply)(const struct delta_io *io, const struct delta_config *cfg);
};

/**
 * @brief Apply a delta patch.
 *
 * On success, the optional @c finalize callback is invoked once after
 * the backend has written the whole target.
 *
 * @param io       I/O callbacks.
 * @param backend  Selected backend.
 * @param cfg      Caller configuration.
 *
 * @retval 0 on success.
 * @retval -EINVAL if an argument is NULL or a mandatory size is zero.
 * @retval negative errno from the backend or an I/O callback.
 */
int delta_apply_patch(const struct delta_io *io, const struct delta_backend *backend,
		      const struct delta_config *cfg);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DELTA_DELTA_H_ */
