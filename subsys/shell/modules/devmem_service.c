/*
 * Copyright (c) 2020 Intel Corporation
 * Copyright (c) 2021 Antmicro <www.antmicro.com>
 * Copyright (c) 2022 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>
#include <zephyr/sys/sys_getopt.h>
#include <zephyr/device.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

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

static int memory_dump(const struct shell *sh, mem_addr_t phys_addr, size_t size, uint8_t width)
{
	uint64_t value;
	size_t data_offset;
	mm_reg_t addr;
	const size_t vsize = width / BITS_PER_BYTE;
	uint8_t hex_data[SHELL_HEXDUMP_BYTES_IN_LINE];

	switch (width) {
	case 8:
	case 16:
	case 32:
#ifdef CONFIG_64BIT
	case 64:
#endif
		break;
	default:
		shell_print(sh, "Incorrect data width: %u", width);
		return -EINVAL;
	}

#if defined(CONFIG_MMU) || defined(CONFIG_PCIE)
	device_map(&addr, phys_addr, size, K_MEM_CACHE_NONE);

	shell_print(sh, "Mapped 0x%lx to 0x%lx\n", phys_addr, addr);
#else
	addr = phys_addr;
#endif /* defined(CONFIG_MMU) || defined(CONFIG_PCIE) */

	for (; size > 0;
	     addr += SHELL_HEXDUMP_BYTES_IN_LINE, size -= MIN(size, SHELL_HEXDUMP_BYTES_IN_LINE)) {
		for (data_offset = 0;
		     size >= vsize && data_offset + vsize <= SHELL_HEXDUMP_BYTES_IN_LINE;
		     data_offset += vsize) {
			switch (width) {
			case 8:
				value = sys_read8(addr + data_offset);
				hex_data[data_offset] = value;
				break;
			case 16:
				value = sys_le16_to_cpu(sys_read16(addr + data_offset));
				sys_put_le16(value, &hex_data[data_offset]);
				break;
			case 32:
				value = sys_le32_to_cpu(sys_read32(addr + data_offset));
				sys_put_le32(value, &hex_data[data_offset]);
				break;
#ifdef CONFIG_64BIT
			case 64:
				value = sys_le64_to_cpu(sys_read64(addr + data_offset));
				sys_put_le64(value, &hex_data[data_offset]);
				break;
#endif /* CONFIG_64BIT */
			}
		}

		shell_hexdump_line(sh, addr, hex_data, MIN(size, SHELL_HEXDUMP_BYTES_IN_LINE));
	}

#if defined(CONFIG_MMU) || defined(CONFIG_PCIE)
	device_unmap(addr, size);
#endif

	return 0;
}

static int cmd_dump(const struct shell *sh, size_t argc, char **argv)
{
	int rv;
	int err = 0;
	size_t size = -1;
	size_t width = 32;
	mem_addr_t addr = -1;

	sys_getopt_optind = 1;
	sys_getopt_init();

	while ((rv = sys_getopt(argc, argv, "a:s:w:")) != -1) {
		switch (rv) {
		case 'a':
			addr = (mem_addr_t)shell_strtoul(sys_getopt_optarg, 16, &err);
			if (err != 0) {
				shell_error(sh, "invalid addr '%s'", sys_getopt_optarg);
				return -EINVAL;
			}
			break;
		case 's':
			size = (size_t)shell_strtoul(sys_getopt_optarg, 0, &err);
			if (err != 0) {
				shell_error(sh, "invalid size '%s'", sys_getopt_optarg);
				return -EINVAL;
			}
			break;
		case 'w':
			width = (size_t)shell_strtoul(sys_getopt_optarg, 0, &err);
			if (err != 0) {
				shell_error(sh, "invalid width '%s'", sys_getopt_optarg);
				return -EINVAL;
			}
			break;
		case '?':
		default:
			return -EINVAL;
		}
	}

	if (addr == -1) {
		shell_error(sh, "'-a <address>' is mandatory");
		return -EINVAL;
	}

	if (size == -1) {
		shell_error(sh, "'-s <size>' is mandatory");
		return -EINVAL;
	}

	return memory_dump(sh, addr, size, width);
}

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

	shell_set_bypass(sh, bypass, NULL);

	return 0;
}

static void bypass_cb(const struct shell *sh, uint8_t *recv, size_t len, void *user_data)
{
	bool escape = false;
	static uint8_t tail;
	uint8_t byte;

	ARG_UNUSED(user_data);

	for (size_t i = 0; i < len; i++) {
		if (tail == CHAR_CAN && recv[i] == CHAR_DC1) {
			escape = true;
			tail = 0;
			break;
		}
		tail = recv[i];

		if (is_ascii(recv[i])) {
			chunk[chunk_element] = recv[i];
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

	if (escape) {
		shell_print(sh, "Number of bytes read: %d", sum);
		set_bypass(sh, NULL);

		if (!littleendian) {
			while (sum > 4) {
				*data = BSWAP_32(*data);
				data++;
				sum = sum - 4;
			}
			if (sum % 4 == 0) {
				*data = BSWAP_32(*data);
			} else if (sum % 4 == 2) {
				*data = BSWAP_16(*data);
			} else if (sum % 4 == 3) {
				*data = BSWAP_24(*data);
			}
		}
		return;
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

	bytes = (unsigned char *)strtoul(argv[1], NULL, 0);
	data = (uint32_t *)strtoul(argv[1], NULL, 0);

	set_bypass(sh, bypass_cb);
	return 0;
}

static int memory_read(const struct shell *sh, mem_addr_t addr, uint8_t width)
{
	uint64_t value;
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
#ifdef CONFIG_64BIT
	case 64:
		value = sys_read64(addr);
		break;
#endif /* CONFIG_64BIT */
	default:
		shell_print(sh, "Incorrect data width");
		err = -EINVAL;
		break;
	}

	if (err == 0) {
		shell_print(sh, "Read value 0x%llx", value);
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
#ifdef CONFIG_64BIT
	case 64:
		sys_write64(value, addr);
		break;
#endif /* CONFIG_64BIT */
	default:
		shell_print(sh, "Incorrect data width");
		err = -EINVAL;
		break;
	}

	return err;
}

/* The syntax of the command is similar to busybox's devmem */
static int cmd_devmem(const struct shell *sh, size_t argc, char **argv)
{
	mem_addr_t phys_addr, addr;
	uint64_t value = 0;
	uint8_t width;

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

	shell_print(sh, "Using data width %d", width);

	if (argc <= 3) {
		return memory_read(sh, addr, width);
	}

	/* If there are more then 3 arguments, that means we are going to write
	 * this value at the address provided
	 */

	value = (uint64_t)strtoull(argv[3], NULL, 16);

	shell_print(sh, "Writing value 0x%llx", value);

	return memory_write(sh, addr, width, value);
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_devmem,
			       SHELL_CMD_ARG(dump, NULL,
					     "Usage:\n"
					     "devmem dump -a <address> -s <size> [-w <width>]\n",
					     cmd_dump, 5, 2),
			       SHELL_CMD_ARG(load, NULL,
					     "Usage:\n"
					     "devmem load [options] [address]\n"
					     "Options:\n"
					     "-e\tlittle-endian parse",
					     cmd_load, 2, 1),
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_ARG_REGISTER(devmem, &sub_devmem,
		   "Read/write physical memory\n"
		   "Usage:\n"
		   "Read memory at address with optional width:\n"
		   "devmem <address> [<width>]\n"
		   "Write memory at address with mandatory width and value:\n"
		   "devmem <address> <width> <value>",
		   cmd_devmem, 2, 2);
