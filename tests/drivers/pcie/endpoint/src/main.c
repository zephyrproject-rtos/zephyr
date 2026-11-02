/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>
#include <zephyr/drivers/pcie/controller.h>
#include <zephyr/drivers/pcie/cap.h>
#include <zephyr/ztest.h>

#define TEST_BDF PCIE_BDF(1, 0, 0)
#define TEST_ID PCIE_ID(0x1234, 0x5678)
#define TEST_CLASS 0x01080200U
#define TEST_CAP 20U

struct fake_ctrl {
	uint32_t config[1024];
	uint32_t table[1024];
	uint32_t bus_address;
	uint32_t reads;
	uint32_t status_reads;
	uint32_t writes;
	uint32_t write_attempts;
	uint32_t fail_write_at;
	uint32_t fail_read_at;
	int read_error;
	int write_error;
	bool translate;
};

static struct fake_ctrl ctrl_data_a, ctrl_data_b;

static void reset_ctrl(struct fake_ctrl *data, bool msix)
{
	memset(data, 0, sizeof(*data));
	data->config[PCIE_CONF_ID] = TEST_ID;
	data->config[PCIE_CONF_CLASSREV] = TEST_CLASS;
	data->config[PCIE_CONF_CMDSTAT] = PCIE_CONF_CMDSTAT_CAPS | PCIE_CONF_CMDSTAT_MEM;
	data->config[PCIE_CONF_CAPPTR] = msix ? TEST_CAP * 4U : 16U * 4U;
	data->config[16] = PCI_CAP_ID_VNDR;
	data->config[TEST_CAP] = PCI_CAP_ID_MSIX | (3U << 16U);
	data->config[TEST_CAP + 1U] = 0U;
	data->config[PCIE_CONF_BAR0] = msix ? 0x20000000U : 0x10000000U;
	data->bus_address = data->config[PCIE_CONF_BAR0];
	data->config[PCIE_CONF_EXT_CAPPTR] = 0x00010001U | (0x110U << 20);
	data->config[68] = 0x00010002U;
	data->config[32] = msix ? 0xbbbbU : 0xaaaaU;
	data->config[33] = UINT32_MAX;
	data->translate = true;
}

static uint32_t fake_read(const struct device *dev, pcie_bdf_t bdf, unsigned int reg)
{
	struct fake_ctrl *data = dev->data;

	data->reads++;
	if (bdf == PCIE_BDF(0, 0, 0) || bdf == PCIE_BDF(1, 1, 0)) {
		switch (reg) {
		case PCIE_CONF_ID:
			return PCIE_ID(0xabcd, 0x1234);
		case PCIE_CONF_TYPE:
			return PCIE_CONF_TYPE_PCI_BRIDGE << 16;
		case PCIE_BUS_NUMBER:
			return bdf == PCIE_BDF(0, 0, 0) ? (1U << 8) : (2U << 8);
		default:
			return 0U;
		}
	}
	if (bdf != TEST_BDF && bdf != PCIE_BDF(2, 0, 0)) {
		return UINT32_MAX;
	}
	if (reg >= PCIE_CONF_BAR0 && reg <= PCIE_CONF_BAR5 &&
	    data->config[reg] == UINT32_MAX) {
		return 0xfffff000U;
	}
	return data->config[reg];
}

static void fake_write(const struct device *dev, pcie_bdf_t bdf, unsigned int reg, uint32_t value)
{
	struct fake_ctrl *data = dev->data;

	zassert_true(bdf == TEST_BDF || bdf == PCIE_BDF(2, 0, 0));
	data->writes++;
	if (reg == PCIE_CONF_CMDSTAT) {
		data->config[reg] = (data->config[reg] & 0xffff0000U & ~value) | (value & 0xffffU);
	} else {
		data->config[reg] = value;
	}
}

static int fake_read_status(const struct device *dev, pcie_bdf_t bdf, unsigned int reg,
			    uint32_t *value)
{
	struct fake_ctrl *data = dev->data;

	data->status_reads++;
	if (data->status_reads == data->fail_read_at) {
		return -EIO;
	}
	if (data->read_error != 0) {
		return data->read_error;
	}
	*value = fake_read(dev, bdf, reg);
	return 0;
}

static int fake_write_status(const struct device *dev, pcie_bdf_t bdf, unsigned int reg,
			     uint32_t value)
{
	struct fake_ctrl *data = dev->data;

	data->write_attempts++;
	if (data->write_attempts == data->fail_write_at) {
		return -EIO;
	}
	if (data->write_error != 0) {
		return data->write_error;
	}
	fake_write(dev, bdf, reg, value);
	return 0;
}

