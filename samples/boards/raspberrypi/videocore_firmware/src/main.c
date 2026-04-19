/*
 * Copyright (c) 2026 Muhammad Waleed Badar
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>

#include <rpi_fw.h>

static void rpi_fw_get_board_info(const struct device *dev)
{
	uint32_t revision = 0;
	int ret;

	static const char *const manufacturers[] = {"Sony UK", "Egoman",  "Embest", "Sony Japan",
						    "Embest",  "Stadium", "Unknown"};

	static const char *const processors[] = {"BCM2835", "BCM2836", "BCM2837", "BCM2711",
						 "Unknown"};

	static const char *const board_types[] = {
		[0x00] = "Model A",       [0x01] = "Model B",      [0x02] = "Model A+",
		[0x03] = "Model B+",      [0x04] = "Pi 2 Model B", [0x05] = "Alpha",
		[0x06] = "CM1",           [0x08] = "Pi 3 Model B", [0x09] = "Zero",
		[0x0a] = "CM3",           [0x0c] = "Zero W",       [0x0d] = "Pi 3 Model B+",
		[0x0e] = "Pi 3 Model A+", [0x10] = "CM3+",         [0x11] = "Pi 4 Model B",
		[0x13] = "Pi 400",        [0x14] = "CM4",          [0x15] = "CM4S",
	};

	static const char *const memory_sizes[] = {"256MB", "512MB", "1GB",    "2GB",
						   "4GB",   "8GB",   "Unknown"};

	/* Board revision */
	ret = rpi_fw_transfer(dev, RPI_FW_TAG_GET_BOARD_REVISION, &revision, sizeof(revision));
	if (ret < 0) {
		printk("Failed to get board revision: %d\n", ret);
		return;
	}

	/* Parse revision fields (new-style encoding) */
	uint32_t mem_idx = (revision >> 20) & 0x7;
	uint32_t mfr_idx = (revision >> 16) & 0xf;
	uint32_t proc_idx = (revision >> 12) & 0xf;
	uint32_t type_idx = (revision >> 4) & 0xff;
	uint32_t rev = revision & 0xf;

	const char *mem = mem_idx < ARRAY_SIZE(memory_sizes) ? memory_sizes[mem_idx] : "Unknown";
	const char *mfr = mfr_idx < ARRAY_SIZE(manufacturers) ? manufacturers[mfr_idx] : "Unknown";
	const char *proc = proc_idx < ARRAY_SIZE(processors) ? processors[proc_idx] : "Unknown";
	const char *type = (type_idx < ARRAY_SIZE(board_types) && board_types[type_idx] != NULL)
				   ? board_types[type_idx]
				   : "Unknown";

	printk("--- Raspberry Pi Board Info ---\n");
	printk("Board revision   : 0x%08x\n", revision);
	printk("Board type       : %s rev 1.%u\n", type, rev);
	printk("Processor        : %s\n", proc);
	printk("Memory           : %s\n", mem);
	printk("Manufacturer     : %s\n", mfr);
	printk("-------------------------------\n");
}

int main(void)
{
	const struct device *dev = DEVICE_DT_GET_ANY(raspberrypi_bcm2711_videocore);

	if (!device_is_ready(dev)) {
		printk("Raspberry Pi VideoCore firmware not found\n");
		return 0;
	}

	rpi_fw_get_board_info(dev);

	return 0;
}
