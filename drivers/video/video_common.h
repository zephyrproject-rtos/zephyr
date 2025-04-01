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

/** @} */

/**
 * @defgroup video_imager Video Imager (image sensor) shared implementation
 *
 * This API is targeting image sensor driver developers.
 *
 * It provides a common implementation that only requires implementing a table of
 * @ref video_imager_mode that lists the various resolution, frame rates and associated I2C
 * configuration registers.
 *
 * The @c video_imager_... functions can be submitted directly to the @rev video_api.
 * If a driver also needs to do extra work before or after applying a mode, it is possible
 * to provide a custom wrapper or skip these default implementation altogether.
 *
 * @{
 */

/**
 * @brief Table entry for an imaging mode of the sensor device.
 *
 * AA mode can be applied to an imager to configure a particular framerate, resolution, and pixel
 * format. The index of the table of modes is meant to match the index of the table of formats.
 */
struct video_imager_mode {
	/* FPS for this mode */
	uint16_t fps;
	/* Multiple lists of registers to allow sharing common sets of registers across modes. */
	const struct video_reg *regs[3];
};

/**
 * @brief A video imager device is expected to have dev->data point to this structure.
 *
 * In order to support custom data structure, it is possible to store an extra pointer
 * in the dev->config struct. See existing drivers for an example.
 */
struct video_imager_config {
	/** List of all formats supported by this sensor */
	const struct video_format_cap *fmts;
	/** Array of modes tables, one table per format cap lislted by "fmts" */
	const struct video_imager_mode **modes;
	/** I2C device to write the registers to */
	struct i2c_dt_spec i2c;
	/** Write a table of registers onto the device */
	int (*write_multi)(const struct i2c_dt_spec *i2c, const struct video_reg *regs);
	/** Reference to a ; */
	struct video_imager_data *data;
};

/**
 * @brief A video imager device is expected to have dev->data point to this structure.
 *
 * In order to support custom data structure, it is possible to store an extra pointer
 * in the dev->config struct. See existing drivers for an example.
 */
struct video_imager_data {
	/** Index of the currently active format in both modes[] and fmts[] */
	int fmt_id;
	/** Currently active video format */
	struct video_format fmt;
	/** Currently active operating mode as defined above */
	const struct video_imager_mode *mode;
};

/**
 * @brief Initialize one row of a @struct video_format_cap with fixed width and height.
 *
 * The minimum and maximum are the same for both width and height fields.
 * @param
 */
#define VIDEO_IMAGER_FORMAT_CAP(pixfmt, width, height)                                             \
	{                                                                                          \
		.width_min = (width), .width_max = (width), .width_step = 0,                       \
		.height_min = (height), .height_max = (height), .height_step = 0,                  \
		.pixelformat = (pixfmt),                                                           \
	}

/**
 * @brief Set the operating mode of the imager as defined in @ref video_imager_mode.
 *
 * If the default immplementation for the video API are used, there is no need to explicitly call
 * this function in the image sensor driver.
 *
 * @param dev Device that has a struct video_imager in @c dev->data.
 * @param mode The mode to apply to the image sensor.
 * @return 0 if successful, or negative error number otherwise.
 */
int video_imager_set_mode(const struct device *dev, const struct video_imager_mode *mode);

/** @brief Default implementation for image drivers frame interval selection */
int video_imager_set_frmival(const struct device *dev, enum video_endpoint_id ep,
			     struct video_frmival *frmival);
/** @brief Default implementation for image drivers frame interval query */
int video_imager_get_frmival(const struct device *dev, enum video_endpoint_id ep,
			     struct video_frmival *frmival);
/** @brief Default implementation for image drivers frame interval enumeration */
int video_imager_enum_frmival(const struct device *dev, enum video_endpoint_id ep,
			      struct video_frmival_enum *fie);
/** @brief Default implementation for image drivers format selection */
int video_imager_set_fmt(const struct device *const dev, enum video_endpoint_id ep,
			 struct video_format *fmt);
/** @brief Default implementation for image drivers format query */
int video_imager_get_fmt(const struct device *dev, enum video_endpoint_id ep,
			 struct video_format *fmt);
/** @brief Default implementation for image drivers format capabilities */
int video_imager_get_caps(const struct device *dev, enum video_endpoint_id ep,
			  struct video_caps *caps);

/**
 * Initialize an imager by loading init_regs onto the device, and setting the default format.
 *
 * @param dev Device that has a struct video_imager in @c dev->data.
 * @param init_regs If non-NULL, table of registers to configure at init.
 * @param default_fmt_idx Default format index to apply at init.
 * @return 0 if successful, or negative error number otherwise.
 */
int video_imager_init(const struct device *dev, const struct video_reg *init_regs,
		      int default_fmt_idx);

/** @} */

#endif /* ZEPHYR_DRIVERS_VIDEO_COMMON_H_ */
