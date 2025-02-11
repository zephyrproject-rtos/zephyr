/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT	nxp_s32_qspi

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nxp_s32_qspi_memc, CONFIG_MEMC_LOG_LEVEL);

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/sys/util.h>

#include <soc.h>
#include "memc_nxp_s32_qspi.h"

/* Mapping between QSPI chip select signals and devicetree chip select identifiers */
#define QSPI_PCSFA1	0
#define QSPI_PCSFA2	1
#define QSPI_PCSFB1	2
#define QSPI_PCSFB2	3

struct memc_nxp_s32_qspi_data {
	uint8_t instance;
};

struct memc_nxp_s32_qspi_config {
	QuadSPI_Type *base;
	const struct pinctrl_dev_config *pincfg;

	const Qspi_Ip_ControllerConfigType *controller_cfg;
};

static inline uint8_t get_instance(QuadSPI_Type *base)
{
	QuadSPI_Type *const base_ptrs[QuadSPI_INSTANCE_COUNT] = IP_QuadSPI_BASE_PTRS;
	uint8_t i;

	for (i = 0; i < QuadSPI_INSTANCE_COUNT; i++) {
		if (base_ptrs[i] == base) {
			break;
		}
	}
	__ASSERT_NO_MSG(i < QuadSPI_INSTANCE_COUNT);

	return i;
}

static int memc_nxp_s32_qspi_init(const struct device *dev)
{
	const struct memc_nxp_s32_qspi_config *config = dev->config;
	struct memc_nxp_s32_qspi_data *data = dev->data;
	Qspi_Ip_StatusType status;

	data->instance = get_instance(config->base);

	if (pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT)) {
		return -EIO;
	}

	status = Qspi_Ip_ControllerInit(data->instance, config->controller_cfg);
	if (status != STATUS_QSPI_IP_SUCCESS) {
		LOG_ERR("Fail to initialize QSPI controller %d (%d)",
			data->instance, status);
		return -EIO;
	}

	return 0;
}

uint8_t memc_nxp_s32_qspi_get_instance(const struct device *dev)
{
	struct memc_nxp_s32_qspi_data *data = dev->data;

	return data->instance;
}

#define QSPI_DATA_CFG(n)								\
	IF_ENABLED(FEATURE_QSPI_DDR, (							\
		.dataRate = _CONCAT(QSPI_IP_DATA_RATE_,					\
			DT_INST_STRING_UPPER_TOKEN(n, data_rate)),			\
		.dataAlign = COND_CODE_1(DT_INST_PROP(n, hold_time_2x),			\
			(QSPI_IP_FLASH_DATA_ALIGN_2X_REFCLK),				\
			(QSPI_IP_FLASH_DATA_ALIGN_REFCLK)),				\
	))

#define QSPI_ADDR_CFG(n) \
	IF_ENABLED(FEATURE_QSPI_ADDR_CFG, (						\
		.columnAddr = DT_INST_PROP_OR(n, column_space, 0),			\
		.wordAddresable = DT_INST_PROP(n, word_addressable),			\
	))

#define QSPI_BYTES_SWAP_ADDR(n)								\
	IF_ENABLED(FEATURE_QSPI_BYTES_SWAP_ADDR,					\
		(.byteSwap = DT_INST_PROP(n, byte_swapping),))

#define QSPI_SAMPLE_DELAY(n)								\
	COND_CODE_1(DT_INST_PROP(n, sample_delay_half_cycle),				\
		(QSPI_IP_SAMPLE_DELAY_HALFCYCLE_EARLY_DQS),				\
		(QSPI_IP_SAMPLE_DELAY_SAME_DQS))

#define QSPI_SAMPLE_PHASE(n)								\
	COND_CODE_1(DT_INST_PROP(n, sample_phase_inverted),				\
		(QSPI_IP_SAMPLE_PHASE_INVERTED),					\
		(QSPI_IP_SAMPLE_PHASE_NON_INVERTED))

#define QSPI_AHB_BUFFERS(n)								\
	{										\
		.masters = DT_INST_PROP(n, ahb_buffers_masters),			\
		.sizes = DT_INST_PROP(n, ahb_buffers_sizes),				\
		.allMasters = (bool)DT_INST_PROP(n, ahb_buffers_all_masters),		\
	}

