/*
 * Copyright (c) 2020 Intel Corporation
 * Copyright (c) 2021 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr/device.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/byteorder.h>

static inline bool is_ascii(uint8_t data)
{
	return (data >= 0x30 && data <= 0x39) || (data >= 0x61 && data <= 0x66) ||
	       (data >= 0x41 && data <= 0x46);
}

static unsigned char *bytes;
static uint32_t *data;
static int sum;
static int chunk_element;
static char chunk[2];
static bool littleendian;

#define CHAR_CAN 0x18
#define CHAR_DC1 0x11

static int set_bypass(const struct shell *sh, shell_bypass_cb_t bypass)
{
	static bool in_use;

	if (bypass && in_use) {
		shell_error(sh, "devmem load supports setting bypass on a single instance.");

		return -EBUSY;
	}

	in_use = !in_use;
	if (in_use) {
		shell_print(sh, "Loading...\npress ctrl-x ctrl-q to escape");
		in_use = true;
	}

	shell_set_bypass(sh, bypass);

	return 0;
}

static void bypass_cb(const struct shell *sh, uint8_t *recv, size_t len)
{
	bool escape = false;
	static uint8_t tail;
	uint8_t byte;

	if (tail == CHAR_CAN && recv[0] == CHAR_DC1) {
		escape = true;
	} else {
		for (int i = 0; i < (len - 1); i++) {
			if (recv[i] == CHAR_CAN && recv[i + 1] == CHAR_DC1) {
				escape = true;
				break;
			}
		}
	}

	if (escape) {
		shell_print(sh, "Number of bytes read: %d", sum);
		set_bypass(sh, NULL);

		if (!littleendian) {
			while (sum > 4) {
				*data = __bswap_32(*data);
				data++;
				sum = sum - 4;
			}
			if (sum % 4 == 0) {
				*data = __bswap_32(*data);
			} else if (sum % 4 == 2) {
				*data = __bswap_16(*data);
			} else if (sum % 4 == 3) {
				*data = __bswap_24(*data);
			}
		}
		return;
	}

	tail = recv[len - 1];

	if (is_ascii(*recv)) {
		chunk[chunk_element] = *recv;
		chunk_element++;
	}

	if (chunk_element == 2) {
		byte = (uint8_t)strtoul(chunk, NULL, 16);
		*bytes = byte;
		bytes++;
		sum++;
		chunk_element = 0;
	}
}

static int cmd_load(const struct shell *sh, size_t argc, char **argv)
{
	littleendian = false;
	char *arg;

	chunk_element = 0;
	sum = 0;

	while (argc >= 2) {
		arg = argv[1] + (!strncmp(argv[1], "--", 2) && argv[1][2]);
		if (!strncmp(arg, "-e", 2)) {
			littleendian = true;
		} else if (!strcmp(arg, "--")) {
			argv++;
			argc--;
			break;
		} else if (arg[0] == '-' && arg[1]) {
			shell_print(sh, "Unknown option \"%s\"", arg);
		} else {
			break;
		}
		argv++;
		argc--;
	}

	bytes = (unsigned char *)strtol(argv[1], NULL, 0);
	data = (uint32_t *)strtol(argv[1], NULL, 0);

	set_bypass(sh, bypass_cb);
	return 0;
}

static int memory_read(const struct shell *sh, mem_addr_t addr, uint8_t width)
{
	uint32_t value;
	int err = 0;

	switch (width) {
	case 8:
		value = sys_read8(addr);
		break;
	case 16:
		value = sys_read16(addr);
		break;
	case 32:
		value = sys_read32(addr);
		break;
	default:
		shell_fprintf(sh, SHELL_NORMAL, "Incorrect data width\n");
		err = -EINVAL;
		break;
	}

	if (err == 0) {
		shell_fprintf(sh, SHELL_NORMAL, "Read value 0x%x\n", value);
	}

	return err;
}

static int memory_write(const struct shell *sh, mem_addr_t addr, uint8_t width, uint64_t value)
{
	int err = 0;

	switch (width) {
	case 8:
		sys_write8(value, addr);
		break;
	case 16:
		sys_write16(value, addr);
		break;
	case 32:
		sys_write32(value, addr);
		break;
	default:
		shell_fprintf(sh, SHELL_NORMAL, "Incorrect data width\n");
		err = -EINVAL;
		break;
	}

	return err;
}

/* The syntax of the command is similar to busybox's devmem */
static int cmd_devmem(const struct shell *sh, size_t argc, char **argv)
{
	mem_addr_t phys_addr, addr;
	uint32_t value = 0;
	uint8_t width;

	if (argc < 2 || argc > 4) {
		return -EINVAL;
	}

	phys_addr = strtoul(argv[1], NULL, 16);

#if defined(CONFIG_MMU) || defined(CONFIG_PCIE)
	device_map((mm_reg_t *)&addr, phys_addr, 0x100, K_MEM_CACHE_NONE);

	shell_print(sh, "Mapped 0x%lx to 0x%lx\n", phys_addr, addr);
#else
	addr = phys_addr;
#endif /* defined(CONFIG_MMU) || defined(CONFIG_PCIE) */

	if (argc < 3) {
		width = 32;
	} else {
		width = strtoul(argv[2], NULL, 10);
	}

	shell_fprintf(sh, SHELL_NORMAL, "Using data width %d\n", width);

	if (argc <= 3) {
		return memory_read(sh, addr, width);
	}

	/* If there are more then 3 arguments, that means we are going to write
	 * this value at the address provided
	 */

	value = strtoul(argv[3], NULL, 16);

	shell_fprintf(sh, SHELL_NORMAL, "Writing value 0x%x\n", value);

	return memory_write(sh, addr, width, value);
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_devmem,
			       SHELL_CMD_ARG(load, NULL,
					     "Usage:\n"
					     "devmem load [options] [address]\n"
					     "Options:\n"
					     "-e\tlittle-endian parse",
					     cmd_load, 2, 1),
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(devmem, &sub_devmem,
		   "Read/write physical memory\n"
		   "devmem address [width [value]]",
		   cmd_devmem);
