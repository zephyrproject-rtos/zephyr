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

#include <zephyr/drivers/flash/flash_simulator.h>

#ifdef CONFIG_ARCH_POSIX

#include "flash_simulator_native.h"
#include "cmdline.h"
#include "soc.h"
#define DEFAULT_FLASH_FILE_PATH "flash.bin"

#endif /* CONFIG_ARCH_POSIX */

struct flash_simulator_config {
	uint8_t *mock_flash;
	uint8_t *prog_unit_buf;
	const struct flash_parameters *flash_parameters;
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	const struct flash_pages_layout *pages_layout;
#endif
	size_t base_offset;
	size_t erase_unit;
	size_t flash_size;
};

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

static DEVICE_API(flash, flash_sim_api);

#ifdef CONFIG_FLASH_SIMULATOR_CALLBACKS
static const struct flash_simulator_params flash_sim_params = {
	.memory_size = FLASH_SIMULATOR_FLASH_SIZE,
	.base_offset = FLASH_SIMULATOR_BASE_OFFSET,
	.erase_unit = FLASH_SIMULATOR_ERASE_UNIT,
	.prog_unit = FLASH_SIMULATOR_PROG_UNIT,
	.explicit_erase = IS_ENABLED(CONFIG_FLASH_SIMULATOR_EXPLICIT_ERASE),
	.erase_value = FLASH_SIMULATOR_ERASE_VALUE,
};
static const struct flash_simulator_cb *flash_simulator_cbs;
#endif /* CONFIG_FLASH_SIMULATOR_CALLBACKS */


static int flash_range_is_valid(const struct device *dev, off_t offset,
				size_t len)
{
	const struct flash_simulator_config *cfg = dev->config;

	if ((offset < 0 || offset >= cfg->flash_size ||
	     (cfg->flash_size - offset) < len)) {
		return 0;
	}

	return 1;
}

