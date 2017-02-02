/*
 * Copyright (c) 2016, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the Intel Corporation nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL CORPORATION OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __REGISTERS_H__
#define __REGISTERS_H__

#include "qm_common.h"
#include "qm_interrupt_router_regs.h"
#include "qm_soc_interrupts.h"
#include "flash_layout.h"

/**
 * Quark D2000 SoC Registers.
 *
 * @defgroup groupQUARKD2000SEREG SoC Registers (D2000)
 * @{
 */

#define QUARK_D2000 (1)
#define HAS_MVIC (1)
#define HAS_SOC_CONTEXT_RETENTION (1)

/**
 * @name System Core
 * @{
 */

/** System Core register map. */
typedef struct {
	QM_RW uint32_t osc0_cfg0;  /**< Hybrid Oscillator Configuration 0. */
	QM_RW uint32_t osc0_stat1; /**< Hybrid Oscillator status 1. */
	QM_RW uint32_t osc0_cfg1;  /**< Hybrid Oscillator configuration 1. */
	QM_RW uint32_t osc1_stat0; /**< RTC Oscillator status 0. */
	QM_RW uint32_t osc1_cfg0;  /**< RTC Oscillator Configuration 0. */
	QM_RW uint32_t reserved;
	QM_RW uint32_t
	    ccu_periph_clk_gate_ctl; /**< Peripheral Clock Gate Control. */
	QM_RW uint32_t
	    ccu_periph_clk_div_ctl0; /**< Peripheral Clock Divider Control 0. */
	QM_RW uint32_t
	    ccu_gpio_db_clk_ctl; /**< Peripheral Clock Divider Control 1. */
	QM_RW uint32_t
	    ccu_ext_clock_ctl; /**< External Clock Control Register. */
	QM_RW uint32_t reserved1;
	QM_RW uint32_t ccu_lp_clk_ctl; /**< System Low Power Clock Control. */
	QM_RW uint32_t wake_mask;      /**< Wake Mask register. */
	QM_RW uint32_t ccu_mlayer_ahb_ctl; /**< AHB Control Register. */
	QM_RW uint32_t ccu_sys_clk_ctl; /**< System Clock Control Register. */
	QM_RW uint32_t osc_lock_0;      /**< Clocks Lock Register. */
	QM_RW uint32_t soc_ctrl;	/**< SoC Control Register. */
	QM_RW uint32_t soc_ctrl_lock;   /**< SoC Control Register Lock. */
} qm_scss_ccu_reg_t;

#if (UNIT_TEST)
qm_scss_ccu_reg_t test_scss_ccu;
#define QM_SCSS_CCU ((qm_scss_ccu_reg_t *)(&test_scss_ccu))

#else
#define QM_SCSS_CCU_BASE (0xB0800000)
#define QM_SCSS_CCU ((qm_scss_ccu_reg_t *)QM_SCSS_CCU_BASE)
#endif

/* The GPS0 register usage. */
#define QM_GPS0_BIT_FM (0) /**< Start Firmware Manager. */

/* Hybrid oscillator output select select (0=Silicon, 1=Crystal) */
#define QM_OSC0_MODE_SEL BIT(3)
#define QM_OSC0_PD BIT(2)
#define QM_OSC1_PD BIT(1)

/* Enable Crystal oscillator. */
#define QM_OSC0_EN_CRYSTAL BIT(0)

/* Crystal oscillator parameters. */
#define OSC0_CFG1_OSC0_FADJ_XTAL_MASK (0x000F0000)
#define OSC0_CFG1_OSC0_FADJ_XTAL_OFFS (16)
#define OSC0_CFG0_OSC0_XTAL_COUNT_VALUE_MASK (0x00600000)
#define OSC0_CFG0_OSC0_XTAL_COUNT_VALUE_OFFS (21)

/* Silicon Oscillator parameters. */
#define OSC0_CFG1_FTRIMOTP_MASK (0x3FF00000)
#define OSC0_CFG1_FTRIMOTP_OFFS (20)
#define OSC0_CFG1_SI_FREQ_SEL_MASK (0x00000300)
#define OSC0_CFG1_SI_FREQ_SEL_OFFS (8)

#define QM_OSC0_LOCK_SI BIT(0)
#define QM_OSC0_LOCK_XTAL BIT(1)
#define QM_OSC0_EN_SI_OSC BIT(1)

#define QM_SI_OSC_1V2_MODE BIT(0)

/* Peripheral clock divider control. */
#define QM_CCU_PERIPH_PCLK_DIV_OFFSET (1)
#define QM_CCU_PERIPH_PCLK_DIV_EN BIT(0)

/* System clock control. */
#define QM_CCU_SYS_CLK_SEL BIT(0)
#define QM_CCU_PERIPH_CLK_EN BIT(1)
#define QM_CCU_ADC_CLK_DIV_OFFSET (16)
#define QM_CCU_ADC_CLK_DIV_DEF_MASK (0xFC00FFFF)
#define QM_CCU_PERIPH_PCLK_DIV_DEF_MASK (0xFFFFFFF8)
#define QM_CCU_RTC_CLK_EN BIT(1)
#define QM_CCU_RTC_CLK_DIV_EN BIT(2)
#define QM_CCU_SYS_CLK_DIV_EN BIT(7)
#define QM_CCU_SYS_CLK_DIV_MASK (0x00000700)

#define QM_OSC0_SI_FREQ_SEL_DEF_MASK (0xFFFFFCFF)
#define QM_CCU_SYS_CLK_DIV_DEF_MASK (0xFFFFF47F)
#define QM_OSC0_SI_FREQ_SEL_4MHZ (3 >> 8)

#define QM_CCU_EXTERN_DIV_OFFSET (3)
#define QM_CCU_EXT_CLK_DIV_EN BIT(2)
#define QM_CCU_GPIO_DB_DIV_OFFSET (2)
#define QM_CCU_GPIO_DB_CLK_DIV_EN BIT(1)
#define QM_CCU_GPIO_DB_CLK_EN BIT(0)
#define QM_CCU_RTC_CLK_DIV_OFFSET (3)
#define QM_CCU_SYS_CLK_DIV_OFFSET (8)
#define QM_CCU_GPIO_DB_CLK_DIV_DEF_MASK (0xFFFFFFE1)
#define QM_CCU_EXT_CLK_DIV_DEF_MASK (0xFFFFFFE3)
#define QM_CCU_RTC_CLK_DIV_DEF_MASK (0xFFFFFF83)
#define QM_CCU_DMA_CLK_EN BIT(6)
#define QM_CCU_WAKE_MASK_RTC_BIT BIT(2)
#define QM_CCU_WAKE_MASK_GPIO_BIT BIT(15)
#define QM_CCU_WAKE_MASK_COMPARATOR_BIT BIT(14)
#define QM_CCU_WAKE_MASK_GPIO_BIT BIT(15)

#define QM_HYB_OSC_PD_LATCH_EN BIT(14)
#define QM_RTC_OSC_PD_LATCH_EN BIT(15)
#define QM_CCU_EXIT_TO_HYBOSC BIT(4)
#define QM_CCU_MEM_HALT_EN BIT(3)
#define QM_CCU_CPU_HALT_EN BIT(2)

#define QM_WAKE_PROBE_MODE_MASK BIT(13)

/** @} */

/**
 * @name General Purpose
 * @{
 */

/** General Purpose register map. */
typedef struct {
	QM_RW uint32_t gps0; /**< General Purpose Sticky Register 0. */
	QM_RW uint32_t gps1; /**< General Purpose Sticky Register 1. */
	QM_RW uint32_t gps2; /**< General Purpose Sticky Register 2. */
	QM_RW uint32_t gps3; /**< General Purpose Sticky Register 3. */
	QM_RW uint32_t reserved;
	QM_RW uint32_t gp0; /**< General Purpose Scratchpad Register 0. */
	QM_RW uint32_t gp1; /**< General Purpose Scratchpad Register 1. */
	QM_RW uint32_t gp2; /**< General Purpose Scratchpad Register 2. */
	QM_RW uint32_t gp3; /**< General Purpose Scratchpad Register 3. */
	QM_RW uint32_t reserved1[3];
	QM_RW uint32_t wo_sp; /**< Write-One-to-Set Scratchpad Register. */
	QM_RW uint32_t
	    wo_st; /**< Write-One-to-Set Sticky Scratchpad Register. */
} qm_scss_gp_reg_t;

#if (UNIT_TEST)
qm_scss_gp_reg_t test_scss_gp;
#define QM_SCSS_GP ((qm_scss_gp_reg_t *)(&test_scss_gp))

#else
#define QM_SCSS_GP_BASE (0xB0800100)
#define QM_SCSS_GP ((qm_scss_gp_reg_t *)QM_SCSS_GP_BASE)
#endif

#define QM_GPS0_POWER_STATES_MASK (BIT(6) | BIT(7) | BIT(8) | BIT(9))
#define QM_GPS0_POWER_STATE_SLEEP BIT(6)
#define QM_GPS0_POWER_STATE_DEEP_SLEEP BIT(7)

/** @} */

/**
 * @name Comparator
 * @{
 */

/** Comparator register map. */
typedef struct {
	QM_RW uint32_t cmp_en;      /**< Comparator enable. */
	QM_RW uint32_t cmp_ref_sel; /**< Comparator reference select. */
	QM_RW uint32_t
	    cmp_ref_pol; /**< Comparator reference polarity select register. */
	QM_RW uint32_t cmp_pwr; /**< Comparator power enable register. */
	QM_RW uint32_t reserved[6];
	QM_RW uint32_t cmp_stat_clr; /**< Comparator clear register. */
} qm_scss_cmp_reg_t;

#if (UNIT_TEST)
qm_scss_cmp_reg_t test_scss_cmp;
#define QM_SCSS_CMP ((qm_scss_cmp_reg_t *)(&test_scss_cmp))

#else
#define QM_SCSS_CMP_BASE (0xB0800300)
#define QM_SCSS_CMP ((qm_scss_cmp_reg_t *)QM_SCSS_CMP_BASE)
#endif

#define QM_AC_HP_COMPARATORS_MASK (0x7FFC0)

/** @} */

/**
 * @name Power Management
 * @{
 */

/** Power Management register map. */
typedef struct {
	QM_RW uint32_t aon_vr; /**< AON Voltage Regulator. */
	QM_RW uint32_t reserved[5];
	QM_RW uint32_t pm_wait; /**< Power Management Wait. */
	QM_RW uint32_t reserved1;
	QM_RW uint32_t p_sts; /**< Processor Status. */
	QM_RW uint32_t reserved2[3];
	QM_RW uint32_t rstc; /**< Reset Control. */
	QM_RW uint32_t rsts; /**< Reset Status. */
	QM_RW uint32_t reserved3[7];
	QM_RW uint32_t pm_lock; /**< Power Management Lock. */
} qm_scss_pmu_reg_t;

