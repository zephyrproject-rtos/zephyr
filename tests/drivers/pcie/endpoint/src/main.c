/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Process Mission
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pcie/cap.h>
#include <zephyr/drivers/pcie/controller.h>
#include <zephyr/drivers/pcie/msi.h>
#include <zephyr/drivers/pcie/pcie.h>
#include <zephyr/ztest.h>

#define DT_DRV_COMPAT vnd_pcie_fake_ctrl

#define TEST_BDF    PCIE_BDF(0, 1, 0)
#define PHANDLE_BDF PCIE_BDF(0, 2, 0)
#define LEGACY_BDF  PCIE_BDF(3, 3, 0)
#define DECOY_BDF   PCIE_BDF(0, 4, 0)
#define BUS2_BDF    PCIE_BDF(2, 0, 0)
#define BUS4_BDF    PCIE_BDF(4, 0, 0)

#define CTRL_A_NODE      DT_NODELABEL(ctrl_a)
#define CTRL_B_NODE      DT_NODELABEL(ctrl_b)
#define CTRL_NO_MSI_NODE DT_NODELABEL(ctrl_no_msi)

struct fake_ep_slot {
	pcie_bdf_t bdf;
	uint32_t regs[256];
	uintptr_t bar0_phys;
};

struct fake_ctrl_data {
	struct fake_ep_slot slots[6];
	uint32_t msi_setup_count;
	pcie_bdf_t last_msi_bdf;
};

struct fake_ctrl_config {
	uintptr_t bar_base;
};

static struct fake_ep_slot *find_slot(struct fake_ctrl_data *data, pcie_bdf_t bdf)
{
	for (size_t i = 0U; i < ARRAY_SIZE(data->slots); i++) {
		if (data->slots[i].bdf == bdf) {
			return &data->slots[i];
		}
	}

	return NULL;
}

static uint32_t fake_conf_read(const struct device *dev, pcie_bdf_t bdf, unsigned int reg)
{
	struct fake_ctrl_data *data = dev->data;
	struct fake_ep_slot *slot = find_slot(data, bdf);

	if (slot == NULL || reg >= ARRAY_SIZE(slot->regs)) {
		return UINT32_MAX;
	}

	return slot->regs[reg];
}

static void fake_conf_write(const struct device *dev, pcie_bdf_t bdf, unsigned int reg,
			    uint32_t val)
{
	struct fake_ctrl_data *data = dev->data;
	struct fake_ep_slot *slot = find_slot(data, bdf);

	if (slot == NULL || reg >= ARRAY_SIZE(slot->regs)) {
		return;
	}

	if (reg == PCIE_CONF_BAR0 && val == UINT32_MAX) {
		slot->regs[reg] = 0xfffff000U;
		return;
	}

	slot->regs[reg] = val;
}

static bool fake_region_translate(const struct device *dev, pcie_bdf_t bdf, bool mem, bool mem64,
				  uintptr_t bar_bus_addr, uintptr_t *bar_phys_addr)
{
	const struct fake_ctrl_config *cfg = dev->config;

	ARG_UNUSED(bdf);
	ARG_UNUSED(mem);
	ARG_UNUSED(mem64);

	*bar_phys_addr = cfg->bar_base + (bar_bus_addr & 0x0fffffffU);
	return true;
}

static uint8_t fake_msi_setup(const struct device *dev, unsigned int priority,
			      msi_vector_t *vectors, uint8_t n_vector)
{
	struct fake_ctrl_data *data = dev->data;

	data->msi_setup_count++;
	data->last_msi_bdf = vectors[0].bdf;

	for (uint8_t i = 0U; i < n_vector; i++) {
		vectors[i].arch.irq = 40U + i;
		vectors[i].arch.address = 0xfee00000U;
		vectors[i].arch.eventid = 0x10U + i;
		vectors[i].arch.priority = priority;
	}

	return n_vector;
}

static DEVICE_API(pcie_ctrl, fake_ctrl_api) = {
	.conf_read = fake_conf_read,
	.conf_write = fake_conf_write,
	.region_translate = fake_region_translate,
	.msi_device_setup = fake_msi_setup,
};

static DEVICE_API(pcie_ctrl, fake_ctrl_no_msi_api) = {
	.conf_read = fake_conf_read,
	.conf_write = fake_conf_write,
	.region_translate = fake_region_translate,
};

