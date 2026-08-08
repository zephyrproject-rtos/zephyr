/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-FileCopyrightText: Copyright (c) 2026 Siemens Mobility GmbH
 * SPDX-License-Identifier: Apache-2.0
 */

#include "zephyr/dt-bindings/amp/ti-k3-amp.h"
#define DT_DRV_COMPAT ti_amp_k3

#include <zephyr/drivers/amp.h>
#include <zephyr/drivers/amp/amp_ti_k3.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/drivers/firmware/tisci/tisci.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(amp_ti_k3, LOG_LEVEL_INF);

// TODO: Get from DT or Kconfig
#define NUM_CORTEX_R_CLUSTERS 2
#define NUM_CORTEX_M_CLUSTERS 1

#define NUM_CORTEX_R_CORES_PER_CLUSTER 2
#define NUM_CORTEX_M_CORES_PER_CLUSTER 1

// TODO: Size is depending on the core config!
#define CORTEX_R_ATCM_START_ADDRESS 0x0
#define CORTEX_R_ATCM_SIZE          KB(32)
#define CORTEX_R_BTCM_START_ADDRESS 0x41010000
#define CORTEX_R_BTCM_SIZE          KB(32)

// TODO: This is mostly belonging in the config and *not* the data
struct amp_ti_k3_generic_core_config {
	const uint8_t tisci_core_id;
	const struct device *power_domain;
	const bool is_allowed_to_change; // status = "okay" check; not used *yet*
};

struct amp_ti_k3_cortex_r_core {
	struct amp_ti_k3_generic_core_config generic;
	// TODO: Indicate, if core got flags from devicetree
	struct amp_ti_k3_boot_options dt_core_options;
	const uintptr_t atcm_external_address;
	const uintptr_t btcm_external_address;
};

struct amp_ti_k3_cortex_r_cluster {
	struct amp_ti_k3_cortex_r_core cores[NUM_CORTEX_R_CORES_PER_CLUSTER];
};

// NOTE: The Cortex-M isn't implemented yet
struct amp_ti_k3_cortex_m_core {
	struct amp_ti_k3_generic_core_config generic;
	const uintptr_t iram_external_address; /* This is 0x00000..0x30000 (internal) @ 0x5000000
						  (external) */
	const uintptr_t dram_external_address; /* This is 0x30000..0x40000 (internal) @ 0x5040000
						  (external) */
};

struct amp_ti_k3_cortex_m_cluster {
	struct amp_ti_k3_cortex_m_core cores[NUM_CORTEX_M_CORES_PER_CLUSTER];
};

struct amp_ti_k3_config {
	DEVICE_MMIO_ROM;
	const struct device *dmsc;
};

struct amp_ti_k3_data {
	DEVICE_MMIO_RAM;
	struct amp_ti_k3_cortex_r_cluster cortex_r_clusters[NUM_CORTEX_R_CLUSTERS];
	struct amp_ti_k3_cortex_m_cluster cortex_m_clusters[NUM_CORTEX_M_CLUSTERS];
};

static int amp_ti_k3_init(const struct device *dev)
{
	// TODO: Get state of all cores (unsure for now)

	// Power Management + Reset state (Maybe also not?)

	return 0;
}

void *amp_ti_k3_get_core_data(const struct device *root_device,
			      const struct amp_core_identification *core_identification)
{
	struct amp_ti_k3_data *data = root_device->data;
	switch (core_identification->cluster_type) {
	case AMP_TI_K3_CLUSTER_TYPE_CORTEX_R:
		if (core_identification->cluster_id >= NUM_CORTEX_R_CLUSTERS || core_identification->core_id >= NUM_CORTEX_R_CORES_PER_CLUSTER) {
			return NULL;;
		}
		return &data->cortex_r_clusters[core_identification->cluster_id]
				.cores[core_identification->core_id];
	case AMP_TI_K3_CLUSTER_TYPE_CORTEX_M:
		if (core_identification->cluster_id >= NUM_CORTEX_M_CLUSTERS || core_identification->core_id >= NUM_CORTEX_M_CORES_PER_CLUSTER) {
			return NULL;;
		}
		return &data->cortex_m_clusters[core_identification->cluster_id]
				.cores[core_identification->core_id];
	}
	__ASSERT(false, "Asked for a Cluster type that doesn't exist");
	return NULL;
}

