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

#define DETACH_CYCLES 24U

#define READER_STACK_SIZE 2048
#define READER_PRIORITY   K_PRIO_PREEMPT(5)

USBD_DEFINE_MSC_LUN(ram, "RAM", "Zephyr", "RAMDisk", "0.00");

#if !defined(CONFIG_USB_MSC_DISK_INITIALIZE_AT_BOOT)
USBH_CONTROLLER_DEFINE(uhs_ctx, DEVICE_DT_GET(DT_NODELABEL(zephyr_uhc0)));
#endif

static struct usbh_msc_lun *test_lun;
static struct usbd_context *test_uds_ctx;

K_THREAD_STACK_DEFINE(reader_stack, READER_STACK_SIZE);
static struct k_thread reader_thread;

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

static bool wait_for_detach(void)
{
	const k_timepoint_t deadline = sys_timepoint_calc(ATTACH_TIMEOUT);

	while (!sys_timepoint_expired(deadline)) {
		if (usbh_msc_lun_get(0) == NULL) {
			return true;
		}

		k_sleep(ATTACH_POLL_INTERVAL);
	}

	return false;
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

	zassert_equal(usbh_msc_write(test_lun, block_count, 1, buf), -EINVAL,
		      "Write starting past the medium was accepted");
	zassert_equal(usbh_msc_write(test_lun, block_count - 1, 2, buf), -EINVAL,
		      "Write straddling the end was accepted");
	zassert_equal(usbh_msc_write(test_lun, 0, 0, buf), -EINVAL, "Empty write was accepted");
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

ZTEST(usbh_msc, test_disk_access_write_sync)
{
	static uint8_t wbuf[2 * RAMDISK_SECTOR_SIZE];
	static uint8_t rbuf[2 * RAMDISK_SECTOR_SIZE];

	zassert_not_null(test_lun, "No medium attached");
	zassert_ok(disk_access_init(DISK_NAME));

	for (size_t i = 0; i < sizeof(wbuf); i++) {
		wbuf[i] = (uint8_t)(i ^ 0xa5U);
	}

	zassert_ok(disk_access_write(DISK_NAME, wbuf, 100, 2));
	zassert_ok(disk_access_ioctl(DISK_NAME, DISK_IOCTL_CTRL_SYNC, NULL));

	memset(rbuf, 0, sizeof(rbuf));
	zassert_ok(disk_access_read(DISK_NAME, rbuf, 100, 2));
	zassert_mem_equal(rbuf, wbuf, sizeof(wbuf), "Read back data differs");
	zassert_false(usbh_msc_is_write_protected(test_lun));
}

ZTEST(usbh_msc, test_null_lun_is_rejected)
{
	static uint8_t buf[RAMDISK_SECTOR_SIZE];
	uint32_t block_count;
	uint32_t block_size;

	zassert_equal(usbh_msc_read(NULL, 0, 1, buf), -ENODEV);
	zassert_equal(usbh_msc_write(NULL, 0, 1, buf), -ENODEV);
	zassert_equal(usbh_msc_sync(NULL), -ENODEV);
	zassert_equal(usbh_msc_get_capacity(NULL, &block_count, &block_size), -ENODEV);
	zassert_false(usbh_msc_is_ready(NULL));
	zassert_false(usbh_msc_is_write_protected(NULL));
	zassert_equal(usbh_msc_get_generation(NULL), 0U);
}

ZTEST(usbh_msc, test_null_capacity_output_is_rejected)
{
	uint32_t block_count;
	uint32_t block_size;

	zassert_not_null(test_lun, "No medium attached");

	zassert_equal(usbh_msc_get_capacity(test_lun, NULL, &block_size), -EINVAL);
	zassert_equal(usbh_msc_get_capacity(test_lun, &block_count, NULL), -EINVAL);
}

/* Repeated detach cycles exhaust the UHC transfer and buffer pools if a
 * cycle leaks an entry.
 */
ZTEST(usbh_msc, test_detach_releases_resources)
{
	static uint8_t buf[RAMDISK_SECTOR_SIZE];

	zassert_not_null(test_lun, "No medium attached");

	for (unsigned int i = 0; i < DETACH_CYCLES; i++) {
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

static atomic_t reads_done;
static int reader_result;

static void reader_entry(void *p1, void *p2, void *p3)
{
	static uint8_t buf[32 * RAMDISK_SECTOR_SIZE];
	struct usbh_msc_lun *lun = p1;
	const uint32_t count = ARRAY_SIZE(buf) / RAMDISK_SECTOR_SIZE;

	ARG_UNUSED(p2);
	ARG_UNUSED(p3);

	do {
		reader_result = usbh_msc_read(lun, 0, count, buf);
		(void)atomic_inc(&reads_done);
	} while (reader_result == 0);
}

ZTEST(usbh_msc, test_detach_with_command_in_flight)
{
	static uint8_t buf[RAMDISK_SECTOR_SIZE];

	zassert_not_null(test_lun, "No medium attached");

	for (unsigned int i = 0; i < DETACH_CYCLES; i++) {
		atomic_clear(&reads_done);

		k_thread_create(&reader_thread, reader_stack, K_THREAD_STACK_SIZEOF(reader_stack),
				reader_entry, test_lun, NULL, NULL, READER_PRIORITY, 0, K_NO_WAIT);

		while (atomic_get(&reads_done) == 0) {
			k_sleep(K_MSEC(1));
		}

		k_sleep(K_USEC(500U * (i % 4U)));
		zassert_ok(usbd_disable(test_uds_ctx), "Failed to detach on cycle %u", i);

		zassert_ok(k_thread_join(&reader_thread, K_SECONDS(15)),
			   "Reader stuck after detach on cycle %u", i);
		zassert_equal(reader_result, -ENODEV, "Reader got %d on cycle %u", reader_result,
			      i);

		zassert_ok(usbd_enable(test_uds_ctx), "Failed to re-attach on cycle %u", i);

		test_lun = wait_for_medium();
		zassert_not_null(test_lun, "Medium did not come back on cycle %u", i);

		zassert_ok(usbh_msc_read(test_lun, 0, 1, buf),
			   "Transfer pool exhausted after %u cycles", i);
	}
}

ZTEST(usbh_msc, test_reattach_changes_generation)
{
	uint32_t generation;

	zassert_not_null(test_lun, "No medium attached");
	zassert_ok(disk_access_init(DISK_NAME));
	zassert_equal(disk_access_status(DISK_NAME), DISK_STATUS_OK);

	generation = usbh_msc_get_generation(test_lun);
	zassert_not_equal(generation, 0U);

	zassert_ok(usbd_disable(test_uds_ctx));
	zassert_true(wait_for_detach(), "Device was not removed");
	zassert_equal(usbh_msc_get_generation(test_lun), 0U);
	zassert_equal(disk_access_status(DISK_NAME), DISK_STATUS_NOMEDIA);

	zassert_ok(usbd_enable(test_uds_ctx));
	test_lun = wait_for_medium();
	zassert_not_null(test_lun, "Medium did not come back");

	zassert_not_equal(usbh_msc_get_generation(test_lun), generation);
	zassert_equal(disk_access_status(DISK_NAME), DISK_STATUS_UNINIT,
		      "Changed medium was not reported");
	zassert_equal(disk_access_status(DISK_NAME), DISK_STATUS_UNINIT,
		      "Changed medium was reported only once");

	zassert_ok(disk_access_ioctl(DISK_NAME, DISK_IOCTL_CTRL_INIT, NULL));
	zassert_equal(disk_access_status(DISK_NAME), DISK_STATUS_OK);
}

static void *usbh_msc_setup(void)
{
	int err;

#if !defined(CONFIG_USB_MSC_DISK_INITIALIZE_AT_BOOT)
	err = usbh_init(&uhs_ctx);
	zassert_ok(err, "Failed to initialize USB host support");

	err = usbh_enable(&uhs_ctx);
	zassert_ok(err, "Failed to enable USB host support");
#endif

	test_uds_ctx = sample_usbd_init_device(NULL);
	zassert_not_null(test_uds_ctx, "Failed to initialize USB device support");

	err = usbd_enable(test_uds_ctx);
	zassert_ok(err, "Failed to enable USB device support");

	test_lun = wait_for_medium();
	zassert_not_null(test_lun, "Medium did not become ready");

	return NULL;
}

ZTEST_SUITE(usbh_msc, NULL, usbh_msc_setup, NULL, NULL, NULL);
