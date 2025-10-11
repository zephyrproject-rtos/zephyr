/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_nrf_vpr_coprocessor

#include <string.h>

#include <zephyr/cache.h>
#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>

#include <hal/nrf_vpr.h>
#if (DT_ANY_INST_HAS_BOOL_STATUS_OKAY(enable_secure) || \
	 DT_ANY_INST_HAS_BOOL_STATUS_OKAY(enable_dma_secure)) && \
	!defined(CONFIG_TRUSTED_EXECUTION_NONSECURE)
#include <hal/nrf_spu.h>
#endif

LOG_MODULE_REGISTER(nordic_vpr_launcher, CONFIG_NORDIC_VPR_LAUNCHER_LOG_LEVEL);

struct nordic_vpr_launcher_config {
	NRF_VPR_Type *vpr;
	uintptr_t exec_addr;
	bool enable_secure;
	bool enable_dma_secure;
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(source_memory)
	uintptr_t src_addr;
	size_t size;
#endif
};

static int nordic_vpr_launcher_init(const struct device *dev)
{
	const struct nordic_vpr_launcher_config *config = dev->config;

	/* Do nothing if execution memory is not specified for a given VPR. */
	if (config->exec_addr == 0) {
		return 0;
	}

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(source_memory)
	if (config->size > 0U) {
		LOG_DBG("Loading VPR (%p) from %p to %p (%zu bytes)", config->vpr,
			(void *)config->src_addr, (void *)config->exec_addr, config->size);
		memcpy((void *)config->exec_addr, (void *)config->src_addr, config->size);
#if defined(CONFIG_DCACHE)
		LOG_DBG("Writing back cache with loaded VPR (from %p %zu bytes)",
			(void *)config->exec_addr, config->size);
		sys_cache_data_flush_range((void *)config->exec_addr, config->size);
#endif
	}
#endif

#if !defined(CONFIG_TRUSTED_EXECUTION_NONSECURE)
#if DT_ANY_INST_HAS_BOOL_STATUS_OKAY(enable_secure)
	if (config->enable_secure) {
		nrf_spu_periph_perm_secattr_set(NRF_SPU00,
						nrf_address_slave_get((uint32_t)config->vpr),
						true);
	}
#endif
#if DT_ANY_INST_HAS_BOOL_STATUS_OKAY(enable_dma_secure)
	if (config->enable_dma_secure) {
		nrf_spu_periph_perm_dmasec_set(NRF_SPU00,
						nrf_address_slave_get((uint32_t)config->vpr),
						true);
	}
#endif
#endif
	LOG_DBG("Launching VPR (%p) from %p", config->vpr, (void *)config->exec_addr);
	nrf_vpr_initpc_set(config->vpr, config->exec_addr);
	nrf_vpr_cpurun_set(config->vpr, true);

	return 0;
}

/* obtain VPR address either from memory or partition */
#define VPR_ADDR(node_id)                                                                          \
	(DT_REG_ADDR(node_id) +                                                                    \
	 COND_CODE_0(DT_FIXED_PARTITION_EXISTS(node_id), (0), (DT_REG_ADDR(DT_GPARENT(node_id)))))

#define NEEDS_COPYING(inst) UTIL_AND(DT_INST_NODE_HAS_PROP(inst, execution_memory),                \
				     DT_INST_NODE_HAS_PROP(inst, source_memory))

#define NORDIC_VPR_LAUNCHER_DEFINE(inst)                                                           \
	IF_ENABLED(NEEDS_COPYING(inst),                                                            \
		   (BUILD_ASSERT((DT_REG_SIZE(DT_INST_PHANDLE(inst, execution_memory)) <=          \
				  DT_REG_SIZE(DT_INST_PHANDLE(inst, source_memory))),              \
				 "Execution memory exceeds source memory size");))                 \
                                                                                                   \
	static const struct nordic_vpr_launcher_config config##inst = {                            \
		.vpr = (NRF_VPR_Type *)DT_INST_REG_ADDR(inst),                                     \
		IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, execution_memory),                          \
			   (.exec_addr = VPR_ADDR(DT_INST_PHANDLE(inst, execution_memory)),))      \
		.enable_secure = DT_INST_PROP(inst, enable_secure),                                \
		.enable_dma_secure = DT_INST_PROP(inst, enable_dma_secure),                        \
		IF_ENABLED(NEEDS_COPYING(inst),                                                    \
			   (.src_addr = VPR_ADDR(DT_INST_PHANDLE(inst, source_memory)),            \
			    .size = DT_REG_SIZE(DT_INST_PHANDLE(inst, execution_memory)),))};      \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, nordic_vpr_launcher_init, NULL, NULL, &config##inst,           \
			      POST_KERNEL, CONFIG_NORDIC_VPR_LAUNCHER_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(NORDIC_VPR_LAUNCHER_DEFINE)
