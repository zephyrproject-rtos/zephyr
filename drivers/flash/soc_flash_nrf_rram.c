/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_rram_controller

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <zephyr/sys/barrier.h>
#include <hal/nrf_rramc.h>

LOG_MODULE_REGISTER(flash_nrf_rram, CONFIG_FLASH_LOG_LEVEL);

#define RRAM DT_INST(0, soc_nv_flash)

#define RRAM_START DT_REG_ADDR(RRAM)
#define RRAM_SIZE  DT_REG_SIZE(RRAM)

#define RRAM_WORD_SIZE 4

#define PAGE_SIZE  DT_PROP(RRAM, erase_block_size)
#define PAGE_COUNT ((RRAM_SIZE) / (PAGE_SIZE))

#define ERASE_VALUE 0xFF

#if CONFIG_NRF_RRAM_WRITE_BUFFER_SIZE > 0
#define WRITE_BUFFER_ENABLE 1
#define WRITE_BUFFER_SIZE   CONFIG_NRF_RRAM_WRITE_BUFFER_SIZE
#define WRITE_LINE_SIZE     16 /* In bytes, one line is 128 bits. */
#else
#define WRITE_BUFFER_ENABLE 0
#define WRITE_BUFFER_SIZE   0
#define WRITE_LINE_SIZE     RRAM_WORD_SIZE
#endif

static inline bool is_within_bounds(off_t addr, size_t len, off_t boundary_start,
				    size_t boundary_size)
{
	return (addr >= boundary_start && (addr < (boundary_start + boundary_size)) &&
		(len <= (boundary_start + boundary_size - addr)));
}

#if WRITE_BUFFER_ENABLE
static void commit_changes(size_t len)
{
	if (nrf_rramc_empty_buffer_check(NRF_RRAMC)) {
		/* The internal write-buffer has been committed to RRAM and is now empty. */
		return;
	}

	if ((len % (WRITE_LINE_SIZE * WRITE_BUFFER_SIZE)) == 0) {
		/* Our last operation was buffer size-aligned, so we're done. */
		return;
	}

	nrf_rramc_task_trigger(NRF_RRAMC, NRF_RRAMC_TASK_COMMIT_WRITEBUF);

	barrier_dmem_fence_full();
}
#endif

static int rram_write(const struct device *dev, off_t addr, const void *data, size_t len)
{
	ARG_UNUSED(dev);

	if (!is_within_bounds(addr, len, 0, RRAM_SIZE)) {
		return -EINVAL;
	}
	addr += RRAM_START;

	LOG_DBG("Write: %p:%zu", (void *)addr, len);

	nrf_rramc_config_t config = {.mode_write = true, .write_buff_size = WRITE_BUFFER_SIZE};

	nrf_rramc_config_set(NRF_RRAMC, &config);

	if (data) {
		memcpy((void *)addr, data, len);
	} else {
		memset((void *)addr, ERASE_VALUE, len);
	}

	barrier_dmem_fence_full(); /* Barrier following our last write. */

#if WRITE_BUFFER_ENABLE
	commit_changes(len);
#endif

	config.mode_write = false;
	nrf_rramc_config_set(NRF_RRAMC, &config);

	return 0;
}

static int flash_nrf_rram_read(const struct device *dev, off_t addr, void *data, size_t len)
{
	ARG_UNUSED(dev);

	if (!is_within_bounds(addr, len, 0, RRAM_SIZE)) {
		return -EINVAL;
	}
	addr += RRAM_START;

	memcpy(data, (void *)addr, len);

	return 0;
}

static int flash_nrf_rram_write(const struct device *dev, off_t addr, const void *data, size_t len)
{
	return rram_write(dev, addr, data, len);
}

static int flash_nrf_rram_erase(const struct device *dev, off_t addr, size_t len)
{
	return rram_write(dev, addr, NULL, len);
}

static const struct flash_parameters *flash_nrf_rram_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	static const struct flash_parameters parameters = {
		.write_block_size = WRITE_LINE_SIZE,
		.erase_value = ERASE_VALUE,
	};

	return &parameters;
}

#if defined(CONFIG_FLASH_PAGE_LAYOUT)
static void flash_nrf_rram_page_layout(const struct device *dev,
				       const struct flash_pages_layout **layout,
				       size_t *layout_size)
{
	ARG_UNUSED(dev);

	static const struct flash_pages_layout pages_layout = {
		.pages_count = PAGE_COUNT,
		.pages_size = PAGE_SIZE,
	};

	*layout = &pages_layout;
	*layout_size = 1;
}
#endif

static const struct flash_driver_api flash_nrf_rram_api = {
	.read = flash_nrf_rram_read,
	.write = flash_nrf_rram_write,
	.erase = flash_nrf_rram_erase,
	.get_parameters = flash_nrf_rram_get_parameters,
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	.page_layout = flash_nrf_rram_page_layout,
#endif
};

static int flash_nrf_rram_init(const struct device *dev)
{
	ARG_UNUSED(dev);

#if CONFIG_NRF_RRAM_READYNEXT_TIMEOUT_VALUE > 0
	nrf_rramc_ready_next_timeout_t params = {
		.value = CONFIG_NRF_RRAM_READYNEXT_TIMEOUT_VALUE,
		.enable = true,
	};
	nrf_rramc_ready_next_timeout_set(NRF_RRAMC, &params);
#endif

	return 0;
}

DEVICE_DT_INST_DEFINE(0, flash_nrf_rram_init, NULL, NULL, NULL, POST_KERNEL,
		      CONFIG_FLASH_INIT_PRIORITY, &flash_nrf_rram_api);
