/*
 * Copyright (c) 2026 Process Mission
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/interrupt_controller/gicv3_its.h>
#include <zephyr/drivers/pcie/cap.h>
#include <zephyr/drivers/pcie/msi.h>
#include <zephyr/kernel/mm.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#define TEST_BDF       PCIE_BDF(0, 1, 0)
#define TEST_IRQ       27U
#define TEST_PRIORITY  3U
#define TEST_DATA      0x41U
#define TEST_ADDRESS   0xfee01000U
#define TEST_ADDRESS64 0x12345678abcdef00ULL
#define MSI_BASE       0x10U
#define MSIX_BASE      0x20U

static uint32_t config_space[64];
static struct msix_vector table[2] __aligned(4);
static bool msi_present;
static bool msix_present;
static size_t write_count;
static size_t map_count;
static size_t mdr_count;
static size_t master_count;
static size_t allocation_count;
static uint64_t its_address;

uint32_t pcie_get_cap(pcie_bdf_t bdf, uint32_t cap_id)
{
	zassert_equal(bdf, TEST_BDF);
	zassert_true(cap_id == PCI_CAP_ID_MSI || cap_id == PCI_CAP_ID_MSIX);

	if (cap_id == PCI_CAP_ID_MSI) {
		return msi_present ? MSI_BASE : 0U;
	}

	return msix_present ? MSIX_BASE : 0U;
}

uint32_t pcie_conf_read(pcie_bdf_t bdf, unsigned int reg)
{
	zassert_equal(bdf, TEST_BDF);
	zassert_true(reg < ARRAY_SIZE(config_space));

	return config_space[reg];
}

void pcie_conf_write(pcie_bdf_t bdf, unsigned int reg, uint32_t value)
{
	zassert_equal(bdf, TEST_BDF);
	zassert_true(reg < ARRAY_SIZE(config_space));
	config_space[reg] = value;
	write_count++;
}

void pcie_set_cmd(pcie_bdf_t bdf, uint32_t bits, bool on)
{
	zassert_equal(bdf, TEST_BDF);
	zassert_equal(bits, PCIE_CONF_CMDSTAT_MASTER);
	zassert_true(on);
	master_count++;
}

uint64_t pcie_msi_map(unsigned int irq, msi_vector_t *vector, uint8_t n_vector)
{
	zassert_equal(irq, TEST_IRQ);
	zassert_not_null(vector);
	zassert_true(n_vector > 0U);
	map_count++;

	return vector->arch.address;
}

uint16_t pcie_msi_mdr(unsigned int irq, msi_vector_t *vector)
{
	zassert_equal(irq, TEST_IRQ);
	zassert_not_null(vector);
	mdr_count++;

	return vector->arch.eventid;
}

uint8_t arch_pcie_msi_vectors_allocate(unsigned int priority, msi_vector_t *vectors,
				       uint8_t n_vector)
{
	zassert_equal(priority, TEST_PRIORITY);
	allocation_count++;

	for (uint8_t i = 0U; i < n_vector; i++) {
		zassert_equal(vectors[i].bdf, TEST_BDF);
		vectors[i].arch.irq = TEST_IRQ;
		vectors[i].arch.address = TEST_ADDRESS;
		vectors[i].arch.eventid = TEST_DATA + i;
		vectors[i].arch.priority = priority;
	}

	return n_vector;
}

bool pcie_get_mbar(pcie_bdf_t bdf, unsigned int bar_index, struct pcie_bar *bar)
{
	zassert_equal(bdf, TEST_BDF);
	zassert_equal(bar_index, 0U);
	bar->phys_addr = (uintptr_t)table;
	bar->size = sizeof(table);

	return true;
}

void k_mem_map_phys_bare(uint8_t **virt_ptr, uintptr_t phys, size_t size, uint32_t flags)
{
	zassert_equal(phys, (uintptr_t)table);
	zassert_true(size <= sizeof(table));
	zassert_equal(flags, K_MEM_CACHE_NONE | K_MEM_PERM_RW);
	*virt_ptr = (uint8_t *)table;
}

/* The GICv3 ITS provider requires hardware; this exercises the API width. */
static uint64_t test_its_address(const struct device *dev)
{
	ARG_UNUSED(dev);

	return its_address;
}

static DEVICE_API(its, test_its_api) = {
	.get_msi_addr = test_its_address,
};

static const struct device test_its = {
	.api = &test_its_api,
};

static msi_vector_t test_vector(uint64_t address, uint16_t data)
{
	msi_vector_t vector = {0};

	vector.bdf = TEST_BDF;
	vector.arch.address = address;
	vector.arch.eventid = data;

	return vector;
}

