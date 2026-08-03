/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_evtg

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/sys/util.h>

#include <fsl_evtg.h>

#define EVTG_NXP_AOI_COUNT 2U /* AOI blocks per instance (AOI0, AOI1). */
#define EVTG_NXP_PT_COUNT  4U /* Product terms per AOI block. */
#define EVTG_NXP_IN_COUNT  4U /* Inputs (A, B, C, D) per product term. */

/* Static configuration for a single EVTG instance, resolved from devicetree. */
struct evtg_nxp_instance {
	evtg_index_t index;
	evtg_flipflop_mode_t ff_mode;
	bool ff_init_enable;
	evtg_flipflop_init_output_t ff_init_value;
	bool input_sync[EVTG_NXP_IN_COUNT];
	/* [aoi][product term][input]; unset cells default to EVTG_FORCE0. */
	uint8_t aoi_term[EVTG_NXP_AOI_COUNT][EVTG_NXP_PT_COUNT][EVTG_NXP_IN_COUNT];
};

struct evtg_nxp_config {
	EVTG_Type *base;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	const struct evtg_nxp_instance *instances;
	uint8_t num_instances;
};

/* Copy an AOI product-term matrix into the fsl_evtg AOI config. */
static void evtg_nxp_apply_aoi(evtg_aoi_config_t *aoi,
			       const uint8_t term[EVTG_NXP_PT_COUNT][EVTG_NXP_IN_COUNT])
{
	evtg_aoi_product_term_config_t *pt[EVTG_NXP_PT_COUNT] = {
		&aoi->productTerm0,
		&aoi->productTerm1,
		&aoi->productTerm2,
		&aoi->productTerm3,
	};

	for (uint8_t t = 0; t < EVTG_NXP_PT_COUNT; t++) {
		pt[t]->aInput = (evtg_aoi_input_config_t)term[t][kEVTG_InputA];
		pt[t]->bInput = (evtg_aoi_input_config_t)term[t][kEVTG_InputB];
		pt[t]->cInput = (evtg_aoi_input_config_t)term[t][kEVTG_InputC];
		pt[t]->dInput = (evtg_aoi_input_config_t)term[t][kEVTG_InputD];
	}
}

static int evtg_nxp_init(const struct device *dev)
{
	const struct evtg_nxp_config *config = dev->config;
	int ret;

	if (!device_is_ready(config->clock_dev)) {
		return -ENODEV;
	}

	ret = clock_control_on(config->clock_dev, config->clock_subsys);
	if (ret < 0) {
		return ret;
	}

	for (uint8_t i = 0; i < config->num_instances; i++) {
		const struct evtg_nxp_instance *inst = &config->instances[i];
		evtg_config_t cfg = {0};

		/* Build config directly; EVTG_GetDefaultConfig() asserts on D-FF. */
		cfg.flipflopMode = inst->ff_mode;

		/* T/D/JK-FF and latch modes need input sync to stay glitch-free. */
		if (inst->ff_mode != kEVTG_FFModeBypass &&
		    inst->ff_mode != kEVTG_FFModeRSTrigger) {
			cfg.enableInputASync = true;
			cfg.enableInputBSync = true;
			cfg.enableInputCSync = true;
			cfg.enableInputDSync = true;
		}
		cfg.enableInputASync |= inst->input_sync[kEVTG_InputA];
		cfg.enableInputBSync |= inst->input_sync[kEVTG_InputB];
		cfg.enableInputCSync |= inst->input_sync[kEVTG_InputC];
		cfg.enableInputDSync |= inst->input_sync[kEVTG_InputD];

		cfg.enableFlipflopInitOutput = inst->ff_init_enable;
		cfg.flipflopInitOutputValue = inst->ff_init_value;

		evtg_nxp_apply_aoi(&cfg.aoi0Config, inst->aoi_term[kEVTG_AOI0]);
		evtg_nxp_apply_aoi(&cfg.aoi1Config, inst->aoi_term[kEVTG_AOI1]);

		EVTG_Init(config->base, inst->index, &cfg);
	}

	return 0;
}

