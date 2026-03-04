/*
 * Copyright (c) 2026 Infineon Technologies AG,
 * or an affiliate of Infineon Technologies AG.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief MFD driver for Infineon AutAnalog subsystem.
 *
 * The AutAnalog (Autonomous Analog) subsystem contains multiple analog peripherals
 * (SAR ADC, DAC, comparators, regulators) coordinated by an
 * Autonomous Controller (AC). This MFD driver manages the shared resources:
 * subsystem initialization, the AC state machine configuration, and shared
 * interrupt dispatch to child peripheral drivers.
 */

#define DT_DRV_COMPAT infineon_autanalog

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <infineon_autanalog.h>

#include "cy_pdl.h"

LOG_MODULE_REGISTER(mfd_infineon_autanalog, CONFIG_MFD_LOG_LEVEL);

/* clang-format off */

/* Interrupt masks for each sub-peripheral within the AutAnalog subsystem */
#define IFX_AUTANALOG_SAR_ADC_INTR_MASK                                                          \
	(CY_AUTANALOG_INT_SAR0_DONE | CY_AUTANALOG_INT_SAR0_EOS | CY_AUTANALOG_INT_SAR0_RESULT | \
	 CY_AUTANALOG_INT_SAR0_RANGE0 | CY_AUTANALOG_INT_SAR0_RANGE1 |                           \
	 CY_AUTANALOG_INT_SAR0_RANGE2 | CY_AUTANALOG_INT_SAR0_RANGE3 |                           \
	 CY_AUTANALOG_INT_SAR0_FIR0_RESULT | CY_AUTANALOG_INT_SAR0_FIR1_RESULT)

#define IFX_AUTANALOG_DAC0_INTR_MASK                                                           \
	(CY_AUTANALOG_INT_DAC0_RANGE0 | CY_AUTANALOG_INT_DAC0_RANGE1 |                         \
	 CY_AUTANALOG_INT_DAC0_RANGE2 | CY_AUTANALOG_INT_DAC0_EPOCH | CY_AUTANALOG_INT_DAC0_EMPTY)

#define IFX_AUTANALOG_DAC1_INTR_MASK                                                           \
	(CY_AUTANALOG_INT_DAC1_RANGE0 | CY_AUTANALOG_INT_DAC1_RANGE1 |                         \
	 CY_AUTANALOG_INT_DAC1_RANGE2 | CY_AUTANALOG_INT_DAC1_EPOCH | CY_AUTANALOG_INT_DAC1_EMPTY)

#define IFX_AUTANALOG_CTB0_INTR_MASK (CY_AUTANALOG_INT_CTB0_COMP0 | CY_AUTANALOG_INT_CTB0_COMP1)

#define IFX_AUTANALOG_CTB1_INTR_MASK (CY_AUTANALOG_INT_CTB1_COMP0 | CY_AUTANALOG_INT_CTB1_COMP1)

#define IFX_AUTANALOG_PTCOMP0_INTR_MASK                                                        \
	(CY_AUTANALOG_INT_PTC0_COMP0 | CY_AUTANALOG_INT_PTC0_COMP1 |                           \
	 CY_AUTANALOG_INT_PTC0_RANGE0 | CY_AUTANALOG_INT_PTC0_RANGE1)

#define IFX_AUTANALOG_AC_INTR_MASK CY_AUTANALOG_INT_AC

/* clang-format on */

