/*
 * Copyright (c) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <kernel.h>
#include <device.h>

#define DT_DRV_COMPAT cdns_qspi_nor

#include <string.h>
#include <drivers/flash.h>
#include <init.h>
#include <soc.h>

#include "flash_cadence_qspi_nor_ll.h"

struct flash_cad_priv {
	cad_qspi_params_t params;
};

static const struct flash_parameters flash_cad_parameters = {
	.write_block_size = QSPI_BYTES_PER_DEV,
	.erase_value = 0xff,
};

static int flash_cad_read(const struct device *dev, off_t offset,
				void *data, size_t len)
{
	int rc;

	if (dev == NULL) {
		printk("Error: No device detected");
		return -ENODEV;
	}

	struct flash_cad_priv *priv = dev->data;
	cad_qspi_params_t *cad_params = &priv->params;

	if ((data == NULL) || (len == 0)) {
		printk("Error: Invalid input parameter for QSPI Write!");
		return -EINVAL;
	}

	rc = cad_qspi_read(cad_params, data, offset, len);

	if (rc < 0) {
		printk("Error: Cadence QSPI Flash Read Failed");
		return rc;
	}

	return 0;
}

static int flash_cad_erase(const struct device *dev, off_t offset,
				size_t len)
{
	int rc;

	if (dev == NULL) {
		printk("Error: No device detected");
		return -ENODEV;
	}

	struct flash_cad_priv *priv = dev->data;
	cad_qspi_params_t *cad_params = &priv->params;

	if (len == 0) {
		printk("Error: Invalid input parameter for QSPI Write!");
		return -EINVAL;
	}

	rc = cad_qspi_erase(cad_params, offset, len);

	if (rc < 0) {
		printk("Error: Cadence QSPI Flash Erase Failed!");
		return rc;
	}

	return 0;
}

static int flash_cad_write(const struct device *dev, off_t offset,
				const void *data, size_t len)
{
	int rc;

	if (dev == NULL) {
		printk("Error: No device detected!");
		return -ENODEV;
	}

	struct flash_cad_priv *priv = dev->data;
	cad_qspi_params_t *cad_params = &priv->params;

	if ((data == NULL) || (len == 0)) {
		printk("Error: Invalid input parameter for QSPI Write!");
		return -EINVAL;
	}

	rc = cad_qspi_write(cad_params, (void *)data, offset, len);

	if (rc < 0) {
		printk("Error: Cadence QSPI Flash Write Failed!");
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

static const struct flash_driver_api flash_cad_api = {
	.erase = flash_cad_erase,
	.write = flash_cad_write,
	.read = flash_cad_read,
	.get_parameters = flash_cad_get_parameters,
};

static int flash_cad_init(const struct device *dev)
{

	int rc;

	if (dev == NULL) {
		printk("Error: No device detected");
		return -ENODEV;
	}

	struct flash_cad_priv *priv = dev->data;
	cad_qspi_params_t *cad_params = &priv->params;

	rc = cad_qspi_init(cad_params, QSPI_CONFIG_CPHA,
			QSPI_CONFIG_CPOL, QSPI_CONFIG_CSDA,
			QSPI_CONFIG_CSDADS, QSPI_CONFIG_CSEOT,
			QSPI_CONFIG_CSSOT, 0);

	if (rc < 0) {
		printk("Error: Cadence QSPI Flash Init Failed");
		return rc;
	}

	return 0;
}

#define CREATE_FLASH_CADENCE_QSPI_DEVICE(inst)					\
	static struct flash_cad_priv flash_cad_priv_##inst = {			\
		.params = {							\
			.reg_base = DT_INST_REG_ADDR(inst),			\
			.data_base = DT_INST_REG_ADDR_BY_IDX(inst, 1),		\
			.clk_rate = DT_INST_PROP(inst, clock_frequency),	\
		},								\
	};									\
	DEVICE_DT_INST_DEFINE(inst,						\
			flash_cad_init,						\
			NULL,							\
			&flash_cad_priv_##inst,					\
			NULL,							\
			POST_KERNEL,						\
			CONFIG_KERNEL_INIT_PRIORITY_DEVICE,			\
			&flash_cad_api);

DT_INST_FOREACH_STATUS_OKAY(CREATE_FLASH_CADENCE_QSPI_DEVICE)