/* Each supplied product-term property must list exactly four cells (A, B, C, D). */
#define EVTG_TERM_ASSERT(child, prop)                                                              \
	IF_ENABLED(DT_NODE_HAS_PROP(child, prop),                                                  \
		   (BUILD_ASSERT(DT_PROP_LEN(child, prop) == EVTG_NXP_IN_COUNT,                    \
				 "nxp,evtg " #prop " must list 4 cells (A, B, C, D)");))

#define EVTG_INSTANCE_ASSERT(child)                                                                \
	BUILD_ASSERT(DT_REG_ADDR(child) < EVTG_EVTG_INST_COUNT,                                    \
		     "nxp,evtg instance index out of range");                                      \
	EVTG_TERM_ASSERT(child, aoi0_term0)                                                        \
	EVTG_TERM_ASSERT(child, aoi0_term1)                                                        \
	EVTG_TERM_ASSERT(child, aoi0_term2)                                                        \
	EVTG_TERM_ASSERT(child, aoi0_term3)                                                        \
	EVTG_TERM_ASSERT(child, aoi1_term0)                                                        \
	EVTG_TERM_ASSERT(child, aoi1_term1)                                                        \
	EVTG_TERM_ASSERT(child, aoi1_term2)                                                        \
	EVTG_TERM_ASSERT(child, aoi1_term3)

/* One product term's cells, or all-EVTG_FORCE0 (disabled) when the property is absent. */
#define EVTG_TERM(child, prop) DT_PROP_OR(child, prop, {0})

/* Build one evtg_nxp_instance entry from a devicetree instance child node. */
#define EVTG_INSTANCE_ENTRY(child)                                                                 \
	{                                                                                          \
		.index = (evtg_index_t)DT_REG_ADDR(child),                                         \
		.ff_mode = (evtg_flipflop_mode_t)DT_ENUM_IDX(child, flip_flop_mode),               \
		.ff_init_enable = DT_NODE_HAS_PROP(child, flip_flop_init_output),                  \
		.ff_init_value =                                                                   \
			(evtg_flipflop_init_output_t)DT_PROP_OR(child, flip_flop_init_output, 0),  \
		.input_sync = {                                                                    \
			DT_PROP(child, input_a_sync),                                              \
			DT_PROP(child, input_b_sync),                                              \
			DT_PROP(child, input_c_sync),                                              \
			DT_PROP(child, input_d_sync),                                              \
		},                                                                                 \
		.aoi_term = {                                                                      \
			{                                                                          \
				EVTG_TERM(child, aoi0_term0),                                      \
				EVTG_TERM(child, aoi0_term1),                                      \
				EVTG_TERM(child, aoi0_term2),                                      \
				EVTG_TERM(child, aoi0_term3),                                      \
			},                                                                         \
			{                                                                          \
				EVTG_TERM(child, aoi1_term0),                                      \
				EVTG_TERM(child, aoi1_term1),                                      \
				EVTG_TERM(child, aoi1_term2),                                      \
				EVTG_TERM(child, aoi1_term3),                                      \
			},                                                                         \
		},                                                                                 \
	},

#define EVTG_NXP_INIT(n)                                                                           \
	BUILD_ASSERT(DT_INST_CHILD_NUM_STATUS_OKAY(n) > 0,                                         \
		     "nxp,evtg node requires at least one enabled instance child");                \
	DT_INST_FOREACH_CHILD_STATUS_OKAY(n, EVTG_INSTANCE_ASSERT)                                 \
	static const struct evtg_nxp_instance evtg_nxp_instances_##n[] = {                         \
		DT_INST_FOREACH_CHILD_STATUS_OKAY(n, EVTG_INSTANCE_ENTRY)                          \
	};                                                                                         \
	static const struct evtg_nxp_config evtg_nxp_config_##n = {                                \
		.base = (EVTG_Type *)DT_INST_REG_ADDR(n),                                          \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                \
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(n, name),              \
		.instances = evtg_nxp_instances_##n,                                               \
		.num_instances = ARRAY_SIZE(evtg_nxp_instances_##n),                               \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, evtg_nxp_init, NULL, NULL, &evtg_nxp_config_##n, POST_KERNEL,     \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE, NULL);

DT_INST_FOREACH_STATUS_OKAY(EVTG_NXP_INIT)
