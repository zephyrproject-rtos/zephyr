/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ZEPHYR_INCLUDE_DRIVERS_FLASH_MCHP_NAND_G1_API_H__
#define __ZEPHYR_INCLUDE_DRIVERS_FLASH_MCHP_NAND_G1_API_H__

#include <zephyr/syscall.h>
#include <zephyr/device.h>

#define MAX_ID_LEN 5

struct nand_info {
	union {
		struct {
			uint8_t mfr_id;
			uint8_t dev_id;
		};
		uint8_t id[MAX_ID_LEN];
	};
	uint16_t pagesize;
	uint16_t oobsize;
	uint16_t blockpages;
	uint16_t blocknum;
	uint8_t  eccbits;
	uint16_t eccsize;
	uint16_t features;
	uint16_t opt_cmd;
	uint16_t timingmode;
	uint8_t  addrcycle;
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
