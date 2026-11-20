/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>
#include <zephyr/drivers/pcie/controller.h>
#include <zephyr/drivers/pcie/cap.h>
#include <zephyr/ztest.h>
#include <zephyr/ztest_error_hook.h>

#define TEST_BDF PCIE_BDF(1, 0, 0)
#define TEST_ID PCIE_ID(0x1234, 0x5678)
#define TEST_CLASS 0x01080200U
#define TEST_CAP 20U
#define TEST_MSI_CAP 24U
#define TEST_TARGET 0x123456789abcdef0ULL
#define TEST_IRQ (IS_ENABLED(CONFIG_ARM64) ? 40U : 10U)

struct fake_ctrl {
	uint32_t config[1024];
	struct msix_vector table[256];
	uint32_t bus_address;
	uint32_t reads;
	uint32_t root_reads;
	uint32_t status_reads;
	uint32_t writes;
	uint32_t write_attempts;
	uint32_t fail_write_at;
	uint32_t fail_read_at;
	uint32_t old_setups;
	uint32_t setups;
	uint32_t irqs;
	uint32_t mappings;
	uint32_t table_maps;
	uint32_t table_unmaps;
	size_t last_unmap_size;
	uint8_t *table_virt[2];
	size_t table_size[2];
	int read_error;
	int write_error;
	int fail_at;
	int setup_error;
	uint8_t bad_setup_count;
	bool translate;
	bool reject_bus_zero;
	bool msi_allocated;
};

static struct fake_ctrl ctrl_data_a, ctrl_data_b;
static bool early_found[2];

#ifdef CONFIG_MMU
void __real_k_mem_map_phys_bare(uint8_t **virt, uintptr_t phys, size_t size, uint32_t flags);
void __real_k_mem_unmap_phys_bare(uint8_t *virt, size_t size);

/* Map fake RAM with matching cache attributes while exercising real MMU ownership. */
void __wrap_k_mem_map_phys_bare(uint8_t **virt, uintptr_t phys, size_t size, uint32_t flags)
{
	struct fake_ctrl *controllers[] = {&ctrl_data_a, &ctrl_data_b};

	for (size_t i = 0U; i < ARRAY_SIZE(controllers); i++) {
		struct fake_ctrl *data = controllers[i];

		if (phys == (uintptr_t)data->table) {
			zassert_true(size <= sizeof(data->table));
			zassert_equal(flags & K_MEM_DIRECT_MAP, 0U);
			__real_k_mem_map_phys_bare(virt, phys, size,
						 K_MEM_CACHE_WB | K_MEM_PERM_RW);
			for (size_t j = 0U; j < ARRAY_SIZE(data->table_virt); j++) {
				if (data->table_virt[j] == NULL) {
					data->table_virt[j] = *virt;
					data->table_size[j] = size;
					data->table_maps++;
					return;
				}
			}
			zassert_unreachable("too many live fake table mappings");
		}
	}
	__real_k_mem_map_phys_bare(virt, phys, size, flags);
}

void __wrap_k_mem_unmap_phys_bare(uint8_t *virt, size_t size)
{
	struct fake_ctrl *controllers[] = {&ctrl_data_a, &ctrl_data_b};

	for (size_t i = 0U; i < ARRAY_SIZE(controllers); i++) {
		struct fake_ctrl *data = controllers[i];

		for (size_t j = 0U; j < ARRAY_SIZE(data->table_virt); j++) {
			if (virt == data->table_virt[j]) {
				__real_k_mem_unmap_phys_bare(virt, size);
				data->table_virt[j] = NULL;
				data->table_unmaps++;
				data->last_unmap_size = size;
				return;
			}
		}
	}
	__real_k_mem_unmap_phys_bare(virt, size);
}
#endif

DEVICE_PCIE_ENDPOINT_DECLARE(DT_NODELABEL(decl_a));
DEVICE_PCIE_ENDPOINT_DECLARE(DT_NODELABEL(decl_b));

STRUCT_SECTION_ITERABLE(pcie_dev, zz_extra_a) = {
	.bdf = PCIE_BDF_NONE,
	.id = TEST_ID,
	.ctrl = DEVICE_DT_GET(DT_NODELABEL(ctrl_a)),
};

STRUCT_SECTION_ITERABLE(pcie_dev, zz_extra_b) = {
	.bdf = PCIE_BDF_NONE,
	.id = TEST_ID,
	.ctrl = DEVICE_DT_GET(DT_NODELABEL(ctrl_a)),
};