static void init_ep_slot(struct fake_ep_slot *slot, pcie_bdf_t bdf, uint32_t id, uint32_t class_rev,
			 uint32_t cap_word, uint32_t ext_cap_word, uint32_t bar0_bus)
{
	memset(slot, 0, sizeof(*slot));
	slot->bdf = bdf;
	slot->regs[PCIE_CONF_ID] = id;
	slot->regs[PCIE_CONF_CMDSTAT] = PCIE_CONF_CMDSTAT_CAPS;
	slot->regs[PCIE_CONF_CLASSREV] = class_rev;
	slot->regs[PCIE_CONF_TYPE] = PCIE_CONF_TYPE_STANDARD;
	slot->regs[PCIE_CONF_BAR0] = bar0_bus;
	slot->regs[PCIE_CONF_CAPPTR] = cap_word << 2;
	slot->regs[cap_word] = PCI_CAP_ID_MSI;
	slot->regs[PCIE_CONF_EXT_CAPPTR] = (ext_cap_word << 2) << 20;
	slot->regs[ext_cap_word] = PCIE_EXT_CAP_ID_PTM;
}

static void init_host_slot(struct fake_ep_slot *slot)
{
	memset(slot, 0, sizeof(*slot));
	slot->bdf = PCIE_HOST_CONTROLLER(0);
	slot->regs[PCIE_CONF_ID] = PCIE_ID(0x1234U, 0x0001U);
	slot->regs[PCIE_CONF_CLASSREV] = 0x06000000U;
	slot->regs[PCIE_CONF_TYPE] = PCIE_CONF_TYPE_STANDARD << 16;
}

static void init_bridge_slot(struct fake_ep_slot *slot, pcie_bdf_t bdf, uint32_t id,
			     uint8_t secondary)
{
	memset(slot, 0, sizeof(*slot));
	slot->bdf = bdf;
	slot->regs[PCIE_CONF_ID] = id;
	slot->regs[PCIE_CONF_TYPE] = PCIE_CONF_TYPE_PCI_BRIDGE << 16;
	slot->regs[PCIE_BUS_NUMBER] = PCIE_BDF_TO_BUS(bdf) | ((uint32_t)secondary << 8) |
					   ((uint32_t)secondary << 16);
}

static int fake_ctrl_a_init(const struct device *dev)
{
	struct fake_ctrl_data *data = dev->data;

	init_host_slot(&data->slots[0]);
	init_ep_slot(&data->slots[1], TEST_BDF, PCIE_ID(0x1234U, 0x5678U), 0x02000001U, 0x10U,
		     0x40U, 0x00100000U);
	init_ep_slot(&data->slots[2], LEGACY_BDF, PCIE_ID(0x1234U, 0xdef0U), 0x05000001U, 0x10U,
		     0x40U, 0x00200000U);
	init_ep_slot(&data->slots[3], DECOY_BDF, PCIE_ID(0x1234U, 0xbeefU), 0x05000002U, 0x10U,
		     0x40U, 0x00210000U);
	return 0;
}

static int fake_ctrl_b_init(const struct device *dev)
{
	struct fake_ctrl_data *data = dev->data;

	init_host_slot(&data->slots[0]);
	init_ep_slot(&data->slots[1], TEST_BDF, PCIE_ID(0x1234U, 0x5678U), 0x02000002U, 0x18U,
		     0x50U, 0x00300000U);
	init_ep_slot(&data->slots[2], PHANDLE_BDF, PCIE_ID(0x1234U, 0x9abcU), 0x06000001U, 0x18U,
		     0x50U, 0x00400000U);
	init_ep_slot(&data->slots[3], BUS2_BDF, PCIE_ID(0x1234U, 0xcafeU), 0x07000001U, 0x18U,
		     0x50U, 0x00500000U);
	init_ep_slot(&data->slots[4], BUS4_BDF, PCIE_ID(0x1234U, 0xd00dU), 0x09000001U, 0x18U,
		     0x50U, 0x00800000U);
	/* Bus 2 is also reachable from bus 0 through this bridge. */
	init_bridge_slot(&data->slots[5], PCIE_BDF(0, 5, 0), PCIE_ID(0x1234U, 0x0003U), 2U);
	return 0;
}

static int fake_ctrl_no_msi_init(const struct device *dev)
{
	struct fake_ctrl_data *data = dev->data;

	init_host_slot(&data->slots[0]);
	init_ep_slot(&data->slots[1], TEST_BDF, PCIE_ID(0x1234U, 0x5678U), 0x02000003U, 0x10U,
		     0x40U, 0x00600000U);
	return 0;
}

