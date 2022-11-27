/*
 * Copyright (c) 2019 Vestas Wind Systems A/S
 * Copyright (c) 2022 Carl Zeiss Meditec AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CAN_MCUX_FLEXCAN_COMMON_H
#define CAN_MCUX_FLEXCAN_COMMON_H

#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/drivers/can.h>
#include <zephyr/drivers/can/transceiver.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/device.h>
#include <zephyr/sys/byteorder.h>
#include <fsl_flexcan.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>

#define SP_IS_SET(inst) DT_INST_NODE_HAS_PROP(inst, sample_point) ||

/* Macro to exclude the sample point algorithm from compilation if not used
 * Without the macro, the algorithm would always waste ROM
 */
#define USE_SP_ALGO (DT_INST_FOREACH_STATUS_OKAY(SP_IS_SET) 0)

#define SP_AND_TIMING_NOT_SET(inst)                                                                \
	(!DT_INST_NODE_HAS_PROP(inst, sample_point) &&                                             \
	 !(DT_INST_NODE_HAS_PROP(inst, prop_seg) && DT_INST_NODE_HAS_PROP(inst, phase_seg1) &&     \
	   DT_INST_NODE_HAS_PROP(inst, phase_seg2))) ||

#if DT_INST_FOREACH_STATUS_OKAY(SP_AND_TIMING_NOT_SET) 0
#error You must either set a sampling-point or timings (phase-seg* and prop-seg)
#endif

#if ((defined(FSL_FEATURE_FLEXCAN_HAS_ERRATA_5641) && FSL_FEATURE_FLEXCAN_HAS_ERRATA_5641) ||      \
(defined(FSL_FEATURE_FLEXCAN_HAS_ERRATA_5829) && FSL_FEATURE_FLEXCAN_HAS_ERRATA_5829))
/* the first valid MB should be occupied by ERRATA 5461 or 5829. */
#define RX_START_IDX 1
#else
#define RX_START_IDX 0
#endif

/*
 * RX message buffers (filters) will take up the first N message
 * buffers. The rest are available for TX use.
 */
#define MCUX_FLEXCAN_MAX_RX (CONFIG_CAN_MAX_FILTER + RX_START_IDX)

/*
 * Convert from RX message buffer index to allocated filter ID and
 * vice versa.
 */
#define RX_MBIDX_TO_ALLOC_IDX(x) (x)
#define ALLOC_IDX_TO_RXMB_IDX(x) (x)

/*
 * Convert from TX message buffer index to allocated TX ID and vice
 * versa.
 */
#define TX_MBIDX_TO_ALLOC_IDX(x) (x - MCUX_FLEXCAN_MAX_RX)
#define ALLOC_IDX_TO_TXMB_IDX(x) (x + MCUX_FLEXCAN_MAX_RX)

/* Convert from back from FLEXCAN IDs to Zephyr CAN IDs. */
#define FLEXCAN_ID_TO_CAN_ID_STD(id)                                                               \
	((uint32_t)((((uint32_t)(id)) & CAN_ID_STD_MASK) >> CAN_ID_STD_SHIFT))
#define FLEXCAN_ID_TO_CAN_ID_EXT(id)                                                               \
	((uint32_t)((((uint32_t)(id)) & (CAN_ID_STD_MASK | CAN_ID_EXT_MASK)) >> CAN_ID_EXT_SHIFT))

struct mcux_flexcan_generic_config {
	CAN_Type *base;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	int clk_source;
	uint32_t bitrate;
	uint32_t sample_point;
	uint32_t sjw;
	uint32_t prop_seg;
	uint32_t phase_seg1;
	uint32_t phase_seg2;
	uint32_t bus_speed_data;
	uint32_t sjw_data;
	uint32_t sample_point_data;
	void (*irq_config_func)(const struct device *dev);
	void (*irq_enable_func)(void);
	void (*irq_disable_func)(void);
	const struct device *phy;
	uint32_t max_bitrate;
#ifdef CONFIG_PINCTRL
	const struct pinctrl_dev_config *pincfg;
#endif
};

int mcux_flexcan_common_verify_frame_filter_flags(const bool is_fd_compatible, const uint8_t flags);

int mcux_flexcan_common_get_max_filters(const struct device *dev, bool ide);

int mcux_flexcan_common_get_max_bitrate(const struct device *dev, uint32_t *max_bitrate);

int mcux_flexcan_common_set_timing(struct can_timing *flexcan_timing,
				   const struct can_timing *reference_timing, bool is_started);

void mcux_flexcan_config_ctrl1(can_mode_t mode, CAN_Type *can_base);

void mcux_flexcan_config_mcr(can_mode_t mode, CAN_Type *can_base);

int mcux_flexcan_common_verify_can_frame_flags(const uint8_t dlc, const uint8_t flags,
					       const uint32_t frame_id, const bool is_fd_frame);

void mcux_flexcan_common_can_filter_to_mbconfig(const struct can_filter *src,
						flexcan_rx_mb_config_t *dest, uint32_t *mask);

int mcux_flexcan_common_get_core_clock(const struct device *dev, uint32_t *rate);

void mcux_flexcan_common_get_state(const struct mcux_flexcan_generic_config *config,
				   bool is_started, enum can_state *state,
				   struct can_bus_err_cnt *const err_cnt);

int mcux_flexcan_common_set_can_mode(const struct mcux_flexcan_generic_config *config,
				     can_mode_t mode, bool is_started, bool is_can_fd_configured);

int mcux_flexcan_common_check_can_start(const struct mcux_flexcan_generic_config *const config,
				    bool is_started);

void mcux_flexcan_common_extract_timing_from_can_timing(flexcan_timing_config_t *flexcan_Timing,
						struct can_timing *timing);

int mcux_flexcan_common_init_check_ready(const struct device *can_transceiver,
					 const struct device *can_clock);

int mcux_flexcan_common_calc_timing(const struct device *can_dev, struct can_timing *timing,
				    uint32_t bitrate, uint32_t sample_point);

void mcux_flexcan_common_config_calc_bitrate(
	const struct device *dev, const struct mcux_flexcan_generic_config *const config,
	struct can_timing *const timing);

void mcux_flexcan_common_init_config(flexcan_config_t *const flexcan_config,
				     const struct can_timing *const timing,
				     const uint32_t clock_freq, const int clock_source,
				     const uint8_t max_mb);

void increment_error_counters(const struct device *dev, uint64_t error);

/* Function for copying CAN 2.0 frames */
void mcux_flexcan_from_can_frame(const struct can_frame *src, flexcan_frame_t *dest);

void mcux_flexcan_to_can_frame(const flexcan_frame_t *src, struct can_frame *dest);

#endif /* CAN_MCUX_FLEXCAN_COMMON_H */
