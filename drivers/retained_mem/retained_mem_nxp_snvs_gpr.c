/*
 * SPDX-FileCopyrightText: 2026 Basalte bv
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_imx_snvs_gpr

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/retained_mem.h>
#include <zephyr/kernel.h>

#include <fsl_snvs_lp.h>

struct nxp_snvs_gpr_data {
	DEVICE_MMIO_RAM;
#ifdef CONFIG_RETAINED_MEM_MUTEXES
	struct k_mutex lock;
#endif
};

struct nxp_snvs_gpr_config {
	DEVICE_MMIO_ROM;
	size_t size;
};

static inline SNVS_Type *get_base(const struct device *dev)
{
	return (SNVS_Type *)DEVICE_MMIO_GET(dev);
}

static inline void nxp_snvs_gpr_lock_take(const struct device *dev)
{
#ifdef CONFIG_RETAINED_MEM_MUTEXES
	struct nxp_snvs_gpr_data *data = dev->data;

	k_mutex_lock(&data->lock, K_FOREVER);
#else
	ARG_UNUSED(dev);
#endif
}

static inline void nxp_snvs_gpr_lock_release(const struct device *dev)
{
#ifdef CONFIG_RETAINED_MEM_MUTEXES
	struct nxp_snvs_gpr_data *data = dev->data;

	k_mutex_unlock(&data->lock);
#else
	ARG_UNUSED(dev);
#endif
}

static int nxp_snvs_gpr_init(const struct device *dev)
{
#ifdef CONFIG_RETAINED_MEM_MUTEXES
	struct nxp_snvs_gpr_data *data = dev->data;

	k_mutex_init(&data->lock);
#endif

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	SNVS_LP_Init(get_base(dev));

	return 0;
}

static ssize_t nxp_snvs_gpr_size(const struct device *dev)
{
	const struct nxp_snvs_gpr_config *config = dev->config;

	return (ssize_t)config->size;
}

static int nxp_snvs_gpr_read(const struct device *dev, off_t offset, uint8_t *buffer, size_t size)
{
	nxp_snvs_gpr_lock_take(dev);

	memcpy(buffer, (uint8_t *)&get_base(dev)->LPGPR[0] + offset, size);

	nxp_snvs_gpr_lock_release(dev);

	return 0;
}

static int nxp_snvs_gpr_write(const struct device *dev, off_t offset, const uint8_t *buffer,
			      size_t size)
{
	nxp_snvs_gpr_lock_take(dev);

	memcpy((uint8_t *)&get_base(dev)->LPGPR[0] + offset, buffer, size);

	nxp_snvs_gpr_lock_release(dev);

	return 0;
}

static int nxp_snvs_gpr_clear(const struct device *dev)
{
	const struct nxp_snvs_gpr_config *config = dev->config;

	nxp_snvs_gpr_lock_take(dev);

	memset((uint8_t *)&get_base(dev)->LPGPR[0], 0, config->size);

	nxp_snvs_gpr_lock_release(dev);

	return 0;
}

static DEVICE_API(retained_mem, nxp_snvs_gpr_api) = {
	.size = nxp_snvs_gpr_size,
	.read = nxp_snvs_gpr_read,
	.write = nxp_snvs_gpr_write,
	.clear = nxp_snvs_gpr_clear,
};

#define NXP_SNVS_GPR_DEVICE(inst)                                                                  \
	static struct nxp_snvs_gpr_data nxp_snvs_gpr_data_##inst;                                  \
	static const struct nxp_snvs_gpr_config nxp_snvs_gpr_config_##inst = {                     \
		DEVICE_MMIO_ROM_INIT(DT_INST_PARENT(inst)),                                        \
		.size = sizeof(((SNVS_Type *)0)->LPGPR),                                           \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, nxp_snvs_gpr_init, NULL, &nxp_snvs_gpr_data_##inst,            \
			      &nxp_snvs_gpr_config_##inst, POST_KERNEL,                            \
			      CONFIG_RETAINED_MEM_INIT_PRIORITY, &nxp_snvs_gpr_api);

DT_INST_FOREACH_STATUS_OKAY(NXP_SNVS_GPR_DEVICE)
