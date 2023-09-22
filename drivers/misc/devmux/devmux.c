/*
 * Copyright (c) 2023, Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_devmux

#include <zephyr/device.h>
#include <zephyr/drivers/misc/devmux/devmux.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

struct devmux_config {
	const struct device **devs;
	const size_t n_devs;
};

struct devmux_data {
	struct k_spinlock lock;
	size_t selected;
};

/* The number of devmux devices */
#define N DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT)

static const struct device *devmux_devices[N];
static const struct devmux_config *devmux_configs[N];
static struct devmux_data *devmux_datas[N];

static bool devmux_device_is_valid(const struct device *dev)
{
	for (size_t i = 0; i < N; ++i) {
		if (dev == devmux_devices[i]) {
			return true;
		}
	}

	return false;
}

static size_t devmux_inst_get(const struct device *dev)
{
	for (size_t i = 0; i < N; i++) {
		if (dev == devmux_devices[i]) {
			return i;
		}
	}

	return SIZE_MAX;
}

const struct devmux_config *devmux_config_get(const struct device *dev)
{
	for (size_t i = 0; i < N; i++) {
		if (dev == devmux_devices[i]) {
			return devmux_configs[i];
		}
	}

	return NULL;
}

struct devmux_data *devmux_data_get(const struct device *dev)
{
	for (size_t i = 0; i < N; i++) {
		if (dev == devmux_devices[i]) {
			return devmux_datas[i];
		}
	}

	return NULL;
}

ssize_t z_impl_devmux_select_get(const struct device *dev)
{
	ssize_t index;
	struct devmux_data *const data = devmux_data_get(dev);

	if (!devmux_device_is_valid(dev)) {
		return -EINVAL;
	}

	K_SPINLOCK(&data->lock)
	{
		index = data->selected;
	}

	return index;
}

#ifdef CONFIG_USERSPACE
ssize_t z_vrfy_devmux_select_get(const struct device *dev)
{
	return z_impl_devmux_select_get(dev);
}
#include <syscalls/devmux_select_get_mrsh.c>
#endif

int z_impl_devmux_select_set(struct device *dev, size_t index)
{
	struct devmux_data *const data = devmux_data_get(dev);
	const struct devmux_config *config = devmux_config_get(dev);

	if (!devmux_device_is_valid(dev) || index >= config->n_devs) {
		return -EINVAL;
	}

	if (!device_is_ready(config->devs[index])) {
		return -ENODEV;
	}

	K_SPINLOCK(&data->lock)
	{
		*dev = *config->devs[index];
		data->selected = index;
	}

	return 0;
}

#ifdef CONFIG_USERSPACE
int z_vrfy_devmux_select_set(struct device *dev, size_t index)
{
	return z_impl_devmux_select_set(dev, index);
}
#include <syscalls/devmux_select_set_mrsh.c>
#endif

static int devmux_init(struct device *const dev)
{
	size_t inst = devmux_inst_get(dev);
	struct devmux_data *const data = dev->data;
	const struct devmux_config *config = dev->config;
	size_t sel = data->selected;

	devmux_configs[inst] = config;
	devmux_datas[inst] = data;

	if (!device_is_ready(config->devs[sel])) {
		return -ENODEV;
	}

	*dev = *config->devs[sel];

	return 0;
}

#define DEVMUX_PHANDLE_TO_DEVICE(node_id, prop, idx)                                               \
	DEVICE_DT_GET(DT_PHANDLE_BY_IDX(node_id, prop, idx))

#define DEVMUX_PHANDLE_DEVICES(_n)                                                                 \
	DT_INST_FOREACH_PROP_ELEM_SEP(_n, devices, DEVMUX_PHANDLE_TO_DEVICE, (,))

#define DEVMUX_SELECTED(_n) DT_INST_PROP(_n, selected)

#define DEVMUX_DEFINE(_n)                                                                          \
	BUILD_ASSERT(DT_INST_PROP_OR(_n, zephyr_mutable, 0),                                       \
		     "devmux nodes must contain the 'zephyr,mutable' property");                   \
	BUILD_ASSERT(DT_INST_PROP_LEN(_n, devices) > 0, "devices array must have non-zero size");  \
	BUILD_ASSERT(DEVMUX_SELECTED(_n) >= 0, "selected must be > 0");                            \
	BUILD_ASSERT(DEVMUX_SELECTED(_n) < DT_INST_PROP_LEN(_n, devices),                          \
		     "selected must be within bounds of devices phandle array");                   \
	static const struct device *demux_devs_##_n[] = {DEVMUX_PHANDLE_DEVICES(_n)};              \
	static const struct devmux_config devmux_config_##_n = {                                   \
		.devs = demux_devs_##_n,                                                           \
		.n_devs = DT_INST_PROP_LEN(_n, devices),                                           \
	};                                                                                         \
	static struct devmux_data devmux_data_##_n = {                                             \
		.selected = DEVMUX_SELECTED(_n),                                                   \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(_n, devmux_init, NULL, &devmux_data_##_n, &devmux_config_##_n,       \
			      PRE_KERNEL_1, CONFIG_DEVMUX_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(DEVMUX_DEFINE)

#define DEVMUX_DEVICE_GET(_n) DEVICE_DT_INST_GET(_n),
static const struct device *devmux_devices[] = {DT_INST_FOREACH_STATUS_OKAY(DEVMUX_DEVICE_GET)};
