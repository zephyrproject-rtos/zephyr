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
	uint16_t bus:8;
	uint16_t ctrl:5;
	uint16_t bit:3;
};

/* Clock settings from pcc node */
/* Target OFMCLK freq */
#define OFMCLK DT_PROP(DT_NODELABEL(pcc), clock_frequency)
/* Core clock prescaler */
#define FPRED_VAL (DT_PROP(DT_NODELABEL(pcc), core_prescaler) - 1)
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
#define NPCX_PWDWN_CTL_INIT DT_FOREACH_PROP_ELEM(DT_NODELABEL(pcc), \
				pwdwn_ctl_val, NPCX_PWDWN_CTL_ITEMS_INIT)

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
#define LFCLK 32768

/* FMUL clock */
#if (OFMCLK > (MAX_OFMCLK / 2))
#define FMCLK (OFMCLK / 2) /* FMUL clock = OFMCLK/2 */
#else
#define FMCLK OFMCLK /* FMUL clock = OFMCLK */
#endif

/* APBs source clock */
#define APBSRC_CLK OFMCLK

/* AHB6 clock */
#if (CORE_CLK > (MAX_OFMCLK / 2))
#define AHB6DIV_VAL 1 /* AHB6_CLK = CORE_CLK/2 */
#else
#define AHB6DIV_VAL 0 /* AHB6_CLK = CORE_CLK */
#endif

/* FIU clock divider */
#if (CORE_CLK > (MAX_OFMCLK / 2))
#define FIUDIV_VAL 1 /* FIU_CLK = CORE_CLK/2 */
#else
#define FIUDIV_VAL 0 /* FIU_CLK = CORE_CLK */
#endif

#if defined(CONFIG_CLOCK_CONTROL_NPCX_SUPP_FIU1)
#if (CORE_CLK > (MAX_OFMCLK / 2))
#define FIU1DIV_VAL 1 /* FIU1_CLK = CORE_CLK/2 */
#else
#define FIU1DIV_VAL 0 /* FIU1_CLK = CORE_CLK */
#endif
#endif /* CONFIG_CLOCK_CONTROL_NPCX_SUPP_FIU1 */

/* I3C clock divider */
#if (OFMCLK == MHZ(120)) /* MCLkD must between 40 mhz to 50 mhz*/
#define MCLKD_SL 2    /* I3C_CLK = (MCLK / 3) */
#elif (OFMCLK <= MHZ(100) && OFMCLK >= MHZ(80))
#define MCLKD_SL 1    /* I3C_CLK = (MCLK / 2) */
#else
#define MCLKD_SL 0    /* I3C_CLK = MCLK */
#endif


/* Get APB clock freq */
#define NPCX_APB_CLOCK(no) (APBSRC_CLK / (APB##no##DIV_VAL + 1))

/*
 * Frequency multiplier M/N value definitions according to the requested
 * OFMCLK (Unit:Hz).
 */
#if (OFMCLK > (MAX_OFMCLK / 2))
#define HFCGN_VAL    0x82 /* Set XF_RANGE as 1 */
#else
#define HFCGN_VAL    0x02
#endif
#if   (OFMCLK == 120000000)
#define HFCGMH_VAL   0x0E
#define HFCGML_VAL   0x4E
#elif (OFMCLK == 100000000)
#define HFCGMH_VAL   0x0B
#define HFCGML_VAL   0xEC
#elif (OFMCLK == 96000000)
#define HFCGMH_VAL   0x0B
#define HFCGML_VAL   0x72
#elif (OFMCLK == 90000000)
#define HFCGMH_VAL   0x0A
#define HFCGML_VAL   0xBA
#elif (OFMCLK == 80000000)
#define HFCGMH_VAL   0x09
#define HFCGML_VAL   0x89
#elif (OFMCLK == 66000000)
#define HFCGMH_VAL   0x07
#define HFCGML_VAL   0xDE
#elif (OFMCLK == 50000000)
#define HFCGMH_VAL   0x0B
#define HFCGML_VAL   0xEC
#elif (OFMCLK == 48000000)
#define HFCGMH_VAL   0x0B
#define HFCGML_VAL   0x72
#else
#error "Unsupported OFMCLK Frequency"
#endif

/* Clock prescaler configurations in different series */
#define VAL_HFCGP   ((FPRED_VAL << 4) | AHB6DIV_VAL)
#if defined(FIU1DIV_VAL)
#define VAL_HFCBCD  ((FIU1DIV_VAL << 4) | (FIUDIV_VAL << 2))
#else
#define VAL_HFCBCD  (FIUDIV_VAL << 4)
#endif /* FIU1DIV_VAL */
#define VAL_HFCBCD1 (APB1DIV_VAL | (APB2DIV_VAL << 4))
#if defined(APB4DIV_VAL)
#define VAL_HFCBCD2 (APB3DIV_VAL | (APB4DIV_VAL << 4))
#else
#define VAL_HFCBCD2 APB3DIV_VAL
#endif /* APB4DIV_VAL */
/* I3C1~I3C3 share the same configuration */
#define VAL_HFCBCD3 MCLKD_SL

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
