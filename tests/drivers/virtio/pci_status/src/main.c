/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>

#include <zephyr/ztest.h>

#include "../../../../../drivers/virtio/virtio_pci.c"

static struct virtio_pci_common_cfg common_cfg;
static struct virtio_pci_data pci_data;
static struct device test_device;

uint32_t pcie_conf_read(pcie_bdf_t bdf, unsigned int reg)
{
	ARG_UNUSED(bdf);
	ARG_UNUSED(reg);

	return 0U;
}

bool pcie_get_mbar(pcie_bdf_t bdf, unsigned int bar_index, struct pcie_bar *mbar)
{
	ARG_UNUSED(bdf);
	ARG_UNUSED(bar_index);
	ARG_UNUSED(mbar);

	return false;
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	memset(&common_cfg, 0, sizeof(common_cfg));
	memset(&pci_data, 0, sizeof(pci_data));
	memset(&test_device, 0, sizeof(test_device));

	pci_data.common_cfg = &common_cfg;
	test_device.data = &pci_data;
}

ZTEST(virtio_pci_status, test_write_sets_status_bit)
{
	virtio_pci_write_status_bit(&test_device, DEVICE_STATUS_DRIVER_OK);

	zassert_equal(common_cfg.device_status, BIT(DEVICE_STATUS_DRIVER_OK));
}

ZTEST(virtio_pci_status, test_read_detects_status_bit)
{
	common_cfg.device_status = BIT(DEVICE_STATUS_FEATURES_OK);

	zassert_true(virtio_pci_read_status_bit(&test_device, DEVICE_STATUS_FEATURES_OK));
	zassert_false(virtio_pci_read_status_bit(&test_device, DEVICE_STATUS_DRIVER_OK));
}

ZTEST(virtio_pci_status, test_write_preserves_existing_bits)
{
	common_cfg.device_status = BIT(DEVICE_STATUS_ACKNOWLEDGE);

	virtio_pci_write_status_bit(&test_device, DEVICE_STATUS_DRIVER);

	zassert_equal(common_cfg.device_status,
		      BIT(DEVICE_STATUS_ACKNOWLEDGE) | BIT(DEVICE_STATUS_DRIVER));
}

ZTEST(virtio_pci_status, test_write_sets_high_status_bit)
{
	virtio_pci_write_status_bit(&test_device, DEVICE_STATUS_FAILED);

	zassert_equal(common_cfg.device_status, BIT(DEVICE_STATUS_FAILED));
}

ZTEST_SUITE(virtio_pci_status, NULL, NULL, before, NULL, NULL);
