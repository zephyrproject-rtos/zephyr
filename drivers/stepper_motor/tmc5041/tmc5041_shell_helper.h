/*
 * SPDX-FileCopyrightText: Copyright (c) 2023 Carl Zeiss Meditec AG
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_TMC_SHELL_HELPER_H
#define ZEPHYR_DRIVERS_TMC_SHELL_HELPER_H

enum tmc_register_type {
	READ,
	WRITE,
	READ_WRITE,
	READ_CLEAR
};

struct tmc_map_t {
	const char *name;
	uint8_t address;
	enum tmc_register_type register_type;
	uint8_t register_width;
};

struct tmc_motor_number_t {
	const char *name;
	uint8_t number;
};

#define TMC_SHELL_REG_MAPPING(_name, _reg, _reg_typ, reg_width)                                    \
	{                                                                                          \
		.name = _name, .address = _reg, .register_type = _reg_typ,                         \
		.register_width = reg_width                                                        \
	}

#define TMC_SHELL_MOTOR_NUMBER(_name, _number)                                                     \
	{                                                                                          \
		.name = _name, .number = _number                                                   \
	}
#endif /* ZEPHYR_DRIVERS_TMC_SHELL_HELPER_H */
