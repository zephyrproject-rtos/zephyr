/*
 * Copyright 2023-2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_FLASH_NXP_S32_QSPI_H_
#define ZEPHYR_DRIVERS_FLASH_NXP_S32_QSPI_H_

#include "jesd216.h"

#define QSPI_ERASE_VALUE 0xff

#define QSPI_IS_ALIGNED(addr, bits) (((addr) & BIT_MASK(bits)) == 0)

#if defined(CONFIG_FLASH_NXP_S32_QSPI_SFDP_RUNTIME)
/* Size of LUT */
#define QSPI_SFDP_LUT_SIZE     130U
/* Size of init operations */
#define QSPI_SFDP_INIT_OP_SIZE 8U
#if defined(CONFIG_FLASH_JESD216_API)
/* Size of all LUT sequences for JESD216 operations */
#define QSPI_JESD216_SEQ_SIZE 8U
#endif /* CONFIG_FLASH_JESD216_API */
#endif /* CONFIG_FLASH_NXP_S32_QSPI_SFDP_RUNTIME */

struct nxp_s32_qspi_config {
	const struct device *controller;
	struct flash_parameters flash_parameters;
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	struct flash_pages_layout layout;
#endif
#if !defined(CONFIG_FLASH_NXP_S32_QSPI_SFDP_RUNTIME)
	const Qspi_Ip_MemoryConfigType memory_cfg;
	enum jesd216_dw15_qer_type qer_type;
	bool quad_mode;
#endif
};

struct nxp_s32_qspi_data {
	uint8_t instance;
	Qspi_Ip_MemoryConnectionType memory_conn_cfg;
	uint8_t read_sfdp_lut_idx;
#if defined(CONFIG_FLASH_NXP_S32_QSPI_SFDP_RUNTIME)
	Qspi_Ip_MemoryConfigType memory_cfg;
	Qspi_Ip_InstrOpType lut_ops[QSPI_SFDP_LUT_SIZE];
	Qspi_Ip_InitOperationType init_ops[QSPI_SFDP_INIT_OP_SIZE];
#endif
#if defined(CONFIG_MULTITHREADING)
	struct k_sem sem;
#endif
};

static ALWAYS_INLINE Qspi_Ip_MemoryConfigType *get_memory_config(const struct device *dev)
{
#if defined(CONFIG_FLASH_NXP_S32_QSPI_SFDP_RUNTIME)
	return &((struct nxp_s32_qspi_data *)dev->data)->memory_cfg;
#else
	return ((Qspi_Ip_MemoryConfigType *)&((const struct nxp_s32_qspi_config *)dev->config)
			->memory_cfg);
#endif
}

static inline void nxp_s32_qspi_lock(const struct device *dev)
{
#ifdef CONFIG_MULTITHREADING
	struct nxp_s32_qspi_data *data = dev->data;

	k_sem_take(&data->sem, K_FOREVER);
#else
	ARG_UNUSED(dev);
#endif
}

static inline void nxp_s32_qspi_unlock(const struct device *dev)
{
#ifdef CONFIG_MULTITHREADING
	struct nxp_s32_qspi_data *data = dev->data;

	k_sem_give(&data->sem);
#else
	ARG_UNUSED(dev);
#endif
}

/*
 * This function retrieves the device instance used by the HAL
 * to access the internal driver state.
 */
uint8_t nxp_s32_qspi_register_device(void);

int nxp_s32_qspi_wait_until_ready(const struct device *dev);

int nxp_s32_qspi_read(const struct device *dev, off_t offset, void *dest, size_t size);

int nxp_s32_qspi_write(const struct device *dev, off_t offset, const void *src, size_t size);

int nxp_s32_qspi_erase(const struct device *dev, off_t offset, size_t size);

const struct flash_parameters *nxp_s32_qspi_get_parameters(const struct device *dev);

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
void nxp_s32_qspi_pages_layout(const struct device *dev, const struct flash_pages_layout **layout,
			       size_t *layout_size);
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

#if defined(CONFIG_FLASH_JESD216_API) || !defined(CONFIG_FLASH_NXP_S32_QSPI_SFDP_RUNTIME)
int nxp_s32_qspi_read_id(const struct device *dev, uint8_t *id);
#endif /* CONFIG_FLASH_JESD216_API || !CONFIG_FLASH_NXP_S32_QSPI_SFDP_RUNTIME */

#endif /* ZEPHYR_DRIVERS_FLASH_NXP_S32_QSPI_H_ */