#if (UNIT_TEST)
qm_scss_pmu_reg_t test_scss_pmu;
#define QM_SCSS_PMU ((qm_scss_pmu_reg_t *)(&test_scss_pmu))

#else
#define QM_SCSS_PMU_BASE (0xB0800540)
#define QM_SCSS_PMU ((qm_scss_pmu_reg_t *)QM_SCSS_PMU_BASE)
#endif

#define QM_P_STS_HALT_INTERRUPT_REDIRECTION BIT(26)

/*
 * Rename ROK_BUF_VREG register bit to ROK_BUF_VREG_STATUS to avoid confusion
 * and preprocessor issues with ROK_BUF_VREG_MASK register bit.
 */
#define QM_AON_VR_ROK_BUF_VREG_STATUS BIT(15)
#define QM_AON_VR_ROK_BUF_VREG_MASK BIT(9)
#define QM_AON_VR_VREG_SEL BIT(8)
#define QM_AON_VR_PASS_CODE (0x9DC4 << 16)
#define QM_AON_VR_VSEL_MASK (0xFFE0)
#define QM_AON_VR_VSEL_1V35 (0xB)
#define QM_AON_VR_VSEL_1V8 (0x10)
#define QM_AON_VR_VSTRB BIT(5)

/** @} */

/**
 * @name Always-on Counters.
 * @{
 */

/** Number of Always-on counter controllers. */
typedef enum { QM_AONC_0 = 0, QM_AONC_NUM } qm_aonc_t;

/** Always-on Counter Controller register map. */
typedef struct {
	QM_RW uint32_t aonc_cnt;  /**< Always-on counter register. */
	QM_RW uint32_t aonc_cfg;  /**< Always-on counter enable. */
	QM_RW uint32_t aonpt_cnt; /**< Always-on periodic timer. */
	QM_RW uint32_t
	    aonpt_stat; /**< Always-on periodic timer status register. */
	QM_RW uint32_t aonpt_ctrl; /**< Always-on periodic timer control. */
	QM_RW uint32_t
	    aonpt_cfg; /**< Always-on periodic timer configuration register. */
} qm_aonc_reg_t;

#define qm_aonc_context_t uint8_t

#define HAS_AONPT_BUSY_BIT (0)

#define QM_AONC_ENABLE (BIT(0))
#define QM_AONC_DISABLE (~QM_AONC_ENABLE)

#define QM_AONPT_INTERRUPT (BIT(0))

#define QM_AONPT_CLR (BIT(0))
#define QM_AONPT_RST (BIT(1))

#if (UNIT_TEST)
qm_aonc_reg_t test_aonc_instance[QM_AONC_NUM];
qm_aonc_reg_t *test_aonc[QM_AONC_NUM];

#define QM_AONC test_aonc

#else
extern qm_aonc_reg_t *qm_aonc[QM_AONC_NUM];
#define QM_AONC_0_BASE (0xB0800700)
#define QM_AONC qm_aonc
#endif

/** @} */

/**
 * @name Peripheral Registers
 * @{
 */

/** Peripheral Registers register map. */
typedef struct {
	QM_RW uint32_t periph_cfg0; /**< Peripheral Configuration. */
	QM_RW uint32_t reserved[2];
	QM_RW uint32_t cfg_lock; /**< Configuration Lock. */
} qm_scss_peripheral_reg_t;

#if (UNIT_TEST)
qm_scss_peripheral_reg_t test_scss_peripheral;
#define QM_SCSS_PERIPHERAL ((qm_scss_peripheral_reg_t *)(&test_scss_peripheral))

#else
#define QM_SCSS_PERIPHERAL_BASE (0xB0800804)
#define QM_SCSS_PERIPHERAL ((qm_scss_peripheral_reg_t *)QM_SCSS_PERIPHERAL_BASE)
#endif

/** @} */

/**
 * @name Pin MUX
 * @{
 */

/** Pin MUX register map. */
typedef struct {
	QM_RW uint32_t pmux_pullup[1]; /**< Pin Mux Pullup. */
	QM_RW uint32_t reserved[3];
	QM_RW uint32_t pmux_slew[1]; /**< Pin Mux Slew Rate. */
	QM_RW uint32_t reserved1[3];
	QM_RW uint32_t pmux_in_en[1]; /**< Pin Mux Input Enable. */
	QM_RW uint32_t reserved2[3];
	QM_RW uint32_t pmux_sel[2]; /**< Pin Mux Select. */
	QM_RW uint32_t reserved3[5];
	QM_RW uint32_t pmux_pullup_lock; /**< Pin Mux Pullup Lock. */
	QM_RW uint32_t pmux_slew_lock;   /**< Pin Mux Slew Rate Lock. */
	QM_RW uint32_t pmux_sel_0_lock;  /**< Pin Mux Select Lock 0. */
	QM_RW uint32_t reserved4[2];
	QM_RW uint32_t pmux_in_en_lock; /**< Pin Mux Slew Rate Lock. */
} qm_scss_pmux_reg_t;

#if (UNIT_TEST)
qm_scss_pmux_reg_t test_scss_pmux;
#define QM_SCSS_PMUX ((qm_scss_pmux_reg_t *)(&test_scss_pmux))

#else
#define QM_SCSS_PMUX_BASE (0xB0800900)
#define QM_SCSS_PMUX ((qm_scss_pmux_reg_t *)QM_SCSS_PMUX_BASE)
#endif

/** @} */

/**
 * @name ID
 * @{
 */

/** Information register map. */
typedef struct {
	QM_RW uint32_t id;    /**< Identification Register. */
	QM_RW uint32_t rev;   /**< Revision Register. */
	QM_RW uint32_t fs;    /**< Flash Size Register. */
	QM_RW uint32_t rs;    /**< RAM Size Register. */
	QM_RW uint32_t cotps; /**< Code OTP Size Register. */
	QM_RW uint32_t dotps; /**< Data OTP Size Register. */
} qm_scss_info_reg_t;

#if (UNIT_TEST)
qm_scss_info_reg_t test_scss_info;
#define QM_SCSS_INFO ((qm_scss_info_reg_t *)(&test_scss_info))

#else
#define QM_SCSS_INFO_BASE (0xB0801000)
#define QM_SCSS_INFO ((qm_scss_info_reg_t *)QM_SCSS_INFO_BASE)
#endif

/** @} */

/**
 * @name PWM / Timer
 * @{
 */

/** Number of PWM / Timer controllers. */
typedef enum { QM_PWM_0 = 0, QM_PWM_NUM } qm_pwm_t;

/** PWM ID type. */
typedef enum { QM_PWM_ID_0 = 0, QM_PWM_ID_1, QM_PWM_ID_NUM } qm_pwm_id_t;

/** PWM / Timer channel register map. */
typedef struct {
	QM_RW uint32_t loadcount;    /**< Load Coun.t */
	QM_RW uint32_t currentvalue; /**< Current Value. */
	QM_RW uint32_t controlreg;   /**< Control. */
	QM_RW uint32_t eoi;	  /**< End Of Interrupt. */
	QM_RW uint32_t intstatus;    /**< Interrupt Status. */
} qm_pwm_channel_t;

/** PWM / Timer register map. */
typedef struct {
	qm_pwm_channel_t timer[QM_PWM_ID_NUM]; /**< 2 Timers. */
	QM_RW uint32_t reserved[30];
	QM_RW uint32_t timersintstatus;    /**< Timers Interrupt Status */
	QM_RW uint32_t timerseoi;	  /**< Timers End Of Interrupt */
	QM_RW uint32_t timersrawintstatus; /**< Timers Raw Interrupt Status */
	QM_RW uint32_t timerscompversion;  /**< Timers Component Version */
	QM_RW uint32_t
	    timer_loadcount2[QM_PWM_ID_NUM]; /**< Timer Load Count 2 */
} qm_pwm_reg_t;

#define qm_pwm_context_t uint8_t

#if (UNIT_TEST)
qm_pwm_reg_t test_pwm_instance[QM_PWM_NUM];
qm_pwm_reg_t *test_pwm[QM_PWM_NUM];
#define QM_PWM test_pwm

#else
extern qm_pwm_reg_t *qm_pwm[QM_PWM_NUM];
/* PWM register base address. */
#define QM_PWM_BASE (0xB0000800)
/* PWM register block. */
#define QM_PWM qm_pwm
#endif

#define PWM_START (1)

#define QM_PWM_CONF_MODE_MASK (0xA)
#define QM_PWM_CONF_INT_EN_MASK (0x4)

#define QM_PWM_INTERRUPT_MASK_OFFSET (0x2)

#define NUM_PWM_CONTROLLER_INTERRUPTS (1)

/**
 * Timer N Control (TimerNControlReg)
 *
 * 31:4  RO  reserved
 * 3     RW  Timer PWM
 *               1 - PWM Mode
 *               0 - Timer Mode
 * 2     RW  Timer Interrupt Mask, set to 1b to mask interrupt.
 * 1     RW  Timer Mode
 *               1 - user-defined count mode
 *               0 - free-running mode
 * 0     RW  Timer Enable
 *               0 - Disable PWM/Timer
 *               1 - Enable PWM/Timer
 */

#define QM_PWM_TIMERNCONTROLREG_TIMER_ENABLE (BIT(0))
#define QM_PWM_TIMERNCONTROLREG_TIMER_MODE (BIT(1))
#define QM_PWM_TIMERNCONTROLREG_TIMER_INTERRUPT_MASK (BIT(2))
#define QM_PWM_TIMERNCONTROLREG_TIMER_PWM (BIT(3))

#define QM_PWM_MODE_TIMER_FREE_RUNNING_VALUE (0)
#define QM_PWM_MODE_TIMER_COUNT_VALUE (QM_PWM_TIMERNCONTROLREG_TIMER_MODE)
#define QM_PWM_MODE_PWM_VALUE                                                  \
	(QM_PWM_TIMERNCONTROLREG_TIMER_PWM | QM_PWM_TIMERNCONTROLREG_TIMER_MODE)

/** @} */

/**
 * @name WDT
 * @{
 */

/** Number of WDT controllers. */
typedef enum { QM_WDT_0 = 0, QM_WDT_NUM } qm_wdt_t;