static bool fake_translate(const struct device *dev, pcie_bdf_t bdf, bool mem, bool mem64,
			   uintptr_t bus_addr, uintptr_t *addr)
{
	struct fake_ctrl *data = dev->data;

	ARG_UNUSED(bdf);
	ARG_UNUSED(mem64);
	if (mem == PCIE_CONF_BAR_IO(data->config[PCIE_CONF_BAR0]) ||
	    !data->translate || bus_addr != data->bus_address) {
		return false;
	}
	*addr = (uintptr_t)data->table;
	return true;
}

static DEVICE_API(pcie_ctrl, fake_api_a) = {
	.conf_read = fake_read,
	.conf_write = fake_write,
	.region_translate = fake_translate,
};

static DEVICE_API(pcie_ctrl, fake_api_b) = {
	.conf_read = fake_read,
	.conf_write = fake_write,
	.region_translate = fake_translate,
	.conf_read_status = fake_read_status,
	.conf_write_status = fake_write_status,
};

static DEVICE_API(pcie_ctrl, old_api) = {
	.conf_read = fake_read,
	.conf_write = fake_write,
	.region_translate = fake_translate,
};

static DEVICE_API(pcie_ctrl, empty_api) = {
	.conf_read = NULL,
};

static int fake_init(const struct device *dev)
{
	struct fake_ctrl *data = dev->data;

	reset_ctrl(data, dev == DEVICE_DT_GET(DT_NODELABEL(ctrl_b)));
	return 0;
}

DEVICE_DT_DEFINE(DT_NODELABEL(ctrl_a), fake_init, NULL, &ctrl_data_a, NULL,
		 PRE_KERNEL_1, 40, &fake_api_a);
DEVICE_DT_DEFINE(DT_NODELABEL(ctrl_b), fake_init, NULL, &ctrl_data_b, NULL,
		 PRE_KERNEL_1, 40, &fake_api_b);
DEVICE_DEFINE(old_ctrl, "old_ctrl", NULL, NULL, &ctrl_data_b, NULL,
	      PRE_KERNEL_1, 40, &old_api);
DEVICE_DEFINE(empty_ctrl, "empty_ctrl", NULL, NULL, NULL, NULL,
	      PRE_KERNEL_1, 40, &empty_api);

static const struct pcie_endpoint ep_a = {
	.ctrl = DEVICE_DT_GET(DT_NODELABEL(ctrl_a)), .bdf = TEST_BDF,
};
static const struct pcie_endpoint ep_b = {
	.ctrl = DEVICE_DT_GET(DT_NODELABEL(ctrl_b)), .bdf = TEST_BDF,
};
static const struct pcie_endpoint ep_empty = {
	.ctrl = DEVICE_GET(empty_ctrl), .bdf = TEST_BDF,
};

static void before(void *fixture)
{
	ARG_UNUSED(fixture);
	reset_ctrl(&ctrl_data_a, false);
	reset_ctrl(&ctrl_data_b, true);
}

ZTEST(pcie_endpoint, test_config_owner_and_legacy_chosen)
{
	uint32_t value;

	zassert_ok(pcie_endpoint_conf_read(&ep_b, 32U, &value));
	zassert_equal(value, 0xbbbbU);
	zassert_equal(pcie_conf_read(TEST_BDF, 32U), 0xaaaaU);
	zassert_ok(pcie_endpoint_conf_write(&ep_b, 32U, 0xccccU));
	zassert_equal(ctrl_data_b.config[32], 0xccccU);
	zassert_equal(ctrl_data_a.config[32], 0xaaaaU);
	zassert_true(ctrl_data_b.status_reads > 0U);
	zassert_equal(ctrl_data_a.status_reads, 0U);
	zassert_ok(pcie_endpoint_conf_read(&ep_a, 33U, &value));
	zassert_equal(value, UINT32_MAX);
}

