/*
 * Copyright (c) 2022 - 2023 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT cdns_qspi_nor
#define DEVICE_NODE   DT_INST(0, micron_mt25qu02g)

#include "flash_cadence_qspi_nor_ll.h"

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/clock_control.h>

LOG_MODULE_REGISTER(flash_cadence, CONFIG_FLASH_LOG_LEVEL);

struct flash_cad_priv {
	DEVICE_MMIO_NAMED_RAM(qspi_reg);
	DEVICE_MMIO_NAMED_RAM(qspi_data);
	struct cad_qspi_params params;
	/* Clock frequency of timer */
	uint32_t freq;
	/* Clock controller dev instance */
	const struct device *clk_dev;
	/* Identifier for timer to get clock freq from clk manager */
	clock_control_subsys_t clkid;
	/* Mutex to prevent multiple processes from accessing the same driver api */
	struct k_mutex qspi_mutex;
};

struct flash_cad_config {
	DEVICE_MMIO_NAMED_ROM(qspi_reg);
	DEVICE_MMIO_NAMED_ROM(qspi_data);
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	struct flash_pages_layout pages_layout;
#endif
};

static const struct flash_parameters flash_cad_parameters = {
	.write_block_size = DT_PROP(DEVICE_NODE, page_size),
	.erase_value = 0xff,
};

#define DEV_DATA(dev) ((struct flash_cad_priv *)((dev)->data))
#define DEV_CFG(dev)  ((struct flash_cad_config *)((dev)->config))

static int flash_cad_read(const struct device *dev, off_t offset, void *data, size_t len)
{
	struct flash_cad_priv *priv = dev->data;
	struct cad_qspi_params *cad_params = &priv->params;
	int rc;

	if ((data == NULL) || (len == 0)) {
		LOG_ERR("Invalid input parameter for QSPI Read!");
		return -EINVAL;
	}

	rc = k_mutex_lock(&priv->qspi_mutex, K_FOREVER);
	if (rc != 0) {
		LOG_ERR("Mutex lock Failed");
		return rc;
	}

	rc = cad_qspi_read(cad_params, data, (uint32_t)offset, len);
	if (rc < 0) {
		LOG_ERR("Cadence QSPI Flash Read Failed");
		k_mutex_unlock(&priv->qspi_mutex);
		return rc;
	}

	k_mutex_unlock(&priv->qspi_mutex);
	return 0;
}

static int flash_cad_erase(const struct device *dev, off_t offset, size_t len)
{
	struct flash_cad_priv *priv = dev->data;
	struct cad_qspi_params *cad_params = &priv->params;
	int rc;

	if (len == 0) {
		LOG_ERR("Invalid input parameter for QSPI Erase!");
		return -EINVAL;
	}

	rc = k_mutex_lock(&priv->qspi_mutex, K_FOREVER);
	if (rc != 0) {
		LOG_ERR("Mutex lock Failed");
		return rc;
	}

	rc = cad_qspi_erase(cad_params, (uint32_t)offset, len);

	if (rc < 0) {
		LOG_ERR("Cadence QSPI Flash Erase Failed!");
		k_mutex_unlock(&priv->qspi_mutex);
		return rc;
	}

	k_mutex_unlock(&priv->qspi_mutex);
	return 0;
}

static int flash_cad_write(const struct device *dev, off_t offset, const void *data, size_t len)
{
	struct flash_cad_priv *priv = dev->data;
	struct cad_qspi_params *cad_params = &priv->params;
	int rc;

	if ((data == NULL) || (len == 0)) {
		LOG_ERR("Invalid input parameter for QSPI Write!");
		return -EINVAL;
	}

	rc = k_mutex_lock(&priv->qspi_mutex, K_FOREVER);
	if (rc != 0) {
		LOG_ERR("Mutex lock Failed");
		return rc;
	}

	rc = cad_qspi_write(cad_params, (void *)data, (uint32_t)offset, len);

	if (rc < 0) {
		LOG_ERR("Cadence QSPI Flash Write Failed!");
		k_mutex_unlock(&priv->qspi_mutex);
		return rc;
	}

	k_mutex_unlock(&priv->qspi_mutex);
	return 0;
}