static struct fake_ctrl_data ctrl_a_data;
static const struct fake_ctrl_config ctrl_a_cfg = {.bar_base = 0x10000000U};
DEVICE_DT_DEFINE(CTRL_A_NODE, fake_ctrl_a_init, NULL, &ctrl_a_data, &ctrl_a_cfg, PRE_KERNEL_1,
		 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &fake_ctrl_api);

static struct fake_ctrl_data ctrl_b_data;
static const struct fake_ctrl_config ctrl_b_cfg = {.bar_base = 0x20000000U};
PCIE_CTRL_SCAN_BUSES_DEFINE(CTRL_B_NODE)
DEVICE_DT_DEFINE(CTRL_B_NODE, fake_ctrl_b_init, NULL, &ctrl_b_data, &ctrl_b_cfg, PRE_KERNEL_1,
		 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &fake_ctrl_api);

static struct fake_ctrl_data ctrl_no_msi_data;
static const struct fake_ctrl_config ctrl_no_msi_cfg = {.bar_base = 0x30000000U};
DEVICE_DT_DEFINE(CTRL_NO_MSI_NODE, fake_ctrl_no_msi_init, NULL, &ctrl_no_msi_data, &ctrl_no_msi_cfg,
		 PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &fake_ctrl_no_msi_api);

DEVICE_PCIE_DECLARE(DT_NODELABEL(ep_a));
DEVICE_PCIE_DECLARE(DT_NODELABEL(ep_b));
DEVICE_PCIE_DECLARE(DT_NODELABEL(ep_b_only_on_a));
DEVICE_PCIE_DECLARE(DT_NODELABEL(ep_b_bus2));
DEVICE_PCIE_DECLARE(DT_NODELABEL(ep_b_bus2_dup));
DEVICE_PCIE_DECLARE(DT_NODELABEL(ep_b_bus4));
DEVICE_PCIE_DECLARE(DT_NODELABEL(ep_b_phandle));
DEVICE_PCIE_DECLARE(DT_NODELABEL(ep_legacy));

struct pcie_dev_holder {
	struct pcie_dev *pcie;
};

static const struct pcie_dev_holder ep_a_holder = {DEVICE_PCIE_INIT(DT_NODELABEL(ep_a), pcie)};
static const struct pcie_dev_holder ep_b_holder = {DEVICE_PCIE_INIT(DT_NODELABEL(ep_b), pcie)};
static const struct pcie_dev_holder ep_b_only_on_a_holder = {
	DEVICE_PCIE_INIT(DT_NODELABEL(ep_b_only_on_a), pcie)};
static const struct pcie_dev_holder ep_b_bus2_holder = {
	DEVICE_PCIE_INIT(DT_NODELABEL(ep_b_bus2), pcie)};
static const struct pcie_dev_holder ep_b_bus2_dup_holder = {
	DEVICE_PCIE_INIT(DT_NODELABEL(ep_b_bus2_dup), pcie)};
static const struct pcie_dev_holder ep_b_bus4_holder = {
	DEVICE_PCIE_INIT(DT_NODELABEL(ep_b_bus4), pcie)};
static const struct pcie_dev_holder ep_b_phandle_holder = {
	DEVICE_PCIE_INIT(DT_NODELABEL(ep_b_phandle), pcie)};
static const struct pcie_dev_holder ep_legacy_holder = {
	DEVICE_PCIE_INIT(DT_NODELABEL(ep_legacy), pcie)};

