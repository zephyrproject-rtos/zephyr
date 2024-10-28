/*
 * Copyright (c) 2019 Laczen
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_sim_eeprom

#ifdef CONFIG_ARCH_POSIX
#include "eeprom_simulator_native.h"
#include "cmdline.h"
#include "soc.h"
#endif /* CONFIG_ARCH_POSIX */

#include <zephyr/device.h>
#include <zephyr/drivers/eeprom.h>

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/stats/stats.h>
#include <string.h>
#include <errno.h>

#define LOG_LEVEL CONFIG_EEPROM_LOG_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(eeprom_simulator);

struct eeprom_sim_config {
	size_t size;
	bool readonly;
};

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

/* simulator statistics */
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

#ifdef CONFIG_ARCH_POSIX
static char *mock_eeprom;
static int eeprom_fd = -1;
static const char *eeprom_file_path;
#define DEFAULT_EEPROM_FILE_PATH "eeprom.bin"
static bool eeprom_erase_at_start;
static bool eeprom_rm_at_exit;
static bool eeprom_in_ram;
#else
static uint8_t mock_eeprom[DT_INST_PROP(0, size)];
#endif /* CONFIG_ARCH_POSIX */

static int eeprom_range_is_valid(const struct device *dev, off_t offset,
				 size_t len)
{
	const struct eeprom_sim_config *config = dev->config;

	if ((offset + len) <= config->size) {
		return 1;
	}

	return 0;
}

static int eeprom_sim_read(const struct device *dev, off_t offset, void *data,
			   size_t len)
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

static int eeprom_sim_write(const struct device *dev, off_t offset,
			    const void *data,
			    size_t len)
{
	const struct eeprom_sim_config *config = dev->config;

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
			goto end;
		} else if (eeprom_sim_stats.eeprom_write_calls ==
				eeprom_sim_thresholds.max_write_calls) {
			if (eeprom_sim_thresholds.max_len == 0) {
				goto end;
			}

			data_part_ignored = true;
		}
	}

	if ((data_part_ignored) && (len > eeprom_sim_thresholds.max_len)) {
		len = eeprom_sim_thresholds.max_len;
	}

	memcpy(EEPROM(offset), data, len);

	STATS_INCN(eeprom_sim_stats, bytes_written, len);

#ifdef CONFIG_EEPROM_SIMULATOR_SIMULATE_TIMING
	/* wait before returning */
	k_busy_wait(CONFIG_EEPROM_SIMULATOR_MIN_WRITE_TIME_US);
	STATS_INCN(eeprom_sim_stats, eeprom_write_time_us,
		   CONFIG_EEPROM_SIMULATOR_MIN_WRITE_TIME_US);
#endif

end:
	SYNC_UNLOCK();
	return 0;
}

static size_t eeprom_sim_size(const struct device *dev)
{
	const struct eeprom_sim_config *config = dev->config;

	return config->size;
}

static const struct eeprom_driver_api eeprom_sim_api = {
	.read = eeprom_sim_read,
	.write = eeprom_sim_write,
	.size = eeprom_sim_size,
};

static const struct eeprom_sim_config eeprom_sim_config_0 = {
	.size = DT_INST_PROP(0, size),
	.readonly = DT_INST_PROP(0, read_only),
};

#ifdef CONFIG_ARCH_POSIX

static int eeprom_mock_init(const struct device *dev)
{
	int rc;

	ARG_UNUSED(dev);

	if (eeprom_in_ram == false && eeprom_file_path == NULL) {
		eeprom_file_path = DEFAULT_EEPROM_FILE_PATH;
	}

	rc = eeprom_mock_init_native(eeprom_in_ram, &mock_eeprom, DT_INST_PROP(0, size), &eeprom_fd,
				     eeprom_file_path, 0xFF, eeprom_erase_at_start);

	if (rc < 0) {
		return -EIO;
	} else {
		return 0;
	}
}

#else

static int eeprom_mock_init(const struct device *dev)
{
	memset(mock_eeprom, 0xFF, ARRAY_SIZE(mock_eeprom));
	return 0;
}

#endif /* CONFIG_ARCH_POSIX */

static int eeprom_sim_init(const struct device *dev)
{
	SYNC_INIT();
	STATS_INIT_AND_REG(eeprom_sim_stats, STATS_SIZE_32, "eeprom_sim_stats");
	STATS_INIT_AND_REG(eeprom_sim_thresholds, STATS_SIZE_32,
			   "eeprom_sim_thresholds");

	return eeprom_mock_init(dev);
}

DEVICE_DT_INST_DEFINE(0, &eeprom_sim_init, NULL, NULL, &eeprom_sim_config_0, POST_KERNEL,
		      CONFIG_EEPROM_INIT_PRIORITY, &eeprom_sim_api);

#ifdef CONFIG_ARCH_POSIX

static void eeprom_native_cleanup(void)
{
	eeprom_mock_cleanup_native(eeprom_in_ram, eeprom_fd, mock_eeprom, DT_INST_PROP(0, size),
				   eeprom_file_path, eeprom_rm_at_exit);
}

static void eeprom_native_options(void)
{
	static struct args_struct_t eeprom_options[] = {
		{.option = "eeprom",
		 .name = "path",
		 .type = 's',
		 .dest = (void *)&eeprom_file_path,
		 .descript = "Path to binary file to be used as EEPROM, by default "
			     "\"" DEFAULT_EEPROM_FILE_PATH "\""},
		{.is_switch = true,
		 .option = "eeprom_erase",
		 .type = 'b',
		 .dest = (void *)&eeprom_erase_at_start,
		 .descript = "Erase the EEPROM content at startup"},
		{.is_switch = true,
		 .option = "eeprom_rm",
		 .type = 'b',
		 .dest = (void *)&eeprom_rm_at_exit,
		 .descript = "Remove the EEPROM file when terminating the execution"},
		{.is_switch = true,
		 .option = "eeprom_in_ram",
		 .type = 'b',
		 .dest = (void *)&eeprom_in_ram,
		 .descript = "Instead of a file, keep the file content just in RAM. If this is "
			     "set, eeprom, eeprom_erase & eeprom_rm are ignored, and the EEPROM "
			     "content is always erased at startup"},
		ARG_TABLE_ENDMARKER
	};

	native_add_command_line_opts(eeprom_options);
}

NATIVE_TASK(eeprom_native_options, PRE_BOOT_1, 1);
NATIVE_TASK(eeprom_native_cleanup, ON_EXIT, 1);

#endif /* CONFIG_ARCH_POSIX */
