/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <drivers/flash.h>
#include <device.h>
#include <devicetree.h>
#include <stdio.h>
#include <string.h>

#if (CONFIG_SPI_NOR - 0) ||				\
	DT_NODE_HAS_STATUS(DT_INST(0, jedec_spi_nor), okay)
#define FLASH_DEVICE DT_LABEL(DT_INST(0, jedec_spi_nor))
#define FLASH_NAME "JEDEC SPI-NOR"
#elif (CONFIG_NORDIC_QSPI_NOR - 0) || \
	DT_NODE_HAS_STATUS(DT_INST(0, nordic_qspi_nor), okay)
#define FLASH_DEVICE DT_LABEL(DT_INST(0, nordic_qspi_nor))
#define FLASH_NAME "JEDEC QSPI-NOR"
#else
#error Unsupported flash driver
#endif

#if defined(CONFIG_BOARD_ADAFRUIT_FEATHER_STM32F405)
#define FLASH_TEST_REGION_OFFSET 0xf000
#elif defined(CONFIG_BOARD_ARTY_A7_ARM_DESIGNSTART_M1) || \
	defined(CONFIG_BOARD_ARTY_A7_ARM_DESIGNSTART_M3)
/* The FPGA bitstream is stored in the lower 536 sectors of the flash. */
#define FLASH_TEST_REGION_OFFSET \
	DT_REG_SIZE(DT_NODE_BY_FIXED_PARTITION_LABEL(fpga_bitstream))
#else
#define FLASH_TEST_REGION_OFFSET 0xff000
#endif
#define FLASH_SECTOR_SIZE        4096

#if 0

extern const uint8_t __device_handles_start[];
extern const uint8_t __device_handles_end[];

static void dump_handles(const char *tag,
			 const device_handle_t *cp)
{
	const device_handle_t *cps = cp;
	unsigned int setnum = 0;
	char buf[128] = {0};
	char *bp = buf;
	const struct device *devlist;
	size_t devcnt = z_device_get_all_static(&devlist);

	printk("handles sets for %s: %p:\n", tag, cp);
	while (true) {
		if ((*cp == DEVICE_HANDLE_SEP)
		    || (*cp == DEVICE_HANDLE_ENDS)) {
			printk("\tS%u: %u elts:%s\n", setnum, (unsigned int)(cp - cps), buf);
			bp = buf;
			*bp = 0;
			cps = cp + 1;
			++setnum;
			if (*cp == DEVICE_HANDLE_ENDS) {
				break;
			}
		} else {
			bp += snprintf(bp, buf + sizeof(buf) -  bp, " %d", *cp);
			if (setnum == 0) {
				bp += snprintf(bp, buf + sizeof(buf) - bp, "[%s]",
					       devlist[*cp].name);
			}
		}
		++cp;
	}
	printk("%s has %u sets\n", tag, setnum);
}

static void showhandles(void)
{
	const struct device *devlist;
	size_t devcnt = z_device_get_all_static(&devlist);
	const struct device *dpe = devlist + devcnt;
	const struct device *dp = devlist;

	printk("%zu devices at %p:\n", devcnt, devlist);
	printk("%u bytes handles at %p\n",
	       __device_handles_end - __device_handles_start,
	       __device_handles_start);
	while (dp < dpe) {
		const char *name = dp->name ? dp->name : "<?>";

		printk("%u: %p: %s\n", (unsigned int)(dp - devlist), dp, name);
		if (dp->handles) {
			dump_handles(name, dp->handles);
		}
		++dp;
	}
}
#else
static void showhandles(void)
{
	const struct device *devlist;
	size_t devcnt = z_device_get_all_static(&devlist);
	const struct device *dpe = devlist + devcnt;
	const struct device *dp = devlist;

	printk("%zu devices at %p:\n", devcnt, devlist);

	while (dp < dpe) {
		const char *name = dp->name ? dp->name : "<?>";
		size_t ndh = 0;
		const device_handle_t *dhp = device_get_requires_handles(dp, &ndh);

		printk("%u: %p: %s:", device_get_handle(dp), dp, name);
		if (ndh > 0) {
			size_t di = 0;

			while (di < ndh) {
				device_handle_t dh = dhp[di];
				const struct device *rdp = device_from_handle(dh);

				printk("\n\tdep %u: %p: %s",
				       dh, rdp, rdp->name ? rdp->name : "<?>");
				++di;
			}
			printk("\n");
		} else {
			printk(" no device dependencies\n");
		}
		++dp;
	}
}
#endif

void main(void)
{
	const uint8_t expected[] = { 0x55, 0xaa, 0x66, 0x99 };
	const size_t len = sizeof(expected);
	uint8_t buf[sizeof(expected)];
	const struct device *flash_dev;
	int rc;

	printf("\n" FLASH_NAME " SPI flash testing\n");
	printf("==========================\n");

	showhandles();

	flash_dev = device_get_binding(FLASH_DEVICE);

	if (!flash_dev) {
		printf("SPI flash driver %s was not found!\n",
		       FLASH_DEVICE);
		return;
	}

	/* Write protection needs to be disabled before each write or
	 * erase, since the flash component turns on write protection
	 * automatically after completion of write and erase
	 * operations.
	 */
	printf("\nTest 1: Flash erase\n");
	flash_write_protection_set(flash_dev, false);

	rc = flash_erase(flash_dev, FLASH_TEST_REGION_OFFSET,
			 FLASH_SECTOR_SIZE);
	if (rc != 0) {
		printf("Flash erase failed! %d\n", rc);
	} else {
		printf("Flash erase succeeded!\n");
	}

	printf("\nTest 2: Flash write\n");
	flash_write_protection_set(flash_dev, false);

	printf("Attempting to write %u bytes\n", len);
	rc = flash_write(flash_dev, FLASH_TEST_REGION_OFFSET, expected, len);
	if (rc != 0) {
		printf("Flash write failed! %d\n", rc);
		return;
	}

	memset(buf, 0, len);
	rc = flash_read(flash_dev, FLASH_TEST_REGION_OFFSET, buf, len);
	if (rc != 0) {
		printf("Flash read failed! %d\n", rc);
		return;
	}

	if (memcmp(expected, buf, len) == 0) {
		printf("Data read matches data written. Good!!\n");
	} else {
		const uint8_t *wp = expected;
		const uint8_t *rp = buf;
		const uint8_t *rpe = rp + len;

		printf("Data read does not match data written!!\n");
		while (rp < rpe) {
			printf("%08x wrote %02x read %02x %s\n",
			       (uint32_t)(FLASH_TEST_REGION_OFFSET + (rp - buf)),
			       *wp, *rp, (*rp == *wp) ? "match" : "MISMATCH");
			++rp;
			++wp;
		}
	}
}
