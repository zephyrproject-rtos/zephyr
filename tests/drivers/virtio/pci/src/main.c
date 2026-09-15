/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/pcie/cap.h>
#include <zephyr/drivers/pcie/controller.h>
#include <zephyr/drivers/pcie/pcie.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#define TEST_CTRL_NODE        DT_NODELABEL(pcie_test)
#define NON_VENDOR_NODE       DT_NODELABEL(virtio_pci_non_vendor)
#define RESERVED_BAR_NODE     DT_NODELABEL(virtio_pci_reserved_bar)
#define LONG_NOTIFY_NODE      DT_NODELABEL(virtio_pci_long_notify)
#define TRUNCATED_NOTIFY_NODE DT_NODELABEL(virtio_pci_truncated_notify)
#define SHORT_COMMON_NODE     DT_NODELABEL(virtio_pci_short_common)
#define NO_CAPS_NODE          DT_NODELABEL(virtio_pci_no_caps)

#define TEST_FIRST_DEVICE 20U
#define TEST_ENDPOINTS    6U
#define TEST_PAGE_SIZE    4096U
#define TEST_CONFIG_SIZE  256U

#define TEST_FIRST_CAP  0x40U
#define TEST_SECOND_CAP 0x50U
#define TEST_THIRD_CAP  0x60U
#define TEST_FOURTH_CAP 0x70U

#define TEST_CAP_COMMON_CFG 1U
#define TEST_CAP_NOTIFY_CFG 2U
#define TEST_CAP_ISR_CFG    3U
#define TEST_BAR_MAX        5U

#define TEST_COMMON_OFFSET         0x000U
#define TEST_ISR_OFFSET            0x100U
#define TEST_NOTIFY_OFFSET         0x200U
#define TEST_COMMON_LENGTH         0x100U
#define TEST_ISR_LENGTH            0x002U
#define TEST_NOTIFY_LENGTH         0x100U
#define TEST_DEVICE_FEATURE_OFFSET 0x004U

struct test_virtio_pci_cap {
	uint8_t cap_vndr;
	uint8_t cap_next;
	uint8_t cap_len;
	uint8_t cfg_type;
	uint8_t bar;
	uint8_t padding[3];
	uint32_t offset;
	uint32_t length;
};

struct test_virtio_pci_notify_cap {
	struct test_virtio_pci_cap cap;
	uint32_t notify_off_multiplier;
};

enum test_endpoint {
	ENDPOINT_NON_VENDOR,
	ENDPOINT_RESERVED_BAR,
	ENDPOINT_LONG_NOTIFY,
	ENDPOINT_TRUNCATED_NOTIFY,
	ENDPOINT_SHORT_COMMON,
	ENDPOINT_NO_CAPS,
};

static const pcie_id_t endpoint_ids[TEST_ENDPOINTS] = {
	PCIE_DT_ID(NON_VENDOR_NODE),
	PCIE_DT_ID(RESERVED_BAR_NODE),
	PCIE_DT_ID(LONG_NOTIFY_NODE),
	PCIE_DT_ID(TRUNCATED_NOTIFY_NODE),
	PCIE_DT_ID(SHORT_COMMON_NODE),
	PCIE_DT_ID(NO_CAPS_NODE),
};

static uint8_t config_space[TEST_ENDPOINTS][TEST_CONFIG_SIZE];
static uint8_t bar_pages[TEST_ENDPOINTS][TEST_PAGE_SIZE] __aligned(TEST_PAGE_SIZE);
static uint32_t command_status[TEST_ENDPOINTS];
static bool bar_probe[TEST_ENDPOINTS];

static pcie_bdf_t endpoint_bdf(enum test_endpoint endpoint)
{
	return PCIE_BDF(0, TEST_FIRST_DEVICE + endpoint, 0);
}

static int endpoint_from_bdf(pcie_bdf_t bdf)
{
	for (int endpoint = 0; endpoint < TEST_ENDPOINTS; endpoint++) {
		if (bdf == endpoint_bdf(endpoint)) {
			return endpoint;
		}
	}

	return -1;
}