static const struct flash_parameters *flash_cad_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	return &flash_cad_parameters;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static void flash_cad_get_layout(const struct device *dev, const struct flash_pages_layout **layout,
				 size_t *layout_size)
{
	const struct flash_cad_config *cfg = DEV_CFG(dev);
	*layout = &cfg->pages_layout;
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static const struct flash_driver_api flash_cad_api = {
	.erase = flash_cad_erase,
	.write = flash_cad_write,
	.read = flash_cad_read,
	.get_parameters = flash_cad_get_parameters,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = flash_cad_get_layout,
#endif
};

static int flash_cad_init(const struct device *dev)
{
	struct flash_cad_priv *priv = dev->data;
	struct cad_qspi_params *cad_params = &priv->params;
	int rc;

	DEVICE_MMIO_NAMED_MAP(dev, qspi_reg, K_MEM_CACHE_NONE);
	DEVICE_MMIO_NAMED_MAP(dev, qspi_data, K_MEM_CACHE_NONE);

	/*
	 * Get clock rate from clock_frequency property if valid,
	 * otherwise, get clock rate from clock manager
	 */
	if (priv->freq == 0U) {
		if (!device_is_ready(priv->clk_dev)) {
			LOG_ERR("clock controller device not ready");
			return -ENODEV;
		}

		rc = clock_control_get_rate(priv->clk_dev, priv->clkid, &priv->freq);
		if (rc != 0) {
			LOG_ERR("Unable to get clock rate: err:%d", rc);
			return rc;
		}
	}

	rc = k_mutex_init(&priv->qspi_mutex);
	if (rc != 0) {
		LOG_ERR("Mutex creation Failed");
		return rc;
	}

	cad_params->reg_base = DEVICE_MMIO_NAMED_GET(dev, qspi_reg);
	cad_params->data_base = DEVICE_MMIO_NAMED_GET(dev, qspi_data);
	cad_params->clk_rate = priv->freq;

	rc = cad_qspi_init(cad_params, QSPI_CONFIG_CPHA, QSPI_CONFIG_CPOL, QSPI_CONFIG_CSDA,
			   QSPI_CONFIG_CSDADS, QSPI_CONFIG_CSEOT, QSPI_CONFIG_CSSOT, 0);

	if (rc < 0) {
		LOG_ERR("Cadence QSPI Flash Init Failed");
		return rc;
	}

	return 0;
}

#define CAD_QSPI_CLOCK_RATE_INIT(inst)                                                             \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, clock_frequency),                                  \
		    (.freq = DT_INST_PROP(inst, clock_frequency), .clk_dev = NULL,                 \
		     .clkid = (clock_control_subsys_t)0,),                                         \
		    (.freq = 0, .clk_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),               \
		     .clkid = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(inst, clkid),))

#define CREATE_FLASH_CADENCE_QSPI_DEVICE(inst)                                                     \
	enum {                                                                                     \
		INST_##inst##_BYTES = (DT_PROP(DEVICE_NODE, size) / 8),                            \
		INST_##inst##_PAGES = (INST_##inst##_BYTES / DT_PROP(DEVICE_NODE, page_size)),     \
	};                                                                                         \
	static struct flash_cad_priv flash_cad_priv_##inst = {                                     \
		.params =                                                                          \
			{                                                                          \
				.data_size = DT_INST_REG_SIZE_BY_IDX(inst, 1),                     \
				.qspi_device_subsector_size =                                      \
					(DT_PROP(DEVICE_NODE, subsector_size)),                    \
				.qspi_device_address_byte = (DT_PROP(DEVICE_NODE, address_byte)),  \
				.qspi_device_page_size = DT_PROP(DEVICE_NODE, page_size),          \
				.qspi_device_bytes_per_block =                                     \
					DT_PROP(DEVICE_NODE, bytes_per_block),                     \
			},                                                                         \
		CAD_QSPI_CLOCK_RATE_INIT(inst)};                                                   \
                                                                                                   \
	static struct flash_cad_config flash_cad_config_##inst = {                                 \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(qspi_reg, DT_DRV_INST(inst)),                   \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(qspi_data, DT_DRV_INST(inst)),                  \
		IF_ENABLED(CONFIG_FLASH_PAGE_LAYOUT,                                               \
			   (.pages_layout = {                                                      \
				    .pages_count = INST_##inst##_PAGES,                            \
				    .pages_size = DT_PROP(DEVICE_NODE, page_size),                 \
			    },))};                                                                \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, flash_cad_init, NULL, &flash_cad_priv_##inst,                  \
			      &flash_cad_config_##inst, POST_KERNEL,                               \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &flash_cad_api);

DT_INST_FOREACH_STATUS_OKAY(CREATE_FLASH_CADENCE_QSPI_DEVICE)
