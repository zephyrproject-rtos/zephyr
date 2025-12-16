/*
 * Copyright 2025 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT	nxp_s32_xspi

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(nxp_s32_xspi_memc, CONFIG_MEMC_LOG_LEVEL);

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/sys/util.h>
#include <zephyr/dt-bindings/xspi/nxp-s32-xspi.h>

#include <soc.h>
#include <Xspi_Ip.h>
#include "memc_nxp_s32_xspi.h"

/* Mapping between XSPI chip select signals and devicetree chip select identifiers */
#define XSPI_PCSFA1	0
#define XSPI_PCSFA2	1
#define XSPI_PCSFB1	2
#define XSPI_PCSFB2	3
#define XSPI_DLLA	0
#define XSPI_DLLB	1

struct memc_nxp_s32_xspi_data {
	uint8_t instance;
};

struct memc_nxp_s32_xspi_config {
	XSPI_Type *base;
	const struct pinctrl_dev_config *pincfg;

	const Xspi_Ip_ControllerConfigType *controller_cfg;
};

static inline uint8_t get_instance(XSPI_Type *base)
{
	XSPI_Type *const base_ptrs[XSPI_INSTANCE_COUNT] = IP_XSPI_BASE_PTRS;
	uint8_t i;

	for (i = 0; i < XSPI_INSTANCE_COUNT; i++) {
		if (base_ptrs[i] == base) {
			break;
		}
	}
	__ASSERT_NO_MSG(i < XSPI_INSTANCE_COUNT);

	return i;
}

static int memc_nxp_s32_xspi_init(const struct device *dev)
{
	const struct memc_nxp_s32_xspi_config *config = dev->config;
	struct memc_nxp_s32_xspi_data *data = dev->data;
	Xspi_Ip_StatusType status;

	data->instance = get_instance(config->base);

	if (pinctrl_apply_state(config->pincfg, PINCTRL_STATE_DEFAULT)) {
		return -EIO;
	}

	status = Xspi_Ip_ControllerInit(data->instance, config->controller_cfg);
	if (status != STATUS_XSPI_IP_SUCCESS) {
		LOG_ERR("Failed to initialize XSPI controller %d (%d)",
			data->instance, status);
		return -EIO;
	}

	return 0;
}

uint8_t memc_nxp_s32_xspi_get_instance(const struct device *dev)
{
	struct memc_nxp_s32_xspi_data *data = dev->data;

	return data->instance;
}

#define XSPI_DATA_CFG(n)								\
		.dataRate = _CONCAT(XSPI_IP_DATA_RATE_,					\
			DT_INST_STRING_UPPER_TOKEN(n, data_rate)),			\
		.dataAlign = COND_CODE_1(DT_INST_PROP(n, hold_time_2x),			\
			(XSPI_IP_FLASH_DATA_ALIGN_2X_REFCLK),				\
			(XSPI_IP_FLASH_DATA_ALIGN_REFCLK)),				\

#define XSPI_ADDR_CFG(n) \
		.columnAddr = DT_INST_PROP(n, column_space),				\
		.wordAddresable = DT_INST_PROP(n, word_addressable),			\
		.dWordAddresable = DT_INST_PROP(n, dword_addressable),			\

#define XSPI_BYTES_SWAP_ADDR(n)								\
		.byteSwap = DT_INST_PROP(n, byte_swapping),

#define XSPI_SAMPLE_DELAY(n)								\
	COND_CODE_1(DT_INST_PROP(n, sample_delay_half_cycle),				\
		(XSPI_IP_SAMPLE_DELAY_HALFCYCLE_EARLY_DQS),				\
		(XSPI_IP_SAMPLE_DELAY_SAME_DQS))

#define XSPI_SAMPLE_PHASE(n)								\
	COND_CODE_1(DT_INST_PROP(n, sample_phase_inverted),				\
		(XSPI_IP_SAMPLE_PHASE_INVERTED),					\
		(XSPI_IP_SAMPLE_PHASE_NON_INVERTED))

