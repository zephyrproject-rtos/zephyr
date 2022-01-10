/*
 * Copyright (c) 2021 BayLibre SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pci_edu_test, LOG_LEVEL_INF);

#include <zephyr/types.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>

#include <zephyr/ztest.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pcie/pcie.h>
#ifdef CONFIG_PCIE_MSI
#include <zephyr/drivers/pcie/msi.h>
#endif

#define DT_DRV_COMPAT qemu_pci_edu

struct pcie_edu_config {
	pcie_bdf_t pcie_bdf;
	pcie_id_t pcie_id;
};

struct pcie_edu_data {
	struct pcie_mbar mem_bar;
	mm_reg_t mem_addr;
#ifdef CONFIG_PCIE_MSI
	struct msi_vector msi;
#endif
};

typedef int (*pcie_edu_get_id_t)(const struct device *dev, uint32_t *id);
typedef int (*pcie_edu_check_liveness_t)(const struct device *dev);
#ifdef CONFIG_PCIE_MSI
typedef int (*pcie_edu_test_msi_t)(const struct device *dev);
typedef uint32_t (*pcie_edu_calc_fact_t)(const struct device *dev, uint32_t val);
#endif
typedef void (*pcie_edu_dma_copy_t)(const struct device *dev, void *buffer, size_t size,
				    bool write);

__subsystem struct pcie_edu_driver_api {
	pcie_edu_get_id_t get_id;
	pcie_edu_check_liveness_t check_liveness;
#ifdef CONFIG_PCIE_MSI
	pcie_edu_test_msi_t test_msi;
	pcie_edu_calc_fact_t calc_fact;
#endif
	pcie_edu_dma_copy_t dma_copy;
};

static int pcie_edu_get_id(const struct device *dev, uint32_t *id)
{
	struct pcie_edu_data *ctx = (struct pcie_edu_data *)dev->data;

	*id = sys_read32((mem_addr_t)ctx->mem_addr);

	return 0;
}

static int pcie_edu_check_liveness(const struct device *dev)
{
	struct pcie_edu_data *ctx = (struct pcie_edu_data *)dev->data;
	uint32_t val = 0xaa55aa55;

	sys_write32(val, (mem_addr_t)((uint8_t *)ctx->mem_addr + 0x04));
	if (sys_read32((mem_addr_t)((uint8_t *)ctx->mem_addr + 0x04)) != ~val)
		return -EIO;

	return 0;
}

#ifdef CONFIG_PCIE_MSI
static volatile uint32_t irq_status;

static void pcie_edu_irq(const void *parameter)
{
	const struct device *dev = parameter;
	struct pcie_edu_data *ctx = (struct pcie_edu_data *)dev->data;

	irq_status = sys_read32((mem_addr_t)((uint8_t *)ctx->mem_addr + 0x24));

	sys_write32(irq_status, (mem_addr_t)((uint8_t *)ctx->mem_addr + 0x64));
}

static int pcie_edu_test_msi(const struct device *dev)
{
	struct pcie_edu_data *ctx = (struct pcie_edu_data *)dev->data;

	irq_status = 0;

	sys_write32(0x10, (mem_addr_t)((uint8_t *)ctx->mem_addr + 0x60));

	k_usleep(1);

	if (irq_status != 0x10)
		return -EIO;

	return 0;
}

static uint32_t pcie_edu_calc_fact(const struct device *dev, uint32_t fact)
{
	struct pcie_edu_data *ctx = (struct pcie_edu_data *)dev->data;

	irq_status = 0;

	/* Enable IRQ for Factorial calculation */
	sys_write32(0x80, (mem_addr_t)((uint8_t *)ctx->mem_addr + 0x20));
	sys_write32(fact, (mem_addr_t)((uint8_t *)ctx->mem_addr + 0x08));

	while (!irq_status) {
		k_usleep(1);
	}

	return sys_read32((mem_addr_t)((uint8_t *)ctx->mem_addr + 0x08));
}
#endif