static struct test_virtio_pci_cap make_cap(uint8_t cfg_type, uint8_t cap_next,
					   uint8_t bar, uint32_t offset, uint32_t length)
{
	return (struct test_virtio_pci_cap){
		.cap_vndr = PCI_CAP_ID_VNDR,
		.cap_next = cap_next,
		.cap_len = sizeof(struct test_virtio_pci_cap),
		.cfg_type = cfg_type,
		.bar = bar,
		.offset = sys_cpu_to_le32(offset),
		.length = sys_cpu_to_le32(length),
	};
}

static void set_capability(enum test_endpoint endpoint, uint32_t byte_offset,
			   const void *capability, size_t size)
{
	zassert_equal(byte_offset % sizeof(uint32_t), 0U);
	zassert_true(byte_offset + size <= TEST_CONFIG_SIZE);
	memcpy(&config_space[endpoint][byte_offset], capability, size);
}

static void set_valid_common_and_isr(enum test_endpoint endpoint, uint8_t common_offset,
				     uint8_t common_next, uint8_t isr_next)
{
	struct test_virtio_pci_cap common_cap =
		make_cap(TEST_CAP_COMMON_CFG, common_next, 0U, TEST_COMMON_OFFSET,
			 TEST_COMMON_LENGTH);
	struct test_virtio_pci_cap isr_cap =
		make_cap(TEST_CAP_ISR_CFG, isr_next, 0U, TEST_ISR_OFFSET, TEST_ISR_LENGTH);

	set_capability(endpoint, common_offset, &common_cap, sizeof(common_cap));
	set_capability(endpoint, common_next, &isr_cap, sizeof(isr_cap));
}

static void set_notify(enum test_endpoint endpoint, uint8_t notify_offset, uint8_t cap_len)
{
	struct test_virtio_pci_notify_cap notify_cap = {
		.cap = make_cap(TEST_CAP_NOTIFY_CFG, 0U, 0U, TEST_NOTIFY_OFFSET,
				TEST_NOTIFY_LENGTH),
		.notify_off_multiplier = sys_cpu_to_le32(0U),
	};

	notify_cap.cap.cap_len = cap_len;
	set_capability(endpoint, notify_offset, &notify_cap, sizeof(notify_cap));
}

static void build_capabilities(void)
{
	struct test_virtio_pci_cap fake_cap;
	uint32_t ignored_extra = 0xa5c35a5cU;

	memset(config_space, 0, sizeof(config_space));

	fake_cap = make_cap(TEST_CAP_COMMON_CFG, TEST_SECOND_CAP, 1U, 0U,
			    TEST_COMMON_LENGTH);
	fake_cap.cap_vndr = 0x05U;
	set_capability(ENDPOINT_NON_VENDOR, TEST_FIRST_CAP, &fake_cap, sizeof(fake_cap));
	set_valid_common_and_isr(ENDPOINT_NON_VENDOR, TEST_SECOND_CAP, TEST_THIRD_CAP,
				 TEST_FOURTH_CAP);
	set_notify(ENDPOINT_NON_VENDOR, TEST_FOURTH_CAP,
		   sizeof(struct test_virtio_pci_notify_cap));

	fake_cap = make_cap(TEST_CAP_COMMON_CFG, TEST_SECOND_CAP, TEST_BAR_MAX + 1U,
			    0U, TEST_COMMON_LENGTH);
	set_capability(ENDPOINT_RESERVED_BAR, TEST_FIRST_CAP, &fake_cap, sizeof(fake_cap));
	set_valid_common_and_isr(ENDPOINT_RESERVED_BAR, TEST_SECOND_CAP, TEST_THIRD_CAP,
				 TEST_FOURTH_CAP);
	set_notify(ENDPOINT_RESERVED_BAR, TEST_FOURTH_CAP,
		   sizeof(struct test_virtio_pci_notify_cap));

	set_valid_common_and_isr(ENDPOINT_LONG_NOTIFY, TEST_FIRST_CAP, TEST_SECOND_CAP,
				 TEST_THIRD_CAP);
	set_notify(ENDPOINT_LONG_NOTIFY, TEST_THIRD_CAP,
		   sizeof(struct test_virtio_pci_notify_cap) + sizeof(ignored_extra));
	set_capability(ENDPOINT_LONG_NOTIFY,
		       TEST_THIRD_CAP + sizeof(struct test_virtio_pci_notify_cap),
		       &ignored_extra, sizeof(ignored_extra));

	set_valid_common_and_isr(ENDPOINT_TRUNCATED_NOTIFY, TEST_FIRST_CAP, TEST_SECOND_CAP,
				 TEST_THIRD_CAP);
	set_notify(ENDPOINT_TRUNCATED_NOTIFY, TEST_THIRD_CAP,
		   sizeof(struct test_virtio_pci_cap));

	fake_cap = make_cap(TEST_CAP_COMMON_CFG, 0U, 0U, TEST_COMMON_OFFSET,
			    TEST_COMMON_LENGTH);
	fake_cap.cap_len = sizeof(fake_cap) - sizeof(uint32_t);
	set_capability(ENDPOINT_SHORT_COMMON, TEST_FIRST_CAP, &fake_cap, sizeof(fake_cap));
}