/** Interrupt mask lookup table indexed by peripheral type */
static const uint32_t ifx_autanalog_intr_masks[IFX_AUTANALOG_PERIPH_COUNT] = {
	[IFX_AUTANALOG_PERIPH_SAR_ADC] = IFX_AUTANALOG_SAR_ADC_INTR_MASK,
	[IFX_AUTANALOG_PERIPH_DAC0] = IFX_AUTANALOG_DAC0_INTR_MASK,
	[IFX_AUTANALOG_PERIPH_DAC1] = IFX_AUTANALOG_DAC1_INTR_MASK,
	[IFX_AUTANALOG_PERIPH_CTB0] = IFX_AUTANALOG_CTB0_INTR_MASK,
	[IFX_AUTANALOG_PERIPH_CTB1] = IFX_AUTANALOG_CTB1_INTR_MASK,
	[IFX_AUTANALOG_PERIPH_PTCOMP0] = IFX_AUTANALOG_PTCOMP0_INTR_MASK,
	[IFX_AUTANALOG_PERIPH_AC] = IFX_AUTANALOG_AC_INTR_MASK,
};

/*
 * Macros to build per-instance STT arrays from devicetree.
 *
 * Two modes are supported:
 *
 * Basic mode (no ac-states property):
 *   A hardcoded 3-state SAR single-shot sequence is generated.  SAR fields
 *   are populated at compile time when the SAR ADC child node is enabled.
 *
 * Advanced mode (ac-states property present):
 *   The full STT is assembled from the ac-states phandle list.  Each phandle
 *   references a node with compatible "infineon,autanalog-ac-state" whose
 *   properties define one AC state including SAR, CTB, PTCOMP and DAC fields.
 */

/** 1 when the ac-states property is present on instance n */
#define HAS_AC_STATES(n) DT_INST_NODE_HAS_PROP(n, ac_states)

/** 1 when the SAR ADC child node is enabled */
#define SAR_IS_USED(n) DT_NODE_HAS_STATUS(DT_CHILD(DT_DRV_INST(n), adc0_80000), okay)

/* clang-format off */

/* ===== Basic mode: hardcoded 3-state SAR single-shot STT ===== */

#define IFX_AUTANALOG_BASIC_NUM_STT 3

