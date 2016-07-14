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

/**
 * Quark SE SoC Registers.
 *
 * @defgroup groupQUARKSESEREG SoC Registers (SE)
 * @{
 */

#define QUARK_SE (1)
#define HAS_4_TIMERS (1)
#define HAS_AON_GPIO (1)
#define HAS_MAILBOX (1)

#if !defined(QM_SENSOR)
#define HAS_APIC (1)
#endif

/**
 * @name System Core
 * @{
 */

/** System Core register map. */
typedef struct {
	QM_RW uint32_t osc0_cfg0;    /**< Hybrid Oscillator Configuration 0 */
	QM_RW uint32_t osc0_stat1;   /**< Hybrid Oscillator status 1 */
	QM_RW uint32_t osc0_cfg1;    /**< Hybrid Oscillator configuration 1 */
	QM_RW uint32_t osc1_stat0;   /**< RTC Oscillator status 0 */
	QM_RW uint32_t osc1_cfg0;    /**< RTC Oscillator Configuration 0 */
	QM_RW uint32_t usb_pll_cfg0; /**< USB Phase lock look configuration */
	QM_RW uint32_t
	    ccu_periph_clk_gate_ctl; /**< Peripheral Clock Gate Control */
	QM_RW uint32_t
	    ccu_periph_clk_div_ctl0; /**< Peripheral Clock Divider Control 0 */
	QM_RW uint32_t
	    ccu_gpio_db_clk_ctl; /**< Peripheral Clock Divider Control 1 */
	QM_RW uint32_t
	    ccu_ext_clock_ctl; /**< External Clock Control Register */
	/** Sensor Subsystem peripheral clock gate control */
	QM_RW uint32_t ccu_ss_periph_clk_gate_ctl;
	QM_RW uint32_t ccu_lp_clk_ctl; /**< System Low Power Clock Control */
	QM_RW uint32_t reserved;
	QM_RW uint32_t ccu_mlayer_ahb_ctl; /**< AHB Control Register */
	QM_RW uint32_t ccu_sys_clk_ctl;    /**< System Clock Control Register */
	QM_RW uint32_t osc_lock_0;	 /**< Clocks Lock Register */
} qm_scss_ccu_reg_t;

#if (UNIT_TEST)
qm_scss_ccu_reg_t test_scss_ccu;
#define QM_SCSS_CCU ((qm_scss_ccu_reg_t *)(&test_scss_ccu))

#else
#define QM_SCSS_CCU_BASE (0xB0800000)
#define QM_SCSS_CCU ((qm_scss_ccu_reg_t *)QM_SCSS_CCU_BASE)
#endif

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

#define QM_OSC0_MODE_SEL BIT(3)
#define QM_OSC0_LOCK_SI BIT(0)
#define QM_OSC0_LOCK_XTAL BIT(1)
#define QM_OSC0_EN_SI_OSC BIT(1)

#define QM_SI_OSC_1V2_MODE BIT(0)

/** Peripheral clock divider control */
#define QM_CCU_PERIPH_PCLK_DIV_OFFSET (1)
#define QM_CCU_PERIPH_PCLK_DIV_EN BIT(0)

/* Clock enable / disable register. */
#define QM_CCU_MLAYER_AHB_CTL (REG_VAL(0xB0800034))

/* System clock control */
#define QM_CCU_SYS_CLK_SEL BIT(0)
#define QM_SCSS_CCU_SYS_CLK_SEL BIT(0)
#define QM_SCSS_CCU_C2_LP_EN BIT(1)
#define QM_SCSS_CCU_SS_LPS_EN BIT(0)
#define QM_CCU_RTC_CLK_EN BIT(1)
#define QM_CCU_RTC_CLK_DIV_EN BIT(2)
#define QM_CCU_SYS_CLK_DIV_EN BIT(7)
#define QM_CCU_SYS_CLK_DIV_MASK (0x00000300)

#define QM_OSC0_SI_FREQ_SEL_DEF_MASK (0xFFFFFCFF)
#define QM_CCU_GPIO_DB_DIV_OFFSET (2)
#define QM_CCU_GPIO_DB_CLK_DIV_EN BIT(1)
#define QM_CCU_GPIO_DB_CLK_EN BIT(0)
#define QM_CCU_RTC_CLK_DIV_OFFSET (3)
#define QM_CCU_SYS_CLK_DIV_OFFSET (8)
#define QM_CCU_DMA_CLK_EN BIT(6)

/** @} */

/**
 * @name General Purpose
 * @{
 */

/** General Purpose register map. */
typedef struct {
	QM_RW uint32_t gps0; /**< General Purpose Sticky Register 0 */
	QM_RW uint32_t gps1; /**< General Purpose Sticky Register 1 */
	QM_RW uint32_t gps2; /**< General Purpose Sticky Register 2 */
	QM_RW uint32_t gps3; /**< General Purpose Sticky Register 3 */
	QM_RW uint32_t reserved;
	QM_RW uint32_t gp0; /**< General Purpose Scratchpad Register 0 */
	QM_RW uint32_t gp1; /**< General Purpose Scratchpad Register 1 */
	QM_RW uint32_t gp2; /**< General Purpose Scratchpad Register 2 */
	QM_RW uint32_t gp3; /**< General Purpose Scratchpad Register 3 */
	QM_RW uint32_t reserved1;
	QM_RW uint32_t id;    /**< Identification Register */
	QM_RW uint32_t rev;   /**< Revision Register */
	QM_RW uint32_t wo_sp; /**< Write-One-to-Set Scratchpad Register */
	QM_RW uint32_t
	    wo_st; /**< Write-One-to-Set Sticky Scratchpad Register */
} qm_scss_gp_reg_t;

#if (UNIT_TEST)
qm_scss_gp_reg_t test_scss_gp;
#define QM_SCSS_GP ((qm_scss_gp_reg_t *)(&test_scss_gp))

#else
#define QM_SCSS_GP_BASE (0xB0800100)
#define QM_SCSS_GP ((qm_scss_gp_reg_t *)QM_SCSS_GP_BASE)
#endif

/** @} */

/**
 * @name Memory Control
 * @{
 */

/** Memory Control register map. */
typedef struct {
	QM_RW uint32_t mem_ctrl; /**< Memory control */
} qm_scss_mem_reg_t;

#if (UNIT_TEST)
qm_scss_mem_reg_t test_scss_mem;
#define QM_SCSS_MEM ((qm_scss_mem_reg_t *)(&test_scss_mem))

