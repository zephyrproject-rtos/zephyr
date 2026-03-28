/*
 * Copyright (c) 2020 Intel Corporation
 * Copyright (c) 2022 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/device.h>

#define DT_DRV_COMPAT	fakedriver_multireg

/*
 * Driver with multiple MMIO regions to manage defined into DT
 */

struct foo_multireg_dev_data {
	int baz;

	DEVICE_MMIO_NAMED_RAM(chip);
	DEVICE_MMIO_NAMED_RAM(dale);
};

struct foo_multireg_dev_data foo_multireg_data;

struct foo_multireg_config_info {
	DEVICE_MMIO_NAMED_ROM(chip);
	DEVICE_MMIO_NAMED_ROM(dale);
};

const struct foo_multireg_config_info foo_multireg_config = {
	DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(chip, DT_DRV_INST(0)),
	DEVICE_MMIO_NAMED_ROM_INIT_BY_NAME(dale, DT_DRV_INST(0))
};

#define DEV_DATA(dev)	((struct foo_multireg_dev_data *)((dev)->data))
#define DEV_CFG(dev)	((struct foo_multireg_config_info *)((dev)->config))

int foo_multireg_init(const struct device *dev)
{
	DEVICE_MMIO_NAMED_MAP(dev, chip, K_MEM_CACHE_NONE);
	DEVICE_MMIO_NAMED_MAP(dev, dale, K_MEM_CACHE_NONE);

	return 0;
}

DEVICE_DEFINE(foo_multireg, "foo_multireg", foo_multireg_init, NULL,
		&foo_multireg_data, &foo_multireg_config,
		POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		(void *)0xDEADBEEF);
/**
 * @brief Test DEVICE_MMIO_NAMED_* macros
 *
 * This is the same as the test_mmio_multiple test but in this test the
 * memory regions are created by the named DT property 'reg'.
 *
 * @see test_mmio_multiple
 *
 * @ingroup kernel_device_tests
 */
ZTEST(device, test_mmio_multireg)
{
	const struct device *dev = device_get_binding("foo_multireg");
	mm_reg_t regs_chip, regs_dale;
	const struct z_device_mmio_rom *rom_chip, *rom_dale;

	zassert_not_null(dev, "null foo_multireg");

	regs_chip = DEVICE_MMIO_NAMED_GET(dev, chip);
	regs_dale = DEVICE_MMIO_NAMED_GET(dev, dale);
	rom_chip = DEVICE_MMIO_NAMED_ROM_PTR(dev, chip);
	rom_dale = DEVICE_MMIO_NAMED_ROM_PTR(dev, dale);

	zassert_not_equal(regs_chip, 0, "bad regs_chip");
	zassert_not_equal(regs_dale, 0, "bad regs_dale");

#ifdef DEVICE_MMIO_IS_IN_RAM
	zassert_equal(rom_chip->phys_addr, DT_INST_REG_ADDR_BY_NAME(0, chip),
		      "bad phys_addr (chip)");
	zassert_equal(rom_chip->size, DT_INST_REG_SIZE_BY_NAME(0, chip),
		      "bad size (chip)");
	zassert_equal(rom_dale->phys_addr, DT_INST_REG_ADDR_BY_NAME(0, dale),
		      "bad phys_addr (dale)");
	zassert_equal(rom_dale->size, DT_INST_REG_SIZE_BY_NAME(0, dale),
		      "bad size (dale)");
#else
	zassert_equal(rom_chip->addr, DT_INST_REG_ADDR_BY_NAME(0, chip),
		      "bad addr (chip)");
	zassert_equal(regs_chip, rom_chip->addr, "bad regs (chip)");
	zassert_equal(rom_dale->addr, DT_INST_REG_ADDR_BY_NAME(0, dale),
		      "bad addr (dale)");
	zassert_equal(regs_dale, rom_dale->addr, "bad regs (dale)");
	zassert_equal(sizeof(struct foo_multireg_dev_data), sizeof(int),
		      "too big foo_multireg_dev_data");
#endif
}
