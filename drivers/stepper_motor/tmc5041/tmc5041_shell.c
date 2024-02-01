/*
 * SPDX-FileCopyrightText: Copyright (c) 2023 Carl Zeiss Meditec AG
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/stepper_motor_controller.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/util.h>

#include <stdlib.h>
#include <string.h>

#include "zephyr/drivers/stepper_motor/tmc5041.h"
#include "tmc5041_shell_helper.h"

static const struct tmc_motor_number_t tmc_motor_map[] = {TMC_SHELL_MOTOR_NUMBER("X", 0),
							  TMC_SHELL_MOTOR_NUMBER("Y", 1)};

static const struct tmc_map_t tmc5013_map[] = {
	TMC_SHELL_REG_MAPPING("GCONF", TMC5041_GCONF, READ_WRITE, 11),
	TMC_SHELL_REG_MAPPING("GSTAT", TMC5041_GSTAT, READ_CLEAR, 4),
	TMC_SHELL_REG_MAPPING("INPUT", TMC5041_INPUT, READ, 32),
	TMC_SHELL_REG_MAPPING("X_COMPARE", TMC5041_X_COMPARE, WRITE, 32),
	TMC_SHELL_REG_MAPPING("PWMCONF_MOTOR_1", TMC5041_PWMCONF(0), WRITE, 22),
	TMC_SHELL_REG_MAPPING("PWMCONF_MOTOR_2", TMC5041_PWMCONF(1), WRITE, 22),
	TMC_SHELL_REG_MAPPING("PWM_STATUS_MOTOR_1", TMC5041_PWM_STATUS(0), WRITE, 22),
	TMC_SHELL_REG_MAPPING("PWM_STATUS_MOTOR_2", TMC5041_PWM_STATUS(1), WRITE, 22),
	TMC_SHELL_REG_MAPPING("RAMPMODE_MOTOR_1", TMC5041_RAMPMODE(0), READ_WRITE, 2),
	TMC_SHELL_REG_MAPPING("RAMPMODE_MOTOR_2", TMC5041_RAMPMODE(1), READ_WRITE, 2),
	TMC_SHELL_REG_MAPPING("XACTUAL_MOTOR_1", TMC5041_XACTUAL(0), READ_WRITE, 32),
	TMC_SHELL_REG_MAPPING("XACTUAL_MOTOR_2", TMC5041_XACTUAL(1), READ_WRITE, 32),
	TMC_SHELL_REG_MAPPING("VACTUAL_MOTOR_1", TMC5041_VACTUAL(0), READ, 32),
	TMC_SHELL_REG_MAPPING("VACTUAL_MOTOR_2", TMC5041_VACTUAL(1), READ, 32),
	TMC_SHELL_REG_MAPPING("VACTUAL_MOTOR_1", TMC5041_VACTUAL(0), READ, 32),
	TMC_SHELL_REG_MAPPING("VACTUAL_MOTOR_2", TMC5041_VACTUAL(1), READ, 32),
	TMC_SHELL_REG_MAPPING("VSTART_MOTOR_1", TMC5041_VSTART(0), WRITE, 18),
	TMC_SHELL_REG_MAPPING("VSTART_MOTOR_2", TMC5041_VSTART(1), WRITE, 18),
	TMC_SHELL_REG_MAPPING("A1_MOTOR_1", TMC5041_A1(0), WRITE, 16),
	TMC_SHELL_REG_MAPPING("A1_MOTOR_2", TMC5041_A1(1), WRITE, 16),
	TMC_SHELL_REG_MAPPING("V1_MOTOR_1", TMC5041_V1(0), WRITE, 20),
	TMC_SHELL_REG_MAPPING("V1_MOTOR_2", TMC5041_V1(1), WRITE, 20),
	TMC_SHELL_REG_MAPPING("AMAX_MOTOR_1", TMC5041_AMAX(0), WRITE, 16),
	TMC_SHELL_REG_MAPPING("AMAX_MOTOR_2", TMC5041_AMAX(1), WRITE, 16),
	TMC_SHELL_REG_MAPPING("VMAX_MOTOR_1", TMC5041_VMAX(0), WRITE, 23),
	TMC_SHELL_REG_MAPPING("VMAX_MOTOR_2", TMC5041_VMAX(1), WRITE, 23),
	TMC_SHELL_REG_MAPPING("DMAX_MOTOR_1", TMC5041_DMAX(0), WRITE, 16),
	TMC_SHELL_REG_MAPPING("DMAX_MOTOR_2", TMC5041_DMAX(1), WRITE, 16),
	TMC_SHELL_REG_MAPPING("D1_MOTOR_1", TMC5041_D1(0), WRITE, 16),
	TMC_SHELL_REG_MAPPING("D1_MOTOR_2", TMC5041_D1(1), WRITE, 16),
	TMC_SHELL_REG_MAPPING("VSTOP_MOTOR_1", TMC5041_VSTOP(0), WRITE, 18),
	TMC_SHELL_REG_MAPPING("VSTOP_MOTOR_2", TMC5041_VSTOP(1), WRITE, 18),
	TMC_SHELL_REG_MAPPING("TZEROWAIT_MOTOR_1", TMC5041_TZEROWAIT(0), WRITE, 16),
	TMC_SHELL_REG_MAPPING("TZEROWAIT_MOTOR_2", TMC5041_TZEROWAIT(1), WRITE, 16),
	TMC_SHELL_REG_MAPPING("XTARGET_MOTOR_1", TMC5041_XTARGET(0), WRITE, 32),
	TMC_SHELL_REG_MAPPING("XTARGET_MOTOR_2", TMC5041_XTARGET(1), WRITE, 32),
	TMC_SHELL_REG_MAPPING("IHOLD_IRUN_MOTOR_1", TMC5041_IHOLD_IRUN(0), WRITE, 14),
	TMC_SHELL_REG_MAPPING("IHOLD_IRUN_MOTOR_2", TMC5041_IHOLD_IRUN(1), WRITE, 14),
	TMC_SHELL_REG_MAPPING("VCOOLTHRS_MOTOR_1", TMC5041_VCOOLTHRS(0), WRITE, 23),
	TMC_SHELL_REG_MAPPING("VCOOLTHRS_MOTOR_2", TMC5041_VCOOLTHRS(1), WRITE, 23),
	TMC_SHELL_REG_MAPPING("VHIGH_MOTOR_1", TMC5041_VHIGH(0), WRITE, 23),
	TMC_SHELL_REG_MAPPING("VHIGH_MOTOR_2", TMC5041_VHIGH(1), WRITE, 23),
	TMC_SHELL_REG_MAPPING("SW_MODE_MOTOR_1", TMC5041_SWMODE(0), WRITE, 12),
	TMC_SHELL_REG_MAPPING("SW_MODE_MOTOR_2", TMC5041_SWMODE(1), WRITE, 12),
	TMC_SHELL_REG_MAPPING("RAMPSTAT_MOTOR_1", TMC5041_RAMPSTAT(0), WRITE, 12),
	TMC_SHELL_REG_MAPPING("RAMPSTAT_MOTOR_2", TMC5041_RAMPSTAT(1), WRITE, 12),
	TMC_SHELL_REG_MAPPING("XLATCH_MOTOR_1", TMC5041_XLATCH(0), WRITE, 12),
	TMC_SHELL_REG_MAPPING("XLATCH_MOTOR_2", TMC5041_XLATCH(1), WRITE, 12),
	TMC_SHELL_REG_MAPPING("MSLUT0_MOTOR_1", TMC5041_MSLUT0(0), WRITE, 32),
	TMC_SHELL_REG_MAPPING("MSLUT0_MOTOR_2", TMC5041_MSLUT0(1), WRITE, 32),
	TMC_SHELL_REG_MAPPING("MSLUT1_MOTOR_1", TMC5041_MSLUT1(0), WRITE, 32),
	TMC_SHELL_REG_MAPPING("MSLUT1_MOTOR_2", TMC5041_MSLUT1(1), WRITE, 32),
	TMC_SHELL_REG_MAPPING("MSLUT2_MOTOR_1", TMC5041_MSLUT2(0), WRITE, 32),
	TMC_SHELL_REG_MAPPING("MSLUT2_MOTOR_2", TMC5041_MSLUT2(1), WRITE, 32),
	TMC_SHELL_REG_MAPPING("MSLUT3_MOTOR_1", TMC5041_MSLUT3(0), WRITE, 32),
	TMC_SHELL_REG_MAPPING("MSLUT3_MOTOR_2", TMC5041_MSLUT3(1), WRITE, 32),
	TMC_SHELL_REG_MAPPING("MSLUT4_MOTOR_1", TMC5041_MSLUT4(0), WRITE, 32),
	TMC_SHELL_REG_MAPPING("MSLUT4_MOTOR_2", TMC5041_MSLUT4(1), WRITE, 32),
	TMC_SHELL_REG_MAPPING("MSLUT5_MOTOR_1", TMC5041_MSLUT5(0), WRITE, 32),
	TMC_SHELL_REG_MAPPING("MSLUT5_MOTOR_2", TMC5041_MSLUT5(1), WRITE, 32),
	TMC_SHELL_REG_MAPPING("MSLUT6_MOTOR_1", TMC5041_MSLUT6(0), WRITE, 32),
	TMC_SHELL_REG_MAPPING("MSLUT6_MOTOR_2", TMC5041_MSLUT6(1), WRITE, 32),
	TMC_SHELL_REG_MAPPING("MSLUT7_MOTOR_1", TMC5041_MSLUT7(0), WRITE, 32),
	TMC_SHELL_REG_MAPPING("MSLUT7_MOTOR_2", TMC5041_MSLUT7(1), WRITE, 32),
	TMC_SHELL_REG_MAPPING("MSLUTSEL_MOTOR_1", TMC5041_MSLUTSEL(0), WRITE, 32),
	TMC_SHELL_REG_MAPPING("MSLUTSEL_MOTOR_2", TMC5041_MSLUTSEL(1), WRITE, 32),
	TMC_SHELL_REG_MAPPING("MSLUTSTART_MOTOR_1", TMC5041_MSLUTSTART(0), WRITE, 32),
	TMC_SHELL_REG_MAPPING("MSLUTSTART_MOTOR_2", TMC5041_MSLUTSTART(1), WRITE, 32),
	TMC_SHELL_REG_MAPPING("MSCNT_MOTOR_1", TMC5041_MSCNT(0), WRITE, 10),
	TMC_SHELL_REG_MAPPING("MSCNT_MOTOR_2", TMC5041_MSCNT(1), WRITE, 10),
	TMC_SHELL_REG_MAPPING("MSCURACT_MOTOR_1", TMC5041_MSCURACT(0), WRITE, 18),
	TMC_SHELL_REG_MAPPING("MSCURACT_MOTOR_2", TMC5041_MSCURACT(1), WRITE, 18),
	TMC_SHELL_REG_MAPPING("CHOPCONF_MOTOR_1", TMC5041_CHOPCONF(0), WRITE, 32),
	TMC_SHELL_REG_MAPPING("CHOPCONF_MOTOR_2", TMC5041_CHOPCONF(1), WRITE, 32),
	TMC_SHELL_REG_MAPPING("COOLCONF_MOTOR_1", TMC5041_COOLCONF(0), WRITE, 25),
	TMC_SHELL_REG_MAPPING("COOLCONF_MOTOR_2", TMC5041_COOLCONF(1), WRITE, 25),
	TMC_SHELL_REG_MAPPING("DRVSTATUS_MOTOR_1", TMC5041_DRVSTATUS(0), WRITE, 32),
	TMC_SHELL_REG_MAPPING("DRVSTATUS_MOTOR_2", TMC5041_DRVSTATUS(1), WRITE, 32),
};

static int trinamic_reg_write_wrapper(const struct shell *shell_instance,
				      const struct device *tmc_device, int register_address,
				      int register_value)
{
	int status =
		stepper_motor_controller_write_reg(tmc_device, register_address, register_value);
	if (status < 0) {
		shell_error(shell_instance, "failed to write register (status %d)", status);
		return status;
	}
	shell_print(shell_instance, "write success: reg <0x%02x> value:      0x%x",
		    register_address, register_value);
	return 0;
}

static int cmd_tmc_read(const struct shell *shell_instance, size_t argc, char **argv)
{
	ARG_UNUSED(argc);

	const struct device *tmc_device;
	uint8_t register_address;
	uint8_t width;

	uint32_t register_value;
	bool register_found = false;
	int err;
	size_t idx_map;

	tmc_device = device_get_binding(argv[1]);
	if (tmc_device == NULL) {
		shell_error(shell_instance, "Device unknown (%s)", argv[1]);
		return -ENODEV;
	}

	/* Lookup symbolic register name */
	for (idx_map = 0U; idx_map < ARRAY_SIZE(tmc5013_map); idx_map++) {
		if (strcmp(argv[2], tmc5013_map[idx_map].name) == 0) {
			register_address = tmc5013_map[idx_map].address;
			width = tmc5013_map[idx_map].register_width;
			register_found = true;
			break;
		}
	}

	if (!register_found) {
		shell_error(shell_instance, "failed to parse register address");
		return -EINVAL;
	}

	err = stepper_motor_controller_read_reg(tmc_device, register_address, &register_value);
	if (err < 0) {
		shell_error(shell_instance, "failed to read register (err %d)", err);
		return err;
	}

	shell_print(shell_instance, "reg <0x%02x> value:      0x%lx", register_address,
		    (BIT_MASK(width) & register_value));

	return 0;
}

