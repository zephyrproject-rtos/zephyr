/*
 * Copyright (c) 2016 RnDity Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <misc/__assert.h>
#include <clock_control/stm32_clock_control.h>

#include "flash_stm32f3x.h"

static int flash_stm32_erase(struct device *dev, off_t offset, size_t size)
{
	u32_t first_page_addr = 0;
	u32_t last_page_addr = 0;
	u16_t no_of_pages = size / CONFIG_FLASH_PAGE_SIZE;
	u16_t page_index = 0;

	/* Check offset and size alignment. */
	if (((offset % CONFIG_FLASH_PAGE_SIZE) != 0) ||
	    ((size % CONFIG_FLASH_PAGE_SIZE) != 0) ||
	    (no_of_pages == 0)) {
		return -EINVAL;
	}

	/* Find address of the first page to be erased. */
	page_index = offset / CONFIG_FLASH_PAGE_SIZE;

	first_page_addr = CONFIG_FLASH_BASE_ADDRESS +
			  page_index * CONFIG_FLASH_PAGE_SIZE;

	__ASSERT_NO_MSG(IS_FLASH_PROGRAM_ADDRESS(first_page_addr));

	/* Find address of the last page to be erased. */
	page_index = ((offset + size) / CONFIG_FLASH_PAGE_SIZE) - 1;

	last_page_addr = CONFIG_FLASH_BASE_ADDRESS +
			 page_index * CONFIG_FLASH_PAGE_SIZE;

	__ASSERT_NO_MSG(IS_FLASH_PROGRAM_ADDRESS(last_page_addr));

	while (no_of_pages) {
		if (flash_stm32_erase_page(dev, first_page_addr)
				!= FLASH_COMPLETE) {
			return -EINVAL;
		}
		no_of_pages--;
		first_page_addr += CONFIG_FLASH_PAGE_SIZE;
	}

	return 0;
}

static int flash_stm32_read(struct device *dev, off_t offset,
			    void *data, size_t len)
{
	u32_t address = CONFIG_FLASH_BASE_ADDRESS + offset;

	__ASSERT_NO_MSG(IS_FLASH_PROGRAM_ADDRESS(address));

	flash_stm32_read_data(data, address, len);

	return 0;
}

static int flash_stm32_write(struct device *dev, off_t offset,
			     const void *data, size_t len)
{
	u16_t halfword = 0;

	u32_t address =
		CONFIG_FLASH_BASE_ADDRESS + offset;

	u8_t remainder = 0;

	if ((len % 2) != 0) {
		remainder = 1;
	}

	len = len / 2;

	while (len--) {
		halfword = *((u8_t *)data++);
		halfword |= *((u8_t *)data++) << 8;
		if (flash_stm32_program_halfword(dev, address, halfword)
				!= FLASH_COMPLETE) {
			return -EINVAL;
		}
		address += 2;
	}

	if (remainder) {
		halfword = (*((u16_t *)data)) & 0x00FF;
		if (flash_stm32_program_halfword(dev, address, halfword)
				!= FLASH_COMPLETE) {
			return -EINVAL;
		}
	}

	return 0;
}

static int flash_stm32_protection_set(struct device *dev, bool enable)
{
	if (enable) {
		flash_stm32_lock(dev);
	} else {
		flash_stm32_unlock(dev);
	}

	return 0;
}

static int flash_stm32_init(struct device *dev)
{
	const struct flash_stm32_dev_config *cfg = FLASH_CFG(dev);

	struct device *clk = device_get_binding(STM32_CLOCK_CONTROL_NAME);

	if (clock_control_on(clk, (clock_control_subsys_t *) &cfg->pclken) != 0)
		return -ENODEV;

	return 0;
}

static const struct flash_driver_api flash_stm32_api = {
	.read = flash_stm32_read,
	.write = flash_stm32_write,
	.erase = flash_stm32_erase,
	.write_protection = flash_stm32_protection_set,
	.write_block_size = 2,
};

static const struct flash_stm32_dev_config flash_device_config = {
	.base = (u32_t *)FLASH_R_BASE,
	.pclken = { .bus = STM32_CLOCK_BUS_APB1,
		    .enr =  LL_AHB1_GRP1_PERIPH_FLASH},
};

static struct flash_stm32_dev_data flash_device_data = {

};

DEVICE_AND_API_INIT(flash_stm32, CONFIG_SOC_FLASH_STM32_DEV_NAME,
		    flash_stm32_init,
		    &flash_device_data,
		    &flash_device_config,
		    POST_KERNEL,
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    &flash_stm32_api);
