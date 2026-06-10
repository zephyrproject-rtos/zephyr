/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Siemens Mobility GmbH
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief AMP OpenAMP remoteproc_ops implementation
 * @ingroup amp_interface
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_AMP_OPENAMP_OPS_RPROC_H_
#define ZEPHYR_INCLUDE_DRIVERS_AMP_OPENAMP_OPS_RPROC_H_

#include <zephyr/drivers/amp.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <openamp/open_amp.h>
#include <openamp/remoteproc.h>

/**
 * @brief Full identification of an AMP core
 */
struct amp_full_identification {
	/** AMP root node */
	const struct device *dev;
	/** Core identification for which OpenAMP rproc is used */
	const struct amp_core_identification core_id;
};

/**
 * @cond INTERNAL_HIDDEN
 *
 * Internal implementaiton details that shouldn't be used by the user
 */
struct remoteproc *zephyr_openamp_init(struct remoteproc *rproc, const struct remoteproc_ops *ops,
				       void *arg);
int zephyr_openamp_config(struct remoteproc *rproc, void *data);
int zephyr_openamp_start(struct remoteproc *rproc);
int zephyr_openamp_stop(struct remoteproc *rproc);
struct remoteproc_mem *zephyr_openamp_get_mem(struct remoteproc *rproc, const char *name,
					      metal_phys_addr_t pa, metal_phys_addr_t da, void *va,
					      size_t size, struct remoteproc_mem *buf);
/** @endcond */

/**
 * @brief remoteproc_ops based on Zephyr drivers
 *
 * This provides remoteproc_ops that are based on top of Zephyr drivers. For
 * rproc_init it is neccessary to pass a pointer to amp_full_identification into
 * the priv argument which lives as long as the rproc argument itself.
 *
 * @see amp_full_identification
 *
 * TODO: Put into a group
 */
static const struct remoteproc_ops zephyr_openamp_ops = {
	.init = zephyr_openamp_init,
	.start = zephyr_openamp_start,
	.stop = zephyr_openamp_stop,
	.config = zephyr_openamp_config,
	.get_mem = zephyr_openamp_get_mem,
};

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_AMP_OPENAMP_OPS_RPROC_H_ */
