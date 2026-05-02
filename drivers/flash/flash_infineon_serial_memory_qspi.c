/*
 * Copyright (c) 2025 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT     infineon_qspi_flash
#define SOC_NV_FLASH_NODE DT_PARENT(DT_INST(0, fixed_partitions))

#define PAGE_LEN DT_PROP(SOC_NV_FLASH_NODE, erase_block_size)

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/logging/log.h>

#include <infineon_kconfig.h>
#include "mtb_serial_memory.h"
#include "cy_device_headers.h"

LOG_MODULE_REGISTER(flash_infineon, CONFIG_FLASH_LOG_LEVEL);

#define TIMEOUT_1_MS (1000ul) /* 1 ms timeout for all blocking functions */
#define MEM_SLOT_NUM (0U)

extern cy_stc_smif_block_config_t smif0BlockConfig;
static mtb_serial_memory_t serial_memory_obj;
static cy_stc_smif_mem_context_t smif_mem_context;
static cy_stc_smif_mem_info_t smif_mem_info;

#ifdef CONFIG_PM
#ifdef CONFIG_SOC_SERIES_PSE84
static uint32_t smif0_crypto_input1;
static uint32_t smif0_crypto_input2;
static uint32_t smif0_crypto_input3;
static uint32_t smif1_crypto_input1;
static uint32_t smif1_crypto_input2;
static uint32_t smif1_crypto_input3;
#endif /* CONFIG_SOC_SERIES_PSE84 */
#endif /* CONFIG_PM */

#if defined(CY_DEVICE_PSE84)
#define SMIF0_CORE0 SMIF0_CORE
#endif /* defined(CY_DEVICE_PSE84) */

const mtb_hal_hf_clock_t flash_clock_ref = {
	.inst_num = 3U,
};

const mtb_hal_clock_t CYBSP_SMIF_CORE_0_XSPI_FLASH_hal_clock = {
	.clock_ref = &flash_clock_ref,
	.interface = &mtb_hal_clock_hf_interface,
};

/* Device config structure */
struct ifx_serial_memory_flash_config {
	uint32_t base_addr;
	uint32_t max_addr;
};

/* Data structure */
struct ifx_serial_memory_flash_data {
	SMIF_Type *base;
	const cy_stc_smif_config_t *config;
	struct k_sem sem;
};

static struct flash_parameters ifx_serial_memory_flash_parameters = {
	.write_block_size = DT_PROP(SOC_NV_FLASH_NODE, write_block_size),
	.erase_value = 0xFF,
};

static inline void ifx_serial_memory_sem_take(const struct device *dev)
{
	struct ifx_serial_memory_flash_data *data = dev->data;

	k_sem_take(&data->sem, K_FOREVER);
}

static inline void ifx_serial_memory_sem_give(const struct device *dev)
{
	struct ifx_serial_memory_flash_data *data = dev->data;

	k_sem_give(&data->sem);
}

static int ifx_serial_memory_flash_read(const struct device *dev, off_t offset, void *data,
					size_t data_len)
{
	cy_rslt_t rslt = CY_RSLT_SUCCESS;
	int ret = 0;

	if (!data_len) {
		return 0;
	}

	ifx_serial_memory_sem_take(dev);

	rslt = mtb_serial_memory_read(&serial_memory_obj, offset, data_len, data);
	if (rslt != CY_RSLT_SUCCESS) {
		LOG_ERR("Error reading @ %lu (Err:0x%x)", offset, rslt);
		ret = -EIO;
	}

	ifx_serial_memory_sem_give(dev);

	return ret;
}

static int ifx_serial_memory_flash_write(const struct device *dev, off_t offset, const void *data,
					 size_t data_len)
{
	cy_rslt_t rslt = CY_RSLT_SUCCESS;
	int ret = 0;

	if (data_len == 0) {
		return 0;
	}

	if (offset < 0) {
		return -EINVAL;
	}

	ifx_serial_memory_sem_take(dev);

	rslt = mtb_serial_memory_write(&serial_memory_obj, offset, data_len, data);
	if (rslt != CY_RSLT_SUCCESS) {
		LOG_ERR("Error in writing @ %lu (Err:0x%x)", offset, rslt);
		ret = -EIO;
	}

	ifx_serial_memory_sem_give(dev);

	return ret;
}

static int ifx_serial_memory_flash_erase(const struct device *dev, off_t offset, size_t size)
{
	cy_rslt_t rslt;
	int ret = 0;

	if (offset < 0) {
		return -EINVAL;
	}

	ifx_serial_memory_sem_take(dev);

	rslt = mtb_serial_memory_erase(&serial_memory_obj, offset, size);
	if (rslt != CY_RSLT_SUCCESS) {
		LOG_ERR("Error in erasing : 0x%x", rslt);
		ret = -EIO;
	}

	ifx_serial_memory_sem_give(dev);

	return ret;
}

#if CONFIG_FLASH_PAGE_LAYOUT
static const struct flash_pages_layout ifx_serial_memory_flash_pages_layout = {
	.pages_count = DT_REG_SIZE(SOC_NV_FLASH_NODE) / PAGE_LEN,
	.pages_size = PAGE_LEN,
};