static void assert_msi64(uint64_t address)
{
	zassert_equal(config_space[MSI_BASE + PCIE_MSI_MAP0], (uint32_t)address);
	zassert_equal(config_space[MSI_BASE + PCIE_MSI_MAP1_64], (uint32_t)(address >> 32));
	zassert_equal(config_space[MSI_BASE + PCIE_MSI_MDR_64], TEST_DATA);
	zassert_true((config_space[MSI_BASE] & PCIE_MSI_MCR_EN) != 0U);
	zassert_equal(master_count, 1U);
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);
	memset(config_space, 0, sizeof(config_space));
	memset(table, 0xa5, sizeof(table));
	msi_present = true;
	msix_present = false;
	write_count = 0U;
	map_count = 0U;
	mdr_count = 0U;
	master_count = 0U;
	allocation_count = 0U;
	its_address = TEST_ADDRESS64;
	config_space[MSI_BASE] = PCI_CAP_ID_MSI | PCIE_MSI_MCR_64 | (1U << PCIE_MSI_MCR_MMC_SHIFT);
	config_space[MSIX_BASE] = PCI_CAP_ID_MSIX | (1U << PCIE_MSIX_MCR_TSIZE_SHIFT);
	/*
	 * Poison the upper address word: in a 32-bit MSI layout this slot is
	 * MDR_32, which the tests either program or expect to stay untouched.
	 */
	config_space[MSI_BASE + PCIE_MSI_MAP1_64] = UINT32_MAX;
}

ZTEST(pcie_msi, test_allocator_low_address)
{
	msi_vector_t vectors[2] = {0};

	for (size_t i = 0U; i < ARRAY_SIZE(vectors); i++) {
		vectors[i].arch.address = UINT64_MAX;
	}

	zassert_equal(
		pcie_msi_vectors_allocate(TEST_BDF, TEST_PRIORITY, vectors, ARRAY_SIZE(vectors)),
		ARRAY_SIZE(vectors));
	zassert_equal(allocation_count, 1U);
	for (size_t i = 0U; i < ARRAY_SIZE(vectors); i++) {
		zassert_equal(vectors[i].arch.address, (uint64_t)TEST_ADDRESS);
	}

	zassert_true(pcie_msi_enable(TEST_BDF, vectors, ARRAY_SIZE(vectors), TEST_IRQ));
	assert_msi64(TEST_ADDRESS);
}

ZTEST(pcie_msi, test_msix_allocator_maps_table)
{
	msi_vector_t vectors[2] = {0};

	msix_present = true;
	for (size_t i = 0U; i < ARRAY_SIZE(vectors); i++) {
		vectors[i].arch.address = UINT64_MAX;
	}
	zassert_equal(
		pcie_msi_vectors_allocate(TEST_BDF, TEST_PRIORITY, vectors, ARRAY_SIZE(vectors)),
		ARRAY_SIZE(vectors));
	for (size_t i = 0U; i < ARRAY_SIZE(vectors); i++) {
		zassert_equal(vectors[i].arch.address, (uint64_t)TEST_ADDRESS);
		zassert_equal_ptr(vectors[i].msix_vector, &table[i]);
		zassert_true(vectors[i].msix);
	}
}

ZTEST(pcie_msi, test_its_low_address)
{
	its_address = TEST_ADDRESS;
	zassert_equal(its_get_msi_addr(&test_its), TEST_ADDRESS);
}

ZTEST(pcie_msi, test_its_high_address)
{
	zassert_equal(its_get_msi_addr(&test_its), TEST_ADDRESS64);
}

ZTEST(pcie_msi, test_msi32_low_address)
{
	msi_vector_t vector = test_vector(TEST_ADDRESS, TEST_DATA);

	config_space[MSI_BASE] &= ~PCIE_MSI_MCR_64;
	config_space[MSI_BASE + PCIE_MSI_MDR_64] = UINT32_MAX;
	zassert_true(pcie_msi_enable(TEST_BDF, &vector, 1U, TEST_IRQ));
	zassert_equal(config_space[MSI_BASE + PCIE_MSI_MAP0], TEST_ADDRESS);
	zassert_equal(config_space[MSI_BASE + PCIE_MSI_MDR_32], TEST_DATA);
	zassert_equal(config_space[MSI_BASE + PCIE_MSI_MDR_64], UINT32_MAX);
	zassert_true((config_space[MSI_BASE] & PCIE_MSI_MCR_EN) != 0U);
	zassert_equal(master_count, 1U);
}

ZTEST(pcie_msi, test_msi64_low_address_clears_upper)
{
	msi_vector_t vector = test_vector(TEST_ADDRESS, TEST_DATA);

	config_space[MSI_BASE + PCIE_MSI_MAP1_64] = UINT32_MAX;
	zassert_true(pcie_msi_enable(TEST_BDF, &vector, 1U, TEST_IRQ));
	assert_msi64(TEST_ADDRESS);
}