#define QSPI_DLL_CFG(n, side, side_upper)						\
	IF_ENABLED(FEATURE_QSPI_HAS_DLL, (						\
		.dllSettings##side_upper = {						\
			.dllMode = _CONCAT(QSPI_IP_DLL_,				\
				DT_INST_STRING_UPPER_TOKEN(n, side##_dll_mode)),	\
			.freqEnable = DT_INST_PROP(n, side##_dll_freq_enable),		\
			.coarseDelay = DT_INST_PROP(n, side##_dll_coarse_delay),	\
			.fineDelay = DT_INST_PROP(n, side##_dll_fine_delay),		\
			.tapSelect = DT_INST_PROP(n, side##_dll_tap_select),		\
			IF_ENABLED(FEATURE_QSPI_DLL_LOOPCONTROL, (			\
			.referenceCounter = DT_INST_PROP(n, side##_dll_ref_counter),	\
			.resolution = DT_INST_PROP(n, side##_dll_resolution),		\
			))								\
		},									\
	))

#define QSPI_READ_MODE(n, side, side_upper)						\
	_CONCAT(QSPI_IP_READ_MODE_, DT_INST_STRING_UPPER_TOKEN(n, side##_rx_clock_source))

#define QSPI_IDLE_SIGNAL_DRIVE(n, side, side_upper)					\
	IF_ENABLED(FEATURE_QSPI_CONFIGURABLE_ISD, (					\
		.io2IdleValue##side_upper = (uint8_t)DT_INST_PROP(n, side##_io2_idle_high),\
		.io3IdleValue##side_upper = (uint8_t)DT_INST_PROP(n, side##_io3_idle_high),\
	))

#define QSPI_PORT_SIZE_FN(node_id, side_upper, port)					\
	COND_CODE_1(IS_EQ(DT_REG_ADDR(node_id), QSPI_PCSF##side_upper##port),		\
		(COND_CODE_1(DT_NODE_HAS_STATUS(node_id, okay),				\
			(.memSize##side_upper##port = DT_PROP(node_id, size) / 8,),	\
			(.memSize##side_upper##port = 0,))),				\
		(EMPTY))

#define QSPI_PORT_SIZE(n, side_upper)							\
	DT_INST_FOREACH_CHILD_VARGS(n, QSPI_PORT_SIZE_FN, side_upper, 1)		\
	DT_INST_FOREACH_CHILD_VARGS(n, QSPI_PORT_SIZE_FN, side_upper, 2)

#define QSPI_SIDE_CFG(n, side, side_upper)						\
	QSPI_IDLE_SIGNAL_DRIVE(n, side, side_upper)					\
	QSPI_DLL_CFG(n, side, side_upper)						\
	QSPI_PORT_SIZE(n, side_upper)							\
	.readMode##side_upper = QSPI_READ_MODE(n, side, side_upper),

#define MEMC_NXP_S32_QSPI_CONTROLLER_CONFIG(n)						\
	BUILD_ASSERT(DT_INST_PROP_LEN(n, ahb_buffers_masters) == QSPI_IP_AHB_BUFFERS,	\
		"ahb-buffers-masters must be of size QSPI_IP_AHB_BUFFERS");		\
	BUILD_ASSERT(DT_INST_PROP_LEN(n, ahb_buffers_sizes) == QSPI_IP_AHB_BUFFERS,	\
		"ahb-buffers-sizes must be of size QSPI_IP_AHB_BUFFERS");		\
	BUILD_ASSERT(									\
		_CONCAT(FEATURE_QSPI_, DT_INST_STRING_UPPER_TOKEN(n, a_rx_clock_source)) == 1,\
		"a-rx-clock-source source mode selected is not supported");		\
											\
	static const Qspi_Ip_ControllerConfigType					\
	memc_nxp_s32_qspi_controller_cfg_##n = {					\
		.csHoldTime = DT_INST_PROP(n, cs_hold_time),				\
		.csSetupTime = DT_INST_PROP(n, cs_setup_time),				\
		.sampleDelay = QSPI_SAMPLE_DELAY(n),					\
		.samplePhase = QSPI_SAMPLE_PHASE(n),					\
		.ahbConfig = QSPI_AHB_BUFFERS(n),					\
		QSPI_SIDE_CFG(n, a, A)							\
		QSPI_DATA_CFG(n)							\
		QSPI_ADDR_CFG(n)							\
		QSPI_BYTES_SWAP_ADDR(n)							\
	}

#define MEMC_NXP_S32_QSPI_INIT_DEVICE(n)						\
	PINCTRL_DT_INST_DEFINE(n);							\
	MEMC_NXP_S32_QSPI_CONTROLLER_CONFIG(n);						\
	static struct memc_nxp_s32_qspi_data memc_nxp_s32_qspi_data_##n;		\
	static const struct memc_nxp_s32_qspi_config memc_nxp_s32_qspi_config_##n = {	\
		.base = (QuadSPI_Type *)DT_INST_REG_ADDR(n),				\
		.controller_cfg = &memc_nxp_s32_qspi_controller_cfg_##n,		\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),				\
	};										\
	DEVICE_DT_INST_DEFINE(n,							\
		memc_nxp_s32_qspi_init,							\
		NULL,									\
		&memc_nxp_s32_qspi_data_##n,						\
		&memc_nxp_s32_qspi_config_##n,						\
		POST_KERNEL,								\
		CONFIG_MEMC_INIT_PRIORITY,						\
		NULL);

DT_INST_FOREACH_STATUS_OKAY(MEMC_NXP_S32_QSPI_INIT_DEVICE)