ZTEST(pcie_endpoint, test_validation_precedes_backend)
{
	const struct pcie_endpoint unset = {.bdf = TEST_BDF};
	struct pcie_bar bar;
	uint32_t value;

	zassert_equal(pcie_endpoint_conf_read(NULL, 0U, &value), -EINVAL);
	zassert_equal(pcie_endpoint_conf_read(&unset, 0U, &value), -EINVAL);
	zassert_equal(pcie_endpoint_conf_read(&ep_empty, 0U, NULL), -EINVAL);
	zassert_equal(pcie_endpoint_conf_read(&ep_empty, 1024U, &value), -EINVAL);
	zassert_equal(pcie_endpoint_conf_read(&ep_empty, 0U, &value), -ENOTSUP);
	zassert_equal(pcie_endpoint_conf_write(&unset, 0U, 0U), -EINVAL);
	zassert_equal(pcie_endpoint_conf_write(&ep_empty, 0U, 0U), -ENOTSUP);
	zassert_equal(pcie_endpoint_get_cap(&ep_empty, 1U, NULL), -EINVAL);
	zassert_equal(pcie_endpoint_get_ext_cap(&unset, 1U, &value), -EINVAL);
	zassert_equal(pcie_endpoint_get_mbar(&ep_empty, 6U, &bar), -EINVAL);
	zassert_equal(pcie_endpoint_get_mbar(&ep_empty, 0U, &bar), -ENOTSUP);
}

ZTEST(pcie_endpoint, test_status_errors_and_all_ones_data)
{
	uint32_t value;
	struct pcie_bar bar;

	ctrl_data_b.read_error = -EIO;
	zassert_equal(pcie_endpoint_conf_read(&ep_b, 0U, &value), -EIO);
	zassert_equal(pcie_endpoint_get_cap(&ep_b, PCI_CAP_ID_MSIX, &value), -EIO);
	zassert_equal(pcie_endpoint_get_mbar(&ep_b, 0U, &bar), -EIO);
	ctrl_data_b.read_error = 0;
	ctrl_data_b.write_error = -EIO;
	zassert_equal(pcie_endpoint_conf_write(&ep_b, 32U, 0U), -EIO);
	zassert_ok(pcie_endpoint_conf_read(&ep_b, 33U, &value));
	zassert_equal(value, UINT32_MAX);
}

ZTEST(pcie_endpoint, test_capabilities_use_owner)
{
	uint32_t cap;

	zassert_ok(pcie_endpoint_get_cap(&ep_b, PCI_CAP_ID_MSIX, &cap));
	zassert_equal(cap, TEST_CAP);
	zassert_equal(pcie_get_cap(TEST_BDF, PCI_CAP_ID_MSIX), 0U);
	zassert_equal(pcie_ctrl_get_cap(ep_b.ctrl, TEST_BDF, PCI_CAP_ID_MSIX), TEST_CAP);
	zassert_ok(pcie_endpoint_get_cap(&ep_b, PCI_CAP_ID_PM, &cap));
	zassert_equal(cap, 0U);
	zassert_ok(pcie_endpoint_get_ext_cap(&ep_b, 2U, &cap));
	zassert_equal(cap, 68U);
	ctrl_data_b.config[68] = 0U;
	zassert_ok(pcie_endpoint_get_ext_cap(&ep_b, 2U, &cap));
	zassert_equal(cap, 0U);
	zassert_equal(pcie_get_ext_cap(TEST_BDF, 2U), 68U);
}

ZTEST(pcie_endpoint, test_malformed_capability_lists_terminate)
{
	uint32_t cap;

	ctrl_data_b.config[TEST_CAP] |= (TEST_CAP * 4U) << 8;
	zassert_equal(pcie_endpoint_get_cap(&ep_b, PCI_CAP_ID_PM, &cap), -EINVAL);
	ctrl_data_b.config[PCIE_CONF_EXT_CAPPTR] = 1U | (0x100U << 20);
	zassert_equal(pcie_endpoint_get_ext_cap(&ep_b, 2U, &cap), -EINVAL);
}

ZTEST(pcie_endpoint, test_bar_access_and_translation_use_owner)
{
	struct pcie_bar bar;
	uint32_t command = ctrl_data_b.config[PCIE_CONF_CMDSTAT];

	zassert_ok(pcie_endpoint_get_mbar(&ep_b, 0U, &bar));
	zassert_equal(bar.phys_addr, (uintptr_t)ctrl_data_b.table);
	zassert_equal(bar.size, sizeof(ctrl_data_b.table));
	zassert_equal(ctrl_data_a.writes, 0U);
	zassert_equal(ctrl_data_b.config[PCIE_CONF_BAR0], ctrl_data_b.bus_address);
	zassert_equal(ctrl_data_b.config[PCIE_CONF_CMDSTAT], command);
	zassert_true(pcie_get_mbar(TEST_BDF, 0U, &bar));
	zassert_equal(bar.phys_addr, (uintptr_t)ctrl_data_a.table);
	ctrl_data_b.translate = false;
	zassert_equal(pcie_endpoint_get_mbar(&ep_b, 0U, &bar), -ENOTSUP);
	zassert_equal(ctrl_data_b.config[PCIE_CONF_CMDSTAT], command);
	zassert_equal(pcie_endpoint_get_mbar(&ep_b, 1U, &bar), -ENOENT);
}


