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
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(hibernation_ram_block)
#include <hal/nrf_memconf.h>
#endif
#if defined(CONFIG_NORDIC_VPR_LAUNCHER_DECOMPRESS)
#include <lz4.h>
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
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(hibernation_ram_block)
	int32_t ram_block_id;
#endif
};

/* Determine if there is a single VPR executing from RRAM (xip). */
#if DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1 && \
	DT_SAME_NODE(DT_PARENT(DT_INST_PHANDLE(0, execution_memory)), DT_NODELABEL(rram_controller))
#define VPR_EXEC_XIP 1
#endif

/* Determine if all code that will be copied is 8 byte aligned. If yes then optimized
 * copying can be used. Macro ends with && so it must be followed by an additional
 * condition.
 */
#define ALIGNED_SIZE_AND_CODE(inst) \
	IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, execution_memory), (                                \
		((DT_REG_SIZE(DT_INST_PHANDLE(inst, execution_memory)) & BIT_MASK(3)) == 0) &&     \
		((DT_REG_ADDR(DT_INST_PHANDLE(inst, execution_memory)) & BIT_MASK(3)) == 0) &&     \
		((DT_REG_ADDR(DT_INST_PHANDLE(inst, source_memory)) & BIT_MASK(3)) == 0) &&))

static int nordic_vpr_launcher_init(const struct device *dev)
{
	const struct nordic_vpr_launcher_config *config = dev->config;

	/* Do nothing if execution memory is not specified for a given VPR. */
	if (config->exec_addr == 0) {
		return 0;
	}

#if defined(CONFIG_NORDIC_VPR_LAUNCHER_FROM_ARRAY) && !defined(VPR_EXEC_XIP)
	static const uint8_t vpr_image[] __aligned(sizeof(uint32_t)) = {
		#include <bin/image.inc>
	};
#if defined(CONFIG_NORDIC_VPR_LAUNCHER_DECOMPRESS)
	void *dst = (void *)config->exec_addr;
	const size_t compressed_size = sizeof(vpr_image);
	const size_t exec_size = DT_REG_SIZE(DT_INST_PHANDLE(0, execution_memory));
	int decompressed_size;

	BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1,
		     "Decompression requires a single VPR instance");

	LOG_INF("Decompressing VPR image (%zu bytes) to %p (%zu bytes max)",
		compressed_size, dst, exec_size);

	decompressed_size = LZ4_decompress_safe((const char *)vpr_image, (char *)dst,
						(int)compressed_size, (int)exec_size);
	if (decompressed_size < 0) {
		LOG_ERR("LZ4 decompression failed: %d", decompressed_size);
		return -EIO;
	}

	LOG_INF("Decompressed VPR image to %zu bytes", (size_t)decompressed_size);
#else
	uint64_t *dst = (uint64_t *)config->exec_addr;
	uint64_t *src = (uint64_t *)vpr_image;
	size_t total_size = sizeof(vpr_image);
	size_t len64 = total_size / sizeof(uint64_t);

	/* Load VPR code from array. */
	LOG_INF("Loading VPR code from array (%p) to %p (%zu bytes)", src, dst, total_size);
	for (size_t i = 0; i < len64; i++) {
		dst[i] = (uint64_t)src[i];
	}

	total_size = total_size - len64 * sizeof(uint64_t);
	if (total_size > 0) {
		uint8_t *dst8 = (uint8_t *)(dst + len64 * sizeof(uint64_t));
		uint8_t *src8 = (uint8_t *)(src + len64);

		memcpy(dst8, src8, total_size);
	}
#endif
#elif DT_ANY_INST_HAS_PROP_STATUS_OKAY(source_memory)
	if (config->size > 0U) {
		LOG_DBG("Loading VPR (%p) from %p to %p (%zu bytes)", config->vpr,
			(void *)config->src_addr, (void *)config->exec_addr, config->size);
#if (DT_INST_FOREACH_STATUS_OKAY(ALIGNED_SIZE_AND_CODE) !defined(CONFIG_SPEED_OPTIMIZATIONS))
		uint64_t *src = (uint64_t *)config->src_addr;
		uint64_t *dst = (uint64_t *)config->exec_addr;

		for (size_t i = 0; i < config->size / sizeof(uint64_t); i++) {
			dst[i] = src[i];
		}
#else
		memcpy((void *)config->exec_addr, (void *)config->src_addr, config->size);
#endif
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

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(hibernation_ram_block)
	/* Enable retention of the RAM block used for VPR hibernation. */
	if (config->ram_block_id >= 0) {
		nrf_memconf_ramblock_control_mask_enable_set(NRF_MEMCONF, config->ram_block_id / 32,
							 BIT(config->ram_block_id % 32), true);
		nrf_memconf_ramblock_ret_mask_enable_set(NRF_MEMCONF, config->ram_block_id / 32,
							 BIT(config->ram_block_id % 32), true);
	}
#endif

	return 0;
}

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
			   (.exec_addr = DT_REG_ADDR(DT_INST_PHANDLE(inst, execution_memory)),))   \
		.enable_secure = DT_INST_PROP(inst, enable_secure),                                \
		.enable_dma_secure = DT_INST_PROP(inst, enable_dma_secure),                        \
		IF_ENABLED(NEEDS_COPYING(inst),                                                    \
			   (.src_addr = DT_REG_ADDR(DT_INST_PHANDLE(inst, source_memory)),         \
			    .size = DT_REG_SIZE(DT_INST_PHANDLE(inst, execution_memory)),))        \
		IF_ENABLED(DT_ANY_INST_HAS_PROP_STATUS_OKAY(hibernation_ram_block),                \
			   (.ram_block_id = DT_INST_PROP_OR(inst, hibernation_ram_block, -1),))    \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(inst, nordic_vpr_launcher_init, NULL, NULL, &config##inst,           \
			      POST_KERNEL, CONFIG_NORDIC_VPR_LAUNCHER_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(NORDIC_VPR_LAUNCHER_DEFINE)
