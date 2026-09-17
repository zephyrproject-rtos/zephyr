/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/ztest.h>
#include <zephyr/device.h>

#define DT_DRV_COMPAT	fakedriver

/*
 * Driver with a single MMIO region to manage
 */

struct foo_single_dev_data {
	DEVICE_MMIO_RAM;
	int baz;
};

struct foo_single_dev_data foo0_data;

struct foo_single_config_info {
	DEVICE_MMIO_ROM;
};

const struct foo_single_config_info foo0_config = {
	DEVICE_MMIO_ROM_INIT(DT_DRV_INST(0)),
};

int foo_single_init(const struct device *dev)
{
	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	return 0;
}

/* fake API pointer, we don't use it at all for this suite */
DEVICE_DEFINE(foo0, "foo0", foo_single_init, NULL,
		&foo0_data, &foo0_config,
		POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		(void *)0xDEADBEEF);

/**
 * @brief Test DEVICE_MMIO_* macros
 *
 * We show that we can make mapping calls and that the address returned by
 * DEVICE_MMIO_GET() is not NULL, indicating that the kernel mapped
 * stuff somewhere.
 *
 * We also perform some checks depending on configuration:
 * - If MMIO addresses are maintained in RAM, check that the ROM struct
 *   was populated correctly.
 * - If MMIO addresses are maintained in ROM, check that the DTS info,
 *   the ROM region, and the result of DEVICE_MMIO_GET() all
 *   point to the same address. We show that no extra memory is used in
 *   dev_data.
 *
 * @ingroup kernel_device_tests
 */
ZTEST(device, test_mmio_single)
{
	const struct z_device_mmio_rom *rom;
	const struct device *dev = device_get_binding("foo0");
	mm_reg_t regs;

	zassert_not_null(dev, "null foo0");

	regs = DEVICE_MMIO_GET(dev);
	rom = DEVICE_MMIO_ROM_PTR(dev);

	/* A sign that something didn't get initialized, shouldn't ever
	 * be 0
	 */
	zassert_not_equal(regs, 0, "NULL regs");

#ifdef DEVICE_MMIO_IS_IN_RAM
	/* The config info should just contain the addr/size from DTS.
	 * The best we can check with 'regs' is that it's nonzero, as if
	 * an MMU is enabled, the kernel chooses the virtual address to
	 * place it at. We don't otherwise look at `regs`; other tests will
	 * prove that k_map() actually works.
	 */
	zassert_equal(rom->phys_addr, DT_INST_REG_ADDR(0), "bad phys_addr");
	zassert_equal(rom->size, DT_INST_REG_SIZE(0), "bad size");
#else
	/* Config info contains base address, which should be the base
	 * address from DTS, and regs should have the same value.
	 * In this configuration dev_data has nothing mmio-related in it
	 */
	zassert_equal(rom->addr, DT_INST_REG_ADDR(0), "bad addr");
	zassert_equal(regs, rom->addr, "bad regs");
	/* Just the baz member */
	zassert_equal(sizeof(struct foo_single_dev_data), sizeof(int),
		      "too big foo_single_dev_data");
#endif
}

/*
 * Driver with multiple MMIO regions to manage
 */

struct foo_mult_dev_data {
	int baz;

	DEVICE_MMIO_NAMED_RAM(corge);
	DEVICE_MMIO_NAMED_RAM(grault);
};

struct foo_mult_dev_data foo12_data;

struct foo_mult_config_info {
	DEVICE_MMIO_NAMED_ROM(corge);
	DEVICE_MMIO_NAMED_ROM(grault);
};

const struct foo_mult_config_info foo12_config = {
	DEVICE_MMIO_NAMED_ROM_INIT(corge, DT_DRV_INST(1)),
	DEVICE_MMIO_NAMED_ROM_INIT(grault, DT_DRV_INST(2))
};

