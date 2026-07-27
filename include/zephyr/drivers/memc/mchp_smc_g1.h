/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __ZEPHYR_INCLUDE_DRIVERS_MEMC_MCHP_SMC_G1_H__
#define __ZEPHYR_INCLUDE_DRIVERS_MEMC_MCHP_SMC_G1_H__

#ifdef CONFIG_MEMC_MCHP_HSMC_G1
#define SMC(name) HSMC_##name
#else
#define SMC(name) SMC_##name
#endif

#define SMC_REG_SETUP(node_id)								\
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, smc_setup_timing),			\
	(SMC(SETUP_NCS_RD_SETUP)(DT_PROP_BY_IDX(node_id, smc_setup_timing, 3)) |	\
	 SMC(SETUP_NRD_SETUP)(DT_PROP_BY_IDX(node_id, smc_setup_timing, 2)) |		\
	 SMC(SETUP_NCS_WR_SETUP)(DT_PROP_BY_IDX(node_id, smc_setup_timing, 1)) |	\
	 SMC(SETUP_NWE_SETUP)(DT_PROP_BY_IDX(node_id, smc_setup_timing, 0))), (0))
#define SMC_REG_PULSE(node_id)								\
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, smc_pulse_timing),			\
	(SMC(PULSE_NCS_RD_PULSE)(DT_PROP_BY_IDX(node_id, smc_pulse_timing, 3)) |	\
	 SMC(PULSE_NRD_PULSE)(DT_PROP_BY_IDX(node_id, smc_pulse_timing, 2)) |		\
	 SMC(PULSE_NCS_WR_PULSE)(DT_PROP_BY_IDX(node_id, smc_pulse_timing, 1)) |	\
	 SMC(PULSE_NWE_PULSE)(DT_PROP_BY_IDX(node_id, smc_pulse_timing, 0))), (0))
#define SMC_REG_CYCLE(node_id)								\
	COND_CODE_1(DT_NODE_HAS_PROP(node_id, smc_cycle_timing),			\
	(SMC(CYCLE_NRD_CYCLE)(DT_PROP_BY_IDX(node_id, smc_cycle_timing, 1)) |		\
	 SMC(CYCLE_NWE_CYCLE)(DT_PROP_BY_IDX(node_id, smc_cycle_timing, 0))), (0))
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
#define SMC_REG_TIMINGS(node_id)							\
	SMC(TIMINGS_NFSEL)(DT_PROP(node_id, nfc_timings_nfsel)) |			\
	SMC(TIMINGS_TWB)(DT_PROP_OR(node_id, nfc_timings_twb, 0)) |			\
	SMC(TIMINGS_TRR)(DT_PROP_OR(node_id, nfc_timings_trr, 0)) |			\
	SMC(TIMINGS_OCMS)(DT_PROP(node_id, nfc_timings_ocms)) |				\
	SMC(TIMINGS_TAR)(DT_PROP_OR(node_id, nfc_timings_tar, 0)) |			\
	SMC(TIMINGS_TADL)(DT_PROP_OR(node_id, nfc_timings_tadl, 0)) |			\
	SMC(TIMINGS_TCLR)(DT_PROP_OR(node_id, nfc_timings_tclr, 0))
#else
#define REG_TIMINGS(node_id) 0
#endif

struct cs_config {
	uint32_t cs;
	uint32_t setup;
	uint32_t pulse;
	uint32_t cycle;
	uint32_t timings;
	uint32_t mode;
};

void smc_cs_conf_init(struct cs_config *cs);
int smc_cs_conf_set_setup(struct cs_config *cs, uint32_t shift, uint32_t ncycles);
int smc_cs_conf_set_pulse(struct cs_config *cs, uint32_t shift, uint32_t ncycles);
int smc_cs_conf_set_cycle(struct cs_config *cs, uint32_t shift, uint32_t ncycles);
int smc_cs_conf_set_timing(struct cs_config *cs, uint32_t shift, uint32_t ncycles);
int smc_cs_conf_apply(const struct device *dev, const struct cs_config *cs);
int smc_set_mck_cfg(const struct device *dev, clock_control_subsys_t cfg);
int smc_get_mck_rate(const struct device *dev, uint32_t *rate);

#endif /* __ZEPHYR_INCLUDE_DRIVERS_MEMC_MCHP_SMC_G1_H__ */
