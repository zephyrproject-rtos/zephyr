/*
 * Copyright (c) 2020 Hubert Miś
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief FT8XX common functions
 */

#ifndef ZEPHYR_DRIVERS_MISC_FT8XX_FT8XX_COMMON_H_
#define ZEPHYR_DRIVERS_MISC_FT8XX_FT8XX_COMMON_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief FT8xx functions to write and read memory
 * @defgroup ft8xx_common FT8xx common functions
 * @ingroup ft8xx_interface
 * @{
 */

/**
 * @brief Write 1 byte (8 bits) to FT8xx memory
 *
 * @param bus bus which to interact with
 *
 * @param address Memory address to write to
 * @param data Byte to write
 */
void ft8xx_wr8(union ft8xx_bus bus, uint32_t address, uint8_t data);

/**
 * @brief Write 2 bytes (16 bits) to FT8xx memory
 *
 * @param bus bus which to interact with
 *
 * @param address Memory address to write to
 * @param data Value to write
 */
void ft8xx_wr16(union ft8xx_bus bus, uint32_t address, uint16_t data);

/**
 * @brief Write 4 bytes (32 bits) to FT8xx memory
 *
 * @param bus bus which to interact with
 *
 * @param address Memory address to write to
 * @param data Value to write
 */
void ft8xx_wr32(union ft8xx_bus bus, uint32_t address, uint32_t data);

/**
 * @brief Write Block of bytes to FT8xx memory
 *
 * @param bus bus which to interact with
 *
 * @param address Memory address to write to
 * @param data data to write
 * @param data_length length of data
 */
void ft8xx_wrblock(union ft8xx_bus bus, uint32_t address, uint8_t *data,
                uint32_t data_length);

/**
 * @brief Write two sequential Block of bytes to FT8xx memory
 *
 * @param bus bus which to interact with
 * @param data data to write
 * @param data_length length of data
 * @param data2 data to write
 * @param data2_length length of data
 * @param padsize size of empty pad at endf of data 
 */
void ft8xx_wrblock_dual(union ft8xx_bus bus, uint32_t address, uint8_t *data,
                uint32_t data_length, 
                uint8_t *data2 
                uint32_t data2_length
                uint8_t padsize)


/**
 * @brief append a int16 to a data block for use with ft8xx_wrblock
 *
 * @param block pointer to data block
 * @param size size of block in bytes
 * @param offset offset in block to append data
 * @param value int16_t value to append data
 * @return new offset value
 */
int ft8xx_append_block_int16(uint8_t * block, uint32_t size, uint32_t offset,
                int16_t value);


/**
 * @brief append a uint16 to a data block for use with ft8xx_wrblock
 *
 * @param block pointer to data block
 * @param size size of block in bytes
 * @param offset offset in block to append data
 * @param value uint16_t value to append data
 * @return new offset value
 */
int ft8xx_append_block_uint16(uint8_t * block, uint32_t size, uint32_t offset,
                uint16_t value);

/**
 * @brief append a int32 to a data block for use with ft8xx_wrblock
 *
 * @param block pointer to data block
 * @param size size of block in bytes
 * @param offset offset in block to append data
 * @param value int32_t value to append 
 * @return new offset value
 */
int ft8xx_append_block_uint16(uint8_t * block, uint32_t size, uint32_t offset,
                int32_t value);

/**
 * @brief append a uint32 to a data block for use with ft8xx_wrblock
 *
 * @param block pointer to data block
 * @param size size of block in bytes
 * @param offset offset in block to append data
 * @param value uint32_t value to append 
 * @return new offset value
 */
int ft8xx_append_block_uint16(uint8_t * block, uint32_t size, uint32_t offset,
                uint32_t value);

/**
 * @brief append a data array to a data array for use with ft8xx_wrblock
 *
 * @param block pointer to data block
 * @param size size of block in bytes
 * @param offset offset in array to append data
 * @param data data array to append to block
 * @param data_size size of array to append
  * @return new offset value
 */
int ft8xx_append_block_data(uint8_t * block, uint32_t size, uint32_t offset,
                uint8_t *data, uint32_t data_size);



/**
 * @brief Read 1 byte (8 bits) from FT8xx memory
 *
 * @param bus bus which to interact with
 *
 * @param address Memory address to read from
 *
 * @return Value read from memory
 */
uint8_t ft8xx_rd8(union ft8xx_bus bus, uint32_t address);

/**
 * @brief Read 2 bytes (16 bits) from FT8xx memory
 *
 * @param bus bus which to interact with
 *
 * @param address Memory address to read from
 *
 * @return Value read from memory
 */
uint16_t ft8xx_rd16(union ft8xx_bus bus, uint32_t address);

/**
 * @brief Read 4 bytes (32 bits) from FT8xx memory
 *
 * @param bus bus which to interact with
 *
 * @param address Memory address to read from
 *
 * @return Value read from memory
 */
uint32_t ft8xx_rd32(union ft8xx_bus bus, uint32_t address);

/**
 * @brief Read Block of bytes from FT8xx memory
 *
 * @param bus bus which to interact with
 *
 * @param address Memory address to read from 
 * @param data pointer data buffer for read data
 * @param data_length length of data
 */
void ft8xx_rdblock(union ft8xx_bus bus, uint32_t address, uint8_t *data,
                uint32_t data_length);



/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_DRIVERS_MISC_FT8XX_FT8XX_COMMON_H_ */
