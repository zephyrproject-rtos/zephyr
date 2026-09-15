/*
 * SPDX-FileCopyrightText: 2026 The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/pcie/cap.h>
#include <zephyr/drivers/pcie/controller.h>
#include <zephyr/drivers/pcie/pcie.h>
#include <zephyr/drivers/virtio.h>
#include <zephyr/drivers/virtio/virtio_config.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#define TEST_NODE      DT_NODELABEL(virtio_pci_test)
#define TEST_CTRL_NODE DT_NODELABEL(pcie_test)
#define TEST_PCIE_DEV  Z_DEVICE_PCIE_NAME(TEST_NODE)
#define TEST_BDF      PCIE_BDF(0, 1, 0)

#define TEST_BAR0_NODE DT_NODELABEL(test_bar0)
#define TEST_BAR1_NODE DT_NODELABEL(test_bar1)
#define TEST_BAR2_NODE DT_NODELABEL(test_bar2)

#define TEST_COMMON_NODE DT_NODELABEL(test_common_cfg)
#define TEST_ISR_NODE    DT_NODELABEL(test_isr_cfg)
#define TEST_NOTIFY_NODE DT_NODELABEL(test_notify_cfg)

#define TEST_FIRST_CAP      0x40U
#define TEST_ISR_CAP        0x50U
#define TEST_NOTIFY_CAP     0x60U
#define TEST_CAP_COMMON_CFG 1U
#define TEST_CAP_NOTIFY_CFG 2U
#define TEST_CAP_ISR_CFG    3U

#define TEST_COMMON_DEVICE_FEATURE_OFFSET 4U
#define TEST_PAGE_SIZE                    4096U

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

extern struct pcie_dev TEST_PCIE_DEV;

static uint8_t bar0[DT_REG_SIZE(TEST_BAR0_NODE)] __aligned(TEST_PAGE_SIZE);
static uint8_t bar1[DT_REG_SIZE(TEST_BAR1_NODE)] __aligned(TEST_PAGE_SIZE);
static uint8_t bar2[DT_REG_SIZE(TEST_BAR2_NODE)] __aligned(TEST_PAGE_SIZE);
static bool bar_probe[3];
static uint32_t command_status = PCIE_CONF_CMDSTAT_CAPS;
static struct test_virtio_pci_cap common_cap;
static struct test_virtio_pci_cap isr_cap;
static struct test_virtio_pci_notify_cap notify_cap;

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
		return bar0;
	case 1:
		return bar1;
	case 2:
		return bar2;
	default:
		return NULL;
	}
}

static size_t test_bar_size(unsigned int bar_index)
{
	switch (bar_index) {
	case 0:
		return sizeof(bar0);
	case 1:
		return sizeof(bar1);
	case 2:
		return sizeof(bar2);
	default:
		return 0U;
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
			return ~((uint32_t)test_bar_size(bar_index) - 1U);
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

void __wrap_arch_irq_enable(unsigned int irq)
{
	ARG_UNUSED(irq);
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	memset(bar0, 0, sizeof(bar0));
	memset(bar1, 0, sizeof(bar1));
	memset(bar2, 0, sizeof(bar2));
	memset(bar_probe, 0, sizeof(bar_probe));
	command_status = PCIE_CONF_CMDSTAT_CAPS;

	common_cap = (struct test_virtio_pci_cap){
		.cap_vndr = PCI_CAP_ID_VNDR,
		.cap_next = TEST_ISR_CAP,
		.cap_len = sizeof(common_cap),
		.cfg_type = TEST_CAP_COMMON_CFG,
		.bar = 0U,
		.offset = sys_cpu_to_le32(DT_REG_ADDR(TEST_COMMON_NODE)),
		.length = sys_cpu_to_le32(DT_REG_SIZE(TEST_COMMON_NODE)),
	};
	isr_cap = (struct test_virtio_pci_cap){
		.cap_vndr = PCI_CAP_ID_VNDR,
		.cap_next = TEST_NOTIFY_CAP,
		.cap_len = sizeof(isr_cap),
		.cfg_type = TEST_CAP_ISR_CFG,
		.bar = 1U,
		.offset = sys_cpu_to_le32(DT_REG_ADDR(TEST_ISR_NODE)),
		.length = sys_cpu_to_le32(DT_REG_SIZE(TEST_ISR_NODE)),
	};
	notify_cap = (struct test_virtio_pci_notify_cap){
		.cap = {
			.cap_vndr = PCI_CAP_ID_VNDR,
			.cap_next = 0U,
			.cap_len = sizeof(notify_cap),
			.cfg_type = TEST_CAP_NOTIFY_CFG,
			.bar = 2U,
			.offset = sys_cpu_to_le32(DT_REG_ADDR(TEST_NOTIFY_NODE)),
			.length = sys_cpu_to_le32(DT_REG_SIZE(TEST_NOTIFY_NODE)),
		},
		.notify_off_multiplier = sys_cpu_to_le32(0U),
	};

	sys_put_le32(BIT(0),
		     &bar0[DT_REG_ADDR(TEST_COMMON_NODE) + TEST_COMMON_DEVICE_FEATURE_OFFSET]);
}

ZTEST(virtio_pci_cap_endianness, test_cap_layout_through_transport_init)
{
	const struct device *dev = DEVICE_DT_GET(TEST_NODE);
	int ret;

	zassert_equal(TEST_PCIE_DEV.bdf, TEST_BDF, "fake PCI endpoint was not discovered");
	zassert_false(device_is_ready(dev), "deferred VirtIO PCI device unexpectedly ready");

	ret = device_init(dev);
	zassert_ok(ret, "VirtIO PCI transport failed to map little-endian capabilities");
	zassert_true(device_is_ready(dev), "VirtIO PCI transport did not become ready");
	zassert_true(virtio_read_device_feature_bit(dev, VIRTIO_F_VERSION_1),
		     "common capability did not map the advertised feature register");
}

ZTEST_SUITE(virtio_pci_cap_endianness, NULL, NULL, before, NULL, NULL);