ZTEST(pcie_endpoint, test_bar_restore_failure_keeps_decode_disabled)
{
	struct pcie_bar bar;

	for (uint32_t high = 0U; high < 2U; high++) {
		reset_ctrl(&ctrl_data_b, true);
		if (high != 0U) {
			ctrl_data_b.config[PCIE_CONF_BAR0] |= 4U;
		}
		ctrl_data_b.fail_write_at = high != 0U ? 5U : 3U;
		zassert_equal(pcie_endpoint_get_mbar(&ep_b, 0U, &bar), -EIO);
		zassert_equal(ctrl_data_b.config[PCIE_CONF_CMDSTAT] &
			      (PCIE_CONF_CMDSTAT_MEM | PCIE_CONF_CMDSTAT_IO), 0U);
	}
}


ZTEST(pcie_endpoint, test_bar_probe_error_restores_decode_after_bar_recovery)
{
	struct pcie_bar bar;
	uint32_t command = ctrl_data_b.config[PCIE_CONF_CMDSTAT];

	ctrl_data_b.fail_read_at = 4U;
	zassert_equal(pcie_endpoint_get_mbar(&ep_b, 0U, &bar), -EIO);
	zassert_equal(ctrl_data_b.config[PCIE_CONF_BAR0], ctrl_data_b.bus_address);
	zassert_equal(ctrl_data_b.config[PCIE_CONF_CMDSTAT], command);
}


ZTEST(pcie_endpoint, test_legacy_bar_probe_preserves_status)
{
	struct pcie_bar bar;
	uint32_t command = ctrl_data_a.config[PCIE_CONF_CMDSTAT];

	zassert_true(pcie_probe_mbar(TEST_BDF, 0U, &bar));
	zassert_equal(bar.phys_addr, (uintptr_t)ctrl_data_a.table);
	zassert_equal(ctrl_data_a.config[PCIE_CONF_CMDSTAT], command);
	zassert_false(pcie_get_iobar(TEST_BDF, 0U, &bar));
	ctrl_data_a.config[PCIE_CONF_BAR0] = 0x1001U;
	ctrl_data_a.bus_address = 0x1000U;
	zassert_true(pcie_get_iobar(TEST_BDF, 0U, &bar));
	zassert_equal(bar.phys_addr, (uintptr_t)ctrl_data_a.table);
	zassert_equal(bar.size, sizeof(ctrl_data_a.table));
	zassert_false(pcie_get_mbar(TEST_BDF, 0U, &bar));
}

ZTEST_SUITE(pcie_endpoint, NULL, NULL, before, NULL, NULL);

ZTEST(pcie_endpoint, test_invalid_bdf_reads_are_rejected_before_access)
{
	const pcie_bdf_t invalid[] = {PCIE_BDF_NONE, TEST_BDF | BIT(24), TEST_BDF | BIT(0)};
	const struct pcie_endpoint *endpoints[] = {&ep_a, &ep_b, &ep_empty};
	uint32_t value;

	for (size_t i = 0U; i < ARRAY_SIZE(endpoints); i++) {
		struct pcie_endpoint ep = *endpoints[i];

		for (size_t j = 0U; j < ARRAY_SIZE(invalid); j++) {
			ep.bdf = invalid[j];
			zassert_equal(pcie_endpoint_conf_read(&ep, 0U, &value), -EINVAL);
		}
	}
	zassert_equal(ctrl_data_a.reads, 0U);
	zassert_equal(ctrl_data_b.reads, 0U);
	zassert_equal(ctrl_data_b.status_reads, 0U);
}

