/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for register map based sensor emulators
 * @ingroup emul_sensor_regmap
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_SENSOR_EMUL_SENSOR_REGMAP_H_
#define ZEPHYR_INCLUDE_DRIVERS_SENSOR_EMUL_SENSOR_REGMAP_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/devicetree.h>
#include <zephyr/drivers/emul.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>

/**
 * @brief Register map based sensor emulators
 * @defgroup emul_sensor_regmap Register map based sensor emulators
 * @ingroup sensor_emulator_backend
 *
 * Describes an I2C register based sensor as data transcribed from its datasheet: the register
 * table and where each measurement lives in it. The emulator implements the bus protocol and
 * the sensor emulator backend API from that description.
 *
 * @{
 */

/** Register is read only, writes are ignored. */
#define EMUL_SENSOR_REG_RO BIT(0)

/**
 * @brief Describe a register and its bus access rules.
 *
 * Each entry declares one address that the driver may access. Registers start at @a reset.
 * Bus writes use @a write_mask to preserve non-writable bits (zero allows all bits), then
 * clear @a self_clear bits. Reads clear
 * @a clear_on_read bits after returning the last byte. These masks describe bit changes only;
 * they do not simulate the commands or interrupts associated with those bits.
 *
 * For example, this one-byte register starts at 0x20 and allows writes to bits 1 and 0:
 * @code{.c}
 * static const struct emul_sensor_reg regs[] = {
 *         { .addr = 0x01, .name = "CONFIG", .bytes = 1,
 *           .reset = 0x20, .write_mask = 0x03 },
 * };
 * @endcode
 */
struct emul_sensor_reg {
	/** Register address. */
	uint8_t addr;
	/** Register name, used in logs. */
	const char *name;
	/**
	 * Bus access policy. Set @ref EMUL_SENSOR_REG_RO to ignore bus writes, or 0 to
	 * allow writes subject to @a write_mask. Sample injection can update read-only
	 * registers. Reads remain allowed in either case.
	 */
	uint8_t flags;
	/** Register width: 1 to 4 bytes, or 0 for the device default. */
	uint8_t bytes;
	/** Value after reset. */
	uint32_t reset;
	/** Bits cleared after a write; no associated command is simulated. */
	uint32_t self_clear;
	/** Writable bits, 0 for all of them. */
	uint32_t write_mask;
	/** Bits cleared once the register has been read (data ready, interrupt status, ...). */
	uint32_t clear_on_read;
};

