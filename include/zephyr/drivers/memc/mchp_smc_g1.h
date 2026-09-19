/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Header file for Microchip SMC G1 extended operations.
 */

#ifndef __ZEPHYR_INCLUDE_DRIVERS_MEMC_MCHP_SMC_G1_H__
#define __ZEPHYR_INCLUDE_DRIVERS_MEMC_MCHP_SMC_G1_H__

#ifdef CONFIG_MEMC_MCHP_HSMC_G1
/** Redefining register names for HSMC controller */
#define SMC(name) HSMC_##name
#else
/** Redefining register names for SMC controller */
#define SMC(name) SMC_##name
#endif

/** Macro Assistant for handling setup parameters */
#define SMC_REG_SETUP(node_id)								\
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, smc_setup_timing),			\
	(SMC(SETUP_NCS_RD_SETUP)(DT_PROP_BY_IDX(node_id, smc_setup_timing, 3)) |	\
	 SMC(SETUP_NRD_SETUP)(DT_PROP_BY_IDX(node_id, smc_setup_timing, 2)) |		\
	 SMC(SETUP_NCS_WR_SETUP)(DT_PROP_BY_IDX(node_id, smc_setup_timing, 1)) |	\
	 SMC(SETUP_NWE_SETUP)(DT_PROP_BY_IDX(node_id, smc_setup_timing, 0))), (0))
/** Macro Assistant for handling pulse parameters */
#define SMC_REG_PULSE(node_id)								\
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, smc_pulse_timing),			\
	(SMC(PULSE_NCS_RD_PULSE)(DT_PROP_BY_IDX(node_id, smc_pulse_timing, 3)) |	\
	 SMC(PULSE_NRD_PULSE)(DT_PROP_BY_IDX(node_id, smc_pulse_timing, 2)) |		\
	 SMC(PULSE_NCS_WR_PULSE)(DT_PROP_BY_IDX(node_id, smc_pulse_timing, 1)) |	\
	 SMC(PULSE_NWE_PULSE)(DT_PROP_BY_IDX(node_id, smc_pulse_timing, 0))), (0))
/** Macro Assistant for handling cycle parameters */
#define SMC_REG_CYCLE(node_id)								\
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, smc_cycle_timing),			\
	(SMC(CYCLE_NRD_CYCLE)(DT_PROP_BY_IDX(node_id, smc_cycle_timing, 1)) |		\
	 SMC(CYCLE_NWE_CYCLE)(DT_PROP_BY_IDX(node_id, smc_cycle_timing, 0))), (0))
/** Macro Assistant for handling mode parameters */
#define SMC_REG_MODE(node_id)								\
	SMC(MODE_TDF_MODE)(DT_PROP(node_id, smc_tdf_mode)) |				\
	COND_CODE_1(DT_PROP(node_id, smc_tdf_mode),					\
		    (SMC(MODE_TDF_CYCLES)(DT_PROP(node_id, smc_tdf_cycles))), (0)) |	\
	COND_CODE_1(DT_ENUM_IDX(node_id, smc_bus_width),				\
		    (SMC(MODE_DBW_BIT_16)), (0)) |					\
	COND_CODE_1(DT_ENUM_IDX(node_id, smc_write_mode),				\
		    (SMC(MODE_WRITE_MODE_NWE_CTRL)), (0)) |				\
	COND_CODE_1(DT_ENUM_IDX(node_id, smc_read_mode),				\
		    (SMC(MODE_READ_MODE_NRD_CTRL)), (0))

#ifdef CONFIG_MEMC_MCHP_HSMC_G1
/** Macro Assistant for handling HSMC timings parameters */
#define SMC_REG_TIMINGS(node_id)							\
	SMC(TIMINGS_NFSEL)(DT_PROP(node_id, nfc_timings_nfsel)) |			\
	SMC(TIMINGS_TWB)(DT_PROP_OR(node_id, nfc_timings_twb, 0)) |			\
	SMC(TIMINGS_TRR)(DT_PROP_OR(node_id, nfc_timings_trr, 0)) |			\
	SMC(TIMINGS_OCMS)(DT_PROP(node_id, nfc_timings_ocms)) |				\
	SMC(TIMINGS_TAR)(DT_PROP_OR(node_id, nfc_timings_tar, 0)) |			\
	SMC(TIMINGS_TADL)(DT_PROP_OR(node_id, nfc_timings_tadl, 0)) |			\
	SMC(TIMINGS_TCLR)(DT_PROP_OR(node_id, nfc_timings_tclr, 0))
#else
/** Macro Assistant for discarding timings parameters */
#define REG_TIMINGS(node_id) 0
#endif

/**
 * @brief SMC/HSMC CS line configuration
 */
struct cs_config {
	/** CS line number */
	uint32_t cs;
	/** CS line SETUP configuration */
	uint32_t setup;
	/** CS line PULSE configuration */
	uint32_t pulse;
	/** CS line CYCLE configuration */
	uint32_t cycle;
	/** CS line TIMINGS configuration */
	uint32_t timings;
	/** CS line MODE configuration */
	uint32_t mode;
};

/**
 * @brief Initialize(clear) the CS line configuration
 *
 * @param cs line configuration structure
 */
void smc_cs_conf_init(struct cs_config *cs);

/**
 * @brief Configure the SETUP timing of the CS
 *
 * @param cs line configuration
 * @param shift shifting in SMC_SETUP register
 * @param ncycles period in nanoseconds
 */
int smc_cs_conf_set_setup(struct cs_config *cs, uint32_t shift, uint32_t ncycles);

/**
 * @brief Configure the PULSE timing of the CS
 *
 * @param cs line configuration
 * @param shift shifting in SMC_PULSE register
 * @param ncycles period in nanoseconds
 */
int smc_cs_conf_set_pulse(struct cs_config *cs, uint32_t shift, uint32_t ncycles);

/**
 * @brief Configure the CYCLE timing of the CS
 *
 * @param cs line configuration
 * @param shift shifting in SMC_CYCLE register
 * @param ncycles period in nanoseconds
 */
int smc_cs_conf_set_cycle(struct cs_config *cs, uint32_t shift, uint32_t ncycles);

/**
 * @brief Configure the TIMING timing of the CS
 *
 * @param cs line configuration
 * @param shift shifting in SMC_TIMING register
 * @param ncycles period in nanoseconds
 */
int smc_cs_conf_set_timing(struct cs_config *cs, uint32_t shift, uint32_t ncycles);

/**
 * @brief Apply configuration to a CS line
 *
 * @param dev smc device instance.
 * @param cs line configuration
 */
int smc_cs_conf_apply(const struct device *dev, const struct cs_config *cs);

/**
 * @brief Set the MCK clock configuration of SMC dev
 *
 * @param dev smc device instance.
 * @param cfg clock configuration
 */
int smc_set_mck_cfg(const struct device *dev, clock_control_subsys_t cfg);

/**
 * @brief Get the MCK clock rate of SMC dev
 *
 * @param dev smc device instance.
 * @param rate clock rate pointer
 */
int smc_get_mck_rate(const struct device *dev, uint32_t *rate);

#endif /* __ZEPHYR_INCLUDE_DRIVERS_MEMC_MCHP_SMC_G1_H__ */
