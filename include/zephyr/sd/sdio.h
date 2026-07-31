/*
 * Copyright 2023 NXP
 * Copyright 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public API for the SDIO host side
 *
 * SDIO function I/O over an SD host controller (@ref sdhc_interface). The I/O
 * is driven through a @ref sdio_dev endpoint, so it can be used either from the
 * SD-card stack (@ref sdio_init_func) or standalone (@ref sdio_dev_init /
 * @ref sdio_func_bind).
 */

#ifndef ZEPHYR_INCLUDE_SD_SDIO_H_
#define ZEPHYR_INCLUDE_SD_SDIO_H_

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sdhc.h>
#include <zephyr/sd/sd_spec.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declaration: the SD-card stack path (see sdio_init_func()). */
struct sd_card;

/**
 * @brief SDIO bus capability flags
 */
enum sdio_caps {
	SDIO_CAP_HS = BIT(0), /*!< High speed timing supported */
	SDIO_CAP_MULTIBLOCK = BIT(1), /*!< Multi-block (CMD53) transfers supported */
	SDIO_CAP_4BIT_BUS = BIT(2), /*!< 4-bit bus width supported */
	SDIO_CAP_SPI = BIT(3), /*!< SPI command/response framing in use */
};

/**
 * @brief SDIO host endpoint
 *
 * Ties SDIO function I/O to an SD host controller. Populated by
 * @ref sdio_dev_init (standalone) or by the SD-card stack.
 */
struct sdio_dev {
	const struct device *sdhc; /*!< SD host controller for I/O */
	uint32_t caps; /*!< Bus capability flags, see @ref sdio_caps */
	struct k_mutex lock; /*!< Serializes access to the bus */
	uint16_t max_blk_size; /*!< Max byte-mode transfer length (func0 CIS) */
};

/**
 * @brief SDIO function definition
 *
 * Stores per-function information. Refers to the owning @ref sdio_dev;
 * @ref card is set only when the function was set up from the SD-card stack.
 */
struct sdio_func {
	enum sdio_func_num num; /*!< Function number */
	struct sdio_dev *dev; /*!< Host endpoint this function is reached through */
	struct sd_card *card; /*!< SD card (SD-card stack path only, else NULL) */
	struct sdio_cis cis; /*!< CIS tuple data for this function */
	uint16_t block_size; /*!< Current block size for this function */
};

/**
 * @brief Initialize an SDIO host endpoint over an SD host controller.
 *
 * Standalone entry point (no SD-card stack): binds @p dev to @p sdhc so it can
 * be used with @ref sdio_func_bind and the function I/O API.
 *
 * @param dev  endpoint to initialize
 * @param sdhc SD host controller
 * @param caps bus capability flags (see @ref sdio_caps)
 * @retval 0 on success
 * @retval -EINVAL invalid argument
 */
int sdio_dev_init(struct sdio_dev *dev, const struct device *sdhc,
		  uint32_t caps);

/**
 * @brief Bind an SDIO function to a host endpoint.
 *
 * Standalone entry point: associates @p func with @p dev and function number
 * @p num. Does not read the function CIS (use the SD-card path for that).
 *
 * @param dev  host endpoint
 * @param func function structure to bind
 * @param num  function number
 * @retval 0 on success
 * @retval -EINVAL invalid argument
 */
int sdio_func_bind(struct sdio_dev *dev, struct sdio_func *func,
		   enum sdio_func_num num);

/**
 * @brief Read and parse the function-0 CCCR register file.
 *
 * Reads the card common control registers over @p dev and fills @p cccr with
 * the parsed capabilities. Does not require an SD card.
 *
 * @param dev       host endpoint
 * @param cccr      filled with the parsed CCCR contents
 * @param probe_uhs also read the UHS/drive-strength registers (CCCR rev 3.00+)
 * @retval 0 on success
 * @retval -EIO I/O error
 */
int sdio_read_cccr(struct sdio_dev *dev, struct sdio_cccr *cccr,
		   bool probe_uhs);

/**
 * @name Endpoint-level I/O
 *
 * Role-neutral SDIO I/O keyed on a host endpoint (@ref sdio_dev) and a function
 * number, with no dependency on @ref sdio_func. The @ref sdio_func based calls
 * further down are thin wrappers over these; per-function state (block size,
 * CIS limits) is supplied explicitly.
 * @{
 */

/**
 * @brief Enable a function through the CCCR I/O-enable register.
 *
 * @param dev         host endpoint
 * @param func        function number to enable
 * @param rdy_timeout I/O-ready timeout in 10ms units (0 to poll once)
 * @retval 0 function was enabled
 * @retval -ETIMEDOUT function did not become ready
 * @retval -EIO I/O error
 */
int sdio_dev_enable_func(struct sdio_dev *dev, enum sdio_func_num func,
			 uint16_t rdy_timeout);