/** Watchdog timer register map. */
typedef struct {
	QM_RW uint32_t wdt_cr;		 /**< Control Register. */
	QM_RW uint32_t wdt_torr;	 /**< Timeout Range Register. */
	QM_RW uint32_t wdt_ccvr;	 /**< Current Counter Value Register. */
	QM_RW uint32_t wdt_crr;		 /**< Current Restart Register. */
	QM_RW uint32_t wdt_stat;	 /**< Interrupt Status Register. */
	QM_RW uint32_t wdt_eoi;		 /**< Interrupt Clear Register. */
	QM_RW uint32_t wdt_comp_param_5; /**<  Component Parameters. */
	QM_RW uint32_t wdt_comp_param_4; /**<  Component Parameters. */
	QM_RW uint32_t wdt_comp_param_3; /**<  Component Parameters. */
	QM_RW uint32_t wdt_comp_param_2; /**<  Component Parameters. */
	QM_RW uint32_t
	    wdt_comp_param_1; /**<  Component Parameters Register 1. */
	QM_RW uint32_t wdt_comp_version; /**<  Component Version Register. */
	QM_RW uint32_t wdt_comp_type;    /**< Component Type Register. */
} qm_wdt_reg_t;

#define qm_wdt_context_t uint8_t

#if (UNIT_TEST)
qm_wdt_reg_t test_wdt_instance[QM_WDT_NUM];
qm_wdt_reg_t *test_wdt[QM_WDT_NUM];
#define QM_WDT test_wdt

#else
extern qm_wdt_reg_t *qm_wdt[QM_WDT_NUM];
/* WDT register base address. */
#define QM_WDT_0_BASE (0xB0000000)

/* WDT register block. */
#define QM_WDT qm_wdt
#endif

/* Watchdog enable. */
#define QM_WDT_CR_WDT_ENABLE (BIT(0))
/* Watchdog mode. */
#define QM_WDT_CR_RMOD (BIT(1))
/* Watchdog mode offset. */
#define QM_WDT_CR_RMOD_OFFSET (1)
/* Watchdog Timeout Mask. */
#define QM_WDT_TORR_TOP_MASK (0xF)
/* Watchdog reload special value. */
#define QM_WDT_RELOAD_VALUE (0x76)
/* Number of WDT controllers. */
#define NUM_WDT_CONTROLLERS (1)
/* Watchdog does not have pause enable. */
#define HAS_WDT_PAUSE (0)
/* Software SoC watch required. */
#define HAS_SW_SOCWATCH (1)
/* Peripheral WDT clock enable mask. */
#define QM_WDT_CLOCK_EN_MASK (BIT(10))
/* Required to enable WDT clock on start. */
#define HAS_WDT_CLOCK_ENABLE (1)

/**
 * WDT timeout table (in clock cycles):
 * Each table entry corresponds with the value loaded
 * into the WDT at the time of a WDT reload for the
 * corresponding timeout range register value.
 *
 * TORR | Timeout (Clock Cycles)
 * 0.   | 2^16 (65536)
 * 1.   | 2^17 (131072)
 * 2.   | 2^18 (262144)
 * 3.   | 2^19 (524288)
 * 4.   | 2^20 (1048576)
 * 5.   | 2^21 (2097152)
 * 6.   | 2^22 (4194304)
 * 7.   | 2^23 (8388608)
 * 8.   | 2^24 (16777216)
 * 9.   | 2^25 (33554432)
 * 10.  | 2^26 (67108864)
 * 11.  | 2^27 (134217728)
 * 12.  | 2^28 (268435456)
 * 13.  | 2^29 (536870912)
 * 14.  | 2^30 (1073741824)
 * 15.  | 2^31 (2147483648)
 */

/** @} */

/**
 * @name UART
 * @{
 */

/* Break character Bit. */
#define QM_UART_LCR_BREAK BIT(6)
/* Divisor Latch Access Bit. */
#define QM_UART_LCR_DLAB BIT(7)

/* Request to Send Bit. */
#define QM_UART_MCR_RTS BIT(1)
/* Loopback Enable Bit. */
#define QM_UART_MCR_LOOPBACK BIT(4)
/* Auto Flow Control Enable Bit. */
#define QM_UART_MCR_AFCE BIT(5)

/* FIFO Enable Bit. */
#define QM_UART_FCR_FIFOE BIT(0)
/* Reset Receive FIFO. */
#define QM_UART_FCR_RFIFOR BIT(1)
/* Reset Transmit FIFO. */
#define QM_UART_FCR_XFIFOR BIT(2)

/* Default FIFO RX & TX Thresholds, half full for both. */
#define QM_UART_FCR_DEFAULT_TX_RX_THRESHOLD (0xB0)
/* Change TX Threshold to empty, keep RX Threshold to default. */
#define QM_UART_FCR_TX_0_RX_1_2_THRESHOLD (0x80)

/* Transmit Holding Register Empty. */
#define QM_UART_IIR_THR_EMPTY (0x02)
/* Received Data Available. */
#define QM_UART_IIR_RECV_DATA_AVAIL (0x04)
/* Receiver Line Status. */
#define QM_UART_IIR_RECV_LINE_STATUS (0x06)
/* Character Timeout. */
#define QM_UART_IIR_CHAR_TIMEOUT (0x0C)
/* Interrupt ID Mask. */
#define QM_UART_IIR_IID_MASK (0x0F)

/* Data Ready Bit. */
#define QM_UART_LSR_DR BIT(0)
/* Overflow Error Bit. */
#define QM_UART_LSR_OE BIT(1)
/* Parity Error Bit. */
#define QM_UART_LSR_PE BIT(2)
/* Framing Error Bit. */
#define QM_UART_LSR_FE BIT(3)
/* Break Interrupt Bit. */
#define QM_UART_LSR_BI BIT(4)
/* Transmit Holding Register Empty Bit. */
#define QM_UART_LSR_THRE BIT(5)
/* Transmitter Empty Bit. */
#define QM_UART_LSR_TEMT BIT(6)
/* Receiver FIFO Error Bit. */
#define QM_UART_LSR_RFE BIT(7)

/* Enable Received Data Available Interrupt. */
#define QM_UART_IER_ERBFI BIT(0)
/* Enable Transmit Holding Register Empty Interrupt. */
#define QM_UART_IER_ETBEI BIT(1)
/* Enable Receiver Line Status Interrupt. */
#define QM_UART_IER_ELSI BIT(2)
/* Programmable THRE Interrupt Mode. */
#define QM_UART_IER_PTIME BIT(7)

/* Line Status Errors. */
#define QM_UART_LSR_ERROR_BITS                                                 \
	(QM_UART_LSR_OE | QM_UART_LSR_PE | QM_UART_LSR_FE | QM_UART_LSR_BI)

/* FIFO Depth. */
#define QM_UART_FIFO_DEPTH (16)
/* FIFO Half Depth. */
#define QM_UART_FIFO_HALF_DEPTH (QM_UART_FIFO_DEPTH / 2)

/* Divisor Latch High Offset. */
#define QM_UART_CFG_BAUD_DLH_OFFS 16
/* Divisor Latch Low Offset. */
#define QM_UART_CFG_BAUD_DLL_OFFS 8
/* Divisor Latch Fraction Offset. */
#define QM_UART_CFG_BAUD_DLF_OFFS 0
/* Divisor Latch High Mask. */
#define QM_UART_CFG_BAUD_DLH_MASK (0xFF << QM_UART_CFG_BAUD_DLH_OFFS)
/* Divisor Latch Low Mask. */
#define QM_UART_CFG_BAUD_DLL_MASK (0xFF << QM_UART_CFG_BAUD_DLL_OFFS)
/* Divisor Latch Fraction Mask. */
#define QM_UART_CFG_BAUD_DLF_MASK (0xFF << QM_UART_CFG_BAUD_DLF_OFFS)

/* Divisor Latch Packing Helper. */
#define QM_UART_CFG_BAUD_DL_PACK(dlh, dll, dlf)                                \
	(dlh << QM_UART_CFG_BAUD_DLH_OFFS | dll << QM_UART_CFG_BAUD_DLL_OFFS | \
	 dlf << QM_UART_CFG_BAUD_DLF_OFFS)

/* Divisor Latch High Unpacking Helper. */
#define QM_UART_CFG_BAUD_DLH_UNPACK(packed)                                    \
	((packed & QM_UART_CFG_BAUD_DLH_MASK) >> QM_UART_CFG_BAUD_DLH_OFFS)
/* Divisor Latch Low Unpacking Helper. */
#define QM_UART_CFG_BAUD_DLL_UNPACK(packed)                                    \
	((packed & QM_UART_CFG_BAUD_DLL_MASK) >> QM_UART_CFG_BAUD_DLL_OFFS)
/* Divisor Latch Fraction Unpacking Helper. */
#define QM_UART_CFG_BAUD_DLF_UNPACK(packed)                                    \
	((packed & QM_UART_CFG_BAUD_DLF_MASK) >> QM_UART_CFG_BAUD_DLF_OFFS)

/** Number of UART controllers. */
typedef enum { QM_UART_0 = 0, QM_UART_1, QM_UART_NUM } qm_uart_t;

/** UART register map. */
typedef struct {
	QM_RW uint32_t
	    rbr_thr_dll;	/**< Rx Buffer/ Tx Holding/ Div Latch Low. */
	QM_RW uint32_t ier_dlh; /**< Interrupt Enable / Divisor Latch High. */
	QM_RW uint32_t iir_fcr; /**< Interrupt Identification / FIFO Control. */
	QM_RW uint32_t lcr;     /**< Line Control. */
	QM_RW uint32_t mcr;     /**< MODEM Control. */
	QM_RW uint32_t lsr;     /**< Line Status. */
	QM_RW uint32_t msr;     /**< MODEM Status. */
	QM_RW uint32_t scr;     /**< Scratchpad. */
	QM_RW uint32_t reserved[23];
	QM_RW uint32_t usr; /**< UART Status. */
	QM_RW uint32_t reserved1[9];
	QM_RW uint32_t htx;     /**< Halt Transmission. */
	QM_RW uint32_t dmasa;   /**< DMA Software Acknowledge. */
	QM_RW uint32_t tcr;     /**< Transceiver Control Register. */
	QM_RW uint32_t de_en;   /**< Driver Output Enable Register. */
	QM_RW uint32_t re_en;   /**< Receiver Output Enable Register. */
	QM_RW uint32_t det;     /**< Driver Output Enable Timing Register. */
	QM_RW uint32_t tat;     /**< TurnAround Timing Register. */
	QM_RW uint32_t dlf;     /**< Divisor Latch Fraction. */
	QM_RW uint32_t rar;     /**< Receive Address Register. */
	QM_RW uint32_t tar;     /**< Transmit Address Register. */
	QM_RW uint32_t lcr_ext; /**< Line Extended Control Register. */
	QM_RW uint32_t padding[204]; /* 0x400 - 0xD0 */
} qm_uart_reg_t;

#define qm_uart_context_t uint8_t

