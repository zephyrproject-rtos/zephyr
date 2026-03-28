/*
 * Copyright (c) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/dai.h>
#include <zephyr/internal/syscall_handler.h>

/**
 * Maximum size of bespoke objects passed to DAI driver.
 * The objects get allocated temporarily on stack for validation,
 * so size needs to be limited.
 */
#define DAI_MAX_BESPOKE_CFG_SIZE 256

static inline int z_vrfy_dai_probe(const struct device *dev)
{
	K_OOPS(K_SYSCALL_DRIVER_DAI(dev, probe));

	return z_impl_dai_probe(dev);
}
#include <zephyr/syscalls/dai_probe_mrsh.c>

static inline int z_vrfy_dai_remove(const struct device *dev)
{
	K_OOPS(K_SYSCALL_DRIVER_DAI(dev, remove));

	return z_impl_dai_remove(dev);
}
#include <zephyr/syscalls/dai_remove_mrsh.c>

static inline int z_vrfy_dai_config_set(const struct device *dev,
					const struct dai_config *cfg,
					const void *bespoke_cfg,
					size_t size)
{
	uint8_t bespoke_cfg_kernel[DAI_MAX_BESPOKE_CFG_SIZE];
	struct dai_config cfg_kernel;

	if (size > DAI_MAX_BESPOKE_CFG_SIZE) {
		return -EINVAL;
	}

	K_OOPS(K_SYSCALL_DRIVER_DAI(dev, config_set));
	K_OOPS(k_usermode_from_copy(&cfg_kernel, cfg, sizeof(cfg_kernel)));

	if (bespoke_cfg) {
		K_OOPS(k_usermode_from_copy(bespoke_cfg_kernel, bespoke_cfg, size));
	}

	return z_impl_dai_config_set(dev, &cfg_kernel,
				     bespoke_cfg ? bespoke_cfg_kernel : NULL, size);
}
#include <zephyr/syscalls/dai_config_set_mrsh.c>

static inline int z_vrfy_dai_config_get(const struct device *dev,
					struct dai_config *cfg,
					enum dai_dir dir)
{
	K_OOPS(K_SYSCALL_DRIVER_DAI(dev, config_get));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(cfg, sizeof(*cfg)));

	return z_impl_dai_config_get(dev, cfg, dir);
}
#include <zephyr/syscalls/dai_config_get_mrsh.c>

static inline int z_vrfy_dai_get_properties_copy(const struct device *dev,
						enum dai_dir dir,
						int stream_id,
						struct dai_properties *dst)
{
	K_OOPS(K_SYSCALL_DRIVER_DAI(dev, get_properties_copy));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(dst, sizeof(*dst)));

	return z_impl_dai_get_properties_copy(dev, dir, stream_id, dst);
}
#include <zephyr/syscalls/dai_get_properties_copy_mrsh.c>

static inline int z_vrfy_dai_trigger(const struct device *dev,
				     enum dai_dir dir,
				     enum dai_trigger_cmd cmd)
{
	K_OOPS(K_SYSCALL_DRIVER_DAI(dev, trigger));

	return z_impl_dai_trigger(dev, dir, cmd);
}
#include <zephyr/syscalls/dai_trigger_mrsh.c>

static inline int z_vrfy_dai_ts_config(const struct device *dev, struct dai_ts_cfg *cfg)
{
	struct dai_ts_cfg cfg_kernel;

	K_OOPS(K_SYSCALL_DRIVER_DAI(dev, ts_config));
	K_OOPS(k_usermode_from_copy(&cfg_kernel, cfg, sizeof(cfg_kernel)));

	return z_impl_dai_ts_config(dev, &cfg_kernel);
}
#include <zephyr/syscalls/dai_ts_config_mrsh.c>

static inline int z_vrfy_dai_ts_start(const struct device *dev, struct dai_ts_cfg *cfg)
{
	struct dai_ts_cfg cfg_kernel;

	K_OOPS(K_SYSCALL_DRIVER_DAI(dev, ts_start));
	K_OOPS(k_usermode_from_copy(&cfg_kernel, cfg, sizeof(cfg_kernel)));

	return z_impl_dai_ts_start(dev, &cfg_kernel);
}
#include <zephyr/syscalls/dai_ts_start_mrsh.c>

static inline int z_vrfy_dai_ts_stop(const struct device *dev, struct dai_ts_cfg *cfg)
{
	struct dai_ts_cfg cfg_kernel;

	K_OOPS(K_SYSCALL_DRIVER_DAI(dev, ts_stop));
	K_OOPS(k_usermode_from_copy(&cfg_kernel, cfg, sizeof(cfg_kernel)));

	return z_impl_dai_ts_stop(dev, &cfg_kernel);
}
#include <zephyr/syscalls/dai_ts_stop_mrsh.c>

static inline int z_vrfy_dai_ts_get(const struct device *dev, struct dai_ts_cfg *cfg,
				    struct dai_ts_data *tsd)
{
	struct dai_ts_cfg cfg_kernel;

	K_OOPS(K_SYSCALL_DRIVER_DAI(dev, ts_get));
	K_OOPS(k_usermode_from_copy(&cfg_kernel, cfg, sizeof(cfg_kernel)));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(tsd, sizeof(*tsd)));

	return z_impl_dai_ts_get(dev, &cfg_kernel, tsd);
}
#include <zephyr/syscalls/dai_ts_get_mrsh.c>

static inline int z_vrfy_dai_config_update(const struct device *dev,
					   const void *bespoke_cfg,
					   size_t size)
{
	uint8_t bespoke_cfg_kernel[DAI_MAX_BESPOKE_CFG_SIZE];

	if (!bespoke_cfg || size > DAI_MAX_BESPOKE_CFG_SIZE) {
		return -EINVAL;
	}

	K_OOPS(K_SYSCALL_DRIVER_DAI(dev, config_update));
	K_OOPS(k_usermode_from_copy(bespoke_cfg_kernel, bespoke_cfg, size));

	return z_impl_dai_config_update(dev, bespoke_cfg_kernel, size);
}
#include <zephyr/syscalls/dai_config_update_mrsh.c>
