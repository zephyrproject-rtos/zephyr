/*
 * Copyright (c) 2023 Laczen
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/bbram.h>
#include <zephyr/drivers/eeprom.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/boot_info/boot_info.h>

#define BACKEND_PHANDLE(inst) DT_PHANDLE(inst, backend)
#define BI_OFF(inst) COND_CODE_1(DT_NODE_HAS_PROP(inst, offset),		\
	(DT_PROP(inst, offset)), (0))
#define BI_SIZE(inst) COND_CODE_1(DT_NODE_HAS_PROP(inst, size),			\
	(DT_PROP(inst, size)), (~0))

#define BBRAM_BACKEND_FCNTS(inst)                                               \
	const struct device *const bi_dev_##inst =				\
		DEVICE_DT_GET(BACKEND_PHANDLE(inst));				\
	const struct device *bi_get_device_##inst(void)				\
	{									\
		return bi_dev_##inst;						\
	}									\
	size_t bi_get_size_##inst(void)                                         \
	{                                                                       \
		if (!device_is_ready(bi_dev_##inst)) {				\
			return 0U;						\
		}								\
										\
		size_t be_size;							\
		int rc = bbram_get_size(bi_dev_##inst, &be_size);		\
										\
		if ((rc != 0) || (be_size < BI_OFF(inst))) {			\
			return 0U;						\
		}								\
										\
		return MIN(BI_SIZE(inst), be_size - BI_OFF(inst));		\
	}                                                                       \
	int bi_get_##inst(void *data)                                           \
	{                                                                       \
		const size_t bi_size = bi_get_size_##inst();			\
		if (bi_size == 0U) {						\
			return -EINVAL;                                         \
		}                                                               \
                                                                                \
		return bbram_read(bi_dev_##inst, BI_OFF(inst), bi_size, data);	\
	}                                                                       \
	int bi_set_##inst(const void *data)                                     \
	{                                                                       \
		const size_t bi_size = bi_get_size_##inst();			\
		if (bi_size == 0U) {						\
			return -EINVAL;                                         \
		}                                                               \
                                                                                \
		return bbram_write(bi_dev_##inst, BI_OFF(inst), bi_size, data);	\
	}

DT_FOREACH_STATUS_OKAY(zephyr_boot_info_bbram, BBRAM_BACKEND_FCNTS)

#define FLASH_BACKEND_FCNTS(inst)                                               \
	const struct device *const bi_dev_##inst = DEVICE_DT_GET(		\
		DT_MTD_FROM_FIXED_PARTITION(BACKEND_PHANDLE(inst)));            \
	const struct device *bi_get_device_##inst(void)				\
	{									\
		return bi_dev_##inst;						\
	}									\
	size_t bi_get_size_##inst(void)                                         \
	{                                                                       \
		if (!device_is_ready(bi_dev_##inst)) {				\
			return 0U;						\
		}								\
										\
		const size_t be_size = DT_REG_SIZE(BACKEND_PHANDLE(inst));	\
		if (be_size < BI_OFF(inst)) {					\
			return 0U;						\
		}								\
										\
		return MIN(BI_SIZE(inst), be_size - BI_OFF(inst));		\
	}                                                                       \
	int bi_get_##inst(void *data)                                           \
	{                                                                       \
		const size_t bi_size = bi_get_size_##inst();			\
										\
		if (bi_size == 0U) {						\
			return -EINVAL;                                         \
		}                                                               \
										\
		const off_t off = DT_REG_ADDR(BACKEND_PHANDLE(inst)) +		\
				  BI_OFF(inst);					\
                                                                                \
		return flash_read(bi_dev_##inst, off, data, bi_size);		\
	}                                                                       \
	int bi_set_##inst(const void *data)                                     \
	{                                                                       \
		const size_t bi_size = bi_get_size_##inst();			\
										\
		if (bi_size == 0U) {						\
			return -EINVAL;                                         \
		}                                                               \
										\
		const off_t off = DT_REG_ADDR(BACKEND_PHANDLE(inst));		\
		uint8_t flash[DT_REG_SIZE(BACKEND_PHANDLE(inst))];		\
		int rc;								\
										\
		rc = flash_read(bi_dev_##inst, off, flash, sizeof(flash));	\
		if ((rc != 0) ||						\
		    (memcmp(data, &flash[BI_OFF(inst)], bi_size) == 0)) {	\
			goto end;						\
		}								\
										\
		rc = flash_erase(bi_dev_##inst, off, sizeof(flash));		\
		if (rc != 0) {							\
			goto end;						\
		}								\
										\
		memcpy(&flash[BI_OFF(inst)], data, bi_size);			\
		rc = flash_write(bi_dev_##inst, off, flash, sizeof(flash));	\
end:										\
		return rc;                                                      \
	}

DT_FOREACH_STATUS_OKAY(zephyr_boot_info_flash, FLASH_BACKEND_FCNTS)

#define EEPROM_BACKEND_FCNTS(inst)                                              \
	const struct device *const bi_dev_##inst =				\
		DEVICE_DT_GET(BACKEND_PHANDLE(inst));				\
	const struct device *bi_get_device_##inst(void)				\
	{									\
		return bi_dev_##inst;						\
	}									\
	size_t bi_get_size_##inst(void)                                         \
	{                                                                       \
		if (!device_is_ready(bi_dev_##inst)) {				\
			return 0U;						\
		}								\
										\
		const size_t be_size = DT_PROP(BACKEND_PHANDLE(inst), size);	\
		if (be_size < BI_OFF(inst)) {					\
			return 0U;						\
		}								\
										\
		return MIN(BI_SIZE(inst), be_size - BI_OFF(inst));		\
	}                                                                       \
	int bi_get_##inst(void *data)                                           \
	{                                                                       \
		const size_t bi_size = bi_get_size_##inst();			\
										\
		if (bi_size == 0U) {						\
			return -EINVAL;                                         \
		}                                                               \
                                                                                \
		return eeprom_read(bi_dev_##inst, BI_OFF(inst), data, bi_size);	\
	}                                                                       \
	int bi_set_##inst(const void *data)                                     \
	{                                                                       \
		const size_t bi_size = bi_get_size_##inst();			\
										\
		if (bi_size == 0U) {						\
			return -EINVAL;                                         \
		}                                                               \
                                                                                \
		return eeprom_write(bi_dev_##inst, BI_OFF(inst), data, bi_size);\
	}

DT_FOREACH_STATUS_OKAY(zephyr_boot_info_eeprom, EEPROM_BACKEND_FCNTS)
