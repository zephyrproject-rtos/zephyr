/*
 * Copyright (c) 2026 Cirrus Logic, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_HAPTICS_CIRRUS_CS40LXX_H_
#define ZEPHYR_DRIVERS_HAPTICS_CIRRUS_CS40LXX_H_

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/pm/device.h>
#include <zephyr/sys/byteorder.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @cond INTERNAL_HIDDEN */

/** Device-agnostic helper macros */
#define CS40LXX_REGISTER_WIDTH sizeof(uint32_t)
#define CS40LXX_ADDRESS_WIDTH  sizeof(uint32_t)

/* clang-format off */
/**
 * @brief Convert a list of uint32_t values to big-endian at compile-time
 *
 * @param[in] __VA_ARGS__ Comma-separated list of uint32_t values
 */
#define CS40LXX_WRITE_BE32(...) FOR_EACH(sys_cpu_to_be32, (,), __VA_ARGS__)

/**
 * @brief Convert a list of uint32_t values to big-endian at compile-time
 *
 * @details Use with @ref cirrus_multi_write to format register writes.
 *
 * @param[in] __VA_ARGS__ Comma-separated list of uint32_t values
 */
#define CS40LXX_MULTI_WRITE_BE32(...)                                                              \
	.buf = (uint32_t[]){FOR_EACH(sys_cpu_to_be32, (,), __VA_ARGS__)},                          \
	.len = NUM_VA_ARGS(__VA_ARGS__)
/* clang-format on */

/**
 * @brief Storage for multi-register writes
 *
 * @details Intended for bulk register writes with contents known at compile-time. See
 * @ref cs40lxx_multi_write() and @ref cs40lxx_raw_multi_write() for usage.
 */
struct cs40lxx_multi_write {
	uint32_t addr; /**< Hardware register address. */
	uint32_t *buf; /**< Array of register contents to be written. */
	uint32_t len;  /**< Number of registers to be written. */
};

/**
 * @brief Bus for a haptic device instance
 */
union cs40lxx_bus {
#if CONFIG_HAPTICS_CS40LXX_I2C
	struct i2c_dt_spec i2c;
#endif /* CONFIG_HAPTICS_CS40LXX_I2C */
#if CONFIG_HAPTICS_CS40LXX_SPI
	struct spi_dt_spec spi;
#endif /* CONFIG_HAPTICS_CS40LXX_SPI */
};

/**
 * @brief Wrapper for @ref device_is_ready()
 *
 * @param[in] bus Pointer to the @ref cs40lxx_bus structure for the haptic device instance
 *
 * @return a value from @ref device_is_ready()
 */
typedef bool (*cs40lxx_io_is_ready)(const union cs40lxx_bus *const bus);

/**
 * @brief Gets a pointer to the device structure for the I/O bus
 *
 * @param[in] bus Pointer to the @ref cs40lxx_bus structure for the haptic device instance
 *
 * @return Pointer to the device structure for the I/O bus instance
 */
typedef const struct device *(*cs40lxx_io_get_device)(const union cs40lxx_bus *const bus);

/**
 * @brief Wrapper for @ref i2c_write_read_dt() or @ref spi_read_dt()
 *
 * @details Register contents are read from the haptic driver in big-endian format and converted
 * to the system's endianness.
 *
 * @param[in] bus Pointer to the @ref cs40lxx_bus structure for the haptic device instance
 * @param[in] addr Hardware register address
 * @param[out] rx Array storage for register contents
 * @param[in] len Number of registers to read
 *
 * @return a value from @ref i2c_write_read_dt() or @ref spi_read_dt()
 */
typedef int (*cs40lxx_io_read)(const union cs40lxx_bus *const bus, const uint32_t addr,
			       uint32_t *const rx, const uint32_t len);

/**
 * @brief Wrapper for @ref i2c_transfer_dt() or @ref spi_write_dt()
 *
 * @details If the system is little-endian, array contents are converted to big-endian and then
 * written to the haptic device. Note that the contents of @p tx is modified in-place.
 *
 * @param[in] bus Pointer to the @ref cs40lxx_bus structure for the haptic device instance
 * @param[in] addr Hardware register address
 * @param[in, out] tx Array storage for register contents
 * @param[in] len Number of registers to write
 *
 * @return a value from @ref i2c_transfer_dt() or @ref spi_write_dt()
 */
typedef int (*cs40lxx_io_write)(const union cs40lxx_bus *const bus, const uint32_t addr,
				uint32_t *const tx, const uint32_t len);

/**
 * @brief Wrapper for @ref i2c_transfer_dt() or @ref spi_write_dt()
 *
 * @details Array contents are written to the haptic driver without changing the endianness.
 *
 * @param[in] bus Pointer to the @ref cs40lxx_bus structure for the haptic device instance
 * @param[in] addr Hardware register address
 * @param[in] tx Array storage for register contents
 * @param[in] len Number of registers to write
 *
 * @return a value from @ref i2c_transfer_dt() or @ref spi_write_dt()
 */
typedef int (*cs40lxx_io_raw_write)(const union cs40lxx_bus *const bus, const uint32_t addr,
				    const uint32_t *const tx, const uint32_t len);