static int amp_ti_k3_prepare_core(const struct device *root_device,
				  const struct amp_core_identification *core_identification,
				  const void *core_options)
{
	const struct amp_ti_k3_config *cfg = root_device->config;
	uint8_t tisci_core_id = 0;
	int ret;
	if (core_identification->cluster_type == AMP_TI_K3_CLUSTER_TYPE_CORTEX_R) {
		struct amp_ti_k3_cortex_r_core *core =
			amp_ti_k3_get_core_data(root_device, core_identification);
		tisci_core_id = core->generic.tisci_core_id;

		ret = tisci_cmd_proc_request(cfg->dmsc, tisci_core_id);
		if (ret < 0) {
			LOG_ERR("Error while requesting processor control: %d", ret);
			goto err;
		}

		const struct amp_ti_k3_boot_options *boot_options = core_options;
		uint32_t set_xor_clear = boot_options->config_flags_set ^ boot_options->config_flags_clear;
		if (set_xor_clear != CORTEX_R_ALL_BITS) {
			uint32_t bitdiff = set_xor_clear ^ CORTEX_R_ALL_BITS;
			LOG_ERR("Cortex-R had some config options specified neither in set or clear. Missing bits: 0x%08llX", (unsigned long long) bitdiff);
			ret = -EINVAL;
			goto err;
		}

		ret = tisci_cmd_set_proc_boot_cfg(cfg->dmsc, tisci_core_id, boot_options->boot_vec,
						  boot_options->config_flags_set,
						  boot_options->config_flags_clear);
		if (ret < 0) {
			LOG_ERR("Error setting processor boot config: %d", ret);
			goto err;
		}

		/* Assert reset */
		ret = tisci_cmd_set_proc_boot_ctrl(cfg->dmsc, tisci_core_id, BIT(0), 0);
		if (ret < 0) {
			LOG_ERR("Error asserting reset signal on processor: %d", ret);
			goto err;
		}

		ret = pm_device_runtime_get(core->generic.power_domain);
		if (ret < 0) {
			LOG_ERR("Error enabling core power domain: %d", ret);
			goto err;
		}
	} else {
		// TODO: Cortex-M and Cortex-A
	}

	return 0;

err:
	if (tisci_cmd_proc_release(cfg->dmsc, tisci_core_id) < 0) {
		LOG_ERR("Error releasing processor after previous error: %d", ret);
	}
	return ret;
}

static int amp_ti_k3_start_core(const struct device *root_device,
				const struct amp_core_identification *core_identification)
{
	const struct amp_ti_k3_config *cfg = root_device->config;
	uint8_t tisci_core_id = 0;
	int ret = 0;
	if (core_identification->cluster_type == AMP_TI_K3_CLUSTER_TYPE_CORTEX_R) {
		struct amp_ti_k3_cortex_r_core *core =
			amp_ti_k3_get_core_data(root_device, core_identification);
		tisci_core_id = core->generic.tisci_core_id;

		// Deassert reset
		ret = tisci_cmd_set_proc_boot_ctrl(cfg->dmsc, tisci_core_id, 0, BIT(0));
		if (ret < 0) {
			LOG_ERR("Error deasserting reset signal: %d", ret);
			goto exit;
		}
	} else {
		// TODO: Cortex-M
	}

exit:
	if (tisci_cmd_proc_release(cfg->dmsc, tisci_core_id) < 0) {
		LOG_ERR("Error releasing processor control: %d", ret);
	}
	return ret;
}

static int amp_ti_k3_stop_core(const struct device *root_device,
			       const struct amp_core_identification *core_identification)
{
	int ret = 0;
	if (core_identification->cluster_type == AMP_TI_K3_CLUSTER_TYPE_CORTEX_R) {
		struct amp_ti_k3_cortex_r_core *core =
			amp_ti_k3_get_core_data(root_device, core_identification);

		// TODO: Track if the core is actually having a clock signal to avoid
		// removing the clock signal twice
		ret = pm_device_runtime_put(core->generic.power_domain);
		if (ret < 0) {
			LOG_ERR("Error removing power from processor core: %d", ret);
		}
	}
	return ret;
}

static void *amp_ti_k3_get_dt_core_config(const struct device *root_device,
					  const struct amp_core_identification *core_identification)
{
	/* This cast works since the generic options are the first element */
	if (core_identification->cluster_type == AMP_TI_K3_CLUSTER_TYPE_CORTEX_R) {
		struct amp_ti_k3_cortex_r_core *core =
			amp_ti_k3_get_core_data(root_device, core_identification);
		return &core->dt_core_options;
	}
	return NULL;
}