#define XSPI_AHB_BUFFERS(n)								\
	{										\
		.masters = DT_INST_PROP(n, ahb_buffers_masters),			\
		.sizes = DT_INST_PROP(n, ahb_buffers_sizes),				\
		.allMasters = (bool)DT_INST_PROP(n, ahb_buffers_all_masters),		\
	}

#define XSPI_DLL_CFG(n, side, side_upper)						\
	IF_ENABLED(FEATURE_XSPI_HAS_DLL, (						\
		.dllSettings[XSPI_DLL##side_upper] = {					\
			.dllMode = _CONCAT(XSPI_IP_DLL_,				\
				DT_INST_STRING_UPPER_TOKEN(n, side##_dll_mode)),	\
			.freqEnable = DT_INST_PROP(n, side##_dll_freq_enable),		\
			.coarseDelay = DT_INST_PROP(n, side##_dll_coarse_delay),	\
			.fineDelay = DT_INST_PROP(n, side##_dll_fine_delay),		\
			.tapSelect = DT_INST_PROP(n, side##_dll_tap_select),		\
			.referenceCounter = DT_INST_PROP(n, side##_dll_ref_counter),	\
			.resolution = DT_INST_PROP(n, side##_dll_resolution),		\
		},									\
	))

#define XSPI_READ_MODE(n, side, side_upper)						\
	_CONCAT(XSPI_IP_READ_MODE_, DT_INST_STRING_UPPER_TOKEN(n, side##_rx_clock_source))

#define XSPI_IDLE_SIGNAL_DRIVE(n, side, side_upper)					\
		.io2IdleValue##side_upper = (uint8_t)DT_INST_PROP(n, side##_io2_idle_high),\
		.io3IdleValue##side_upper = (uint8_t)DT_INST_PROP(n, side##_io3_idle_high),\

#define XSPI_PORT_SIZE_FN(node_id, side_upper, port)					\
	COND_CODE_1(IS_EQ(DT_REG_ADDR_RAW(node_id), XSPI_PCSF##side_upper##port),	\
		(COND_CODE_1(DT_NODE_HAS_STATUS_OKAY(node_id),				\
			(.memSize##side_upper##port = DT_PROP(node_id, size) / BITS_PER_BYTE,),\
			(.memSize##side_upper##port = 0,))),				\
		(EMPTY))

#define XSPI_PORT_SIZE(n, side_upper)							\
	DT_INST_FOREACH_CHILD_VARGS(n, XSPI_PORT_SIZE_FN, side_upper, 1)		\
	DT_INST_FOREACH_CHILD_VARGS(n, XSPI_PORT_SIZE_FN, side_upper, 2)

#define XSPI_SIDE_CFG(n, side, side_upper)						\
	XSPI_IDLE_SIGNAL_DRIVE(n, side, side_upper)					\
	XSPI_DLL_CFG(n, side, side_upper)						\
	XSPI_PORT_SIZE(n, side_upper)							\
	.readMode##side_upper = XSPI_READ_MODE(n, side, side_upper),

#define XSPI_DQS_OUTPUT_ENABLE(n)							\
	.dqsAsAnOutput = DT_INST_PROP(n, dqs_as_an_output),

#define XSPI_DIFFERENTIAL_CLOCK_CFG(n)							\
	.differentialClockA = DT_INST_PROP(n, a_differential_clock),

#define XSPI_ERROR_HANDLING_CFG(n)							\
	.enableHrespMask = DT_INST_PROP(n, enable_hresp_mask),				\
	.errPayloadHigh = DT_INST_PROP(n, error_payload_high),				\
	.errPayloadLow = DT_INST_PROP(n, error_payload_low),

#define XSPI_TIMEOUT_CFG(n)								\
	.ahbTimeout = DT_INST_PROP(n, ahb_timeout),					\
	.transactionTimeout = DT_INST_PROP(n, transaction_timeout),			\
	.arbitrationTimeout = DT_INST_PROP(n, arbitration_timeout),

#define XSPI_PRIORITY_CFG(n)								\
	.Tg1FixPrio = DT_INST_PROP(n, tg1_fixed_priority),

#define XSPI_LOCK_CFG(n)								\
	.lockConfiguration = DT_INST_PROP(n, lock_configuration),

#define SFP_MDAD_NODE(n) DT_INST_CHILD(n, sfp_mdad)

#define XSPI_SECURE_ATTRIBUTE(node_id)							\
	((DT_PROP(node_id, secure_attribute) == NXP_S32_XSPI_NON_SECURE) ?		\
		XSPI_IP_SFP_UNSECURE :							\
	 (DT_PROP(node_id, secure_attribute) == NXP_S32_XSPI_SECURE) ?			\
		XSPI_IP_SFP_SECURE :							\
	 (DT_PROP(node_id, secure_attribute) ==						\
		(NXP_S32_XSPI_NON_SECURE | NXP_S32_XSPI_SECURE)) ?			\
		XSPI_IP_SFP_BOTH :							\
	 XSPI_IP_SFP_RESERVED)

#define _XSPI_SFP_MDAD_CFG(node_id, n)							\
	{										\
		.SecureAttribute = XSPI_SECURE_ATTRIBUTE(node_id),			\
		.MaskType = DT_ENUM_IDX(node_id, mask_type),				\
		.Valid = true,								\
		.Mask = DT_PROP(node_id, mask),						\
		.DomainId = DT_PROP(node_id, domain_id),				\
	},

#define XSPI_SFP_MDAD_CFG(n)								\
	.Mdad = {									\
		DT_FOREACH_CHILD_STATUS_OKAY_VARGS(SFP_MDAD_NODE(n), _XSPI_SFP_MDAD_CFG, n)\
	},

#define SFP_FRAD_NODE(n) DT_INST_CHILD(n, sfp_frad)

#define XSPI_ACP_POLICY(node_id)							\
	((DT_PROP(node_id, master_domain_acp_policy) ==					\
		NXP_S32_XSPI_SECURE) ?							\
		XSPI_IP_SFP_ACP_SEC_RW_NONSEC_R :					\
	 (DT_PROP(node_id, master_domain_acp_policy) ==					\
		(NXP_S32_XSPI_NON_SECURE | NXP_S32_XSPI_PRIVILEGE)) ?			\
		XSPI_IP_SFP_ACP_PRI_RW_USER_R :						\
	 (DT_PROP(node_id, master_domain_acp_policy) ==					\
		(NXP_S32_XSPI_SECURE | NXP_S32_XSPI_PRIVILEGE)) ?			\
		XSPI_IP_SFP_ACP_SECPRI_RW_ALL_R :					\
	 (DT_PROP(node_id, master_domain_acp_policy) ==					\
		(NXP_S32_XSPI_NON_SECURE | NXP_S32_XSPI_SECURE |			\
		 NXP_S32_XSPI_PRIVILEGE)) ?						\
		XSPI_IP_SFP_ACP_ALL_RW :						\
		XSPI_IP_SFP_ACP_ALL_R)

#define XSPI_ACP_VALID(policy) \
	(((policy) == NXP_S32_XSPI_SECURE) || \
	 ((policy) == (NXP_S32_XSPI_SECURE | NXP_S32_XSPI_PRIVILEGE)) || \
	 ((policy) == (NXP_S32_XSPI_NON_SECURE | NXP_S32_XSPI_PRIVILEGE)) || \
	 ((policy) == (NXP_S32_XSPI_NON_SECURE | NXP_S32_XSPI_SECURE | NXP_S32_XSPI_PRIVILEGE)))

#define _XSPI_VALIDATE_FRAD_ACP(node_id, n)						\
	BUILD_ASSERT(XSPI_ACP_VALID(DT_PROP(node_id, master_domain_acp_policy)),	\
		"Invalid master-domain-acp-policy: PRIVILEGE cannot be used alone");

#define XSPI_VALIDATE_FRAD_ACP(n)							\
	COND_CODE_1(DT_NODE_EXISTS(SFP_FRAD_NODE(n)),					\
		(DT_FOREACH_CHILD_STATUS_OKAY_VARGS(SFP_FRAD_NODE(n),			\
			_XSPI_VALIDATE_FRAD_ACP, n)), ())

#define _XSPI_SFP_FRAD_CFG(node_id, n)							\
	{										\
		.StartAddress = DT_REG_ADDR(node_id),					\
		.EndAddress = DT_REG_ADDR(node_id) + DT_REG_SIZE(node_id) - 1,		\
		.Valid = true,								\
		.MdAcp = {XSPI_ACP_POLICY(node_id), XSPI_ACP_POLICY(node_id)},		\
	},

#define XSPI_SFP_FRAD_CFG(n)								\
	.Frad = {									\
		DT_FOREACH_CHILD_STATUS_OKAY_VARGS(SFP_FRAD_NODE(n), _XSPI_SFP_FRAD_CFG, n)\
	},

#define XSPI_SFP_CFG(n)									\
	.SfpCfg = {									\
		XSPI_SFP_MDAD_CFG(n)							\
		XSPI_SFP_FRAD_CFG(n)							\
		.SfpEnable = true,							\
	},

#define MEMC_NXP_S32_XSPI_CONTROLLER_CONFIG(n)						\
	BUILD_ASSERT(DT_INST_PROP_LEN(n, ahb_buffers_masters) == XSPI_IP_AHB_BUFFERS,	\
		"ahb-buffers-masters must be of size XSPI_IP_AHB_BUFFERS");		\
	BUILD_ASSERT(DT_INST_PROP_LEN(n, ahb_buffers_sizes) == XSPI_IP_AHB_BUFFERS,	\
		"ahb-buffers-sizes must be of size XSPI_IP_AHB_BUFFERS");		\
	XSPI_VALIDATE_FRAD_ACP(n)							\
											\
	static const Xspi_Ip_ControllerConfigType					\
	memc_nxp_s32_xspi_controller_cfg_##n = {					\
		.csHoldTime = DT_INST_PROP(n, cs_hold_time),				\
		.csSetupTime = DT_INST_PROP(n, cs_setup_time),				\
		.sampleDelay = XSPI_SAMPLE_DELAY(n),					\
		.samplePhase = XSPI_SAMPLE_PHASE(n),					\
		.ahbConfig = XSPI_AHB_BUFFERS(n),					\
		XSPI_SIDE_CFG(n, a, A)							\
		XSPI_DATA_CFG(n)							\
		XSPI_ADDR_CFG(n)							\
		XSPI_BYTES_SWAP_ADDR(n)							\
		XSPI_DQS_OUTPUT_ENABLE(n)						\
		XSPI_DIFFERENTIAL_CLOCK_CFG(n)						\
		XSPI_ERROR_HANDLING_CFG(n)						\
		XSPI_TIMEOUT_CFG(n)							\
		XSPI_PRIORITY_CFG(n)							\
		XSPI_LOCK_CFG(n)							\
		XSPI_SFP_CFG(n)								\
	}

#define MEMC_NXP_S32_XSPI_INIT_DEVICE(n)						\
	PINCTRL_DT_INST_DEFINE(n);							\
	MEMC_NXP_S32_XSPI_CONTROLLER_CONFIG(n);						\
	static struct memc_nxp_s32_xspi_data memc_nxp_s32_xspi_data_##n;		\
	static const struct memc_nxp_s32_xspi_config memc_nxp_s32_xspi_config_##n = {	\
		.base = (XSPI_Type *)DT_INST_REG_ADDR(n),				\
		.controller_cfg = &memc_nxp_s32_xspi_controller_cfg_##n,		\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),				\
	};										\
	DEVICE_DT_INST_DEFINE(n,							\
		memc_nxp_s32_xspi_init,							\
		NULL,									\
		&memc_nxp_s32_xspi_data_##n,						\
		&memc_nxp_s32_xspi_config_##n,						\
		POST_KERNEL,								\
		CONFIG_MEMC_INIT_PRIORITY,						\
		NULL);

DT_INST_FOREACH_STATUS_OKAY(MEMC_NXP_S32_XSPI_INIT_DEVICE)