#if (UNIT_TEST)
qm_uart_reg_t test_uart_instance;
qm_uart_reg_t *test_uart[QM_UART_NUM];
#define QM_UART test_uart

#else
/* UART register base address. */
#define QM_UART_0_BASE (0xB0002000)
#define QM_UART_1_BASE (0xB0002400)
/* UART register block. */
extern qm_uart_reg_t *qm_uart[QM_UART_NUM];
#define QM_UART qm_uart
#endif

/** @} */

/**
 * @name SPI
 * @{
 */

/** Number of SPI controllers. */
typedef enum { QM_SPI_MST_0 = 0, QM_SPI_SLV_0, QM_SPI_NUM } qm_spi_t;

/** SPI register map. */
typedef struct {
	QM_RW uint32_t ctrlr0; /**< Control Register 0. */
	QM_RW uint32_t ctrlr1; /**< Control Register 1. */
	QM_RW uint32_t ssienr; /**< SSI Enable Register. */
	QM_RW uint32_t mwcr;   /**< Microwire Control Register. */
	QM_RW uint32_t ser;    /**< Slave Enable Register. */
	QM_RW uint32_t baudr;  /**< Baud Rate Select. */
	QM_RW uint32_t txftlr; /**< Transmit FIFO Threshold Level. */
	QM_RW uint32_t rxftlr; /**< Receive FIFO Threshold Level. */
	QM_RW uint32_t txflr;  /**< Transmit FIFO Level Register. */
	QM_RW uint32_t rxflr;  /**< Receive FIFO Level Register. */
	QM_RW uint32_t sr;     /**< Status Register. */
	QM_RW uint32_t imr;    /**< Interrupt Mask Register. */
	QM_RW uint32_t isr;    /**< Interrupt Status Register. */
	QM_RW uint32_t risr;   /**< Raw Interrupt Status Register. */
	QM_RW uint32_t
	    txoicr; /**< Tx FIFO Overflow Interrupt Clear Register. */
	QM_RW uint32_t
	    rxoicr; /**< Rx FIFO Overflow Interrupt Clear Register. */
	QM_RW uint32_t
	    rxuicr; /**< Rx FIFO Underflow Interrupt Clear Register. */
	QM_RW uint32_t msticr;  /**< Multi-Master Interrupt Clear Register. */
	QM_RW uint32_t icr;     /**< Interrupt Clear Register. */
	QM_RW uint32_t dmacr;   /**< DMA Control Register. */
	QM_RW uint32_t dmatdlr; /**< DMA Transmit Data Level. */
	QM_RW uint32_t dmardlr; /**< DMA Receive Data Level. */
	QM_RW uint32_t idr;     /**< Identification Register. */
	QM_RW uint32_t ssi_comp_version; /**< coreKit Version ID register. */
	QM_RW uint32_t dr[36];		 /**< Data Register. */
	QM_RW uint32_t rx_sample_dly;    /**< RX Sample Delay Register. */
	QM_RW uint32_t padding[0x1C4];   /* (0x800 - 0xF0) / 4 */
} qm_spi_reg_t;

#define qm_spi_context_t uint8_t

#if (UNIT_TEST)
qm_spi_reg_t test_spi;
qm_spi_reg_t *test_spi_controllers[QM_SPI_NUM];

#define QM_SPI test_spi_controllers

#else
/* SPI Master register base address. */
#define QM_SPI_MST_0_BASE (0xB0001000)
extern qm_spi_reg_t *qm_spi_controllers[QM_SPI_NUM];
#define QM_SPI qm_spi_controllers

/* SPI Slave register base address. */
#define QM_SPI_SLV_BASE (0xB0001800)
#endif

/* SPI Ctrlr0 register */
#define QM_SPI_CTRLR0_DFS_32_MASK (0x001F0000)
#define QM_SPI_CTRLR0_TMOD_MASK (0x00000300)
#define QM_SPI_CTRLR0_SCPOL_SCPH_MASK (0x000000C0)
#define QM_SPI_CTRLR0_FRF_MASK (0x00000030)
#define QM_SPI_CTRLR0_DFS_32_OFFSET (16)
#define QM_SPI_CTRLR0_TMOD_OFFSET (8)
#define QM_SPI_CTRLR0_SCPOL_SCPH_OFFSET (6)
#define QM_SPI_CTRLR0_FRF_OFFSET (4)
#define QM_SPI_CTRLR0_SLV_OE BIT(10)

/* SPI SSI Enable register */
#define QM_SPI_SSIENR_SSIENR BIT(0)

/* SPI Status register */
#define QM_SPI_SR_BUSY BIT(0)
#define QM_SPI_SR_TFNF BIT(1)
#define QM_SPI_SR_TFE BIT(2)
#define QM_SPI_SR_RFNE BIT(3)
#define QM_SPI_SR_RFF BIT(4)

/* SPI Interrupt Mask register */
#define QM_SPI_IMR_MASK_ALL (0x00)
#define QM_SPI_IMR_TXEIM BIT(0)
#define QM_SPI_IMR_TXOIM BIT(1)
#define QM_SPI_IMR_RXUIM BIT(2)
#define QM_SPI_IMR_RXOIM BIT(3)
#define QM_SPI_IMR_RXFIM BIT(4)

/* SPI Interrupt Status register */
#define QM_SPI_ISR_TXEIS BIT(0)
#define QM_SPI_ISR_TXOIS BIT(1)
#define QM_SPI_ISR_RXUIS BIT(2)
#define QM_SPI_ISR_RXOIS BIT(3)
#define QM_SPI_ISR_RXFIS BIT(4)

/* SPI Raw Interrupt Status register */
#define QM_SPI_RISR_TXEIR BIT(0)
#define QM_SPI_RISR_TXOIR BIT(1)
#define QM_SPI_RISR_RXUIR BIT(2)
#define QM_SPI_RISR_RXOIR BIT(3)
#define QM_SPI_RISR_RXFIR BIT(4)

/* SPI DMA control */
#define QM_SPI_DMACR_RDMAE BIT(0)
#define QM_SPI_DMACR_TDMAE BIT(1)

/** @} */

/**
 * @name RTC
 * @{
 */

/** Number of RTC controllers. */
typedef enum { QM_RTC_0 = 0, QM_RTC_NUM } qm_rtc_t;

/** RTC register map. */
typedef struct {
	QM_RW uint32_t rtc_ccvr;	 /**< Current Counter Value Register. */
	QM_RW uint32_t rtc_cmr;		 /**< Current Match Register. */
	QM_RW uint32_t rtc_clr;		 /**< Counter Load Register. */
	QM_RW uint32_t rtc_ccr;		 /**< Counter Control Register. */
	QM_RW uint32_t rtc_stat;	 /**< Interrupt Status Register. */
	QM_RW uint32_t rtc_rstat;	/**< Interrupt Raw Status Register. */
	QM_RW uint32_t rtc_eoi;		 /**< End of Interrupt Register. */
	QM_RW uint32_t rtc_comp_version; /**< End of Interrupt Register. */
} qm_rtc_reg_t;

#define qm_rtc_context_t uint8_t

#define QM_RTC_CCR_INTERRUPT_ENABLE BIT(0)
#define QM_RTC_CCR_INTERRUPT_MASK BIT(1)
#define QM_RTC_CCR_ENABLE BIT(2)

#if (UNIT_TEST)
qm_rtc_reg_t test_rtc_instance[QM_RTC_NUM];
qm_rtc_reg_t *test_rtc[QM_RTC_NUM];

#define QM_RTC test_rtc

#else
extern qm_rtc_reg_t *qm_rtc[QM_RTC_NUM];
/* RTC register base address. */
#define QM_RTC_BASE (0xB0000400)

/* RTC register block. */
#define QM_RTC qm_rtc
#endif

/** @} */

/**
 * @name I2C
 * @{
 */

/** Number of I2C controllers. */
typedef enum { QM_I2C_0 = 0, QM_I2C_NUM } qm_i2c_t;

/** I2C register map. */
typedef struct {
	QM_RW uint32_t ic_con;      /**< Control Register. */
	QM_RW uint32_t ic_tar;      /**< Master Target Address. */
	QM_RW uint32_t ic_sar;      /**< Slave Address. */
	QM_RW uint32_t ic_hs_maddr; /**< High Speed Master ID. */
	QM_RW uint32_t ic_data_cmd; /**< Data Buffer and Command. */
	QM_RW uint32_t
	    ic_ss_scl_hcnt; /**< Standard Speed Clock SCL High Count. */
	QM_RW uint32_t
	    ic_ss_scl_lcnt; /**< Standard Speed Clock SCL Low Count. */
	QM_RW uint32_t ic_fs_scl_hcnt; /**< Fast Speed Clock SCL High Count. */
	QM_RW uint32_t
	    ic_fs_scl_lcnt; /**< Fast Speed I2C Clock SCL Low Count. */
	QM_RW uint32_t
	    ic_hs_scl_hcnt; /**< High Speed I2C Clock SCL High Count. */
	QM_RW uint32_t
	    ic_hs_scl_lcnt;	  /**< High Speed I2C Clock SCL Low Count. */
	QM_RW uint32_t ic_intr_stat; /**< Interrupt Status. */
	QM_RW uint32_t ic_intr_mask; /**< Interrupt Mask. */
	QM_RW uint32_t ic_raw_intr_stat; /**< Raw Interrupt Status. */
	QM_RW uint32_t ic_rx_tl;	 /**< Receive FIFO Threshold Level. */
	QM_RW uint32_t ic_tx_tl;	 /**< Transmit FIFO Threshold Level. */
	QM_RW uint32_t
	    ic_clr_intr; /**< Clear Combined and Individual Interrupt. */
	QM_RW uint32_t ic_clr_rx_under;   /**< Clear RX_UNDER Interrupt. */
	QM_RW uint32_t ic_clr_rx_over;    /**< Clear RX_OVER Interrupt. */
	QM_RW uint32_t ic_clr_tx_over;    /**< Clear TX_OVER Interrupt. */
	QM_RW uint32_t ic_clr_rd_req;     /**< Clear RD_REQ Interrupt. */
	QM_RW uint32_t ic_clr_tx_abrt;    /**< Clear TX_ABRT Interrupt. */
	QM_RW uint32_t ic_clr_rx_done;    /**< Clear RX_DONE Interrupt. */
	QM_RW uint32_t ic_clr_activity;   /**< Clear ACTIVITY Interrupt. */
	QM_RW uint32_t ic_clr_stop_det;   /**< Clear STOP_DET Interrupt. */
	QM_RW uint32_t ic_clr_start_det;  /**< Clear START_DET Interrupt. */
	QM_RW uint32_t ic_clr_gen_call;   /**< Clear GEN_CALL Interrupt. */
	QM_RW uint32_t ic_enable;	 /**< Enable. */
	QM_RW uint32_t ic_status;	 /**< Status. */
	QM_RW uint32_t ic_txflr;	  /**< Transmit FIFO Level. */
	QM_RW uint32_t ic_rxflr;	  /**< Receive FIFO Level. */
	QM_RW uint32_t ic_sda_hold;       /**< SDA Hold. */
	QM_RW uint32_t ic_tx_abrt_source; /**< Transmit Abort Source. */
	QM_RW uint32_t reserved;
	QM_RW uint32_t ic_dma_cr;    /**< SDA Setup. */
	QM_RW uint32_t ic_dma_tdlr;  /**< DMA Transmit Data Level Register. */
	QM_RW uint32_t ic_dma_rdlr;  /**< I2C Receive Data Level Register. */
	QM_RW uint32_t ic_sda_setup; /**< SDA Setup. */
	QM_RW uint32_t ic_ack_general_call; /**< General Call Ack. */
	QM_RW uint32_t ic_enable_status;    /**< Enable Status. */
	QM_RW uint32_t ic_fs_spklen; /**< SS and FS Spike Suppression Limit. */
	QM_RW uint32_t ic_hs_spklen; /**< HS spike suppression limit. */
	QM_RW uint32_t
	    ic_clr_restart_det; /**< clear the RESTART_DET interrupt. */
	QM_RW uint32_t reserved1[18];
	QM_RW uint32_t ic_comp_param_1; /**< Configuration Parameters. */
	QM_RW uint32_t ic_comp_version; /**< Component Version. */
	QM_RW uint32_t ic_comp_type;    /**< Component Type. */
} qm_i2c_reg_t;