#define DEV_DATA(dev)	((struct foo_mult_dev_data *)((dev)->data))
#define DEV_CFG(dev)	((struct foo_mult_config_info *)((dev)->config))

int foo_mult_init(const struct device *dev)
{
	DEVICE_MMIO_NAMED_MAP(dev, corge, K_MEM_CACHE_NONE);
	DEVICE_MMIO_NAMED_MAP(dev, grault, K_MEM_CACHE_NONE);

	return 0;
}

DEVICE_DEFINE(foo12, "foo12", foo_mult_init, NULL,
		&foo12_data, &foo12_config,
		POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		(void *)0xDEADBEEF);

/**
 * @brief Test DEVICE_MMIO_NAMED_* macros
 *
 * We show that we can make mapping calls and that the address returned by
 * DEVICE_MMIO_NAMED_GET() is not NULL, indicating that the kernel mapped
 * stuff somewhere.
 *
 * We show that this works for a device instance that has two named regions,
 * 'corge' and 'grault' that respectively come from DTS instances 1 and 2.
 *
 * We also perform some checks depending on configuration:
 * - If MMIO addresses are maintained in RAM, check that the ROM struct
 *   was populated correctly.
 * - If MMIO addresses are maintained in ROM, check that the DTS info,
 *   the ROM region, and the result of DEVICE_MMIO_NAMED_GET() all
 *   point to the same address. We show that no extra memory is used in
 *   dev_data.
 *
 * @ingroup kernel_device_tests
 */
ZTEST(device, test_mmio_multiple)
{
	/* See comments for test_mmio_single */
	const struct device *dev = device_get_binding("foo12");
	mm_reg_t regs_corge, regs_grault;
	const struct z_device_mmio_rom *rom_corge, *rom_grault;

	zassert_not_null(dev, "null foo12");

	regs_corge = DEVICE_MMIO_NAMED_GET(dev, corge);
	regs_grault = DEVICE_MMIO_NAMED_GET(dev, grault);
	rom_corge = DEVICE_MMIO_NAMED_ROM_PTR(dev, corge);
	rom_grault = DEVICE_MMIO_NAMED_ROM_PTR(dev, grault);

	zassert_not_equal(regs_corge, 0, "bad regs_corge");
	zassert_not_equal(regs_grault, 0, "bad regs_grault");

#ifdef DEVICE_MMIO_IS_IN_RAM
	zassert_equal(rom_corge->phys_addr, DT_INST_REG_ADDR(1),
		      "bad phys_addr (corge)");
	zassert_equal(rom_corge->size, DT_INST_REG_SIZE(1),
		      "bad size (corge)");
	zassert_equal(rom_grault->phys_addr, DT_INST_REG_ADDR(2),
		      "bad phys_addr (grault)");
	zassert_equal(rom_grault->size, DT_INST_REG_SIZE(2),
		      "bad size (grault)");
#else
	zassert_equal(rom_corge->addr, DT_INST_REG_ADDR(1),
		      "bad addr (corge)");
	zassert_equal(regs_corge, rom_corge->addr, "bad regs (corge)");
	zassert_equal(rom_grault->addr, DT_INST_REG_ADDR(2),
		      "bad addr (grault)");
	zassert_equal(regs_grault, rom_grault->addr, "bad regs (grault)");
	zassert_equal(sizeof(struct foo_mult_dev_data), sizeof(int),
		      "too big foo_mult_dev_data");
#endif
}

/*
 * RAM buffer standing in for an MMIO region, so that the register access
 * helpers can be exercised on any platform. 64-bit aligned for the 64-bit
 * accessors.
 *
 * sys_read64()/sys_write64() are only guaranteed on 64-bit targets, so the
 * 64-bit helpers are only exercised there.
 */
static uint64_t fake_regs[4];

#define FAKE_REG8	1U
#define FAKE_REG16	2U
#define FAKE_REG32	4U
#define FAKE_REG64	8U
#define FAKE_REG_BITS	16U

