/*
 * Copyright (c) 2016 RnDity Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_FLASH_FLASH_STM32F3X_H_
#define ZEPHYR_DRIVERS_FLASH_FLASH_STM32F3X_H_

#include <soc.h>
#include <flash.h>
#include <clock_control.h>
#include <flash_registers.h>

struct flash_stm32_dev_config {
	u32_t *base;
	struct stm32_pclken pclken;
};

struct flash_stm32_dev_data {
	/* For future use. */
};

#define FLASH_CFG(dev)							\
	((const struct flash_stm32_dev_config * const)(dev)->config->config_info)
#define FLASH_DATA(dev)							\
	((struct flash_stm32_dev_data * const)(dev)->driver_data)
#define FLASH_STRUCT(base)						\
	(volatile struct stm32_flash *)(base)

/* Flash programming timeout definition. */
#define FLASH_ER_PRG_TIMEOUT	((u32_t)0x000B0000)

enum flash_status {
	FLASH_BUSY = 1,
	FLASH_ERROR_WRITE_PROTECTION,
	FLASH_ERROR_PROGRAM,
	FLASH_COMPLETE,
	FLASH_TIMEOUT
};

void flash_stm32_lock(struct device *flash);

void flash_stm32_unlock(struct device *flash);

u8_t flash_stm32_program_halfword(struct device *flash,
				     u32_t address,
				     u16_t data);

u8_t flash_stm32_program_word(struct device *flash,
				 u32_t address,
				 u32_t data);

void flash_stm32_read_data(void *data, u32_t address, size_t len);

u8_t flash_stm32_wait_for_last_operation(struct device *flash,
					    u32_t timeout);

u8_t flash_stm32_get_status(struct device *flash);

u8_t flash_stm32_erase_page(struct device *flash,
			       u32_t page_address);

u8_t flash_stm32_erase_all_pages(struct device *flash);

#endif /* ZEPHYR_DRIVERS_FLASH_FLASH_STM32F3X_H_ */
