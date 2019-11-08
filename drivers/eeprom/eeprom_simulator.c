/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <drivers/eeprom.h>
#include <init.h>
#include <kernel.h>
#include <sys/util.h>
#include <stats/stats.h>
#include <string.h>

#define LOG_LEVEL CONFIG_EEPROM_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(eeprom_simulator);

struct eeprom_sim_config {
	size_t size;
	bool readonly;
};

#define DEV_NAME(dev) ((dev)->config->name)
#define DEV_CONFIG(dev) ((dev)->config->config_info)

#define EEPROM(addr) (mock_eeprom + (addr))

#if defined(CONFIG_MULTITHREADING)
/* semaphore for locking flash resources (tickers) */
static struct k_sem sem_lock;
#define SYNC_INIT() k_sem_init(&sem_lock, 1, 1)
#define SYNC_LOCK() k_sem_take(&sem_lock, K_FOREVER)
#define SYNC_UNLOCK() k_sem_give(&sem_lock)
#else
#define SYNC_INIT()
#define SYNC_LOCK()
#define SYNC_UNLOCK()
#endif

/* simulator statistcs */
STATS_SECT_START(eeprom_sim_stats)
STATS_SECT_ENTRY32(bytes_read)		/* total bytes read */
STATS_SECT_ENTRY32(bytes_written)	/* total bytes written */
STATS_SECT_ENTRY32(eeprom_read_calls)	/* calls to eeprom_read() */
STATS_SECT_ENTRY32(eeprom_read_time_us) /* time spent in eeprom_read() */
STATS_SECT_ENTRY32(eeprom_write_calls)  /* calls to eeprom_write() */
STATS_SECT_ENTRY32(eeprom_write_time_us)/* time spent in eeprom_write() */
STATS_SECT_END;

STATS_SECT_DECL(eeprom_sim_stats) eeprom_sim_stats;
STATS_NAME_START(eeprom_sim_stats)
STATS_NAME(eeprom_sim_stats, bytes_read)
STATS_NAME(eeprom_sim_stats, bytes_written)
STATS_NAME(eeprom_sim_stats, eeprom_read_calls)
STATS_NAME(eeprom_sim_stats, eeprom_read_time_us)
STATS_NAME(eeprom_sim_stats, eeprom_write_calls)
STATS_NAME(eeprom_sim_stats, eeprom_write_time_us)
STATS_NAME_END(eeprom_sim_stats);

/* simulator dynamic thresholds */
STATS_SECT_START(eeprom_sim_thresholds)
STATS_SECT_ENTRY32(max_write_calls)
STATS_SECT_ENTRY32(max_len)
STATS_SECT_END;

STATS_SECT_DECL(eeprom_sim_thresholds) eeprom_sim_thresholds;
STATS_NAME_START(eeprom_sim_thresholds)
STATS_NAME(eeprom_sim_thresholds, max_write_calls)
STATS_NAME(eeprom_sim_thresholds, max_len)
STATS_NAME_END(eeprom_sim_thresholds);

static u8_t mock_eeprom[DT_INST_0_ZEPHYR_SIM_EEPROM_SIZE];

static int eeprom_range_is_valid(struct device *dev, off_t offset, size_t len)
{
	const struct eeprom_sim_config *config = DEV_CONFIG(dev);

	if ((offset + len) < config->size) {
		return 1;
	}

	return 0;
}

static int eeprom_sim_read(struct device *dev, const off_t offset, void *data,
			  const size_t len)
{
	if (!len) {
		return 0;
	}

	if (!eeprom_range_is_valid(dev, offset, len)) {
		LOG_WRN("attempt to read past device boundary");
		return -EINVAL;
	}

	SYNC_LOCK();

	STATS_INC(eeprom_sim_stats, eeprom_read_calls);
	memcpy(data, EEPROM(offset), len);
	STATS_INCN(eeprom_sim_stats, bytes_read, len);

	SYNC_UNLOCK();

#ifdef CONFIG_EEPROM_SIMULATOR_SIMULATE_TIMING
	k_busy_wait(CONFIG_EEPROM_SIMULATOR_MIN_READ_TIME_US);
	STATS_INCN(eeprom_sim_stats, eeprom_read_time_us,
		   CONFIG_EEPROM_SIMULATOR_MIN_READ_TIME_US);
#endif

	return 0;
}

static int eeprom_sim_write(struct device *dev, const off_t offset,
			   const void *data, const size_t len)
{
	const struct eeprom_sim_config *config = DEV_CONFIG(dev);

	if (config->readonly) {
		LOG_WRN("attempt to write to read-only device");
		return -EACCES;
	}

	if (!len) {
		return 0;
	}

	if (!eeprom_range_is_valid(dev, offset, len)) {
		LOG_WRN("attempt to write past device boundary");
		return -EINVAL;
	}

	SYNC_LOCK();

	STATS_INC(eeprom_sim_stats, eeprom_write_calls);

	bool data_part_ignored = false;

	if (eeprom_sim_thresholds.max_write_calls != 0) {
		if (eeprom_sim_stats.eeprom_write_calls >
			eeprom_sim_thresholds.max_write_calls) {
			return 0;
		} else if (eeprom_sim_stats.eeprom_write_calls ==
				eeprom_sim_thresholds.max_write_calls) {
			if (eeprom_sim_thresholds.max_len == 0) {
				return 0;
			}

			data_part_ignored = true;
		}
	}

	for (u32_t i = 0; i < len; i++) {
		if (data_part_ignored) {
			if (i >= eeprom_sim_thresholds.max_len) {
				return 0;
			}
		}

		*(EEPROM(offset + i)) = *((u8_t *)data + i);
	}

	STATS_INCN(eeprom_sim_stats, bytes_written, len);

#ifdef CONFIG_EEPROM_SIMULATOR_SIMULATE_TIMING
	/* wait before returning */
	k_busy_wait(CONFIG_EEPROM_SIMULATOR_MIN_WRITE_TIME_US);
	STATS_INCN(eeprom_sim_stats, eeprom_write_time_us,
		   CONFIG_EEPROM_SIMULATOR_MIN_WRITE_TIME_US);
#endif

	SYNC_UNLOCK();

	return 0;
}

static size_t eeprom_sim_size(struct device *dev)
{
	const struct eeprom_sim_config *config = DEV_CONFIG(dev);

	return config->size;
}

static const struct eeprom_driver_api eeprom_sim_api = {
	.read = eeprom_sim_read,
	.write = eeprom_sim_write,
	.size = eeprom_sim_size,
};

static const struct eeprom_sim_config eeprom_sim_config_0 = {
	.size = DT_INST_0_ZEPHYR_SIM_EEPROM_SIZE,
	.readonly = DT_INST_0_ZEPHYR_SIM_EEPROM_READ_ONLY,
};

static int eeprom_sim_init(struct device *dev)
{
	SYNC_INIT();
	STATS_INIT_AND_REG(eeprom_sim_stats, STATS_SIZE_32, "eeprom_sim_stats");
	STATS_INIT_AND_REG(eeprom_sim_thresholds, STATS_SIZE_32,
			   "eeprom_sim_thresholds");
	memset(mock_eeprom, 0xFF, ARRAY_SIZE(mock_eeprom));

	return 0;
}

DEVICE_AND_API_INIT(eeprom_sim_0, DT_INST_0_ZEPHYR_SIM_EEPROM_LABEL,
		    &eeprom_sim_init, NULL, &eeprom_sim_config_0, POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &eeprom_sim_api);
