/*
 * Copyright (c) 2017 Linaro Limited.
 * Copyright (c) 2018 Nordic Semiconductor.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdlib.h>

#include <zephyr.h>
#include <drivers/flash.h>
#include <shell/shell.h>

#define PR_SHELL(shell, fmt, ...)				\
	shell_fprintf(shell, SHELL_NORMAL, fmt, ##__VA_ARGS__)
#define PR_ERROR(shell, fmt, ...)				\
	shell_fprintf(shell, SHELL_ERROR, fmt, ##__VA_ARGS__)

/* Assumption: our devices have less than 64MB of memory */
#define RESERVED_MEM_MAP (CONFIG_SRAM_BASE_ADDRESS + 0x4000000)
#define FLASH_MEM         CONFIG_FLASH_BASE_ADDRESS
#define RAM_MEM           CONFIG_SRAM_BASE_ADDRESS

/* MPU test command help texts */
#define READ_CMD_HELP  "Read from a reserved address in the memory map"
#define WRITE_CMD_HELP "Write in to boot FLASH/ROM"
#define RUN_CMD_HELP   "Run code located in RAM"
#define MTEST_CMD_HELP "Memory Test writes or reads a memory location.\n"    \
			"Command accepts 1 (Read) or 2 (Write) parameters."

static int cmd_read(const struct shell *shell, size_t argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	u32_t *p_mem = (u32_t *) RESERVED_MEM_MAP;

	/* Reads from an address that is reserved in the memory map */
	PR_SHELL(shell, "The value is: %d\n", *p_mem);

	return 0;
}

#if defined(CONFIG_SOC_FLASH_MCUX)
static int cmd_write_mcux(const struct shell *shell, size_t argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	struct device *flash_dev;
	u32_t value[2];
	u32_t offset;

	flash_dev = device_get_binding(DT_FLASH_DEV_NAME);

	/* 128K reserved to the application */
	offset = FLASH_MEM + 0x20000;
	value[0] = 0xBADC0DE;
	value[1] = 0xBADC0DE;

	PR_SHELL(shell, "write address: 0x%x\n", offset);

	flash_write_protection_set(flash_dev, false);

	if (flash_write(flash_dev, offset, value,
				sizeof(value)) != 0) {
		PR_ERROR(shell, "Flash write failed!\n");
		return 1;
	}

	flash_write_protection_set(flash_dev, true);

	return 0;

}
#elif defined(CONFIG_SOC_FLASH_STM32)
static int cmd_write_stm32(const struct shell *shell, size_t argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	struct device *flash_dev;

	flash_dev = device_get_binding(DT_FLASH_DEV_NAME);

	/* 16K reserved to the application */
	u32_t offset = FLASH_MEM + 0x4000;
	u32_t value = 0xBADC0DE;

	PR_SHELL(shell, "write address: 0x%x\n", offset);

	flash_write_protection_set(flash_dev, false);

	if (flash_write(flash_dev, offset, &value, sizeof(value)) != 0) {
		PR_ERROR(shell, "Flash write failed!\n");
		return 1;
	}

	flash_write_protection_set(flash_dev, true);

	return 0;
}
#else
static int cmd_write(const struct shell *shell, size_t argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	/* 16K reserved to the application */
	u32_t *p_mem = (u32_t *) (FLASH_MEM + 0x4000);

	PR_SHELL(shell, "write address: 0x%x\n", FLASH_MEM + 0x4000);

	/* Write in to boot FLASH/ROM */
	*p_mem = 0xBADC0DE;

	return 0;
}
#endif /* SOC_FLASH_MCUX */

static int cmd_run(const struct shell *shell, size_t argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	void (*func_ptr)(void) = (void (*)(void)) RAM_MEM;

	/* Run code located in RAM */
	func_ptr();

	return 0;
}

static int cmd_mtest(const struct shell *shell, size_t argc, char *argv[])
{
	u32_t *mem;
	u32_t val;

	val = (u32_t)strtol(argv[1], NULL, 16);
	mem = (u32_t *) val;

	if (argc == 2) {
		PR_SHELL(shell, "The value is: 0x%x\n", *mem);
	} else {
		*mem = (u32_t) strtol(argv[2], NULL, 16);
	}

	return 0;
}

void main(void)
{

}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_mpu,
	SHELL_CMD_ARG(mtest, NULL, MTEST_CMD_HELP, cmd_mtest, 2, 1),
	SHELL_CMD(read, NULL, READ_CMD_HELP, cmd_read),
	SHELL_CMD(run, NULL, RUN_CMD_HELP, cmd_run),
#if defined(CONFIG_SOC_FLASH_MCUX)
	SHELL_CMD(write, NULL, WRITE_CMD_HELP, cmd_write_mcux),
#elif defined(CONFIG_SOC_FLASH_STM32)
	SHELL_CMD(write, NULL, WRITE_CMD_HELP, cmd_write_stm32),
#else
	SHELL_CMD(write, NULL, WRITE_CMD_HELP, cmd_write),
#endif /* SOC_FLASH_MCUX*/
	SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(mpu, &sub_mpu, "MPU related commands.", NULL);


