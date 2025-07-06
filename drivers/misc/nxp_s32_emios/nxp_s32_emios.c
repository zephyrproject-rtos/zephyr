/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>

#define LOG_MODULE_NAME nxp_s32_emios
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME, CONFIG_NXP_S32_EMIOS_LOG_LEVEL);

#include <Emios_Mcl_Ip.h>
#include <Emios_Mcl_Ip_Irq.h>

#define DT_DRV_COMPAT	nxp_s32_emios

struct nxp_s32_emios_config {
	uint8_t instance;
	Emios_Mcl_Ip_ConfigType *mcl_info;
	void (*irq_config)(void);
};

static int nxp_s32_emios_init(const struct device *dev)
{
	const struct nxp_s32_emios_config *config = dev->config;

	if (Emios_Mcl_Ip_Init(config->instance, config->mcl_info)) {
		LOG_ERR("Could not initialize eMIOS");
		return -EINVAL;
	}

	config->irq_config();

	return 0;
}

#define MAX_MASTER_BUS_PERIOD	65535U
#define MIN_MASTER_BUS_PERIOD	2U
#define MAX_GLOB_PRESCALER	256U
#define MIN_GLOB_PRESCALER	1U

#define NXP_S32_EMIOS_MASTER_BUS_MODE(mode)	DT_CAT(EMIOS_IP_, mode)

#define NXP_S32_EMIOS_INSTANCE_CHECK(idx, n)							\
	((DT_INST_REG_ADDR(n) == IP_EMIOS_##idx##_BASE) ? idx : 0)

#define NXP_S32_EMIOS_GET_INSTANCE(n)								\
	LISTIFY(__DEBRACKET eMIOS_INSTANCE_COUNT, NXP_S32_EMIOS_INSTANCE_CHECK, (|), n)

#define NXP_S32_EMIOS_GENERATE_GLOBAL_CONFIG(n)							\
	BUILD_ASSERT(IN_RANGE(DT_INST_PROP(n, clock_divider),					\
			      MIN_GLOB_PRESCALER, MAX_GLOB_PRESCALER),				\
		     "Divider for eMIOS global prescaler is out of range");			\
	const Emios_Ip_GlobalConfigType nxp_s32_emios_##n##_global_config = {			\
		.allowDebugMode = true,								\
		.clkDivVal = DT_INST_PROP(n, clock_divider) - 1U,				\
		.enableGlobalTimeBase = true							\
	};

#define NXP_S32_EMIOS_MASTER_BUS_CONFIG(node_id)						\
	{											\
		.hwChannel = DT_PROP(node_id, channel),						\
		.defaultPeriod = MAX_MASTER_BUS_PERIOD,						\
		.masterBusPrescaler = DT_PROP(node_id, prescaler) - 1,				\
		.allowDebugMode = DT_PROP(node_id, freeze),					\
		.masterMode = NXP_S32_EMIOS_MASTER_BUS_MODE(DT_STRING_TOKEN(node_id, mode)),	\
		.masterBusAltPrescaler = 0,							\
	},

#define NXP_S32_EMIOS_GENERATE_MASTER_BUS_CONFIG(n)						\
	const Emios_Ip_MasterBusConfigType nxp_s32_emios_##n##_master_bus_config[] = {		\
		DT_FOREACH_CHILD_STATUS_OKAY(DT_INST_CHILD(n, master_bus),			\
					     NXP_S32_EMIOS_MASTER_BUS_CONFIG)			\
	};

#define NXP_S32_EMIOS_GENERATE_CONFIG(n)							\
	NXP_S32_EMIOS_GENERATE_GLOBAL_CONFIG(n)							\
	NXP_S32_EMIOS_GENERATE_MASTER_BUS_CONFIG(n)						\
	const Emios_Mcl_Ip_ConfigType nxp_s32_emios_##n##_mcl_config = {			\
		.channelsNumber = ARRAY_SIZE(nxp_s32_emios_##n##_master_bus_config),		\
		.emiosGlobalConfig = &nxp_s32_emios_##n##_global_config,			\
		.masterBusConfig = &nxp_s32_emios_##n##_master_bus_config			\
	};

#define EMIOS_INTERRUPT_NAME(name)	DT_CAT3(EMIOS, name, _IRQ)

/*
 * The real interrupt handlers only defined in some circumstances, just add
 * weak implementations to avoid populating so many preprocessor directives
 */
#define EMIOS_INTERRUPT_DEFINE(node_id, prop, idx)						\
	__weak void EMIOS_INTERRUPT_NAME(DT_STRING_TOKEN_BY_IDX(node_id, prop, idx))(void) {}

#define NXP_S32_EMIOS_INTERRUPT_DEFINE(n)							\
	DT_INST_FOREACH_PROP_ELEM(n, interrupt_names, EMIOS_INTERRUPT_DEFINE)

#define EMIOS_INTERRUPT_CONFIG(node_id, prop, idx)						\
	do {											\
		IRQ_CONNECT(DT_IRQ_BY_IDX(node_id, idx, irq),					\
			    DT_IRQ_BY_IDX(node_id, idx, priority),				\
			    EMIOS_INTERRUPT_NAME(DT_STRING_TOKEN_BY_IDX(node_id, prop, idx)),\
			    DEVICE_DT_GET(node_id),						\
			    0);									\
		irq_enable(DT_IRQ_BY_IDX(node_id, idx, irq));					\
	} while (false);

#define NXP_S32_EMIOS_INTERRUPT_CONFIG(n)							\
	static void nxp_s32_emios_##n##_interrupt_config(void)					\
	{											\
		DT_INST_FOREACH_PROP_ELEM(n, interrupt_names, EMIOS_INTERRUPT_CONFIG)		\
	}

#define NXP_S32_EMIOS_INIT_DEVICE(n)								\
	NXP_S32_EMIOS_GENERATE_CONFIG(n)							\
	NXP_S32_EMIOS_INTERRUPT_DEFINE(n)							\
	NXP_S32_EMIOS_INTERRUPT_CONFIG(n)							\
	const struct nxp_s32_emios_config nxp_s32_emios_##n##_config = {			\
		.instance = NXP_S32_EMIOS_GET_INSTANCE(n),					\
		.mcl_info = (Emios_Mcl_Ip_ConfigType *)&nxp_s32_emios_##n##_mcl_config,		\
		.irq_config = nxp_s32_emios_##n##_interrupt_config,				\
	};											\
	DEVICE_DT_INST_DEFINE(n,								\
			&nxp_s32_emios_init,							\
			NULL,									\
			NULL,									\
			&nxp_s32_emios_##n##_config,						\
			POST_KERNEL,								\
			CONFIG_NXP_S32_EMIOS_INIT_PRIORITY,					\
			NULL);

DT_INST_FOREACH_STATUS_OKAY(NXP_S32_EMIOS_INIT_DEVICE)