#define qm_i2c_context_t uint8_t

#if (UNIT_TEST)
qm_i2c_reg_t test_i2c_instance[QM_I2C_NUM];
qm_i2c_reg_t *test_i2c[QM_I2C_NUM];

#define QM_I2C test_i2c

#else
/* I2C Master register base address. */
#define QM_I2C_0_BASE (0xB0002800)

/** I2C register block. */
extern qm_i2c_reg_t *qm_i2c[QM_I2C_NUM];
#define QM_I2C qm_i2c
#endif

#define QM_I2C_IC_ENABLE_CONTROLLER_EN BIT(0)
#define QM_I2C_IC_ENABLE_CONTROLLER_ABORT BIT(1)
#define QM_I2C_IC_ENABLE_STATUS_IC_EN BIT(0)
#define QM_I2C_IC_CON_MASTER_MODE BIT(0)
#define QM_I2C_IC_CON_SLAVE_DISABLE BIT(6)
#define QM_I2C_IC_CON_10BITADDR_MASTER BIT(4)
#define QM_I2C_IC_CON_10BITADDR_MASTER_OFFSET (4)
#define QM_I2C_IC_CON_10BITADDR_SLAVE BIT(3)
#define QM_I2C_IC_CON_10BITADDR_SLAVE_OFFSET (3)
#define QM_I2C_IC_CON_SPEED_OFFSET (1)
#define QM_I2C_IC_CON_SPEED_SS BIT(1)
#define QM_I2C_IC_CON_SPEED_FS_FSP BIT(2)
#define QM_I2C_IC_CON_SPEED_MASK (0x06)
#define QM_I2C_IC_CON_RESTART_EN BIT(5)
#define QM_I2C_IC_CON_STOP_DET_IFADDRESSED BIT(7)
#define QM_I2C_IC_DATA_CMD_READ BIT(8)
#define QM_I2C_IC_DATA_CMD_STOP_BIT_CTRL BIT(9)
#define QM_I2C_IC_DATA_CMD_LSB_MASK (0x000000FF)
#define QM_I2C_IC_RAW_INTR_STAT_RX_FULL BIT(2)
#define QM_I2C_IC_RAW_INTR_STAT_TX_ABRT BIT(6)
#define QM_I2C_IC_RAW_INTR_STAT_GEN_CALL BIT(11)
#define QM_I2C_IC_RAW_INTR_STAT_RESTART_DETECTED BIT(12)
#define QM_I2C_IC_TX_ABRT_SOURCE_NAK_MASK (0x1F)
#define QM_I2C_IC_TX_ABRT_SOURCE_ARB_LOST BIT(12)
#define QM_I2C_IC_TX_ABRT_SOURCE_ABRT_SBYTE_NORSTRT BIT(9)
#define QM_I2C_IC_TX_ABRT_SOURCE_ALL_MASK (0x1FFFF)
#define QM_I2C_IC_STATUS_BUSY_MASK (0x00000060)
#define QM_I2C_IC_STATUS_RFF BIT(4)
#define QM_I2C_IC_STATUS_RFNE BIT(3)
#define QM_I2C_IC_STATUS_TFE BIT(2)
#define QM_I2C_IC_STATUS_TNF BIT(1)
#define QM_I2C_IC_INTR_MASK_ALL (0x00)
#define QM_I2C_IC_INTR_MASK_RX_UNDER BIT(0)
#define QM_I2C_IC_INTR_MASK_RX_OVER BIT(1)
#define QM_I2C_IC_INTR_MASK_RX_FULL BIT(2)
#define QM_I2C_IC_INTR_MASK_TX_OVER BIT(3)
#define QM_I2C_IC_INTR_MASK_TX_EMPTY BIT(4)
#define QM_I2C_IC_INTR_MASK_RD_REQ BIT(5)
#define QM_I2C_IC_INTR_MASK_TX_ABORT BIT(6)
#define QM_I2C_IC_INTR_MASK_RX_DONE BIT(7)
#define QM_I2C_IC_INTR_MASK_ACTIVITY BIT(8)
#define QM_I2C_IC_INTR_MASK_STOP_DETECTED BIT(9)
#define QM_I2C_IC_INTR_MASK_START_DETECTED BIT(10)
#define QM_I2C_IC_INTR_MASK_GEN_CALL_DETECTED BIT(11)
#define QM_I2C_IC_INTR_MASK_RESTART_DETECTED BIT(12)
#define QM_I2C_IC_INTR_STAT_RX_UNDER BIT(0)
#define QM_I2C_IC_INTR_STAT_RX_OVER BIT(1)
#define QM_I2C_IC_INTR_STAT_RX_FULL BIT(2)
#define QM_I2C_IC_INTR_STAT_TX_OVER BIT(3)
#define QM_I2C_IC_INTR_STAT_TX_EMPTY BIT(4)
#define QM_I2C_IC_INTR_STAT_RD_REQ BIT(5)
#define QM_I2C_IC_INTR_STAT_TX_ABRT BIT(6)
#define QM_I2C_IC_INTR_STAT_RX_DONE BIT(7)
#define QM_I2C_IC_INTR_STAT_STOP_DETECTED BIT(9)
#define QM_I2C_IC_INTR_STAT_START_DETECTED BIT(10)
#define QM_I2C_IC_INTR_STAT_GEN_CALL_DETECTED BIT(11)
#define QM_I2C_IC_LCNT_MAX (65525)
#define QM_I2C_IC_LCNT_MIN (8)
#define QM_I2C_IC_HCNT_MAX (65525)
#define QM_I2C_IC_HCNT_MIN (6)
#define QM_I2C_IC_TAR_MASK (0x3FF)

#define QM_I2C_FIFO_SIZE (16)

/* I2C DMA */
#define QM_I2C_IC_DMA_CR_RX_ENABLE BIT(0)
#define QM_I2C_IC_DMA_CR_TX_ENABLE BIT(1)

/** @} */

/**
 * @name GPIO
 * @{
 */

/** Number of GPIO controllers. */
typedef enum { QM_GPIO_0 = 0, QM_GPIO_NUM } qm_gpio_t;

/** GPIO register map. */
typedef struct {
	QM_RW uint32_t gpio_swporta_dr;  /**< Port A Data. */
	QM_RW uint32_t gpio_swporta_ddr; /**< Port A Data Direction. */
	QM_RW uint32_t reserved[10];
	QM_RW uint32_t gpio_inten;	 /**< Interrupt Enable. */
	QM_RW uint32_t gpio_intmask;       /**< Interrupt Mask. */
	QM_RW uint32_t gpio_inttype_level; /**< Interrupt Type. */
	QM_RW uint32_t gpio_int_polarity;  /**< Interrupt Polarity. */
	QM_RW uint32_t gpio_intstatus;     /**< Interrupt Status. */
	QM_RW uint32_t gpio_raw_intstatus; /**< Raw Interrupt Status. */
	QM_RW uint32_t gpio_debounce;      /**< Debounce Enable. */
	QM_RW uint32_t gpio_porta_eoi;     /**< Clear Interrupt. */
	QM_RW uint32_t gpio_ext_porta;     /**< Port A External Port. */
	QM_RW uint32_t reserved1[3];
	QM_RW uint32_t gpio_ls_sync;      /**< Synchronization Level. */
	QM_RW uint32_t gpio_id_code;      /**< GPIO ID code. */
	QM_RW uint32_t gpio_int_bothedge; /**< Interrupt both edge type. */
	QM_RW uint32_t gpio_ver_id_code;  /**< GPIO Component Version. */
	QM_RW uint32_t gpio_config_reg2;  /**< GPIO Configuration Register 2. */
	QM_RW uint32_t gpio_config_reg1;  /**< GPIO Configuration Register 1. */
} qm_gpio_reg_t;

#define qm_gpio_context_t uint8_t

#define QM_NUM_GPIO_PINS (25)

#if (UNIT_TEST)
qm_gpio_reg_t test_gpio_instance;
qm_gpio_reg_t *test_gpio[QM_GPIO_NUM];

#define QM_GPIO test_gpio
#else

/* GPIO register base address. */
#define QM_GPIO_BASE (0xB0000C00)

/* GPIO register block. */
extern qm_gpio_reg_t *qm_gpio[QM_GPIO_NUM];
#define QM_GPIO qm_gpio
#endif

/** @} */

/**
 * @name ADC
 * @{
 */

/** Number of ADC controllers. */
typedef enum { QM_ADC_0 = 0, QM_ADC_NUM } qm_adc_t;