static int cmd_tmc_write(const struct shell *shell_instance, size_t argc, char **argv)
{
	ARG_UNUSED(argc);

	const struct device *tmc_device;
	uint8_t register_address;
	enum tmc_register_type reg_type;
	char *end_ptr;

	uint32_t register_value;
	bool register_found = false;
	int status;
	size_t idx_map;

	tmc_device = device_get_binding(argv[1]);
	if (tmc_device == NULL) {
		shell_error(shell_instance, "Device unknown (%s)", argv[1]);
		return -ENODEV;
	}

	/* Lookup symbolic register name */
	for (idx_map = 0U; idx_map < ARRAY_SIZE(tmc5013_map); idx_map++) {
		if (strcmp(argv[2], tmc5013_map[idx_map].name) == 0) {
			register_address = tmc5013_map[idx_map].address;
			reg_type = tmc5013_map[idx_map].register_type;
			register_found = true;
			break;
		}
	}

	if (!register_found) {
		shell_error(shell_instance, "failed to parse register address");
		return -EINVAL;
	}

	if ((reg_type == READ) || (reg_type == READ_CLEAR)) {
		shell_error(shell_instance, "error: attempting to write into a read only register");
		return -EINVAL;
	}

	register_value = (uint32_t)strtoul(argv[3], &end_ptr, 16);
	if (*end_ptr != '\0') {
		shell_error(shell_instance, "failed to parse write value");
		return -EINVAL;
	}

	status = stepper_motor_controller_write_reg(tmc_device, register_address, register_value);
	if (status < 0) {
		shell_error(shell_instance, "failed to write register (status %d)", status);
		return status;
	}
	shell_print(shell_instance, "write success: reg <0x%02x> value:      0x%x",
		    register_address, register_value);

	return 0;
}

