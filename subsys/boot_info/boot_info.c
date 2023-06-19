/*
 * Copyright (c) 2023 Laczen
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/bbram.h>
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
