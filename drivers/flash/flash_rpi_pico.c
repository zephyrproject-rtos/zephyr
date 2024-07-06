/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 * Copyright (c) 2022 Yonatan Schachter
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>
#include <stddef.h>

#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <zephyr/toolchain.h>

#include <hardware/flash.h>
#include <hardware/regs/io_qspi.h>
#include <hardware/regs/pads_qspi.h>
#include <hardware/structs/ssi.h>
#include <hardware/structs/xip_ctrl.h>
#include <hardware/resets.h>
#include <pico/bootrom.h>

LOG_MODULE_REGISTER(flash_rpi_pico, CONFIG_FLASH_LOG_LEVEL);

#define DT_DRV_COMPAT raspberrypi_pico_flash_controller

#define PAGE_SIZE 256
#define SECTOR_SIZE DT_PROP(DT_CHOSEN(zephyr_flash), erase_block_size)
#define ERASE_VALUE 0xff
#define FLASH_SIZE KB(CONFIG_FLASH_SIZE)
#define FLASH_BASE CONFIG_FLASH_BASE_ADDRESS
#define SSI_BASE_ADDRESS DT_REG_ADDR(DT_CHOSEN(zephyr_flash_controller))

static const struct flash_parameters flash_rpi_parameters = {
	.write_block_size = 1,
	.erase_value = ERASE_VALUE,
};

/**
 * Low level flash functions are based on:
 * github.com/raspberrypi/pico-bootrom/blob/master/bootrom/program_flash_generic.c
 * and
 * github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_flash/flash.c
 */

#define FLASHCMD_PAGE_PROGRAM 0x02
#define FLASHCMD_READ_STATUS 0x05
#define FLASHCMD_WRITE_ENABLE 0x06
#define BOOT2_SIZE_WORDS 64

enum outover {
	OUTOVER_NORMAL = 0,
	OUTOVER_INVERT,
	OUTOVER_LOW,
	OUTOVER_HIGH
};

static ssi_hw_t *const ssi = (ssi_hw_t *)SSI_BASE_ADDRESS;
static uint32_t boot2_copyout[BOOT2_SIZE_WORDS];
static bool boot2_copyout_valid;
static uint8_t flash_ram_buffer[PAGE_SIZE];

static void __no_inline_not_in_flash_func(flash_init_boot2_copyout)(void)
{
	if (boot2_copyout_valid)
		return;
	for (int i = 0; i < BOOT2_SIZE_WORDS; ++i)
		boot2_copyout[i] = ((uint32_t *)FLASH_BASE)[i];
	__compiler_memory_barrier();
	boot2_copyout_valid = true;
}

static void __no_inline_not_in_flash_func(flash_enable_xip_via_boot2)(void)
{
	((void (*)(void))((uint32_t)boot2_copyout+1))();
}

void __no_inline_not_in_flash_func(flash_cs_force)(enum outover over)
{
	io_rw_32 *reg = (io_rw_32 *) (IO_QSPI_BASE + IO_QSPI_GPIO_QSPI_SS_CTRL_OFFSET);
	*reg = (*reg & ~IO_QSPI_GPIO_QSPI_SS_CTRL_OUTOVER_BITS)
		| (over << IO_QSPI_GPIO_QSPI_SS_CTRL_OUTOVER_LSB);
	(void) *reg;
}

int __no_inline_not_in_flash_func(flash_was_aborted)()
{
	return *(io_rw_32 *) (IO_QSPI_BASE + IO_QSPI_GPIO_QSPI_SD1_CTRL_OFFSET)
		   & IO_QSPI_GPIO_QSPI_SD1_CTRL_INOVER_BITS;
}

void __no_inline_not_in_flash_func(flash_put_get)(const uint8_t *tx, uint8_t *rx, size_t count,
						size_t rx_skip)
{
	const uint max_in_flight = 16 - 2;
	size_t tx_count = count;
	size_t rx_count = count;
	bool did_something;
	uint32_t tx_level;
	uint32_t rx_level;
	uint8_t rxbyte;

	while (tx_count || rx_skip || rx_count) {
		tx_level = ssi_hw->txflr;
		rx_level = ssi_hw->rxflr;
		did_something = false;
		if (tx_count && tx_level + rx_level < max_in_flight) {
			ssi->dr0 = (uint32_t) (tx ? *tx++ : 0);
			--tx_count;
			did_something = true;
		}
		if (rx_level) {
			rxbyte = ssi->dr0;
			did_something = true;
			if (rx_skip) {
				--rx_skip;
			} else {
				if (rx)
					*rx++ = rxbyte;
				--rx_count;
			}
		}

		if (!did_something && __builtin_expect(flash_was_aborted(), 0))
			break;
	}
	flash_cs_force(OUTOVER_HIGH);
}

void __no_inline_not_in_flash_func(flash_put_get_wrapper)(uint8_t cmd, const uint8_t *tx,
					uint8_t *rx, size_t count)
{
	flash_cs_force(OUTOVER_LOW);
	ssi->dr0 = cmd;
	flash_put_get(tx, rx, count, 1);
}

static ALWAYS_INLINE void flash_put_cmd_addr(uint8_t cmd, uint32_t addr)
{
	flash_cs_force(OUTOVER_LOW);
	addr |= cmd << 24;
	for (int i = 0; i < 4; ++i) {
		ssi->dr0 = addr >> 24;
		addr <<= 8;
	}
}

