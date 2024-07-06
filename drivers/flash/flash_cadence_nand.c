/*
 * Copyright (c) 2023, Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT cdns_nand

#include "socfpga_system_manager.h"

#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/kernel.h>

/* Check if reset property is defined */
#define CDNS_NAND_RESET_SUPPORT DT_ANY_INST_HAS_PROP_STATUS_OKAY(resets)

#if CDNS_NAND_RESET_SUPPORT
#include <zephyr/drivers/reset.h>
#endif

#include "flash_cadence_nand_ll.h"

#define DEV_CFG(_dev)  ((const struct flash_cadence_nand_config *)(_dev)->config)
#define DEV_DATA(_dev) ((struct flash_cadence_nand_data *const)(_dev)->data)

#define FLASH_WRITE_SIZE DT_PROP(DT_INST(0, DT_DRV_COMPAT), block_size)

#ifdef CONFIG_BOARD_INTEL_SOCFPGA_AGILEX5_SOCDK
#define DFI_CFG_OFFSET 0xFC
/* To check the DFI register setting for NAND in the System Manager */
#define DFI_SEL_CHK    (SOCFPGA_SYSMGR_REG_BASE + DFI_CFG_OFFSET)
#endif

LOG_MODULE_REGISTER(flash_cdns_nand, CONFIG_FLASH_LOG_LEVEL);

struct flash_cadence_nand_data {
	DEVICE_MMIO_NAMED_RAM(nand_reg);
	DEVICE_MMIO_NAMED_RAM(sdma);
	/* device info structure */
	struct cadence_nand_params params;
	/* Mutex to prevent multiple processes from accessing the same driver api */
	struct k_mutex nand_mutex;
#if CONFIG_CDNS_NAND_INTERRUPT_SUPPORT
	/* Semaphore to send a signal from an interrupt handler to a thread  */
	struct k_sem interrupt_sem;
#endif
};

struct flash_cadence_nand_config {
	DEVICE_MMIO_NAMED_ROM(nand_reg);
	DEVICE_MMIO_NAMED_ROM(sdma);
#if CDNS_NAND_RESET_SUPPORT
	/* Reset controller device configuration for NAND*/
	const struct reset_dt_spec reset;
	/* Reset controller device configuration for Combo Phy*/
	const struct reset_dt_spec combo_phy_reset;
#endif
#if CONFIG_CDNS_NAND_INTERRUPT_SUPPORT
	void (*irq_config)(void);
#endif
};

static const struct flash_parameters flash_cdns_parameters = {.write_block_size = FLASH_WRITE_SIZE,
							      .erase_value = 0xFF};

#if CONFIG_FLASH_PAGE_LAYOUT

struct flash_pages_layout flash_cdns_pages_layout;

void flash_cdns_page_layout(const struct device *nand_dev, const struct flash_pages_layout **layout,
			    size_t *layout_size)
{
	struct flash_cadence_nand_data *const nand_data = DEV_DATA(nand_dev);
	struct cadence_nand_params *nand_param = &nand_data->params;

	flash_cdns_pages_layout.pages_count = nand_param->page_count;
	flash_cdns_pages_layout.pages_size = nand_param->page_size;
	*layout = &flash_cdns_pages_layout;
	*layout_size = 1;
}

#endif

static int flash_cdns_nand_erase(const struct device *nand_dev, k_off_t offset, size_t len)
{
	struct flash_cadence_nand_data *const nand_data = DEV_DATA(nand_dev);
	struct cadence_nand_params *nand_param = &nand_data->params;
	int ret;

	k_mutex_lock(&nand_data->nand_mutex, K_FOREVER);

	ret = cdns_nand_erase(nand_param, offset, len);

	k_mutex_unlock(&nand_data->nand_mutex);

	return ret;
}

static int flash_cdns_nand_write(const struct device *nand_dev, k_off_t offset, const void *data,
				 size_t len)
{
	struct flash_cadence_nand_data *const nand_data = DEV_DATA(nand_dev);
	struct cadence_nand_params *nand_param = &nand_data->params;
	int ret;

	if (data == NULL) {
		LOG_ERR("Invalid input parameter for NAND Flash Write!");
		return -EINVAL;
	}

	k_mutex_lock(&nand_data->nand_mutex, K_FOREVER);

	ret = cdns_nand_write(nand_param, data, offset, len);

	k_mutex_unlock(&nand_data->nand_mutex);

	return ret;
}

static int flash_cdns_nand_read(const struct device *nand_dev, k_off_t offset, void *data,
				size_t len)
{
	struct flash_cadence_nand_data *const nand_data = DEV_DATA(nand_dev);
	struct cadence_nand_params *nand_param = &nand_data->params;
	int ret;

	if (data == NULL) {
		LOG_ERR("Invalid input parameter for NAND Flash Read!");
		return -EINVAL;
	}

	k_mutex_lock(&nand_data->nand_mutex, K_FOREVER);

	ret = cdns_nand_read(nand_param, data, offset, len);

	k_mutex_unlock(&nand_data->nand_mutex);

	return ret;
}

static const struct flash_parameters *flash_cdns_get_parameters(const struct device *nand_dev)
{
	ARG_UNUSED(nand_dev);

	return &flash_cdns_parameters;
}
static const struct flash_driver_api flash_cdns_nand_api = {
	.erase = flash_cdns_nand_erase,
	.write = flash_cdns_nand_write,
	.read = flash_cdns_nand_read,
	.get_parameters = flash_cdns_get_parameters,
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	.page_layout = flash_cdns_page_layout,
#endif
};

