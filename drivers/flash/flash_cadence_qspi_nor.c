/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT cdns_qspi_nor

#include "flash_cadence_qspi_nor_ll.h"

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(flash_cadence, CONFIG_FLASH_LOG_LEVEL);

struct flash_cad_priv {
	DEVICE_MMIO_NAMED_RAM(qspi_reg);
	DEVICE_MMIO_NAMED_RAM(qspi_data);
	struct cad_qspi_params params;
};

struct flash_cad_config {
	DEVICE_MMIO_NAMED_ROM(qspi_reg);
	DEVICE_MMIO_NAMED_ROM(qspi_data);
};

static const struct flash_parameters flash_cad_parameters = {
	.write_block_size = QSPI_BYTES_PER_DEV,
	.erase_value = 0xff,
};

#define DEV_DATA(dev)	((struct flash_cad_priv *)((dev)->data))
#define DEV_CFG(dev)	((struct flash_cad_config *)((dev)->config))

static int flash_cad_read(const struct device *dev, off_t offset,
				void *data, size_t len)
{
	struct flash_cad_priv *priv = dev->data;
	struct cad_qspi_params *cad_params = &priv->params;
	int rc;

	if ((data == NULL) || (len == 0)) {
		LOG_ERR("Invalid input parameter for QSPI Read!");
		return -EINVAL;
	}

	rc = cad_qspi_read(cad_params, data, (uint32_t)offset, len);

	if (rc < 0) {
		LOG_ERR("Cadence QSPI Flash Read Failed");
		return rc;
	}

	return 0;
}

static int flash_cad_erase(const struct device *dev, off_t offset,
				size_t len)
{
	struct flash_cad_priv *priv = dev->data;
	struct cad_qspi_params *cad_params = &priv->params;
	int rc;

	if (len == 0) {
		LOG_ERR("Invalid input parameter for QSPI Erase!");
		return -EINVAL;
	}

	rc = cad_qspi_erase(cad_params, (uint32_t)offset, len);

	if (rc < 0) {
		LOG_ERR("Cadence QSPI Flash Erase Failed!");
		return rc;
	}

	return 0;
}

static int flash_cad_write(const struct device *dev, off_t offset,
				const void *data, size_t len)
{
	struct flash_cad_priv *priv = dev->data;
	struct cad_qspi_params *cad_params = &priv->params;
	int rc;

	if ((data == NULL) || (len == 0)) {
		LOG_ERR("Invalid input parameter for QSPI Write!");
		return -EINVAL;
	}

	rc = cad_qspi_write(cad_params, (void *)data, (uint32_t)offset, len);

	if (rc < 0) {
		LOG_ERR("Cadence QSPI Flash Write Failed!");
		return rc;
	}

	return 0;
}

static const struct flash_parameters *
flash_cad_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	return &flash_cad_parameters;
}

static DEVICE_API(flash, flash_cad_api) = {
	.erase = flash_cad_erase,
	.write = flash_cad_write,
	.read = flash_cad_read,
	.get_parameters = flash_cad_get_parameters,
};

static int flash_cad_init(const struct device *dev)
{
	struct flash_cad_priv *priv = dev->data;
	struct cad_qspi_params *cad_params = &priv->params;
	int rc;

	DEVICE_MMIO_NAMED_MAP(dev, qspi_reg, K_MEM_CACHE_NONE);
	DEVICE_MMIO_NAMED_MAP(dev, qspi_data, K_MEM_CACHE_NONE);

	cad_params->reg_base = DEVICE_MMIO_NAMED_GET(dev, qspi_reg);
	cad_params->data_base = DEVICE_MMIO_NAMED_GET(dev, qspi_data);

	rc = cad_qspi_init(cad_params, QSPI_CONFIG_CPHA,
			QSPI_CONFIG_CPOL, QSPI_CONFIG_CSDA,
			QSPI_CONFIG_CSDADS, QSPI_CONFIG_CSEOT,
			QSPI_CONFIG_CSSOT, 0);

	if (rc < 0) {
		LOG_ERR("Cadence QSPI Flash Init Failed");
		return rc;
	}

	return 0;
}

#define CREATE_FLASH_CADENCE_QSPI_DEVICE(inst)				\
	static struct flash_cad_priv flash_cad_priv_##inst = {		\
		.params = {						\
			.clk_rate = DT_INST_PROP(inst, clock_frequency),\
			.data_size = DT_INST_REG_SIZE_BY_IDX(inst, 1),  \
		},							\
	};								\
									\
	static struct flash_cad_config flash_cad_config_##inst = {	\
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(			\
				qspi_reg, DT_DRV_INST(inst)),		\
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(			\
				qspi_data, DT_DRV_INST(inst)),		\
	};								\
									\
	DEVICE_DT_INST_DEFINE(inst,					\
			flash_cad_init,					\
			NULL,						\
			&flash_cad_priv_##inst,				\
			&flash_cad_config_##inst,			\
			POST_KERNEL,					\
			CONFIG_KERNEL_INIT_PRIORITY_DEVICE,		\
			&flash_cad_api);

DT_INST_FOREACH_STATUS_OKAY(CREATE_FLASH_CADENCE_QSPI_DEVICE)
