/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/tc_util.h>

#include <zephyr/drivers/pcie/pcie.h>
#include <ibecc.h>

#define CONFIG_EDAC_LOG_LEVEL LOG_LEVEL_DBG

static uint8_t mock_sys_in8(io_port_t port)
{
	/* Needed for NMI handling simulation */
	if (port == NMI_STS_CNT_REG) {
		TC_PRINT("Simulate sys_in8(NMI_STS_CNT_REG)=>SERR\n");
		return NMI_STS_SRC_SERR;
	}

	TC_PRINT("Simulate sys_in8(0x%x)=>0\n", port);

	return 0;
}

static void mock_sys_out8(uint8_t data, io_port_t port)
{
	TC_PRINT("Simulate sys_out8() NOP\n");
}

static uint64_t mock_sys_read64(uint64_t addr)
{
#if defined(IBECC_ENABLED)
	if (addr == IBECC_ECC_ERROR_LOG) {
		TC_PRINT("Simulate sys_read64(IBECC_ECC_ERROR_LOG)=>CERRSTS\n");
		return ECC_ERROR_CERRSTS;
	}

	if (addr == IBECC_PARITY_ERROR_LOG) {
		TC_PRINT("Simulate sys_read64(IBECC_PARITY_ERROR_LOG)=>1\n");
		return 1;
	}
#endif /* IBECC_ENABLED */

	TC_PRINT("Simulate sys_read64(0x%llx)=>0\n", addr);

	return 0;
}

static void mock_sys_write64(uint64_t data, uint64_t reg)
{
	TC_PRINT("Simulate sys_write64() NOP\n");
}

static void mock_conf_write(pcie_bdf_t bdf, unsigned int reg, uint32_t data)
{
	TC_PRINT("Simulate pcie_conf_write() NOP\n");
}

static uint32_t mock_conf_read(pcie_bdf_t bdf, unsigned int reg)
{
#if defined(EMULATE_SKU)
	if (bdf == PCI_HOST_BRIDGE && reg == PCIE_CONF_ID) {
		TC_PRINT("Simulate PCI device, SKU 0x%x\n", EMULATE_SKU);

		return EMULATE_SKU;
	}
#endif /* EMULATE_SKU */

#if defined(IBECC_ENABLED)
	if (bdf == PCI_HOST_BRIDGE && reg == CAPID0_C_REG) {
		TC_PRINT("Simulate IBECC enabled\n");

		return CAPID0_C_IBECC_ENABLED;
	}
#endif /* IBECC_ENABLED */

	TC_PRINT("Simulate pcie_conf_read()=>0\n");

	return 0;
}

/* Redefine PCIE access */
#define pcie_conf_read(bdf, reg)	mock_conf_read(bdf, reg)
#define pcie_conf_write(bdf, reg, val)	mock_conf_write(bdf, reg, val)

/* Redefine sys_in function */
#define sys_in8(port)			mock_sys_in8(port)
#define sys_out8(data, port)		mock_sys_out8(data, port)
#define sys_read64(addr)		mock_sys_read64(addr)
#define sys_write64(data, addr)		mock_sys_write64(data, addr)

/* Include source code to test some static functions */
#include "edac_ibecc.c"

ZTEST(ibecc_cov, test_static_functions)
{
	const struct device *const dev = DEVICE_DT_GET(DEVICE_NODE);
	struct ibecc_error error_data;
	uint64_t log_data;
	int ret;

	TC_PRINT("Start testing static functions\n");

	zassert_not_null(dev, "Device not found");

	/* Catch failed PCIE probe case */
	ret = edac_ibecc_init(dev);
	zassert_equal(ret, -ENODEV, "");

	ret = edac_ecc_error_log_get(dev, &log_data);
	if (IS_ENABLED(IBECC_ENABLED)) {
		zassert_equal(ret, 0, "");
	} else {
		zassert_equal(ret, -ENODATA, "");
	}

	ret = edac_parity_error_log_get(dev, &log_data);
	if (IS_ENABLED(IBECC_ENABLED)) {
		zassert_equal(ret, 0, "");
	} else {
		zassert_equal(ret, -ENODATA, "");
	}

	/* Catch passing zero errlog case */
	parse_ecclog(dev, 0, &error_data);

	/* Test errsts clear not set case */
	ibecc_errsts_clear(PCI_HOST_BRIDGE);

	/* Test errcmd clear case */
	ibecc_errcmd_setup(PCI_HOST_BRIDGE, false);
}

ZTEST(ibecc_cov, test_trigger_nmi_handler)
{
	bool ret;

	ret = z_x86_do_kernel_nmi(NULL);
	zassert_true(ret, "Test NMI handling");
}

ZTEST_SUITE(ibecc_cov, NULL, NULL, NULL, NULL, NULL);