/**
 * @brief Program a function's block size in its FBR.
 *
 * @param dev   host endpoint
 * @param func  function number
 * @param bsize block size
 * @retval 0 block size was set
 * @retval -EIO I/O error
 */
int sdio_dev_set_block_size(struct sdio_dev *dev, enum sdio_func_num func,
			    uint16_t bsize);

/**
 * @brief Read a byte from a function register (CMD52).
 * @param dev  host endpoint
 * @param func function number
 * @param reg  register address
 * @param val  filled with the byte read
 * @retval 0 on success, -EIO on I/O error, -EBUSY if the bus is busy
 */
int sdio_dev_read_byte(struct sdio_dev *dev, enum sdio_func_num func,
		       uint32_t reg, uint8_t *val);

/**
 * @brief Write a byte to a function register (CMD52).
 * @param dev       host endpoint
 * @param func      function number
 * @param reg       register address
 * @param write_val value to write
 * @retval 0 on success, -EIO on I/O error, -EBUSY if the bus is busy
 */
int sdio_dev_write_byte(struct sdio_dev *dev, enum sdio_func_num func,
			uint32_t reg, uint8_t write_val);

/**
 * @brief Write a byte and read back the result (CMD52).
 * @param dev       host endpoint
 * @param func      function number
 * @param reg       register address
 * @param write_val value to write
 * @param read_val  filled with the value read back
 * @retval 0 on success, -EIO on I/O error, -EBUSY if the bus is busy
 */
int sdio_dev_rw_byte(struct sdio_dev *dev, enum sdio_func_num func,
		     uint32_t reg, uint8_t write_val, uint8_t *read_val);

/**
 * @brief Read bytes from a fixed-address FIFO (CMD53).
 * @param dev        host endpoint
 * @param func       function number
 * @param reg        FIFO register address
 * @param data       filled with the data read
 * @param len        number of bytes to read
 * @param block_size negotiated block size (0 to force byte mode)
 * @param max_byte   byte-mode transfer limit
 * @retval 0 on success, -EIO on I/O error, -EBUSY if the bus is busy
 */
int sdio_dev_read_fifo(struct sdio_dev *dev, enum sdio_func_num func,
		       uint32_t reg, uint8_t *data, uint32_t len,
		       uint16_t block_size, uint16_t max_byte);

/**
 * @brief Write bytes to a fixed-address FIFO (CMD53).
 * @param dev        host endpoint
 * @param func       function number
 * @param reg        FIFO register address
 * @param data       data to write
 * @param len        number of bytes to write
 * @param block_size negotiated block size (0 to force byte mode)
 * @param max_byte   byte-mode transfer limit
 * @retval 0 on success, -EIO on I/O error, -EBUSY if the bus is busy
 */
int sdio_dev_write_fifo(struct sdio_dev *dev, enum sdio_func_num func,
			uint32_t reg, uint8_t *data, uint32_t len,
			uint16_t block_size, uint16_t max_byte);

/**
 * @brief Read blocks from a fixed-address FIFO (CMD53, block mode).
 * @param dev        host endpoint
 * @param func       function number
 * @param reg        FIFO register address
 * @param data       filled with the data read
 * @param blocks     number of blocks to read
 * @param block_size block size
 * @retval 0 on success, -EIO on I/O error, -EBUSY if the bus is busy
 */
int sdio_dev_read_blocks_fifo(struct sdio_dev *dev, enum sdio_func_num func,
			      uint32_t reg, uint8_t *data, uint32_t blocks,
			      uint16_t block_size);

/**
 * @brief Write blocks to a fixed-address FIFO (CMD53, block mode).
 * @param dev        host endpoint
 * @param func       function number
 * @param reg        FIFO register address
 * @param data       data to write
 * @param blocks     number of blocks to write
 * @param block_size block size
 * @retval 0 on success, -EIO on I/O error, -EBUSY if the bus is busy
 */
int sdio_dev_write_blocks_fifo(struct sdio_dev *dev, enum sdio_func_num func,
			       uint32_t reg, uint8_t *data, uint32_t blocks,
			       uint16_t block_size);

/**
 * @brief Copy bytes from an incrementing-address window (CMD53).
 * @param dev        host endpoint
 * @param func       function number
 * @param reg        start register address
 * @param data       filled with the data read
 * @param len        number of bytes to read
 * @param block_size negotiated block size (0 to force byte mode)
 * @param max_byte   byte-mode transfer limit
 * @retval 0 on success, -EIO on I/O error, -EBUSY if the bus is busy
 */
int sdio_dev_read_addr(struct sdio_dev *dev, enum sdio_func_num func,
		       uint32_t reg, uint8_t *data, uint32_t len,
		       uint16_t block_size, uint16_t max_byte);