#define IFX_AUTANALOG_BASIC_STT(n)                                                               \
	static cy_stc_autanalog_stt_ac_t ifx_autanalog_ac_stt_##n[] = {                          \
		{                                                                                \
			.unlock = true,                                                          \
			.condition = CY_AUTANALOG_STT_AC_CONDITION_BLOCK_READY,                  \
			.action = CY_AUTANALOG_STT_AC_ACTION_WAIT_FOR,                           \
			.branchState = 1,                                                        \
		},                                                                               \
		{                                                                                \
			.condition = CY_AUTANALOG_STT_AC_CONDITION_SAR_DONE,                     \
			.action = CY_AUTANALOG_STT_AC_ACTION_WAIT_FOR,                           \
			.branchState = 0,                                                        \
		},                                                                               \
		{                                                                                \
			.condition = CY_AUTANALOG_STT_AC_CONDITION_FALSE,                        \
			.action = CY_AUTANALOG_STT_AC_ACTION_STOP,                               \
		},                                                                               \
	};                                                                                       \
	static cy_stc_autanalog_stt_sar_t ifx_autanalog_sar_stt_##n[] = {                        \
		{.unlock = SAR_IS_USED(n), .enable = SAR_IS_USED(n)},                            \
		{.unlock = SAR_IS_USED(n), .enable = SAR_IS_USED(n), .trigger = SAR_IS_USED(n)}, \
		{.unlock = SAR_IS_USED(n), .enable = SAR_IS_USED(n)},                            \
	};                                                                                       \
	static cy_stc_autanalog_stt_t ifx_autanalog_stt_##n[] = {                                \
		{.ac = &ifx_autanalog_ac_stt_##n[0], .sar = {&ifx_autanalog_sar_stt_##n[0]}},    \
		{.ac = &ifx_autanalog_ac_stt_##n[1], .sar = {&ifx_autanalog_sar_stt_##n[1]}},    \
		{.ac = &ifx_autanalog_ac_stt_##n[2], .sar = {&ifx_autanalog_sar_stt_##n[2]}},    \
	};

/* ===== Advanced mode: STT from DT ac-states phandle list ===== */

/** Get the DT node id of the i-th ac-state phandle for instance n */
#define AC_STATE_NODE(n, i) DT_INST_PHANDLE_BY_IDX(n, ac_states, i)

/** Number of AC states derived from the ac-states phandle list length */
#define NUM_STT_STATES(n) DT_INST_PROP_LEN(n, ac_states)

/** Build one cy_stc_autanalog_stt_ac_t initialiser from a state child node */
#define IFX_AUTANALOG_AC_STT_ENTRY(i, n)                                                          \
	{                                                                                         \
		.unlock = (bool)DT_PROP(AC_STATE_NODE(n, i), ac_unlock),                          \
		.lpMode = (bool)DT_PROP(AC_STATE_NODE(n, i), ac_lp_mode),                         \
		.condition = (cy_en_autanalog_stt_ac_condition_t)DT_PROP(AC_STATE_NODE(n, i),     \
									 ac_condition),           \
		.action =                                                                         \
			(cy_en_autanalog_stt_ac_action_t)DT_PROP(AC_STATE_NODE(n, i), ac_action), \
		.branchState = (uint8_t)DT_PROP(AC_STATE_NODE(n, i), branch_state),               \
		.trigInt = (bool)DT_PROP(AC_STATE_NODE(n, i), ac_trig_int),                       \
		.count = (uint16_t)DT_PROP(AC_STATE_NODE(n, i), ac_count),                        \
		.unlockGpioOut = (bool)DT_PROP(AC_STATE_NODE(n, i), ac_unlock_gpio_out),          \
		.gpioOut = (cy_en_autanalog_stt_ac_gpio_out_t)DT_PROP(AC_STATE_NODE(n, i),        \
								      ac_gpio_out),               \
	}

/** Build one cy_stc_autanalog_stt_sar_t initialiser from a state child node */
#define IFX_AUTANALOG_SAR_STT_ENTRY(i, n)                                                      \
	{                                                                                      \
		.unlock = (bool)DT_PROP(AC_STATE_NODE(n, i), sar_unlock),                      \
		.enable = (bool)DT_PROP(AC_STATE_NODE(n, i), sar_enable),                      \
		.trigger = (bool)DT_PROP(AC_STATE_NODE(n, i), sar_trigger),                    \
		.entryState = (uint8_t)DT_PROP(AC_STATE_NODE(n, i), sar_entry_state),          \
	}

/** Build one cy_stc_autanalog_stt_ctb_t initialiser for CTB0 */
#define IFX_AUTANALOG_CTB0_STT_ENTRY(i, n)                                                     \
	{                                                                                      \
		.unlock = (bool)DT_PROP(AC_STATE_NODE(n, i), ctb0_unlock),                     \
		.enableOpamp0 = (bool)DT_PROP(AC_STATE_NODE(n, i), ctb0_enable_opamp0),        \
		.cfgOpamp0 = (uint8_t)DT_PROP(AC_STATE_NODE(n, i), ctb0_cfg_opamp0),           \
		.gainOpamp0 = (cy_en_autanalog_stt_ctb_oa_gain_t)DT_PROP(AC_STATE_NODE(n, i),  \
									 ctb0_gain_opamp0),    \
		.enableOpamp1 = (bool)DT_PROP(AC_STATE_NODE(n, i), ctb0_enable_opamp1),        \
		.cfgOpamp1 = (uint8_t)DT_PROP(AC_STATE_NODE(n, i), ctb0_cfg_opamp1),           \
		.gainOpamp1 = (cy_en_autanalog_stt_ctb_oa_gain_t)DT_PROP(AC_STATE_NODE(n, i),  \
									 ctb0_gain_opamp1),    \
	}

/** Build one cy_stc_autanalog_stt_ctb_t initialiser for CTB1 */
#define IFX_AUTANALOG_CTB1_STT_ENTRY(i, n)                                                     \
	{                                                                                      \
		.unlock = (bool)DT_PROP(AC_STATE_NODE(n, i), ctb1_unlock),                     \
		.enableOpamp0 = (bool)DT_PROP(AC_STATE_NODE(n, i), ctb1_enable_opamp0),        \
		.cfgOpamp0 = (uint8_t)DT_PROP(AC_STATE_NODE(n, i), ctb1_cfg_opamp0),           \
		.gainOpamp0 = (cy_en_autanalog_stt_ctb_oa_gain_t)DT_PROP(AC_STATE_NODE(n, i),  \
									 ctb1_gain_opamp0),    \
		.enableOpamp1 = (bool)DT_PROP(AC_STATE_NODE(n, i), ctb1_enable_opamp1),        \
		.cfgOpamp1 = (uint8_t)DT_PROP(AC_STATE_NODE(n, i), ctb1_cfg_opamp1),           \
		.gainOpamp1 = (cy_en_autanalog_stt_ctb_oa_gain_t)DT_PROP(AC_STATE_NODE(n, i),  \
									 ctb1_gain_opamp1),    \
	}

/** Build one cy_stc_autanalog_stt_ptcomp_t initialiser for PTCOMP0 */
#define IFX_AUTANALOG_PTCOMP0_STT_ENTRY(i, n)                                                  \
	{                                                                                      \
		.unlock = (bool)DT_PROP(AC_STATE_NODE(n, i), ptcomp0_unlock),                  \
		.enableComp0 = (bool)DT_PROP(AC_STATE_NODE(n, i), ptcomp0_enable_comp0),       \
		.dynCfgIdxComp0 =                                                              \
			(uint8_t)DT_PROP(AC_STATE_NODE(n, i), ptcomp0_dyn_cfg_idx_comp0),      \
		.enableComp1 = (bool)DT_PROP(AC_STATE_NODE(n, i), ptcomp0_enable_comp1),       \
		.dynCfgIdxComp1 =                                                              \
			(uint8_t)DT_PROP(AC_STATE_NODE(n, i), ptcomp0_dyn_cfg_idx_comp1),      \
	}

/** Build one cy_stc_autanalog_stt_dac_t initialiser for DAC0 */
#define IFX_AUTANALOG_DAC0_STT_ENTRY(i, n)                                                     \
	{                                                                                      \
		.unlock = (bool)DT_PROP(AC_STATE_NODE(n, i), dac0_unlock),                     \
		.enable = (bool)DT_PROP(AC_STATE_NODE(n, i), dac0_enable),                     \
		.trigger = (bool)DT_PROP(AC_STATE_NODE(n, i), dac0_trigger),                   \
		.channel = (uint8_t)DT_PROP(AC_STATE_NODE(n, i), dac0_channel),                \
		.direction = (cy_en_autanalog_stt_dac_dir_t)DT_PROP(AC_STATE_NODE(n, i),       \
								    dac0_direction),           \
	}

/** Build one cy_stc_autanalog_stt_dac_t initialiser for DAC1 */
#define IFX_AUTANALOG_DAC1_STT_ENTRY(i, n)                                                     \
	{                                                                                      \
		.unlock = (bool)DT_PROP(AC_STATE_NODE(n, i), dac1_unlock),                     \
		.enable = (bool)DT_PROP(AC_STATE_NODE(n, i), dac1_enable),                     \
		.trigger = (bool)DT_PROP(AC_STATE_NODE(n, i), dac1_trigger),                   \
		.channel = (uint8_t)DT_PROP(AC_STATE_NODE(n, i), dac1_channel),                \
		.direction = (cy_en_autanalog_stt_dac_dir_t)DT_PROP(AC_STATE_NODE(n, i),       \
								    dac1_direction),           \
	}

/** Build one cy_stc_autanalog_stt_t (composite) entry linking all sub-peripherals */
#define IFX_AUTANALOG_STT_ENTRY(i, n)                                                          \
	{                                                                                      \
		.ac = &ifx_autanalog_ac_stt_##n[i],                                            \
		.prb = NULL,                                                                   \
		.ctb = {&ifx_autanalog_ctb0_stt_##n[i], &ifx_autanalog_ctb1_stt_##n[i]},       \
		.ptcomp = {&ifx_autanalog_ptcomp0_stt_##n[i]},                                 \
		.dac = {&ifx_autanalog_dac0_stt_##n[i], &ifx_autanalog_dac1_stt_##n[i]},       \
		.sar = {&ifx_autanalog_sar_stt_##n[i]},                                        \
	}

#define IFX_AUTANALOG_ADVANCED_STT(n)                                                          \
	static cy_stc_autanalog_stt_ac_t ifx_autanalog_ac_stt_##n[] = {                        \
		LISTIFY(NUM_STT_STATES(n), IFX_AUTANALOG_AC_STT_ENTRY, (,), n)};               \
	static cy_stc_autanalog_stt_sar_t ifx_autanalog_sar_stt_##n[] = {                      \
		LISTIFY(NUM_STT_STATES(n), IFX_AUTANALOG_SAR_STT_ENTRY, (,), n)};              \
	static cy_stc_autanalog_stt_ctb_t ifx_autanalog_ctb0_stt_##n[] = {                     \
		LISTIFY(NUM_STT_STATES(n), IFX_AUTANALOG_CTB0_STT_ENTRY, (,), n)};             \
	static cy_stc_autanalog_stt_ctb_t ifx_autanalog_ctb1_stt_##n[] = {                     \
		LISTIFY(NUM_STT_STATES(n), IFX_AUTANALOG_CTB1_STT_ENTRY, (,), n)};             \
	static cy_stc_autanalog_stt_ptcomp_t ifx_autanalog_ptcomp0_stt_##n[] = {               \
		LISTIFY(NUM_STT_STATES(n), IFX_AUTANALOG_PTCOMP0_STT_ENTRY, (,), n)};          \
	static cy_stc_autanalog_stt_dac_t ifx_autanalog_dac0_stt_##n[] = {                     \
		LISTIFY(NUM_STT_STATES(n), IFX_AUTANALOG_DAC0_STT_ENTRY, (,), n)};             \
	static cy_stc_autanalog_stt_dac_t ifx_autanalog_dac1_stt_##n[] = {                     \
		LISTIFY(NUM_STT_STATES(n), IFX_AUTANALOG_DAC1_STT_ENTRY, (,), n)};             \
	static cy_stc_autanalog_stt_t ifx_autanalog_stt_##n[] = {                              \
		LISTIFY(NUM_STT_STATES(n), IFX_AUTANALOG_STT_ENTRY, (,), n)};