static int flash_sim_read(const struct device *dev, const off_t offset,
			  void *data,
			  const size_t len)
{
	const struct flash_simulator_config *cfg = dev->config;

	if (!flash_range_is_valid(dev, offset, len)) {
		return -EINVAL;
	}

	if (!IS_ENABLED(CONFIG_FLASH_SIMULATOR_UNALIGNED_READ)) {
		if ((offset % cfg->flash_parameters->write_block_size) ||
		    (len % cfg->flash_parameters->write_block_size)) {
			return -EINVAL;
		}
	}

	FLASH_SIM_STATS_INC(flash_sim_stats, flash_read_calls);

	memcpy(data, cfg->mock_flash + offset, len);
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
	const struct flash_simulator_config *cfg = dev->config;

	if (!flash_range_is_valid(dev, offset, len)) {
		return -EINVAL;
	}

	if ((offset % cfg->flash_parameters->write_block_size) ||
	    (len % cfg->flash_parameters->write_block_size)) {
		return -EINVAL;
	}

	FLASH_SIM_STATS_INC(flash_sim_stats, flash_write_calls);

#ifdef CONFIG_FLASH_SIMULATOR_EXPLICIT_ERASE
	/* check if any unit has been already programmed */
	memset(cfg->prog_unit_buf, cfg->flash_parameters->erase_value, cfg->flash_parameters->write_block_size);
#else
	memcpy(cfg->prog_unit_buf, cfg->mock_flash + offset, cfg->flash_parameters->write_block_size);
#endif
	for (uint32_t i = 0; i < len; i += cfg->flash_parameters->write_block_size) {
		if (memcmp(cfg->prog_unit_buf, cfg->mock_flash + offset + i, cfg->flash_parameters->write_block_size)) {
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

#ifdef CONFIG_FLASH_SIMULATOR_CALLBACKS
	flash_simulator_write_byte_cb_t write_cb = NULL;
	const struct flash_simulator_cb *cb = flash_simulator_cbs;

	if (cb != NULL) {
		write_cb = cb->write_byte;
	}
#endif /* CONFIG_FLASH_SIMULATOR_CALLBACKS */

	for (uint32_t i = 0; i < len; i++) {
#ifdef CONFIG_FLASH_SIMULATOR_STATS
		if (data_part_ignored) {
			if (i >= flash_sim_thresholds.max_len) {
				return 0;
			}
		}
#endif /* CONFIG_FLASH_SIMULATOR_STATS */

		uint8_t data_val = *((const uint8_t *)data + i);

#ifdef CONFIG_FLASH_SIMULATOR_EXPLICIT_ERASE
		if (cfg->flash_parameters->erase_value == 0xff) {
			/* only pull bits to zero */
			data_val &= *(cfg->mock_flash + offset + i);
		} else {
			/* only pull bits to one */
			data_val |= *(cfg->mock_flash + offset + i);
		}
#endif
#ifdef CONFIG_FLASH_SIMULATOR_CALLBACKS
		if (write_cb != NULL) {
			int ret = write_cb(dev, offset + i, data_val);

			if (ret < 0) {
				return ret;
			}
			data_val = (uint8_t)ret;
		}
#endif /* CONFIG_FLASH_SIMULATOR_CALLBACKS */
		*(cfg->mock_flash + offset + i) = data_val;
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

static int unit_erase(const struct device *dev, const uint32_t unit)
{
	const struct flash_simulator_config *cfg = dev->config;
	const off_t unit_addr = unit * cfg->erase_unit;

#ifdef CONFIG_FLASH_SIMULATOR_CALLBACKS
	flash_simulator_erase_unit_cb_t erase_cb = NULL;
	const struct flash_simulator_cb *cb = flash_simulator_cbs;

	if (cb != NULL) {
		erase_cb = cb->erase_unit;
	}
	if (erase_cb != NULL) {
		return erase_cb(dev, unit_addr);
	}
#endif /* CONFIG_FLASH_SIMULATOR_CALLBACKS */

	/* erase the memory unit by setting it to erase value */
	memset(cfg->mock_flash + unit_addr, cfg->flash_parameters->erase_value,
	       cfg->erase_unit);
	return 0;
}

static int flash_sim_erase(const struct device *dev, const off_t offset,
			   const size_t len)
{
	const struct flash_simulator_config *cfg = dev->config;

	if (!flash_range_is_valid(dev, offset, len)) {
		return -EINVAL;
	}

	/* erase operation must be aligned to the erase unit boundary */
	if ((offset % cfg->erase_unit) ||
	    (len % cfg->erase_unit)) {
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
	uint32_t unit_start = offset / cfg->erase_unit;

	/* erase as many units as necessary and increase their erase counter */
	for (uint32_t i = 0; i < len / cfg->erase_unit; i++) {
		int ret;

		ERASE_CYCLES_INC(unit_start + i);
		ret = unit_erase(dev, unit_start + i);
		if (ret < 0) {
			return ret;
		}
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
static void flash_sim_page_layout(const struct device *dev,
				  const struct flash_pages_layout **layout,
				  size_t *layout_size)
{
	const struct flash_simulator_config *cfg = dev->config;

	*layout = cfg->pages_layout;
	*layout_size = 1;
}
#endif

static int flash_sim_get_size(const struct device *dev, uint64_t *size)
{
	const struct flash_simulator_config *cfg = dev->config;

	*size = cfg->flash_size;

	return 0;
}
static const struct flash_parameters *
flash_sim_get_parameters(const struct device *dev)
{
	const struct flash_simulator_config *cfg = dev->config;

	return cfg->flash_parameters;
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
	const struct flash_simulator_config *cfg = dev->config;

	memset(cfg->mock_flash, cfg->flash_parameters->erase_value,cfg->flash_size);
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
	const struct flash_simulator_config *cfg = dev->config;

	*mock_size = cfg->flash_size;
	return cfg->mock_flash;
}

const struct flash_simulator_params *z_impl_flash_simulator_get_params(const struct device *dev)
{
	ARG_UNUSED(dev);

#ifdef CONFIG_FLASH_SIMULATOR_CALLBACKS
	return &flash_sim_params;
#else
	return NULL;
#endif
}

#ifdef CONFIG_FLASH_SIMULATOR_CALLBACKS
void z_impl_flash_simulator_set_callbacks(const struct device *dev,
					  const struct flash_simulator_cb *cb)
{
	ARG_UNUSED(dev);
	flash_simulator_cbs = cb;
}
#endif /* CONFIG_FLASH_SIMULATOR_CALLBACKS */

#ifdef CONFIG_USERSPACE

#include <zephyr/internal/syscall_handler.h>

void *z_vrfy_flash_simulator_get_memory(const struct device *dev,
				      size_t *mock_size)
{
	K_OOPS(K_SYSCALL_SPECIFIC_DRIVER(dev, K_OBJ_DRIVER_FLASH, &flash_sim_api));

	return z_impl_flash_simulator_get_memory(dev, mock_size);
}

void z_vrfy_flash_simulator_set_callbacks(const struct device *dev,
					 const struct flash_simulator_cb *cb)
{
	K_OOPS(K_SYSCALL_SPECIFIC_DRIVER(dev, K_OBJ_DRIVER_FLASH, &flash_sim_api));

	z_impl_flash_simulator_set_callbacks(dev, cb);
}

const struct flash_simulator_params *z_vrfy_flash_simulator_get_params(const struct device *dev)
{
	K_OOPS(K_SYSCALL_SPECIFIC_DRIVER(dev, K_OBJ_DRIVER_FLASH, &flash_sim_api));

	return z_impl_flash_simulator_get_params(dev);
}

#include <zephyr/syscalls/flash_simulator_get_memory_mrsh.c>

#endif /* CONFIG_USERSPACE */

#define SOC_NV_FLASH_NODE(n) DT_INST_FOREACH_CHILD(n, IDENTITY)
#define FLASH_SIMULATOR_BASE_OFFSET(n) DT_REG_ADDR(SOC_NV_FLASH_NODE(n))
#define FLASH_SIMULATOR_ERASE_UNIT(n) DT_PROP(SOC_NV_FLASH_NODE(n), erase_block_size)
#define FLASH_SIMULATOR_PROG_UNIT(n) DT_PROP(SOC_NV_FLASH_NODE(n), write_block_size)
#define FLASH_SIMULATOR_FLASH_SIZE(n) DT_REG_SIZE(SOC_NV_FLASH_NODE(n))
#define FLASH_SIMULATOR_ERASE_VALUE(n) DT_PROP(DT_PARENT(SOC_NV_FLASH_NODE(n)), erase_value)
#define FLASH_SIMULATOR_PAGE_COUNT(n) (FLASH_SIMULATOR_FLASH_SIZE(n) / FLASH_SIMULATOR_ERASE_UNIT(n))

#define FLASH_SIMULATOR_INIT(n) 									\
	static uint8_t mock_flash_##n[FLASH_SIMULATOR_FLASH_SIZE(n)];					\
													\
	static uint8_t prog_unit_buf_##n[FLASH_SIMULATOR_PROG_UNIT(n)];					\
													\
	static const struct flash_parameters flash_sim_parameters_##n = {				\
		.write_block_size = FLASH_SIMULATOR_PROG_UNIT(n),					\
		.erase_value = FLASH_SIMULATOR_ERASE_VALUE(n),						\
		.caps = {										\
			IF_ENABLED(CONFIG_FLASH_SIMULATOR_EXPLICIT_ERASE,				\
				(.no_explicit_erase = false,))						\
		},											\
	}; 												\
													\
	IF_ENABLED(CONFIG_FLASH_PAGE_LAYOUT, (								\
		static const struct flash_pages_layout flash_sim_pages_layout_##n = {			\
			.pages_count = FLASH_SIMULATOR_PAGE_COUNT(n),					\
			.pages_size = FLASH_SIMULATOR_ERASE_UNIT(n),					\
		};											\
	))												\
													\
	static const struct flash_simulator_config flash_simulator_config_##n = {			\
		.mock_flash = mock_flash_##n,								\
		.prog_unit_buf = prog_unit_buf_##n,							\
		.flash_parameters = &flash_sim_parameters_##n, 						\
		.base_offset = FLASH_SIMULATOR_BASE_OFFSET(n), 						\
		.erase_unit =  FLASH_SIMULATOR_ERASE_UNIT(n), 						\
		.flash_size = FLASH_SIMULATOR_FLASH_SIZE(n), 						\
		IF_ENABLED(CONFIG_FLASH_PAGE_LAYOUT, (							\
			.pages_layout = &flash_sim_pages_layout_##n,)					\
		)											\
	};												\
													\
	BUILD_ASSERT((FLASH_SIMULATOR_ERASE_UNIT(n) % FLASH_SIMULATOR_PROG_UNIT(n)) == 0,		\
		     "Erase unit must be a multiple of program unit");					\
													\
	DEVICE_DT_INST_DEFINE(n, flash_init, NULL, NULL, &flash_simulator_config_##n,			\
		POST_KERNEL, CONFIG_FLASH_INIT_PRIORITY, &flash_sim_api);

DT_INST_FOREACH_STATUS_OKAY(FLASH_SIMULATOR_INIT);
