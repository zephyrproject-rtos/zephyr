/*
 * Copyright (c) 2026 Cirrus Logic, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_AUDIO_CS35LXX_H_
#define ZEPHYR_DRIVERS_AUDIO_CS35LXX_H_

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/spi.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @cond INTERNAL_HIDDEN */

#define CS35LXX_REGISTER_WIDTH sizeof(uint32_t)
#define CS35LXX_ADDRESS_WIDTH  sizeof(uint32_t)

union cs35lxx_bus {
#if CONFIG_AUDIO_CODEC_CS35LXX_I2C
	struct i2c_dt_spec i2c;
#endif /* CONFIG_AUDIO_CODEC_CS35LXX_I2C */
#if CONFIG_AUDIO_CODEC_CS35LXX_SPI
	struct spi_dt_spec spi;
#endif /* CONFIG_AUDIO_CODEC_CS35LXX_SPI */
};

typedef bool (*cs35lxx_io_is_ready)(const union cs35lxx_bus *const bus);
typedef const struct device *(*cs35lxx_io_get_device)(const union cs35lxx_bus *const bus);
typedef int (*cs35lxx_io_read)(const union cs35lxx_bus *const bus, const uint32_t addr,
			       uint32_t *const rx, const uint32_t len);
typedef int (*cs35lxx_io_write)(const union cs35lxx_bus *const bus, const uint32_t addr,
				uint32_t *const tx, const uint32_t len);
typedef int (*cs35lxx_io_raw_write)(const union cs35lxx_bus *const bus, const uint32_t addr,
				    const uint32_t *const tx, const uint32_t len);

struct cs35lxx_io {
	cs35lxx_io_is_ready is_ready;
	cs35lxx_io_get_device get_device;
	cs35lxx_io_read read;
	cs35lxx_io_write write;
	cs35lxx_io_raw_write raw_write;
};

#if CONFIG_AUDIO_CODEC_CS35LXX_I2C
extern const struct cs35lxx_io cs35lxx_io_i2c;
#endif /* CONFIG_AUDIO_CODEC_CS35LXX_I2C */

#if CONFIG_AUDIO_CODEC_CS35LXX_SPI
extern const struct cs35lxx_io cs35lxx_io_spi;
#endif /* CONFIG_AUDIO_CODEC_CS35LXX_SPI */

struct cs35lxx_io_bus {
	const union cs35lxx_bus bus;
	const struct cs35lxx_io *const io;
};

bool cs35lxx_is_bus_ready(const struct cs35lxx_io_bus *const io_bus);

const struct device *cs35lxx_get_control_port(const struct cs35lxx_io_bus *const io_bus);

int cs35lxx_burst_read(const struct cs35lxx_io_bus *const io_bus, const uint32_t addr,
		       uint32_t *const rx, const uint32_t len);

int cs35lxx_read(const struct cs35lxx_io_bus *const io_bus, const uint32_t addr,
		 uint32_t *const rx);

int cs35lxx_burst_write(const struct cs35lxx_io_bus *const io_bus, const uint32_t addr,
			uint32_t *const tx, const uint32_t len);

int cs35lxx_write(const struct cs35lxx_io_bus *const io_bus, const uint32_t addr,
		  const uint32_t val);

int cs35lxx_raw_burst_write(const struct cs35lxx_io_bus *const io_bus, const uint32_t addr,
			    const uint32_t *const tx, const uint32_t len);

int cs35lxx_raw_write(const struct cs35lxx_io_bus *const io_bus, const uint32_t addr,
		      const uint32_t val);

int cs35lxx_update(const struct cs35lxx_io_bus *const io_bus, const uint32_t addr,
		   const uint32_t mask, const uint32_t val);

/** @endcond */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ZEPHYR_DRIVERS_AUDIO_CS35LXX_H_ */