/** ADC register map. */
typedef struct {
	QM_RW uint32_t adc_seq0; /**< ADC Channel Sequence Table Entry 0 */
	QM_RW uint32_t adc_seq1; /**< ADC Channel Sequence Table Entry 1 */
	QM_RW uint32_t adc_seq2; /**< ADC Channel Sequence Table Entry 2 */
	QM_RW uint32_t adc_seq3; /**< ADC Channel Sequence Table Entry 3 */
	QM_RW uint32_t adc_seq4; /**< ADC Channel Sequence Table Entry 4 */
	QM_RW uint32_t adc_seq5; /**< ADC Channel Sequence Table Entry 5 */
	QM_RW uint32_t adc_seq6; /**< ADC Channel Sequence Table Entry 6 */
	QM_RW uint32_t adc_seq7; /**< ADC Channel Sequence Table Entry 7 */
	QM_RW uint32_t adc_cmd;  /**< ADC Command Register  */
	QM_RW uint32_t adc_intr_status; /**< ADC Interrupt Status Register */
	QM_RW uint32_t adc_intr_enable; /**< ADC Interrupt Enable Register */
	QM_RW uint32_t adc_sample;      /**< ADC Sample Register */
	QM_RW uint32_t adc_calibration; /**< ADC Calibration Data Register */
	QM_RW uint32_t adc_fifo_count;  /**< ADC FIFO Count Register */
	QM_RW uint32_t adc_op_mode;     /**< ADC Operating Mode Register */
} qm_adc_reg_t;

#if (UNIT_TEST)
qm_adc_reg_t test_adc;

#define QM_ADC ((qm_adc_reg_t *)(&test_adc))
#else
/* ADC register block */
#define QM_ADC ((qm_adc_reg_t *)QM_ADC_BASE)

/* ADC register base. */
#define QM_ADC_BASE (0xB0004000)
#endif

#define QM_ADC_DIV_MAX (1023)
#define QM_ADC_DELAY_MAX (0x1FFF)
#define QM_ADC_CAL_MAX (0x3F)
#define QM_ADC_FIFO_LEN (32)
#define QM_ADC_FIFO_CLEAR (0xFFFFFFFF)
/* ADC sequence table */
#define QM_ADC_CAL_SEQ_TABLE_DEFAULT (0x80808080)
/* ADC command */
#define QM_ADC_CMD_SW_OFFSET (24)
#define QM_ADC_CMD_SW_MASK (0xFF000000)
#define QM_ADC_CMD_CAL_DATA_OFFSET (16)
#define QM_ADC_CMD_RESOLUTION_OFFSET (14)
#define QM_ADC_CMD_RESOLUTION_MASK (0xC000)
#define QM_ADC_CMD_NS_OFFSET (4)
#define QM_ADC_CMD_NS_MASK (0x1F0)
#define QM_ADC_CMD_IE_OFFSET (3)
#define QM_ADC_CMD_IE BIT(3)
/* Interrupt enable */
#define QM_ADC_INTR_ENABLE_CC BIT(0)
#define QM_ADC_INTR_ENABLE_FO BIT(1)
#define QM_ADC_INTR_ENABLE_CONT_CC BIT(2)
/* Interrupt status */
#define QM_ADC_INTR_STATUS_CC BIT(0)
#define QM_ADC_INTR_STATUS_FO BIT(1)
#define QM_ADC_INTR_STATUS_CONT_CC BIT(2)
/* Operating mode */
#define QM_ADC_OP_MODE_IE BIT(27)
#define QM_ADC_OP_MODE_DELAY_OFFSET (0x3)
#define QM_ADC_OP_MODE_DELAY_MASK (0xFFF8)
#define QM_ADC_OP_MODE_OM_MASK (0x7)

/** @} */

/**
 * @name Flash
 * @{
 */

#define NUM_FLASH_CONTROLLERS (1)
#define HAS_FLASH_WRITE_DISABLE (1)

/** Number of Flash controllers. */
typedef enum { QM_FLASH_0 = 0, QM_FLASH_NUM } qm_flash_t;

/** Flash register map. */
typedef struct {
	QM_RW uint32_t tmg_ctrl;      /**< TMG_CTRL */
	QM_RW uint32_t rom_wr_ctrl;   /**< ROM_WR_CTRL */
	QM_RW uint32_t rom_wr_data;   /**< ROM_WR_DATA */
	QM_RW uint32_t flash_wr_ctrl; /**< FLASH_WR_CTRL */
	QM_RW uint32_t flash_wr_data; /**< FLASH_WR_DATA */
	QM_RW uint32_t flash_stts;    /**< FLASH_STTS */
	QM_RW uint32_t ctrl;	  /**< CTRL */
	QM_RW uint32_t fpr_rd_cfg[4]; /**< 4 FPR_RD_CFG registers */
	QM_RW uint32_t
	    mpr_wr_cfg;		 /**< Flash Write Protection Control Register */
	QM_RW uint32_t mpr_vsts; /**< Protection Status Register */
	QM_RW uint32_t mpr_vdata; /**< MPR Violation Data Value Register */
} qm_flash_reg_t;

#define qm_flash_context_t uint8_t

#define QM_FLASH_REGION_DATA_0_SIZE (0x1000)
#define QM_FLASH_REGION_DATA_0_PAGES (0x02)

#if (UNIT_TEST)
qm_flash_reg_t test_flash_instance;
qm_flash_reg_t *test_flash[QM_FLASH_NUM];
uint8_t test_flash_page[0x800];

#define QM_FLASH test_flash

#define QM_FLASH_REGION_DATA_0_BASE (test_flash_page)
#define QM_FLASH_REGION_SYS_0_BASE (test_flash_page)
#define QM_FLASH_REGION_OTP_0_BASE (test_flash_page)

#define QM_FLASH_PAGE_MASK (0xCFF)
#define QM_FLASH_MAX_ADDR (0xFFFFFFFF)
#else

/* Flash physical address mappings */
#define QM_FLASH_REGION_DATA_0_BASE (0x00200000)
#define QM_FLASH_REGION_SYS_0_BASE (0x00180000)
#define QM_FLASH_REGION_OTP_0_BASE (0x00000000)

#define QM_FLASH_PAGE_MASK (0xF800)
#define QM_FLASH_MAX_ADDR (0x8000)

/* Flash controller register base address. */
#define QM_FLASH_BASE_0 (0xB0100000)

/* Flash controller register block. */
extern qm_flash_reg_t *qm_flash[QM_FLASH_NUM];
#define QM_FLASH qm_flash

#endif

#define QM_FLASH_REGION_DATA_BASE_OFFSET (0x04)
#define QM_FLASH_MAX_WAIT_STATES (0xF)
#define QM_FLASH_MAX_US_COUNT (0x3F)
#define QM_FLASH_MAX_PAGE_NUM                                                  \
	(QM_FLASH_MAX_ADDR / (4 * QM_FLASH_PAGE_SIZE_DWORDS))
#define QM_FLASH_CLK_SLOW BIT(14)
#define QM_FLASH_LVE_MODE BIT(5)

/* Flash mask to clear timing. */
#define QM_FLASH_TMG_DEF_MASK (0xFFFFFC00)
/* Flash mask to clear micro seconds. */
#define QM_FLASH_MICRO_SEC_COUNT_MASK (0x3F)
/* Flash mask to clear wait state. */
#define QM_FLASH_WAIT_STATE_MASK (0x3C0)
/* Flash wait state offset bit. */
#define QM_FLASH_WAIT_STATE_OFFSET (6)
/* Flash write disable offset bit. */
#define QM_FLASH_WRITE_DISABLE_OFFSET (4)
/* Flash write disable value. */
#define QM_FLASH_WRITE_DISABLE_VAL BIT(4)

/* Flash page erase request. */
#define ER_REQ BIT(1)
/* Flash page erase done. */
#define ER_DONE (1)
/* Flash page write request. */
#define WR_REQ (1)
/* Flash page write done. */
#define WR_DONE BIT(1)

/* Flash write address offset. */
#define WR_ADDR_OFFSET (2)
/* Flash perform mass erase includes OTP region. */
#define MASS_ERASE_INFO BIT(6)
/* Flash perform mass erase. */
#define MASS_ERASE BIT(7)

/* ROM read disable for upper 4k. */
#define ROM_RD_DIS_U BIT(3)
/* ROM read disable for lower 4k. */
#define ROM_RD_DIS_L BIT(2)

#define QM_FLASH_ADDRESS_MASK (0x7FF)
/* Increment by 4 bytes each time, but there is an offset of 2, so 0x10. */
#define QM_FLASH_ADDR_INC (0x10)

/* Flash page size in dwords. */
#define QM_FLASH_PAGE_SIZE_DWORDS (0x200)
/* Flash page size in bytes. */
#define QM_FLASH_PAGE_SIZE_BYTES (0x800)
/* Flash page size in bits. */
#define QM_FLASH_PAGE_SIZE_BITS (11)
/* OTP ROM_PROG bit. */
#define QM_FLASH_STTS_ROM_PROG (BIT(2))

/** @} */

/**
 * @name Flash Protection Region
 * @{
 */

/**
 * FPR register map.
 */
typedef enum {
	QM_FPR_0, /**< FPR 0. */
	QM_FPR_1, /**< FPR 1. */
	QM_FPR_2, /**< FPR 2. */
	QM_FPR_3, /**< FPR 3. */
	QM_FPR_NUM
} qm_fpr_id_t;

#define qm_fpr_context_t uint8_t

/* The addressing granularity of FPRs. */
#define QM_FPR_GRANULARITY (1024)

/** @} */

/**
 * @name Memory Protection Region
 * @{
 */

/* MPR identifier */
typedef enum {
	QM_MPR_0 = 0, /**< Memory Protection Region 0. */
	QM_MPR_1,     /**< Memory Protection Region 1. */
	QM_MPR_2,     /**< Memory Protection Region 2. */
	QM_MPR_3,     /**< Memory Protection Region 3. */
	QM_MPR_NUM    /**< Number of Memory Protection Regions. */
} qm_mpr_id_t;

/** Memory Protection Region register map. */
typedef struct {
	QM_RW uint32_t mpr_cfg[4]; /**< MPR CFG */
	QM_RW uint32_t mpr_vdata;  /**< MPR_VDATA  */
	QM_RW uint32_t mpr_vsts;   /**< MPR_VSTS  */
} qm_mpr_reg_t;

#define qm_mpr_context_t uint8_t

/* The addressing granularity of MPRs. */
#define QM_MPR_GRANULARITY (1024)

#if (UNIT_TEST)
qm_mpr_reg_t test_mpr;

#define QM_MPR ((qm_mpr_reg_t *)(&test_mpr))

#else

#define QM_MPR_BASE (0xB0400000)
#define QM_MPR ((qm_mpr_reg_t *)QM_MPR_BASE)

#endif

#define QM_MPR_RD_EN_OFFSET (20)
#define QM_MPR_RD_EN_MASK 0x700000
#define QM_MPR_WR_EN_OFFSET (24)
#define QM_MPR_WR_EN_MASK 0x7000000
#define QM_MPR_EN_LOCK_OFFSET (30)
#define QM_MPR_EN_LOCK_MASK 0xC0000000
#define QM_MPR_UP_BOUND_OFFSET (10)
#define QM_MPR_VSTS_VALID BIT(31)