static void pcie_edu_dma_copy(const struct device *dev, void *buffer, size_t size, bool write)
{
	struct pcie_edu_data *ctx = (struct pcie_edu_data *)dev->data;
	size_t sz = MIN(4096, size);

#ifdef CONFIG_PCIE_MSI
	irq_status = 0;
#endif

	if (write) {
		sys_write64((uint64_t)buffer, (mem_addr_t)((uint8_t *)ctx->mem_addr + 0x80));
		sys_write64(0x40000, (mem_addr_t)((uint8_t *)ctx->mem_addr + 0x88));
		sys_write64(sz, (mem_addr_t)((uint8_t *)ctx->mem_addr + 0x90));
#ifdef CONFIG_PCIE_MSI
		sys_write32(5, (mem_addr_t)((uint8_t *)ctx->mem_addr + 0x98));
#else
		sys_write32(1, (mem_addr_t)((uint8_t *)ctx->mem_addr + 0x98));
#endif
	} else {
		sys_write64(0x40000, (mem_addr_t)((uint8_t *)ctx->mem_addr + 0x80));
		sys_write64((uint64_t)buffer, (mem_addr_t)((uint8_t *)ctx->mem_addr + 0x88));
		sys_write64(sz, (mem_addr_t)((uint8_t *)ctx->mem_addr + 0x90));
#ifdef CONFIG_PCIE_MSI
		sys_write32(7, (mem_addr_t)((uint8_t *)ctx->mem_addr + 0x98));
#else
		sys_write32(3, (mem_addr_t)((uint8_t *)ctx->mem_addr + 0x98));
#endif
	}

#ifdef CONFIG_PCIE_MSI
	while (!irq_status) {
		k_usleep(1);
	}
#else
	while (sys_read32((mem_addr_t)((uint8_t *)ctx->mem_addr + 0x98)) & 1 == 1) {
		k_usleep(1);
	}
#endif
}

static int pcie_edu_init(const struct device *dev)
{
	const struct pcie_edu_config *cfg = (const struct pcie_edu_config *)dev->config;
	struct pcie_edu_data *ctx = (struct pcie_edu_data *)dev->data;
#ifdef CONFIG_PCIE_MSI
	int ret;
#endif

	if (!pcie_probe(cfg->pcie_bdf, cfg->pcie_id)) {
		LOG_INF("edu probe [%02x:%02x.%x] fail", PCIE_BDF_TO_BUS(cfg->pcie_bdf),
		PCIE_BDF_TO_DEV(cfg->pcie_bdf), PCIE_BDF_TO_FUNC(cfg->pcie_bdf));
		return -EINVAL;
	}

	LOG_INF("edu probe [%02x:%02x.%x]", PCIE_BDF_TO_BUS(cfg->pcie_bdf),
		PCIE_BDF_TO_DEV(cfg->pcie_bdf), PCIE_BDF_TO_FUNC(cfg->pcie_bdf));

	pcie_get_mbar(cfg->pcie_bdf, 0, &ctx->mem_bar);

	pcie_set_cmd(cfg->pcie_bdf, PCIE_CONF_CMDSTAT_MEM, true);

#ifdef CONFIG_PCIE_MSI
	ret = pcie_msi_vectors_allocate(cfg->pcie_bdf, 0, &ctx->msi, 1);
	if (ret != 1) {
		return -EIO;
	}

	ret = pcie_msi_vector_connect(cfg->pcie_bdf, &ctx->msi, pcie_edu_irq, dev, 0);
	if (!ret) {
		return -EIO;
	}

	ret = pcie_msi_enable(cfg->pcie_bdf, &ctx->msi, 1, 0);
	if (!ret) {
		return -EIO;
	}
#endif

	device_map(&ctx->mem_addr, ctx->mem_bar.phys_addr, ctx->mem_bar.size, K_MEM_CACHE_NONE);

	LOG_INF("MEM %lx -> %p", ctx->mem_bar.phys_addr, (void *)ctx->mem_addr);

	return 0;
}

static struct pcie_edu_driver_api pcie_edu_api = {
	.get_id = pcie_edu_get_id,
	.check_liveness = pcie_edu_check_liveness,
#ifdef CONFIG_PCIE_MSI
	.test_msi = pcie_edu_test_msi,
	.calc_fact = pcie_edu_calc_fact,
#endif
	.dma_copy = pcie_edu_dma_copy,
};

#define PCIE_TESTED_DEVICE_INIT(n)							\
	static struct pcie_edu_data edu_dev_data_##n;					\
	static const struct pcie_edu_config edu_dev_cfg_##n = {				\
		.pcie_bdf = DT_INST_REG_ADDR(n),					\
		.pcie_id = DT_INST_REG_SIZE(n),						\
	};										\
	DEVICE_DT_INST_DEFINE(n, &pcie_edu_init, NULL,					\
			      &edu_dev_data_##n, &edu_dev_cfg_##n,			\
			      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,		\
			      &pcie_edu_api);

DT_INST_FOREACH_STATUS_OKAY(PCIE_TESTED_DEVICE_INIT)

#define PCIE_EDU_DEV(n) DEVICE_DT_GET(DT_DRV_INST(n)),

static const struct device *dev[] = {
	DT_INST_FOREACH_STATUS_OKAY(PCIE_EDU_DEV)
	NULL
};

