/*
 * Copyright (c) 2022-2025 Macronix International Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <stdio.h>
#include <string.h>

#define SPI_NAND_TEST_REGION_OFFSET 0x40000
#define SPI_NAND_BLOCK_SIZE         0x20000U
#define SPI_NAND_PAGE_SIZE          0x800

#if defined(CONFIG_FLASH_STM32_OSPI) || defined(CONFIG_FLASH_STM32_QSPI) ||                        \
	defined(CONFIG_FLASH_STM32_XSPI)
#define SPI_NAND_MULTI_SECTOR_TEST
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(jedec_spi_nand)
#define SPI_NAND_COMPAT jedec_spi_nand
#else
#define SPI_NAND_COMPAT invalid
#endif

void single_sector_test(const struct device *flash_dev)
{
	const size_t len = SPI_NAND_PAGE_SIZE;
	uint8_t buf[SPI_NAND_PAGE_SIZE];
	int rc;
	uint8_t *buf_wr = (uint8_t * )malloc (SPI_NAND_PAGE_SIZE);
	uint8_t *erased = (uint8_t * )malloc (SPI_NAND_PAGE_SIZE);

	memset (erased, 0xff, SPI_NAND_PAGE_SIZE);
	for (int n = 0; n < SPI_NAND_PAGE_SIZE; n++) {
		buf_wr[n] = rand() % 0x100;
	}

	printf("\nPerform test on single sector");
	/* Write protection needs to be disabled before each write or
	 * erase, since the flash component turns on write protection
	 * automatically after completion of write and erase
	 * operations.
	 */
	printf("\nTest 1: Flash erase\n");

	/* Full flash erase if SPI_NAND_TEST_REGION_OFFSET = 0 and
	 * SPI_NAND_BLOCK_SIZE = flash size
	 */
	rc = flash_erase(flash_dev, SPI_NAND_TEST_REGION_OFFSET, SPI_NAND_BLOCK_SIZE);
	if (rc != 0) {
		printf("Flash erase failed! %d\n", rc);
	} else {
		/* Check erased pattern */
		memset(buf, 0, len);
		rc = flash_read(flash_dev, SPI_NAND_TEST_REGION_OFFSET, buf, len);
		if (rc != 0) {
			printf("Flash read failed! %d\n", rc);
			return;
		}
		if (memcmp(erased, buf, len) != 0) {
			printf("Flash erase failed at offset 0x%x got 0x%x\n",
			       SPI_NAND_TEST_REGION_OFFSET, *(uint32_t *)buf);
			return;
		}
		printf("Flash erase succeeded!\n");
	}
	printf("\nTest 2: Flash write\n");

	printf("Attempting to write %zu bytes\n", len);
	rc = flash_write(flash_dev, SPI_NAND_TEST_REGION_OFFSET, buf_wr, len);
	if (rc != 0) {
		printf("Flash write failed! %d\n", rc);
		return;
	}

	memset(buf, 0, len);
	rc = flash_read(flash_dev, SPI_NAND_TEST_REGION_OFFSET, buf, len);
	if (rc != 0) {
		printf("Flash read failed! %d\n", rc);
		return;
	}

	if (memcmp(buf_wr, buf, len) == 0) {
		printf("Data read matches data written. Good!!\n");
	} else {
		const uint8_t *wp = buf_wr;
		const uint8_t *rp = buf;
		const uint8_t *rpe = rp + len;

		printf("Data read does not match data written!!\n");
		while (rp < rpe) {
			printf("%08x wrote %02x read %02x %s\n",
			       (uint32_t)(SPI_NAND_TEST_REGION_OFFSET + (rp - buf)), *wp, *rp,
			       (*rp == *wp) ? "match" : "MISMATCH");
			++rp;
			++wp;
		}
	}
}