/* Number of STT entries — basic or advanced */
#define IFX_AUTANALOG_NUM_STT(n)                                                               \
	COND_CODE_1(HAS_AC_STATES(n), (NUM_STT_STATES(n)), (IFX_AUTANALOG_BASIC_NUM_STT))

/* clang-format on */

/* Driver data and config structures */

struct ifx_autanalog_child {
	const struct device *dev;
	ifx_autanalog_child_isr_t isr;
};

struct ifx_autanalog_mfd_data {
	struct ifx_autanalog_child children[IFX_AUTANALOG_PERIPH_COUNT];
	uint32_t interrupt_mask;
};

struct ifx_autanalog_mfd_config {
	void (*irq_config_func)(const struct device *dev);
	cy_stc_autanalog_t *init_param;
};

/* Public API implementations */

void ifx_autanalog_set_irq_handler(const struct device *dev, const struct device *child_dev,
				   enum ifx_autanalog_periph periph,
				   ifx_autanalog_child_isr_t handler)
{
	struct ifx_autanalog_mfd_data *data = dev->data;

	if (periph >= IFX_AUTANALOG_PERIPH_COUNT) {
		LOG_ERR("Invalid peripheral type: %d", periph);
		return;
	}

	data->children[periph].dev = child_dev;
	data->children[periph].isr = handler;

	/* Enable the interrupt mask for this peripheral */
	data->interrupt_mask |= ifx_autanalog_intr_masks[periph];
	Cy_AutAnalog_SetInterruptMask(data->interrupt_mask);
}

