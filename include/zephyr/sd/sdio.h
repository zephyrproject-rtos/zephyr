/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for SDIO subsystem
 */

#ifndef ZEPHYR_INCLUDE_SD_SDIO_H_
#define ZEPHYR_INCLUDE_SD_SDIO_H_

#include <zephyr/device.h>
#include <zephyr/drivers/sdhc.h>
#include <zephyr/sd/sd.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize SDIO function.
 *
 * Initializes SDIO card function. The card function will not be enabled,
 * but after this call returns the SDIO function structure can be used to read
 * and write data from the card.
 * @param func: function structure to initialize
 * @param card: SD card to enable function on
 * @param num: function number to initialize
 * @retval 0 function was initialized successfully
 * @retval -EIO: I/O error
 */
int sdio_init_func(struct sd_card *card, struct sdio_func *func,
		   enum sdio_func_num num);

/**
 * @brief Enable SDIO function
 *
 * Enables SDIO card function. @ref sdio_init_func must be called to
 * initialized the function structure before enabling it in the card.
 * @param func: function to enable
 * @retval 0 function was enabled successfully
 * @retval -ETIMEDOUT: card I/O timed out
 * @retval -EIO: I/O error
 */
int sdio_enable_func(struct sdio_func *func);

/**
 * @brief Set block size of SDIO function
 *
 * Set desired block size for SDIO function, used by block transfers
 * to SDIO registers.
 * @param func: function to set block size for
 * @param bsize: block size
 * @retval 0 block size was set
 * @retval -EINVAL: unsupported/invalid block size
 * @retval -EIO: I/O error
 */
int sdio_set_block_size(struct sdio_func *func, uint16_t bsize);

/**
 * @brief Read byte from SDIO register
 *
 * Reads byte from SDIO register
 * @param func: function to read from
 * @param reg: register address to read from
 * @param val: filled with byte value read from register
 * @retval 0 read succeeded
 * @retval -EBUSY: card is busy with another request
 * @retval -ETIMEDOUT: card read timed out
 * @retval -EIO: I/O error
 */
int sdio_read_byte(struct sdio_func *func, uint32_t reg, uint8_t *val);

/**
 * @brief Write byte to SDIO register
 *
 * Writes byte to SDIO register
 * @param func: function to write to
 * @param reg: register address to write to
 * @param write_val: value to write to register
 * @retval 0 write succeeded
 * @retval -EBUSY: card is busy with another request
 * @retval -ETIMEDOUT: card write timed out
 * @retval -EIO: I/O error
 */
int sdio_write_byte(struct sdio_func *func, uint32_t reg, uint8_t write_val);

/**
 * @brief Write byte to SDIO register, and read result
 *
 * Writes byte to SDIO register, and reads the register after write
 * @param func: function to write to
 * @param reg: register address to write to
 * @param write_val: value to write to register
 * @param read_val: filled with value read from register
 * @retval 0 write succeeded
 * @retval -EBUSY: card is busy with another request
 * @retval -ETIMEDOUT: card write timed out
 * @retval -EIO: I/O error
 */
int sdio_rw_byte(struct sdio_func *func, uint32_t reg, uint8_t write_val,
		 uint8_t *read_val);

/**
 * @brief Read bytes from SDIO fifo
 *
 * Reads bytes from SDIO register, treating it as a fifo. Reads will
 * all be done from same address.
 * @param func: function to read from
 * @param reg: register address of fifo
 * @param data: filled with data read from fifo
 * @param len: length of data to read from card
 * @retval 0 read succeeded
 * @retval -EBUSY: card is busy with another request
 * @retval -ETIMEDOUT: card read timed out
 * @retval -EIO: I/O error
 */
int sdio_read_fifo(struct sdio_func *func, uint32_t reg, uint8_t *data,
		   uint32_t len);

/**
 * @brief Write bytes to SDIO fifo
 *
 * Writes bytes to SDIO register, treating it as a fifo. Writes will
 * all be done to same address.
 * @param func: function to write to
 * @param reg: register address of fifo
 * @param data: data to write to fifo
 * @param len: length of data to write to card
 * @retval 0 write succeeded
 * @retval -EBUSY: card is busy with another request
 * @retval -ETIMEDOUT: card write timed out
 * @retval -EIO: I/O error
 */
int sdio_write_fifo(struct sdio_func *func, uint32_t reg, uint8_t *data,
		    uint32_t len);

/**
 * @brief Read blocks from SDIO fifo
 *
 * Reads blocks from SDIO register, treating it as a fifo. Reads will
 * all be done from same address.
 * @param func: function to read from
 * @param reg: register address of fifo
 * @param data: filled with data read from fifo
 * @param blocks: number of blocks to read from fifo
 * @retval 0 read succeeded
 * @retval -EBUSY: card is busy with another request
 * @retval -ETIMEDOUT: card read timed out
 * @retval -EIO: I/O error
 */
int sdio_read_blocks_fifo(struct sdio_func *func, uint32_t reg, uint8_t *data,
			  uint32_t blocks);

/**
 * @brief Write blocks to SDIO fifo
 *
 * Writes blocks from SDIO register, treating it as a fifo. Writes will
 * all be done to same address.
 * @param func: function to write to
 * @param reg: register address of fifo
 * @param data: data to write to fifo
 * @param blocks: number of blocks to write to fifo
 * @retval 0 write succeeded
 * @retval -EBUSY: card is busy with another request
 * @retval -ETIMEDOUT: card write timed out
 * @retval -EIO: I/O error
 */
int sdio_write_blocks_fifo(struct sdio_func *func, uint32_t reg, uint8_t *data,
			   uint32_t blocks);

/**
 * @brief Copy bytes from an SDIO card
 *
 * Copies bytes from an SDIO card, starting from provided address.
 * @param func: function to read from
 * @param reg: register address to start copy at
 * @param data: buffer to copy data into
 * @param len: length of data to read
 * @retval 0 read succeeded
 * @retval -EBUSY: card is busy with another request
 * @retval -ETIMEDOUT: card read timed out
 * @retval -EIO: I/O error
 */
int sdio_read_addr(struct sdio_func *func, uint32_t reg, uint8_t *data,
		   uint32_t len);

/**
 * @brief Copy bytes to an SDIO card
 *
 * Copies bytes to an SDIO card, starting from provided address.
 *
 * @param func: function to write to
 * @param reg: register address to start copy at
 * @param data: buffer to copy data from
 * @param len: length of data to write
 * @retval 0 write succeeded
 * @retval -EBUSY: card is busy with another request
 * @retval -ETIMEDOUT: card write timed out
 * @retval -EIO: I/O error
 */
int sdio_write_addr(struct sdio_func *func, uint32_t reg, uint8_t *data,
		    uint32_t len);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SD_SDMMC_H_ */
