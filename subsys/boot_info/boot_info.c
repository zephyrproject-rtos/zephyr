/*
 * Copyright (c) 2023 Laczen
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/bbram.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/drivers/eeprom.h>
#include <zephyr/boot_info/boot_info.h>

#define BACKEND_PHANDLE(inst) DT_PHANDLE(inst, backend)

#define BACKEND_K_SEM_DEFINE(inst)                                              \
	COND_CODE_1(CONFIG_MULTITHREADING,                                      \
		    (static K_SEM_DEFINE(k_sem_##inst, 1, 1)), ())

#define BACKEND_K_SEM_TAKE(inst)                                                \
	COND_CODE_1(CONFIG_MULTITHREADING,                                      \
		    (k_sem_take(&k_sem_##inst, K_FOREVER)), ())

#define BACKEND_K_SEM_GIVE(inst)                                                \
	COND_CODE_1(CONFIG_MULTITHREADING, (k_sem_give(&k_sem_##inst)), ())

#define RAM_BACKEND_FCNTS(inst)                                                 \
	BACKEND_K_SEM_DEFINE(inst);                                             \
	size_t bi_get_size_##inst(void)                                         \
	{                                                                       \
		return DT_REG_SIZE(BACKEND_PHANDLE(inst));                      \
	}                                                                       \
	int bi_get_##inst(void *data)                                           \
	{                                                                       \
		BACKEND_K_SEM_TAKE(inst);                                       \
		memcpy(data, (uint8_t *)DT_REG_ADDR(BACKEND_PHANDLE(inst)),     \
		       DT_REG_SIZE(BACKEND_PHANDLE(inst)));                     \
		BACKEND_K_SEM_GIVE(inst);                                       \
		return 0;                                                       \
	}	                                                                \
	int bi_set_##inst(const void *data)                                     \
	{                                                                       \
		BACKEND_K_SEM_TAKE(inst);                                       \
		memcpy((uint8_t *)DT_REG_ADDR(BACKEND_PHANDLE(inst)), data,     \
		       DT_REG_SIZE(BACKEND_PHANDLE(inst)));                     \
		BACKEND_K_SEM_GIVE(inst);                                       \
		return 0;                                                       \
	}

DT_FOREACH_STATUS_OKAY(zephyr_boot_info_ram, RAM_BACKEND_FCNTS)

#define BBRAM_BACKEND_FCNTS(inst)                                               \
	BACKEND_K_SEM_DEFINE(inst);                                             \
	const struct device *const bi_dev_##inst =				\
		DEVICE_DT_GET(BACKEND_PHANDLE(inst));				\
	size_t bi_get_size_##inst(void)                                         \
	{                                                                       \
		if (!device_is_ready(bi_dev_##inst)) {                          \
			return 0U;                                              \
		}                                                               \
                                                                                \
		size_t bbram_size = 0;                                          \
		int rc;                                                         \
		BACKEND_K_SEM_TAKE(inst);                                       \
		rc = bbram_get_size(bi_dev_##inst, &bbram_size);                \
		BACKEND_K_SEM_GIVE(inst);                                       \
		return (rc == 0) ? bbram_size : 0U;                             \
	}                                                                       \
	int bi_get_##inst(void *data)                                           \
	{                                                                       \
		if (!device_is_ready(bi_dev_##inst)) {                          \
			return -ENODEV;                                         \
		}                                                               \
                                                                                \
		size_t bbram_size = 0;                                          \
		int rc;                                                         \
		BACKEND_K_SEM_TAKE(inst);                                       \
		rc = bbram_get_size(bi_dev_##inst, &bbram_size);                \
		if (rc == 0) {                                                  \
			rc = bbram_read(bi_dev_##inst, 0, bbram_size, data);    \
		}                                                               \
		BACKEND_K_SEM_GIVE(inst);                                       \
		return rc;                                                      \
	}                                                                       \
	int bi_set_##inst(const void *data)                                     \
	{                                                                       \
		if (!device_is_ready(bi_dev_##inst)) {                          \
			return -ENODEV;                                         \
		}                                                               \
                                                                                \
		size_t bbram_size = 0;                                          \
		int rc;                                                         \
		BACKEND_K_SEM_TAKE(inst);                                       \
		rc = bbram_get_size(bi_dev_##inst, &bbram_size);                \
		if (rc == 0) {                                                  \
			rc = bbram_write(bi_dev_##inst, 0, bbram_size, data);   \
		}                                                               \
		BACKEND_K_SEM_GIVE(inst);                                       \
		return rc;                                                      \
	}

DT_FOREACH_STATUS_OKAY(zephyr_boot_info_bbram, BBRAM_BACKEND_FCNTS)

#define FLASHPART_OFF(inst) DT_REG_ADDR(BACKEND_PHANDLE(inst))
#define FLASHPART_SIZE(inst) DT_REG_SIZE(BACKEND_PHANDLE(inst))
#define FLASHBI_SIZE(inst) MIN(FLASHPART_SIZE(inst), DT_PROP(inst, size))

#define FLASH_BACKEND_FCNTS(inst)                                               \
	BACKEND_K_SEM_DEFINE(inst);                                             \
	const struct device *const bi_dev_##inst = DEVICE_DT_GET(		\
		DT_MTD_FROM_FIXED_PARTITION(BACKEND_PHANDLE(inst)));            \
	size_t bi_get_size_##inst(void)                                         \
	{                                                                       \
		return FLASHBI_SIZE(inst);					\
	}                                                                       \
	int bi_get_##inst(void *data)                                           \
	{                                                                       \
		if (!device_is_ready(bi_dev_##inst)) {				\
			return -ENODEV;                                         \
		}                                                               \
                                                                                \
		int rc;                                                         \
		BACKEND_K_SEM_TAKE(inst);                                       \
		rc = flash_read(bi_dev_##inst, FLASHPART_OFF(inst), data,	\
				FLASHBI_SIZE(inst));				\
		BACKEND_K_SEM_GIVE(inst);                                       \
		return rc;                                                      \
	}                                                                       \
	int bi_set_##inst(const void *data)                                     \
	{                                                                       \
		if (!device_is_ready(bi_dev_##inst)) {                          \
			return -ENODEV;                                         \
		}                                                               \
                                                                                \
		uint8_t flash[FLASHBI_SIZE(inst)];				\
		int rc;                                                         \
		BACKEND_K_SEM_TAKE(inst);                                       \
		rc = flash_read(bi_dev_##inst, FLASHPART_OFF(inst), flash,	\
				sizeof(flash));					\
		if ((rc != 0) || (memcmp(data, flash, sizeof(flash)) == 0)) {   \
			goto end;						\
		}								\
		rc = flash_erase(bi_dev_##inst, FLASHPART_OFF(inst),		\
				 FLASHPART_SIZE(inst));				\
		if (rc != 0) {							\
			goto end;						\
		}								\
		rc = flash_write(bi_dev_##inst, FLASHPART_OFF(inst), data,	\
				 FLASHBI_SIZE(inst));				\
end:										\
		BACKEND_K_SEM_GIVE(inst);                                       \
		return rc;                                                      \
	}

DT_FOREACH_STATUS_OKAY(zephyr_boot_info_flash, FLASH_BACKEND_FCNTS)

#define EEPROM_SIZE(inst) DT_PROP(BACKEND_PHANDLE(inst), size)
#define EEPROMBI_OFF(inst) COND_CODE_1(DT_NODE_HAS_PROP(inst, eeprom_offset),   \
	(DT_PROP(inst, eeprom_offset)), (0))
#define EEPROMBI_SIZE(inst) COND_CODE_1(DT_NODE_HAS_PROP(inst, size),		\
	(DT_PROP(inst, size)), (EEPROM_SIZE(inst) - EEPROMBI_OFF(inst)))
#define EEPROMBI_SIZE_CHECK(inst) BUILD_ASSERT(					\
	(EEPROM_SIZE(inst)) >= (EEPROMBI_OFF(inst) + EEPROMBI_SIZE(inst)),	\
	"bootinfo section exceeds eeprom size, modify size or eeprom-offset"	\
	"property.")

#define EEPROM_BACKEND_FCNTS(inst)                                              \
	EEPROMBI_SIZE_CHECK(inst);						\
	BACKEND_K_SEM_DEFINE(inst);                                             \
	const struct device *const bi_dev_##inst =				\
		DEVICE_DT_GET(BACKEND_PHANDLE(inst));				\
	size_t bi_get_size_##inst(void)                                         \
	{                                                                       \
		return EEPROMBI_SIZE(inst);					\
	}                                                                       \
	int bi_get_##inst(void *data)                                           \
	{                                                                       \
		if (!device_is_ready(bi_dev_##inst)) {				\
			return -ENODEV;                                         \
		}                                                               \
                                                                                \
		int rc;                                                         \
		BACKEND_K_SEM_TAKE(inst);                                       \
		rc = eeprom_read(bi_dev_##inst, EEPROMBI_OFF(inst), data,	\
				 EEPROMBI_SIZE(inst));				\
		BACKEND_K_SEM_GIVE(inst);                                       \
		return rc;                                                      \
	}                                                                       \
	int bi_set_##inst(const void *data)                                     \
	{                                                                       \
		if (!device_is_ready(bi_dev_##inst)) {                          \
			return -ENODEV;                                         \
		}                                                               \
                                                                                \
		int rc;                                                         \
		BACKEND_K_SEM_TAKE(inst);                                       \
		rc = eeprom_write(bi_dev_##inst, EEPROMBI_OFF(inst), data,	\
				  EEPROMBI_SIZE(inst));				\
		BACKEND_K_SEM_GIVE(inst);                                       \
		return rc;                                                      \
	}

DT_FOREACH_STATUS_OKAY(zephyr_boot_info_eeprom, EEPROM_BACKEND_FCNTS)
