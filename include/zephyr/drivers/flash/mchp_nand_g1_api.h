/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Microchip NAND Flash G1 API interface.
 */

#ifndef __ZEPHYR_INCLUDE_DRIVERS_FLASH_MCHP_NAND_G1_API_H__
#define __ZEPHYR_INCLUDE_DRIVERS_FLASH_MCHP_NAND_G1_API_H__

#include <zephyr/syscall.h>
#include <zephyr/device.h>

/**
 * @brief Maximum length of the NAND Flash ID data.
 */
#define MAX_ID_LEN 5

/**
 * @brief NAND flash device information.
 *
 * This structure contains the identification, geometry, ECC, feature, and
 * timing information of a NAND flash device.
 */
struct nand_info {
	/**
	 * @brief NAND device identification data.
	 *
	 * The identification can be accessed either through the individual
	 * manufacturer and device ID fields or through the raw @ref id array.
	 */
	union {
		struct {
			/** NAND manufacturer ID. */
			uint8_t mfr_id;
			/** NAND device ID. */
			uint8_t dev_id;
		};

		/**
		 * @brief Raw NAND ID data.
		 */
		uint8_t id[MAX_ID_LEN];
	};
	/** Number of data bytes per page. */
	uint16_t pagesize;
	/** Number of spare bytes per page. */
	uint16_t oobsize;
	/** Number of pages per block. */
	uint16_t blockpages;
	/** Number of blocks per chip. */
	uint16_t blocknum;
	/** Number of bits ECC correctability per sector. */
	uint8_t  eccbits;
	/** ECC sector size in bytes. */
	uint16_t eccsize;
	/** Features supported. */
	uint16_t features;
	/** Optional commands supported. */
	uint16_t opt_cmd;
	/** Timing mode support. */
	uint16_t timingmode;
	/** Number of address cycles */
	uint8_t  addrcycle;
	/** Number of bits per cell. */
	uint8_t  cellbits;
};

/**
 *  @brief  Read page from nandflash with ecc
 *
 *  @param  dev             : nandflash dev
 *  @param  page            : page index
 *  @param  buf             : buffer to store read data
 *  @param  oob_req         : oob is required
 *
 *  @return 0 on success, no flipped bit found
 *  @return >0 positive value indicating the number of corrected bit flips,
 *          or the maximum corrected bit flips in an ecc sector of the page
 *  @return -EBADMSG indicating ECC error cannot be corrected
 *  @return <0 negative values for other errors
 */
__syscall int nand_read_page(const struct device *dev,
			     unsigned int page, void *buf, int oob_req);

/**
 *  @brief  Write page to nandflash with ecc
 *
 *  @param  dev             : nandflash dev
 *  @param  page            : page index
 *  @param  buf             : data to write
 *  @param  oob_req         : oob is required
 *
 *  @return 0 on success
 *  @return -EIO indicating a nand status error, page programming failed
 *  @return <0 negative values for other errors
 */
__syscall int nand_write_page(const struct device *dev,
			      unsigned int page, const void *buf, int oob_req);

/**
 *  @brief  Read page raw data from nandflash
 *
 *  @param  dev             : nandflash dev
 *  @param  page            : page index
 *  @param  buf             : buffer to store read data
 *  @param  oob_req         : oob is required
 *
 *  @return 0 on success
 *  @return <0 negative values for other errors
 */
__syscall int nand_read_page_raw(const struct device *dev,
				 unsigned int page, void *buf, int oob_req);

/**
 *  @brief  Write page raw data to nandflash
 *
 *  @param  dev             : nandflash dev
 *  @param  page            : page index
 *  @param  buf             : data to write
 *  @param  oob_req         : oob is required
 *
 *  @return 0 on success
 *  @return -EIO indicating a nand status error, page programming failed
 *  @return <0 negative values for other errors
 */
__syscall int nand_write_page_raw(const struct device *dev,
				  unsigned int page, const void *buf, int oob_req);

/**
 *  @brief  Read page oob data from nandflash
 *
 *  @param  dev             : nandflash dev
 *  @param  page            : page index
 *  @param  buf             : buffer to store read data
 *
 *  @return 0 on success
 *  @return <0 negative values for other errors
 */
__syscall int nand_read_oob(const struct device *dev,
			    unsigned int page, void *buf);

/**
 *  @brief  Write page oob data to nandflash
 *
 *  @param  dev             : nandflash dev
 *  @param  page            : page index
 *  @param  buf             : data to write
 *
 *  @return 0 on success
 *  @return -EIO indicating a nand status error, page programming failed
 *  @return <0 negative values for other errors
 */
__syscall int nand_write_oob(const struct device *dev,
			     unsigned int page, const void *buf);

/**
 *  @brief  Erase block of nandflash
 *
 *  @param  dev             : nandflash dev
 *  @param  block           : block index
 *
 *  @return 0 on success
 *  @return -EIO indicating a nand status error, block erase failed
 *  @return <0 negative values for other errors
 */
__syscall int nand_erase(const struct device *dev, unsigned int block);

/**
 *  @brief  Get information of nandflash
 *
 *  @param  dev             : nandflash dev
 *  @param  info            : pointer to a nand_info structure
 *
 *  @return 0 on success
 */
__syscall int nand_info(const struct device *dev, struct nand_info *info);

#include <zephyr/syscalls/mchp_nand_g1_api.h>

#endif /* __ZEPHYR_INCLUDE_DRIVERS_FLASH_MCHP_NAND_G1_API_H__ */