static int cmd_tmc_default(const struct shell *shell_instance, size_t argc, char **argv, void *data)
{
	ARG_UNUSED(argc);

	const struct device *tmc_device;
	int register_address, register_value;
	int motor_number = 0;

	for (uint8_t idx_motor = 0U; idx_motor < ARRAY_SIZE(tmc_motor_map); idx_motor++) {
		if (strcmp(argv[2], tmc_motor_map[idx_motor].name) == 0) {
			motor_number = tmc_motor_map[idx_motor].number;
			break;
		}
	}

	shell_print(shell_instance, "default motor %d", motor_number);

	tmc_device = device_get_binding(argv[1]);
	if (tmc_device == NULL) {
		shell_error(shell_instance, "Device unknown (%s)", argv[1]);
		return -ENODEV;
	}

	register_address = TMC5041_GCONF;
	register_value = 0x8;
	trinamic_reg_write_wrapper(shell_instance, tmc_device, register_address, register_value);

	register_address = TMC5041_CHOPCONF(motor_number);
	register_value = 0x100C5;
	trinamic_reg_write_wrapper(shell_instance, tmc_device, register_address, register_value);

	register_address = TMC5041_IHOLD_IRUN(motor_number);
	register_value = 0x11F05;
	trinamic_reg_write_wrapper(shell_instance, tmc_device, register_address, register_value);

	register_address = TMC5041_TZEROWAIT(motor_number);
	register_value = 0x2710;
	trinamic_reg_write_wrapper(shell_instance, tmc_device, register_address, register_value);

	register_address = TMC5041_PWMCONF(motor_number);
	register_value = 0x401C8;
	trinamic_reg_write_wrapper(shell_instance, tmc_device, register_address, register_value);

	register_address = TMC5041_VHIGH(motor_number);
	register_value = 0x61A80;
	trinamic_reg_write_wrapper(shell_instance, tmc_device, register_address, register_value);

	register_address = TMC5041_VCOOLTHRS(motor_number);
	register_value = 0x7530;
	trinamic_reg_write_wrapper(shell_instance, tmc_device, register_address, register_value);

	register_address = TMC5041_A1(motor_number);
	register_value = 0x3E8;
	trinamic_reg_write_wrapper(shell_instance, tmc_device, register_address, register_value);

	register_address = TMC5041_V1(motor_number);
	register_value = 0xC350;
	trinamic_reg_write_wrapper(shell_instance, tmc_device, register_address, register_value);

	register_address = TMC5041_AMAX(motor_number);
	register_value = 0x1F4;
	trinamic_reg_write_wrapper(shell_instance, tmc_device, register_address, register_value);

	register_address = TMC5041_DMAX(motor_number);
	register_value = 0x2BC;
	trinamic_reg_write_wrapper(shell_instance, tmc_device, register_address, register_value);

	register_address = TMC5041_VMAX(motor_number);
	register_value = 0x304D0;
	trinamic_reg_write_wrapper(shell_instance, tmc_device, register_address, register_value);

	register_address = TMC5041_D1(motor_number);
	register_value = 0x578;
	trinamic_reg_write_wrapper(shell_instance, tmc_device, register_address, register_value);

	register_address = TMC5041_VSTOP(motor_number);
	register_value = 0x10;
	trinamic_reg_write_wrapper(shell_instance, tmc_device, register_address, register_value);

	register_address = TMC5041_RAMPMODE(motor_number);
	register_value = 0x00;
	trinamic_reg_write_wrapper(shell_instance, tmc_device, register_address, register_value);

	return 0;
}