ZTEST(pcie_msi, test_no_capability_does_not_modify_device)
{
	msi_present = false;
	zassert_false(pcie_msi_enable(TEST_BDF, NULL, 1U, TEST_IRQ));
	zassert_equal(write_count, 0U);
	zassert_equal(map_count, 0U);
	zassert_equal(mdr_count, 0U);
	zassert_equal(master_count, 0U);
}

ZTEST(pcie_msi, test_msix_programs_each_vector)
{
	const uint64_t addresses[] = {TEST_ADDRESS64, TEST_ADDRESS};
	msi_vector_t vectors[ARRAY_SIZE(addresses)];

	msix_present = true;
	for (size_t i = 0U; i < ARRAY_SIZE(vectors); i++) {
		vectors[i] = test_vector(addresses[i], TEST_DATA + i);
		vectors[i].msix_vector = &table[i];
	}

	zassert_true(pcie_msi_enable(TEST_BDF, vectors, ARRAY_SIZE(vectors), TEST_IRQ));
	for (size_t i = 0U; i < ARRAY_SIZE(vectors); i++) {
		zassert_equal(table[i].msg_addr, (uint32_t)addresses[i]);
		zassert_equal(table[i].msg_up_addr, (uint32_t)(addresses[i] >> 32));
		zassert_equal(table[i].msg_data, TEST_DATA + i);
		zassert_equal(table[i].vector_ctrl, 0U);
	}
	zassert_true((config_space[MSIX_BASE] & PCIE_MSIX_MCR_EN) != 0U);
	zassert_equal(map_count, ARRAY_SIZE(vectors));
	zassert_equal(mdr_count, ARRAY_SIZE(vectors));
	zassert_equal(master_count, 1U);
}

ZTEST(pcie_msi, test_msi64_preserves_its_address)
{
	uint64_t address = its_get_msi_addr(&test_its);
	msi_vector_t vector = test_vector(address, TEST_DATA);

	zassert_true(pcie_msi_enable(TEST_BDF, &vector, 1U, TEST_IRQ));
	assert_msi64(TEST_ADDRESS64);
}

ZTEST(pcie_msi, test_msi32_rejects_high_address_without_writes)
{
	msi_vector_t vector = test_vector(TEST_ADDRESS64, TEST_DATA);
	uint32_t saved_config[ARRAY_SIZE(config_space)];

	config_space[MSI_BASE] &= ~PCIE_MSI_MCR_64;
	config_space[MSI_BASE] |= PCIE_MSI_MCR_EN;
	config_space[MSI_BASE + PCIE_MSI_MAP0] = TEST_ADDRESS;
	config_space[MSI_BASE + PCIE_MSI_MDR_32] = TEST_DATA;
	config_space[PCIE_CONF_CMDSTAT] |= PCIE_CONF_CMDSTAT_MASTER;
	memcpy(saved_config, config_space, sizeof(saved_config));
	zassert_false(pcie_msi_enable(TEST_BDF, &vector, 1U, TEST_IRQ));
	zassert_mem_equal(config_space, saved_config, sizeof(saved_config));
	zassert_equal(write_count, 0U);
	zassert_equal(mdr_count, 0U);
	zassert_equal(master_count, 0U);
}

ZTEST(pcie_msi, test_msi32_accepts_last_32bit_address)
{
	msi_vector_t vector = test_vector(0xfffffffcULL, TEST_DATA);

	config_space[MSI_BASE] &= ~PCIE_MSI_MCR_64;
	zassert_true(pcie_msi_enable(TEST_BDF, &vector, 1U, TEST_IRQ));
	zassert_equal(config_space[MSI_BASE + PCIE_MSI_MAP0], 0xfffffffcU);
}

ZTEST(pcie_msi, test_msi32_rejects_4gib_address)
{
	msi_vector_t vector = test_vector(0x100000000ULL, TEST_DATA);

	config_space[MSI_BASE] &= ~PCIE_MSI_MCR_64;
	zassert_false(pcie_msi_enable(TEST_BDF, &vector, 1U, TEST_IRQ));
	zassert_equal(write_count, 0U);
	zassert_equal(mdr_count, 0U);
	zassert_equal(master_count, 0U);
}

ZTEST(pcie_msi, test_msi64_accepts_4gib_address)
{
	msi_vector_t vector = test_vector(0x100000000ULL, TEST_DATA);

	zassert_true(pcie_msi_enable(TEST_BDF, &vector, 1U, TEST_IRQ));
	assert_msi64(0x100000000ULL);
}

ZTEST_SUITE(pcie_msi, NULL, NULL, before, NULL, NULL);
