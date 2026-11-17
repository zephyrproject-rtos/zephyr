/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/logging/log_backend.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log_msg.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include "virtio_pci.c"

#define PCI_CONFIG_SPACE_SIZE 256U
#define MMIO_SIZE             0x1000U
#define NOTIFY_OFFSET         0x300U
#define NOTIFY_REGION_LEN     8U
#define GUARD_LEN             16U
#define CANARY                0xA5U

static uint8_t config_space[PCI_CONFIG_SPACE_SIZE];
static uint8_t mmio[MMIO_SIZE];
static unsigned int mbar_calls[8];
static size_t mbar_call_count;

static struct pcie_dev pcie = {.bdf = PCIE_BDF(0, 1, 0)};
static struct virtio_pci_config config = {.pcie = &pcie};
static struct virtio_pci_data data;
static struct device dev = {.config = &config, .data = &data};

static atomic_t error_log_count;
static bool log_lock_was_free;

static void test_log_process(const struct log_backend *const backend, union log_msg_generic *msg)
{
	ARG_UNUSED(backend);

	if (log_msg_get_level(&msg->log) != LOG_LEVEL_ERR) {
		return;
	}

	atomic_inc(&error_log_count);

	k_spinlock_key_t key;

	if (k_spin_trylock(&data.notify_lock, &key) == 0) {
		log_lock_was_free = true;
		k_spin_unlock(&data.notify_lock, key);
	} else {
		log_lock_was_free = false;
	}
}

static const struct log_backend_api test_log_api = {
	.process = test_log_process,
};

LOG_BACKEND_DEFINE(virtio_pci_notify_backend, test_log_api, false, NULL);

uint32_t pcie_conf_read(pcie_bdf_t bdf, unsigned int reg)
{
	uint32_t val = 0xffffffffU;

	ARG_UNUSED(bdf);

	if ((reg + 1U) * sizeof(uint32_t) <= sizeof(config_space)) {
		memcpy(&val, &config_space[reg * sizeof(uint32_t)], sizeof(val));
	}

	return val;
}

void pcie_conf_write(pcie_bdf_t bdf, unsigned int reg, uint32_t data)
{
	ARG_UNUSED(bdf);

	if ((reg + 1U) * sizeof(uint32_t) <= sizeof(config_space)) {
		memcpy(&config_space[reg * sizeof(uint32_t)], &data, sizeof(data));
	}
}

bool pcie_get_mbar(pcie_bdf_t bdf, unsigned int bar_index, struct pcie_bar *mbar)
{
	ARG_UNUSED(bdf);

	if (mbar_call_count < ARRAY_SIZE(mbar_calls)) {
		mbar_calls[mbar_call_count++] = bar_index;
	}

	mbar->phys_addr = (uintptr_t)mmio;
	mbar->size = sizeof(mmio);

	return true;
}

static void write_cap(uint32_t byte_off, uint8_t cfg_type, uint8_t bar, uint32_t offset,
		      uint32_t length, uint8_t next, uint32_t multiplier)
{
	struct virtio_pci_notify_cap cap = {0};

	cap.cap.cap_vndr = 0x09U;
	cap.cap.cap_next = next;
	cap.cap.cap_len = (cfg_type == VIRTIO_PCI_CAP_NOTIFY_CFG)
				  ? sizeof(struct virtio_pci_notify_cap)
				  : sizeof(struct virtio_pci_cap);
	cap.cap.cfg_type = cfg_type;
	cap.cap.bar = bar;
	cap.cap.offset = offset;
	cap.cap.length = length;
	cap.notify_off_multiplier = multiplier;

	memcpy(&config_space[byte_off], &cap, cap.cap.cap_len);
}

static bool notify_mapped(void)
{
	for (size_t i = 0U; i < mbar_call_count; i++) {
		if (mbar_calls[i] == 3U) {
			return true;
		}
	}

	return false;
}

static void reset_fixture(uint32_t notify_len)
{
	memset(config_space, 0, sizeof(config_space));
	memset(mmio, 0, sizeof(mmio));
	memset(&data, 0, sizeof(data));
	memset(mbar_calls, 0, sizeof(mbar_calls));
	mbar_call_count = 0U;
	atomic_set(&error_log_count, 0);
	log_lock_was_free = false;

	/* Status register: capability list valid. */
	config_space[6] = BIT(4);
	/* Capability pointer at 0x40. */
	config_space[0x34] = 0x40;

	write_cap(0x40, VIRTIO_PCI_CAP_COMMON_CFG, 0, 0x000, 0x100, 0x50, 0);
	write_cap(0x50, VIRTIO_PCI_CAP_ISR_CFG, 0, 0x100, 0x10, 0x60, 0);
	write_cap(0x60, VIRTIO_PCI_CAP_DEVICE_CFG, 0, 0x200, 0x40, 0x70, 0);
	write_cap(0x70, VIRTIO_PCI_CAP_NOTIFY_CFG, 3, NOTIFY_OFFSET, notify_len, 0, 1);

	struct virtio_pci_common_cfg *common = (struct virtio_pci_common_cfg *)&mmio[0];

	memset(common, 0, sizeof(*common));
	/* Advertise VIRTIO_F_VERSION_1 (bit 32) in feature word 1. */
	common->device_feature = 1;
}

