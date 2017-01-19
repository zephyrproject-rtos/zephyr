/*
 * Copyright (c) 2016 RnDity Sp. z o.o.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef DRIVERS_FLASH_FLASH_STM32_H_
#define DRIVERS_FLASH_FLASH_STM32_H_

#include <soc.h>
#include <flash.h>
#include <clock_control.h>
#include <flash_registers.h>

struct flash_stm32_dev_config {
	uint32_t *base;
	clock_control_subsys_t clock_subsys;
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
#define FLASH_ER_PRG_TIMEOUT	((uint32_t)0x000B0000)

enum flash_status {
	FLASH_BUSY = 1,
	FLASH_ERROR_WRITE_PROTECTION,
	FLASH_ERROR_PROGRAM,
	FLASH_COMPLETE,
	FLASH_TIMEOUT
};

void flash_stm32_lock(struct device *flash);

void flash_stm32_unlock(struct device *flash);

uint8_t flash_stm32_program_halfword(struct device *flash,
				     uint32_t address,
				     uint16_t data);

uint8_t flash_stm32_program_word(struct device *flash,
				 uint32_t address,
				 uint32_t data);

void flash_stm32_read_data(void *data, uint32_t address, size_t len);

uint8_t flash_stm32_wait_for_last_operation(struct device *flash,
					    uint32_t timeout);

uint8_t flash_stm32_get_status(struct device *flash);

uint8_t flash_stm32_erase_page(struct device *flash,
			       uint32_t page_address);

uint8_t flash_stm32_erase_all_pages(struct device *flash);

#endif /* DRIVERS_FLASH_FLASH_STM32_H_ */
