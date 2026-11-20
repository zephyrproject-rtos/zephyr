/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
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
#include <zephyr/sys/iterable_sections.h>
#include <zephyr/ztest.h>

#define TEST_CTRL_NODE DT_NODELABEL(pcie_test)
#define BOUNDARY_NODE  DT_NODELABEL(virtio_pci_boundary)
#define PAST_BAR_NODE  DT_NODELABEL(virtio_pci_past_bar)
#define WRAP_NODE      DT_NODELABEL(virtio_pci_wrap)

#define TEST_FIRST_DEVICE 20U
#define TEST_ENDPOINTS    3U
#define TEST_PAGE_SIZE    4096U
#define TEST_CONFIG_SIZE  256U

#define TEST_FIRST_CAP  0x40U
#define TEST_SECOND_CAP 0x50U
#define TEST_THIRD_CAP  0x60U

#define TEST_CAP_COMMON_CFG 1U
#define TEST_CAP_NOTIFY_CFG 2U
#define TEST_CAP_ISR_CFG    3U

#define TEST_BOUNDARY_OFFSET       0x0f00U
#define TEST_BOUNDARY_LENGTH       0x0100U
#define TEST_PAST_BAR_LENGTH       0x0200U
#define TEST_WRAP_OFFSET           (UINT32_MAX - 0x0fU)
#define TEST_WRAP_LENGTH           0x20U
#define TEST_ISR_OFFSET            0x0100U
#define TEST_NOTIFY_OFFSET         0x0200U
#define TEST_DEVICE_FEATURE_OFFSET 0x0004U

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
	ENDPOINT_BOUNDARY,
	ENDPOINT_PAST_BAR,
	ENDPOINT_WRAP,
};

static const pcie_id_t endpoint_ids[TEST_ENDPOINTS] = {
	PCIE_DT_ID(BOUNDARY_NODE),
	PCIE_DT_ID(PAST_BAR_NODE),
	PCIE_DT_ID(WRAP_NODE),
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
					   uint32_t offset, uint32_t length)
{
	return (struct test_virtio_pci_cap){
		.cap_vndr = PCI_CAP_ID_VNDR,
		.cap_next = cap_next,
		.cap_len = sizeof(struct test_virtio_pci_cap),
		.cfg_type = cfg_type,
		.bar = 0U,
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

static void build_valid_endpoint(void)
{
	struct test_virtio_pci_cap common =
		make_cap(TEST_CAP_COMMON_CFG, TEST_SECOND_CAP, TEST_BOUNDARY_OFFSET,
			 TEST_BOUNDARY_LENGTH);
	struct test_virtio_pci_cap isr =
		make_cap(TEST_CAP_ISR_CFG, TEST_THIRD_CAP, TEST_ISR_OFFSET, sizeof(uint16_t));
	struct test_virtio_pci_notify_cap notify = {
		.cap = make_cap(TEST_CAP_NOTIFY_CFG, 0U, TEST_NOTIFY_OFFSET, sizeof(uint16_t)),
		.notify_off_multiplier = sys_cpu_to_le32(0U),
	};

	set_capability(ENDPOINT_BOUNDARY, TEST_FIRST_CAP, &common, sizeof(common));
	set_capability(ENDPOINT_BOUNDARY, TEST_SECOND_CAP, &isr, sizeof(isr));
	set_capability(ENDPOINT_BOUNDARY, TEST_THIRD_CAP, &notify, sizeof(notify));
	sys_put_le32(BIT(0), &bar_pages[ENDPOINT_BOUNDARY][TEST_BOUNDARY_OFFSET +
							      TEST_DEVICE_FEATURE_OFFSET]);
}

static void build_invalid_endpoint(enum test_endpoint endpoint, uint32_t offset, uint32_t length)
{
	struct test_virtio_pci_cap common = make_cap(TEST_CAP_COMMON_CFG, 0U, offset, length);

	set_capability(endpoint, TEST_FIRST_CAP, &common, sizeof(common));
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

	memset(config_space, 0, sizeof(config_space));
	memset(bar_pages, 0, sizeof(bar_pages));
	memset(bar_probe, 0, sizeof(bar_probe));
	for (int endpoint = 0; endpoint < TEST_ENDPOINTS; endpoint++) {
		command_status[endpoint] = PCIE_CONF_CMDSTAT_CAPS;
	}

	build_valid_endpoint();
	build_invalid_endpoint(ENDPOINT_PAST_BAR, TEST_BOUNDARY_OFFSET, TEST_PAST_BAR_LENGTH);
	build_invalid_endpoint(ENDPOINT_WRAP, TEST_WRAP_OFFSET, TEST_WRAP_LENGTH);
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
		zassert_ok(ret, "capability ending at BAR boundary was rejected");
		zassert_true(device_is_ready(dev),
			     "boundary VirtIO PCI device did not become ready");
	} else {
		zassert_equal(ret, -EINVAL, "out-of-bounds PCI capability was accepted");
		zassert_false(device_is_ready(dev), "invalid VirtIO PCI device became ready");
	}
}

ZTEST(virtio_pci_cap_bounds, test_capability_ending_at_bar_boundary_is_accepted)
{
	assert_endpoint(DEVICE_DT_GET(BOUNDARY_NODE), PCIE_DT_ID(BOUNDARY_NODE),
			ENDPOINT_BOUNDARY, true);
}

ZTEST(virtio_pci_cap_bounds, test_capability_past_bar_is_rejected)
{
	assert_endpoint(DEVICE_DT_GET(PAST_BAR_NODE), PCIE_DT_ID(PAST_BAR_NODE),
			ENDPOINT_PAST_BAR, false);
}

ZTEST(virtio_pci_cap_bounds, test_large_offset_cannot_wrap_bounds_check)
{
	assert_endpoint(DEVICE_DT_GET(WRAP_NODE), PCIE_DT_ID(WRAP_NODE), ENDPOINT_WRAP, false);
}

ZTEST_SUITE(virtio_pci_cap_bounds, NULL, NULL, before, NULL, NULL);
