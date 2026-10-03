/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/pcie/cap.h>
#include <zephyr/drivers/pcie/controller.h>
#include <zephyr/drivers/pcie/pcie.h>
#include <zephyr/ztest.h>

#define TEST_CTRL_NODE   DT_NODELABEL(pcie_test)
#define CYCLE_NODE       DT_NODELABEL(virtio_pci_cycle)
#define LINEAR_NODE      DT_NODELABEL(virtio_pci_linear)

#define TEST_FIRST_DEVICE   20U
#define TEST_ENDPOINTS      2U
#define TEST_CONFIG_SIZE    256U
#define TEST_FIRST_CAP  0x40U
#define TEST_SECOND_CAP 0x50U

/* Keep the pre-fix capability walker from hanging the regression test. */
#define TEST_CAP_READ_LIMIT 16U
#define TEST_CFG_TYPE_OTHER 0xffU

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

enum test_endpoint {
	ENDPOINT_CYCLE,
	ENDPOINT_LINEAR,
};

static const pcie_id_t endpoint_ids[TEST_ENDPOINTS] = {
	PCIE_DT_ID(CYCLE_NODE),
	PCIE_DT_ID(LINEAR_NODE),
};

static uint8_t config_space[TEST_ENDPOINTS][TEST_CONFIG_SIZE];
static uint32_t cap_word_reads[TEST_ENDPOINTS];
static bool read_limit_hit[TEST_ENDPOINTS];

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

static struct test_virtio_pci_cap make_cap(uint8_t cap_next)
{
	return (struct test_virtio_pci_cap){
		.cap_vndr = PCI_CAP_ID_VNDR,
		.cap_next = cap_next,
		.cap_len = sizeof(struct test_virtio_pci_cap),
		.cfg_type = TEST_CFG_TYPE_OTHER,
	};
}

static void set_capability(enum test_endpoint endpoint, uint32_t byte_offset,
			   const struct test_virtio_pci_cap *capability)
{
	zassert_equal(byte_offset % sizeof(uint32_t), 0U);
	zassert_true(byte_offset + sizeof(*capability) <= TEST_CONFIG_SIZE);
	memcpy(&config_space[endpoint][byte_offset], capability, sizeof(*capability));
}

static void build_capabilities(void)
{
	struct test_virtio_pci_cap first_cap;
	struct test_virtio_pci_cap second_cap;

	memset(config_space, 0, sizeof(config_space));

	first_cap = make_cap(TEST_SECOND_CAP);
	second_cap = make_cap(TEST_FIRST_CAP);
	set_capability(ENDPOINT_CYCLE, TEST_FIRST_CAP, &first_cap);
	set_capability(ENDPOINT_CYCLE, TEST_SECOND_CAP, &second_cap);

	second_cap = make_cap(0U);
	set_capability(ENDPOINT_LINEAR, TEST_FIRST_CAP, &first_cap);
	set_capability(ENDPOINT_LINEAR, TEST_SECOND_CAP, &second_cap);
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
		return PCIE_CONF_CMDSTAT_CAPS;
	case PCIE_CONF_CAPPTR:
		return TEST_FIRST_CAP;
	default:
		break;
	}

	if (byte_offset >= TEST_FIRST_CAP && byte_offset + sizeof(word) <= TEST_CONFIG_SIZE) {
		if (cap_word_reads[endpoint] >= TEST_CAP_READ_LIMIT) {
			read_limit_hit[endpoint] = true;
			return 0U;
		}

		cap_word_reads[endpoint]++;
		memcpy(&word, &config_space[endpoint][byte_offset], sizeof(word));
		return word;
	}

	return 0U;
}

static void test_pcie_conf_write(const struct device *dev, pcie_bdf_t bdf, unsigned int reg,
				 uint32_t data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(bdf);
	ARG_UNUSED(reg);
	ARG_UNUSED(data);
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

	memset(cap_word_reads, 0, sizeof(cap_word_reads));
	memset(read_limit_hit, 0, sizeof(read_limit_hit));
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

static void assert_chain_rejected(const struct device *dev, pcie_id_t id,
				  enum test_endpoint endpoint)
{
	int ret;

	zassert_equal(discovered_bdf(id), endpoint_bdf(endpoint),
		      "synthetic PCI endpoint %d was not discovered", endpoint);
	zassert_false(device_is_ready(dev), "deferred VirtIO PCI device unexpectedly ready");
	zassert_not_null(dev->ops.init, "VirtIO PCI device has no init callback");

	ret = dev->ops.init(dev);

	zassert_not_equal(ret, 0, "VirtIO PCI transport unexpectedly initialized");
	zassert_false(read_limit_hit[endpoint], "capability walk exhausted defensive read limit");
	zassert_equal(cap_word_reads[endpoint], 8U, "unexpected capability read count");
}

ZTEST(virtio_pci_cap_cycle, test_cycle_is_rejected)
{
	assert_chain_rejected(DEVICE_DT_GET(CYCLE_NODE), PCIE_DT_ID(CYCLE_NODE), ENDPOINT_CYCLE);
}

ZTEST(virtio_pci_cap_cycle, test_linear_chain_still_terminates)
{
	assert_chain_rejected(DEVICE_DT_GET(LINEAR_NODE), PCIE_DT_ID(LINEAR_NODE),
			      ENDPOINT_LINEAR);
}

ZTEST_SUITE(virtio_pci_cap_cycle, NULL, NULL, before, NULL, NULL);
