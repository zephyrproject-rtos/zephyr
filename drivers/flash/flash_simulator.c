/*
 * Copyright (c) 2023-2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT zephyr_sim_flash

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/linker/devicetree_regions.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/random/random.h>
#include <zephyr/stats/stats.h>
#include <string.h>

#ifdef CONFIG_ARCH_POSIX

#include "flash_simulator_native.h"
#include "cmdline.h"
#include "soc.h"
#define DEFAULT_FLASH_FILE_PATH "flash.bin"

#endif /* CONFIG_ARCH_POSIX */

/* configuration derived from DT */
#ifdef CONFIG_ARCH_POSIX
#define SOC_NV_FLASH_NODE DT_INST_CHILD(0, flash_0)
#else
#define SOC_NV_FLASH_NODE DT_INST_CHILD(0, flash_sim_0)
#endif /* CONFIG_ARCH_POSIX */

#define FLASH_SIMULATOR_BASE_OFFSET DT_REG_ADDR(SOC_NV_FLASH_NODE)
#define FLASH_SIMULATOR_ERASE_UNIT DT_PROP(SOC_NV_FLASH_NODE, erase_block_size)
#define FLASH_SIMULATOR_PROG_UNIT DT_PROP(SOC_NV_FLASH_NODE, write_block_size)
#define FLASH_SIMULATOR_FLASH_SIZE DT_REG_SIZE(SOC_NV_FLASH_NODE)

#define FLASH_SIMULATOR_ERASE_VALUE \
		DT_PROP(DT_PARENT(SOC_NV_FLASH_NODE), erase_value)

#define FLASH_SIMULATOR_PAGE_COUNT (FLASH_SIMULATOR_FLASH_SIZE / \
				    FLASH_SIMULATOR_ERASE_UNIT)

#if (FLASH_SIMULATOR_ERASE_UNIT % FLASH_SIMULATOR_PROG_UNIT)
#error "Erase unit must be a multiple of program unit"
#endif

#define MOCK_FLASH(offset) (mock_flash + (offset))

/* maximum number of pages that can be tracked by the stats module */
#define STATS_PAGE_COUNT_THRESHOLD 256

#define STATS_SECT_EC(N, _) STATS_SECT_ENTRY32(erase_cycles_unit##N)
#define STATS_NAME_EC(N, _) STATS_NAME(flash_sim_stats, erase_cycles_unit##N)

#define STATS_SECT_DIRTYR(N, _) STATS_SECT_ENTRY32(dirty_read_unit##N)
#define STATS_NAME_DIRTYR(N, _) STATS_NAME(flash_sim_stats, dirty_read_unit##N)

#ifdef CONFIG_FLASH_SIMULATOR_STATS
/* increment a unit erase cycles counter */
#define ERASE_CYCLES_INC(U)						     \
	do {								     \
		if (U < STATS_PAGE_COUNT_THRESHOLD) {			     \
			(*(&flash_sim_stats.erase_cycles_unit0 + (U)) += 1); \
		}							     \
	} while (false)

#if (CONFIG_FLASH_SIMULATOR_STAT_PAGE_COUNT > STATS_PAGE_COUNT_THRESHOLD)
       /* Limitation above is caused by used LISTIFY                        */
       /* Using FLASH_SIMULATOR_FLASH_PAGE_COUNT allows to avoid terrible   */
       /* error logg at the output and work with the stats module partially */
       #define FLASH_SIMULATOR_FLASH_PAGE_COUNT STATS_PAGE_COUNT_THRESHOLD
#else
#define FLASH_SIMULATOR_FLASH_PAGE_COUNT CONFIG_FLASH_SIMULATOR_STAT_PAGE_COUNT
#endif