int ifx_autanalog_start_autonomous_control(const struct device *dev)
{
	ARG_UNUSED(dev);

	Cy_AutAnalog_StartAutonomousControl();

	return 0;
}

int ifx_autanalog_pause_autonomous_control(const struct device *dev)
{
	ARG_UNUSED(dev);

	Cy_AutAnalog_PauseAutonomousControl();

	return 0;
}

/**
 * @brief Shared ISR for the AutAnalog subsystem
 *
 * Reads the interrupt cause register and dispatches to registered child handlers
 * based on the interrupt source.
 */
static void autanalog_mfd_isr(const struct device *dev)
{
	struct ifx_autanalog_mfd_data *data = dev->data;
	uint32_t int_source = Cy_AutAnalog_GetInterruptCause();

	for (int i = 0; i < IFX_AUTANALOG_PERIPH_COUNT; i++) {
		if ((int_source & ifx_autanalog_intr_masks[i]) != 0) {
			if (data->children[i].isr != NULL) {
				data->children[i].isr(data->children[i].dev);
			}
		}
	}

	Cy_AutAnalog_ClearInterrupt(int_source);
}

/**
 * @brief Initialize the AutAnalog MFD device
 *
 * Initializes the peripheral clock, the AutAnalog hardware block with the AC
 * configuration, and wires up the shared interrupt.
 */