/**
 * @brief Storage for I/O bus function pointers
 *
 * @details Used to generically support I/O operations regardless of bus implementation details.
 * See @ref cs40lxx_io_is_ready(), @ref cs40lxx_io_get_device(), @ref cs40lxx_io_read(),
 * @ref cs40lxx_io_write(), and @ref cs40lxx_io_raw_write() for details.
 */
struct cs40lxx_io {
	cs40lxx_io_is_ready is_ready;
	cs40lxx_io_get_device get_device;
	cs40lxx_io_read read;
	cs40lxx_io_write write;
	cs40lxx_io_raw_write raw_write;
};

#if CONFIG_HAPTICS_CS40LXX_I2C
/** I2C-based I/O functions */
extern const struct cs40lxx_io cs40lxx_io_i2c;
#endif /* CONFIG_HAPTICS_CS40LXX_I2C */

#if CONFIG_HAPTICS_CS40LXX_SPI
/** SPI-based I/O functions */
extern const struct cs40lxx_io cs40lxx_io_spi;
#endif /* CONFIG_HAPTICS_CS40LXX_SPI */

/**
 * @brief Bus and corresponding I/O functions for a haptic device instance
 */
struct cs40lxx_io_bus {
	const union cs40lxx_bus bus;
	const struct cs40lxx_io *const io;
};

/**
 * @brief Refer to @ref cs40lxx_io_is_ready()
 */
bool cs40lxx_is_bus_ready(const struct cs40lxx_io_bus *const io_bus);

/**
 * @brief Refer to @ref cs40lxx_io_get_device()
 */
const struct device *cs40lxx_get_control_port(const struct cs40lxx_io_bus *const io_bus);

/**
 * @brief Refer to @ref cs40lxx_io_read()
 */
int cs40lxx_burst_read(const struct cs40lxx_io_bus *const io_bus, const uint32_t addr,
		       uint32_t *const rx, const uint32_t len);

/**
 * @brief Extension of @ref cs40lxx_burst_read()
 *
 * @details Read from a single register, i.e., call @ref cs40lxx_burst_read() with length 1.
 *
 * @param[in] bus Pointer to the @ref cs40lxx_bus structure for the haptic device instance
 * @param[in] addr Hardware register address
 * @param[out] rx Storage for register contents
 *
 * @return a value from @ref cs40lxx_burst_read()
 */
int cs40lxx_read(const struct cs40lxx_io_bus *const io_bus, const uint32_t addr,
		 uint32_t *const rx);

/**
 * @brief Refer to @ref cs40lxx_io_write()
 */
int cs40lxx_burst_write(const struct cs40lxx_io_bus *const io_bus, const uint32_t addr,
			uint32_t *const tx, const uint32_t len);

/**
 * @brief Extension of @ref cs40lxx_burst_write()
 *
 * @details Write to a single register, i.e., call @ref cs40lxx_burst_write() with length 1.
 *
 * @param[in] bus Pointer to the @ref cs40lxx_bus structure for the haptic device instance
 * @param[in] addr Hardware register address
 * @param[in] val Register content to write
 *
 * @return a value from @ref cs40lxx_burst_write()
 */
int cs40lxx_write(const struct cs40lxx_io_bus *const io_bus, const uint32_t addr, uint32_t val);

/**
 * @brief Refer to @ref cs40lxx_io_raw_write()
 */
int cs40lxx_raw_burst_write(const struct cs40lxx_io_bus *const io_bus, const uint32_t addr,
			    const uint32_t *const tx, const uint32_t len);

/**
 * @brief Extension of @ref cs40lxx_raw_burst_write()
 *
 * @details Write to a single register without changing endianness of register content, i.e., call
 * @ref cs40lxx_raw_burst_write() with length 1.
 *
 * @param[in] bus Pointer to the @ref cs40lxx_bus structure for the haptic device instance
 * @param[in] addr Hardware register address
 * @param[in] val Register content to write
 *
 * @return a value from @ref cs40lxx_raw_burst_write()
 */
int cs40lxx_raw_write(const struct cs40lxx_io_bus *const io_bus, const uint32_t addr,
		      const uint32_t val);

/**
 * @brief Write a sequence of multi-register transactions
 *
 * @param[in] bus Pointer to the @ref cs40lxx_bus structure for the haptic device instance
 * @param[in] multi_write Refer to @ref cs40lxx_multi_write
 * @param[in] len Number of multi-register sequences to write
 *
 * @retval 0 if success
 * @retval <0 if failure
 */
int cs40lxx_multi_write(const struct cs40lxx_io_bus *const io_bus,
			const struct cs40lxx_multi_write *const multi_write, const uint32_t len);

/**
 * @brief Write a sequence of multi-register transactions
 *
 * @details Similar to @ref cs40lxx_multi_write() except register contents are written without
 * changing endianness.
 *
 * @param[in] bus Pointer to the @ref cs40lxx_bus structure for the haptic device instance
 * @param[in] multi_write Refer to @ref cs40lxx_multi_write
 * @param[in] len Number of multi-register sequences to write
 *
 * @retval 0 if success
 * @retval <0 if failure
 */
int cs40lxx_raw_multi_write(const struct cs40lxx_io_bus *const io_bus,
			    const struct cs40lxx_multi_write *const multi_write,
			    const uint32_t len);

/** @endcond */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ZEPHYR_DRIVERS_HAPTICS_CIRRUS_CS40LXX_H_ */