static void check_fake_regs(uint8_t *regs)
{
	zassert_equal(regs[FAKE_REG8], 0xA5, "bad 8-bit write");
	zassert_equal(*(uint16_t *)&regs[FAKE_REG16], 0x1234, "bad 16-bit write");
	zassert_equal(*(uint32_t *)&regs[FAKE_REG32], 0xDEADBEEF, "bad 32-bit write");
#ifdef CONFIG_64BIT
	zassert_equal(*(uint64_t *)&regs[FAKE_REG64], 0xFEDCBA9876543210ULL,
		      "bad 64-bit write");
#endif
}

/**
 * @brief Test DEVICE_MMIO_* register access macros
 *
 * A device object whose MMIO storage points at a RAM buffer is built at
 * runtime, then each accessor is checked against the buffer contents.
 *
 * @ingroup kernel_device_tests
 */
ZTEST(device, test_mmio_single_access)
{
	uint8_t *regs = (uint8_t *)fake_regs;
	uint32_t *bits = (uint32_t *)&regs[FAKE_REG_BITS];
#ifdef DEVICE_MMIO_IS_IN_RAM
	mm_reg_t mmio_ram = (mm_reg_t)fake_regs;
	struct device fake = { .data = &mmio_ram };
#else
	struct z_device_mmio_rom mmio_rom = { .addr = (mm_reg_t)fake_regs };
	struct device fake = { .config = &mmio_rom };
#endif
	const struct device *dev = &fake;

	memset(fake_regs, 0, sizeof(fake_regs));

	DEVICE_MMIO_WRITE8(dev, FAKE_REG8, 0xA5);
	DEVICE_MMIO_WRITE16(dev, FAKE_REG16, 0x1234);
	DEVICE_MMIO_WRITE32(dev, FAKE_REG32, 0xDEADBEEF);
#ifdef CONFIG_64BIT
	DEVICE_MMIO_WRITE64(dev, FAKE_REG64, 0xFEDCBA9876543210ULL);
#endif
	check_fake_regs(regs);

	zassert_equal(DEVICE_MMIO_READ8(dev, FAKE_REG8), 0xA5, "bad 8-bit read");
	zassert_equal(DEVICE_MMIO_READ16(dev, FAKE_REG16), 0x1234, "bad 16-bit read");
	zassert_equal(DEVICE_MMIO_READ32(dev, FAKE_REG32), 0xDEADBEEF, "bad 32-bit read");
#ifdef CONFIG_64BIT
	zassert_equal(DEVICE_MMIO_READ64(dev, FAKE_REG64), 0xFEDCBA9876543210ULL,
		      "bad 64-bit read");
#endif

	DEVICE_MMIO_SET_BIT(dev, FAKE_REG_BITS, 3);
	zassert_equal(*bits, BIT(3), "bad set_bit");
	zassert_not_equal(DEVICE_MMIO_TEST_BIT(dev, FAKE_REG_BITS, 3), 0, "bad test_bit");
	zassert_equal(DEVICE_MMIO_TEST_BIT(dev, FAKE_REG_BITS, 4), 0, "bad test_bit");
	DEVICE_MMIO_CLEAR_BIT(dev, FAKE_REG_BITS, 3);
	zassert_equal(*bits, 0, "bad clear_bit");

	zassert_equal(DEVICE_MMIO_TEST_AND_SET_BIT(dev, FAKE_REG_BITS, 5), 0,
		      "bad test_and_set_bit");
	zassert_equal(*bits, BIT(5), "bad test_and_set_bit");
	zassert_not_equal(DEVICE_MMIO_TEST_AND_CLEAR_BIT(dev, FAKE_REG_BITS, 5), 0,
			  "bad test_and_clear_bit");
	zassert_equal(*bits, 0, "bad test_and_clear_bit");

	DEVICE_MMIO_SET_BITS(dev, FAKE_REG_BITS, 0xF0F0);
	zassert_equal(*bits, 0xF0F0, "bad set_bits");
	DEVICE_MMIO_CLEAR_BITS(dev, FAKE_REG_BITS, 0x00F0);
	zassert_equal(*bits, 0xF000, "bad clear_bits");
}

