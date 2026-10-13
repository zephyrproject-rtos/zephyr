/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sd/sd.h>
#include <zephyr/sd/sdmmc.h>
#include <zephyr/drivers/disk.h>
#include <zephyr/pm/pm.h>
#include <zephyr/ztest.h>

#define PM_SECTOR_SIZE  512
#define PM_SECTOR_COUNT 16
#define PM_BUF_SIZE     (PM_SECTOR_SIZE * PM_SECTOR_COUNT)

/* Long enough for the PM policy to select a low-power state. */
#define PM_IDLE_WINDOW_MS 100

static const struct device *const pm_sdhc_dev = DEVICE_DT_GET(DT_ALIAS(sdhc0));
static struct sd_card pm_card;
static uint8_t wbuf[PM_BUF_SIZE] __aligned(CONFIG_SDHC_BUFFER_ALIGNMENT);
static uint8_t rbuf[PM_BUF_SIZE] __aligned(CONFIG_SDHC_BUFFER_ALIGNMENT);
static uint32_t pm_block_addr;

static atomic_t pm_entries;

static void pm_state_entry(enum pm_state state)
{
	ARG_UNUSED(state);

	atomic_inc(&pm_entries);
}

static struct pm_notifier pm_entry_notifier = {
	.state_entry = pm_state_entry,
};

/* Stay idle and check that the system entered a low-power state. */
static void check_low_power_entry(void)
{
	atomic_clear(&pm_entries);
	k_sleep(K_MSEC(PM_IDLE_WINDOW_MS));
	zassert_true(atomic_get(&pm_entries) > 0,
		     "System did not enter a low-power state during the idle window");
}

static void fill_pattern(uint8_t seed)
{
	for (size_t i = 0; i < sizeof(wbuf); i++) {
		wbuf[i] = (uint8_t)(i + seed);
	}
}

static void write_pattern(uint8_t seed)
{
	fill_pattern(seed);
	zassert_equal(sdmmc_write_blocks(&pm_card, wbuf, pm_block_addr, PM_SECTOR_COUNT), 0,
		      "Write failed");
}

static void read_and_check(void)
{
	memset(rbuf, 0, sizeof(rbuf));
	zassert_equal(sdmmc_read_blocks(&pm_card, rbuf, pm_block_addr, PM_SECTOR_COUNT), 0,
		      "Read failed");
	zassert_mem_equal(rbuf, wbuf, sizeof(rbuf), "Read data does not match written data");
}

static void *pm_setup(void)
{
	uint32_t sector_count;

	zassert_true(device_is_ready(pm_sdhc_dev), "SDHC device is not ready");
	zassert_equal(sd_is_card_present(pm_sdhc_dev), 1, "SD card not present in slot");
	zassert_equal(sd_init(pm_sdhc_dev, &pm_card), 0, "Card initialization failed");
	zassert_equal(sdmmc_ioctl(&pm_card, DISK_IOCTL_GET_SECTOR_COUNT, &sector_count), 0,
		      "IOCTL sector count read failed");

	pm_block_addr = sector_count / 4;

	pm_notifier_register(&pm_entry_notifier);

	return NULL;
}

static void pm_teardown(void *fixture)
{
	ARG_UNUSED(fixture);

	pm_notifier_unregister(&pm_entry_notifier);
}

/*
 * The transfer thread blocks inside the driver while it waits for the
 * controller, and the SD stack only busy-waits between commands. Any
 * low-power state entry seen here happened during a transfer.
 */
ZTEST(sd_stack_pm, test_no_low_power_during_transfer)
{
	atomic_clear(&pm_entries);

	for (int i = 0; i < 8; i++) {
		write_pattern((uint8_t)i);
		read_and_check();
	}

	zassert_equal(atomic_get(&pm_entries), 0,
		      "System entered a low-power state during a transfer");
}

/* A low-power state between commands must not lose the host configuration. */
ZTEST(sd_stack_pm, test_transfer_after_low_power)
{
	for (int cycle = 0; cycle < 3; cycle++) {
		check_low_power_entry();
		write_pattern((uint8_t)(0x40 + cycle));
		check_low_power_entry();
		read_and_check();
	}
}

/* Card init after a low-power state, then a transfer across further ones. */
ZTEST(sd_stack_pm, test_init_after_low_power)
{
	check_low_power_entry();
	zassert_equal(sd_init(pm_sdhc_dev, &pm_card), 0, "Card initialization failed");

	check_low_power_entry();
	write_pattern(0x80);
	check_low_power_entry();
	read_and_check();
}

ZTEST_SUITE(sd_stack_pm, NULL, pm_setup, NULL, NULL, pm_teardown);
