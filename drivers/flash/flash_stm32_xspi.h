/*
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_FLASH_XSPI_STM32_H_
#define ZEPHYR_DRIVERS_FLASH_XSPI_STM32_H_

#define STM32_XSPI_FIFO_THRESHOLD       4U

/* Valid range is [0, 255] */
#define STM32_XSPI_CLOCK_PRESCALER_MIN  0U
#define STM32_XSPI_CLOCK_PRESCALER_MAX  255U
#define STM32_XSPI_CLOCK_COMPUTE(bus_freq, prescaler) ((bus_freq) / ((prescaler) + 1U))

/* Max Time value during reset or erase operation */
#define STM32_XSPI_RESET_MAX_TIME               100U
#define STM32_XSPI_BULK_ERASE_MAX_TIME          460000U
#define STM32_XSPI_SECTOR_ERASE_MAX_TIME        1000U
#define STM32_XSPI_SUBSECTOR_4K_ERASE_MAX_TIME  400U
#define STM32_XSPI_WRITE_REG_MAX_TIME           40U

/* used as default value for DTS writeoc */
#define SPI_NOR_WRITEOC_NONE 0xFF

typedef void (*irq_config_func_t)(const struct device *dev);

struct flash_stm32_xspi_config {
	const struct stm32_pclken pclken; /* clock subsystem */
	const struct stm32_pclken pclken_ker; /* clock subsystem */
	const struct stm32_pclken pclken_mgr; /* clock subsystem */
	irq_config_func_t irq_config;
	size_t flash_size;
	uint32_t max_frequency;
	int data_mode; /* SPI or QSPI or OSPI */
	int data_rate; /* DTR or STR */
	const struct pinctrl_dev_config *pcfg;
#if STM32_XSPI_RESET_GPIO
	const struct gpio_dt_spec reset;
#endif /* STM32_XSPI_RESET_GPIO */
};

struct flash_stm32_xspi_data {
	/* XSPI handle is modifiable ; so part of data struct */
	XSPI_HandleTypeDef hxspi;
	struct k_sem sem;
	struct k_sem sync;
#if defined(CONFIG_FLASH_PAGE_LAYOUT)
	struct flash_pages_layout layout;
#endif
	struct jesd216_erase_type erase_types[JESD216_NUM_ERASE_TYPES];
	/* Number of bytes per page */
	uint16_t page_size;
	/* Address width in bytes */
	uint8_t address_width;
	/* Read operation dummy cycles */
	uint8_t read_dummy;
	uint32_t read_opcode;
	uint32_t write_opcode;
	enum jesd216_mode_type read_mode;
	enum jesd216_dw15_qer_type qer_type;
#if defined(CONFIG_FLASH_JESD216_API)
	/* Table to hold the jedec Read ID given by the octoFlash const */
	uint8_t jedec_id[JESD216_READ_ID_LEN];
#endif /* CONFIG_FLASH_JESD216_API */
	int cmd_status;
};

#endif /* ZEPHYR_DRIVERS_FLASH_XSPI_STM32_H_ */