static void reset_ctrl(struct fake_ctrl *data, bool msix)
{
#ifdef CONFIG_MMU
	for (size_t i = 0U; i < ARRAY_SIZE(data->table_virt); i++) {
		if (data->table_virt[i] != NULL) {
			__real_k_mem_unmap_phys_bare(data->table_virt[i], data->table_size[i]);
		}
	}
#endif
	memset(data, 0, sizeof(*data));
	data->config[PCIE_CONF_ID] = TEST_ID;
	data->config[PCIE_CONF_CLASSREV] = TEST_CLASS;
	data->config[PCIE_CONF_CMDSTAT] = PCIE_CONF_CMDSTAT_CAPS | PCIE_CONF_CMDSTAT_MEM;
	data->config[PCIE_CONF_CAPPTR] = msix ? TEST_CAP * 4U : 16U * 4U;
	data->config[16] = PCI_CAP_ID_VNDR;
	data->config[TEST_CAP] = PCI_CAP_ID_MSIX | (3U << PCIE_MSIX_MCR_TSIZE_SHIFT);
	data->config[TEST_CAP + PCIE_MSIX_TR] = 0U;
	data->config[PCIE_CONF_BAR0] = msix ? 0x20000000U : 0x10000000U;
	data->bus_address = data->config[PCIE_CONF_BAR0];
	data->config[PCIE_CONF_EXT_CAPPTR] = 0x00010001U | (0x110U << 20);
	data->config[68] = 0x00010002U;
	data->config[32] = msix ? 0xbbbbU : 0xaaaaU;
	data->config[33] = UINT32_MAX;
	data->translate = true;
	data->fail_at = -1;
	data->setup_error = -ENOSPC;
}