/* simulator statistics */
STATS_SECT_START(flash_sim_stats)
STATS_SECT_ENTRY32(bytes_read)		/* total bytes read */
STATS_SECT_ENTRY32(bytes_written)       /* total bytes written */
STATS_SECT_ENTRY32(double_writes)       /* num. of writes to non-erased units */
STATS_SECT_ENTRY32(flash_read_calls)    /* calls to flash_read() */
STATS_SECT_ENTRY32(flash_read_time_us)  /* time spent in flash_read() */
STATS_SECT_ENTRY32(flash_write_calls)   /* calls to flash_write() */
STATS_SECT_ENTRY32(flash_write_time_us) /* time spent in flash_write() */
STATS_SECT_ENTRY32(flash_erase_calls)   /* calls to flash_erase() */
STATS_SECT_ENTRY32(flash_erase_time_us) /* time spent in flash_erase() */
/* -- per-unit statistics -- */
/* erase cycle count for unit */
LISTIFY(FLASH_SIMULATOR_FLASH_PAGE_COUNT, STATS_SECT_EC, ())
/* number of read operations on worn out erase units */
LISTIFY(FLASH_SIMULATOR_FLASH_PAGE_COUNT, STATS_SECT_DIRTYR, ())
STATS_SECT_END;

STATS_SECT_DECL(flash_sim_stats) flash_sim_stats;
STATS_NAME_START(flash_sim_stats)
STATS_NAME(flash_sim_stats, bytes_read)
STATS_NAME(flash_sim_stats, bytes_written)
STATS_NAME(flash_sim_stats, double_writes)
STATS_NAME(flash_sim_stats, flash_read_calls)
STATS_NAME(flash_sim_stats, flash_read_time_us)
STATS_NAME(flash_sim_stats, flash_write_calls)
STATS_NAME(flash_sim_stats, flash_write_time_us)
STATS_NAME(flash_sim_stats, flash_erase_calls)
STATS_NAME(flash_sim_stats, flash_erase_time_us)
LISTIFY(FLASH_SIMULATOR_FLASH_PAGE_COUNT, STATS_NAME_EC, ())
LISTIFY(FLASH_SIMULATOR_FLASH_PAGE_COUNT, STATS_NAME_DIRTYR, ())
STATS_NAME_END(flash_sim_stats);

/* simulator dynamic thresholds */
STATS_SECT_START(flash_sim_thresholds)
STATS_SECT_ENTRY32(max_write_calls)
STATS_SECT_ENTRY32(max_erase_calls)
STATS_SECT_ENTRY32(max_len)
STATS_SECT_END;

STATS_SECT_DECL(flash_sim_thresholds) flash_sim_thresholds;
STATS_NAME_START(flash_sim_thresholds)
STATS_NAME(flash_sim_thresholds, max_write_calls)
STATS_NAME(flash_sim_thresholds, max_erase_calls)
STATS_NAME(flash_sim_thresholds, max_len)
STATS_NAME_END(flash_sim_thresholds);

#define FLASH_SIM_STATS_INC(group__, var__) STATS_INC(group__, var__)
#define FLASH_SIM_STATS_INCN(group__, var__, n__) STATS_INCN(group__, var__, n__)
#define FLASH_SIM_STATS_INIT_AND_REG(group__, size__, name__) \
	STATS_INIT_AND_REG(group__, size__, name__)


#else

#define ERASE_CYCLES_INC(U) do {} while (false)
#define FLASH_SIM_STATS_INC(group__, var__)
#define FLASH_SIM_STATS_INCN(group__, var__, n__)
#define FLASH_SIM_STATS_INIT_AND_REG(group__, size__, name__)

#endif /* CONFIG_FLASH_SIMULATOR_STATS */


#ifdef CONFIG_ARCH_POSIX
static uint8_t *mock_flash;
static int flash_fd = -1;
static const char *flash_file_path;
static bool flash_erase_at_start;
static bool flash_rm_at_exit;
static bool flash_in_ram;
#else
#if DT_NODE_HAS_PROP(DT_PARENT(SOC_NV_FLASH_NODE), memory_region)
#define FLASH_SIMULATOR_MREGION \
	LINKER_DT_NODE_REGION_NAME( \
	DT_PHANDLE(DT_PARENT(SOC_NV_FLASH_NODE), memory_region))
static uint8_t mock_flash[FLASH_SIMULATOR_FLASH_SIZE] Z_GENERIC_SECTION(FLASH_SIMULATOR_MREGION);
#else
static uint8_t mock_flash[FLASH_SIMULATOR_FLASH_SIZE];
#endif
#endif /* CONFIG_ARCH_POSIX */

static DEVICE_API(flash, flash_sim_api);