static int init_fixture(uint32_t notify_len)
{
	reset_fixture(notify_len);

	return virtio_pci_init_common(&dev);
}

static void fill_notify_window(void)
{
	memset(&mmio[NOTIFY_OFFSET - GUARD_LEN], CANARY, GUARD_LEN + NOTIFY_REGION_LEN + GUARD_LEN);
}

static void assert_notify_untouched(void)
{
	for (size_t i = 0U; i < GUARD_LEN + NOTIFY_REGION_LEN + GUARD_LEN; i++) {
		zassert_equal(mmio[NOTIFY_OFFSET - GUARD_LEN + i], CANARY,
			      "unexpected write at byte %zu", NOTIFY_OFFSET - GUARD_LEN + i);
	}
}

static void assert_notify_written(size_t offset, uint16_t value)
{
	uint8_t expected[sizeof(uint16_t)];

	sys_put_le16(value, expected);

	for (size_t i = 0U; i < GUARD_LEN + NOTIFY_REGION_LEN + GUARD_LEN; i++) {
		size_t pos = NOTIFY_OFFSET - GUARD_LEN + i;

		if (i >= GUARD_LEN + offset && i < GUARD_LEN + offset + sizeof(expected)) {
			zassert_equal(mmio[pos], expected[i - GUARD_LEN - offset],
				      "unexpected byte at offset %zu", pos);
		} else {
			zassert_equal(mmio[pos], CANARY, "unexpected write at byte %zu", pos);
		}
	}
}

ZTEST(virtio_pci_notify_driver, test_init_rejects_short_notify_region)
{
	for (uint32_t len = 0U; len <= 1U; len++) {
		int ret = init_fixture(len);

		zassert_not_equal(ret, 0, "initialization must fail for length %u", len);
		zassert_false(notify_mapped(), "NOTIFY mapping must not be called for length %u",
			      len);
	}
}

ZTEST(virtio_pci_notify_driver, test_init_maps_valid_notify_region)
{
	int ret = init_fixture(NOTIFY_REGION_LEN);

	zassert_equal(ret, 0);
	zassert_true(notify_mapped());
	zassert_equal(data.notify_len, NOTIFY_REGION_LEN);
	zassert_equal(data.notify_off_multiplier, 1U);
	zassert_equal(data.notify_cfg, (volatile uint8_t *)&mmio[NOTIFY_OFFSET]);
}

ZTEST(virtio_pci_notify_driver, test_valid_invalid_valid_sequence)
{
	zassert_equal(init_fixture(NOTIFY_REGION_LEN), 0);

	struct virtio_pci_common_cfg *common = (struct virtio_pci_common_cfg *)&mmio[0];

	atomic_set(&error_log_count, 0);
	log_lock_was_free = false;

	fill_notify_window();
	common->queue_notify_off = sys_cpu_to_le16(0U);
	virtio_pci_notify_queue(&dev, 0x1234U);
	zassert_equal(common->queue_select, sys_cpu_to_le16(0x1234U));
	assert_notify_written(0U, 0x1234U);
	zassert_equal(atomic_get(&error_log_count), 0);

	fill_notify_window();
	common->queue_notify_off = sys_cpu_to_le16(NOTIFY_REGION_LEN - 1U);
	virtio_pci_notify_queue(&dev, 0x2345U);
	assert_notify_untouched();
	zassert_equal(atomic_get(&error_log_count), 1);
	zassert_true(log_lock_was_free, "error log must be emitted after unlocking");

	fill_notify_window();
	common->queue_notify_off = sys_cpu_to_le16(3U);
	virtio_pci_notify_queue(&dev, 0x3456U);
	assert_notify_written(3U, 0x3456U);
	zassert_equal(atomic_get(&error_log_count), 1);
}

ZTEST(virtio_pci_notify_driver, test_driver_rejects_32bit_product_wrap)
{
	zassert_equal(init_fixture(NOTIFY_REGION_LEN), 0);

	struct virtio_pci_common_cfg *common = (struct virtio_pci_common_cfg *)&mmio[0];

	fill_notify_window();
	atomic_set(&error_log_count, 0);
	data.notify_off_multiplier = 0x20000U;
	common->queue_notify_off = sys_cpu_to_le16(0x8000U);
	virtio_pci_notify_queue(&dev, 0x4567U);
	assert_notify_untouched();
	zassert_equal(atomic_get(&error_log_count), 1);
}

ZTEST(virtio_pci_notify_driver, test_driver_rejects_offset_plus_two_wrap)
{
	zassert_equal(init_fixture(NOTIFY_REGION_LEN), 0);

	struct virtio_pci_common_cfg *common = (struct virtio_pci_common_cfg *)&mmio[0];

	fill_notify_window();
	atomic_set(&error_log_count, 0);
	data.notify_off_multiplier = 0x10001U;
	common->queue_notify_off = sys_cpu_to_le16(0xffffU);
	virtio_pci_notify_queue(&dev, 0x5678U);
	assert_notify_untouched();
	zassert_equal(atomic_get(&error_log_count), 1);
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	log_backend_enable(&virtio_pci_notify_backend, NULL, LOG_LEVEL_DBG);
}

static void after(void *fixture)
{
	ARG_UNUSED(fixture);

	log_backend_disable(&virtio_pci_notify_backend);
}

ZTEST_SUITE(virtio_pci_notify_driver, NULL, NULL, before, after, NULL);
