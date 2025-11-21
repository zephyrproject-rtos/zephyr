/*
 * Copyright (c) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/dai.h>
#include <zephyr/internal/syscall_handler.h>

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
					const void *bespoke_cfg)
{
	K_OOPS(K_SYSCALL_DRIVER_DAI(dev, config_set));
	K_OOPS(K_SYSCALL_MEMORY_READ(cfg, sizeof(*cfg)));

	/* TODO: using CONFIG_MMU_PAGE_SIZE as a placeholder, need to
	 * merge with Adrian's PR to add bespoke_cfg size */
	K_OOPS(K_SYSCALL_MEMORY_READ(bespoke_cfg, CONFIG_MMU_PAGE_SIZE));

	/* TODO: shall a kernel copy made of cfg */

	return z_impl_dai_config_set(dev, cfg, bespoke_cfg);
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
	K_OOPS(K_SYSCALL_DRIVER_DAI(dev, ts_config));
	K_OOPS(K_SYSCALL_MEMORY_READ(cfg, sizeof(*cfg)));

	return z_impl_dai_ts_config(dev, cfg);
}
#include <zephyr/syscalls/dai_ts_config_mrsh.c>

static inline int z_vrfy_dai_ts_start(const struct device *dev, struct dai_ts_cfg *cfg)
{
	K_OOPS(K_SYSCALL_DRIVER_DAI(dev, ts_start));
	K_OOPS(K_SYSCALL_MEMORY_READ(cfg, sizeof(*cfg)));

	return z_impl_dai_ts_start(dev, cfg);
}
#include <zephyr/syscalls/dai_ts_start_mrsh.c>

static inline int z_vrfy_dai_ts_stop(const struct device *dev, struct dai_ts_cfg *cfg)
{
	K_OOPS(K_SYSCALL_DRIVER_DAI(dev, ts_stop));
	K_OOPS(K_SYSCALL_MEMORY_READ(cfg, sizeof(*cfg)));

	return z_impl_dai_ts_stop(dev, cfg);
}
#include <zephyr/syscalls/dai_ts_stop_mrsh.c>

static inline int z_vrfy_dai_ts_get(const struct device *dev, struct dai_ts_cfg *cfg,
				    struct dai_ts_data *tsd)
{
	K_OOPS(K_SYSCALL_DRIVER_DAI(dev, ts_get));
	K_OOPS(K_SYSCALL_MEMORY_READ(cfg, sizeof(*cfg)));
	K_OOPS(K_SYSCALL_MEMORY_WRITE(tsd, sizeof(*tsd)));

	return z_impl_dai_ts_get(dev, cfg, tsd);
}
#include <zephyr/syscalls/dai_ts_get_mrsh.c>

static inline int z_vrfy_dai_config_update(const struct device *dev,
					   const void *bespoke_cfg,
					   size_t size)
{
	K_OOPS(K_SYSCALL_DRIVER_DAI(dev, config_update));
	K_OOPS(K_SYSCALL_MEMORY_READ(bespoke_cfg, size));

	return z_impl_dai_config_update(dev, bespoke_cfg, size);
}
#include <zephyr/syscalls/dai_config_update_mrsh.c>