static int ifx_autanalog_mfd_init(const struct device *dev)
{
	const struct ifx_autanalog_mfd_config *config = dev->config;
	struct ifx_autanalog_mfd_data *data = dev->data;
	cy_rslt_t result_val;

	memset(data, 0, sizeof(*data));

	/* Initialize the peripheral clock and power */
	Cy_SysClk_PeriGroupSlaveInit(CY_MMIO_PASS_PERI_NR, CY_MMIO_PASS_GROUP_NR,
				     CY_MMIO_PASS_SLAVE_NR, CY_MMIO_PASS_CLK_HF_NR);

	/* Initialize the AutAnalog block with AC configuration */
	result_val = Cy_AutAnalog_Init(config->init_param);
	if (result_val != CY_RSLT_SUCCESS) {
		LOG_ERR("Failed to initialize AutAnalog subsystem");
		return -EIO;
	}

	/* Configure and enable the shared interrupt */
	config->irq_config_func(dev);

	LOG_DBG("AutAnalog MFD initialized");
	return 0;
}

/**
 * Stub for the legacy ifx_autanalog_init() called from ifx_cycfg_init.c.
 * Initialization is handled by the MFD device model, so this is a no-op.
 */
int ifx_autanalog_init(void)
{
	return 0;
}

/* clang-format off */

/* Device instantiation — all PDL config structures are generated per-instance from DT */

