/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef LD_DVFS_H
#define LD_DVFS_H

#include <dvfs_oppoint.h>
#include <nrfs_common.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Function for initializing the Dynamic Voltage and Frequency Scaling service
 *        from LD perspective..
 *
 */
void ld_dvfs_init(void);

/**
 * @brief Function for clearing the zero bias
 *
 */
void ld_dvfs_clear_zbb(void);

/**
 * @brief Configure ABB registers to transition process.
 *
 * @param transient_opp current operation point
 * @param curr_targ_opp target operation point
 */
void ld_dvfs_configure_abb_for_transition(enum dvfs_frequency_setting transient_opp,
					  enum dvfs_frequency_setting curr_targ_opp);

/**
 * @brief Configure hsfll depending on selected oppoint
 *
 * @param enum oppoint target operation point
 * @return 0 value indicates no error.
 * @return -EINVAL invalid oppoint or domain.
 * @return -ETIMEDOUT frequency change took more than HSFLL_FREQ_CHANGE_MAX_DELAY_MS
 */
int32_t ld_dvfs_configure_hsfll(enum dvfs_frequency_setting oppoint);

/**
 * @brief Background process during scaling.
 *
 * @param downscaling indicates if down-scaling is running
 */
void ld_dvfs_scaling_background_process(bool downscaling);

/**
 * @brief Last step for local domain in downscale procedure
 *
 * @param downscaling indicates if down-scaling is running
 */
void ld_dvfs_scaling_finish(bool downscaling);

#ifdef __cplusplus
}
#endif

#endif /* LD_DVFS_H */