#else
#define QM_SCSS_MEM_BASE (0xB0800200)
#define QM_SCSS_MEM ((qm_scss_mem_reg_t *)QM_SCSS_MEM_BASE)
#endif

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
 * @name APIC
 * @{
 */

typedef struct {
	QM_RW uint32_t reg;
	QM_RW uint32_t pad[3];
} apic_reg_pad_t;

/** APIC register block type. */
typedef struct {
	QM_RW apic_reg_pad_t reserved0[2];
	QM_RW apic_reg_pad_t id;      /**< LAPIC ID */
	QM_RW apic_reg_pad_t version; /**< LAPIC version*/
	QM_RW apic_reg_pad_t reserved1[4];
	QM_RW apic_reg_pad_t tpr;    /**< Task priority*/
	QM_RW apic_reg_pad_t apr;    /**< Arbitration priority */
	QM_RW apic_reg_pad_t ppr;    /**< Processor priority */
	QM_RW apic_reg_pad_t eoi;    /**< End of interrupt */
	QM_RW apic_reg_pad_t rrd;    /**< Remote read */
	QM_RW apic_reg_pad_t ldr;    /**< Logical destination */
	QM_RW apic_reg_pad_t dfr;    /**< Destination format */
	QM_RW apic_reg_pad_t svr;    /**< Spurious vector */
	QM_RW apic_reg_pad_t isr[8]; /**< In-service */
	QM_RW apic_reg_pad_t tmr[8]; /**< Trigger mode */
	QM_RW apic_reg_pad_t irr[8]; /**< Interrupt request */
	QM_RW apic_reg_pad_t esr;    /**< Error status */
	QM_RW apic_reg_pad_t reserved2[6];
	QM_RW apic_reg_pad_t lvtcmci;   /**< Corrected Machine Check vector */
	QM_RW apic_reg_pad_t icr[2];    /**< Interrupt command */
	QM_RW apic_reg_pad_t lvttimer;  /**< Timer vector */
	QM_RW apic_reg_pad_t lvtts;     /**< Thermal sensor vector */
	QM_RW apic_reg_pad_t lvtpmcr;   /**< Perfmon counter vector */
	QM_RW apic_reg_pad_t lvtlint0;  /**< Local interrupt 0 vector */
	QM_RW apic_reg_pad_t lvtlint1;  /**< Local interrupt 1 vector */
	QM_RW apic_reg_pad_t lvterr;    /**< Error vector */
	QM_RW apic_reg_pad_t timer_icr; /**< Timer initial count */
	QM_RW apic_reg_pad_t timer_ccr; /**< Timer current count */
	QM_RW apic_reg_pad_t reserved3[4];
	QM_RW apic_reg_pad_t timer_dcr; /**< Timer divide configuration */
} qm_lapic_reg_t;

#if (UNIT_TEST)
qm_lapic_reg_t test_lapic;
#define QM_LAPIC ((qm_lapic_reg_t *)(&test_lapic))

#else
/** Local APIC */
#define QM_LAPIC_BASE (0xFEE00000)
#define QM_LAPIC ((qm_lapic_reg_t *)QM_LAPIC_BASE)
#endif

#define QM_INT_CONTROLLER QM_LAPIC

/*
 * Quark SE has a HW limitation that prevents a LAPIC EOI from being broadcast
 * into IOAPIC. To trigger this manually we must write the vector number being
 * serviced into the IOAPIC EOI register.
 */
#if defined(ISR_HANDLED) || defined(QM_SENSOR)
#define QM_ISR_EOI(vector)
#else
#define QM_ISR_EOI(vector)                                                     \
	do {                                                                   \
		QM_INT_CONTROLLER->eoi.reg = 0;                                \
		QM_IOAPIC->eoi.reg = vector;                                   \
	} while (0)
#endif

typedef struct {
	QM_RW apic_reg_pad_t ioregsel; /**< Register selector */
	QM_RW apic_reg_pad_t iowin;    /**< Register window */
	QM_RW apic_reg_pad_t reserved[2];
	QM_RW apic_reg_pad_t eoi; /**< EOI register */
} qm_ioapic_reg_t;

#define QM_IOAPIC_REG_VER (0x01)    /**< IOAPIC version */
#define QM_IOAPIC_REG_REDTBL (0x10) /**< Redirection table base */

#if (UNIT_TEST)
qm_ioapic_reg_t test_ioapic;
#define QM_IOAPIC ((qm_ioapic_reg_t *)(&test_ioapic))

#else
/** IO / APIC base address. */
#define QM_IOAPIC_BASE (0xFEC00000)
#define QM_IOAPIC ((qm_ioapic_reg_t *)QM_IOAPIC_BASE)
#endif

/** @} */

/**
 * @name Interrupt
 * @{
 */

/** SS I2C Interrupt register map. */
typedef struct {
	QM_RW uint32_t err_mask;
	QM_RW uint32_t rx_avail_mask;
	QM_RW uint32_t tx_req_mask;
	QM_RW uint32_t stop_det_mask;
} int_ss_i2c_reg_t;

/** SS SPI Interrupt register map. */
typedef struct {
	QM_RW uint32_t err_int_mask;
	QM_RW uint32_t rx_avail_mask;
	QM_RW uint32_t tx_req_mask;
} int_ss_spi_reg_t;

/** Interrupt register map. */
typedef struct {
	QM_RW uint32_t int_ss_adc_err_mask;
	QM_RW uint32_t int_ss_adc_irq_mask;
	QM_RW uint32_t int_ss_gpio_0_intr_mask;
	QM_RW uint32_t int_ss_gpio_1_intr_mask;
	int_ss_i2c_reg_t int_ss_i2c_0;
	int_ss_i2c_reg_t int_ss_i2c_1;
	int_ss_spi_reg_t int_ss_spi_0;
	int_ss_spi_reg_t int_ss_spi_1;
	QM_RW uint32_t int_i2c_mst_0_mask;
	QM_RW uint32_t int_i2c_mst_1_mask;
	QM_RW uint32_t reserved;
	QM_RW uint32_t int_spi_mst_0_mask;
	QM_RW uint32_t int_spi_mst_1_mask;
	QM_RW uint32_t int_spi_slv_mask;
	QM_RW uint32_t int_uart_0_mask;
	QM_RW uint32_t int_uart_1_mask;
	QM_RW uint32_t int_i2s_mask;
	QM_RW uint32_t int_gpio_mask;
	QM_RW uint32_t int_pwm_timer_mask;
	QM_RW uint32_t int_usb_mask;
	QM_RW uint32_t int_rtc_mask;
	QM_RW uint32_t int_watchdog_mask;
	QM_RW uint32_t int_dma_channel_0_mask;
	QM_RW uint32_t int_dma_channel_1_mask;
	QM_RW uint32_t int_dma_channel_2_mask;
	QM_RW uint32_t int_dma_channel_3_mask;
	QM_RW uint32_t int_dma_channel_4_mask;
	QM_RW uint32_t int_dma_channel_5_mask;
	QM_RW uint32_t int_dma_channel_6_mask;
	QM_RW uint32_t int_dma_channel_7_mask;
	QM_RW uint32_t int_mailbox_mask;
	QM_RW uint32_t int_comparators_ss_halt_mask;
	QM_RW uint32_t int_comparators_host_halt_mask;
	QM_RW uint32_t int_comparators_ss_mask;
	QM_RW uint32_t int_comparators_host_mask;
	QM_RW uint32_t int_host_bus_err_mask;
	QM_RW uint32_t int_dma_error_mask;
	QM_RW uint32_t
	    int_sram_controller_mask; /**< Interrupt Routing Mask 28 */
	QM_RW uint32_t
	    int_flash_controller_0_mask; /**< Interrupt Routing Mask 29 */
	QM_RW uint32_t
	    int_flash_controller_1_mask;   /**< Interrupt Routing Mask 30 */
	QM_RW uint32_t int_aon_timer_mask; /**< Interrupt Routing Mask 31 */
	QM_RW uint32_t int_adc_pwr_mask;   /**< Interrupt Routing Mask 32 */
	QM_RW uint32_t int_adc_calib_mask; /**< Interrupt Routing Mask 33 */
	QM_RW uint32_t int_aon_gpio_mask;
	QM_RW uint32_t lock_int_mask_reg; /**< Interrupt Mask Lock Register */
} qm_scss_int_reg_t;

/** Number of SCSS interrupt mask registers (excluding mask lock register) */
#define QM_SCSS_INT_MASK_NUMREG                                                \
	((sizeof(qm_scss_int_reg_t) / sizeof(uint32_t)) - 1)

/** Default POR SCSS interrupt mask (all interrupts masked) */
#define QM_SCSS_INT_MASK_DEFAULT (0xFFFFFFFF)

#if (UNIT_TEST)
qm_scss_int_reg_t test_scss_int;
#define QM_SCSS_INT ((qm_scss_int_reg_t *)(&test_scss_int))

#else
/* System control subsystem interrupt masking register block. */
#define QM_SCSS_INT_BASE (0xB0800400)
#define QM_SCSS_INT ((qm_scss_int_reg_t *)QM_SCSS_INT_BASE)
#endif

#define QM_INT_SRAM_CONTROLLER_HOST_HALT_MASK BIT(16)
#define QM_INT_SRAM_CONTROLLER_HOST_MASK BIT(0)
#define QM_INT_SRAM_CONTROLLER_SS_HALT_MASK BIT(24)
#define QM_INT_SRAM_CONTROLLER_SS_MASK BIT(8)
#define QM_INT_FLASH_CONTROLLER_HOST_HALT_MASK BIT(16)
#define QM_INT_FLASH_CONTROLLER_HOST_MASK BIT(0)
#define QM_INT_FLASH_CONTROLLER_SS_HALT_MASK BIT(24)
#define QM_INT_FLASH_CONTROLLER_SS_MASK BIT(8)
#define QM_INT_ADC_PWR_MASK BIT(8)
#define QM_INT_ADC_CALIB_MASK BIT(8)

/** @} */

/**
 * @name Power Management
 * @{
 */

/** Power Management register map. */
typedef struct {
	QM_RW uint32_t p_lvl2; /**< Processor level 2 */
	QM_RW uint32_t reserved[4];
	QM_RW uint32_t pm1c; /**< Power management 1 control */
	QM_RW uint32_t reserved1[9];
	QM_RW uint32_t aon_vr;     /**< AON Voltage Regulator */
	QM_RW uint32_t plat3p3_vr; /**< Platform 3p3 voltage regulator */
	QM_RW uint32_t plat1p8_vr; /**< Platform 1p8 voltage regulator */
	QM_RW uint32_t host_vr;    /**< Host Voltage Regulator */
	QM_RW uint32_t slp_cfg;    /**< Sleeping Configuration */
	/** Power Management Network (PMNet) Control and Status */
	QM_RW uint32_t pmnetcs;
	QM_RW uint32_t pm_wait; /**< Power Management Wait */
	QM_RW uint32_t reserved2;
	QM_RW uint32_t p_sts; /**< Processor Status */
	QM_RW uint32_t reserved3[3];
	QM_RW uint32_t rstc; /**< Reset Control */
	QM_RW uint32_t rsts; /**< Reset Status */
	QM_RW uint32_t reserved4[6];
	QM_RW uint32_t vr_lock; /**< Voltage regulator lock */
	QM_RW uint32_t pm_lock; /**< Power Management Lock */
} qm_scss_pmu_reg_t;

#if (UNIT_TEST)
qm_scss_pmu_reg_t test_scss_pmu;
#define QM_SCSS_PMU ((qm_scss_pmu_reg_t *)(&test_scss_pmu))

#else
#define QM_SCSS_PMU_BASE (0xB0800504)
#define QM_SCSS_PMU ((qm_scss_pmu_reg_t *)QM_SCSS_PMU_BASE)
#endif

#define QM_SS_CFG_ARC_RUN_REQ_A BIT(24)
#define QM_P_STS_HALT_INTERRUPT_REDIRECTION BIT(26)
#define QM_P_STS_ARC_HALT BIT(14)

/** @} */

/**
 * @name Sensor Subsystem
 * @{
 */

#define QM_SCSS_SLP_CFG_LPMODE_EN BIT(8)
#define QM_SCSS_SLP_CFG_RTC_DIS BIT(7)
#define QM_SCSS_PM1C_SLPEN BIT(13)
#define QM_SCSS_HOST_VR_EN BIT(7)
#define QM_SCSS_PLAT3P3_VR_EN BIT(7)
#define QM_SCSS_PLAT1P8_VR_EN BIT(7)
#define QM_SCSS_HOST_VR_VREG_SEL BIT(6)
#define QM_SCSS_PLAT3P3_VR_VREG_SEL BIT(6)
#define QM_SCSS_PLAT1P8_VR_VREG_SEL BIT(6)
#define QM_SCSS_VR_ROK BIT(10)
#define QM_SCSS_VR_EN BIT(7)
#define QM_SCSS_VR_VREG_SEL BIT(6)

/** Sensor Subsystem register map. */
typedef struct {
	QM_RW uint32_t ss_cfg; /**< Sensor Subsystem Configuration */
	QM_RW uint32_t ss_sts; /**< Sensor Subsystem status */
} qm_scss_ss_reg_t;

#if (UNIT_TEST)
qm_scss_ss_reg_t test_scss_ss;
#define QM_SCSS_SS ((qm_scss_ss_reg_t *)(&test_scss_ss))

#else
#define QM_SCSS_SS_BASE (0xB0800600)
#define QM_SCSS_SS ((qm_scss_ss_reg_t *)QM_SCSS_SS_BASE)
#endif

#define QM_SS_STS_HALT_INTERRUPT_REDIRECTION BIT(26)

/** @} */

/**
 * @name Always-on controllers.
 * @{
 */

#define QM_AON_VR_VSEL_MASK (0xFFE0)
#define QM_AON_VR_VSEL_1V2 (0x8)
#define QM_AON_VR_VSEL_1V35 (0xB)
#define QM_AON_VR_VSEL_1V8 (0x10)
#define QM_AON_VR_EN BIT(7)
#define QM_AON_VR_VSTRB BIT(5)

/** Number of SCSS Always-on controllers. */
typedef enum { QM_SCSS_AON_0 = 0, QM_SCSS_AON_NUM } qm_scss_aon_t;

/** Always-on Controller register map. */
typedef struct {
	QM_RW uint32_t aonc_cnt;  /**< Always-on counter register. */
	QM_RW uint32_t aonc_cfg;  /**< Always-on counter enable. */
	QM_RW uint32_t aonpt_cnt; /**< Always-on periodic timer. */
	QM_RW uint32_t
	    aonpt_stat; /**< Always-on periodic timer status register. */
	QM_RW uint32_t aonpt_ctrl; /**< Always-on periodic timer control. */
	QM_RW uint32_t
	    aonpt_cfg; /**< Always-on periodic timer configuration register. */
} qm_scss_aon_reg_t;

#if (UNIT_TEST)
qm_scss_aon_reg_t test_scss_aon;
#define QM_SCSS_AON ((qm_scss_aon_reg_t *)(&test_scss_aon))

#else
#define QM_SCSS_AON_BASE (0xB0800700)
#define QM_SCSS_AON ((qm_scss_aon_reg_t *)QM_SCSS_AON_BASE)
#endif

/** @} */

/**
 * @name Peripheral Registers
 * @{
 */

/** Peripheral Registers register map. */
typedef struct {
	QM_RW uint32_t usb_phy_cfg0; /**< USB Configuration */
	QM_RW uint32_t periph_cfg0;  /**< Peripheral Configuration */
	QM_RW uint32_t reserved[2];
	QM_RW uint32_t cfg_lock; /**< Configuration Lock */
} qm_scss_peripheral_reg_t;

#if (UNIT_TEST)
qm_scss_peripheral_reg_t test_scss_peripheral;
#define QM_SCSS_PERIPHERAL ((qm_scss_peripheral_reg_t *)(&test_scss_peripheral))

#else
#define QM_SCSS_PERIPHERAL_BASE (0xB0800800)
#define QM_SCSS_PERIPHERAL ((qm_scss_peripheral_reg_t *)QM_SCSS_PERIPHERAL_BASE)
#endif

/** @} */

/**
 * @name Pin MUX
 * @{
 */

/** Pin MUX register map. */
typedef struct {
	QM_RW uint32_t pmux_pullup[4]; /**< Pin Mux Pullup */
	QM_RW uint32_t pmux_slew[4];   /**< Pin Mux Slew Rate */
	QM_RW uint32_t pmux_in_en[4];  /**< Pin Mux Input Enable */
	QM_RW uint32_t pmux_sel[5];    /**< Pin Mux Select */
	QM_RW uint32_t reserved[2];
	QM_RW uint32_t pmux_pullup_lock; /**< Pin Mux Pullup Lock */
	QM_RW uint32_t pmux_slew_lock;   /**< Pin Mux Slew Rate Lock */
	QM_RW uint32_t pmux_sel_lock[3]; /**< Pin Mux Select Lock */
	QM_RW uint32_t pmux_in_en_lock;  /**< Pin Mux Slew Rate Lock */
} qm_scss_pmux_reg_t;

#if (UNIT_TEST)
qm_scss_pmux_reg_t test_scss_pmux;
#define QM_SCSS_PMUX ((qm_scss_pmux_reg_t *)(&test_scss_pmux))

#else
#define QM_SCSS_PMUX_BASE (0xB0800900)
#define QM_SCSS_PMUX ((qm_scss_pmux_reg_t *)QM_SCSS_PMUX_BASE)
#endif

/* Pin MUX slew rate registers and settings */
#define QM_PMUX_SLEW_4MA_DRIVER (0xFFFFFFFF)
#define QM_PMUX_SLEW0 (REG_VAL(0xB0800910))
#define QM_PMUX_SLEW1 (REG_VAL(0xB0800914))
#define QM_PMUX_SLEW2 (REG_VAL(0xB0800918))
#define QM_PMUX_SLEW3 (REG_VAL(0xB080091C))

/** @} */

/**
 * @name ID
 * @{
 */

/** Information register map. */
typedef struct {
	QM_RW uint32_t id;
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
 * @name Mailbox
 * @{
 */

#define QM_MBOX_TRIGGER_CH_INT BIT(31)

/** Mailbox register structure. */
typedef struct {
	QM_RW uint32_t ch_ctrl;    /**< Channel Control Word */
	QM_RW uint32_t ch_data[4]; /**< Channel Payload Data Word 0 */
	QM_RW uint32_t ch_sts;     /**< Channel status */
} qm_mailbox_t;

/** Mailbox register map. */
typedef struct {
	qm_mailbox_t mbox[8];	  /**< 8 Mailboxes */
	QM_RW uint32_t mbox_chall_sts; /**< All channel status */
} qm_scss_mailbox_reg_t;

#if (UNIT_TEST)
qm_scss_mailbox_reg_t test_scss_mailbox;
#define QM_SCSS_MAILBOX ((qm_scss_mailbox_reg_t *)(&test_scss_mailbox))

#else
#define QM_SCSS_MAILBOX_BASE (0xB0800A00)
#define QM_SCSS_MAILBOX ((qm_scss_mailbox_reg_t *)QM_SCSS_MAILBOX_BASE)
#endif

/** @} */

/**
 * @name IRQs and Interrupts
 * @{
 */

/* IRQs and interrupt vectors.
 *
 * Any IRQ > 1 actually has a SCSS mask register offset of +1.
 * The vector numbers must be defined without arithmetic expressions nor
 * parentheses because they are expanded as token concatenation.
 */

#define QM_INT_VECTOR_DOUBLE_FAULT 8
#define QM_INT_VECTOR_PIC_TIMER 32

#define QM_IRQ_RTC_0 11
#define QM_IRQ_RTC_0_MASK_OFFSET 12
#define QM_IRQ_RTC_0_VECTOR 47

#define QM_IRQ_PWM_0 9
#define QM_IRQ_PWM_0_MASK_OFFSET 10
#define QM_IRQ_PWM_0_VECTOR 45

#define QM_IRQ_USB_0 (10)
#define QM_IRQ_USB_0_MASK_OFFSET (11)
#define QM_IRQ_USB_0_VECTOR 46

#define QM_IRQ_SPI_MASTER_0 2
#define QM_IRQ_SPI_MASTER_0_MASK_OFFSET 3
#define QM_IRQ_SPI_MASTER_0_VECTOR 38

#define QM_IRQ_SPI_MASTER_1 3
#define QM_IRQ_SPI_MASTER_1_MASK_OFFSET 4
#define QM_IRQ_SPI_MASTER_1_VECTOR 39

#define QM_IRQ_WDT_0 12
#define QM_IRQ_WDT_0_MASK_OFFSET 13
#define QM_IRQ_WDT_0_VECTOR 48

#define QM_IRQ_GPIO_0 8
#define QM_IRQ_GPIO_0_MASK_OFFSET 9
#define QM_IRQ_GPIO_0_VECTOR 44

#define QM_IRQ_I2C_0 0
#define QM_IRQ_I2C_0_MASK_OFFSET 0
#define QM_IRQ_I2C_0_VECTOR 36

#define QM_IRQ_I2C_1 1
#define QM_IRQ_I2C_1_MASK_OFFSET 1
#define QM_IRQ_I2C_1_VECTOR 37

#define QM_IRQ_MBOX 21
#define QM_IRQ_MBOX_MASK_OFFSET 22
#define QM_IRQ_MBOX_VECTOR 57

#define QM_IRQ_AC 22
#define QM_IRQ_AC_MASK_OFFSET 26
#define QM_IRQ_AC_VECTOR 58

#define QM_IRQ_SRAM 25
#define QM_IRQ_SRAM_MASK_OFFSET 29
#define QM_IRQ_SRAM_VECTOR 61

#define QM_IRQ_FLASH_0 26
#define QM_IRQ_FLASH_0_MASK_OFFSET 30
#define QM_IRQ_FLASH_0_VECTOR 62

#define QM_IRQ_FLASH_1 27
#define QM_IRQ_FLASH_1_MASK_OFFSET 31
#define QM_IRQ_FLASH_1_VECTOR 63

#define QM_IRQ_AONPT_0 28
#define QM_IRQ_AONPT_0_MASK_OFFSET 32
#define QM_IRQ_AONPT_0_VECTOR 64

#define QM_IRQ_AONGPIO_0 31
#define QM_IRQ_AONGPIO_0_MASK_OFFSET 35
#define QM_IRQ_AONGPIO_0_VECTOR 67

#define QM_IRQ_UART_0 5
#define QM_IRQ_UART_0_MASK_OFFSET 6
#define QM_IRQ_UART_0_VECTOR 41

#define QM_IRQ_UART_1 6
#define QM_IRQ_UART_1_MASK_OFFSET 7
#define QM_IRQ_UART_1_VECTOR 42

#define QM_IRQ_DMA_0 13
#define QM_IRQ_DMA_0_MASK_OFFSET 14
#define QM_IRQ_DMA_0_VECTOR 49

#define QM_IRQ_DMA_1 14
#define QM_IRQ_DMA_1_MASK_OFFSET 15
#define QM_IRQ_DMA_1_VECTOR 50

#define QM_IRQ_DMA_2 15
#define QM_IRQ_DMA_2_MASK_OFFSET 16
#define QM_IRQ_DMA_2_VECTOR 51

#define QM_IRQ_DMA_3 16
#define QM_IRQ_DMA_3_MASK_OFFSET 17
#define QM_IRQ_DMA_3_VECTOR 52

#define QM_IRQ_DMA_4 17
#define QM_IRQ_DMA_4_MASK_OFFSET 18
#define QM_IRQ_DMA_4_VECTOR 53

#define QM_IRQ_DMA_5 18
#define QM_IRQ_DMA_5_MASK_OFFSET 19
#define QM_IRQ_DMA_5_VECTOR 54

#define QM_IRQ_DMA_6 19
#define QM_IRQ_DMA_6_MASK_OFFSET 20
#define QM_IRQ_DMA_6_VECTOR 55

#define QM_IRQ_DMA_7 20
#define QM_IRQ_DMA_7_MASK_OFFSET 21
#define QM_IRQ_DMA_7_VECTOR 56

#define QM_IRQ_DMA_ERR 24
#define QM_IRQ_DMA_ERR_MASK_OFFSET 28
#define QM_IRQ_DMA_ERR_VECTOR 60

#define QM_SS_IRQ_ADC_PWR 29
#define QM_SS_IRQ_ADC_PWR_MASK_OFFSET 33
#define QM_SS_IRQ_ADC_PWR_VECTOR 65

#define QM_SS_IRQ_ADC_CAL 30
#define QM_SS_IRQ_ADC_CAL_MASK_OFFSET 34
#define QM_SS_IRQ_ADC_CAL_VECTOR 66

/** @} */

/**
 * @name PWM / Timer
 * @{
 */

/** Number of PWM / Timer controllers. */
typedef enum { QM_PWM_0 = 0, QM_PWM_NUM } qm_pwm_t;

/** PWM ID type. */
typedef enum {
	QM_PWM_ID_0 = 0,
	QM_PWM_ID_1,
	QM_PWM_ID_2,
	QM_PWM_ID_3,
	QM_PWM_ID_NUM
} qm_pwm_id_t;

/** PWM / Timer channel register map. */
typedef struct {
	QM_RW uint32_t loadcount;    /**< Load Count */
	QM_RW uint32_t currentvalue; /**< Current Value */
	QM_RW uint32_t controlreg;   /**< Control */
	QM_RW uint32_t eoi;	  /**< End Of Interrupt */
	QM_RW uint32_t intstatus;    /**< Interrupt Status */
} qm_pwm_channel_t;

/** PWM / Timer register map. */
typedef struct {
	qm_pwm_channel_t timer[QM_PWM_ID_NUM]; /**< 4 Timers */
	QM_RW uint32_t reserved[20];
	QM_RW uint32_t timersintstatus;     /**< Timers Interrupt Status */
	QM_RW uint32_t timerseoi;	   /**< Timers End Of Interrupt */
	QM_RW uint32_t timersrawintstatus;  /**< Timers Raw Interrupt Status */
	QM_RW uint32_t timerscompversion;   /**< Timers Component Version */
	QM_RW uint32_t timer_loadcount2[4]; /**< Timer Load Count 2 */
} qm_pwm_reg_t;

#if (UNIT_TEST)
qm_pwm_reg_t test_pwm_t;
#define QM_PWM ((qm_pwm_reg_t *)(&test_pwm_t))

#else
/** PWM register base address */
#define QM_PWM_BASE (0xB0000800)
/** PWM register block */
#define QM_PWM ((qm_pwm_reg_t *)QM_PWM_BASE)
#endif

#define QM_PWM_INTERRUPT_MASK_OFFSET (2)

/** @} */

/**
 * @name WDT
 * @{
 */

/** Number of WDT controllers. */
typedef enum { QM_WDT_0 = 0, QM_WDT_NUM } qm_wdt_t;

/** Watchdog timer register map. */
typedef struct {
	QM_RW uint32_t wdt_cr;		 /**< Control Register */
	QM_RW uint32_t wdt_torr;	 /**< Timeout Range Register */
	QM_RW uint32_t wdt_ccvr;	 /**< Current Counter Value Register */
	QM_RW uint32_t wdt_crr;		 /**< Current Restart Register */
	QM_RW uint32_t wdt_stat;	 /**< Interrupt Status Register */
	QM_RW uint32_t wdt_eoi;		 /**< Interrupt Clear Register */
	QM_RW uint32_t wdt_comp_param_5; /**<  Component Parameters */
	QM_RW uint32_t wdt_comp_param_4; /**<  Component Parameters */
	QM_RW uint32_t wdt_comp_param_3; /**<  Component Parameters */
	QM_RW uint32_t wdt_comp_param_2; /**<  Component Parameters */
	QM_RW uint32_t
	    wdt_comp_param_1; /**<  Component Parameters Register 1 */
	QM_RW uint32_t wdt_comp_version; /**<  Component Version Register */
	QM_RW uint32_t wdt_comp_type;    /**< Component Type Register */
} qm_wdt_reg_t;

#if (UNIT_TEST)
qm_wdt_reg_t test_wdt;
#define QM_WDT ((qm_wdt_reg_t *)(&test_wdt))

#else
/** WDT register base address */
#define QM_WDT_BASE (0xB0000000)

/** WDT register block */
#define QM_WDT ((qm_wdt_reg_t *)QM_WDT_BASE)
#endif

/** @} */

/**
 * @name UART
 * @{
 */

/** Number of UART controllers. */
typedef enum { QM_UART_0 = 0, QM_UART_1, QM_UART_NUM } qm_uart_t;

/** UART register map. */
typedef struct {
	QM_RW uint32_t rbr_thr_dll; /**< Rx Buffer/ Tx Holding/ Div Latch Low */
	QM_RW uint32_t ier_dlh; /**< Interrupt Enable / Divisor Latch High */
	QM_RW uint32_t iir_fcr; /**< Interrupt Identification / FIFO Control */
	QM_RW uint32_t lcr;     /**< Line Control */
	QM_RW uint32_t mcr;     /**< MODEM Control */
	QM_RW uint32_t lsr;     /**< Line Status */
	QM_RW uint32_t msr;     /**< MODEM Status */
	QM_RW uint32_t scr;     /**< Scratchpad */
	QM_RW uint32_t reserved[23];
	QM_RW uint32_t usr; /**< UART Status */
	QM_RW uint32_t reserved1[9];
	QM_RW uint32_t htx;   /**< Halt Transmission */
	QM_RW uint32_t dmasa; /**< DMA Software Acknowledge */
	QM_RW uint32_t reserved2[5];
	QM_RW uint32_t dlf;	   /**< Divisor Latch Fraction */
	QM_RW uint32_t padding[0xCF]; /* (0x400 - 0xC4) / 4 */
} qm_uart_reg_t;

#if (UNIT_TEST)
qm_uart_reg_t test_uart_instance;
qm_uart_reg_t *test_uart[QM_UART_NUM];
#define QM_UART test_uart

#else
/** UART register base address */
#define QM_UART_0_BASE (0xB0002000)
#define QM_UART_1_BASE (0xB0002400)
/** UART register block */
extern qm_uart_reg_t *qm_uart[QM_UART_NUM];
#define QM_UART qm_uart
#endif

/** @} */

/**
 * @name SPI
 * @{
 */

/** Number of SPI controllers. */
typedef enum {
	QM_SPI_MST_0 = 0,
	QM_SPI_MST_1,
	QM_SPI_SLV_0,
	QM_SPI_NUM
} qm_spi_t;

/** SPI register map. */
typedef struct {
	QM_RW uint32_t ctrlr0; /**< Control Register 0 */
	QM_RW uint32_t ctrlr1; /**< Control Register 1 */
	QM_RW uint32_t ssienr; /**< SSI Enable Register */
	QM_RW uint32_t mwcr;   /**< Microwire Control Register */
	QM_RW uint32_t ser;    /**< Slave Enable Register */
	QM_RW uint32_t baudr;  /**< Baud Rate Select */
	QM_RW uint32_t txftlr; /**< Transmit FIFO Threshold Level */
	QM_RW uint32_t rxftlr; /**< Receive FIFO Threshold Level */
	QM_RW uint32_t txflr;  /**< Transmit FIFO Level Register */
	QM_RW uint32_t rxflr;  /**< Receive FIFO Level Register */
	QM_RW uint32_t sr;     /**< Status Register */
	QM_RW uint32_t imr;    /**< Interrupt Mask Register */
	QM_RW uint32_t isr;    /**< Interrupt Status Register */
	QM_RW uint32_t risr;   /**< Raw Interrupt Status Register */
	QM_RW uint32_t txoicr; /**< Tx FIFO Overflow Interrupt Clear Register*/
	QM_RW uint32_t rxoicr; /**< Rx FIFO Overflow Interrupt Clear Register */
	QM_RW uint32_t rxuicr; /**< Rx FIFO Underflow Interrupt Clear Register*/
	QM_RW uint32_t msticr; /**< Multi-Master Interrupt Clear Register */
	QM_RW uint32_t icr;    /**< Interrupt Clear Register */
	QM_RW uint32_t dmacr;  /**< DMA Control Register */
	QM_RW uint32_t dmatdlr;		 /**< DMA Transmit Data Level */
	QM_RW uint32_t dmardlr;		 /**< DMA Receive Data Level */
	QM_RW uint32_t idr;		 /**< Identification Register */
	QM_RW uint32_t ssi_comp_version; /**< coreKit Version ID register */
	QM_RW uint32_t dr[36];		 /**< Data Register */
	QM_RW uint32_t rx_sample_dly;    /**< RX Sample Delay Register */
	QM_RW uint32_t padding[0xC4];    /* (0x400 - 0xF0) / 4 */
} qm_spi_reg_t;

#if (UNIT_TEST)
qm_spi_reg_t test_spi;
qm_spi_reg_t *test_spi_controllers[QM_SPI_NUM];

#define QM_SPI test_spi_controllers

#else
/** SPI Master register base address */
#define QM_SPI_MST_0_BASE (0xB0001000)
#define QM_SPI_MST_1_BASE (0xB0001400)
extern qm_spi_reg_t *qm_spi_controllers[QM_SPI_NUM];
#define QM_SPI qm_spi_controllers

/** SPI Slave register base address */
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
	QM_RW uint32_t rtc_ccvr;	 /**< Current Counter Value Register */
	QM_RW uint32_t rtc_cmr;		 /**< Current Match Register */
	QM_RW uint32_t rtc_clr;		 /**< Counter Load Register */
	QM_RW uint32_t rtc_ccr;		 /**< Counter Control Register */
	QM_RW uint32_t rtc_stat;	 /**< Interrupt Status Register */
	QM_RW uint32_t rtc_rstat;	/**< Interrupt Raw Status Register */
	QM_RW uint32_t rtc_eoi;		 /**< End of Interrupt Register */
	QM_RW uint32_t rtc_comp_version; /**< End of Interrupt Register */
} qm_rtc_reg_t;

#if (UNIT_TEST)
qm_rtc_reg_t test_rtc;
#define QM_RTC ((qm_rtc_reg_t *)(&test_rtc))

#else
/** RTC register base address. */
#define QM_RTC_BASE (0xB0000400)

/** RTC register block. */
#define QM_RTC ((qm_rtc_reg_t *)QM_RTC_BASE)
#endif

/** @} */

/**
 * @name I2C
 * @{
 */

/** Number of I2C controllers. */
typedef enum { QM_I2C_0 = 0, QM_I2C_1, QM_I2C_NUM } qm_i2c_t;

/** I2C register map. */
typedef struct {
	QM_RW uint32_t ic_con;      /**< Control Register */
	QM_RW uint32_t ic_tar;      /**< Master Target Address */
	QM_RW uint32_t ic_sar;      /**< Slave Address */
	QM_RW uint32_t ic_hs_maddr; /**< High Speed Master ID */
	QM_RW uint32_t ic_data_cmd; /**< Data Buffer and Command */
	QM_RW uint32_t
	    ic_ss_scl_hcnt; /**< Standard Speed Clock SCL High Count */
	QM_RW uint32_t
	    ic_ss_scl_lcnt; /**< Standard Speed Clock SCL Low Count */
	QM_RW uint32_t ic_fs_scl_hcnt; /**< Fast Speed Clock SCL High Count */
	QM_RW uint32_t
	    ic_fs_scl_lcnt; /**< Fast Speed I2C Clock SCL Low Count */
	QM_RW uint32_t
	    ic_hs_scl_hcnt; /**< High Speed I2C Clock SCL High Count */
	QM_RW uint32_t
	    ic_hs_scl_lcnt;	  /**< High Speed I2C Clock SCL Low Count */
	QM_RW uint32_t ic_intr_stat; /**< Interrupt Status */
	QM_RW uint32_t ic_intr_mask; /**< Interrupt Mask */
	QM_RW uint32_t ic_raw_intr_stat; /**< Raw Interrupt Status */
	QM_RW uint32_t ic_rx_tl;	 /**< Receive FIFO Threshold Level */
	QM_RW uint32_t ic_tx_tl;	 /**< Transmit FIFO Threshold Level */
	QM_RW uint32_t
	    ic_clr_intr; /**< Clear Combined and Individual Interrupt */
	QM_RW uint32_t ic_clr_rx_under;   /**< Clear RX_UNDER Interrupt */
	QM_RW uint32_t ic_clr_rx_over;    /**< Clear RX_OVER Interrupt */
	QM_RW uint32_t ic_clr_tx_over;    /**< Clear TX_OVER Interrupt */
	QM_RW uint32_t ic_clr_rd_req;     /**< Clear RD_REQ Interrupt */
	QM_RW uint32_t ic_clr_tx_abrt;    /**< Clear TX_ABRT Interrupt */
	QM_RW uint32_t ic_clr_rx_done;    /**< Clear RX_DONE Interrupt */
	QM_RW uint32_t ic_clr_activity;   /**< Clear ACTIVITY Interrupt */
	QM_RW uint32_t ic_clr_stop_det;   /**< Clear STOP_DET Interrupt */
	QM_RW uint32_t ic_clr_start_det;  /**< Clear START_DET Interrupt */
	QM_RW uint32_t ic_clr_gen_call;   /**< Clear GEN_CALL Interrupt */
	QM_RW uint32_t ic_enable;	 /**< Enable */
	QM_RW uint32_t ic_status;	 /**< Status */
	QM_RW uint32_t ic_txflr;	  /**< Transmit FIFO Level */
	QM_RW uint32_t ic_rxflr;	  /**< Receive FIFO Level */
	QM_RW uint32_t ic_sda_hold;       /**< SDA Hold */
	QM_RW uint32_t ic_tx_abrt_source; /**< Transmit Abort Source */
	QM_RW uint32_t reserved;
	QM_RW uint32_t ic_dma_cr;    /**< SDA Setup */
	QM_RW uint32_t ic_dma_tdlr;  /**< DMA Transmit Data Level Register */
	QM_RW uint32_t ic_dma_rdlr;  /**< I2C Receive Data Level Register */
	QM_RW uint32_t ic_sda_setup; /**< SDA Setup */
	QM_RW uint32_t ic_ack_general_call; /**< General Call Ack */
	QM_RW uint32_t ic_enable_status;    /**< Enable Status */
	QM_RW uint32_t ic_fs_spklen; /**< SS and FS Spike Suppression Limit */
	QM_RW uint32_t ic_hs_spklen; /**< HS spike suppression limit */
	QM_RW uint32_t reserved1[19];
	QM_RW uint32_t ic_comp_param_1; /**< Configuration Parameters */
	QM_RW uint32_t ic_comp_version; /**< Component Version */
	QM_RW uint32_t ic_comp_type;    /**< Component Type */
	QM_RW uint32_t padding[0xC0];   /* Padding (0x400-0xFC)/4 */
} qm_i2c_reg_t;

#if (UNIT_TEST)
qm_i2c_reg_t test_i2c_instance[QM_I2C_NUM];
qm_i2c_reg_t *test_i2c[QM_I2C_NUM];

#define QM_I2C test_i2c

#else
/** I2C Master register base address. */
#define QM_I2C_0_BASE (0xB0002800)
#define QM_I2C_1_BASE (0xB0002C00)

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
#define QM_I2C_IC_DATA_CMD_READ BIT(8)
#define QM_I2C_IC_DATA_CMD_STOP_BIT_CTRL BIT(9)
#define QM_I2C_IC_DATA_CMD_LSB_MASK (0x000000FF)
#define QM_I2C_IC_RAW_INTR_STAT_RX_FULL BIT(2)
#define QM_I2C_IC_RAW_INTR_STAT_TX_ABRT BIT(6)
#define QM_I2C_IC_TX_ABRT_SOURCE_NAK_MASK (0x1F)
#define QM_I2C_IC_TX_ABRT_SOURCE_ARB_LOST BIT(12)
#define QM_I2C_IC_TX_ABRT_SOURCE_ABRT_SBYTE_NORSTRT BIT(9)
#define QM_I2C_IC_TX_ABRT_SOURCE_ALL_MASK (0x1FFFF)
#define QM_I2C_IC_STATUS_BUSY_MASK (0x00000060)
#define QM_I2C_IC_STATUS_RFNE BIT(3)
#define QM_I2C_IC_STATUS_TFE BIT(2)
#define QM_I2C_IC_STATUS_TNF BIT(1)
#define QM_I2C_IC_INTR_MASK_ALL (0x00)
#define QM_I2C_IC_INTR_MASK_RX_UNDER BIT(0)
#define QM_I2C_IC_INTR_MASK_RX_OVER BIT(1)
#define QM_I2C_IC_INTR_MASK_RX_FULL BIT(2)
#define QM_I2C_IC_INTR_MASK_TX_OVER BIT(3)
#define QM_I2C_IC_INTR_MASK_TX_EMPTY BIT(4)
#define QM_I2C_IC_INTR_MASK_TX_ABORT BIT(6)
#define QM_I2C_IC_INTR_MASK_STOP_DETECTED BIT(9)
#define QM_I2C_IC_INTR_MASK_START_DETECTED BIT(10)
#define QM_I2C_IC_INTR_STAT_RX_UNDER BIT(0)
#define QM_I2C_IC_INTR_STAT_RX_OVER BIT(1)
#define QM_I2C_IC_INTR_STAT_RX_FULL BIT(2)
#define QM_I2C_IC_INTR_STAT_TX_OVER BIT(3)
#define QM_I2C_IC_INTR_STAT_TX_EMPTY BIT(4)
#define QM_I2C_IC_INTR_STAT_TX_ABRT BIT(6)
#define QM_I2C_IC_LCNT_MAX (65525)
#define QM_I2C_IC_LCNT_MIN (8)
#define QM_I2C_IC_HCNT_MAX (65525)
#define QM_I2C_IC_HCNT_MIN (6)

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
typedef enum { QM_GPIO_0 = 0, QM_AON_GPIO_0 = 1, QM_GPIO_NUM } qm_gpio_t;

/** GPIO register map. */
typedef struct {
	QM_RW uint32_t gpio_swporta_dr;  /**< Port A Data */
	QM_RW uint32_t gpio_swporta_ddr; /**< Port A Data Direction */
	QM_RW uint32_t gpio_swporta_ctl; /**< Port A Data Source */
	QM_RW uint32_t reserved[9];
	QM_RW uint32_t gpio_inten;	 /**< Interrupt Enable */
	QM_RW uint32_t gpio_intmask;       /**< Interrupt Mask */
	QM_RW uint32_t gpio_inttype_level; /**< Interrupt Type */
	QM_RW uint32_t gpio_int_polarity;  /**< Interrupt Polarity */
	QM_RW uint32_t gpio_intstatus;     /**< Interrupt Status */
	QM_RW uint32_t gpio_raw_intstatus; /**< Raw Interrupt Status */
	QM_RW uint32_t gpio_debounce;      /**< Debounce Enable */
	QM_RW uint32_t gpio_porta_eoi;     /**< Clear Interrupt */
	QM_RW uint32_t gpio_ext_porta;     /**< Port A External Port */
	QM_RW uint32_t reserved1[3];
	QM_RW uint32_t gpio_ls_sync; /**< Synchronization Level */
	QM_RW uint32_t reserved2;
	QM_RW uint32_t gpio_int_bothedge; /**< Interrupt both edge type */
	QM_RW uint32_t reserved3;
	QM_RW uint32_t gpio_config_reg2; /**< GPIO Configuration Register 2 */
	QM_RW uint32_t gpio_config_reg1; /**< GPIO Configuration Register 1 */
} qm_gpio_reg_t;

#define QM_NUM_GPIO_PINS (32)
#define QM_NUM_AON_GPIO_PINS (6)

#if (UNIT_TEST)
qm_gpio_reg_t test_gpio_instance;
qm_gpio_reg_t *test_gpio[QM_GPIO_NUM];

#define QM_GPIO test_gpio
#else

/** GPIO register base address */
#define QM_GPIO_BASE (0xB0000C00)
#define QM_AON_GPIO_BASE (QM_SCSS_CCU_BASE + 0xB00)

/** GPIO register block */
extern qm_gpio_reg_t *qm_gpio[QM_GPIO_NUM];
#define QM_GPIO qm_gpio
#endif

/** @} */

/**
 * @name Flash
 * @{
 */

/** Number of Flash controllers. */
typedef enum { QM_FLASH_0 = 0, QM_FLASH_1, QM_FLASH_NUM } qm_flash_t;

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
	QM_RW uint32_t padding[0x3FFF2]; /* (0x100000 - 0x38) / 4 */
} qm_flash_reg_t;

#if (UNIT_TEST)
qm_flash_reg_t test_flash_instance;
qm_flash_reg_t *test_flash[QM_FLASH_NUM];
uint8_t test_flash_page[0x800];

#define QM_FLASH test_flash

#define QM_FLASH_REGION_SYS_1_BASE (test_flash_page)
#define QM_FLASH_REGION_SYS_0_BASE (test_flash_page)
#define QM_FLASH_REGION_OTP_0_BASE (test_flash_page)

#define QM_FLASH_PAGE_MASK (0xCFF)
#define QM_FLASH_MAX_ADDR (0xFFFFFFFF)
#else

/* Flash physical address mappings */

#define QM_FLASH_REGION_SYS_1_BASE (0x40030000)
#define QM_FLASH_REGION_SYS_0_BASE (0x40000000)
#define QM_FLASH_REGION_OTP_0_BASE (0xFFFFE000)

#define QM_FLASH_PAGE_MASK (0x3F800)
#define QM_FLASH_MAX_ADDR (0x30000)

/** Flash controller register base address. */
#define QM_FLASH_BASE_0 (0xB0100000)
#define QM_FLASH_BASE_1 (0xB0200000)

/** Flash controller register block. */
extern qm_flash_reg_t *qm_flash[QM_FLASH_NUM];
#define QM_FLASH qm_flash

#endif

#define QM_FLASH_REGION_DATA_BASE_OFFSET (0x00)
#define QM_FLASH_MAX_WAIT_STATES (0xF)
#define QM_FLASH_MAX_US_COUNT (0x3F)
#define QM_FLASH_MAX_PAGE_NUM                                                  \
	(QM_FLASH_MAX_ADDR / (4 * QM_FLASH_PAGE_SIZE_DWORDS))
#define QM_FLASH_LVE_MODE BIT(5)

/** @} */

/**
 * @name Memory Protection Region
 * @{
 */

/** Memory Protection Region register map. */
typedef struct {
	QM_RW uint32_t mpr_cfg[4]; /**< MPR CFG */
	QM_RW uint32_t mpr_vdata;  /**< MPR_VDATA  */
	QM_RW uint32_t mpr_vsts;   /**< MPR_VSTS  */
} qm_mpr_reg_t;

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

#define QM_OSC0_PD BIT(2)

/** External Clock Control @{*/
#define QM_CCU_EXTERN_DIV_OFFSET (3)
#define QM_CCU_EXT_CLK_DIV_EN BIT(2)
/** End of External clock control @}*/

/** @} */

/**
 * @name Peripheral Clock
 * @{
 */

/** Peripheral clock type. */
typedef enum {
	CLK_PERIPH_REGISTER = BIT(0), /**< Peripheral Clock Gate Enable. */
	CLK_PERIPH_CLK = BIT(1),      /**< Peripheral Clock Enable. */
	CLK_PERIPH_I2C_M0 = BIT(2),   /**< I2C Master 0 Clock Enable. */
	CLK_PERIPH_I2C_M1 = BIT(3),   /**< I2C Master 1 Clock Enable. */
	CLK_PERIPH_SPI_S = BIT(4),    /**< SPI Slave Clock Enable. */
	CLK_PERIPH_SPI_M0 = BIT(5),   /**< SPI Master 0 Clock Enable. */
	CLK_PERIPH_SPI_M1 = BIT(6),   /**< SPI Master 1 Clock Enable. */
	CLK_PERIPH_GPIO_INTERRUPT = BIT(7), /**< GPIO Interrupt Clock Enable. */
	CLK_PERIPH_GPIO_DB = BIT(8),	/**< GPIO Debounce Clock Enable. */
	CLK_PERIPH_I2S = BIT(9),	    /**< I2S Clock Enable. */
	CLK_PERIPH_WDT_REGISTER = BIT(10),  /**< Watchdog Clock Enable. */
	CLK_PERIPH_RTC_REGISTER = BIT(11),  /**< RTC Clock Gate Enable. */
	CLK_PERIPH_PWM_REGISTER = BIT(12),  /**< PWM Clock Gate Enable. */
	CLK_PERIPH_GPIO_REGISTER = BIT(13), /**< GPIO Clock Gate Enable. */
	CLK_PERIPH_SPI_M0_REGISTER =
	    BIT(14), /**< SPI Master 0 Clock Gate Enable. */
	CLK_PERIPH_SPI_M1_REGISTER =
	    BIT(15), /**< SPI Master 1 Clock Gate Enable. */
	CLK_PERIPH_SPI_S_REGISTER =
	    BIT(16), /**< SPI Slave Clock Gate Enable. */
	CLK_PERIPH_UARTA_REGISTER = BIT(17), /**< UARTA Clock Gate Enable. */
	CLK_PERIPH_UARTB_REGISTER = BIT(18), /**< UARTB Clock Gate Enable. */
	CLK_PERIPH_I2C_M0_REGISTER =
	    BIT(19), /**< I2C Master 0 Clock Gate Enable. */
	CLK_PERIPH_I2C_M1_REGISTER =
	    BIT(20), /**< I2C Master 1 Clock Gate Enable. */
	CLK_PERIPH_I2S_REGISTER = BIT(21), /**< I2S Clock Gate Enable. */
	CLK_PERIPH_ALL = 0x3FFFFF	  /**< Quark SE peripherals Mask. */
} clk_periph_t;

/* Default mask values */
#define CLK_EXTERN_DIV_DEF_MASK (0xFFFFFFE3)
#define CLK_SYS_CLK_DIV_DEF_MASK (0xFFFFFC7F)
#define CLK_RTC_DIV_DEF_MASK (0xFFFFFF83)
#define CLK_GPIO_DB_DIV_DEF_MASK (0xFFFFFFE1)
#define CLK_PERIPH_DIV_DEF_MASK (0xFFFFFFF9)

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
	QM_DMA_CHANNEL_2,     /**< DMA channel id for channel 2 */
	QM_DMA_CHANNEL_3,     /**< DMA channel id for channel 3 */
	QM_DMA_CHANNEL_4,     /**< DMA channel id for channel 4 */
	QM_DMA_CHANNEL_5,     /**< DMA channel id for channel 5 */
	QM_DMA_CHANNEL_6,     /**< DMA channel id for channel 6 */
	QM_DMA_CHANNEL_7,     /**< DMA channel id for channel 7 */
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
	DMA_HW_IF_SPI_MASTER_1_TX = 0x6, /**< SPI_Master_1_TX */
	DMA_HW_IF_SPI_MASTER_1_RX = 0x7, /**< SPI_Master_1_RX */
	DMA_HW_IF_SPI_SLAVE_TX = 0x8,    /**< SPI_Slave_TX */
	DMA_HW_IF_SPI_SLAVE_RX = 0x9,    /**< SPI_Slave_RX */
	DMA_HW_IF_I2S_PLAYBACK = 0xa,    /**< I2S_Playback channel */
	DMA_HW_IF_I2S_CAPTURE = 0xb,     /**< I2S_Capture channel */
	DMA_HW_IF_I2C_MASTER_0_TX = 0xc, /**< I2C_Master_0_TX */
	DMA_HW_IF_I2C_MASTER_0_RX = 0xd, /**< I2C_Master_0_RX */
	DMA_HW_IF_I2C_MASTER_1_TX = 0xe, /**< I2C_Master_1_TX */
	DMA_HW_IF_I2C_MASTER_1_RX = 0xf, /**< I2C_Master_1_RX */
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
#define QM_DMA_CFG_H_SRC_PER_OFFSET (7)
#define QM_DMA_CFG_H_SRC_PER_MASK (0xf << QM_DMA_CFG_H_SRC_PER_OFFSET)
#define QM_DMA_CFG_H_DEST_PER_OFFSET (11)
#define QM_DMA_CFG_H_DEST_PER_MASK (0xf << QM_DMA_CFG_H_DEST_PER_OFFSET)

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

/** Channel write enable in the misc channel enable register. */
#define QM_DMA_MISC_CHAN_EN_WE_OFFSET (8)

/** Controller enable bit in the misc config register. */
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
 * @name USB
 * @{
 */

/** USB register base address */
#define QM_USB_BASE (0xB0500000)

/* USB PLL enable bit*/
#define QM_USB_PLL_PDLD BIT(0)
/* USB PLL has locked when this bit is 1*/
#define QM_USB_PLL_LOCK BIT(14)
/* Default values to setup the USB PLL*/
#define QM_USB_PLL_CFG0_DEFAULT (0x00003104)
/* USB PLL register*/
#define QM_USB_PLL_CFG0 (REG_VAL(0xB0800014))

/* USB clock enable bit*/
#define QM_CCU_USB_CLK_EN BIT(1)

/** @} */

/**
 * @name Versioning
 * @{
 */

#if (UNIT_TEST)
uint32_t test_rom_version;
#define ROM_VERSION_ADDRESS &test_rom_version;
#else
#define ROM_VERSION_ADDRESS (0xFFFFFFEC);
#endif

/** @} */

/** @} */

#endif /* __REGISTERS_H__ */