/**
 * @brief Test DEVICE_MMIO_NAMED_* register access macros
 *
 * Same as test_mmio_single_access, for the named region 'corge'.
 *
 * @ingroup kernel_device_tests
 */
ZTEST(device, test_mmio_named_access)
{
	uint8_t *regs = (uint8_t *)fake_regs;
	uint32_t *bits = (uint32_t *)&regs[FAKE_REG_BITS];
#ifdef DEVICE_MMIO_IS_IN_RAM
	struct foo_mult_dev_data data = { .corge = (mm_reg_t)fake_regs };
	struct device fake = { .data = &data };
#else
	struct foo_mult_config_info config = { .corge = { .addr = (mm_reg_t)fake_regs } };
	struct device fake = { .config = &config };
#endif
	const struct device *dev = &fake;

	memset(fake_regs, 0, sizeof(fake_regs));

	DEVICE_MMIO_NAMED_WRITE8(dev, corge, FAKE_REG8, 0xA5);
	DEVICE_MMIO_NAMED_WRITE16(dev, corge, FAKE_REG16, 0x1234);
	DEVICE_MMIO_NAMED_WRITE32(dev, corge, FAKE_REG32, 0xDEADBEEF);
#ifdef CONFIG_64BIT
	DEVICE_MMIO_NAMED_WRITE64(dev, corge, FAKE_REG64, 0xFEDCBA9876543210ULL);
#endif
	check_fake_regs(regs);

	zassert_equal(DEVICE_MMIO_NAMED_READ8(dev, corge, FAKE_REG8), 0xA5, "bad 8-bit read");
	zassert_equal(DEVICE_MMIO_NAMED_READ16(dev, corge, FAKE_REG16), 0x1234,
		      "bad 16-bit read");
	zassert_equal(DEVICE_MMIO_NAMED_READ32(dev, corge, FAKE_REG32), 0xDEADBEEF,
		      "bad 32-bit read");
#ifdef CONFIG_64BIT
	zassert_equal(DEVICE_MMIO_NAMED_READ64(dev, corge, FAKE_REG64), 0xFEDCBA9876543210ULL,
		      "bad 64-bit read");
#endif

	DEVICE_MMIO_NAMED_SET_BIT(dev, corge, FAKE_REG_BITS, 3);
	zassert_equal(*bits, BIT(3), "bad set_bit");
	zassert_not_equal(DEVICE_MMIO_NAMED_TEST_BIT(dev, corge, FAKE_REG_BITS, 3), 0,
			  "bad test_bit");
	zassert_equal(DEVICE_MMIO_NAMED_TEST_BIT(dev, corge, FAKE_REG_BITS, 4), 0,
		      "bad test_bit");
	DEVICE_MMIO_NAMED_CLEAR_BIT(dev, corge, FAKE_REG_BITS, 3);
	zassert_equal(*bits, 0, "bad clear_bit");

	zassert_equal(DEVICE_MMIO_NAMED_TEST_AND_SET_BIT(dev, corge, FAKE_REG_BITS, 5), 0,
		      "bad test_and_set_bit");
	zassert_equal(*bits, BIT(5), "bad test_and_set_bit");
	zassert_not_equal(DEVICE_MMIO_NAMED_TEST_AND_CLEAR_BIT(dev, corge, FAKE_REG_BITS, 5), 0,
			  "bad test_and_clear_bit");
	zassert_equal(*bits, 0, "bad test_and_clear_bit");

	DEVICE_MMIO_NAMED_SET_BITS(dev, corge, FAKE_REG_BITS, 0xF0F0);
	zassert_equal(*bits, 0xF0F0, "bad set_bits");
	DEVICE_MMIO_NAMED_CLEAR_BITS(dev, corge, FAKE_REG_BITS, 0x00F0);
	zassert_equal(*bits, 0xF000, "bad clear_bits");
}