static void test_dev_probe(void)
{
	struct pcie_edu_driver_api *api;
	const struct pcie_edu_config *cfg;
	int i;

	for (i = 0 ; dev[i] ; ++i) {
		LOG_INF("edu device %d", i);

		zassert_not_null(dev[i], "");

		cfg = (const struct pcie_edu_config *)dev[i]->config;
		LOG_INF("edu device %d bdf [%02x:%02x.%x]", i,
			PCIE_BDF_TO_BUS(cfg->pcie_bdf), PCIE_BDF_TO_DEV(cfg->pcie_bdf),
			PCIE_BDF_TO_FUNC(cfg->pcie_bdf));

		zassert_equal(dev[i]->state->initialized, true, "");
		zassert_equal(dev[i]->state->init_res, 0, "");

		api = (struct pcie_edu_driver_api *)dev[i]->api;
		zassert_not_null(api, "");
	}
}

static void test_run_get_id(void)
{
	struct pcie_edu_driver_api *api;
	int ret;
	uint32_t id;
	int i;

	for (i = 0 ; dev[i] ; ++i) {
		LOG_INF("edu device %d", i);

		api = (struct pcie_edu_driver_api *)dev[i]->api;

		ret = api->get_id(dev[i], &id);
		zassert_equal(ret, 0, "");

		LOG_INF("id %08x", id);
	}
}

static void test_run_check_liveness(void)
{
	struct pcie_edu_driver_api *api;
	int ret;
	int i;

	for (i = 0 ; dev[i] ; ++i) {
		LOG_INF("edu device %d", i);

		api = (struct pcie_edu_driver_api *)dev[i]->api;

		ret = api->check_liveness(dev[i]);
		zassert_equal(ret, 0, "");
	}
}

#ifdef CONFIG_PCIE_MSI
static void test_run_test_msi(void)
{
	struct pcie_edu_driver_api *api;
	int ret;
	int i;

	for (i = 0 ; dev[i] ; ++i) {
		LOG_INF("edu device %d", i);

		api = (struct pcie_edu_driver_api *)dev[i]->api;

		ret = api->test_msi(dev[i]);
		zassert_equal(ret, 0, "");
	}
}

static void test_run_calc_fact(void)
{
	struct pcie_edu_driver_api *api;
	uint32_t ret;
	int i;

	for (i = 0 ; dev[i] ; ++i) {
		LOG_INF("edu device %d", i);

		api = (struct pcie_edu_driver_api *)dev[i]->api;

		ret = api->calc_fact(dev[i], 0);
		zassert_equal(ret, 1, "");
		LOG_INF("fact(%d)=%d", 1, ret);

		ret = api->calc_fact(dev[i], 5);
		zassert_equal(ret, 120, "");
		LOG_INF("fact(%d)=%d", 5, ret);

		ret = api->calc_fact(dev[i], 10);
		zassert_equal(ret, 3628800, "");
		LOG_INF("fact(%d)=%d", 10, ret);
	}
}
#else
static void test_run_test_msi(void)
{
	LOG_INF("Skipping, MSI disabled");
}

static void test_run_calc_fact(void)
{
	LOG_INF("Skipping, MSI disabled");
}
#endif

static void test_run_dma_copy(void)
{
	const struct pcie_edu_config *cfg;
	struct pcie_edu_driver_api *api;
	int i;

	/* First copy the device BDF in the device memory */

	for (i = 0 ; dev[i] ; ++i) {
		pcie_bdf_t pcie_bdf;

		api = (struct pcie_edu_driver_api *)dev[i]->api;
		cfg = (const struct pcie_edu_config *)dev[i]->config;

		LOG_INF("edu device %d write %x", i, cfg->pcie_bdf);

		pcie_bdf = cfg->pcie_bdf;

		api->dma_copy(dev[i], &pcie_bdf, sizeof(pcie_bdf), true);
	}

	/* Then read back and compare with BFD */

	for (i = 0 ; dev[i] ; ++i) {
		pcie_bdf_t pcie_bdf = PCIE_BDF_NONE;

		api = (struct pcie_edu_driver_api *)dev[i]->api;
		cfg = (const struct pcie_edu_config *)dev[i]->config;

		api->dma_copy(dev[i], &pcie_bdf, sizeof(pcie_bdf), false);

		LOG_INF("edu device %d read %x", i, pcie_bdf);

		zassert_equal(cfg->pcie_bdf, pcie_bdf, "");
	}
}

void test_main(void)
{
	ztest_test_suite(pci_edu_test,
			 ztest_unit_test(test_dev_probe),
			 ztest_unit_test(test_run_get_id),
			 ztest_unit_test(test_run_check_liveness),
			 ztest_unit_test(test_run_test_msi),
			 ztest_unit_test(test_run_calc_fact),
			 ztest_unit_test(test_run_dma_copy)
			 );

	ztest_run_test_suite(pci_edu_test);
}