void __no_inline_not_in_flash_func(flash_write_partial_internal)(uint32_t addr, const uint8_t *data,
					size_t size)
{
	uint8_t status_reg;

	flash_put_get_wrapper(FLASHCMD_WRITE_ENABLE, NULL, NULL, 0);
	flash_put_cmd_addr(FLASHCMD_PAGE_PROGRAM, addr);
	flash_put_get(data, NULL, size, 4);

	do {
		flash_put_get_wrapper(FLASHCMD_READ_STATUS, NULL, &status_reg, 1);
	} while (status_reg & 0x1 && !flash_was_aborted());
}

void __no_inline_not_in_flash_func(flash_write_partial)(uint32_t flash_offs, const uint8_t *data,
							size_t count)
{
	rom_connect_internal_flash_fn connect_internal_flash = (rom_connect_internal_flash_fn)
					rom_func_lookup_inline(ROM_FUNC_CONNECT_INTERNAL_FLASH);
	rom_flash_exit_xip_fn flash_exit_xip = (rom_flash_exit_xip_fn)
						rom_func_lookup_inline(ROM_FUNC_FLASH_EXIT_XIP);
	rom_flash_flush_cache_fn flash_flush_cache = (rom_flash_flush_cache_fn)
						rom_func_lookup_inline(ROM_FUNC_FLASH_FLUSH_CACHE);

	flash_init_boot2_copyout();

	__compiler_memory_barrier();

	connect_internal_flash();
	flash_exit_xip();
	flash_write_partial_internal(flash_offs, data, count);
	flash_flush_cache();
	flash_enable_xip_via_boot2();
}

static bool is_valid_range(k_off_t offset, uint32_t size)
{
	return (offset >= 0) && ((offset + size) <= FLASH_SIZE);
}

static int flash_rpi_read(const struct device *dev, k_off_t offset, void *data, size_t size)
{
	if (size == 0) {
		return 0;
	}

	if (!is_valid_range(offset, size)) {
		LOG_ERR("Read range exceeds the flash boundaries");
		return -EINVAL;
	}

	memcpy(data, (uint8_t *)(CONFIG_FLASH_BASE_ADDRESS + offset), size);

	return 0;
}

static int flash_rpi_write(const struct device *dev, k_off_t offset, const void *data, size_t size)
{
	uint32_t key;
	size_t bytes_to_write;
	uint8_t *data_pointer = (uint8_t *)data;

	if (size == 0) {
		return 0;
	}

	if (!is_valid_range(offset, size)) {
		LOG_ERR("Write range exceeds the flash boundaries. Offset=%#lx, Size=%u",
				offset, size);
		return -EINVAL;
	}

	key = irq_lock();

	if ((offset & (PAGE_SIZE - 1)) > 0) {
		bytes_to_write = MIN(PAGE_SIZE - (offset & (PAGE_SIZE - 1)), size);
		memcpy(flash_ram_buffer, data_pointer, bytes_to_write);
		flash_write_partial(offset, flash_ram_buffer, bytes_to_write);
		data_pointer += bytes_to_write;
		size -= bytes_to_write;
		offset += bytes_to_write;
	}

	while (size >= PAGE_SIZE) {
		bytes_to_write = PAGE_SIZE;
		memcpy(flash_ram_buffer, data_pointer, bytes_to_write);
		flash_range_program(offset, flash_ram_buffer, bytes_to_write);
		data_pointer += bytes_to_write;
		size -= bytes_to_write;
		offset += bytes_to_write;
	}

	if (size > 0) {
		memcpy(flash_ram_buffer, data_pointer, size);
		flash_write_partial(offset, flash_ram_buffer, size);
	}

	irq_unlock(key);

	return 0;
}

static int flash_rpi_erase(const struct device *dev, k_off_t offset, size_t size)
{
	uint32_t key;

	if (size == 0) {
		return 0;
	}

	if (!is_valid_range(offset, size)) {
		LOG_ERR("Erase range exceeds the flash boundaries. Offset=%#lx, Size=%u",
				offset, size);
		return -EINVAL;
	}

	if ((offset % SECTOR_SIZE) || (size % SECTOR_SIZE)) {
		LOG_ERR("Erase range is not a multiple of the sector size. Offset=%#lx, Size=%u",
				offset, size);
		return -EINVAL;
	}

	key = irq_lock();

	flash_range_erase(offset, size);

	irq_unlock(key);

	return 0;
}

static const struct flash_parameters *flash_rpi_get_parameters(const struct device *dev)
{
	ARG_UNUSED(dev);

	return &flash_rpi_parameters;
}

#if CONFIG_FLASH_PAGE_LAYOUT

static const struct flash_pages_layout flash_rpi_pages_layout = {
	.pages_count = FLASH_SIZE / SECTOR_SIZE,
	.pages_size = SECTOR_SIZE,
};

void flash_rpi_page_layout(const struct device *dev, const struct flash_pages_layout **layout,
				size_t *layout_size)
{
	*layout = &flash_rpi_pages_layout;
	*layout_size = 1;
}

#endif /* CONFIG_FLASH_PAGE_LAYOUT */

static const struct flash_driver_api flash_rpi_driver_api = {
	.read = flash_rpi_read,
	.write = flash_rpi_write,
	.erase = flash_rpi_erase,
	.get_parameters = flash_rpi_get_parameters,
#ifdef CONFIG_FLASH_PAGE_LAYOUT
	.page_layout = flash_rpi_page_layout,
#endif /* CONFIG_FLASH_PAGE_LAYOUT */
};

DEVICE_DT_INST_DEFINE(0, NULL, NULL, NULL, NULL, POST_KERNEL,
		      CONFIG_FLASH_INIT_PRIORITY, &flash_rpi_driver_api);