static uint32_t test_pcie_conf_read(const struct device *dev, pcie_bdf_t bdf, unsigned int reg)
{
	int endpoint = endpoint_from_bdf(bdf);
	uint32_t byte_offset = reg * sizeof(uint32_t);
	uint32_t word;

	ARG_UNUSED(dev);

	if (endpoint < 0) {
		return reg == PCIE_CONF_ID ? PCIE_ID_NONE : 0U;
	}

	switch (reg) {
	case PCIE_CONF_ID:
		return endpoint_ids[endpoint];
	case PCIE_CONF_TYPE:
	case PCIE_CONF_CLASSREV:
		return 0U;
	case PCIE_CONF_CMDSTAT:
		return command_status[endpoint];
	case PCIE_CONF_CAPPTR:
		return TEST_FIRST_CAP;
	case PCIE_CONF_BAR0:
		if (bar_probe[endpoint]) {
			return ~(TEST_PAGE_SIZE - 1U);
		}
		return (uint32_t)k_mem_phys_addr(bar_pages[endpoint]);
	default:
		break;
	}

	if (byte_offset >= TEST_FIRST_CAP && byte_offset + sizeof(word) <= TEST_CONFIG_SIZE) {
		memcpy(&word, &config_space[endpoint][byte_offset], sizeof(word));
		return word;
	}

	return 0U;
}

static void test_pcie_conf_write(const struct device *dev, pcie_bdf_t bdf, unsigned int reg,
				 uint32_t data)
{
	int endpoint = endpoint_from_bdf(bdf);

	ARG_UNUSED(dev);

	if (endpoint < 0) {
		return;
	}

	if (reg == PCIE_CONF_CMDSTAT) {
		command_status[endpoint] = data;
	} else if (reg == PCIE_CONF_BAR0) {
		bar_probe[endpoint] = data == UINT32_MAX;
	}
}

static bool test_pcie_region_allocate(const struct device *dev, pcie_bdf_t bdf, bool mem,
				      bool mem64, size_t bar_size, uintptr_t *bar_bus_addr)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(bdf);
	ARG_UNUSED(mem);
	ARG_UNUSED(mem64);
	ARG_UNUSED(bar_size);
	ARG_UNUSED(bar_bus_addr);

	return false;
}

static bool test_pcie_region_get_allocate_base(const struct device *dev, pcie_bdf_t bdf,
					       bool mem, bool mem64, size_t align,
					       uintptr_t *bar_base_addr)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(bdf);
	ARG_UNUSED(mem);
	ARG_UNUSED(mem64);
	ARG_UNUSED(align);
	ARG_UNUSED(bar_base_addr);

	return false;
}

static DEVICE_API(pcie_ctrl, test_pcie_ctrl_api) = {
	.conf_read = test_pcie_conf_read,
	.conf_write = test_pcie_conf_write,
	.region_allocate = test_pcie_region_allocate,
	.region_get_allocate_base = test_pcie_region_get_allocate_base,
};