/** @} */

/**
 * @name PIC
 * @{
 */

/** PIC timer register structure. */
typedef struct {
	QM_RW uint32_t reg;
	QM_RW uint32_t pad[3];
} pic_timer_reg_pad_t;

/** PIC timer register map. */
typedef struct {
	QM_RW pic_timer_reg_pad_t lvttimer; /**< Local Vector Table Timer */
	QM_RW pic_timer_reg_pad_t reserved[5];
	QM_RW pic_timer_reg_pad_t timer_icr; /**< Initial Count Register */
	QM_RW pic_timer_reg_pad_t timer_ccr; /**< Current Count Register */
} qm_pic_timer_reg_t;

#define qm_pic_timer_context_t uint8_t

#if (UNIT_TEST)
qm_pic_timer_reg_t test_pic_timer;
#define QM_PIC_TIMER ((qm_pic_timer_reg_t *)(&test_pic_timer))

#else
/* PIC timer base address. */
#define QM_PIC_TIMER_BASE (0xFEE00320)
#define QM_PIC_TIMER ((qm_pic_timer_reg_t *)QM_PIC_TIMER_BASE)
#endif

/** @} */

/**
 * @name Peripheral Clock
 * @{
 */

/** Peripheral clock register map. */
typedef enum {
	CLK_PERIPH_REGISTER = BIT(0), /**< Peripheral Clock Gate Enable. */
	CLK_PERIPH_CLK = BIT(1),      /**< Peripheral Clock Enable. */
	CLK_PERIPH_I2C_M0 = BIT(2),   /**< I2C Master 0 Clock Enable. */
	CLK_PERIPH_SPI_S = BIT(4),    /**< SPI Slave Clock Enable. */
	CLK_PERIPH_SPI_M0 = BIT(5),   /**< SPI Master 0 Clock Enable. */
	CLK_PERIPH_GPIO_INTERRUPT = BIT(7), /**< GPIO Interrupt Clock Enable. */
	CLK_PERIPH_GPIO_DB = BIT(8),	/**< GPIO Debounce Clock Enable. */
	CLK_PERIPH_WDT_REGISTER = BIT(10),  /**< Watchdog Clock Enable. */
	CLK_PERIPH_RTC_REGISTER = BIT(11),  /**< RTC Clock Gate Enable. */
	CLK_PERIPH_PWM_REGISTER = BIT(12),  /**< PWM Clock Gate Enable. */
	CLK_PERIPH_GPIO_REGISTER = BIT(13), /**< GPIO Clock Gate Enable. */
	CLK_PERIPH_SPI_M0_REGISTER =
	    BIT(14), /**< SPI Master 0 Clock Gate Enable. */
	CLK_PERIPH_SPI_S_REGISTER =
	    BIT(16), /**< SPI Slave Clock Gate Enable. */
	CLK_PERIPH_UARTA_REGISTER = BIT(17), /**< UARTA Clock Gate Enable. */
	CLK_PERIPH_UARTB_REGISTER = BIT(18), /**< UARTB Clock Gate Enable. */
	CLK_PERIPH_I2C_M0_REGISTER =
	    BIT(19),		  /**< I2C Master 0 Clock Gate Enable. */
	CLK_PERIPH_ADC = BIT(22), /**< ADC Clock Enable. */
	CLK_PERIPH_ADC_REGISTER = BIT(23), /**< ADC Clock Gate Enable. */
	CLK_PERIPH_ALL = 0xCFFFFF /**< Quark D2000 peripherals Enable. */
} clk_periph_t;

/* Default mask values */
#define CLK_EXTERN_DIV_DEF_MASK (0xFFFFFFE3)
#define CLK_SYS_CLK_DIV_DEF_MASK (0xFFFFF87F)
#define CLK_RTC_DIV_DEF_MASK (0xFFFFFF83)
#define CLK_GPIO_DB_DIV_DEF_MASK (0xFFFFFFE1)
#define CLK_ADC_DIV_DEF_MASK (0xFC00FFFF)
#define CLK_PERIPH_DIV_DEF_MASK (0xFFFFFFF9)

/** @} */

/**
 * @name MVIC
 * @{
 */

/** MVIC register structure. */
typedef struct {
	QM_RW uint32_t reg;
	QM_RW uint32_t pad[3];
} mvic_reg_pad_t;

/** MVIC register map. */
typedef struct {
	QM_RW mvic_reg_pad_t tpr; /**< Task priority. */
	QM_RW mvic_reg_pad_t reserved;
	QM_RW mvic_reg_pad_t ppr; /**< Processor priority. */
	QM_RW mvic_reg_pad_t eoi; /**< End of interrupt. */
	QM_RW mvic_reg_pad_t reserved1[3];
	QM_RW mvic_reg_pad_t sivr; /**< Spurious vector. */
	QM_RW mvic_reg_pad_t reserved2;
	QM_RW mvic_reg_pad_t isr; /**< In-service. */
	QM_RW mvic_reg_pad_t reserved3[15];
	QM_RW mvic_reg_pad_t irr; /**< Interrupt request. */
	QM_RW mvic_reg_pad_t reserved4[16];
	QM_RW mvic_reg_pad_t lvttimer; /**< Timer vector. */
	QM_RW mvic_reg_pad_t reserved5[5];
	QM_RW mvic_reg_pad_t icr; /**< Timer initial count. */
	QM_RW mvic_reg_pad_t ccr; /**< Timer current count. */
} qm_mvic_reg_t;

#define qm_irq_context_t uint8_t

#define QM_MVIC_REG_VER (0x01)    /* MVIC version. */
#define QM_MVIC_REG_REDTBL (0x10) /* Redirection table base. */

#if (UNIT_TEST)
qm_mvic_reg_t test_mvic;
#define QM_MVIC ((qm_mvic_reg_t *)(&test_mvic))

#else
/* Interrupt Controller base address. */
#define QM_MVIC_BASE (0xFEE00080)
#define QM_MVIC ((qm_mvic_reg_t *)QM_MVIC_BASE)
#endif

#define QM_INT_CONTROLLER QM_MVIC
/* Signal the interrupt controller that the interrupt was handled.  The vector
 * argument is ignored. */
#if defined(ENABLE_EXTERNAL_ISR_HANDLING)
#define QM_ISR_EOI(vector)
#else
#define QM_ISR_EOI(vector) (QM_INT_CONTROLLER->eoi.reg = 0)
#endif

typedef struct {
	QM_RW mvic_reg_pad_t ioregsel; /**< Register selector. */
	QM_RW mvic_reg_pad_t iowin;    /**< Register window. */
} qm_ioapic_reg_t;

#if (UNIT_TEST)
qm_ioapic_reg_t test_ioapic;
#define QM_IOAPIC ((qm_ioapic_reg_t *)(&test_ioapic))

#else
/* IO/APIC. */
#define QM_IOAPIC_BASE (0xFEC00000)
#define QM_IOAPIC ((qm_ioapic_reg_t *)QM_IOAPIC_BASE)
#endif

/** @} */

/**
 * @name DMA
 * @{
 */

/** DMA instances. */
typedef enum {
	QM_DMA_0,  /**< DMA controller id. */
	QM_DMA_NUM /**< Number of DMA controllers. */
} qm_dma_t;

/** DMA channel IDs. */
typedef enum {
	QM_DMA_CHANNEL_0 = 0, /**< DMA channel id for channel 0 */
	QM_DMA_CHANNEL_1,     /**< DMA channel id for channel 1 */
	QM_DMA_CHANNEL_NUM    /**< Number of DMA channels */
} qm_dma_channel_id_t;

/** DMA hardware handshake interfaces. */
typedef enum {
	DMA_HW_IF_UART_A_TX = 0x0,       /**< UART_A_TX */
	DMA_HW_IF_UART_A_RX = 0x1,       /**< UART_A_RX */
	DMA_HW_IF_UART_B_TX = 0x2,       /**< UART_B_TX*/
	DMA_HW_IF_UART_B_RX = 0x3,       /**< UART_B_RX */
	DMA_HW_IF_SPI_MASTER_0_TX = 0x4, /**< SPI_Master_0_TX */
	DMA_HW_IF_SPI_MASTER_0_RX = 0x5, /**< SPI_Master_0_RX */
	DMA_HW_IF_SPI_SLAVE_TX = 0x8,    /**< SPI_Slave_TX */
	DMA_HW_IF_SPI_SLAVE_RX = 0x9,    /**< SPI_Slave_RX */
	DMA_HW_IF_I2C_MASTER_0_TX = 0xc, /**< I2C_Master_0_TX */
	DMA_HW_IF_I2C_MASTER_0_RX = 0xd, /**< I2C_Master_0_RX */
} qm_dma_handshake_interface_t;

/** DMA channel register map. */
typedef struct {
	QM_RW uint32_t sar_low;		   /**< SAR */
	QM_RW uint32_t sar_high;	   /**< SAR */
	QM_RW uint32_t dar_low;		   /**< DAR */
	QM_RW uint32_t dar_high;	   /**< DAR */
	QM_RW uint32_t llp_low;		   /**< LLP */
	QM_RW uint32_t llp_high;	   /**< LLP */
	QM_RW uint32_t ctrl_low;	   /**< CTL */
	QM_RW uint32_t ctrl_high;	  /**< CTL */
	QM_RW uint32_t src_stat_low;       /**< SSTAT */
	QM_RW uint32_t src_stat_high;      /**< SSTAT */
	QM_RW uint32_t dst_stat_low;       /**< DSTAT */
	QM_RW uint32_t dst_stat_high;      /**< DSTAT */
	QM_RW uint32_t src_stat_addr_low;  /**< SSTATAR */
	QM_RW uint32_t src_stat_addr_high; /**< SSTATAR */
	QM_RW uint32_t dst_stat_addr_low;  /**< DSTATAR */
	QM_RW uint32_t dst_stat_addr_high; /**< DSTATAR */
	QM_RW uint32_t cfg_low;		   /**< CFG */
	QM_RW uint32_t cfg_high;	   /**< CFG */
	QM_RW uint32_t src_sg_low;	 /**< SGR */
	QM_RW uint32_t src_sg_high;	/**< SGR */
	QM_RW uint32_t dst_sg_low;	 /**< DSR */
	QM_RW uint32_t dst_sg_high;	/**< DSR */
} qm_dma_chan_reg_t;

#define qm_dma_context_t uint8_t