/**
 * @brief Copy bytes to an incrementing-address window (CMD53).
 * @param dev        host endpoint
 * @param func       function number
 * @param reg        start register address
 * @param data       data to write
 * @param len        number of bytes to write
 * @param block_size negotiated block size (0 to force byte mode)
 * @param max_byte   byte-mode transfer limit
 * @retval 0 on success, -EIO on I/O error, -EBUSY if the bus is busy
 */
int sdio_dev_write_addr(struct sdio_dev *dev, enum sdio_func_num func,
			uint32_t reg, uint8_t *data, uint32_t len,
			uint16_t block_size, uint16_t max_byte);

/** @} */

/**
 * @brief Initialize SDIO function on an SD card.
 *
 * SD-card stack entry point. Binds the function to the card's host endpoint and
 * reads its CIS. After this call the function can be used for I/O.
 *
 * @param card SD card to enable function on
 * @param func function structure to initialize
 * @param num  function number to initialize
 * @retval 0 function was initialized successfully
 * @retval -EIO: I/O error
 */
int sdio_init_func(struct sd_card *card, struct sdio_func *func,
		   enum sdio_func_num num);

/**
 * @brief Enable SDIO function
 *
 * @param func: function to enable
 * @retval 0 function was enabled successfully
 * @retval -ETIMEDOUT: card I/O timed out
 * @retval -EIO: I/O error
 */
int sdio_enable_func(struct sdio_func *func);

/**
 * @brief Set block size of SDIO function
 *
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
 * @param func: function to read from
 * @param reg: register address to read from
 * @param val: filled with byte value read from register
 * @retval 0 read succeeded
 * @retval -EIO: I/O error
 */
int sdio_read_byte(struct sdio_func *func, uint32_t reg, uint8_t *val);

/**
 * @brief Write byte to SDIO register
 *
 * @param func: function to write to
 * @param reg: register address to write to
 * @param write_val: value to write to register
 * @retval 0 write succeeded
 * @retval -EIO: I/O error
 */
int sdio_write_byte(struct sdio_func *func, uint32_t reg, uint8_t write_val);

/**
 * @brief Write byte to SDIO register, and read result
 *
 * @param func: function to write to
 * @param reg: register address to write to
 * @param write_val: value to write to register
 * @param read_val: filled with value read from register
 * @retval 0 write succeeded
 * @retval -EIO: I/O error
 */
int sdio_rw_byte(struct sdio_func *func, uint32_t reg, uint8_t write_val,
		 uint8_t *read_val);

/**
 * @brief Read bytes from SDIO fifo (fixed address)
 *
 * @param func: function to read from
 * @param reg: register address of fifo
 * @param data: filled with data read from fifo
 * @param len: length of data to read from card
 * @retval 0 read succeeded
 * @retval -EIO: I/O error
 */
int sdio_read_fifo(struct sdio_func *func, uint32_t reg, uint8_t *data,
		   uint32_t len);

/**
 * @brief Write bytes to SDIO fifo (fixed address)
 *
 * @param func: function to write to
 * @param reg: register address of fifo
 * @param data: data to write to fifo
 * @param len: length of data to write to card
 * @retval 0 write succeeded
 * @retval -EIO: I/O error
 */
int sdio_write_fifo(struct sdio_func *func, uint32_t reg, uint8_t *data,
		    uint32_t len);

/**
 * @brief Read blocks from SDIO fifo (fixed address)
 *
 * @param func: function to read from
 * @param reg: register address of fifo
 * @param data: filled with data read from fifo
 * @param blocks: number of blocks to read from fifo
 * @retval 0 read succeeded
 * @retval -EIO: I/O error
 */
int sdio_read_blocks_fifo(struct sdio_func *func, uint32_t reg, uint8_t *data,
			  uint32_t blocks);

/**
 * @brief Write blocks to SDIO fifo (fixed address)
 *
 * @param func: function to write to
 * @param reg: register address of fifo
 * @param data: data to write to fifo
 * @param blocks: number of blocks to write to fifo
 * @retval 0 write succeeded
 * @retval -EIO: I/O error
 */
int sdio_write_blocks_fifo(struct sdio_func *func, uint32_t reg, uint8_t *data,
			   uint32_t blocks);

/**
 * @brief Copy bytes from an SDIO card (incrementing address)
 *
 * @param func: function to read from
 * @param reg: register address to start copy at
 * @param data: buffer to copy data into
 * @param len: length of data to read
 * @retval 0 read succeeded
 * @retval -EIO: I/O error
 */
int sdio_read_addr(struct sdio_func *func, uint32_t reg, uint8_t *data,
		   uint32_t len);

/**
 * @brief Copy bytes to an SDIO card (incrementing address)
 *
 * @param func: function to write to
 * @param reg: register address to start copy at
 * @param data: buffer to copy data from
 * @param len: length of data to write
 * @retval 0 write succeeded
 * @retval -EIO: I/O error
 */
int sdio_write_addr(struct sdio_func *func, uint32_t reg, uint8_t *data,
		    uint32_t len);

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_SD_SDIO_H_ */
