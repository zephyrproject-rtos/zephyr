/*
 * Copyright 2023-2024 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/logging/log.h>

#include <Qspi_Ip.h>

#include "flash_nxp_s32_qspi.h"

LOG_MODULE_REGISTER(flash_nxp_s32_qspi, CONFIG_FLASH_LOG_LEVEL);

static ALWAYS_INLINE bool area_is_subregion(const struct device *dev, off_t offset, size_t size)
{
	Qspi_Ip_MemoryConfigType *memory_cfg = get_memory_config(dev);

	return ((offset >= 0) && (offset < memory_cfg->memSize) &&
		((memory_cfg->memSize - offset) >= size));
}

uint8_t nxp_s32_qspi_register_device(void)
{
	static uint8_t instance_cnt;

	return instance_cnt++;
}

/* Must be called with lock */
int nxp_s32_qspi_wait_until_ready(const struct device *dev)
{
	struct nxp_s32_qspi_data *data = dev->data;
	Qspi_Ip_StatusType status;
	uint32_t timeout = 0xFFFFFF;
	int ret = 0;

	do {
		status = Qspi_Ip_GetMemoryStatus(data->instance);
		timeout--;
	} while ((status == STATUS_QSPI_IP_BUSY) && (timeout > 0));

	if (status != STATUS_QSPI_IP_SUCCESS) {
		LOG_ERR("Failed to read memory status (%d)", status);
		ret = -EIO;
	} else if (timeout == 0) {
		LOG_ERR("Timeout, memory is busy");
		ret = -ETIMEDOUT;
	}

	return ret;
}

int nxp_s32_qspi_read(const struct device *dev, off_t offset, void *dest, size_t size)
{
	struct nxp_s32_qspi_data *data = dev->data;
	Qspi_Ip_StatusType status;
	int ret = 0;

	if (!dest) {
		return -EINVAL;
	}

	if (!area_is_subregion(dev, offset, size)) {
		return -EINVAL;
	}

	if (size) {
		nxp_s32_qspi_lock(dev);

		status = Qspi_Ip_Read(data->instance, (uint32_t)offset, (uint8_t *)dest,
				      (uint32_t)size);
		if (status != STATUS_QSPI_IP_SUCCESS) {
			LOG_ERR("Failed to read %zu bytes at 0x%lx (%d)", size, offset, status);
			ret = -EIO;
		}

		nxp_s32_qspi_unlock(dev);
	}

	return ret;
}

int nxp_s32_qspi_write(const struct device *dev, off_t offset, const void *src, size_t size)
{
	const struct nxp_s32_qspi_config *config = dev->config;
	struct nxp_s32_qspi_data *data = dev->data;
	Qspi_Ip_MemoryConfigType *memory_cfg = get_memory_config(dev);
	Qspi_Ip_StatusType status;
	size_t max_write = (size_t)MIN(QSPI_IP_MAX_WRITE_SIZE, memory_cfg->pageSize);
	size_t len;
	int ret = 0;

	if (!src || !size) {
		return -EINVAL;
	}

	if (!area_is_subregion(dev, offset, size) ||
	    (offset % config->flash_parameters.write_block_size) ||
	    (size % config->flash_parameters.write_block_size)) {
		return -EINVAL;
	}

	nxp_s32_qspi_lock(dev);

	while (size) {
		len = MIN(max_write - (offset % max_write), size);
		status = Qspi_Ip_Program(data->instance, (uint32_t)offset, (const uint8_t *)src,
					 (uint32_t)len);
		if (status != STATUS_QSPI_IP_SUCCESS) {
			LOG_ERR("Failed to write %zu bytes at 0x%lx (%d)", len, offset, status);
			ret = -EIO;
			break;
		}

		ret = nxp_s32_qspi_wait_until_ready(dev);
		if (ret != 0) {
			break;
		}

		if (IS_ENABLED(CONFIG_FLASH_NXP_S32_QSPI_VERIFY_WRITE)) {
			status = Qspi_Ip_ProgramVerify(data->instance, (uint32_t)offset,
						       (const uint8_t *)src, (uint32_t)len);
			if (status != STATUS_QSPI_IP_SUCCESS) {
				LOG_ERR("Write verification failed at 0x%lx (%d)", offset, status);
				ret = -EIO;
				break;
			}
		}

		size -= len;
		src = (const uint8_t *)src + len;
		offset += len;
	}

	nxp_s32_qspi_unlock(dev);

	return ret;
}