#if defined SPI_NAND_MULTI_SECTOR_TEST
void multi_sector_test(const struct device *flash_dev)
{
	const size_t len = sizeof(SPI_NAND_PAGE_SIZE);
	uint8_t buf[SPI_NAND_PAGE_SIZE];
	int rc;
	uint8_t *buf_wr = (uint8_t * )malloc (SPI_NAND_PAGE_SIZE);
	uint8_t *erased = (uint8_t * )malloc (SPI_NAND_PAGE_SIZE);

	memset (erased, 0xff, SPI_NAND_PAGE_SIZE);
	for (int n = 0; n < SPI_NAND_PAGE_SIZE; n++) {
		buf_wr[n] = rand() % 0x100;
	}
	printf("\nPerform test on multiple consecutive sectors");

	/* Write protection needs to be disabled before each write or
	 * erase, since the flash component turns on write protection
	 * automatically after completion of write and erase
	 * operations.
	 */
	printf("\nTest 1: Flash erase\n");

	/* Full flash erase if SPI_NAND_TEST_REGION_OFFSET = 0 and
	 * SPI_NAND_BLOCK_SIZE = flash size
	 * Erase 2 sectors for check for erase of consequtive sectors
	 */
	rc = flash_erase(flash_dev, SPI_NAND_TEST_REGION_OFFSET, SPI_NAND_BLOCK_SIZE * 2);
	if (rc != 0) {
		printf("Flash erase failed! %d\n", rc);
	} else {
		/* Read the content and check for erased */
		memset(buf, 0, len);
		size_t offs = SPI_NAND_TEST_REGION_OFFSET;

		while (offs < SPI_NAND_TEST_REGION_OFFSET + 2 * SPI_NAND_BLOCK_SIZE) {
			rc = flash_read(flash_dev, offs, buf, len);
			if (rc != 0) {
				printf("Flash read failed! %d\n", rc);
				return;
			}
			if (memcmp(erased, buf, len) != 0) {
				printf("Flash erase failed at offset 0x%x got 0x%x\n", offs,
				       *(uint32_t *)buf);
				return;
			}
			offs += SPI_NAND_BLOCK_SIZE;
		}
		printf("Flash erase succeeded!\n");
	}

	printf("\nTest 2: Flash write\n");

	size_t offs = SPI_NAND_TEST_REGION_OFFSET;

	while (offs < SPI_NAND_TEST_REGION_OFFSET + 2 * SPI_NAND_BLOCK_SIZE) {
		printf("Attempting to write %zu bytes at offset 0x%x\n", len, offs);
		rc = flash_write(flash_dev, offs, buf_wr, len);
		if (rc != 0) {
			printf("Flash write failed! %d\n", rc);
			return;
		}

		memset(buf, 0, len);
		rc = flash_read(flash_dev, offs, buf, len);
		if (rc != 0) {
			printf("Flash read failed! %d\n", rc);
			return;
		}

		if (memcmp(buf_wr, buf, len) == 0) {
			printf("Data read matches data written. Good!!\n");
		} else {
			const uint8_t *wp = buf_wr;
			const uint8_t *rp = buf;
			const uint8_t *rpe = rp + len;

			printf("Data read does not match data written!!\n");
			while (rp < rpe) {
				printf("%08x wrote %02x read %02x %s\n",
				       (uint32_t)(offs + (rp - buf)), *wp, *rp,
				       (*rp == *wp) ? "match" : "MISMATCH");
				++rp;
				++wp;
			}
		}
		offs += SPI_NAND_BLOCK_SIZE;
	}
}
#endif

int main(void)
{
	const struct device *flash_dev = DEVICE_DT_GET_ONE(SPI_NAND_COMPAT);

	if (!device_is_ready(flash_dev)) {
		printk("%s: device not ready.\n", flash_dev->name);
		return 0;
	}

	printf("\n%s SPI NAND flash testing\n", flash_dev->name);
	printf("==========================\n");

	single_sector_test(flash_dev);
#if defined SPI_NAND_MULTI_SECTOR_TEST
	multi_sector_test(flash_dev);
#endif
	return 0;
}
