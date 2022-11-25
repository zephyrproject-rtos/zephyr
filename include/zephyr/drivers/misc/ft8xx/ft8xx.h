/*
 * Copyright (c) 2020 Hubert Miś
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief FT8XX public API
 */

#ifndef ZEPHYR_DRIVERS_MISC_FT8XX_FT8XX_H_
#define ZEPHYR_DRIVERS_MISC_FT8XX_FT8XX_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum ft8xx_chip_types {
	FT8xx_CHIP_ID_FT800 = 0x08000100,
	FT8xx_CHIP_ID_FT810 = 0x08100100,
	FT8xx_CHIP_ID_FT811 = 0x08110100,
	FT8xx_CHIP_ID_FT812 = 0x08120100,
	FT8xx_CHIP_ID_FT813 = 0x08130100,

	FT8xx_CHIP_ID_BT815 = 0x08150100,
	FT8xx_CHIP_ID_BT816 = 0x08160100,

	FT8xx_CHIP_ID_BT817 = 0x08170100,
	FT8xx_CHIP_ID_BT818 = 0x08180100,
}

struct ft8xx_config {
	uint16_t vsize;
	uint16_t voffset;
	uint16_t vcycle;
	uint16_t vsync0;
	uint16_t vsync1;
	uint16_t hsize;
	uint16_t hoffset;
	uint16_t hcycle;
	uint16_t hsync0;
	uint16_t hsync1;
	uint8_t pclk;
	uint8_t pclk_pol :1;
	uint8_t cspread  :1;
	uint8_t swizzle  :4;
	
	uint32_t chip_id;
	uint32_t chip_type;


	union ft8xx_bus bus;
	const struct ft8xx_bus_io *bus_io;

	struct gpio_dt_spec irq_gpio;
};

struct ft8xx_data_t {
//	const struct ft8xx_config *config;
	ft8xx_int_callback irq_callback;
	uint chip_id;
	uint chip_type;
	struct ft8xx_memory_map_t *memory_map;
	struct ft8xx_register_address_map_t *register_map;

	struct ft8xx_touch_transform *ctrform;

	static uint16_t reg_cmd_read;
	static uint16_t reg_cmd_write;

	static uint32_t flash_size;
	static uint16_t flash_status;

};




#define FT8XX_BUS_SPI DT_ANY_INST_ON_BUS_STATUS_OKAY(spi)

union ft8xx_bus {
	struct spi_dt_spec spi;
};

typedef int (*ft8xx_bus_check_fn)(const union ft8xx_bus *bus);
typedef int (*ft8xx_reg_read_fn)(const union ft8xx_bus *bus,
				  uint32_t start, uint8_t *buf, int size);
typedef int (*ft8xx_reg_write_fn)(const union ft8xx_bus *bus,
				   uint32_t reg, uint8_t val);



struct ft8xx_bus_io {
	ft8xx_bus_check_fn check;
	ft8xx_reg_read_fn read;
	ft8xx_reg_write_fn write;
};

#define FT8XX_SPI_OPERATION (SPI_WORD_SET(8) | SPI_OP_MODE_MASTER)

#ifdef CONFIG_SPI_EXTENDED_MODES

#endif


extern const struct ft8xx_bus_io ft8xx_bus_io_spi;


/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_MISC_FT8XX_FT8XX_H_ */