/*
 * Not using driver model, toplevel definition
 */
DEVICE_MMIO_TOPLEVEL(foo3, DT_DRV_INST(3));
DEVICE_MMIO_TOPLEVEL_STATIC(foo4, DT_DRV_INST(4));

/**
 * @brief Test DEVICE_MMIO_TOPLEVEL_* macros
 *
 * We show that we can make mapping calls and that the address returned by
 * DEVICE_MMIO_TOPLEVEL_GET() is not NULL, indicating that the kernel mapped
 * stuff somewhere.
 *
 * We do this for two different MMIO toplevel instances; one declared
 * statically and one not.
 *
 * We also perform some checks depending on configuration:
 * - If MMIO addresses are maintained in RAM, check that the ROM struct
 *   was populated correctly.
 * - If MMIO addresses are maintained in ROM, check that the DTS info,
 *   the ROM region, and the result of DEVICE_MMIO_TOPLEVEL_GET() all
 *   point to the same address
 *
 * @ingroup kernel_device_tests
 */
ZTEST(device, test_mmio_toplevel)
{
	mm_reg_t regs_foo3, regs_foo4;
	const struct z_device_mmio_rom *rom_foo3, *rom_foo4;

	DEVICE_MMIO_TOPLEVEL_MAP(foo3, K_MEM_CACHE_NONE);
	DEVICE_MMIO_TOPLEVEL_MAP(foo4, K_MEM_CACHE_NONE);

	regs_foo3 = DEVICE_MMIO_TOPLEVEL_GET(foo3);
	regs_foo4 = DEVICE_MMIO_TOPLEVEL_GET(foo4);
	rom_foo3 = DEVICE_MMIO_TOPLEVEL_ROM_PTR(foo3);
	rom_foo4 = DEVICE_MMIO_TOPLEVEL_ROM_PTR(foo4);

	zassert_not_equal(regs_foo3, 0, "bad regs_corge");
	zassert_not_equal(regs_foo4, 0, "bad regs_grault");

#ifdef DEVICE_MMIO_IS_IN_RAM
	zassert_equal(rom_foo3->phys_addr, DT_INST_REG_ADDR(3),
		      "bad phys_addr (foo3)");
	zassert_equal(rom_foo3->size, DT_INST_REG_SIZE(3),
		      "bad size (foo3)");
	zassert_equal(rom_foo4->phys_addr, DT_INST_REG_ADDR(4),
		      "bad phys_addr (foo4)");
	zassert_equal(rom_foo4->size, DT_INST_REG_SIZE(4),
		      "bad size (foo4)");
#else
	zassert_equal(rom_foo3->addr, DT_INST_REG_ADDR(3),
		      "bad addr (foo3)");
	zassert_equal(regs_foo3, rom_foo3->addr, "bad regs (foo3)");
	zassert_equal(rom_foo4->addr, DT_INST_REG_ADDR(4),
		      "bad addr (foo4)");
	zassert_equal(regs_foo4, rom_foo4->addr, "bad regs (foo4)");
#endif
}

/**
 * @brief Test DEVICE_MMIO_TOPLEVEL_* register access macros
 *
 * Same as test_mmio_single_access, for the toplevel region 'foo3'. The
 * toplevel address can only be redirected to the RAM buffer when MMIO
 * addresses are maintained in RAM, so the test is skipped otherwise.
 *
 * @ingroup kernel_device_tests
 */