static void cmd_tmc_register(size_t idx, struct shell_static_entry *entry);

SHELL_DYNAMIC_CMD_CREATE(dcmd_tmc_register, cmd_tmc_register);

static void cmd_tmc_register(size_t idx, struct shell_static_entry *entry)
{
	if (idx < ARRAY_SIZE(tmc5013_map)) {
		entry->syntax = tmc5013_map[idx].name;

	} else {
		entry->syntax = NULL;
	}

	entry->handler = NULL;
	entry->help = "Lists the registers.";
	entry->subcmd = NULL;
}

static void cmd_tmc_device_name_register(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_lookup(idx, NULL);

	entry->syntax = (dev != NULL) ? dev->name : NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = &dcmd_tmc_register;
}

SHELL_DYNAMIC_CMD_CREATE(dsub_tmc_device_name_register, cmd_tmc_device_name_register);

static void cmd_tmc_motor_number(size_t idx, struct shell_static_entry *entry)
{
	if (idx < ARRAY_SIZE(tmc_motor_map)) {
		entry->syntax = tmc_motor_map[idx].name;

	} else {
		entry->syntax = NULL;
	}

	entry->handler = NULL;
	entry->help = "Lists the Motors.";
	entry->subcmd = NULL;
}

SHELL_DYNAMIC_CMD_CREATE(dcmd_tmc_motor_number, cmd_tmc_motor_number);