static uint32_t fake_read(const struct device *dev, pcie_bdf_t bdf, unsigned int reg)
{
	struct fake_ctrl *data = dev->data;

	data->reads++;
	if (bdf == PCIE_BDF(0, 0, 0)) {
		data->root_reads++;
	}
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
	if (bdf != TEST_BDF && bdf != PCIE_BDF(2, 0, 0) && bdf != PCIE_BDF(128, 0, 0)) {
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
	if (data->reject_bus_zero && PCIE_BDF_TO_BUS(bdf) == 0U) {
		return -ERANGE;
	}
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

static int fake_setup(const struct device *dev, unsigned int priority, msi_vector_t *vectors,
		      uint8_t count, uint8_t *allocated)
{
	struct fake_ctrl *data = dev->data;

	data->setups++;
	*allocated = 0U;
	if (data->msi_allocated) {
		return -EALREADY;
	}
	for (uint8_t i = 0U; i < count; i++) {
		zassert_equal_ptr(vectors[i].ctrl, dev);
		zassert_equal(vectors[i].bdf, TEST_BDF);
		if (i == data->fail_at) {
			/* Revoke every fake ITS mapping and IRQ acquired by this request. */
			data->irqs -= i;
			data->mappings -= i;
			return data->setup_error;
		}
		data->irqs++;
		data->mappings++;
		vectors[i].arch.irq = TEST_IRQ + i;
		vectors[i].arch.address = TEST_TARGET;
		vectors[i].arch.eventid = i;
		vectors[i].arch.priority = priority;
	}
	*allocated = count;
	data->msi_allocated = true;
	if (data->bad_setup_count == 1U) {
		*allocated = count - 1U;
	} else if (data->bad_setup_count == 2U) {
		return -EIO;
	}
	return 0;
}

static uint8_t fake_old_setup(const struct device *dev, unsigned int priority,
			      msi_vector_t *vectors, uint8_t count)
{
	struct fake_ctrl *data = dev->data;

	data->old_setups++;
	for (uint8_t i = 0U; i < count; i++) {
		if (i == data->fail_at) {
			return i;
		}
		data->irqs++;
		vectors[i].arch.irq = TEST_IRQ + i;
		vectors[i].arch.address = TEST_TARGET;
		vectors[i].arch.eventid = i;
		vectors[i].arch.priority = priority;
	}
	return count;
}

static DEVICE_API(pcie_ctrl, fake_api_a) = {
	.conf_read = fake_read,
	.conf_write = fake_write,
	.region_translate = fake_translate,
	.msi_device_setup = fake_old_setup,
	.msi_device_setup_errno = fake_setup,
};

static DEVICE_API(pcie_ctrl, fake_api_b) = {
	.conf_read = fake_read,
	.conf_write = fake_write,
	.region_translate = fake_translate,
	.conf_read_status = fake_read_status,
	.conf_write_status = fake_write_status,
	.msi_device_setup = fake_old_setup,
	.msi_device_setup_errno = fake_setup,
};

static DEVICE_API(pcie_ctrl, old_api) = {
	.conf_read = fake_read,
	.conf_write = fake_write,
	.region_translate = fake_translate,
	.msi_device_setup = fake_old_setup,
};

static DEVICE_API(pcie_ctrl, empty_api) = {
	.conf_read = NULL,
};

static bool early_scan(const struct pcie_endpoint *ep, pcie_id_t id, void *arg)
{
	bool *found = arg;

	if (id == TEST_ID) {
		*found = ep->bdf == TEST_BDF;
		return false;
	}
	return true;
}

static int fake_init(const struct device *dev)
{
	struct fake_ctrl *data = dev->data;
	struct pcie_scan_ctrl_opt opt = {
		.cb = early_scan,
		.cb_data = &early_found[dev == DEVICE_DT_GET(DT_NODELABEL(ctrl_b))],
		.flags = PCIE_SCAN_RECURSIVE,
	};

	reset_ctrl(data, dev == DEVICE_DT_GET(DT_NODELABEL(ctrl_b)));
	if (IS_ENABLED(PCIE_TEST_FAIL_CTRL_B) && dev == DEVICE_DT_GET(DT_NODELABEL(ctrl_b))) {
		return -EIO;
	}
	return pcie_scan_ctrl(dev, &opt);
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
	if (IS_ENABLED(PCIE_TEST_FAIL_CTRL_B)) {
		ztest_test_skip();
	}
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

struct scan_result {
	const struct device *ctrl;
	uint32_t count;
	bool stop;
};

static bool scan_cb(const struct pcie_endpoint *ep, pcie_id_t id, void *arg)
{
	struct scan_result *result = arg;

	zassert_equal_ptr(ep->ctrl, result->ctrl);
	zassert_equal(id, TEST_ID);
	result->count++;
	return !result->stop;
}

static bool mask_bdf(const struct pcie_endpoint *ep, void *cb_data)
{
	ARG_UNUSED(cb_data);

	zassert_equal_ptr(ep->ctrl, ep_b.ctrl);
	return ep->bdf == PCIE_BDF(1, 1, 0);
}

ZTEST(pcie_endpoint, test_scan_recursion_mask_and_stop)
{
	struct scan_result result = {.ctrl = ep_b.ctrl};
	struct pcie_scan_ctrl_opt opt = {
		.cb = scan_cb, .cb_data = &result, .flags = PCIE_SCAN_RECURSIVE,
	};

	zassert_ok(pcie_scan_ctrl(ep_b.ctrl, &opt));
	zassert_equal(result.count, 2U);
	zassert_equal(ctrl_data_a.reads, 0U);
	result.count = 0U;
	opt.scan_masking_fn = mask_bdf;
	zassert_ok(pcie_scan_ctrl(ep_b.ctrl, &opt));
	zassert_equal(result.count, 1U);
	result.count = 0U;
	opt.scan_masking_fn = NULL;
	result.stop = true;
	zassert_ok(pcie_scan_ctrl(ep_b.ctrl, &opt));
	zassert_equal(result.count, 1U);
	zassert_equal(pcie_scan_ctrl(NULL, &opt), -EINVAL);
	zassert_equal(pcie_scan_ctrl(ep_b.ctrl, NULL), -EINVAL);
	zassert_equal(pcie_scan(NULL), -EINVAL);
	ctrl_data_b.read_error = -EIO;
	zassert_equal(pcie_scan_ctrl(ep_b.ctrl, &opt), -EIO);
}

static bool mask_root(const struct pcie_endpoint *ep, void *cb_data)
{
	ARG_UNUSED(cb_data);

	return ep->bdf == PCIE_BDF(0, 0, 0);
}

ZTEST(pcie_endpoint, test_scan_nonzero_bus_does_not_access_bus_zero)
{
	struct scan_result result = {.ctrl = ep_b.ctrl};
	struct pcie_scan_ctrl_opt opt = {
		.bus = 128U, .cb = scan_cb, .cb_data = &result, .flags = PCIE_SCAN_RECURSIVE,
	};

	ctrl_data_b.reject_bus_zero = true;
	zassert_ok(pcie_scan_ctrl(ep_b.ctrl, &opt));
	zassert_equal(result.count, 1U);
	zassert_equal(ctrl_data_b.root_reads, 0U);
	zassert_equal(ctrl_data_a.reads, 0U);
}

ZTEST(pcie_endpoint, test_scan_masked_root_is_not_read)
{
	struct scan_result result = {.ctrl = ep_b.ctrl};
	struct pcie_scan_ctrl_opt opt = {
		.cb = scan_cb, .cb_data = &result, .flags = PCIE_SCAN_RECURSIVE,
		.scan_masking_fn = mask_root,
	};

	zassert_ok(pcie_scan_ctrl(ep_b.ctrl, &opt));
	zassert_equal(result.count, 0U);
	zassert_equal(ctrl_data_b.root_reads, 0U);
	zassert_equal(ctrl_data_a.reads, 0U);
}

ZTEST(pcie_binding, test_static_declarations_bind_only_to_owner)
{
	struct pcie_dev *a = &Z_DEVICE_PCIE_NAME(DT_NODELABEL(decl_a));
	struct pcie_dev *b = &Z_DEVICE_PCIE_NAME(DT_NODELABEL(decl_b));
	struct pcie_endpoint ep = pcie_dev_endpoint(a);

	zassert_equal_ptr(a->ctrl, DEVICE_DT_GET(PCIE_DT_CONTROLLER(DT_NODELABEL(decl_a))));
	zassert_equal_ptr(b->ctrl, DEVICE_DT_GET(PCIE_DT_CONTROLLER(DT_NODELABEL(decl_b))));
	zassert_not_equal(a->ctrl, b->ctrl);
	zassert_equal(a->bdf, device_is_ready(a->ctrl) ? TEST_BDF : PCIE_BDF_NONE);
	zassert_equal(b->bdf, device_is_ready(b->ctrl) ? TEST_BDF : PCIE_BDF_NONE);
	zassert_equal(ep.bdf, a->bdf);
	zassert_equal_ptr(ep.ctrl, a->ctrl);
	zassert_equal(zz_extra_a.bdf, PCIE_BDF(2, 0, 0));
	zassert_equal(zz_extra_b.bdf, PCIE_BDF_NONE);
}

static void handler(const void *arg)
{
	ARG_UNUSED(arg);
}

static bool legacy_scan_cb(pcie_bdf_t bdf, pcie_id_t id, void *arg)
{
	uint32_t *count = arg;

	zassert_true(bdf == TEST_BDF || bdf == PCIE_BDF(2, 0, 0));
	zassert_equal(id, TEST_ID);
	(*count)++;
	return true;
}

static bool legacy_scan_mask(pcie_bdf_t bdf)
{
	return bdf == PCIE_BDF(1, 1, 0);
}

ZTEST(pcie_endpoint, test_legacy_scan_mask_and_callback)
{
	uint32_t count = 0U;
	struct pcie_scan_opt opt = {
		.cb = legacy_scan_cb, .cb_data = &count, .flags = PCIE_SCAN_RECURSIVE,
		.scan_masking_fn = legacy_scan_mask,
	};

	zassert_ok(pcie_scan(&opt));
	zassert_equal(count, 1U);
	zassert_equal(ctrl_data_b.reads, 0U);
}

static bool legacy_mask_root(pcie_bdf_t bdf)
{
	return bdf == PCIE_BDF(0, 0, 0);
}

ZTEST(pcie_endpoint, test_legacy_scan_masked_root_is_not_read)
{
	uint32_t count = 0U;
	struct pcie_scan_opt opt = {
		.cb = legacy_scan_cb, .cb_data = &count, .flags = PCIE_SCAN_RECURSIVE,
		.scan_masking_fn = legacy_mask_root,
	};

	zassert_ok(pcie_scan(&opt));
	zassert_equal(count, 0U);
	zassert_equal(ctrl_data_a.root_reads, 0U);
	zassert_equal(ctrl_data_b.reads, 0U);
}

ZTEST(pcie_endpoint, test_legacy_msi_partial_allocation_and_enable)
{
	msi_vector_t vectors[4];
	uint32_t command = ctrl_data_a.config[PCIE_CONF_CMDSTAT];

	reset_ctrl(&ctrl_data_a, true);
	ctrl_data_a.fail_at = 2;
	zassert_equal(pcie_msi_vectors_allocate(TEST_BDF, 0U, vectors, 4U), 2U);
	zassert_equal(ctrl_data_a.old_setups, 1U);
	zassert_equal(ctrl_data_a.setups, 0U);
	zassert_equal(ctrl_data_a.irqs, 2U);
	zassert_equal_ptr(vectors[0].ctrl, ep_a.ctrl);
	zassert_true(pcie_msi_vector_connect(TEST_BDF, &vectors[0], handler, NULL, 0U));
	zassert_true(pcie_msi_enable(TEST_BDF, vectors, 2U, 0U));
	irq_disable(TEST_IRQ);
	irq_disable(TEST_IRQ + 1U);
	zassert_equal(ctrl_data_a.table[1].msg_up_addr, (uint32_t)(TEST_TARGET >> 32));
	zassert_equal(ctrl_data_a.config[PCIE_CONF_CMDSTAT], command | PCIE_CONF_CMDSTAT_MASTER);
	zassert_equal(ctrl_data_b.reads, 0U);
	reset_ctrl(&ctrl_data_a, false);
	ctrl_data_a.config[PCIE_CONF_CAPPTR] = TEST_MSI_CAP * 4U;
	ctrl_data_a.config[TEST_MSI_CAP] = PCI_CAP_ID_MSI | PCIE_MSI_MCR_64 |
		(2U << PCIE_MSI_MCR_MMC_SHIFT);
	zassert_equal(pcie_msi_vectors_allocate(TEST_BDF, 0U, vectors, 4U), 4U);
	zassert_true(pcie_msi_enable(TEST_BDF, vectors, 4U, 0U));
	zassert_true(pcie_msi_enable(TEST_BDF, vectors, 1U, 0U));
	zassert_equal(ctrl_data_a.config[TEST_MSI_CAP] & PCIE_MSI_MCR_MME, 0U);
}

ZTEST(pcie_endpoint, test_msi_old_only_backend_rejected_before_access)
{
	const struct pcie_endpoint ep = {.ctrl = DEVICE_GET(old_ctrl), .bdf = TEST_BDF};
	msi_vector_t vector;
	uint8_t count = 99U;

	zassert_equal(pcie_endpoint_msi_vectors_allocate(&ep, 0U, &vector, 1U, &count), -ENOTSUP);
	zassert_equal(count, 0U);
	zassert_equal(ctrl_data_b.reads, 0U);
	zassert_equal(ctrl_data_b.writes, 0U);
	zassert_equal(ctrl_data_b.old_setups, 0U);
}

ZTEST(pcie_endpoint, test_msi_validation_precedes_backend)
{
	const struct pcie_endpoint unset = {.bdf = TEST_BDF};
	msi_vector_t vector = {0};
	uint8_t count = 99U;

	zassert_equal(pcie_endpoint_msi_vectors_allocate(NULL, 0U, &vector, 1U, &count), -EINVAL);
	zassert_equal(count, 0U);
	zassert_equal(pcie_endpoint_msi_vectors_allocate(&unset, 0U, &vector, 1U, &count), -EINVAL);
	zassert_equal(pcie_endpoint_msi_vectors_allocate(&ep_empty, 0U, NULL, 1U, &count), -EINVAL);
	zassert_equal(pcie_endpoint_msi_vectors_allocate(&ep_empty, 0U, &vector, 0U, &count),
		      -EINVAL);
	zassert_equal(pcie_endpoint_msi_vectors_allocate(&ep_empty, 0U, &vector, 1U, NULL),
		      -EINVAL);
	zassert_equal(pcie_endpoint_msi_vectors_allocate(&ep_empty, 0U, &vector, 1U, &count),
		      -ENOTSUP);
	zassert_equal(pcie_endpoint_msi_enable(&unset, &vector, 1U, 0U), -EINVAL);
	zassert_equal(pcie_endpoint_msi_vector_connect(&unset, &vector, handler, NULL, 0U),
		      -EINVAL);
}

ZTEST(pcie_endpoint, test_msix_lifecycle_uses_one_owner)
{
	msi_vector_t vectors[2];
	uint8_t count;

	zassert_ok(pcie_endpoint_msi_vectors_allocate(&ep_b, 0U, vectors, 2U, &count));
	zassert_equal(count, 2U);
	zassert_equal_ptr(vectors[0].ctrl, ep_b.ctrl);
	if (IS_ENABLED(CONFIG_MMU)) {
		zassert_equal_ptr(vectors[0].msix_vector, ctrl_data_b.table_virt[0]);
	} else {
		zassert_equal_ptr(vectors[0].msix_vector, ctrl_data_b.table);
	}
	zassert_ok(pcie_endpoint_msi_vector_connect(&ep_b, &vectors[0], handler, NULL, 0U));
	irq_disable(TEST_IRQ);
	zassert_equal(ctrl_data_a.reads, 0U);
	zassert_false(pcie_msi_vector_connect(TEST_BDF, &vectors[0], handler, NULL, 0U));
	zassert_equal(pcie_endpoint_msi_vector_connect(&ep_a, &vectors[0], handler, NULL, 0U),
		      -EINVAL);
	ctrl_data_a.reads = 0U;
	zassert_ok(pcie_endpoint_msi_enable(&ep_b, vectors, count, 0U));
	zassert_equal(ctrl_data_a.reads, 0U);
	zassert_equal(ctrl_data_a.writes, 0U);
	zassert_equal(ctrl_data_b.table[0].msg_addr, (uint32_t)TEST_TARGET);
	zassert_equal(ctrl_data_b.table[0].msg_up_addr, (uint32_t)(TEST_TARGET >> 32));
	zassert_equal(ctrl_data_b.table[1].msg_data, 1U);
	zassert_true((ctrl_data_b.config[TEST_CAP] & PCIE_MSIX_MCR_EN) != 0U);
}

ZTEST(pcie_endpoint, test_msi_failure_rolls_back_resources)
{
	msi_vector_t vectors[4];
	uint8_t count;

	for (int failure = 0; failure < 4; failure++) {
		ctrl_data_b.fail_at = failure;
		zassert_equal(pcie_endpoint_msi_vectors_allocate(&ep_b, 0U, vectors, 4U, &count),
			      -ENOSPC);
		zassert_equal(count, 0U);
		zassert_equal(ctrl_data_b.irqs, 0U);
		zassert_equal(ctrl_data_b.mappings, 0U);
		if (IS_ENABLED(CONFIG_MMU)) {
			zassert_equal(ctrl_data_b.table_maps, failure + 1U);
			zassert_equal(ctrl_data_b.table_unmaps, failure + 1U);
			zassert_equal(ctrl_data_b.last_unmap_size,
				      sizeof(struct msix_vector) * 4U);
		}
		for (uint8_t i = 0U; i < 4U; i++) {
			zassert_is_null(vectors[i].ctrl);
			zassert_is_null(vectors[i].msix_vector);
		}
	}
	ctrl_data_b.fail_at = 0;
	ctrl_data_b.setup_error = -ENOTSUP;
	zassert_equal(pcie_endpoint_msi_vectors_allocate(&ep_b, 0U, vectors, 1U, &count), -ENOTSUP);
	zassert_equal(count, 0U);
}

ZTEST(pcie_endpoint, test_enable_failure_keeps_allocated_vectors)
{
	msi_vector_t vector;
	uint8_t count;

	zassert_ok(pcie_endpoint_msi_vectors_allocate(&ep_b, 0U, &vector, 1U, &count));
	ctrl_data_b.write_error = -EIO;
	zassert_equal(pcie_endpoint_msi_enable(&ep_b, &vector, 1U, 0U), -EIO);
	zassert_equal(ctrl_data_b.irqs, 1U);
	zassert_equal(ctrl_data_b.mappings, 1U);
	zassert_equal_ptr(vector.ctrl, ep_b.ctrl);
	ctrl_data_b.write_error = 0;
	zassert_ok(pcie_endpoint_msi_enable(&ep_b, &vector, 1U, 0U));
}

ZTEST(pcie_endpoint, test_late_enable_failure_keeps_connected_vectors)
{
	for (uint8_t mode = 0U; mode < 2U; mode++) {
		bool msix = mode != 0U;
		msi_vector_t vector, saved;
		uint8_t count;

		reset_ctrl(&ctrl_data_b, msix);
		if (!msix) {
			ctrl_data_b.config[PCIE_CONF_CAPPTR] = TEST_MSI_CAP * 4U;
			ctrl_data_b.config[TEST_MSI_CAP] = PCI_CAP_ID_MSI | PCIE_MSI_MCR_64;
		}
		zassert_ok(pcie_endpoint_msi_vectors_allocate(&ep_b, 0U, &vector, 1U, &count));
		zassert_ok(pcie_endpoint_msi_vector_connect(&ep_b, &vector, handler, NULL, 0U));
		zassert_true(irq_is_enabled(TEST_IRQ));
		memcpy(&saved, &vector, sizeof(saved));
		ctrl_data_b.write_attempts = 0U;
		/* Fail the bus-master write after enabling the interrupt source. */
		ctrl_data_b.fail_write_at = msix ? 3U : 5U;
		zassert_equal(pcie_endpoint_msi_enable(&ep_b, &vector, 1U, 0U), -EIO);
		zassert_mem_equal(&vector, &saved, sizeof(vector));
		zassert_equal(ctrl_data_b.irqs, 1U);
		zassert_equal(ctrl_data_b.mappings, 1U);
		zassert_true(irq_is_enabled(TEST_IRQ));
		zassert_true((ctrl_data_b.config[msix ? TEST_CAP : TEST_MSI_CAP] &
			      (msix ? PCIE_MSIX_MCR_EN : PCIE_MSI_MCR_EN)) != 0U);
		if (msix) {
			zassert_equal(ctrl_data_b.config[TEST_CAP] & PCIE_MSIX_MCR_FMASK, 0U);
		}
		zassert_equal(ctrl_data_b.config[PCIE_CONF_CMDSTAT] & PCIE_CONF_CMDSTAT_MASTER,
			      0U);
		ctrl_data_b.fail_write_at = 0U;
		zassert_ok(pcie_endpoint_msi_enable(&ep_b, &vector, 1U, 0U));
		zassert_true((ctrl_data_b.config[PCIE_CONF_CMDSTAT] &
			      PCIE_CONF_CMDSTAT_MASTER) != 0U);
		irq_disable(TEST_IRQ);
	}
}

ZTEST(pcie_endpoint, test_msi_capacity_and_table_bounds)
{
	msi_vector_t vectors[5];
	uint8_t count;

	zassert_equal(pcie_endpoint_msi_vectors_allocate(&ep_b, 0U, vectors, 5U, &count), -ENOSPC);
	zassert_equal(ctrl_data_b.setups, 0U);
	ctrl_data_b.config[TEST_CAP + PCIE_MSIX_TR] = sizeof(ctrl_data_b.table) - 8U;
	zassert_equal(pcie_endpoint_msi_vectors_allocate(&ep_b, 0U, vectors, 1U, &count), -EINVAL);
	zassert_equal(count, 0U);
	zassert_equal(ctrl_data_b.setups, 0U);
}

ZTEST(pcie_endpoint, test_msi_64bit_and_32bit_targets)
{
	msi_vector_t vector;
	uint8_t count;

	ctrl_data_b.config[PCIE_CONF_CAPPTR] = TEST_MSI_CAP * 4U;
	ctrl_data_b.config[TEST_MSI_CAP] = PCI_CAP_ID_MSI | PCIE_MSI_MCR_64;
	zassert_ok(pcie_endpoint_msi_vectors_allocate(&ep_b, 0U, &vector, 1U, &count));
	zassert_ok(pcie_endpoint_msi_enable(&ep_b, &vector, 1U, 0U));
	zassert_equal(ctrl_data_b.config[TEST_MSI_CAP + PCIE_MSI_MAP0], (uint32_t)TEST_TARGET);
	zassert_equal(ctrl_data_b.config[TEST_MSI_CAP + PCIE_MSI_MAP1_64],
		      (uint32_t)(TEST_TARGET >> 32));
	ctrl_data_b.config[TEST_MSI_CAP] = PCI_CAP_ID_MSI;
	ctrl_data_b.writes = 0U;
	zassert_equal(pcie_endpoint_msi_enable(&ep_b, &vector, 1U, 0U), -ENOTSUP);
	zassert_equal(ctrl_data_b.writes, 0U);
	zassert_equal(ctrl_data_b.irqs, 1U);
	vector.arch.address = 0xfee00000U;
	zassert_ok(pcie_endpoint_msi_enable(&ep_b, &vector, 1U, 0U));
}

ZTEST(pcie_endpoint, test_msix_duplicate_allocation_is_rejected)
{
	msi_vector_t first, second, saved;
	uint8_t count;

	zassert_ok(pcie_endpoint_msi_vectors_allocate(&ep_b, 0U, &first, 1U, &count));
	memcpy(&saved, &first, sizeof(saved));
	zassert_equal(pcie_endpoint_msi_vectors_allocate(&ep_b, 0U, &second, 1U, &count),
		      -EALREADY);
	zassert_equal(count, 0U);
	zassert_equal(ctrl_data_b.irqs, 1U);
	zassert_equal(ctrl_data_b.mappings, 1U);
	if (IS_ENABLED(CONFIG_MMU)) {
		zassert_equal(ctrl_data_b.table_maps, 2U);
		zassert_equal(ctrl_data_b.table_unmaps, 1U);
		zassert_equal_ptr(first.msix_vector, ctrl_data_b.table_virt[0]);
	} else {
		zassert_equal_ptr(first.msix_vector, ctrl_data_b.table);
	}
	zassert_is_null(second.ctrl);
	zassert_mem_equal(&first, &saved, sizeof(first));
	zassert_ok(pcie_endpoint_msi_enable(&ep_b, &first, 1U, 0U));
}

ZTEST(pcie_endpoint, test_msi_multi_vector_layout)
{
	msi_vector_t vectors[4];
	uint8_t count;

	ctrl_data_b.config[PCIE_CONF_CAPPTR] = TEST_MSI_CAP * 4U;
	ctrl_data_b.config[TEST_MSI_CAP] = PCI_CAP_ID_MSI | PCIE_MSI_MCR_64 |
		(2U << PCIE_MSI_MCR_MMC_SHIFT);
	zassert_ok(pcie_endpoint_msi_vectors_allocate(&ep_b, 0U, vectors, 4U, &count));
	zassert_ok(pcie_endpoint_msi_enable(&ep_b, vectors, count, 0U));
	zassert_equal(ctrl_data_b.config[TEST_MSI_CAP] & PCIE_MSI_MCR_MME,
		      2U << PCIE_MSI_MCR_MME_SHIFT);
	for (uint8_t invalid = 0U; invalid < 3U; invalid++) {
		for (uint8_t i = 0U; i < count; i++) {
			vectors[i].arch.eventid = i;
			vectors[i].arch.address = TEST_TARGET;
		}
		if (invalid == 0U) {
			vectors[0].arch.eventid = 2U;
		} else if (invalid == 1U) {
			vectors[1].arch.eventid = 5U;
		} else {
			vectors[2].arch.address += 4U;
		}
		ctrl_data_b.writes = 0U;
		zassert_equal(pcie_endpoint_msi_enable(&ep_b, vectors, count, 0U), -EINVAL);
		zassert_equal(ctrl_data_b.writes, 0U);
	}
}

void ztest_post_fatal_error_hook(unsigned int reason, const struct arch_esf *esf)
{
	ARG_UNUSED(esf);
	zassert_equal(reason, K_ERR_KERNEL_PANIC);
}

ZTEST(pcie_endpoint, test_partial_setup_success_is_fatal)
{
	msi_vector_t vectors[2];
	uint8_t count;

	ctrl_data_b.bad_setup_count = 1U;
	ztest_set_fault_valid(true);
	pcie_endpoint_msi_vectors_allocate(&ep_b, 0U, vectors, 2U, &count);
	ztest_set_fault_valid(false);
	zassert_unreachable("partial success must not appear to be a clean allocation failure");
}

ZTEST(pcie_endpoint, test_setup_error_with_allocations_is_fatal)
{
	msi_vector_t vectors[2];
	uint8_t count;

	ctrl_data_b.bad_setup_count = 2U;
	ztest_set_fault_valid(true);
	pcie_endpoint_msi_vectors_allocate(&ep_b, 0U, vectors, 2U, &count);
	ztest_set_fault_valid(false);
	zassert_unreachable("an error with remaining allocations must not discard ownership");
}

static void *suite_setup(void)
{
	zassert_true(early_found[0]);
	zassert_equal(early_found[1], !IS_ENABLED(PCIE_TEST_FAIL_CTRL_B));
	return NULL;
}

ZTEST_SUITE(pcie_binding, NULL, suite_setup, NULL, NULL, NULL);
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