ZTEST(device, test_mmio_toplevel_access)
{
#ifdef DEVICE_MMIO_IS_IN_RAM
	uint8_t *regs = (uint8_t *)fake_regs;
	uint32_t *bits = (uint32_t *)&regs[FAKE_REG_BITS];

	*DEVICE_MMIO_TOPLEVEL_RAM_PTR(foo3) = (mm_reg_t)fake_regs;
	memset(fake_regs, 0, sizeof(fake_regs));

	DEVICE_MMIO_TOPLEVEL_WRITE8(foo3, FAKE_REG8, 0xA5);
	DEVICE_MMIO_TOPLEVEL_WRITE16(foo3, FAKE_REG16, 0x1234);
	DEVICE_MMIO_TOPLEVEL_WRITE32(foo3, FAKE_REG32, 0xDEADBEEF);
#ifdef CONFIG_64BIT
	DEVICE_MMIO_TOPLEVEL_WRITE64(foo3, FAKE_REG64, 0xFEDCBA9876543210ULL);
#endif
	check_fake_regs(regs);

	zassert_equal(DEVICE_MMIO_TOPLEVEL_READ8(foo3, FAKE_REG8), 0xA5, "bad 8-bit read");
	zassert_equal(DEVICE_MMIO_TOPLEVEL_READ16(foo3, FAKE_REG16), 0x1234,
		      "bad 16-bit read");
	zassert_equal(DEVICE_MMIO_TOPLEVEL_READ32(foo3, FAKE_REG32), 0xDEADBEEF,
		      "bad 32-bit read");
#ifdef CONFIG_64BIT
	zassert_equal(DEVICE_MMIO_TOPLEVEL_READ64(foo3, FAKE_REG64), 0xFEDCBA9876543210ULL,
		      "bad 64-bit read");
#endif

	DEVICE_MMIO_TOPLEVEL_SET_BIT(foo3, FAKE_REG_BITS, 3);
	zassert_equal(*bits, BIT(3), "bad set_bit");
	zassert_not_equal(DEVICE_MMIO_TOPLEVEL_TEST_BIT(foo3, FAKE_REG_BITS, 3), 0,
			  "bad test_bit");
	zassert_equal(DEVICE_MMIO_TOPLEVEL_TEST_BIT(foo3, FAKE_REG_BITS, 4), 0, "bad test_bit");
	DEVICE_MMIO_TOPLEVEL_CLEAR_BIT(foo3, FAKE_REG_BITS, 3);
	zassert_equal(*bits, 0, "bad clear_bit");

	zassert_equal(DEVICE_MMIO_TOPLEVEL_TEST_AND_SET_BIT(foo3, FAKE_REG_BITS, 5), 0,
		      "bad test_and_set_bit");
	zassert_equal(*bits, BIT(5), "bad test_and_set_bit");
	zassert_not_equal(DEVICE_MMIO_TOPLEVEL_TEST_AND_CLEAR_BIT(foo3, FAKE_REG_BITS, 5), 0,
			  "bad test_and_clear_bit");
	zassert_equal(*bits, 0, "bad test_and_clear_bit");

	DEVICE_MMIO_TOPLEVEL_SET_BITS(foo3, FAKE_REG_BITS, 0xF0F0);
	zassert_equal(*bits, 0xF0F0, "bad set_bits");
	DEVICE_MMIO_TOPLEVEL_CLEAR_BITS(foo3, FAKE_REG_BITS, 0x00F0);
	zassert_equal(*bits, 0xF000, "bad clear_bits");
#else
	ztest_test_skip();
#endif
}

/**
 * @brief device_map() test
 *
 * Show that device_map() populates a memory address. We don't do anything else;
 * tests for k_map() will prove that virtual memory mapping actually works.
 */
ZTEST(device, test_mmio_device_map)
{
#ifdef DEVICE_MMIO_IS_IN_RAM
	mm_reg_t regs = 0;

	device_map(&regs, 0xF0000000, 0x1000, K_MEM_CACHE_NONE);

	zassert_not_equal(regs, 0, "bad regs");

	device_unmap(regs, 0x1000);
#else
	ztest_test_skip();
#endif
}
