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
	FT8xx_CHIP_ID_FT800 = 0x0800100,
	FT8xx_CHIP_ID_FT810 = 0x0810100,
	FT8xx_CHIP_ID_FT811 = 0x0811100,
	FT8xx_CHIP_ID_FT812 = 0x0812100,
	FT8xx_CHIP_ID_FT813 = 0x0813100,
}

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