/* DMA channel control register offsets and masks. */
#define QM_DMA_CTL_L_INT_EN_MASK BIT(0)
#define QM_DMA_CTL_L_DST_TR_WIDTH_OFFSET (1)
#define QM_DMA_CTL_L_DST_TR_WIDTH_MASK (0x7 << QM_DMA_CTL_L_DST_TR_WIDTH_OFFSET)
#define QM_DMA_CTL_L_SRC_TR_WIDTH_OFFSET (4)
#define QM_DMA_CTL_L_SRC_TR_WIDTH_MASK (0x7 << QM_DMA_CTL_L_SRC_TR_WIDTH_OFFSET)
#define QM_DMA_CTL_L_DINC_OFFSET (7)
#define QM_DMA_CTL_L_DINC_MASK (0x3 << QM_DMA_CTL_L_DINC_OFFSET)
#define QM_DMA_CTL_L_SINC_OFFSET (9)
#define QM_DMA_CTL_L_SINC_MASK (0x3 << QM_DMA_CTL_L_SINC_OFFSET)
#define QM_DMA_CTL_L_DEST_MSIZE_OFFSET (11)
#define QM_DMA_CTL_L_DEST_MSIZE_MASK (0x7 << QM_DMA_CTL_L_DEST_MSIZE_OFFSET)
#define QM_DMA_CTL_L_SRC_MSIZE_OFFSET (14)
#define QM_DMA_CTL_L_SRC_MSIZE_MASK (0x7 << QM_DMA_CTL_L_SRC_MSIZE_OFFSET)
#define QM_DMA_CTL_L_TT_FC_OFFSET (20)
#define QM_DMA_CTL_L_TT_FC_MASK (0x7 << QM_DMA_CTL_L_TT_FC_OFFSET)
#define QM_DMA_CTL_L_LLP_DST_EN_MASK BIT(27)
#define QM_DMA_CTL_L_LLP_SRC_EN_MASK BIT(28)
#define QM_DMA_CTL_H_BLOCK_TS_OFFSET (0)
#define QM_DMA_CTL_H_BLOCK_TS_MASK (0xfff << QM_DMA_CTL_H_BLOCK_TS_OFFSET)
#define QM_DMA_CTL_H_BLOCK_TS_MAX 4095
#define QM_DMA_CTL_H_BLOCK_TS_MIN 1

/* DMA channel config register offsets and masks. */
#define QM_DMA_CFG_L_CH_SUSP_MASK BIT(8)
#define QM_DMA_CFG_L_FIFO_EMPTY_MASK BIT(9)
#define QM_DMA_CFG_L_HS_SEL_DST_OFFSET 10
#define QM_DMA_CFG_L_HS_SEL_DST_MASK BIT(QM_DMA_CFG_L_HS_SEL_DST_OFFSET)
#define QM_DMA_CFG_L_HS_SEL_SRC_OFFSET 11
#define QM_DMA_CFG_L_HS_SEL_SRC_MASK BIT(QM_DMA_CFG_L_HS_SEL_SRC_OFFSET)
#define QM_DMA_CFG_L_DST_HS_POL_OFFSET 18
#define QM_DMA_CFG_L_DST_HS_POL_MASK BIT(QM_DMA_CFG_L_DST_HS_POL_OFFSET)
#define QM_DMA_CFG_L_SRC_HS_POL_OFFSET 19
#define QM_DMA_CFG_L_SRC_HS_POL_MASK BIT(QM_DMA_CFG_L_SRC_HS_POL_OFFSET)
#define QM_DMA_CFG_L_RELOAD_SRC_MASK BIT(30)
#define QM_DMA_CFG_L_RELOAD_DST_MASK BIT(31)
#define QM_DMA_CFG_H_DS_UPD_EN_OFFSET (5)
#define QM_DMA_CFG_H_DS_UPD_EN_MASK BIT(QM_DMA_CFG_H_DS_UPD_EN_OFFSET)
#define QM_DMA_CFG_H_SS_UPD_EN_OFFSET (6)
#define QM_DMA_CFG_H_SS_UPD_EN_MASK BIT(QM_DMA_CFG_H_SS_UPD_EN_OFFSET)
#define QM_DMA_CFG_H_SRC_PER_OFFSET (7)
#define QM_DMA_CFG_H_SRC_PER_MASK (0xf << QM_DMA_CFG_H_SRC_PER_OFFSET)
#define QM_DMA_CFG_H_DEST_PER_OFFSET (11)
#define QM_DMA_CFG_H_DEST_PER_MASK (0xf << QM_DMA_CFG_H_DEST_PER_OFFSET)

#define QM_DMA_ENABLE_CLOCK(dma)                                               \
	(QM_SCSS_CCU->ccu_mlayer_ahb_ctl |= QM_CCU_DMA_CLK_EN)

/** DMA interrupt register map. */
typedef struct {
	QM_RW uint32_t raw_tfr_low;	   /**< RawTfr */
	QM_RW uint32_t raw_tfr_high;	  /**< RawTfr */
	QM_RW uint32_t raw_block_low;	 /**< RawBlock */
	QM_RW uint32_t raw_block_high;	/**< RawBlock */
	QM_RW uint32_t raw_src_trans_low;     /**< RawSrcTran */
	QM_RW uint32_t raw_src_trans_high;    /**< RawSrcTran */
	QM_RW uint32_t raw_dst_trans_low;     /**< RawDstTran */
	QM_RW uint32_t raw_dst_trans_high;    /**< RawDstTran */
	QM_RW uint32_t raw_err_low;	   /**< RawErr */
	QM_RW uint32_t raw_err_high;	  /**< RawErr */
	QM_RW uint32_t status_tfr_low;	/**< StatusTfr */
	QM_RW uint32_t status_tfr_high;       /**< StatusTfr */
	QM_RW uint32_t status_block_low;      /**< StatusBlock */
	QM_RW uint32_t status_block_high;     /**< StatusBlock */
	QM_RW uint32_t status_src_trans_low;  /**< StatusSrcTran */
	QM_RW uint32_t status_src_trans_high; /**< StatusSrcTran */
	QM_RW uint32_t status_dst_trans_low;  /**< StatusDstTran */
	QM_RW uint32_t status_dst_trans_high; /**< StatusDstTran */
	QM_RW uint32_t status_err_low;	/**< StatusErr */
	QM_RW uint32_t status_err_high;       /**< StatusErr */
	QM_RW uint32_t mask_tfr_low;	  /**< MaskTfr */
	QM_RW uint32_t mask_tfr_high;	 /**< MaskTfr */
	QM_RW uint32_t mask_block_low;	/**< MaskBlock */
	QM_RW uint32_t mask_block_high;       /**< MaskBlock */
	QM_RW uint32_t mask_src_trans_low;    /**< MaskSrcTran */
	QM_RW uint32_t mask_src_trans_high;   /**< MaskSrcTran */
	QM_RW uint32_t mask_dst_trans_low;    /**< MaskDstTran */
	QM_RW uint32_t mask_dst_trans_high;   /**< MaskDstTran */
	QM_RW uint32_t mask_err_low;	  /**< MaskErr */
	QM_RW uint32_t mask_err_high;	 /**< MaskErr */
	QM_RW uint32_t clear_tfr_low;	 /**< ClearTfr */
	QM_RW uint32_t clear_tfr_high;	/**< ClearTfr */
	QM_RW uint32_t clear_block_low;       /**< ClearBlock */
	QM_RW uint32_t clear_block_high;      /**< ClearBlock */
	QM_RW uint32_t clear_src_trans_low;   /**< ClearSrcTran */
	QM_RW uint32_t clear_src_trans_high;  /**< ClearSrcTran */
	QM_RW uint32_t clear_dst_trans_low;   /**< ClearDstTran */
	QM_RW uint32_t clear_dst_trans_high;  /**< ClearDstTran */
	QM_RW uint32_t clear_err_low;	 /**< ClearErr */
	QM_RW uint32_t clear_err_high;	/**< ClearErr */
	QM_RW uint32_t status_int_low;	/**< StatusInt */
	QM_RW uint32_t status_int_high;       /**< StatusInt */
} qm_dma_int_reg_t;

/* DMA interrupt status register bits. */
#define QM_DMA_INT_STATUS_TFR BIT(0)
#define QM_DMA_INT_STATUS_BLOCK BIT(1)
#define QM_DMA_INT_STATUS_ERR BIT(4)

/** DMA miscellaneous register map. */
typedef struct {
	QM_RW uint32_t cfg_low;      /**< DmaCfgReg */
	QM_RW uint32_t cfg_high;     /**< DmaCfgReg */
	QM_RW uint32_t chan_en_low;  /**< ChEnReg */
	QM_RW uint32_t chan_en_high; /**< ChEnReg */
	QM_RW uint32_t id_low;       /**< DmaIdReg */
	QM_RW uint32_t id_high;      /**< DmaIdReg */
	QM_RW uint32_t test_low;     /**< DmaTestReg */
	QM_RW uint32_t test_high;    /**< DmaTestReg */
	QM_RW uint32_t reserved[4];  /**< Reserved */
} qm_dma_misc_reg_t;

/* Channel write enable in the misc channel enable register. */
#define QM_DMA_MISC_CHAN_EN_WE_OFFSET (8)

/* Controller enable bit in the misc config register. */
#define QM_DMA_MISC_CFG_DMA_EN BIT(0)

typedef struct {
	QM_RW qm_dma_chan_reg_t chan_reg[8]; /**< Channel Register */
	QM_RW qm_dma_int_reg_t int_reg;      /**< Interrupt Register */
	QM_RW uint32_t reserved[12];	 /**< Reserved (SW HS) */
	QM_RW qm_dma_misc_reg_t misc_reg;    /**< Miscellaneous Register */
} qm_dma_reg_t;

#if (UNIT_TEST)
qm_dma_reg_t test_dma_instance[QM_DMA_NUM];
qm_dma_reg_t *test_dma[QM_DMA_NUM];
#define QM_DMA test_dma
#else
#define QM_DMA_BASE (0xB0700000)
extern qm_dma_reg_t *qm_dma[QM_DMA_NUM];
#define QM_DMA qm_dma
#endif

/** @} */

/**
 * @name Hardware Fixes
 * @{
 */

/* Refer to "HARDWARE_ISSUES.rst" for fix description. */
#define FIX_1 (1)
#define FIX_2 (0)
#define FIX_3 (1)

/** @} */

/**
 * @name Versioning
 * @{
 */

#if (UNIT_TEST)
uint32_t test_rom_version;
#define ROM_VERSION_ADDRESS &test_rom_version;
#else
#define ROM_VERSION_ADDRESS                                                    \
	(BL_DATA_FLASH_REGION_BASE +                                           \
	 (BL_DATA_SECTION_BASE_PAGE * QM_FLASH_PAGE_SIZE_BYTES) +              \
	 sizeof(qm_flash_data_trim_t))
#endif

/** @} */

/** @} */

#endif /* __REGISTERS_H__ */