DEVICE_DT_DEFINE(TEST_CTRL_NODE, NULL, NULL, NULL, NULL, PRE_KERNEL_1, CONFIG_PCIE_INIT_PRIORITY,
		 &test_pcie_ctrl_api);

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	memset(bar_pages, 0, sizeof(bar_pages));
	memset(bar_probe, 0, sizeof(bar_probe));
	for (int endpoint = 0; endpoint < TEST_ENDPOINTS; endpoint++) {
		command_status[endpoint] = PCIE_CONF_CMDSTAT_CAPS;
		sys_put_le32(BIT(0),
			     &bar_pages[endpoint][TEST_COMMON_OFFSET + TEST_DEVICE_FEATURE_OFFSET]);
	}
	command_status[ENDPOINT_NO_CAPS] = 0U;
	build_capabilities();
}

static pcie_bdf_t discovered_bdf(pcie_id_t id)
{
	STRUCT_SECTION_FOREACH(pcie_dev, dev) {
		if (dev->id == id) {
			return dev->bdf;
		}
	}

	return PCIE_BDF_NONE;
}

static void assert_endpoint(const struct device *dev, pcie_id_t id,
			    enum test_endpoint endpoint, bool should_initialize)
{
	int ret;

	zassert_equal(discovered_bdf(id), endpoint_bdf(endpoint),
		      "synthetic PCI endpoint %d was not discovered", endpoint);
	zassert_false(device_is_ready(dev), "deferred VirtIO PCI device unexpectedly ready");

	ret = device_init(dev);
	if (should_initialize) {
		zassert_ok(ret, "VirtIO PCI transport rejected capability scenario %d", endpoint);
		zassert_true(device_is_ready(dev), "VirtIO PCI transport did not become ready");
	} else {
		zassert_equal(ret, -EINVAL,
			      "VirtIO PCI transport accepted invalid scenario %d", endpoint);
		zassert_false(device_is_ready(dev), "failed VirtIO PCI transport became ready");
	}
}

ZTEST(virtio_pci_cap, test_non_vendor_capability_is_skipped)
{
	assert_endpoint(DEVICE_DT_GET(NON_VENDOR_NODE), PCIE_DT_ID(NON_VENDOR_NODE),
			ENDPOINT_NON_VENDOR, true);
}

ZTEST(virtio_pci_cap, test_reserved_bar_capability_is_skipped)
{
	assert_endpoint(DEVICE_DT_GET(RESERVED_BAR_NODE), PCIE_DT_ID(RESERVED_BAR_NODE),
			ENDPOINT_RESERVED_BAR, true);
}

ZTEST(virtio_pci_cap, test_longer_capability_is_accepted)
{
	assert_endpoint(DEVICE_DT_GET(LONG_NOTIFY_NODE), PCIE_DT_ID(LONG_NOTIFY_NODE),
			ENDPOINT_LONG_NOTIFY, true);
}

ZTEST(virtio_pci_cap, test_truncated_notify_capability_is_rejected)
{
	assert_endpoint(DEVICE_DT_GET(TRUNCATED_NOTIFY_NODE),
			PCIE_DT_ID(TRUNCATED_NOTIFY_NODE), ENDPOINT_TRUNCATED_NOTIFY,
			false);
}

ZTEST(virtio_pci_cap, test_short_base_capability_is_rejected)
{
	assert_endpoint(DEVICE_DT_GET(SHORT_COMMON_NODE), PCIE_DT_ID(SHORT_COMMON_NODE),
			ENDPOINT_SHORT_COMMON, false);
}

ZTEST(virtio_pci_cap, test_missing_capability_list_is_rejected)
{
	assert_endpoint(DEVICE_DT_GET(NO_CAPS_NODE), PCIE_DT_ID(NO_CAPS_NODE),
			ENDPOINT_NO_CAPS, false);
}

ZTEST_SUITE(virtio_pci_cap, NULL, NULL, before, NULL, NULL);