ZTEST(pcie_endpoint, test_invalid_bdf_writes_are_rejected_before_access)
{
	const pcie_bdf_t invalid[] = {PCIE_BDF_NONE, TEST_BDF | BIT(24), TEST_BDF | BIT(0)};
	const struct pcie_endpoint *endpoints[] = {&ep_a, &ep_b, &ep_empty};

	ctrl_data_b.write_error = -EIO;
	for (size_t i = 0U; i < ARRAY_SIZE(endpoints); i++) {
		struct pcie_endpoint ep = *endpoints[i];

		for (size_t j = 0U; j < ARRAY_SIZE(invalid); j++) {
			ep.bdf = invalid[j];
			zassert_equal(pcie_endpoint_conf_write(&ep, 0U, 0U), -EINVAL);
		}
	}
	zassert_equal(ctrl_data_a.writes, 0U);
	zassert_equal(ctrl_data_b.writes, 0U);
	zassert_equal(ctrl_data_b.write_attempts, 0U);
}

ZTEST(pcie_endpoint, test_bridge_non_bar_registers_are_not_probed)
{
	struct pcie_bar bar;

	ctrl_data_b.config[PCIE_CONF_TYPE] = PCIE_CONF_TYPE_PCI_BRIDGE << 16;
	for (uint32_t index = 2U; index < 6U; index++) {
		ctrl_data_b.config[PCIE_CONF_BAR0 + index] = ctrl_data_b.bus_address;
		zassert_equal(pcie_endpoint_get_mbar(&ep_b, index, &bar), -ENOENT);
	}
	zassert_equal(ctrl_data_b.writes, 0U);
}

ZTEST(pcie_endpoint, test_last_bar_cannot_cross_header_boundary)
{
	const uint32_t types[] = {PCIE_CONF_TYPE_STANDARD, PCIE_CONF_TYPE_PCI_BRIDGE};
	const uint32_t indices[] = {5U, 1U};
	struct pcie_bar bar;

	for (size_t i = 0U; i < ARRAY_SIZE(types); i++) {
		reset_ctrl(&ctrl_data_b, true);
		ctrl_data_b.config[PCIE_CONF_TYPE] = types[i] << 16;
		ctrl_data_b.config[PCIE_CONF_BAR0 + indices[i]] = ctrl_data_b.bus_address | 4U;
		zassert_equal(pcie_endpoint_get_mbar(&ep_b, indices[i], &bar), -ENOENT);
		zassert_equal(ctrl_data_b.writes, 0U);
	}
}

ZTEST(pcie_endpoint, test_legal_bar_positions_are_probed)
{
	const uint32_t types[] = {PCIE_CONF_TYPE_STANDARD, PCIE_CONF_TYPE_PCI_BRIDGE,
				  PCIE_CONF_TYPE_PCI_BRIDGE, PCIE_CONF_TYPE_PCI_BRIDGE};
	const uint32_t indices[] = {5U, 0U, 1U, 0U};
	struct pcie_bar bar;

	for (size_t i = 0U; i < ARRAY_SIZE(types); i++) {
		reset_ctrl(&ctrl_data_b, true);
		ctrl_data_b.config[PCIE_CONF_TYPE] = types[i] << 16;
		ctrl_data_b.config[PCIE_CONF_BAR0 + indices[i]] =
			ctrl_data_b.bus_address | (i == 3U ? 4U : 0U);
		zassert_ok(pcie_endpoint_get_mbar(&ep_b, indices[i], &bar));
		zassert_equal(bar.phys_addr, (uintptr_t)ctrl_data_b.table);
		zassert_equal(bar.size, sizeof(ctrl_data_b.table));
	}
}

ZTEST(pcie_endpoint, test_unsupported_header_is_not_probed)
{
	struct pcie_bar bar;

	ctrl_data_b.config[PCIE_CONF_TYPE] = PCIE_CONF_TYPE_CARDBUS_BRIDGE << 16;
	zassert_equal(pcie_endpoint_get_mbar(&ep_b, 0U, &bar), -ENOTSUP);
	ctrl_data_b.config[PCIE_CONF_TYPE] = 0x7fU << 16;
	zassert_equal(pcie_endpoint_get_mbar(&ep_b, 0U, &bar), -ENOTSUP);
	ctrl_data_b.config[PCIE_CONF_TYPE] = UINT32_MAX;
	zassert_equal(pcie_endpoint_get_mbar(&ep_b, 0U, &bar), -ENOENT);
	zassert_equal(ctrl_data_b.writes, 0U);
}

ZTEST(pcie_endpoint, test_bar_header_read_error_precedes_writes)
{
	struct pcie_bar bar;

	ctrl_data_b.fail_read_at = 1U;
	zassert_equal(pcie_endpoint_get_mbar(&ep_b, 0U, &bar), -EIO);
	zassert_equal(ctrl_data_b.status_reads, 1U);
	zassert_equal(ctrl_data_b.writes, 0U);
}