ZTEST(pcie_endpoint, test_conf_cap_bar_isolation)
{
	const struct device *ctrl_a = DEVICE_DT_GET(CTRL_A_NODE);
	const struct device *ctrl_b = DEVICE_DT_GET(CTRL_B_NODE);
	struct pcie_bar bar_a = {0};
	struct pcie_bar bar_b = {0};

	/* Explicit controller selection */
	zassert_equal(pcie_conf_read(PCIE_ENDPOINT(ctrl_a, TEST_BDF), PCIE_CONF_CLASSREV),
		      0x02000001U);
	zassert_equal(pcie_conf_read(PCIE_ENDPOINT(ctrl_b, TEST_BDF), PCIE_CONF_CLASSREV),
		      0x02000002U);

	/* NULL controller falls back to zephyr,pcie-controller (ctrl_a) */
	zassert_equal(pcie_conf_read(PCIE_ENDPOINT_BDF(TEST_BDF), PCIE_CONF_CLASSREV), 0x02000001U);

	/* Writes stay isolated per controller */
	pcie_conf_write(PCIE_ENDPOINT(ctrl_b, TEST_BDF), 0x20U, 0xa5a55a5aU);
	zassert_equal(pcie_conf_read(PCIE_ENDPOINT(ctrl_b, TEST_BDF), 0x20U), 0xa5a55a5aU);
	zassert_equal(pcie_conf_read(PCIE_ENDPOINT(ctrl_a, TEST_BDF), 0x20U), 0U);

	/* Capability and extended capability lookups */
	zassert_equal(pcie_get_cap(PCIE_ENDPOINT_BDF(TEST_BDF), PCI_CAP_ID_MSI), 0x10U);
	zassert_equal(pcie_get_cap(PCIE_ENDPOINT(ctrl_b, TEST_BDF), PCI_CAP_ID_MSI), 0x18U);
	zassert_equal(pcie_get_ext_cap(PCIE_ENDPOINT_BDF(TEST_BDF), PCIE_EXT_CAP_ID_PTM), 0x40U);
	zassert_equal(pcie_get_ext_cap(PCIE_ENDPOINT(ctrl_b, TEST_BDF), PCIE_EXT_CAP_ID_PTM),
		      0x50U);

	/* BAR translation uses each endpoint's controller */
	zassert_true(pcie_get_mbar(PCIE_ENDPOINT_BDF(TEST_BDF), 0U, &bar_a));
	zassert_true(pcie_get_mbar(PCIE_ENDPOINT(ctrl_b, TEST_BDF), 0U, &bar_b));
	zassert_equal(bar_a.phys_addr, 0x10100000U);
	zassert_equal(bar_b.phys_addr, 0x20300000U);
}

struct scan_record {
	const struct device *expected_ctrl;
	uint32_t count;
	bool seen_test_bdf;
	bool seen_second_bdf;
	pcie_bdf_t second_bdf;
};

static bool test_scan_cb(const struct pcie_endpoint *ep, pcie_id_t id, void *cb_data)
{
	struct scan_record *rec = cb_data;

	ARG_UNUSED(id);

	zassert_equal(ep->ctrl, rec->expected_ctrl);
	rec->count++;

	if (ep->bdf == TEST_BDF) {
		rec->seen_test_bdf = true;
	} else if (ep->bdf == rec->second_bdf) {
		rec->seen_second_bdf = true;
	}

	return true;
}

ZTEST(pcie_endpoint, test_scan_isolation)
{
	const struct device *ctrl_a = DEVICE_DT_GET(CTRL_A_NODE);
	const struct device *ctrl_b = DEVICE_DT_GET(CTRL_B_NODE);
	struct scan_record rec_a = {
		.expected_ctrl = ctrl_a,
		.second_bdf = DECOY_BDF,
	};
	struct scan_record rec_b = {
		.expected_ctrl = ctrl_b,
		.second_bdf = PHANDLE_BDF,
	};
	struct pcie_scan_opt opt_a = {
		.ctrl = NULL,
		.bus = 0U,
		.cb = test_scan_cb,
		.cb_data = &rec_a,
	};
	struct pcie_scan_opt opt_b = {
		.ctrl = ctrl_b,
		.bus = 0U,
		.cb = test_scan_cb,
		.cb_data = &rec_b,
	};

	zassert_equal(pcie_scan(&opt_a), 0);
	zassert_equal(rec_a.count, 3U);
	zassert_true(rec_a.seen_test_bdf);
	zassert_true(rec_a.seen_second_bdf);

	zassert_equal(pcie_scan(&opt_b), 0);
	zassert_equal(rec_b.count, 3U);
	zassert_true(rec_b.seen_test_bdf);
	zassert_true(rec_b.seen_second_bdf);
}

