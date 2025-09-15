/*
 * Copyright (c) 2025 Core Devices LLC
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef INCLUDE_ZEPHYR_DRIVERS_DMA_SF32LB_H_
#define INCLUDE_ZEPHYR_DRIVERS_DMA_SF32LB_H_

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/dma.h>
#include <zephyr/dt-bindings/dma/sf32lb-dma-config.h>
#include <zephyr/sys/util.h>

/**
 * @brief DMA (SF32LB specifics)
 * @defgroup dma_sf32lb DMA (SF32LB specifics)
 * @ingroup dma_interface
 * @{
 */

/** @brief SF32LB DMA DT spec */
struct sf32lb_dma_dt_spec {
	/** DMA controller */
	const struct device *dev;
	/** DMA channel */
	uint8_t channel;
	/** DMA request */
	uint8_t request;
	/** DMA configuration */
	uint8_t config;
};

/**
 * @brief Initialize a `sf32lb_dma_dt_spec` structure from a DT node.
 *
 * @param node_id DT node identifier
 */
#define SF32LB_DMA_DT_SPEC_GET(node_id)                                                            \
	{                                                                                          \
		.dev = DEVICE_DT_GET(DT_DMAS_CTLR(node_id)),                                       \
		.channel = DT_DMAS_CELL_BY_IDX(node_id, 0, channel),                               \
		.request = DT_DMAS_CELL_BY_IDX(node_id, 0, request),                               \
		.config = DT_DMAS_CELL_BY_IDX(node_id, 0, config),                                 \
	}

/**
 * @brief Initialize a `sf32lb_dma_dt_spec` structure from a DT node.
 *
 * @param node_id DT node identifier
 */
#define SF32LB_DMA_DT_SPEC_GET_BY_NAME(node_id, name)                                              \
	{                                                                                          \
		.dev = DEVICE_DT_GET(DT_DMAS_CTLR_BY_NAME(node_id, name)),                         \
		.channel = DT_DMAS_CELL_BY_NAME(node_id, name, channel),                           \
		.request = DT_DMAS_CELL_BY_NAME(node_id, name, request),                           \
		.config = DT_DMAS_CELL_BY_NAME(node_id, name, config),                             \
	}

/**
 * @brief Initialize a `sf32lb_dma_dt_spec` structure from a DT instance.
 *
 * @param index DT instance index
 */
#define SF32LB_DMA_DT_INST_SPEC_GET(index) SF32LB_DMA_DT_SPEC_GET(DT_DRV_INST(index))

/**
 * @brief Initialize a `sf32lb_dma_dt_spec` structure from a DT instance.
 *
 * @param index DT instance index
 * @param name DMA name
 */
#define SF32LB_DMA_DT_INST_SPEC_GET_BY_NAME(index, name)                                           \
	SF32LB_DMA_DT_SPEC_GET_BY_NAME(DT_DRV_INST(index), name)

/**
 * @brief Check if the DMA controller is ready
 *
 * @param spec SF32LB DMA DT spec
 *
 * @return true If the DMA controller is ready.
 * @return false If the DMA controller is not ready.
 */
static inline bool sf32lb_dma_is_ready_dt(const struct sf32lb_dma_dt_spec *spec)
{
	return device_is_ready(spec->dev);
}

/**
 * @brief Initialize a DMA configuration structure from a DT spec.
 *
 * This function sets the following fields in the dma_config structure:
 * - dma_slot
 * - channel_priority
 *
 * @param spec SF32LB DMA DT spec
 * @param[out] config DMA configuration
 */
static inline void sf32lb_dma_config_init_dt(const struct sf32lb_dma_dt_spec *spec,
					     struct dma_config *config)
{
	config->dma_slot = spec->request;
	config->channel_priority = FIELD_GET(SF32LB_DMA_PL_MSK, spec->config);
}

/**
 * @brief Configure a DMA channel from a DT spec.
 *
 * @param spec SF32LB DMA DT spec
 * @param config DMA configuration
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP If the configuration is not supported.
 * @retval -errno Other negative errno code failure (see dma_config()).
 */
static inline int sf32lb_dma_config_dt(const struct sf32lb_dma_dt_spec *spec,
				       struct dma_config *config)
{
	return dma_config(spec->dev, spec->channel, config);
}

/**
 * @brief Start a DMA transfer from a DT spec.
 *
 * @param spec SF32LB DMA DT spec
 *
 * @retval 0 If successful.
 * @retval -errno Negative errno code failure (see dma_start()).
 */
static inline int sf32lb_dma_start_dt(const struct sf32lb_dma_dt_spec *spec)
{
	return dma_start(spec->dev, spec->channel);
}

/**
 * @brief Stop a DMA transfer from a DT spec.
 *
 * @param spec SF32LB DMA DT spec
 *
 * @retval 0 If successful.
 * @retval -errno Negative errno code failure (see dma_stop()).
 */
static inline int sf32lb_dma_stop_dt(const struct sf32lb_dma_dt_spec *spec)
{
	return dma_stop(spec->dev, spec->channel);
}

/**
 * @brief Reload a DMA transfer from a DT spec.
 *
 * @param spec SF32LB DMA DT spec
 * @param src Source address
 * @param dst Destination address
 * @param size Transfer size
 *
 * @retval 0 If successful.
 * @retval -errno Negative errno code failure (see dma_reload()).
 */
static inline int sf32lb_dma_reload_dt(const struct sf32lb_dma_dt_spec *spec, uintptr_t src,
				       uintptr_t dst, size_t size)
{
	return dma_reload(spec->dev, spec->channel, src, dst, size);
}

/**
 * @brief Get the status of a DMA transfer from a DT spec.
 *
 * @param spec SF32LB DMA DT spec
 * @param[out] status Status output
 *
 * @retval 0 If successful.
 * @retval -errno Negative errno code failure (see dma_get_status()).
 */
static inline int sf32lb_dma_get_status_dt(const struct sf32lb_dma_dt_spec *spec,
					   struct dma_status *status)
{
	return dma_get_status(spec->dev, spec->channel, status);
}

/** @} */

#endif /* INCLUDE_ZEPHYR_DRIVERS_DMA_SF32LB_H_ */
