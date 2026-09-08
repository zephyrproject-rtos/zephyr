/*
 * Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/ztest.h>
#include <zephyr/storage/disk_access.h>
#include <zephyr/usb/usbd.h>
#include <zephyr/usb/usbh.h>
#include <zephyr/usb/class/usbd_msc.h>
#include <zephyr/logging/log.h>
#include <sample_usbd.h>

#include <zephyr/usb/class/usbh_msc.h>

LOG_MODULE_REGISTER(usbh_msc_test, LOG_LEVEL_INF);

#define RAMDISK_SECTOR_SIZE  512U
#define RAMDISK_SECTOR_COUNT 192U

#define DISK_NAME "USB"

#define ATTACH_TIMEOUT       K_SECONDS(5)
#define ATTACH_POLL_INTERVAL K_MSEC(50)

USBD_DEFINE_MSC_LUN(ram, "RAM", "Zephyr", "RAMDisk", "0.00");

USBH_CONTROLLER_DEFINE(uhs_ctx, DEVICE_DT_GET(DT_NODELABEL(zephyr_uhc0)));

static struct usbh_msc_lun *test_lun;
static struct usbd_context *test_uds_ctx;

static struct usbh_msc_lun *wait_for_medium(void)
{
	const k_timepoint_t deadline = sys_timepoint_calc(ATTACH_TIMEOUT);

	while (!sys_timepoint_expired(deadline)) {
		struct usbh_msc_lun *lun = usbh_msc_lun_get(0);

		if (usbh_msc_is_ready(lun)) {
			return lun;
		}

		k_sleep(ATTACH_POLL_INTERVAL);
	}

	return NULL;
}

ZTEST(usbh_msc, test_capacity)
{
	uint32_t block_count;
	uint32_t block_size;

	zassert_not_null(test_lun, "No medium attached");
	zassert_ok(usbh_msc_get_capacity(test_lun, &block_count, &block_size));
	zassert_equal(block_size, RAMDISK_SECTOR_SIZE, "Unexpected block size %u", block_size);
	zassert_equal(block_count, RAMDISK_SECTOR_COUNT, "Unexpected block count %u", block_count);
	zassert_true(usbh_msc_is_ready(test_lun));
}

ZTEST(usbh_msc, test_write_read_single_block)
{
	static uint8_t wbuf[RAMDISK_SECTOR_SIZE];
	static uint8_t rbuf[RAMDISK_SECTOR_SIZE];

	zassert_not_null(test_lun, "No medium attached");

	for (size_t i = 0; i < sizeof(wbuf); i++) {
		wbuf[i] = (uint8_t)(i ^ 0x5aU);
	}

	zassert_ok(usbh_msc_write(test_lun, 1, 1, wbuf));

	memset(rbuf, 0, sizeof(rbuf));
	zassert_ok(usbh_msc_read(test_lun, 1, 1, rbuf));
	zassert_mem_equal(rbuf, wbuf, sizeof(wbuf), "Read back data differs");
}

/* A request larger than the UHC buffer pool has to be split into several
 * commands rather than rejected or truncated.
 */
ZTEST(usbh_msc, test_write_read_multi_block)
{
	static uint8_t wbuf[32 * RAMDISK_SECTOR_SIZE];
	static uint8_t rbuf[32 * RAMDISK_SECTOR_SIZE];
	const uint32_t count = ARRAY_SIZE(wbuf) / RAMDISK_SECTOR_SIZE;

	zassert_not_null(test_lun, "No medium attached");

	for (size_t i = 0; i < sizeof(wbuf); i++) {
		wbuf[i] = (uint8_t)(i * 7U);
	}

	zassert_ok(usbh_msc_write(test_lun, 8, count, wbuf));

	memset(rbuf, 0, sizeof(rbuf));
	zassert_ok(usbh_msc_read(test_lun, 8, count, rbuf));
	zassert_mem_equal(rbuf, wbuf, sizeof(wbuf), "Read back data differs");
}

/* Alternating write and read commands must not carry state between each
 * other: a stale CSW tag, an unconsumed residue or a short data stage would
 * show up as one round picking up the previous round's pattern.
 */
ZTEST(usbh_msc, test_alternating_write_read)
{
	static uint8_t wbuf[4 * RAMDISK_SECTOR_SIZE];
	static uint8_t rbuf[4 * RAMDISK_SECTOR_SIZE];
	const uint32_t count = ARRAY_SIZE(wbuf) / RAMDISK_SECTOR_SIZE;

	zassert_not_null(test_lun, "No medium attached");

	for (unsigned int round = 0; round < 4U; round++) {
		const uint32_t lba = 48U + (round * count);

		for (size_t i = 0; i < sizeof(wbuf); i++) {
			wbuf[i] = (uint8_t)(i + (round * 0x11U));
		}

		zassert_ok(usbh_msc_write(test_lun, lba, count, wbuf), "Write failed in round %u",
			   round);

		memset(rbuf, 0, sizeof(rbuf));
		zassert_ok(usbh_msc_read(test_lun, lba, count, rbuf), "Read failed in round %u",
			   round);
		zassert_mem_equal(rbuf, wbuf, sizeof(wbuf), "Read back data differs in round %u",
				  round);
	}

	/* Every round must still be intact once the last one is done, so a
	 * later command cannot have written over an earlier block.
	 */
	for (unsigned int round = 0; round < 4U; round++) {
		const uint32_t lba = 48U + (round * count);

		for (size_t i = 0; i < sizeof(wbuf); i++) {
			wbuf[i] = (uint8_t)(i + (round * 0x11U));
		}

		memset(rbuf, 0, sizeof(rbuf));
		zassert_ok(usbh_msc_read(test_lun, lba, count, rbuf), "Re-read failed for round %u",
			   round);
		zassert_mem_equal(rbuf, wbuf, sizeof(wbuf), "Round %u was overwritten", round);
	}
}