static const struct flash_parameters flash_sim_parameters = {
	.write_block_size = FLASH_SIMULATOR_PROG_UNIT,
	.erase_value = FLASH_SIMULATOR_ERASE_VALUE,
	.caps = {
#if !defined(CONFIG_FLASH_SIMULATOR_EXPLICIT_ERASE)
		.no_explicit_erase = false,
#endif
	},
};

static int flash_range_is_valid(const struct device *dev, off_t offset,
				size_t len)
{
	ARG_UNUSED(dev);

	if ((offset < 0 || offset >= FLASH_SIMULATOR_FLASH_SIZE ||
	     (FLASH_SIMULATOR_FLASH_SIZE - offset) < len)) {
		return 0;
	}

	return 1;
}

static int flash_sim_read(const struct device *dev, const off_t offset,
			  void *data,
			  const size_t len)
{
	ARG_UNUSED(dev);

	if (!flash_range_is_valid(dev, offset, len)) {
		return -EINVAL;
	}

	if (!IS_ENABLED(CONFIG_FLASH_SIMULATOR_UNALIGNED_READ)) {
		if ((offset % FLASH_SIMULATOR_PROG_UNIT) ||
		    (len % FLASH_SIMULATOR_PROG_UNIT)) {
			return -EINVAL;
		}
	}

	FLASH_SIM_STATS_INC(flash_sim_stats, flash_read_calls);

	memcpy(data, MOCK_FLASH(offset), len);
	FLASH_SIM_STATS_INCN(flash_sim_stats, bytes_read, len);

#ifdef CONFIG_FLASH_SIMULATOR_SIMULATE_TIMING
	k_busy_wait(CONFIG_FLASH_SIMULATOR_MIN_READ_TIME_US);
	FLASH_SIM_STATS_INCN(flash_sim_stats, flash_read_time_us,
		   CONFIG_FLASH_SIMULATOR_MIN_READ_TIME_US);
#endif

	return 0;
}

static int flash_sim_write(const struct device *dev, const off_t offset,
			   const void *data, const size_t len)
{
	uint8_t buf[FLASH_SIMULATOR_PROG_UNIT];
	ARG_UNUSED(dev);

	if (!flash_range_is_valid(dev, offset, len)) {
		return -EINVAL;
	}

	if ((offset % FLASH_SIMULATOR_PROG_UNIT) ||
	    (len % FLASH_SIMULATOR_PROG_UNIT)) {
		return -EINVAL;
	}

	FLASH_SIM_STATS_INC(flash_sim_stats, flash_write_calls);

#if defined(CONFIG_FLASH_SIMULATOR_EXPLICIT_ERASE)
	/* check if any unit has been already programmed */
	memset(buf, FLASH_SIMULATOR_ERASE_VALUE, sizeof(buf));
#else
	memcpy(buf, MOCK_FLASH(offset), sizeof(buf));
#endif
	for (uint32_t i = 0; i < len; i += FLASH_SIMULATOR_PROG_UNIT) {
		if (memcmp(buf, MOCK_FLASH(offset + i), sizeof(buf))) {
			FLASH_SIM_STATS_INC(flash_sim_stats, double_writes);
#if !CONFIG_FLASH_SIMULATOR_DOUBLE_WRITES
			return -EIO;
#endif
		}
	}

#ifdef CONFIG_FLASH_SIMULATOR_STATS
	bool data_part_ignored = false;

	if (flash_sim_thresholds.max_write_calls != 0) {
		if (flash_sim_stats.flash_write_calls >
			flash_sim_thresholds.max_write_calls) {
			return 0;
		} else if (flash_sim_stats.flash_write_calls ==
				flash_sim_thresholds.max_write_calls) {
			if (flash_sim_thresholds.max_len == 0) {
				return 0;
			}

			data_part_ignored = true;
		}
	}
#endif

	for (uint32_t i = 0; i < len; i++) {
#ifdef CONFIG_FLASH_SIMULATOR_STATS
		if (data_part_ignored) {
			if (i >= flash_sim_thresholds.max_len) {
				return 0;
			}
		}
#endif /* CONFIG_FLASH_SIMULATOR_STATS */

		/* only pull bits to zero */
#if defined(CONFIG_FLASH_SIMULATOR_EXPLICIT_ERASE)
#if FLASH_SIMULATOR_ERASE_VALUE == 0xFF
		*(MOCK_FLASH(offset + i)) &= *((uint8_t *)data + i);
#else
		*(MOCK_FLASH(offset + i)) |= *((uint8_t *)data + i);
#endif
#else
		*(MOCK_FLASH(offset + i)) = *((uint8_t *)data + i);
#endif
	}

	FLASH_SIM_STATS_INCN(flash_sim_stats, bytes_written, len);

#ifdef CONFIG_FLASH_SIMULATOR_SIMULATE_TIMING
	/* wait before returning */
	k_busy_wait(CONFIG_FLASH_SIMULATOR_MIN_WRITE_TIME_US);
	FLASH_SIM_STATS_INCN(flash_sim_stats, flash_write_time_us,
		   CONFIG_FLASH_SIMULATOR_MIN_WRITE_TIME_US);
#endif

	return 0;
}

