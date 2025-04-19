/*
 * Copyright (c) 2025 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_VIDEO_COMMON_H_
#define ZEPHYR_DRIVERS_VIDEO_COMMON_H_

#include <stddef.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/i2c.h>

/**
 * Type used by register tables that have either the address or value 16-bit wide.
 */
struct video_reg {
	/** Address of the register to write to as well as  */
	uint32_t addr;
	/** Value to write in this address */
	uint32_t data;
};

/**
 * @defgroup video_cci Video Camera Control Interface (CCI) handling
 *
 * The Camera Control Interface (CCI) is an I2C communication scheme part of MIPI-CSI.
 * It defines how register addresses and register values are packed into I2C messages.
 *
 * After the I2C device address, I2C messages payload contain:
 *
 * 1. THe 8-bit or 16-bit of the address in big-endian, written to the device.
 * 3. The 8-bit of the register data either read or written.
 *
 * To write to registers larger than 8-bit, multiple read/writes messages are issued.
 * Endianness and segmentation of larger registers are defined on a per-sensor basis.
 *
 * @{
 */

/** @cond INTERNAL_HIDDEN */
#define VIDEO_REG_ENDIANNESS_MASK		(uint32_t)(GENMASK(24, 24))
#define VIDEO_REG_ADDR_SIZE_MASK		(uint32_t)(GENMASK(23, 20))
#define VIDEO_REG_DATA_SIZE_MASK		(uint32_t)(GENMASK(19, 16))
#define VIDEO_REG_ADDR_MASK			(uint32_t)(GENMASK(15, 0))

#define VIDEO_REG(addr_size, data_size, endianness)                                                \
	(FIELD_PREP(VIDEO_REG_ADDR_SIZE_MASK, (addr_size)) |                                       \
	 FIELD_PREP(VIDEO_REG_DATA_SIZE_MASK, (data_size)) |                                       \
	 FIELD_PREP(VIDEO_REG_ENDIANNESS_MASK, (endianness)))
/** @endcond */

/** Flag a register as 8-bit address size, 8-bit data size */
#define VIDEO_REG_ADDR8_DATA8		VIDEO_REG(1, 1, false)
/** Flag a register as 8-bit address size, 16-bit data size, little-endian */
#define VIDEO_REG_ADDR8_DATA16_LE	VIDEO_REG(1, 2, false)
/** Flag a register as 8-bit address size, 16-bit data size, big-endian */
#define VIDEO_REG_ADDR8_DATA16_BE	VIDEO_REG(1, 2, true)
/** Flag a register as 8-bit address size, 24-bit data size, little-endian */
#define VIDEO_REG_ADDR8_DATA24_LE	VIDEO_REG(1, 3, false)
/** Flag a register as 8-bit address size, 24-bit data size, big-endian */
#define VIDEO_REG_ADDR8_DATA24_BE	VIDEO_REG(1, 3, true)
/** Flag a register as 8-bit address size, 32-bit data size, little-endian */
#define VIDEO_REG_ADDR8_DATA32_LE	VIDEO_REG(1, 4, false)
/** Flag a register as 8-bit address size, 32-bit data size, big-endian */
#define VIDEO_REG_ADDR8_DATA32_BE	VIDEO_REG(1, 4, true)
/** Flag a register as 16-bit address size, 8-bit data size */
#define VIDEO_REG_ADDR16_DATA8		VIDEO_REG(2, 1, false)
/** Flag a register as 16-bit address size, 16-bit data size, little-endian */
#define VIDEO_REG_ADDR16_DATA16_LE	VIDEO_REG(2, 2, false)
/** Flag a register as 16-bit address size, 16-bit data size, big-endian */
#define VIDEO_REG_ADDR16_DATA16_BE	VIDEO_REG(2, 2, true)
/** Flag a register as 16-bit address size, 24-bit data size, little-endian */
#define VIDEO_REG_ADDR16_DATA24_LE	VIDEO_REG(2, 3, false)
/** Flag a register as 16-bit address size, 24-bit data size, big-endian */
#define VIDEO_REG_ADDR16_DATA24_BE	VIDEO_REG(2, 3, true)
/** Flag a register as 16-bit address size, 32-bit data size, little-endian */
#define VIDEO_REG_ADDR16_DATA32_LE	VIDEO_REG(2, 4, false)
/** Flag a register as 16-bit address size, 32-bit data size, big-endian */
#define VIDEO_REG_ADDR16_DATA32_BE	VIDEO_REG(2, 4, true)

/**
 * @brief Write a Camera Control Interface register value with the specified address and size.
 *
 * The size of the register address and register data passed as flags in the high bits of
 * @p reg_addr in the unused bits of the address.
 *
 * @brief i2c Reference to the video device on an I2C bus.
 * @brief reg_addr Address of the register to fill with @reg_value along with size information.
 * @brief reg_value Value to write at this address, the size to write is encoded in the address.
 */
int video_write_cci_reg(const struct i2c_dt_spec *i2c, uint32_t reg_addr, uint32_t reg_value);


/**
 * @brief Perform a read-modify-write operation on a register given an address, mask and value.
 *
 * The size of the register address and register data passed as flags in the high bits of
 * @p reg_addr in the unused bits of the address.
 *
 * @brief i2c Reference to the video device on an I2C bus.
 * @brief reg_addr Address of the register to fill with @reg_value along with size information.
 * @brief field_mask Mask of the field to insert into the existing value.
 * @brief field_value Value to write at this address, the size to write is encoded in the address.
 */
int video_write_cci_field(const struct i2c_dt_spec *i2c, uint32_t reg_addr, uint32_t field_mask,
			  uint32_t field_value);

/**
 * @brief Read a Camera Control Interace register value from the specified address and size.
 *
 * The size of the register address and register data passed as flags in the high bits of
 * @p reg_addr in the unused bits of the address.
 *
 * @brief i2c Reference to the video device on an I2C bus.
 * @brief reg_addr Address of the register to fill with @reg_value along with size information.
 * @brief reg_value Value to write at this address, the size to write is encoded in the address.
 *                  This is a 32-bit integer pointer even when reading 8-bit or 16 bits value.
 */
int video_read_cci_reg(const struct i2c_dt_spec *i2c, uint32_t reg_addr, uint32_t *reg_value);

/**
 * @brief Write a complete table of registers to a device one by one.
 *
 * The address present in the registers need to be encoding the size information using the macros
 * such as @ref VIDEO_ADDR16_REG8(). The last element must be empty (@c {0}) to mark the end of the
 * table.
 *
 * @brief i2c Reference to the video device on an I2C bus.
 * @brief regs Array of address/value pairs to write to the device sequentially.
 */
int video_write_cci_multi(const struct i2c_dt_spec *i2c, const struct video_reg *regs);

#endif /* ZEPHYR_DRIVERS_VIDEO_COMMON_H_ */
