/*
 * Copyright (c) 2025 tinyVision.ai Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_VIDEO_COMMON_H_
#define ZEPHYR_DRIVERS_VIDEO_COMMON_H_

#include <stddef.h>

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/types.h>

/**
 * @brief Register for building tables supporting 8/16 bit address and 8/16/24/32 bit value sizes.
 *
 * A flag in the address indicates the size of the register address, and the size and endianness of
 * the value.
 *
 * If willing to save space on large register tables, a more compact version @ref video_reg8 and
 * @ref video_reg16 can be used, however only supporting 8-bit values .
 *
 * The data field is in CPU-native endianness, and the library functions will perform the
 * endianness swap as needed while transmitting data.
 */
struct video_reg {
	/** Address of the register, and other flags if used with the @ref video_cci API. */
	uint32_t addr;
	/** Value to write to this address */
	uint32_t data;
};

/** Register for building tables supporting 8-bit addresses and 8-bit value sizes */
struct video_reg8 {
	/** Address of the register */
	uint8_t addr;
	/** Value to write to this address */
	uint8_t data;
};

/** Register for building tables supporting 16-bit addresses and 8-bit value sizes */
struct video_reg16 {
	/** Address of the register */
	uint16_t addr;
	/** Value to write to this address */
	uint8_t data;
};

/**
 * @defgroup video_cci Video Camera Control Interface (CCI) handling
 *
 * The Camera Control Interface (CCI) is an I2C communication scheme part of MIPI-CSI.
 * It defines how register addresses and register values are packed into I2C messages.
 *
 * After the I2C device address, I2C messages payload contain:
 *
 * 1. The 8-bit or 16-bit of the address in big-endian, written to the device.
 * 2. The 8-bit of the register data either read or written.
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

/**
 * @defgroup video_cci_reg_flags Flags describing a Video CCI register
 *
 * For a given drivers, register sizes are not expected to change, so macros can be used when
 * defining registers:
 *
 * @code{.c}
 * #define SENSORNAME_REG8(addr)  ((uint32_t)(addr) | VIDEO_REG_ADDR16_DATA8)
 * #define SENSORNAME_REG16(addr) ((uint32_t)(addr) | VIDEO_REG_ADDR16_DATA16_LE)
 * #define SENSORNAME_REG24(addr) ((uint32_t)(addr) | VIDEO_REG_ADDR16_DATA24_LE)
 * @endcode
 *
 * Then the macros can be used directly in register definitions:
 *
 * @code{.c}
 * #define SENSORNAME_REG_EXPOSURE_LEVEL SENSORNAME_REG16(0x3060)
 * #define SENSORNAME_REG_AN_GAIN_LEVEL  SENSORNAME_REG8(0x3062)
 * ...
 * video_write_cci_reg(&cfg->i2c, SENSORNAME_REG_EXPOSURE_LEVEL, 3000);
 * video_write_cci_reg(&cfg->i2c, SENSORNAME_REG_AN_GAIN_LEVEL, 220);
 * @endcode
 *
 * Or used directly inline:
 * @code{.c}
 * video_write_cci_reg(&cfg->i2c, SENSORNAME_REG16(0x3060), 3000);
 * video_write_cci_reg(&cfg->i2c, SENSORNAME_REG8(0x3062), 220);
 * @endcode
 *
 * As well as in register tables:
 *
 * @code{.c}
 * struct video_reg init_regs[] = {
 *	{SENSORNAME_REG_EXPOSURE_LEVEL, 2000},
 *	{SENSORNAME_REG_AN_GAIN_LEVEL, 180},
 *	...
 *	{0},
 * };
 * @endcode
 *
 * @{
 */
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
/** @} */

/**
 * @brief Write a Camera Control Interface register value with the specified address and size.
 *
 * The size of the register address and register data passed as flags in the high bits of
 * @p reg_addr in the unused bits of the address.
 *
 * @brief i2c Reference to the video device on an I2C bus.
 * @brief reg_addr Address of the register to fill with @p reg_data along with size information.
 * @brief reg_data Value to write at this address, the size to write is encoded in the address.
 */
int video_write_cci_reg(const struct i2c_dt_spec *i2c, uint32_t reg_addr, uint32_t reg_data);


/**
 * @brief Perform a read-modify-write operation on a register given an address, mask and value.
 *
 * The size of the register address and register data passed as flags in the high bits of
 * @p reg_addr in the unused bits of the address.
 *
 * @brief i2c Reference to the video device on an I2C bus.
 * @brief reg_addr Address of the register to fill with @p reg_data along with size information.
 * @brief field_mask Mask of the field to insert into the existing value.
 * @brief field_value Value to write at this address, the size to write is encoded in the address.
 */
int video_modify_cci_reg(const struct i2c_dt_spec *i2c, uint32_t reg_addr, uint32_t field_mask,
			 uint32_t field_value);

/**
 * @brief Read a Camera Control Interace register value from the specified address and size.
 *
 * The size of the register address and register data passed as flags in the high bits of
 * @p reg_addr in the unused bits of the address.
 *
 * @brief i2c Reference to the video device on an I2C bus.
 * @brief reg_addr Address of the register to fill with @p reg_data along with size information.
 * @brief reg_data Value to write at this address, the size to write is encoded in the address.
 *                  This is a 32-bit integer pointer even when reading 8-bit or 16 bits value.
 */
int video_read_cci_reg(const struct i2c_dt_spec *i2c, uint32_t reg_addr, uint32_t *reg_data);

/**
 * @brief Write a complete table of registers to a device one by one.
 *
 * The address present in the registers need to be encoding the size information using the macros
 * such as @ref VIDEO_REG_ADDR16_DATA8. The last element must be empty (@c {0}) to mark the end of
 * the table.
 *
 * @brief i2c Reference to the video device on an I2C bus.
 * @brief regs Array of address/value pairs to write to the device sequentially.
 * @brief num_regs Number of registers entries in the table.
 */
int video_write_cci_multiregs(const struct i2c_dt_spec *i2c, const struct video_reg *regs,
			      size_t num_regs);

/**
 * @brief Write a complete table of registers to a device one by one.
 *
 * The registers address are 8-bit wide and values are 8-bit wide.
 *
 * @brief i2c Reference to the video device on an I2C bus.
 * @brief regs Array of address/value pairs to write to the device sequentially.
 * @brief num_regs Number of registers entries in the table.
 */
int video_write_cci_multiregs8(const struct i2c_dt_spec *i2c, const struct video_reg8 *regs,
			       size_t num_regs);

/**
 * @brief Write a complete table of registers to a device one by one.
 *
 * The registers address are 16-bit wide and values are 8-bit wide.
 *
 * @brief i2c Reference to the video device on an I2C bus.
 * @brief regs Array of address/value pairs to write to the device sequentially.
 * @brief num_regs Number of registers entries in the table.
 */
int video_write_cci_multiregs16(const struct i2c_dt_spec *i2c, const struct video_reg16 *regs,
				size_t num_regs);

/** @} */

#endif /* ZEPHYR_DRIVERS_VIDEO_COMMON_H_ */