static void ifx_serial_memory_flash_page_layout(const struct device *dev,
						const struct flash_pages_layout **layout,
						size_t *layout_size)
{
	*layout = &ifx_serial_memory_flash_pages_layout;

	/*
	 * For flash memories which have uniform page sizes, this routine
	 * returns an array of length 1, which specifies the page size and
	 * number of pages in the memory.
	 */
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static const struct flash_parameters *
ifx_serial_memory_flash_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	return &ifx_serial_memory_flash_parameters;
}

#ifdef CONFIG_PM
cy_en_syspm_status_t
ifx_serial_memory_flash_pm_callback(cy_stc_syspm_callback_params_t *callbackParams,
				    cy_en_syspm_callback_mode_t mode)
{
	if (mode == CY_SYSPM_CHECK_READY) {
#ifdef CONFIG_SOC_SERIES_PSE84
		smif0_crypto_input1 = SMIF_CRYPTO_INPUT1(SMIF0_CORE);
		smif0_crypto_input2 = SMIF_CRYPTO_INPUT2(SMIF0_CORE);
		smif0_crypto_input3 = SMIF_CRYPTO_INPUT3(SMIF0_CORE);
		smif1_crypto_input1 = SMIF_CRYPTO_INPUT1(SMIF1_CORE);
		smif1_crypto_input2 = SMIF_CRYPTO_INPUT2(SMIF1_CORE);
		smif1_crypto_input3 = SMIF_CRYPTO_INPUT3(SMIF1_CORE);
#endif /* CONFIG_SOC_SERIES_PSE84 */
	}

	if (mode == CY_SYSPM_AFTER_TRANSITION) {
#ifdef CONFIG_SOC_SERIES_PSE84
		SMIF_CRYPTO_INPUT1(SMIF0_CORE) = smif0_crypto_input1;
		SMIF_CRYPTO_INPUT2(SMIF0_CORE) = smif0_crypto_input2;
		SMIF_CRYPTO_INPUT3(SMIF0_CORE) = smif0_crypto_input3;
		SMIF_CRYPTO_INPUT1(SMIF1_CORE) = smif1_crypto_input1;
		SMIF_CRYPTO_INPUT2(SMIF1_CORE) = smif1_crypto_input2;
		SMIF_CRYPTO_INPUT3(SMIF1_CORE) = smif1_crypto_input3;

		smif0_crypto_input1 = 0;
		smif0_crypto_input2 = 0;
		smif0_crypto_input3 = 0;
		smif1_crypto_input1 = 0;
		smif1_crypto_input2 = 0;
		smif1_crypto_input3 = 0;
#endif /* CONFIG_SOC_SERIES_PSE84 */
	}

	return 0;
}

cy_stc_syspm_callback_params_t flash_deep_sleep_param = {NULL, NULL};

cy_stc_syspm_callback_t flash_deep_sleep = {&ifx_serial_memory_flash_pm_callback,
					    CY_SYSPM_DEEPSLEEP,
					    CY_SYSPM_SKIP_BEFORE_TRANSITION,
					    &flash_deep_sleep_param,
					    NULL,
					    NULL,
					    0};
#endif /* CONFIG_PM */

static int ifx_serial_memory_flash_init(const struct device *dev)
{
	struct ifx_serial_memory_flash_data *data = dev->data;

	cy_rslt_t result;

	/* Set-up serial memory. */
	result = mtb_serial_memory_setup(&serial_memory_obj, MTB_SERIAL_MEMORY_CHIP_SELECT_1,
					 SMIF0_CORE0, &CYBSP_SMIF_CORE_0_XSPI_FLASH_hal_clock,
					 &smif_mem_context, &smif_mem_info, &smif0BlockConfig);
	if (result != CY_RSLT_SUCCESS) {
		LOG_ERR("serial memory setup failed (QSPI) : 0x%x", result);
	}

	k_sem_init(&data->sem, 1, 1);

#ifdef CONFIG_PM
	Cy_SysPm_RegisterCallback(&flash_deep_sleep);
#endif /* CONFIG_PM */

	return result;
}

static DEVICE_API(flash, ifx_serial_memory_flash_driver_api) = {
	.read = ifx_serial_memory_flash_read,
	.write = ifx_serial_memory_flash_write,
	.erase = ifx_serial_memory_flash_erase,
	.get_parameters = ifx_serial_memory_flash_get_parameters,
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	.page_layout = ifx_serial_memory_flash_page_layout,
#endif /* CONFIG_FLASH_PAGE_LAYOUT */
};

static struct ifx_serial_memory_flash_data flash_data;

static const struct ifx_serial_memory_flash_config flash_config = {
	.base_addr = DT_REG_ADDR(SOC_NV_FLASH_NODE),
	.max_addr = DT_REG_ADDR(SOC_NV_FLASH_NODE) + DT_REG_SIZE(SOC_NV_FLASH_NODE)};

DEVICE_DT_INST_DEFINE(0, ifx_serial_memory_flash_init, NULL, &flash_data, &flash_config,
		      POST_KERNEL, CONFIG_FLASH_INIT_PRIORITY, &ifx_serial_memory_flash_driver_api);