static int amp_ti_k3_get_virtual_address(const struct device *root_device,
					 const struct amp_core_identification *core_identification,
					 struct amp_memory_mapping *mapping)
{
	// TODO: xTCM size check based on dualcore check
	// TODO: Length checks
	// TODO: Get whether ATCM or BTCM is considered at 0x0 via devicetree option
	// or previous core startup
	if (core_identification->cluster_type == AMP_TI_K3_CLUSTER_TYPE_CORTEX_R) {
		struct amp_ti_k3_cortex_r_core *core =
			amp_ti_k3_get_core_data(root_device, core_identification);
		if (IN_RANGE(mapping->target_device_address, CORTEX_R_ATCM_START_ADDRESS,
			     CORTEX_R_ATCM_START_ADDRESS + CORTEX_R_ATCM_SIZE)) {
			mapping->own_virtual_address_start = core->atcm_external_address;
			mapping->mapped_region_size = CORTEX_R_ATCM_SIZE;
			mapping->target_device_area_start = CORTEX_R_ATCM_START_ADDRESS;
			return 0;
		}

		if (IN_RANGE(mapping->target_device_address, CORTEX_R_BTCM_START_ADDRESS,
			     CORTEX_R_BTCM_START_ADDRESS + CORTEX_R_BTCM_SIZE)) {
			mapping->own_virtual_address_start = core->btcm_external_address;
			mapping->mapped_region_size = CORTEX_R_BTCM_SIZE;
			mapping->target_device_area_start = CORTEX_R_BTCM_START_ADDRESS;
			return 0;
		}
		// TODO: Get from DT; this should be a generic node
		if (IN_RANGE(mapping->target_device_address, 0x70000000, 0x70000000 + MB(2))) {
			mapping->own_virtual_address_start = 0x70000000;
			mapping->target_device_area_start = 0x70000000;
			mapping->mapped_region_size = MB(2);
		}
		return 0;
	}

	LOG_ERR("Tried to request invalid mapping for address %" PRIuPTR " with size %zx",
		mapping->target_device_address, mapping->target_area_size);

	return -EINVAL;
}

static DEVICE_API(amp, amp_ti_k3_driver_api) = {
	.amp_prepare_core = amp_ti_k3_prepare_core,
	.amp_start_core = amp_ti_k3_start_core,
	.amp_stop_core = amp_ti_k3_stop_core,
	.amp_get_dt_core_config = amp_ti_k3_get_dt_core_config,
	.amp_get_virtual_address = amp_ti_k3_get_virtual_address,
};

/* Generic field that every core type has; node_id = core */
#define AMP_TI_K3_GENERIC_CORE_OPTIONS(node_id)                                                    \
	{                                                                                          \
		.is_allowed_to_change = DT_NODE_HAS_STATUS_OKAY(node_id),                          \
		.power_domain = DEVICE_DT_GET(DT_PHANDLE(node_id, power_domains)),                 \
		.tisci_core_id = DT_PROP(node_id, ti_tisci_core_id),                               \
	}

/* Single Cortex-R core; node_id = core */
/* No trailing comma at the end due to the necesssity of DT_FOREACH_CHILD_SEP mixing! */
#define AMP_TI_K3_CORTEX_R_CORE_OPTIONS(node_id)                                                   \
	[DT_REG_ADDR(node_id)] = {                                                                 \
		.atcm_external_address = DT_PROP(node_id, atcm_address),                           \
		.btcm_external_address = DT_PROP(node_id, btcm_address),                           \
		.generic = AMP_TI_K3_GENERIC_CORE_OPTIONS(node_id),                                \
		.dt_core_options = {                                                               \
			.config_flags_set = DT_PROP_OR(node_id, config_set_flags, 0),              \
			.config_flags_clear = DT_PROP_OR(node_id, config_clear_flags, 0),          \
			.boot_vec = DT_PROP_OR(node_id, boot_vec, 0),                       \
		},                                                                                 \
	}

/* Full Cortex-R cluster; node_id = cluster */
#define AMP_TI_K3_CORTEX_R_SINGLE_CLUSTER(node_id)                                                 \
	[DT_REG_ADDR(node_id)] = {                                                                 \
		.cores = {DT_FOREACH_CHILD_SEP(node_id, AMP_TI_K3_CORTEX_R_CORE_OPTIONS, (, ))}},

/*
 * It's necessary to use DT_FOREACH_CHILD_SEP and DT_FOREACH_CHILD (without _SEP)
 * since for some reason using the same inside themself leads to compiler errors
 */

/* All Cortex-R clusters; node_id = cortex_r_clusters */
#define AMP_TI_K3_CORTEX_R_CLUSTERS(node_id)                                                       \
	{DT_FOREACH_CHILD(node_id, AMP_TI_K3_CORTEX_R_SINGLE_CLUSTER)}

#define AMP_TI_K3_INIT(n)                                                                          \
	static const struct amp_ti_k3_config amp_ti_k3_##n##_config = {                            \
		.dmsc = DEVICE_DT_GET(DT_PHANDLE(DT_DRV_INST(n), dmsc)),                           \
	};                                                                                         \
                                                                                                   \
	static struct amp_ti_k3_data amp_ti_k3_##n##_data = {                                      \
		.cortex_r_clusters =                                                               \
			AMP_TI_K3_CORTEX_R_CLUSTERS(DT_CHILD(DT_DRV_INST(n), r5f_clusters_0)),     \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, amp_ti_k3_init, NULL, &amp_ti_k3_##n##_data,                      \
			      &amp_ti_k3_##n##_config, PRE_KERNEL_2, CONFIG_AMP_INIT_PRIORITY,     \
			      &amp_ti_k3_driver_api);

DT_INST_FOREACH_STATUS_OKAY(AMP_TI_K3_INIT)