/** @cond INTERNAL_HIDDEN */
#define Z_EMUL_SENSOR_REG_DECLARE(_reg, _addr, ...) \
	enum { z_emul_sensor_reg_addr_##_reg = (_addr) };
#define Z_EMUL_SENSOR_REG_ENTRY(_reg, _addr, ...) \
	{ .addr = (_addr), .name = #_reg, __VA_ARGS__ },
/** @endcond */

/**
 * @brief Define a register table and its symbolic addresses from one list.
 *
 * Use at file scope. The list calls its argument once per register, passing a short name,
 * an address from 0 to 255, and optional emul_sensor_reg member initializers. Each name
 * becomes a log label and an internally prefixed address constant. Names must be unique
 * within the source file.
 *
 * @code{.c}
 * #define SENSOR_REGS(R) \
 *         R(CONFIG, 0x01, .reset = 0x20) \
 *         R(STATUS, 0x09, .flags = EMUL_SENSOR_REG_RO)
 *
 * EMUL_SENSOR_REG_TABLE_DEFINE(regs, SENSOR_REGS);
 * @endcode
 * A channel can refer to `.ready = { .reg = EMUL_SENSOR_REG_ADDR(STATUS), .mask = BIT(0) }`.
 * Pass the generated table to EMUL_SENSOR_REGMAP_DEFINE().
 *
 * @param _table Name of the static const emul_sensor_reg array to create.
 * @param _list Register list macro accepting one argument, as shown above.
 */
#define EMUL_SENSOR_REG_TABLE_DEFINE(_table, _list) \
	_list(Z_EMUL_SENSOR_REG_DECLARE) \
	static const struct emul_sensor_reg _table[] = { _list(Z_EMUL_SENSOR_REG_ENTRY) }

/**
 * @brief Get the integer address associated with a register's short name.
 *
 * @param _reg Short name in a list passed to EMUL_SENSOR_REG_TABLE_DEFINE().
 * @return Register address, usable in a constant initializer.
 */
#define EMUL_SENSOR_REG_ADDR(_reg) z_emul_sensor_reg_addr_##_reg

/**
 * @brief Identify bits in a register.
 *
 * A channel uses this pair to select a data format or to mark a sample as ready. For a
 * selector, the masked value is shifted down to bit 0 before it indexes the variants table.
 * For example, `{ .reg = 0x01, .mask = 0x0c }` selects using bits 3 and 2 of register 0x01.
 * For ready status, every bit in the mask is set when a sample is injected.
 */
struct emul_sensor_bits {
	/** Register address. */
	uint8_t reg;
	/** Bit mask; contiguous for a selector. Zero disables the field. */
	uint32_t mask;
};

/**
 * @brief Override a channel's sample format for one configuration value.
 *
 * Entries in @ref emul_sensor_channel::variants describe changes in resolution, bit position,
 * scale, or range. Zero @a bits, @a pos, or @a lsb inherits the corresponding channel value.
 * The bounds inherit together when both are zero; otherwise both replace the channel bounds.
 * Signedness and offset always come from the channel.
 *
 * For example, `.variants[1] = { .lsb = 0.5 }` changes only the scale when the selector is 1.
 */
struct emul_sensor_field {
	/** Raw code width, 1 to 32 bits (0 inherits in a variant). */
	uint8_t bits;
	/** Position of the least significant bit in the data word. */
	uint8_t pos;
	/** Positive, finite scale in Sensor API units per raw count; 0 inherits. */
	double lsb;
	/** Lower measurement bound, in Sensor API units. */
	double min;
	/** Upper measurement bound, in Sensor API units. */
	double max;
};

/**
 * @brief Map a Sensor API channel to its raw register representation.
 *
 * The sensor emulator backend converts an injected value to a raw code using
 * `(value - offset) / lsb`. It rounds to the nearest integer, clamps to the representable
 * range, and stores the code in @a bits bits starting at @a pos. Other bits are preserved.
 * Signed codes use two's complement. Values and bounds use the units of @a chan.
 *
 * The data word spans consecutive register addresses starting at @a reg. All registers in
 * that word must have the same width. The register map's byte order applies both within
 * each register and across the word. The field must fit within 64 bits.
 *
 * The example below stores relative humidity as an unsigned 12-bit value in bits 15 through 4.
 * SENSOR_CHAN_HUMIDITY uses percent, so `.lsb = 0.1` means 0.1 percentage point per count.
 * A reading of 50 percent becomes 500 counts, or 0x1f40 after shifting left by 4 bits:
 * @code{.c}
 * static const struct emul_sensor_channel channels[] = {
 *         { .chan = SENSOR_CHAN_HUMIDITY, .reg = 0x10,
 *           .bits = 12, .pos = 4, .lsb = 0.1, .min = 0.0, .max = 100.0 },
 * };
 * @endcode
 *
 * Set @a select to model a format that changes with a configuration register. Its field
 * value selects a @a variants entry by index (0 through 7). If the selector exceeds this
 * range, the last entry, `variants[7]`, is used. Nonzero variant members override the base
 * format as described by emul_sensor_field. A zero selector mask uses the base format alone.
 * After storing a sample, the backend sets @a ready bits, if configured.
 */
struct emul_sensor_channel {
	/** Scalar channel type, addressed with channel index 0 by the backend. */
	enum sensor_channel chan;
	/** Address of the first register of the data word. */
	uint8_t reg;
	/** Two's complement data. */
	bool is_signed;
	/** Raw code width, 1 to 32 bits. */
	uint8_t bits;
	/** Position of the least significant bit in the data word. */
	uint8_t pos;
	/** Positive, finite scale in Sensor API units per raw count. */
	double lsb;
	/** Sensor API value of a raw code of 0. */
	double offset;
	/** Lower bound in Sensor API units; both bounds 0 derive the range from the raw code. */
	double min;
	/** Upper measurement bound, in Sensor API units. */
	double max;
	/** Configuration field selecting the active variant, mask 0 for none. */
	struct emul_sensor_bits select;
	/** Field description per value of @a select. */
	struct emul_sensor_field variants[8];
	/** Status bits set when a new sample is written. */
	struct emul_sensor_bits ready;
};

/**
 * @brief Combine register and channel tables with the bus format.
 *
 * The register table defines valid addresses and their access rules. The channel table
 * tells the backend where to store injected samples. Bus accesses to undeclared addresses
 * fail with -EIO. Addresses advance by one after each complete register transfer.
 *
 * Tables and register names must remain valid for the emulator's lifetime. Usually they
 * are static const arrays passed to EMUL_SENSOR_REGMAP_DEFINE(), which creates this
 * description and separate mutable state for each enabled I2C instance:
 * @code{.c}
 * EMUL_SENSOR_REGMAP_DEFINE(regs, channels, .reg_bytes = 2, .big_endian = true);
 * @endcode
 *
 * Here each register holds two bytes unless its entry overrides the width. Set
 * @a addr_ignore to discard address flags, such as a burst-access bit, before lookup.
 * The bus uses a single address byte; the I2C device address comes from devicetree.
 */
struct emul_sensor_regmap {
	/** Register table, retained for the lifetime of the emulator. */
	const struct emul_sensor_reg *regs;
	/** Number of register entries. */
	size_t num_regs;
	/** Channel table, retained for the lifetime of the emulator. */
	const struct emul_sensor_channel *channels;
	/** Number of channel entries. */
	size_t num_channels;
	/** Default register width: 1 to 4 bytes, or 0 for 1 byte. */
	uint8_t reg_bytes;
	/** Most significant byte first on the bus. */
	bool big_endian;
	/** Bits of the register address byte that are not part of the address. */
	uint8_t addr_ignore;
};

/** @cond INTERNAL_HIDDEN */
struct emul_sensor_regmap_data {
	struct k_mutex lock;
	uint32_t regs[256];
	uint16_t ptr;
	uint8_t pos;
};

extern const struct i2c_emul_api emul_sensor_regmap_i2c_api;
extern const struct emul_sensor_driver_api emul_sensor_regmap_backend_api;

int emul_sensor_regmap_init(const struct emul *target, const struct device *parent);

#define Z_EMUL_SENSOR_REGMAP_DT_INST_DEFINE(inst, desc)                                            \
	static struct emul_sensor_regmap_data emul_sensor_regmap_data_##inst;                      \
	EMUL_DT_INST_DEFINE(inst, emul_sensor_regmap_init, &emul_sensor_regmap_data_##inst, &desc, \
			    &emul_sensor_regmap_i2c_api, &emul_sensor_regmap_backend_api)

/* Instances on other buses (SPI) are left without an emulator. */
#define Z_EMUL_SENSOR_REGMAP_DT_INST_I2C(inst, desc)                                               \
	IF_ENABLED(DT_INST_ON_BUS(inst, i2c),                                            \
		   (Z_EMUL_SENSOR_REGMAP_DT_INST_DEFINE(inst, desc);))
/** @endcond */

/**
 * @brief Define an emulator for every enabled I2C instance of `DT_DRV_COMPAT`
 *
 * @param _regs Array of @ref emul_sensor_reg
 * @param _channels Array of @ref emul_sensor_channel
 * @param ... Remaining @ref emul_sensor_regmap initializers, for example `.reg_bytes = 2`
 */
#define EMUL_SENSOR_REGMAP_DEFINE(_regs, _channels, ...)                                           \
	static const struct emul_sensor_regmap UTIL_CAT(emul_sensor_regmap_, DT_DRV_COMPAT) = {    \
		.regs = _regs,                                                                     \
		.num_regs = ARRAY_SIZE(_regs),                                                     \
		.channels = _channels,                                                             \
		.num_channels = ARRAY_SIZE(_channels),                                             \
		__VA_ARGS__};                                                                      \
	DT_INST_FOREACH_STATUS_OKAY_VARGS(Z_EMUL_SENSOR_REGMAP_DT_INST_I2C,                        \
					  UTIL_CAT(emul_sensor_regmap_, DT_DRV_COMPAT))

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_SENSOR_EMUL_SENSOR_REGMAP_H_ */