#if CONFIG_CDNS_NAND_INTERRUPT_SUPPORT

static void cdns_nand_irq_handler(const struct device *nand_dev)
{
	struct flash_cadence_nand_data *const nand_data = DEV_DATA(nand_dev);
	struct cadence_nand_params *nand_param = &nand_data->params;

	cdns_nand_irq_handler_ll(nand_param);
	k_sem_give(&nand_param->interrupt_sem_t);
}

#endif

static int flash_cdns_nand_init(const struct device *nand_dev)
{
	DEVICE_MMIO_NAMED_MAP(nand_dev, nand_reg, K_MEM_CACHE_NONE);
	DEVICE_MMIO_NAMED_MAP(nand_dev, sdma, K_MEM_CACHE_NONE);
	const struct flash_cadence_nand_config *nand_config = DEV_CFG(nand_dev);
	struct flash_cadence_nand_data *const nand_data = DEV_DATA(nand_dev);
	struct cadence_nand_params *nand_param = &nand_data->params;
	int ret;

#ifdef CONFIG_BOARD_INTEL_SOCFPGA_AGILEX5_SOCDK
	uint32_t status;

	status = sys_read32(DFI_SEL_CHK);
	if ((status & 1) != 0) {
		LOG_ERR("DFI not configured for NAND Flash controller!!!");
		return -ENODEV;
	}
#endif

#if CDNS_NAND_RESET_SUPPORT
	/* Reset Combo phy and NAND only if reset controller driver is supported */
	if ((nand_config->combo_phy_reset.dev != NULL) && (nand_config->reset.dev != NULL)) {
		if (!device_is_ready(nand_config->reset.dev)) {
			LOG_ERR("Reset controller device not ready");
			return -ENODEV;
		}

		ret = reset_line_toggle(nand_config->combo_phy_reset.dev,
					nand_config->combo_phy_reset.id);
		if (ret != 0) {
			LOG_ERR("Combo phy reset failed");
			return ret;
		}

		ret = reset_line_toggle(nand_config->reset.dev, nand_config->reset.id);
		if (ret != 0) {
			LOG_ERR("NAND reset failed");
			return ret;
		}
	}
#endif
	nand_param->nand_base = DEVICE_MMIO_NAMED_GET(nand_dev, nand_reg);
	nand_param->sdma_base = DEVICE_MMIO_NAMED_GET(nand_dev, sdma);
	ret = k_mutex_init(&nand_data->nand_mutex);
	if (ret != 0) {
		LOG_ERR("Mutex creation Failed");
		return ret;
	}

#if CONFIG_CDNS_NAND_INTERRUPT_SUPPORT

	if (nand_config->irq_config == NULL) {
		LOG_ERR("Interrupt function not initialized!!");
		return -EINVAL;
	}
	nand_config->irq_config();
	ret = k_sem_init(&nand_param->interrupt_sem_t, 0, 1);
	if (ret != 0) {
		LOG_ERR("Semaphore creation Failed");
		return ret;
	}
#endif
	nand_param->page_count =
		(nand_param->npages_per_block * nand_param->nblocks_per_lun * nand_param->nluns);
	/* NAND Memory Controller init */
	ret = cdns_nand_init(nand_param);
	if (ret != 0) {
		LOG_ERR("NAND initialization Failed");
		return ret;
	}
	return 0;
}

#define CDNS_NAND_RESET_SPEC_INIT(inst)                                                            \
	.reset = RESET_DT_SPEC_INST_GET_BY_IDX(inst, 0),                                           \
	.combo_phy_reset = RESET_DT_SPEC_INST_GET_BY_IDX(inst, 1),

#define CREATE_FLASH_CADENCE_NAND_DEVICE(inst)                                                     \
	IF_ENABLED(CONFIG_CDNS_NAND_INTERRUPT_SUPPORT,                                             \
		   (static void cdns_nand_irq_config_##inst(void);))                               \
	struct flash_cadence_nand_data flash_cadence_nand_data_##inst = {                          \
		.params = {                                                                        \
			.datarate_mode = DT_INST_PROP(inst, data_rate_mode),                       \
		}};                                                                                \
	const struct flash_cadence_nand_config flash_cadence_nand_config_##inst = {                \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(nand_reg, DT_DRV_INST(inst)),                   \
		DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(sdma, DT_DRV_INST(inst)),                       \
		IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, resets), (CDNS_NAND_RESET_SPEC_INIT(inst))) \
			IF_ENABLED(CONFIG_CDNS_NAND_INTERRUPT_SUPPORT,                             \
				   (.irq_config = cdns_nand_irq_config_##inst,))};                 \
	DEVICE_DT_INST_DEFINE(inst, flash_cdns_nand_init, NULL, &flash_cadence_nand_data_##inst,   \
			      &flash_cadence_nand_config_##inst, POST_KERNEL,                      \
			      CONFIG_FLASH_INIT_PRIORITY, &flash_cdns_nand_api);                   \
	IF_ENABLED(CONFIG_CDNS_NAND_INTERRUPT_SUPPORT,                                             \
		   (static void cdns_nand_irq_config_##inst(void)                                  \
		   {										   \
			   IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority),            \
				       cdns_nand_irq_handler, DEVICE_DT_INST_GET(inst), 0);        \
			   irq_enable(DT_INST_IRQN(inst));                                         \
		   }))

DT_INST_FOREACH_STATUS_OKAY(CREATE_FLASH_CADENCE_NAND_DEVICE)