static void unit_erase(const uint32_t unit)
{
	const off_t unit_addr = unit * FLASH_SIMULATOR_ERASE_UNIT;

	/* erase the memory unit by setting it to erase value */
	memset(MOCK_FLASH(unit_addr), FLASH_SIMULATOR_ERASE_VALUE,
	       FLASH_SIMULATOR_ERASE_UNIT);
}

static int flash_sim_erase(const struct device *dev, const off_t offset,
			   const size_t len)
{
	ARG_UNUSED(dev);

	if (!flash_range_is_valid(dev, offset, len)) {
		return -EINVAL;
	}

	/* erase operation must be aligned to the erase unit boundary */
	if ((offset % FLASH_SIMULATOR_ERASE_UNIT) ||
	    (len % FLASH_SIMULATOR_ERASE_UNIT)) {
		return -EINVAL;
	}

	FLASH_SIM_STATS_INC(flash_sim_stats, flash_erase_calls);

#ifdef CONFIG_FLASH_SIMULATOR_STATS
	if ((flash_sim_thresholds.max_erase_calls != 0) &&
	    (flash_sim_stats.flash_erase_calls >=
		flash_sim_thresholds.max_erase_calls)){
		return 0;
	}
#endif
	/* the first unit to be erased */
	uint32_t unit_start = offset / FLASH_SIMULATOR_ERASE_UNIT;

	/* erase as many units as necessary and increase their erase counter */
	for (uint32_t i = 0; i < len / FLASH_SIMULATOR_ERASE_UNIT; i++) {
		ERASE_CYCLES_INC(unit_start + i);
		unit_erase(unit_start + i);
	}

#ifdef CONFIG_FLASH_SIMULATOR_SIMULATE_TIMING
	/* wait before returning */
	k_busy_wait(CONFIG_FLASH_SIMULATOR_MIN_ERASE_TIME_US);
	FLASH_SIM_STATS_INCN(flash_sim_stats, flash_erase_time_us,
		   CONFIG_FLASH_SIMULATOR_MIN_ERASE_TIME_US);
#endif

	return 0;
}

#ifdef CONFIG_FLASH_PAGE_LAYOUT
static const struct flash_pages_layout flash_sim_pages_layout = {
	.pages_count = FLASH_SIMULATOR_PAGE_COUNT,
	.pages_size = FLASH_SIMULATOR_ERASE_UNIT,
};

static void flash_sim_page_layout(const struct device *dev,
				  const struct flash_pages_layout **layout,
				  size_t *layout_size)
{
	*layout = &flash_sim_pages_layout;
	*layout_size = 1;
}
#endif

static int flash_sim_get_size(const struct device *dev, uint64_t *size)
{
	ARG_UNUSED(dev);

	*size = FLASH_SIMULATOR_FLASH_SIZE;

	return 0;
}
static const struct flash_parameters *
flash_sim_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	return &flash_sim_parameters;
}

static DEVICE_API(flash, flash_sim_api) = {
	.read = flash_sim_read,
	.write = flash_sim_write,
	.erase = flash_sim_erase,
	.get_parameters = flash_sim_get_parameters,
	.get_size = flash_sim_get_size,
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	.page_layout = flash_sim_page_layout,
#endif
};

#ifdef CONFIG_ARCH_POSIX

