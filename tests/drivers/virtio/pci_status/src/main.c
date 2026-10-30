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
#include <zephyr/drivers/virtio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#define TEST_NODE      DT_NODELABEL(virtio_pci_test)
#define TEST_CTRL_NODE DT_NODELABEL(pcie_test)
#define TEST_PCIE_DEV  Z_DEVICE_PCIE_NAME(TEST_NODE)
#define TEST_BDF       PCIE_BDF(0, 31, 0)

#define TEST_FIRST_CAP      0x40U
#define TEST_ISR_CAP        0x50U
#define TEST_NOTIFY_CAP     0x60U
#define TEST_CAP_COMMON_CFG 1U
#define TEST_CAP_NOTIFY_CFG 2U
#define TEST_CAP_ISR_CFG    3U
#define TEST_BAR_SIZE      0x20000U

/* Byte-symmetric, nonzero lengths isolate this test from capability-endian fixes. */
#define TEST_CAP_LENGTH 0x00010100U

#define TEST_STATUS_ACKNOWLEDGE BIT(0)
#define TEST_STATUS_DRIVER      BIT(1)
#define TEST_STATUS_DRIVER_OK   BIT(2)
#define TEST_STATUS_FEATURES_OK BIT(3)

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

struct test_virtio_pci_common_cfg {
	uint32_t device_feature_select;
	uint32_t device_feature;
	uint32_t driver_feature_select;
	uint32_t driver_feature;
	uint16_t config_msix_vector;
	uint16_t num_queues;
	uint8_t device_status;
	uint8_t config_generation;
	uint16_t queue_select;
	uint16_t queue_size;
	uint16_t queue_msix_vector;
	uint16_t queue_enable;
	uint16_t queue_notify_off;
	uint64_t queue_desc;
	uint64_t queue_driver;
	uint64_t queue_device;
	uint16_t queue_notify_data;
	uint16_t queue_reset;
	uint16_t admin_queue_index;
	uint16_t admin_queue_num;
};

union test_common_page {
	struct test_virtio_pci_common_cfg cfg;
	uint8_t bytes[TEST_BAR_SIZE];
};

extern struct pcie_dev TEST_PCIE_DEV;

static union test_common_page common_page __aligned(TEST_BAR_SIZE);
static uint8_t isr_page[TEST_BAR_SIZE] __aligned(TEST_BAR_SIZE);
static uint8_t notify_page[TEST_BAR_SIZE] __aligned(TEST_BAR_SIZE);

static struct test_virtio_pci_cap common_cap;
static struct test_virtio_pci_cap isr_cap;
static struct test_virtio_pci_notify_cap notify_cap;
static bool bar_probe[3];
static uint32_t command_status;

static uint32_t read_cap_word(const void *cap, unsigned int cap_reg, unsigned int reg)
{
	uint32_t word;
	size_t offset = (reg - cap_reg) * sizeof(word);

	memcpy(&word, (const uint8_t *)cap + offset, sizeof(word));
	return word;
}

static uint8_t *test_bar(unsigned int bar_index)
{
	switch (bar_index) {
	case 0:
		return common_page.bytes;
	case 1:
		return isr_page;
	case 2:
		return notify_page;
	default:
		return NULL;
	}
}

static uint32_t test_pcie_conf_read(const struct device *dev, pcie_bdf_t bdf, unsigned int reg)
{
	const unsigned int common_reg = TEST_FIRST_CAP / sizeof(uint32_t);
	const unsigned int isr_reg = TEST_ISR_CAP / sizeof(uint32_t);
	const unsigned int notify_reg = TEST_NOTIFY_CAP / sizeof(uint32_t);
	const unsigned int common_last = common_reg + sizeof(common_cap) / sizeof(uint32_t) - 1U;
	const unsigned int isr_last = isr_reg + sizeof(isr_cap) / sizeof(uint32_t) - 1U;
	const unsigned int notify_last = notify_reg + sizeof(notify_cap) / sizeof(uint32_t) - 1U;

	ARG_UNUSED(dev);

	if (bdf != TEST_BDF) {
		return reg == PCIE_CONF_ID ? PCIE_ID_NONE : 0U;
	}

	switch (reg) {
	case PCIE_CONF_ID:
		return PCIE_DT_ID(TEST_NODE);
	case PCIE_CONF_TYPE:
	case PCIE_CONF_CLASSREV:
		return 0U;
	case PCIE_CONF_CMDSTAT:
		return command_status;
	case PCIE_CONF_CAPPTR:
		return TEST_FIRST_CAP;
	default:
		break;
	}

	if (IN_RANGE(reg, PCIE_CONF_BAR0, PCIE_CONF_BAR0 + 2U)) {
		unsigned int bar_index = reg - PCIE_CONF_BAR0;

		if (bar_probe[bar_index]) {
			return ~(TEST_BAR_SIZE - 1U);
		}
		return (uint32_t)k_mem_phys_addr(test_bar(bar_index));
	}

	if (IN_RANGE(reg, common_reg, common_last)) {
		return read_cap_word(&common_cap, common_reg, reg);
	}
	if (IN_RANGE(reg, isr_reg, isr_last)) {
		return read_cap_word(&isr_cap, isr_reg, reg);
	}
	if (IN_RANGE(reg, notify_reg, notify_last)) {
		return read_cap_word(&notify_cap, notify_reg, reg);
	}

	return 0U;
}

