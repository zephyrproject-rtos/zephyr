/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/cpu_freq/cpu_freq.h>
#include <zephyr/cpu_freq/pstate.h>
#include <zephyr/drivers/firmware/scmi/perf.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <errno.h>

LOG_MODULE_REGISTER(scmi_cpu_freq, CONFIG_CPU_FREQ_LOG_LEVEL);

/**
 * @brief Vendor config for an arm,scmi-perf-pstate DTS node.
 *
 * Populated at compile time from DTS properties.  The cpu_freq core
 * stores a pointer to this struct in struct pstate::config.
 */
struct scmi_perf_pstate_cfg {
	/** SCMI performance domain index. */
	uint32_t domain_id;
	/** Target frequency in kHz. */
	uint32_t perf_level_khz;
};

#define SCMI_PERF_PSTATE_CFG_DEFINE(node_id)					\
	static const struct scmi_perf_pstate_cfg				\
		_CONCAT(scmi_perf_pstate_cfg_, DT_DEP_ORD(node_id)) = {	\
			.domain_id      = DT_PROP(node_id, scmi_perf_domain),	\
			.perf_level_khz = DT_PROP(node_id, scmi_perf_level_khz),\
		};

DT_FOREACH_CHILD_STATUS_OKAY(DT_PATH(performance_states),
			     SCMI_PERF_PSTATE_CFG_DEFINE)

#define SCMI_PERF_PSTATE_DEFINE(node_id)					\
	PSTATE_DT_DEFINE(node_id,						\
		&_CONCAT(scmi_perf_pstate_cfg_, DT_DEP_ORD(node_id)));

DT_FOREACH_CHILD_STATUS_OKAY(DT_PATH(performance_states),
			     SCMI_PERF_PSTATE_DEFINE)

int cpu_freq_pstate_set(const struct pstate *state)
{
	const struct scmi_perf_pstate_cfg *cfg;
	int ret;

	if (state == NULL) {
		LOG_ERR("P-state is NULL");
		return -EINVAL;
	}

	cfg = (const struct scmi_perf_pstate_cfg *)state->config;

	if (cfg == NULL) {
		LOG_ERR("P-state vendor config is NULL");
		return -EINVAL;
	}

	LOG_DBG("SCMI cpu_freq: domain=%u level=%u kHz (load_threshold=%u%%)",
		cfg->domain_id, cfg->perf_level_khz, state->load_threshold);

	ret = scmi_perf_level_set(cfg->domain_id, cfg->perf_level_khz);
	if (ret < 0) {
		LOG_ERR("SCMI PERFORMANCE_LEVEL_SET domain=%u level=%u kHz failed: %d",
			cfg->domain_id, cfg->perf_level_khz, ret);
		return ret;
	}

	/*
	 * Update the system clock frequency so that SysTick and other
	 * cycle-based timers remain accurate after the frequency change.
	 *
	 * Requires CONFIG_SYSTEM_CLOCK_HW_CYCLES_PER_SEC_RUNTIME_UPDATE=y
	 * when CONFIG_CORTEX_M_SYSTICK is enabled.
	 */
#if defined(CONFIG_CORTEX_M_SYSTICK)
#if !defined(CONFIG_SYSTEM_CLOCK_HW_CYCLES_PER_SEC_RUNTIME_UPDATE)
#error "CPU_FREQ_SCMI with SysTick requires " \
	"CONFIG_SYSTEM_CLOCK_HW_CYCLES_PER_SEC_RUNTIME_UPDATE=y"
#endif
	z_sys_clock_hw_cycles_per_sec_update(cfg->perf_level_khz * 1000U);
#endif
	return 0;
}