ZTEST(pcie_endpoint, test_static_binding)
{
	const struct device *ctrl_a = DEVICE_DT_GET(CTRL_A_NODE);
	const struct device *ctrl_b = DEVICE_DT_GET(CTRL_B_NODE);

	zassert_equal(ep_a_holder.pcie->ctrl, ctrl_a);
	zassert_equal(ep_a_holder.pcie->bdf, TEST_BDF);
	zassert_equal(ep_a_holder.pcie->class_rev, 0x02000001U);

	zassert_equal(ep_b_holder.pcie->ctrl, ctrl_b);
	zassert_equal(ep_b_holder.pcie->bdf, TEST_BDF);
	zassert_equal(ep_b_holder.pcie->class_rev, 0x02000002U);

	zassert_equal(ep_b_phandle_holder.pcie->ctrl, ctrl_b);
	zassert_equal(ep_b_phandle_holder.pcie->bdf, PHANDLE_BDF);

	/* Declaration on ctrl_b must not bind to matching VID/DID on ctrl_a */
	zassert_equal(ep_b_only_on_a_holder.pcie->ctrl, ctrl_b);
	zassert_equal(ep_b_only_on_a_holder.pcie->bdf, PCIE_BDF_NONE);

	/* Bus 2 is reachable through the bridge and also listed in scan-buses. */
	zassert_equal(ep_b_bus2_holder.pcie->ctrl, ctrl_b);
	zassert_equal(ep_b_bus2_dup_holder.pcie->ctrl, ctrl_b);
	zassert_true((ep_b_bus2_holder.pcie->bdf == BUS2_BDF &&
		      ep_b_bus2_dup_holder.pcie->bdf == PCIE_BDF_NONE) ||
		     (ep_b_bus2_holder.pcie->bdf == PCIE_BDF_NONE &&
		      ep_b_bus2_dup_holder.pcie->bdf == BUS2_BDF));

	/* Bus 4 is only reachable through scan-buses. */
	zassert_equal(ep_b_bus4_holder.pcie->ctrl, ctrl_b);
	zassert_equal(ep_b_bus4_holder.pcie->bdf, BUS4_BDF);

	/* The legacy node binds through the chosen controller's additional bus. */
	zassert_is_null(ep_legacy_holder.pcie->ctrl);
	zassert_equal(ep_legacy_holder.pcie->bdf, LEGACY_BDF);
	zassert_equal(pcie_conf_read(PCIE_DEV_ENDPOINT(ep_legacy_holder.pcie), PCIE_CONF_ID),
		      PCIE_ID(0x1234U, 0xdef0U));
}

#ifndef CONFIG_DYNAMIC_INTERRUPTS
int arch_irq_connect_dynamic(unsigned int irq, unsigned int priority,
			     void (*routine)(const void *parameter), const void *parameter,
			     uint32_t flags)
{
	ARG_UNUSED(priority);
	ARG_UNUSED(routine);
	ARG_UNUSED(parameter);
	ARG_UNUSED(flags);

	return (int)irq;
}
#endif

static void dummy_isr(const void *param)
{
	ARG_UNUSED(param);
}