static void test_pcie_conf_write(const struct device *dev, pcie_bdf_t bdf, unsigned int reg,
				 uint32_t data)
{
	ARG_UNUSED(dev);

	if (bdf != TEST_BDF) {
		return;
	}

	if (reg == PCIE_CONF_CMDSTAT) {
		command_status = data;
	} else if (IN_RANGE(reg, PCIE_CONF_BAR0, PCIE_CONF_BAR0 + 2U)) {
		bar_probe[reg - PCIE_CONF_BAR0] = data == UINT32_MAX;
	}
}

#ifdef CONFIG_X86
/* x86 provides its own PCI host; emulate only its public config-space backend. */
uint32_t __wrap_pcie_conf_read(pcie_bdf_t bdf, unsigned int reg)
{
	return test_pcie_conf_read(NULL, bdf, reg);
}

void __wrap_pcie_conf_write(pcie_bdf_t bdf, unsigned int reg, uint32_t data)
{
	test_pcie_conf_write(NULL, bdf, reg, data);
}
#else
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
#endif

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	memset(&common_page, 0, sizeof(common_page));
	memset(isr_page, 0, sizeof(isr_page));
	memset(notify_page, 0, sizeof(notify_page));
	memset(bar_probe, 0, sizeof(bar_probe));
	command_status = PCIE_CONF_CMDSTAT_CAPS;

	common_cap = (struct test_virtio_pci_cap){
		.cap_vndr = PCI_CAP_ID_VNDR,
		.cap_next = TEST_ISR_CAP,
		.cap_len = sizeof(common_cap),
		.cfg_type = TEST_CAP_COMMON_CFG,
		.bar = 0U,
		.length = sys_cpu_to_le32(TEST_CAP_LENGTH),
	};
	isr_cap = (struct test_virtio_pci_cap){
		.cap_vndr = PCI_CAP_ID_VNDR,
		.cap_next = TEST_NOTIFY_CAP,
		.cap_len = sizeof(isr_cap),
		.cfg_type = TEST_CAP_ISR_CFG,
		.bar = 1U,
		.length = sys_cpu_to_le32(TEST_CAP_LENGTH),
	};
	notify_cap = (struct test_virtio_pci_notify_cap){
		.cap = {
			.cap_vndr = PCI_CAP_ID_VNDR,
			.cap_next = 0U,
			.cap_len = sizeof(notify_cap),
			.cfg_type = TEST_CAP_NOTIFY_CFG,
			.bar = 2U,
			.length = sys_cpu_to_le32(TEST_CAP_LENGTH),
		},
		.notify_off_multiplier = sys_cpu_to_le32(0U),
	};

	common_page.cfg.device_feature = sys_cpu_to_le32(BIT(0));
}

ZTEST(virtio_pci_status, test_status_round_trip_through_transport_api)
{
	const struct device *dev = DEVICE_DT_GET(TEST_NODE);
	uint8_t expected_status;
	int ret;

	zassert_equal(TEST_PCIE_DEV.bdf, TEST_BDF, "synthetic PCI endpoint was not discovered");
	zassert_false(device_is_ready(dev), "deferred VirtIO PCI device unexpectedly ready");

	ret = device_init(dev);
	zassert_equal(ret, 0, "VirtIO PCI transport failed to initialize");
	zassert_true(device_is_ready(dev), "VirtIO PCI transport did not become ready");

	expected_status = TEST_STATUS_ACKNOWLEDGE | TEST_STATUS_DRIVER;
	zassert_equal(common_page.cfg.device_status, expected_status,
		      "initial PCI status bits were not written as bytes");

	zassert_equal(virtio_commit_feature_bits(dev), 0,
		      "FEATURES_OK status bit did not round-trip through the PCI transport");
	expected_status |= TEST_STATUS_FEATURES_OK;
	zassert_equal(common_page.cfg.device_status, expected_status,
		      "FEATURES_OK status bit was not preserved in the byte-sized field");

	virtio_finalize_init(dev);
	expected_status |= TEST_STATUS_DRIVER_OK;
	zassert_equal(common_page.cfg.device_status, expected_status,
		      "DRIVER_OK status bit was not preserved in the byte-sized field");
}

ZTEST_SUITE(virtio_pci_status, NULL, NULL, before, NULL, NULL);
