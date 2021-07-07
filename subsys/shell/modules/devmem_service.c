/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <device.h>
#include <shell/shell.h>
#include <stdlib.h>

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
	default:
		shell_fprintf(sh, SHELL_NORMAL, "Incorrect data width\n");
		err = -EINVAL;
		break;
	}

	if (err == 0) {
		shell_fprintf(sh, SHELL_NORMAL, "Read value 0x%lx\n", value);
	}

	return err;
}

static int memory_write(const struct shell *sh, mem_addr_t addr,
			uint8_t width, uint64_t value)
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

	shell_fprintf(sh, SHELL_NORMAL, "Writing value 0x%lx\n", value);

	return memory_write(sh, addr, width, value);
}

SHELL_CMD_REGISTER(devmem, NULL, "Read/write physical memory\""
		   "devmem address [width [value]]", cmd_devmem);