static int nxp_s32_qspi_erase_block(const struct device *dev, off_t offset, size_t size,
				    size_t *erase_size)
{
	struct nxp_s32_qspi_data *data = dev->data;
	Qspi_Ip_MemoryConfigType *memory_cfg = get_memory_config(dev);
	Qspi_Ip_EraseVarConfigType *etp = NULL;
	Qspi_Ip_EraseVarConfigType *etp_tmp;
	Qspi_Ip_StatusType status;
	int ret = 0;

	/*
	 * Find the erase type with bigger size that can erase all or part of the
	 * requested memory size
	 */
	for (uint8_t i = 0; i < QSPI_IP_ERASE_TYPES; i++) {
		etp_tmp = (Qspi_Ip_EraseVarConfigType *)&(memory_cfg->eraseSettings.eraseTypes[i]);
		if ((etp_tmp->eraseLut != QSPI_IP_LUT_INVALID) &&
		    QSPI_IS_ALIGNED(offset, etp_tmp->size) && (BIT(etp_tmp->size) <= size) &&
		    ((etp == NULL) || (etp_tmp->size > etp->size))) {

			etp = etp_tmp;
		}
	}
	if (etp != NULL) {
		*erase_size = BIT(etp->size);
		status = Qspi_Ip_EraseBlock(data->instance, (uint32_t)offset, *erase_size);
		if (status != STATUS_QSPI_IP_SUCCESS) {
			LOG_ERR("Failed to erase %zu bytes at 0x%lx (%d)", *erase_size,
				(long)offset, status);
			ret = -EIO;
		}
	} else {
		LOG_ERR("Can't find erase size to erase %zu bytes", size);
		ret = -EINVAL;
	}

	return ret;
}

int nxp_s32_qspi_erase(const struct device *dev, off_t offset, size_t size)
{
	struct nxp_s32_qspi_data *data = dev->data;
	Qspi_Ip_MemoryConfigType *memory_cfg = get_memory_config(dev);
	Qspi_Ip_StatusType status;
	size_t erase_size;
	int ret = 0;

	if (!area_is_subregion(dev, offset, size) || !size) {
		return -EINVAL;
	}

	nxp_s32_qspi_lock(dev);

	if (size == memory_cfg->memSize) {
		status = Qspi_Ip_EraseChip(data->instance);
		if (status != STATUS_QSPI_IP_SUCCESS) {
			LOG_ERR("Failed to erase chip (%d)", status);
			ret = -EIO;
		}
	} else {
		while (size > 0) {
			erase_size = 0;

			ret = nxp_s32_qspi_erase_block(dev, offset, size, &erase_size);
			if (ret != 0) {
				break;
			}

			ret = nxp_s32_qspi_wait_until_ready(dev);
			if (ret != 0) {
				break;
			}

			if (IS_ENABLED(CONFIG_FLASH_NXP_S32_QSPI_VERIFY_ERASE)) {
				status = Qspi_Ip_EraseVerify(data->instance, (uint32_t)offset,
							     erase_size);
				if (status != STATUS_QSPI_IP_SUCCESS) {
					LOG_ERR("Erase verification failed at 0x%lx (%d)", offset,
						status);
					ret = -EIO;
					break;
				}
			}

			offset += erase_size;
			size -= erase_size;
		}
	}

	nxp_s32_qspi_unlock(dev);

	return ret;
}

const struct flash_parameters *nxp_s32_qspi_get_parameters(const struct device *dev)
{
	const struct nxp_s32_qspi_config *config = dev->config;

	return &config->flash_parameters;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
void nxp_s32_qspi_pages_layout(const struct device *dev, const struct flash_pages_layout **layout,
			       size_t *layout_size)
{
	const struct nxp_s32_qspi_config *config = dev->config;

	*layout = &config->layout;
	*layout_size = 1;
}
#endif /* CONFIG_FLASH_PAGE_LAYOUT */

#if defined(CONFIG_FLASH_JESD216_API) || !defined(CONFIG_FLASH_NXP_S32_QSPI_SFDP_RUNTIME)
int nxp_s32_qspi_read_id(const struct device *dev, uint8_t *id)
{
	struct nxp_s32_qspi_data *data = dev->data;
	Qspi_Ip_StatusType status;
	int ret = 0;

	nxp_s32_qspi_lock(dev);

	status = Qspi_Ip_ReadId(data->instance, id);
	if (status != STATUS_QSPI_IP_SUCCESS) {
		LOG_ERR("Failed to read device ID (%d)", status);
		ret = -EIO;
	}

	nxp_s32_qspi_unlock(dev);

	return ret;
}
#endif /* CONFIG_FLASH_JESD216_API || !CONFIG_FLASH_NXP_S32_QSPI_SFDP_RUNTIME */
