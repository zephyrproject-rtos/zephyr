/*
 * Copyright (c) 2020 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _NUVOTON_NPCX_SOC_CLOCK_H_
#define _NUVOTON_NPCX_SOC_CLOCK_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/devicetree.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Common clock control device node for all NPCX series */
#define NPCX_CLK_CTRL_NODE DT_NODELABEL(pcc)

/**
 * @brief NPCX clock configuration structure
 *
 * Used to indicate the device's clock bus type and corresponding PWDWN_CTL
 * register/bit to turn on/off its source clock.
 */
struct npcx_clk_cfg {
	uint16_t bus: 8;
	uint16_t ctrl: 5;
	uint16_t bit: 3;
};

/* Clock settings from pcc node */
/* Target OFMCLK freq */
#define OFMCLK      DT_PROP(DT_NODELABEL(pcc), clock_frequency)
/* Core clock prescaler */
#define FPRED_VAL   (DT_PROP(DT_NODELABEL(pcc), core_prescaler) - 1)
/* APB1 clock divider */
#define APB1DIV_VAL (DT_PROP(DT_NODELABEL(pcc), apb1_prescaler) - 1)
/* APB2 clock divider */
#define APB2DIV_VAL (DT_PROP(DT_NODELABEL(pcc), apb2_prescaler) - 1)
/* APB3 clock divider */
#define APB3DIV_VAL (DT_PROP(DT_NODELABEL(pcc), apb3_prescaler) - 1)
/* APB4 clock divider if supported */
#if DT_NODE_HAS_PROP(DT_NODELABEL(pcc), apb4_prescaler)
#if defined(CONFIG_CLOCK_CONTROL_NPCX_SUPP_APB4) /* Supported in NPCX9 and later series */
#define APB4DIV_VAL (DT_PROP(DT_NODELABEL(pcc), apb4_prescaler) - 1)
#else
#error "APB4 clock divider is not supported but defined in pcc node!"
#endif /* CONFIG_CLOCK_CONTROL_NPCX_SUPP_APB4 */
#endif

/* Construct a uint8_t array from 'pwdwn-ctl-val' prop for PWDWN_CTL initialization. */
#define NPCX_PWDWN_CTL_ITEMS_INIT(node, prop, idx) DT_PROP_BY_IDX(node, prop, idx),
#define NPCX_PWDWN_CTL_INIT                                                                        \
	DT_FOREACH_PROP_ELEM(DT_NODELABEL(pcc), pwdwn_ctl_val, NPCX_PWDWN_CTL_ITEMS_INIT)

/*
 * NPCX7 and later series clock tree macros:
 * (Please refer Figure 58. for more information.)
 *
 * Maximum OFMCLK in npcx7/9 series is 100MHz,
 * Maximum OFMCLK in npcx4 series is 120MHz,
 *
 * Suggestion for npcx series:
 * - OFMCLK   > MAX_OFMCLK/2, XF_RANGE should be 1, else 0.
 * - CORE_CLK > MAX_OFMCLK/2, AHB6DIV should be 1, else 0.
 * - CORE_CLK > MAX_OFMCLK/2, FIUDIV should be 1, else 0.
 */
/* Core domain clock */
#define CORE_CLK (OFMCLK / DT_PROP(DT_NODELABEL(pcc), core_prescaler))
/* Low Frequency clock */
#define LFCLK    32768

/**
 * @brief Function to notify clock driver that backup the counter value of
 *        low-frequency timer before ec entered deep idle state.
 */
void npcx_clock_capture_low_freq_timer(void);

/**
 * @brief Function to notify clock driver that compensate the counter value of
 *        system timer by low-frequency timer after ec left deep idle state.
 *
 */
void npcx_clock_compensate_system_timer(void);

/**
 * @brief Function to get time ticks in system sleep/deep sleep state. The unit
 *        is ticks.
 *
 */
uint64_t npcx_clock_get_sleep_ticks(void);

/**
 * @brief Function to configure system sleep settings. After ec received "wfi"
 *        instruction, ec will enter sleep/deep sleep state for better power
 *        consumption.
 *
 * @param is_deep A boolean indicating ec enters deep sleep or sleep state
 * @param is_instant A boolean indicating 'Instant Wake-up' from deep idle is
 *                   enabled
 */
void npcx_clock_control_turn_on_system_sleep(bool is_deep, bool is_instant);

/**
 * @brief Function to turn off system sleep mode.
 */
void npcx_clock_control_turn_off_system_sleep(void);

#ifdef __cplusplus
}
#endif

#endif /* _NUVOTON_NPCX_SOC_CLOCK_H_ */