#define IFX_AUTANALOG_MFD_INIT(n)                                                              \
                                                                                               \
	/* Per-instance STT arrays — basic or advanced mode */                                 \
	COND_CODE_1(HAS_AC_STATES(n),                                                          \
		    (IFX_AUTANALOG_ADVANCED_STT(n)),                                           \
		    (IFX_AUTANALOG_BASIC_STT(n)))                                              \
                                                                                               \
	/* Per-instance AC out-trigger masks (all empty — extensible later) */                 \
	static cy_en_autanalog_ac_out_trigger_mask_t ifx_autanalog_ac_out_trig_mask_##n[] = {  \
		CY_AUTANALOG_AC_OUT_TRIG_MASK_EMPTY, CY_AUTANALOG_AC_OUT_TRIG_MASK_EMPTY,      \
		CY_AUTANALOG_AC_OUT_TRIG_MASK_EMPTY, CY_AUTANALOG_AC_OUT_TRIG_MASK_EMPTY,      \
		CY_AUTANALOG_AC_OUT_TRIG_MASK_EMPTY, CY_AUTANALOG_AC_OUT_TRIG_MASK_EMPTY,      \
		CY_AUTANALOG_AC_OUT_TRIG_MASK_EMPTY, CY_AUTANALOG_AC_OUT_TRIG_MASK_EMPTY,      \
	};                                                                                     \
                                                                                               \
	/* Per-instance AC config */                                                           \
	static cy_stc_autanalog_ac_t ifx_autanalog_ac_cfg_##n = {                              \
		.gpioOutEn = CY_AUTANALOG_STT_AC_GPIO_OUT_DISABLED,                            \
		.mask =                                                                        \
			{                                                                      \
				&ifx_autanalog_ac_out_trig_mask_##n[0U],                       \
				&ifx_autanalog_ac_out_trig_mask_##n[1U],                       \
				&ifx_autanalog_ac_out_trig_mask_##n[2U],                       \
				&ifx_autanalog_ac_out_trig_mask_##n[3U],                       \
				&ifx_autanalog_ac_out_trig_mask_##n[4U],                       \
				&ifx_autanalog_ac_out_trig_mask_##n[5U],                       \
				&ifx_autanalog_ac_out_trig_mask_##n[6U],                       \
				&ifx_autanalog_ac_out_trig_mask_##n[7U],                       \
			},                                                                     \
		.timer =                                                                       \
			{                                                                      \
				.enable = false,                                               \
				.clkSrc = CY_AUTANALOG_TIMER_CLK_LP,                           \
				.period = 0U,                                                  \
			},                                                                     \
	};                                                                                     \
                                                                                               \
	/* Per-instance subsystem configuration */                                             \
	static cy_stc_autanalog_cfg_t ifx_autanalog_subsys_cfg_##n = {                         \
		.prb = NULL,                                                                   \
		.ac = &ifx_autanalog_ac_cfg_##n,                                               \
		.ctb = {NULL, NULL},                                                           \
		.ptcomp = {NULL},                                                              \
		.dac = {NULL, NULL},                                                           \
		.sar = {NULL},                                                                 \
	};                                                                                     \
                                                                                               \
	/* Per-instance init parameters for Cy_AutAnalog_Init() */                             \
	static cy_stc_autanalog_t ifx_autanalog_init_param_##n = {                             \
		.configuration = &ifx_autanalog_subsys_cfg_##n,                                \
		.numSttEntries = IFX_AUTANALOG_NUM_STT(n),                                     \
		.stateTransitionTable = &ifx_autanalog_stt_##n[0U],                            \
	};                                                                                     \
                                                                                               \
	static void ifx_autanalog_mfd_config_func_##n(const struct device *dev);               \
                                                                                               \
	static const struct ifx_autanalog_mfd_config ifx_autanalog_mfd_config_##n = {          \
		.irq_config_func = ifx_autanalog_mfd_config_func_##n,                          \
		.init_param = &ifx_autanalog_init_param_##n,                                   \
	};                                                                                     \
                                                                                               \
	static struct ifx_autanalog_mfd_data ifx_autanalog_mfd_data_##n;                       \
                                                                                               \
	DEVICE_DT_INST_DEFINE(n, &ifx_autanalog_mfd_init, NULL, &ifx_autanalog_mfd_data_##n,   \
			      &ifx_autanalog_mfd_config_##n, POST_KERNEL,                      \
			      CONFIG_MFD_INFINEON_AUTANALOG_INIT_PRIORITY, NULL);              \
                                                                                               \
	static void ifx_autanalog_mfd_config_func_##n(const struct device *dev)                \
	{                                                                                      \
		ARG_UNUSED(dev);                                                               \
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), autanalog_mfd_isr,      \
			    DEVICE_DT_INST_GET(n), 0);                                         \
		irq_enable(DT_INST_IRQN(n));                                                   \
	}

DT_INST_FOREACH_STATUS_OKAY(IFX_AUTANALOG_MFD_INIT)

/* clang-format on */