ZTEST(usbh_msc, test_range_is_checked)
{
	static uint8_t buf[RAMDISK_SECTOR_SIZE];
	uint32_t block_count;
	uint32_t block_size;

	zassert_not_null(test_lun, "No medium attached");
	zassert_ok(usbh_msc_get_capacity(test_lun, &block_count, &block_size));

	zassert_equal(usbh_msc_read(test_lun, block_count, 1, buf), -EINVAL,
		      "Read starting past the medium was accepted");
	zassert_equal(usbh_msc_read(test_lun, block_count - 1, 2, buf), -EINVAL,
		      "Read straddling the end was accepted");
	zassert_equal(usbh_msc_read(test_lun, 0, 0, buf), -EINVAL, "Empty read was accepted");
	zassert_equal(usbh_msc_read(test_lun, 0, block_count + 1, buf), -EINVAL,
		      "Oversized read was accepted");
}

/* A rejected request must not leave the transport out of step. */
ZTEST(usbh_msc, test_transport_survives_rejection)
{
	static uint8_t buf[RAMDISK_SECTOR_SIZE];
	uint32_t block_count;
	uint32_t block_size;

	zassert_not_null(test_lun, "No medium attached");
	zassert_ok(usbh_msc_get_capacity(test_lun, &block_count, &block_size));

	zassert_equal(usbh_msc_read(test_lun, block_count, 1, buf), -EINVAL);
	zassert_ok(usbh_msc_read(test_lun, 0, 1, buf), "Transport did not recover");
}

/* The disk access layer is what a file system is mounted on, so check it
 * reports the same medium the class driver does.
 */
ZTEST(usbh_msc, test_disk_access)
{
	static uint8_t buf[RAMDISK_SECTOR_SIZE];
	uint32_t sector_count;
	uint32_t sector_size;

	zassert_not_null(test_lun, "No medium attached");

	zassert_ok(disk_access_init(DISK_NAME));
	zassert_equal(disk_access_status(DISK_NAME), DISK_STATUS_OK);

	zassert_ok(disk_access_ioctl(DISK_NAME, DISK_IOCTL_GET_SECTOR_COUNT, &sector_count));
	zassert_equal(sector_count, RAMDISK_SECTOR_COUNT, "Unexpected sector count %u",
		      sector_count);

	zassert_ok(disk_access_ioctl(DISK_NAME, DISK_IOCTL_GET_SECTOR_SIZE, &sector_size));
	zassert_equal(sector_size, RAMDISK_SECTOR_SIZE, "Unexpected sector size %u", sector_size);

	zassert_ok(disk_access_read(DISK_NAME, buf, 0, 1));
}

ZTEST(usbh_msc, test_null_lun_is_rejected)
{
	static uint8_t buf[RAMDISK_SECTOR_SIZE];
	uint32_t block_count;
	uint32_t block_size;

	zassert_equal(usbh_msc_read(NULL, 0, 1, buf), -ENODEV);
	zassert_equal(usbh_msc_write(NULL, 0, 1, buf), -ENODEV);
	zassert_equal(usbh_msc_get_capacity(NULL, &block_count, &block_size), -ENODEV);
	zassert_false(usbh_msc_is_ready(NULL));
}

ZTEST(usbh_msc, test_null_capacity_output_is_rejected)
{
	uint32_t block_count;
	uint32_t block_size;

	zassert_not_null(test_lun, "No medium attached");

	zassert_equal(usbh_msc_get_capacity(test_lun, NULL, &block_size), -EINVAL);
	zassert_equal(usbh_msc_get_capacity(test_lun, &block_count, NULL), -EINVAL);
}

/* A detach has to release the controller resources the command in flight was
 * holding. The UHC transfer and buffer pools hold CONFIG_UHC_XFER_COUNT and
 * CONFIG_UHC_BUF_COUNT entries, so leaking one per cycle only shows up once
 * the cycles outnumber them.
 */
ZTEST(usbh_msc, test_detach_releases_resources)
{
	static uint8_t buf[RAMDISK_SECTOR_SIZE];

	zassert_not_null(test_lun, "No medium attached");

	for (unsigned int i = 0; i < 24U; i++) {
		zassert_ok(usbd_disable(test_uds_ctx), "Failed to detach on cycle %u", i);

		zassert_equal(usbh_msc_read(test_lun, 0, 1, buf), -ENODEV,
			      "Read was accepted while detached on cycle %u", i);

		zassert_ok(usbd_enable(test_uds_ctx), "Failed to re-attach on cycle %u", i);

		test_lun = wait_for_medium();
		zassert_not_null(test_lun, "Medium did not come back on cycle %u", i);

		zassert_ok(usbh_msc_read(test_lun, 0, 1, buf),
			   "Transfer pool exhausted after %u cycles", i);
	}
}

static void *usbh_msc_setup(void)
{
	int err;

	err = usbh_init(&uhs_ctx);
	zassert_ok(err, "Failed to initialize USB host support");

	err = usbh_enable(&uhs_ctx);
	zassert_ok(err, "Failed to enable USB host support");

	test_uds_ctx = sample_usbd_init_device(NULL);
	zassert_not_null(test_uds_ctx, "Failed to initialize USB device support");

	err = usbd_enable(test_uds_ctx);
	zassert_ok(err, "Failed to enable USB device support");

	test_lun = wait_for_medium();
	zassert_not_null(test_lun, "Medium did not become ready");

	return NULL;
}

ZTEST_SUITE(usbh_msc, NULL, usbh_msc_setup, NULL, NULL, NULL);
