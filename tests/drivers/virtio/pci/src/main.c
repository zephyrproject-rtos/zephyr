/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/ztest.h>

#include "../../../../../drivers/virtio/virtio_pci.c"

#define TEST_BDF PCIE_BDF(0, 1, 0)

#define TEST_CFG_DWORDS       64U
#define TEST_STATUS_REG       0x01U
#define TEST_CAP_PTR_REG      0x0dU
#define TEST_CAP_LIST_PRESENT BIT(20)
#define TEST_FIRST_CAP        0x40U
#define TEST_SECOND_CAP       0x60U
#define TEST_GUARD            0x5AA55AA5U

static uint32_t config_space[TEST_CFG_DWORDS];

uint32_t pcie_conf_read(pcie_bdf_t bdf, unsigned int reg)
{
	zassert_equal(bdf, TEST_BDF);
	zassert_true(reg < ARRAY_SIZE(config_space),
		     "configuration register %u is out of range", reg);

	return config_space[reg];
}

bool pcie_get_mbar(pcie_bdf_t bdf, unsigned int bar_index, struct pcie_bar *mbar)
{
	ARG_UNUSED(bdf);
	ARG_UNUSED(bar_index);
	ARG_UNUSED(mbar);

	return false;
}

static void set_capability(uint32_t byte_offset, const void *capability, size_t size)
{
	zassert_equal(byte_offset % sizeof(uint32_t), 0U);
	zassert_true(byte_offset + size <= sizeof(config_space));

	memcpy((uint8_t *)config_space + byte_offset, capability, size);
}

static struct virtio_pci_cap make_cap(uint8_t cfg_type, uint8_t cap_next)
{
	return (struct virtio_pci_cap) {
		.cap_vndr = PCI_CAP_ID_VNDR,
		.cap_next = cap_next,
		.cap_len = sizeof(struct virtio_pci_cap),
		.cfg_type = cfg_type,
		.bar = 2U,
		.offset = 0x12340000U,
		.length = 0x1000U,
	};
}

static bool read_cap(uint8_t cfg_type, void *cap, size_t size)
{
	return virtio_pci_read_cap(TEST_BDF, cfg_type, cap, size);
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	memset(config_space, 0, sizeof(config_space));
	config_space[TEST_STATUS_REG] = TEST_CAP_LIST_PRESENT;
	config_space[TEST_CAP_PTR_REG] = TEST_FIRST_CAP;
}

ZTEST(virtio_pci_cap, test_non_vendor_capability_is_skipped)
{
	struct virtio_pci_cap fake_cap = make_cap(VIRTIO_PCI_CAP_COMMON_CFG, TEST_SECOND_CAP);
	struct virtio_pci_cap virtio_cap = make_cap(VIRTIO_PCI_CAP_COMMON_CFG, 0U);
	struct virtio_pci_cap result = { 0 };

	fake_cap.cap_vndr = 0x05U;
	fake_cap.bar = 1U;
	virtio_cap.bar = 4U;
	virtio_cap.offset = 0x56780000U;

	set_capability(TEST_FIRST_CAP, &fake_cap, sizeof(fake_cap));
	set_capability(TEST_SECOND_CAP, &virtio_cap, sizeof(virtio_cap));

	zassert_true(read_cap(VIRTIO_PCI_CAP_COMMON_CFG, &result, sizeof(result)));
	zassert_equal(result.cap_vndr, PCI_CAP_ID_VNDR);
	zassert_equal(result.bar, virtio_cap.bar);
	zassert_equal(result.offset, virtio_cap.offset);
}

ZTEST(virtio_pci_cap, test_reserved_bar_capability_is_skipped)
{
	struct virtio_pci_cap reserved_cap = make_cap(VIRTIO_PCI_CAP_COMMON_CFG, TEST_SECOND_CAP);
	struct virtio_pci_cap virtio_cap = make_cap(VIRTIO_PCI_CAP_COMMON_CFG, 0U);
	struct virtio_pci_cap result = { 0 };

	reserved_cap.bar = VIRTIO_PCI_BAR_MAX + 1U;
	virtio_cap.bar = 3U;
	virtio_cap.offset = 0x87650000U;

	set_capability(TEST_FIRST_CAP, &reserved_cap, sizeof(reserved_cap));
	set_capability(TEST_SECOND_CAP, &virtio_cap, sizeof(virtio_cap));

	zassert_true(read_cap(VIRTIO_PCI_CAP_COMMON_CFG, &result, sizeof(result)));
	zassert_equal(result.bar, virtio_cap.bar);
	zassert_equal(result.offset, virtio_cap.offset);
}

ZTEST(virtio_pci_cap, test_longer_capability_is_accepted_without_overrun)
{
	struct virtio_pci_notify_cap notify_cap = {
		.cap = make_cap(VIRTIO_PCI_CAP_NOTIFY_CFG, 0U),
		.notify_off_multiplier = 0x10203040U,
	};
	struct {
		struct virtio_pci_notify_cap cap;
		uint32_t guard;
	} result = {
		.guard = TEST_GUARD,
	};
	uint32_t ignored_extra = 0xa5c35a5cU;

	notify_cap.cap.cap_len = sizeof(notify_cap) + sizeof(ignored_extra);
	set_capability(TEST_FIRST_CAP, &notify_cap, sizeof(notify_cap));
	set_capability(TEST_FIRST_CAP + sizeof(notify_cap), &ignored_extra, sizeof(ignored_extra));

	zassert_true(read_cap(VIRTIO_PCI_CAP_NOTIFY_CFG, &result.cap, sizeof(result.cap)));
	zassert_equal(result.cap.notify_off_multiplier, notify_cap.notify_off_multiplier);
	zassert_equal(result.guard, TEST_GUARD);
}

ZTEST(virtio_pci_cap, test_truncated_notify_capability_is_rejected)
{
	struct virtio_pci_cap notify_cap = make_cap(VIRTIO_PCI_CAP_NOTIFY_CFG, 0U);
	struct virtio_pci_notify_cap result = {
		.notify_off_multiplier = TEST_GUARD,
	};

	set_capability(TEST_FIRST_CAP, &notify_cap, sizeof(notify_cap));

	zassert_false(read_cap(VIRTIO_PCI_CAP_NOTIFY_CFG, &result, sizeof(result)));
	zassert_equal(result.notify_off_multiplier, TEST_GUARD);
}

ZTEST(virtio_pci_cap, test_short_base_capability_is_rejected)
{
	struct virtio_pci_cap short_cap = make_cap(VIRTIO_PCI_CAP_COMMON_CFG, 0U);
	struct virtio_pci_cap result = { 0 };

	short_cap.cap_len = sizeof(short_cap) - sizeof(uint32_t);
	set_capability(TEST_FIRST_CAP, &short_cap, sizeof(short_cap));

	zassert_false(read_cap(VIRTIO_PCI_CAP_COMMON_CFG, &result, sizeof(result)));
}

ZTEST(virtio_pci_cap, test_missing_capability_list_is_rejected)
{
	struct virtio_pci_cap result = { 0 };

	config_space[TEST_STATUS_REG] = 0U;

	zassert_false(read_cap(VIRTIO_PCI_CAP_COMMON_CFG, &result, sizeof(result)));
}

ZTEST_SUITE(virtio_pci_cap, NULL, NULL, before, NULL, NULL);
