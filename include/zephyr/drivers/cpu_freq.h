/*
 * Copyright (c) 2025 Intel Corp.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CPU_FREQ_H
#define ZEPHYR_INCLUDE_DRIVERS_CPU_FREQ_H

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/util_macro.h>


#define CPU_FREQ_NODE DT_NODELABEL(cpu_freq)

/* Identify the number of structured paired entries */
#define CPU_FREQ_CFG_NUM_ENTRIES (DT_PROP_LEN(CPU_FREQ_NODE, cfgdata) / 2)

/* Identify the raw number of data elements */
#define CPU_FREQ_CFG_NUM_RAW_ELEMENTS    DT_PROP_LEN(CPU_FREQ_NODE, cfgdata)

#define CPU_FREQ_CFG_ENTRY(idx, node_id)      \
	{                                     \
	DT_PROP_BY_IDX(node_id, cfgdata, UTIL_X2(idx)),  \
	DT_PROP_BY_IDX(node_id, cfgdata, UTIL_INC(UTIL_X2(idx))) \
	}

#define CPU_FREQ_CFG_RAW_DATA(idx, node_id)      \
	DT_PROP_BY_IDX(node_id, cfgdata, idx)

struct cpu_freq_cfg {
	unsigned long long threshold;
	unsigned long long frequency;
};

__subsystem struct cpu_freq_driver_api {
	int (*get_cfg_id)(const struct device *dev, unsigned int cpu_id,
			  unsigned int *cfg_id);
	int (*set_by_cfg_id)(const struct device *dev, unsigned int cpu_bitmask,
			 unsigned int id);
	int (*set_by_load)(const struct device *dev, unsigned int cpu_bitmask,
			   unsigned int load);
	int (*get_cfg)(const struct device *dev, unsigned int cfg_id,
		       struct cpu_freq_cfg *cfg);
};

/**
 * Get the current CPU frequency configuration ID in use by the specified CPU
 *
 * @param cpu_id CPU to query
 * @param cfg_id Pointer to where to save CPU frequency configuration ID
 *
 * @retval 0 on success, -EINVAL or -ENOSYS on error
 */
static inline int cpu_freq_get_cfg_id(const struct device *dev,
				      unsigned int cpu_id, unsigned int *cfg_id)
{
	const struct cpu_freq_driver_api *api =
		(const struct cpu_freq_driver_api *)dev->api;

	if ((cpu_id >= CONFIG_MP_MAX_NUM_CPUS) || (cfg_id == NULL)) {
		return -EINVAL;
	}

	if (api->get_cfg_id == NULL) {
		return -ENOSYS;
	}

	return api->get_cfg_id(dev, cpu_id, cfg_id);
}

/**
 * Set the frequency of the specified CPUs based on the configuration ID
 *
 * @param cpu_bitmask Bitmask of relevant CPUs
 * @param cfg_id Configuration ID that contains the frequency to use
 *
 * @retval 0 on success, -EINVAL or -ENOSYS on error
 */
static inline int cpu_freq_set_by_cfg_id(const struct device *dev,
					 unsigned int cpu_bitmask,
					 unsigned int cfg_id)
{
	const struct cpu_freq_driver_api *api =
		(const struct cpu_freq_driver_api *)dev->api;

	if ((cpu_bitmask > BIT(CONFIG_MP_MAX_NUM_CPUS) - 1) ||
	    (cpu_bitmask == 0) || (cfg_id >= CPU_FREQ_CFG_NUM_ENTRIES)) {
		return -EINVAL;
	}

	if (api->set_by_cfg_id == NULL) {
		return -ENOSYS;
	}

	return api->set_by_cfg_id(dev, cpu_bitmask, cfg_id);
}

/**
 * Set the frequency of the specified CPUs based on specified load
 *
 * @param cpu_bitmask Bitmask of relevant CPUs
 * @param load Load percentage (0-100) to determine frequency
 *
 * @retval 0 on success, -EINVAL or -ENOSYS on error
 */
static inline int cpu_freq_set_by_load(const struct device *dev,
				       unsigned int cpu_bitmask,
				       unsigned int load)
{
	const struct cpu_freq_driver_api *api =
		(const struct cpu_freq_driver_api *)dev->api;

	if ((cpu_bitmask > BIT(CONFIG_MP_MAX_NUM_CPUS) - 1) ||
	    (cpu_bitmask == 0) || (load > 100)) {
		return -EINVAL;
	}

	if (api->set_by_load == NULL) {
		return -ENOSYS;
	}

	return api->set_by_load(dev, cpu_bitmask, load);
}

/**
 * @brief Given an ID, obtain the associated configuration data
 *
 * @param cfg_id Configuration ID to query
 * @param freq Pointer to where save the frequency identified by @a freq_id
 *
 * @retval 0 on success, -EINVAL on error
 */
static inline int cpu_freq_get_cfg(const struct device *dev,
				   unsigned int cfg_id,
				   struct cpu_freq_cfg *cfg)
{
	const struct cpu_freq_driver_api *api =
		(const struct cpu_freq_driver_api *)dev->api;

	if ((cfg_id >= CPU_FREQ_CFG_NUM_ENTRIES) || (cfg == NULL)) {
		return -EINVAL;
	}

	if (api->get_cfg == NULL) {
		return -ENOSYS;
	}

	return api->get_cfg(dev, cfg_id, cfg);
}

/**
 * @brief Return the number of CPU frequency configurations available
 *
 * @retval Number of CPU frequency configurations
 */
static inline unsigned int cpu_freq_num_cfgs(void)
{
	return CPU_FREQ_CFG_NUM_ENTRIES;
}
#endif /* ZEPHYR_INCLUDE_DRIVERS_CPU_FREQ_H */