static void cmd_tmc_device_name_motor_number(size_t idx, struct shell_static_entry *entry)
{
	const struct device *dev = shell_device_lookup(idx, NULL);

	entry->syntax = (dev != NULL) ? dev->name : NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = &dcmd_tmc_motor_number;
}

SHELL_DYNAMIC_CMD_CREATE(dsub_tmc_device_name_motor_number, cmd_tmc_device_name_motor_number);

SHELL_STATIC_SUBCMD_SET_CREATE(
	sub_trinamic_cmds,
	SHELL_CMD_ARG(read, &dsub_tmc_device_name_register,
		      "Read register values\n"
		      "Usage: trinamic read <device> <reg_addr>",
		      cmd_tmc_read, 3, 0),

	SHELL_CMD_ARG(write, &dsub_tmc_device_name_register,
		      "Write value into register\n"
		      "Usage: trinamic write <device> <reg_addr> <value>",
		      cmd_tmc_write, 4, 0),
	SHELL_CMD_ARG(default, &dsub_tmc_device_name_motor_number,
		      "Default Setup in order to test if motors are working\n"
		      "Usage: trinamic default <device> <motor name>\n"
		      "Preconfigured Values are taken from page 72 of datasheet\n"
		      "https://www.trinamic.com/fileadmin/assets/Products/ICs_Documents/"
		      "TMC5041_datasheet.pdf",
		      cmd_tmc_default, 3, 0),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(tmc5041, &sub_trinamic_cmds, "Trinamic motor controller commands", NULL);
