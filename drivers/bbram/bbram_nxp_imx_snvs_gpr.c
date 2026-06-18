/*
 * SPDX-FileCopyrightText: 2026 Basalte bv
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_imx_snvs_gpr

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/bbram.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <fsl_snvs_lp.h>

#define LPGPR_REG_BYTES sizeof(uint32_t)

struct bbram_nxp_snvs_gpr_data {
	DEVICE_MMIO_RAM;
	/*
	 * Snapshot of LPSR captured before SNVS_LP_Init clears LVD/PGD, so the
	 * first call to check_invalid/check_standby_power can still report any
	 * event that happened while the SoC was off.
	 */
	uint32_t boot_lpsr;
};

struct bbram_nxp_snvs_gpr_config {
	DEVICE_MMIO_ROM;
	size_t size;
};

static inline SNVS_Type *get_base(const struct device *dev)
{
	return (SNVS_Type *)DEVICE_MMIO_GET(dev);
}

static int bbram_nxp_snvs_gpr_get_size(const struct device *dev, size_t *size)
{
	const struct bbram_nxp_snvs_gpr_config *config = dev->config;

	*size = config->size;
	return 0;
}

static int bbram_nxp_snvs_gpr_read(const struct device *dev, size_t offset, size_t size,
				   uint8_t *data)
{
	const struct bbram_nxp_snvs_gpr_config *config = dev->config;
	SNVS_Type *base = get_base(dev);
	size_t to_copy;

	if (size < 1 || offset + size > config->size) {
		return -EFAULT;
	}

	for (size_t read = 0; read < size; read += to_copy) {
		size_t word = (offset + read) / LPGPR_REG_BYTES;
		size_t begin = (offset + read) % LPGPR_REG_BYTES;
		uint32_t reg = base->LPGPR[word];

		to_copy = MIN(LPGPR_REG_BYTES - begin, size - read);
		bytecpy(data + read, (uint8_t *)&reg + begin, to_copy);
	}

	return 0;
}

static int bbram_nxp_snvs_gpr_write(const struct device *dev, size_t offset, size_t size,
				    const uint8_t *data)
{
	const struct bbram_nxp_snvs_gpr_config *config = dev->config;
	SNVS_Type *base = get_base(dev);
	size_t to_copy;

	if (size < 1 || offset + size > config->size) {
		return -EFAULT;
	}

	for (size_t written = 0; written < size; written += to_copy) {
		size_t word = (offset + written) / LPGPR_REG_BYTES;
		size_t begin = (offset + written) % LPGPR_REG_BYTES;
		uint32_t reg = base->LPGPR[word];

		to_copy = MIN(LPGPR_REG_BYTES - begin, size - written);
		bytecpy((uint8_t *)&reg + begin, data + written, to_copy);
		base->LPGPR[word] = reg;
	}

	return 0;
}

static int bbram_nxp_snvs_gpr_check_invalid(const struct device *dev)
{
	struct bbram_nxp_snvs_gpr_data *data = dev->data;
	SNVS_Type *base = get_base(dev);
	uint32_t flags = data->boot_lpsr | base->LPSR;

	if ((flags & SNVS_LPSR_ESVD_MASK) == 0U) {
		return 0;
	}

	base->LPSR = SNVS_LPSR_ESVD_MASK;
	data->boot_lpsr &= ~SNVS_LPSR_ESVD_MASK;
	return -EFAULT;
}

static int bbram_nxp_snvs_gpr_check_standby_power(const struct device *dev)
{
	struct bbram_nxp_snvs_gpr_data *data = dev->data;
	SNVS_Type *base = get_base(dev);
	uint32_t flags = data->boot_lpsr | base->LPSR;

	if ((flags & SNVS_LPSR_LVD_MASK) == 0U) {
		return 0;
	}

	base->LPSR = SNVS_LPSR_LVD_MASK;
	data->boot_lpsr &= ~SNVS_LPSR_LVD_MASK;
	return -EFAULT;
}

static int bbram_nxp_snvs_gpr_init(const struct device *dev)
{
	struct bbram_nxp_snvs_gpr_data *data = dev->data;
	SNVS_Type *base;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	base = get_base(dev);

	data->boot_lpsr = base->LPSR;

	/*
	 * Initialize the SNVS LP section. This ungates the SNVS_LP clock and
	 * writes the magic value to LPPGDR/LPLVDR that unlocks the LP register
	 * area; without it accesses to LPGPR are silently dropped. The LVD
	 * status flag is auto-cleared here, which is why we snapshotted LPSR
	 * above.
	 */
	SNVS_LP_Init(base);

	return 0;
}

static DEVICE_API(bbram, bbram_nxp_snvs_gpr_api) = {
	.get_size = bbram_nxp_snvs_gpr_get_size,
	.read = bbram_nxp_snvs_gpr_read,
	.write = bbram_nxp_snvs_gpr_write,
	.check_invalid = bbram_nxp_snvs_gpr_check_invalid,
	.check_standby_power = bbram_nxp_snvs_gpr_check_standby_power,
};

#define BBRAM_NXP_SNVS_GPR_INIT(inst)                                                              \
	static struct bbram_nxp_snvs_gpr_data bbram_nxp_snvs_gpr_data_##inst;                      \
	static const struct bbram_nxp_snvs_gpr_config bbram_nxp_snvs_gpr_config_##inst = {         \
		DEVICE_MMIO_ROM_INIT(DT_INST_PARENT(inst)),                                        \
		.size = sizeof(((SNVS_Type *)0)->LPGPR),                                           \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(inst, bbram_nxp_snvs_gpr_init, NULL,                                 \
			      &bbram_nxp_snvs_gpr_data_##inst,                                     \
			      &bbram_nxp_snvs_gpr_config_##inst, POST_KERNEL,                      \
			      CONFIG_BBRAM_INIT_PRIORITY, &bbram_nxp_snvs_gpr_api);

DT_INST_FOREACH_STATUS_OKAY(BBRAM_NXP_SNVS_GPR_INIT)
