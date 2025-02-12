/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nordic_nrf_vpr_coprocessor

#include <string.h>

#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/toolchain.h>

#include <hal/nrf_vpr.h>
#if defined(CONFIG_SOC_NRF54L15_ENGA_CPUAPP) && !defined(CONFIG_TRUSTED_EXECUTION_NONSECURE)
#include <hal/nrf_spu.h>
#endif

LOG_MODULE_REGISTER(nordic_vpr_launcher, CONFIG_NORDIC_VPR_LAUNCHER_LOG_LEVEL);

struct nordic_vpr_launcher_config {
	NRF_VPR_Type *vpr;
	uintptr_t exec_addr;
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(source_memory)
	uintptr_t src_addr;
	size_t size;
#endif
};

static int nordic_vpr_launcher_init(const struct device *dev)
{
	const struct nordic_vpr_launcher_config *config = dev->config;

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(source_memory)
	if (config->size > 0U) {
		LOG_DBG("Loading VPR (%p) from %p to %p (%zu bytes)", config->vpr,
			(void *)config->src_addr, (void *)config->exec_addr, config->size);
		memcpy((void *)config->exec_addr, (void *)config->src_addr, config->size);
	}
#endif

#if defined(CONFIG_SOC_NRF54L15_ENGA_CPUAPP) && !defined(CONFIG_TRUSTED_EXECUTION_NONSECURE)
	nrf_spu_periph_perm_secattr_set(NRF_SPU00, nrf_address_slave_get((uint32_t)config->vpr),
					true);
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

#define NORDIC_VPR_LAUNCHER_DEFINE(inst)                                                           \
	IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, source_memory),                                     \
		   (BUILD_ASSERT((DT_REG_SIZE(DT_INST_PHANDLE(inst, execution_memory)) <=          \
				  DT_REG_SIZE(DT_INST_PHANDLE(inst, source_memory))),              \
				 "Execution memory exceeds source memory size");))                 \
                                                                                                   \
	static const struct nordic_vpr_launcher_config config##inst = {                            \
		.vpr = (NRF_VPR_Type *)DT_INST_REG_ADDR(inst),                                     \
		.exec_addr = VPR_ADDR(DT_INST_PHANDLE(inst, execution_memory)),                    \
		IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, source_memory),                             \
			   (.src_addr = VPR_ADDR(DT_INST_PHANDLE(inst, source_memory)),            \
			    .size = DT_REG_SIZE(DT_INST_PHANDLE(inst, execution_memory)),))};      \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, nordic_vpr_launcher_init, NULL, NULL, &config##inst,           \
			      POST_KERNEL, CONFIG_NORDIC_VPR_LAUNCHER_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(NORDIC_VPR_LAUNCHER_DEFINE)