static int flash_mock_init(const struct device *dev)
{
	int rc;
	ARG_UNUSED(dev);

	if (flash_in_ram == false && flash_file_path == NULL) {
		flash_file_path = DEFAULT_FLASH_FILE_PATH;
	}

	rc = flash_mock_init_native(flash_in_ram, &mock_flash, FLASH_SIMULATOR_FLASH_SIZE,
				    &flash_fd, flash_file_path, FLASH_SIMULATOR_ERASE_VALUE,
				    flash_erase_at_start);

	if (rc < 0) {
		return -EIO;
	} else {
		return 0;
	}
}

#else
#if DT_NODE_HAS_PROP(DT_PARENT(SOC_NV_FLASH_NODE), memory_region)
static int flash_mock_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	return 0;
}
#else
static int flash_mock_init(const struct device *dev)
{
	ARG_UNUSED(dev);
	memset(mock_flash, FLASH_SIMULATOR_ERASE_VALUE, ARRAY_SIZE(mock_flash));
	return 0;
}
#endif /* DT_NODE_HAS_PROP(DT_PARENT(SOC_NV_FLASH_NODE), memory_region) */
#endif /* CONFIG_ARCH_POSIX */

static int flash_init(const struct device *dev)
{
	FLASH_SIM_STATS_INIT_AND_REG(flash_sim_stats, STATS_SIZE_32, "flash_sim_stats");
	FLASH_SIM_STATS_INIT_AND_REG(flash_sim_thresholds, STATS_SIZE_32,
			   "flash_sim_thresholds");
	return flash_mock_init(dev);
}

DEVICE_DT_INST_DEFINE(0, flash_init, NULL,
		    NULL, NULL, POST_KERNEL, CONFIG_FLASH_INIT_PRIORITY,
		    &flash_sim_api);

#ifdef CONFIG_ARCH_POSIX

static void flash_native_cleanup(void)
{
	flash_mock_cleanup_native(flash_in_ram, flash_fd, mock_flash,
				  FLASH_SIMULATOR_FLASH_SIZE, flash_file_path,
				  flash_rm_at_exit);
}

static void flash_native_options(void)
{
	static struct args_struct_t flash_options[] = {
		{ .option = "flash",
		  .name = "path",
		  .type = 's',
		  .dest = (void *)&flash_file_path,
		  .descript = "Path to binary file to be used as flash, by default \""
				DEFAULT_FLASH_FILE_PATH "\""},
		{ .is_switch = true,
		  .option = "flash_erase",
		  .type = 'b',
		  .dest = (void *)&flash_erase_at_start,
		  .descript = "Erase the flash content at startup" },
		{ .is_switch = true,
		  .option = "flash_rm",
		  .type = 'b',
		  .dest = (void *)&flash_rm_at_exit,
		  .descript = "Remove the flash file when terminating the execution" },
		{ .is_switch = true,
		  .option = "flash_in_ram",
		  .type = 'b',
		  .dest = (void *)&flash_in_ram,
		  .descript = "Instead of a file, keep the file content just in RAM. If this is "
			      "set, flash, flash_erase & flash_rm are ignored. The flash content"
			      " is always erased at startup" },
		ARG_TABLE_ENDMARKER
	};

	native_add_command_line_opts(flash_options);
}

NATIVE_TASK(flash_native_options, PRE_BOOT_1, 1);
NATIVE_TASK(flash_native_cleanup, ON_EXIT, 1);

#endif /* CONFIG_ARCH_POSIX */

/* Extension to generic flash driver API */
void *z_impl_flash_simulator_get_memory(const struct device *dev,
					size_t *mock_size)
{
	ARG_UNUSED(dev);

	*mock_size = FLASH_SIMULATOR_FLASH_SIZE;
	return mock_flash;
}

#ifdef CONFIG_USERSPACE

#include <zephyr/internal/syscall_handler.h>

void *z_vrfy_flash_simulator_get_memory(const struct device *dev,
				      size_t *mock_size)
{
	K_OOPS(K_SYSCALL_SPECIFIC_DRIVER(dev, K_OBJ_DRIVER_FLASH, &flash_sim_api));

	return z_impl_flash_simulator_get_memory(dev, mock_size);
}

#include <zephyr/syscalls/flash_simulator_get_memory_mrsh.c>

#endif /* CONFIG_USERSPACE */
