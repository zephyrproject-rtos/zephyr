/*
 * Copyright (c) 2019 Laczen
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_sim_eeprom

#include <device.h>
#include <drivers/eeprom.h>

#include <init.h>
#include <kernel.h>
#include <sys/util.h>
#include <stats/stats.h>
#include <string.h>
#include <errno.h>

#ifdef CONFIG_ARCH_POSIX
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include "cmdline.h"
#include "soc.h"
#endif

#define LOG_LEVEL CONFIG_EEPROM_LOG_LEVEL
#include <logging/log.h>
LOG_MODULE_REGISTER(eeprom_simulator);

struct eeprom_sim_config {
	size_t size;
	bool readonly;
};

#define DEV_NAME(dev) ((dev)->name)
#define DEV_CONFIG(dev) ((dev)->config)

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

#ifdef CONFIG_ARCH_POSIX
static uint8_t *mock_eeprom;
static int eeprom_fd = -1;
static const char *eeprom_file_path;
static const char default_eeprom_file_path[] = "eeprom.bin";
#else
static uint8_t mock_eeprom[DT_INST_PROP(0, size)];
#endif /* CONFIG_ARCH_POSIX */

static int eeprom_range_is_valid(const struct device *dev, off_t offset,
				 size_t len)
{
	const struct eeprom_sim_config *config = DEV_CONFIG(dev);

	if ((offset + len) < config->size) {
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
	const struct eeprom_sim_config *config = DEV_CONFIG(dev);

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
	if (eeprom_file_path == NULL) {
		eeprom_file_path = default_eeprom_file_path;
	}

	eeprom_fd = open(eeprom_file_path, O_RDWR | O_CREAT, (mode_t)0600);
	if (eeprom_fd == -1) {
		posix_print_warning("Failed to open eeprom device file ",
				    "%s: %s\n",
				    eeprom_file_path, strerror(errno));
		return -EIO;
	}

	if (ftruncate(eeprom_fd, DT_INST_PROP(0, size)) == -1) {
		posix_print_warning("Failed to resize eeprom device file ",
				    "%s: %s\n",
				    eeprom_file_path, strerror(errno));
		return -EIO;
	}

	mock_eeprom = mmap(NULL, DT_INST_PROP(0, size),
			  PROT_WRITE | PROT_READ, MAP_SHARED, eeprom_fd, 0);
	if (mock_eeprom == MAP_FAILED) {
		posix_print_warning("Failed to mmap eeprom device file "
				    "%s: %s\n",
				    eeprom_file_path, strerror(errno));
		return -EIO;
	}

	return 0;
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

DEVICE_AND_API_INIT(eeprom_sim_0, DT_INST_LABEL(0),
		    &eeprom_sim_init, NULL, &eeprom_sim_config_0, POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &eeprom_sim_api);

#ifdef CONFIG_ARCH_POSIX

static void eeprom_native_posix_cleanup(void)
{
	if ((mock_eeprom != MAP_FAILED) && (mock_eeprom != NULL)) {
		munmap(mock_eeprom, DT_INST_PROP(0, size));
	}

	if (eeprom_fd != -1) {
		close(eeprom_fd);
	}
}

static void eeprom_native_posix_options(void)
{
	static struct args_struct_t eeprom_options[] = {
		{ .manual = false,
		  .is_mandatory = false,
		  .is_switch = false,
		  .option = "eeprom",
		  .name = "path",
		  .type = 's',
		  .dest = (void *)&eeprom_file_path,
		  .call_when_found = NULL,
		  .descript = "Path to binary file to be used as eeprom" },
		ARG_TABLE_ENDMARKER
	};

	native_add_command_line_opts(eeprom_options);
}


NATIVE_TASK(eeprom_native_posix_options, PRE_BOOT_1, 1);
NATIVE_TASK(eeprom_native_posix_cleanup, ON_EXIT, 1);

#endif /* CONFIG_ARCH_POSIX */
