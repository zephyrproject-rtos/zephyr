/*
 * Copyright (c) 2017 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdlib.h>

#include <zephyr.h>
#include <flash.h>
#include <misc/printk.h>
#include <shell/shell.h>

/* Assumption: our devices have less than 64MB of memory */
#define RESERVED_MEM_MAP (CONFIG_SRAM_BASE_ADDRESS + 0x4000000)
#define FLASH_MEM         CONFIG_FLASH_BASE_ADDRESS
#define RAM_MEM           CONFIG_SRAM_BASE_ADDRESS

/* MPU test command help texts */
#define READ_CMD_HELP  "Read from a reserved address in the memory map"
#define WRITE_CMD_HELP "Write in to boot FLASH/ROM"
#define RUN_CMD_HELP   "Run code located in RAM"
#define MTEST_CMD_HELP "Memory Test writes or reads a memory location"

static int shell_cmd_read(int argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	u32_t *p_mem = (u32_t *) RESERVED_MEM_MAP;

	/* Reads from an address that is reserved in the memory map */
	printk("The value is: %d", *p_mem);

	return 0;
}

#if defined(CONFIG_SOC_FLASH_MCUX)
static int shell_cmd_write_mcux(int argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	struct device *flash_dev;
	flash_dev = device_get_binding(CONFIG_SOC_FLASH_MCUX_DEV_NAME);

	u32_t value[2];

	/* 128K reserved to the application */
	u32_t offset = FLASH_MEM + 0x20000;
	value[0] = 0xBADC0DE;
	value[1] = 0xBADC0DE;

	printk("write address: 0x%x\n", offset);

	flash_write_protection_set(flash_dev, false);

	if (flash_write(flash_dev, offset, value,
				sizeof(value)) != 0) {
		printk("Flash write failed!\n");
		return 1;
	}

	flash_write_protection_set(flash_dev, true);

	return 0;

}
#elif defined(CONFIG_SOC_FLASH_STM32)
static int shell_cmd_write_stm32(int argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	struct device *flash_dev;
	flash_dev = device_get_binding(CONFIG_SOC_FLASH_STM32_DEV_NAME);

	/* 16K reserved to the application */
	u32_t offset = FLASH_MEM + 0x4000;
	u32_t value = 0xBADC0DE;

	printk("write address: 0x%x\n", offset);

	flash_write_protection_set(flash_dev, false);

	if (flash_write(flash_dev, offset, &value,
				sizeof(value)) != 0) {
		printk("Flash write failed!\n");
		return 1;
	}

	flash_write_protection_set(flash_dev, true);

	return 0;
}
#else
static int shell_cmd_write(int argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	/* 16K reserved to the application */
	u32_t *p_mem = (u32_t *) (FLASH_MEM + 0x4000);

	printk("write address: 0x%x\n", FLASH_MEM + 0x4000);

	/* Write in to boot FLASH/ROM */
	*p_mem = 0xBADC0DE;

	return 0;
}
#endif /* SOC_FLASH_MCUX */

static int shell_cmd_run(int argc, char *argv[])
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	void (*func_ptr)(void) = (void (*)(void)) RAM_MEM;

	/* Run code located in RAM */
	func_ptr();

	return 0;
}

static int shell_cmd_mtest(int argc, char *argv[])
{
	if (argc > 3) {
		printk("mtest accepts 1 (Read) or 2 (Write) parameters\n");
		return 1;
	}

	u32_t val = (u32_t)strtol(argv[1], NULL, 16);

	u32_t *mem = (u32_t *) val;

	if (argc == 2) {
		printk("The value is: 0x%x\n", *mem);
	} else {
		*mem = (u32_t) strtol(argv[2], NULL, 16);
	}

	return 0;
}

#define SHELL_MODULE "mpu_test"

static struct shell_cmd commands[] = {
	{ "read", shell_cmd_read, READ_CMD_HELP},
#if defined(CONFIG_SOC_FLASH_MCUX)
	{ "write", shell_cmd_write_mcux, WRITE_CMD_HELP },
#elif defined(CONFIG_SOC_FLASH_STM32)
	{ "write", shell_cmd_write_stm32, WRITE_CMD_HELP },
#else
	{ "write", shell_cmd_write, WRITE_CMD_HELP },
#endif /* SOC_FLASH_MCUX*/
	{ "run", shell_cmd_run, RUN_CMD_HELP },
	{ "mtest", shell_cmd_mtest, MTEST_CMD_HELP },
	{ NULL, NULL, NULL }
};

void main(void)
{
	SHELL_REGISTER(SHELL_MODULE, commands);
}
