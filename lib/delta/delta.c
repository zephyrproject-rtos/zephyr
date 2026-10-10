/*
 * Copyright (c) 2026 Clovis Corde
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/delta/delta.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(delta, CONFIG_DELTA_LOG_LEVEL);

int delta_apply_patch(const struct delta_io *io, const struct delta_backend *backend,
		      const struct delta_config *cfg)
{
	int ret;

	if (io == NULL || backend == NULL || cfg == NULL || io->read_source == NULL ||
	    io->read_patch == NULL || io->write_target == NULL || backend->apply == NULL) {
		return -EINVAL;
	}
	if (cfg->patch_size == 0 || cfg->max_target_size == 0) {
		return -EINVAL;
	}

	ret = backend->apply(io, cfg);
	if (ret != 0) {
		return ret;
	}

	if (io->finalize != NULL) {
		ret = io->finalize(io->ctx);
	}

	return ret;
}
