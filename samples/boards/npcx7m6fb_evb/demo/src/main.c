/*
 * Copyright (c) 2020 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>
#include <drivers/uart.h>
#include <shell/shell.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <linker/linker-defs.h>

void main(void)
{
	const struct device *uart_dev;
	int ret;


	/* Get uart_dev device */
	uart_dev = device_get_binding("UART_1");
	if (uart_dev == NULL) {
		printk("Error: didn't find %x device\n", uart_dev);
		return;
	}
	printk("Start the shell test!!!");
}


/******************************************************************************/
/* Shell command functions */

/* Memory test command help texts form Chromium EC */
#define DUMP_CMD_HELP   "md [.b|.h|.s] addr [count]\n" \
	"Dump memory values, optionally specifying the format"
#define RW_CMD_HELP     "rw [.b|.h] addr [value]\n" \
	"Read or write a word in memory optionally specifying the size"

enum format {
	FMT_WORD,
	FMT_HALF,
	FMT_BYTE,
	FMT_STRING,
};

static void show_val(const struct shell *shell, uint32_t address,
					uint32_t index, enum format fmt)
{
	uint32_t val;
	uintptr_t ptr = address;

	switch (fmt) {
	case FMT_WORD:
		if (0 == (index % 4))
			shell_print(shell, "\n%08X:", address + index * 4);
		val = *((uint32_t *)ptr + index);
		shell_print(shell, " %08x", val);
		break;
	case FMT_HALF:
		if (0 == (index % 8))
			shell_print(shell, "\n%08X:", address + index * 2);
		val = *((uint16_t *)ptr + index);
		shell_print(shell, " %04x", val);
		break;
	case FMT_BYTE:
		if (0 == (index % 16))
			shell_print(shell, "\n%08X:", address + index);
		val = *((uint8_t *)ptr + index);
		shell_print(shell, " %02x", val);
		break;
	case FMT_STRING:
		if (0 == (index % 32))
			shell_print(shell, "\n%08X: ", address + index);
		val = *((uint8_t *)ptr + index);
		if (val >= ' ' && val <= '~')
			shell_print(shell, "%c", val);
		else
			shell_print(shell, "\\x%02x", val);
		break;
	}
}

static int cmd_mem_dump(const struct shell *shell, size_t argc, char *argv[])
{
	uint32_t address, i, num = 1;
	char *e;
	enum format fmt = FMT_WORD;

	if (argc > 1) {
		if ((argv[1][0] == '.') && (strlen(argv[1]) == 2)) {
			switch (argv[1][1]) {
			case 'b':
				fmt = FMT_BYTE;
				break;
			case 'h':
				fmt = FMT_HALF;
				break;
			case 's':
				fmt = FMT_STRING;
				break;
			default:
				return -EINVAL;
			}
			argc--;
			argv++;
		}
	}

	if (argc < 2)
		return -EINVAL;

	address = strtol(argv[1], &e, 0);
	if (*e)
		return -EINVAL;

	if (argc >= 3)
		num = strtol(argv[2], &e, 0);

	for (i = 0; i < num; i++) {
		show_val(shell, address, i, fmt);
	}
	shell_print(shell, "\n");
	return 0;
}

static int cmd_read_word(const struct shell *shell, size_t argc, char *argv[])
{
	volatile uint32_t *address;
	uint32_t value;
	unsigned access_size = 4;
	unsigned argc_offs = 0;
	char *e;

	if (argc < 2)
		return -EINVAL;

	if (argc > 2) {
		if ((argv[1][0] == '.') && (strlen(argv[1]) == 2)) {
			argc_offs = 1;
			switch (argv[1][1]) {
			case 'b':
				access_size = 1;
				break;
			case 'h':
				access_size = 2;
				break;
			default:
				return -EINVAL;
			}
		}
	}
	address = (uint32_t *)(uintptr_t)strtol(argv[1 + argc_offs], &e, 0);
	if (*e)
		return -EINVAL;

	/* Just reading? */
	if ((argc - argc_offs) < 3) {
		switch (access_size) {
		case 1:
			shell_print(shell, "read 0x%pP = 0x%02x\n",
				 address, *((uint8_t *)address));
			break;
		case 2:
			shell_print(shell, "read 0x%pP = 0x%04x\n",
				 address, *((uint16_t *)address));
			break;

		default:
			shell_print(shell, "read 0x%pP = 0x%08x\n",  address,
					*address);
			break;
		}
		return 0;
	}

	/* Writing! */
	value = strtol(argv[2 + argc_offs], &e, 0);
	if (*e)
		return -EINVAL;

	switch (access_size) {
	case 1:
		shell_print(shell, "write 0x%pP = 0x%02x\n", address,
			(uint8_t)value);
		*((uint8_t *)address) = (uint8_t)value;
		break;
	case 2:
		shell_print(shell, "write 0x%pP = 0x%04x\n", address,
				(uint16_t)value);
		*((uint16_t *)address) = (uint16_t)value;
		break;
	default:
		shell_print(shell, "write 0x%pP = 0x%02x\n", address, value);
		*address = value;
		break;
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_mem,
	SHELL_CMD(md, NULL, DUMP_CMD_HELP, cmd_mem_dump),
	SHELL_CMD(rw, NULL, RW_CMD_HELP, cmd_read_word),
	SHELL_SUBCMD_SET_END /* Array terminated. */
);
SHELL_CMD_REGISTER(mem, &sub_mem, "Memory related commands.", NULL);