ZTEST(pcie_endpoint, test_msi_controller_routing)
{
	const struct device *ctrl_a = DEVICE_DT_GET(CTRL_A_NODE);
	const struct device *ctrl_b = DEVICE_DT_GET(CTRL_B_NODE);
	const struct device *ctrl_no_msi = DEVICE_DT_GET(CTRL_NO_MSI_NODE);
	msi_vector_t vec_a = {0};
	msi_vector_t vec_b = {0};

	ctrl_a_data.msi_setup_count = 0U;
	ctrl_b_data.msi_setup_count = 0U;

	zassert_true(pcie_is_msi(PCIE_ENDPOINT(ctrl_b, TEST_BDF)));
	zassert_equal(pcie_msi_vectors_allocate(PCIE_ENDPOINT(ctrl_b, TEST_BDF), 0U, &vec_b, 1U),
		      1U);
	zassert_equal(ctrl_b_data.msi_setup_count, 1U);
	zassert_equal(ctrl_a_data.msi_setup_count, 0U);
	zassert_equal(vec_b.ctrl, ctrl_b);
	zassert_equal(vec_b.bdf, TEST_BDF);

	/* Vector allocated on ctrl_b must be rejected by ctrl_a and NULL (chosen = ctrl_a) */
	zassert_false(pcie_msi_vector_connect(PCIE_ENDPOINT(ctrl_a, TEST_BDF), &vec_b, dummy_isr,
					      NULL, 0U));
	zassert_false(
		pcie_msi_vector_connect(PCIE_ENDPOINT_BDF(TEST_BDF), &vec_b, dummy_isr, NULL, 0U));
	zassert_false(pcie_msi_enable(PCIE_ENDPOINT(ctrl_a, TEST_BDF), &vec_b, 1U, 40U));
	zassert_false(pcie_msi_enable(PCIE_ENDPOINT_BDF(TEST_BDF), &vec_b, 1U, 40U));
	zassert_true(pcie_msi_vector_connect(PCIE_ENDPOINT(ctrl_b, TEST_BDF), &vec_b, dummy_isr,
					     NULL, 0U));
	zassert_true(pcie_msi_enable(PCIE_ENDPOINT(ctrl_b, TEST_BDF), &vec_b, 1U, 40U));
	zassert_equal(pcie_conf_read(PCIE_ENDPOINT(ctrl_b, TEST_BDF), 0x18U + PCIE_MSI_MAP0),
		      0xfee00000U);
	zassert_equal(pcie_conf_read(PCIE_ENDPOINT(ctrl_b, TEST_BDF), 0x18U + PCIE_MSI_MDR_32),
		      0x10U);
	zassert_not_equal(pcie_conf_read(PCIE_ENDPOINT(ctrl_b, TEST_BDF), 0x18U + PCIE_MSI_MCR) &
				  PCIE_MSI_MCR_EN,
			  0U);
	zassert_equal(pcie_conf_read(PCIE_ENDPOINT(ctrl_a, TEST_BDF), 0x10U + PCIE_MSI_MAP0), 0U);
	zassert_equal(pcie_conf_read(PCIE_ENDPOINT(ctrl_a, TEST_BDF), 0x10U + PCIE_MSI_MDR_32), 0U);
	zassert_equal(pcie_conf_read(PCIE_ENDPOINT(ctrl_a, TEST_BDF), 0x10U + PCIE_MSI_MCR) &
			      PCIE_MSI_MCR_EN,
		      0U);

	/* NULL controller defaults to chosen (ctrl_a) and is equivalent to explicit ctrl_a */
	zassert_equal(pcie_msi_vectors_allocate(PCIE_ENDPOINT_BDF(LEGACY_BDF), 0U, &vec_a, 1U), 1U);
	zassert_equal(ctrl_a_data.msi_setup_count, 1U);
	zassert_equal(ctrl_a_data.last_msi_bdf, LEGACY_BDF);
	zassert_is_null(vec_a.ctrl);
	zassert_true(pcie_msi_enable(PCIE_ENDPOINT(ctrl_a, LEGACY_BDF), &vec_a, 1U, 40U));

	/* Controller without msi_device_setup returns 0 safely */
	zassert_equal(
		pcie_msi_vectors_allocate(PCIE_ENDPOINT(ctrl_no_msi, TEST_BDF), 0U, &vec_b, 1U),
		0U);
}

static int unready_ctrl_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	return -EIO;
}

static struct fake_ctrl_data ctrl_unready_data;
DEVICE_DEFINE(ctrl_unready, "pcie_ctrl_unready", unready_ctrl_init, NULL, &ctrl_unready_data, NULL,
	      POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, &fake_ctrl_api);

ZTEST(pcie_endpoint, test_scan_unready_ctrl_enodev)
{
	const struct device *ctrl_no_msi = DEVICE_DT_GET(CTRL_NO_MSI_NODE);
	const struct device *ctrl_unready = DEVICE_GET(ctrl_unready);
	struct scan_record rec = {
		.expected_ctrl = ctrl_no_msi,
		.second_bdf = PCIE_BDF(1, 0, 0),
	};
	struct pcie_scan_opt opt = {
		.ctrl = ctrl_unready,
		.bus = 0U,
		.cb = test_scan_cb,
		.cb_data = &rec,
		.flags = PCIE_SCAN_RECURSIVE,
	};

	/* Unready controller must return -ENODEV from pcie_scan() */
	zassert_equal(pcie_scan(&opt), -ENODEV);

	/* Recursive bridge scan preserves ctrl_no_msi on secondary bus endpoints */
	init_bridge_slot(&ctrl_no_msi_data.slots[2], PCIE_BDF(0, 5, 0), PCIE_ID(0x1234U, 0x1111U),
			 1U);
	init_ep_slot(&ctrl_no_msi_data.slots[3], PCIE_BDF(1, 0, 0), PCIE_ID(0x1234U, 0x2222U),
		     0x02000004U, 0x10U, 0x40U, 0x00700000U);

	opt.ctrl = ctrl_no_msi;
	zassert_equal(pcie_scan(&opt), 0);
	zassert_true(rec.seen_second_bdf);
	zassert_equal(rec.count, 3U);
}

ZTEST_SUITE(pcie_endpoint, NULL, NULL, NULL, NULL, NULL);
