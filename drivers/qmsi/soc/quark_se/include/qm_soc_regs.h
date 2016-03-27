/*
 * Copyright (c) 2015, Intel Corporation
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
 * Quark SE SOC Register file.
 *
 * @brief Quark SE SOC Registers.
 *
 */

#define QUARK_SE (1)
#define HAS_4_TIMERS (1)
#define HAS_AON_GPIO (1)
#define HAS_APIC (1)

/**
 * @defgroup groupSEREG Quark SE Registers
  @{
 */

/* Core system registers */

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
	    ccu_periph_clk_div_ctl0; /**< Peripheral Clock Divider Control 0  */
	QM_RW uint32_t
	    ccu_gpio_db_clk_ctl; /**< Peripheral Clock Divider Control 1 */
	QM_RW uint32_t
	    ccu_ext_clock_ctl; /**< External Clock Control Register */
	QM_RW uint32_t ccu_ss_periph_clk_gate_ctl; /**< Sensor subsustem
						     peripheral clock gate
						     control */
	QM_RW uint32_t ccu_lp_clk_ctl; /**< System Low Power Clock Control */
	QM_RW uint32_t reserved;
	QM_RW uint32_t ccu_mlayer_ahb_ctl; /**< AHB Control Register */
	QM_RW uint32_t ccu_sys_clk_ctl;    /**< System Clock Control Register */
	QM_RW uint32_t osc_lock_0;	 /**< Clocks Lock Register */
} qm_scss_ccu_reg_t;

/** system control subsystem clock control unit register block */
#if (UNIT_TEST)
qm_scss_ccu_reg_t test_scss_ccu;
#define QM_SCSS_CCU ((qm_scss_ccu_reg_t *)(&test_scss_ccu))

#else
#define QM_SCSS_CCU_BASE (0xB0800000)
#define QM_SCSS_CCU ((qm_scss_ccu_reg_t *)QM_SCSS_CCU_BASE)
#endif

#define QM_OSC0_MODE_SEL BIT(3)
#define QM_OSC0_LOCK_SI BIT(0)
#define QM_OSC0_LOCK_XTAL BIT(1)
#define QM_OSC0_EN_SI_OSC BIT(1)

#define QM_OSC0_SI_FREQ_SEL_DEF_MASK (0xFFFFFCFF)

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

/** system control subsystem general purpose register block */
#if (UNIT_TEST)
qm_scss_gp_reg_t test_scss_gp;
#define QM_SCSS_GP ((qm_scss_gp_reg_t *)(&test_scss_gp))

#else
#define QM_SCSS_GP_BASE (0xB0800100)
/** system control subsystem general purpose register block */
#define QM_SCSS_GP ((qm_scss_gp_reg_t *)QM_SCSS_GP_BASE)
#endif

typedef struct {
	QM_RW uint32_t mem_ctrl; /**< Memory control */
} qm_scss_mem_reg_t;

#if (UNIT_TEST)
qm_scss_mem_reg_t test_scss_mem;
#define QM_SCSS_MEM ((qm_scss_mem_reg_t *)(&test_scss_mem))

#else
/** system control subsystem memory control register block */
#define QM_SCSS_MEM_BASE (0xB0800200)
#define QM_SCSS_MEM ((qm_scss_mem_reg_t *)QM_SCSS_MEM_BASE)
#endif

typedef struct {
	QM_RW uint32_t cmp_en;      /**< Comparator enable */
	QM_RW uint32_t cmp_ref_sel; /**< Comparator reference select */
	QM_RW uint32_t
	    cmp_ref_pol; /**< Comparator reference polarity select register */
	QM_RW uint32_t cmp_pwr; /**< Comparator power enable register */
	QM_RW uint32_t reserved[6];
	QM_RW uint32_t cmp_stat_clr; /**< Comparator clear register */
} qm_scss_cmp_reg_t;

#if (UNIT_TEST)
qm_scss_cmp_reg_t test_scss_cmp;
#define QM_SCSS_CMP ((qm_scss_cmp_reg_t *)(&test_scss_cmp))

#else
/** system control subsystem comparators register block */
#define QM_SCSS_CMP_BASE (0xB0800300)
#define QM_SCSS_CMP ((qm_scss_cmp_reg_t *)QM_SCSS_CMP_BASE)
#endif

typedef struct {
	QM_RW uint32_t reg;
	QM_RW uint32_t pad[3];
} apic_reg_pad_t;

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
#if !defined(USE_ISR_EOI)
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
/** IO/APIC */
#define QM_IOAPIC_BASE (0xFEC00000)
#define QM_IOAPIC ((qm_ioapic_reg_t *)QM_IOAPIC_BASE)
#endif

typedef struct {
	QM_RW uint32_t err_mask;
	QM_RW uint32_t rx_avail_mask;
	QM_RW uint32_t tx_req_mask;
	QM_RW uint32_t stop_det_mask;
} int_ss_i2c_reg_t;

typedef struct {
	QM_RW uint32_t err_int_mask;
	QM_RW uint32_t rx_avail_mask;
	QM_RW uint32_t tx_req_mask;
} int_ss_spi_reg_t;

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
	QM_RW uint32_t int_sram_controller_mask;
	QM_RW uint32_t int_flash_controller_0_mask;
	QM_RW uint32_t int_flash_controller_1_mask;
	QM_RW uint32_t int_aon_timer_mask;
	QM_RW uint32_t int_adc_pwr_mask;
	QM_RW uint32_t int_adc_calib_mask;
	QM_RW uint32_t int_aon_gpio_mask;
	QM_RW uint32_t lock_int_mask_reg;
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
/** system control subsystem Interrupt masking register block */
#define QM_SCSS_INT_BASE (0xB0800400)
#define QM_SCSS_INT ((qm_scss_int_reg_t *)QM_SCSS_INT_BASE)
#endif

#define QM_INT_SRAM_CONTROLLER_HOST_HALT_MASK BIT(16)
#define QM_INT_SRAM_CONTROLLER_HOST_MASK BIT(0)
#define QM_INT_FLASH_CONTROLLER_HOST_HALT_MASK BIT(16)
#define QM_INT_FLASH_CONTROLLER_HOST_MASK BIT(0)

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
	QM_RW uint32_t
	    pmnetcs; /**< Power Management Network (PMNet) Control and
Status */
	QM_RW uint32_t pm_wait; /**< Power Management Wait */
	QM_RW uint32_t reserved2;
	QM_RW uint32_t p_sts; /**< Processor Status */
	QM_RW uint32_t reserved3[3];
	QM_RW uint32_t rstc; /**< Reset control */
	QM_RW uint32_t rsts; /**< Reset status */
	QM_RW uint32_t reserved4[6];
	QM_RW uint32_t vr_lock; /**< Voltage regulator lock */
	QM_RW uint32_t pm_lock; /**< Power management lock */

} qm_scss_pmu_reg_t;

#if (UNIT_TEST)
qm_scss_pmu_reg_t test_scss_pmu;
#define QM_SCSS_PMU ((qm_scss_pmu_reg_t *)(&test_scss_pmu))

#else
/** system control subsystem power management register block */
#define QM_SCSS_PMU_BASE (0xB0800504)
#define QM_SCSS_PMU ((qm_scss_pmu_reg_t *)QM_SCSS_PMU_BASE)
#endif

#define QM_P_STS_HALT_INTERRUPT_REDIRECTION BIT(26)

typedef struct {
	QM_RW uint32_t ss_cfg; /**< Sensor Subsystem Configuration */
	QM_RW uint32_t ss_sts; /**< Sensor subsystem status */
} qm_scss_ss_reg_t;

#if (UNIT_TEST)
qm_scss_ss_reg_t test_scss_ss;
#define QM_SCSS_SS ((qm_scss_ss_reg_t *)(&test_scss_ss))

#else
/** system control subsystem sensor subsystem register block */
#define QM_SCSS_SS_BASE (0xB0800600)
#define QM_SCSS_SS ((qm_scss_ss_reg_t *)QM_SCSS_SS_BASE)
#endif

/**
 * Number of SCSS Always on controllers.
 */
typedef enum { QM_SCSS_AON_0 = 0, QM_SCSS_AON_NUM } qm_scss_aon_t;

typedef struct {
	QM_RW uint32_t aonc_cnt;  /**< Always on counter register */
	QM_RW uint32_t aonc_cfg;  /**< Always on counter enable */
	QM_RW uint32_t aonpt_cnt; /**< Always on periodic timer */
	QM_RW uint32_t
	    aonpt_stat; /**< Always on periodic timer status register */
	QM_RW uint32_t aonpt_ctrl; /**< Always on periodic timer control */
	QM_RW uint32_t
	    aonpt_cfg; /**< Always on periodic timer configuration register */
} qm_scss_aon_reg_t;

#if (UNIT_TEST)
qm_scss_aon_reg_t test_scss_aon;
#define QM_SCSS_AON ((qm_scss_aon_reg_t *)(&test_scss_aon))

#else
/** system control subsystem always on register block */
#define QM_SCSS_AON_BASE (0xB0800700)
#define QM_SCSS_AON ((qm_scss_aon_reg_t *)QM_SCSS_AON_BASE)
#endif

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
/** system control subsystem peripheral register block */
#define QM_SCSS_PERIPHERAL_BASE (0xB0800800)
#define QM_SCSS_PERIPHERAL ((qm_scss_peripheral_reg_t *)QM_SCSS_PERIPHERAL_BASE)
#endif

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
/** system control subsystem pin muxing register block */
#define QM_SCSS_PMUX_BASE (0xB0800900)
#define QM_SCSS_PMUX ((qm_scss_pmux_reg_t *)QM_SCSS_PMUX_BASE)
#endif

typedef struct {
	QM_RW uint32_t ch_ctrl;    /**< Channel Control Word */
	QM_RW uint32_t ch_data[4]; /**< Channel Payload Data Word 0 */
	QM_RW uint32_t ch_sts;     /**< Channel status */
} qm_mailbox_t;

typedef struct {
	qm_mailbox_t mbox[8];	  /**< 8 Mailboxes */
	QM_RW uint32_t mbox_chall_sts; /**< All channel status */
} qm_scss_mailbox_reg_t;

#if (UNIT_TEST)
qm_scss_mailbox_reg_t test_scss_mailbox;
#define QM_SCSS_MAILBOX ((qm_scss_mailbox_reg_t *)(&test_scss_mailbox))

#else
/** system control subsystem mailbox register block */
#define QM_SCSS_MAILBOX_BASE (0xB0800A00)
#define QM_SCSS_MAILBOX ((qm_scss_mailbox_reg_t *)QM_SCSS_MAILBOX_BASE)
#endif

typedef struct {
	QM_RW uint32_t id;
} qm_scss_info_reg_t;

#if (UNIT_TEST)
qm_scss_info_reg_t test_scss_info;
#define QM_SCSS_INFO ((qm_scss_info_reg_t *)(&test_scss_info))

#else
/** system control subsystem information register block */
#define QM_SCSS_INFO_BASE (0xB0801000)
#define QM_SCSS_INFO ((qm_scss_info_reg_t *)QM_SCSS_INFO_BASE)
#endif

/* Peripheral registers */

/**
 * Number of PWM/Timer controllers.
 */
typedef enum { QM_PWM_0 = 0, QM_PWM_NUM } qm_pwm_t;

/**
 * PWM id type.
 */
typedef enum {
	QM_PWM_ID_0 = 0,
	QM_PWM_ID_1,
	QM_PWM_ID_2,
	QM_PWM_ID_3,
	QM_PWM_ID_NUM
} qm_pwm_id_t;

/**
 * PWM / Timer register map.
 */

typedef struct {
	QM_RW uint32_t loadcount;    /**< Load Count */
	QM_RW uint32_t currentvalue; /**< Current Value */
	QM_RW uint32_t controlreg;   /**< Control */
	QM_RW uint32_t eoi;	  /**< End Of Interrupt */
	QM_RW uint32_t intstatus;    /**< Interrupt Status */
} qm_pwm_channel_t;

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

/**
 * Number of WDT controllers.
 */
typedef enum { QM_WDT_0 = 0, QM_WDT_NUM } qm_wdt_t;

/**
 * Watchdog timer register block type.
 */
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

/**
 * Number of UART controllers.
 */
typedef enum { QM_UART_0 = 0, QM_UART_1, QM_UART_NUM } qm_uart_t;

/**
 * UART register block type.
 */

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
qm_uart_reg_t test_uart[QM_UART_NUM];
#define QM_UART ((qm_uart_reg_t *)(&test_uart))

#else
/** UART register base address */
#define QM_UART_BASE (0xB0002000)
/** UART register block */
#define QM_UART ((qm_uart_reg_t *)QM_UART_BASE)
#endif

/**
 * Number of RTC controllers.
 */
typedef enum { QM_RTC_0 = 0, QM_RTC_NUM } qm_rtc_t;

/**
 * QM RTC Register block type.
 */
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
/** RTC register base address */
#define QM_RTC_BASE (0xB0000400)
/** RTC register block */
#define QM_RTC ((qm_rtc_reg_t *)QM_RTC_BASE)
#endif

/**
 * Number of SPI controllers.
 */
typedef enum {
	QM_SPI_MST_0 = 0,
	QM_SPI_MST_1,
	QM_SPI_SLV_0,
	QM_SPI_NUM
} qm_spi_t;

/**
 * SPI register entries
 */
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

/**
 * Number of I2C controllers.
 */
typedef enum { QM_I2C_0 = 0, QM_I2C_1, QM_I2C_NUM } qm_i2c_t;

/**
 * I2C Register block type.
 */
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
	QM_RW uint32_t ic_dma_cr;	 /**< SDA Setup */
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
qm_i2c_reg_t test_i2c[QM_I2C_NUM];

#define QM_I2C ((qm_i2c_reg_t *)(&test_i2c))

#else
/** I2C Master register base address */
#define QM_I2C_BASE (0xB0002800)

/** I2C register block */
#define QM_I2C ((qm_i2c_reg_t *)QM_I2C_BASE)
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

/**
 * Number of GPIO controllers.
 */
typedef enum { QM_GPIO_0 = 0, QM_AON_GPIO_0 = 1, QM_GPIO_NUM } qm_gpio_t;

/**
 * GPIO register block type.
 */
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

/**
 * Number of Flash controllers.
 */
typedef enum { QM_FLASH_0 = 0, QM_FLASH_1, QM_FLASH_NUM } qm_flash_t;

/**
 * Flash register block type.
 */
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
	QM_RW uint32_t mpr_vdata;	/** MPR_VDATA */
	QM_RW uint32_t padding[0x3FFF2]; /* (0x100000 - 0x38) / 4 */
} qm_flash_reg_t;

#define QM_FLASH_PAGE_MASK (0x3F800)

#if (UNIT_TEST)
qm_flash_reg_t test_flash[QM_FLASH_NUM];
uint32_t test_flash_page[0x200];

#define QM_FLASH_BASE ((uint32_t *)&test_flash)
#define QM_FLASH ((qm_flash_reg_t *)(&test_flash))

#define QM_FLASH_REGION_SYS_1_BASE (test_flash_page)
#define QM_FLASH_REGION_SYS_0_BASE (test_flash_page)
#define QM_FLASH_REGION_OTP_0_BASE (test_flash_page)

#else

/** Flash controller 0 register block */
#define QM_FLASH_BASE (0xB0100000)
#define QM_FLASH ((qm_flash_reg_t *)QM_FLASH_BASE)

/** Flash physical address mappings */
#define QM_FLASH_REGION_SYS_1_BASE (0x40030000)
#define QM_FLASH_REGION_SYS_0_BASE (0x40000000)
#define QM_FLASH_REGION_OTP_0_BASE (0xFFFFE000)

#endif

#define QM_FLASH_REGION_DATA_BASE_OFFSET (0x00)
#define QM_FLASH_MAX_WAIT_STATES (0xF)
#define QM_FLASH_MAX_US_COUNT (0x3F)
#define QM_FLASH_MAX_ADDR (0x30000)
#define QM_FLASH_MAX_PAGE_NUM (QM_FLASH_MAX_ADDR / (4 * QM_FLASH_PAGE_SIZE))

/**
 * Memory Protection Region register block type.
 */
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

/**
 * DMA register block type.
 */
/**
 * DMA Channel struct
 */
typedef struct {
	QM_RW uint32_t sar;     /**< Source address */
	QM_RW uint32_t dar;     /**< Destination address */
	QM_RW uint32_t llp;     /**< Linked list pointer */
	QM_RW uint32_t ctrl_l;  /**< Control, lower */
	QM_RW uint32_t ctrl_u;  /**< Control, upper */
	QM_RW uint32_t sstat;   /**< Source status */
	QM_RW uint32_t dstat;   /**< Destination status */
	QM_RW uint32_t sstatar; /**< Source status address */
	QM_RW uint32_t dstatar; /**< Destination status address */
	QM_RW uint32_t cfg_l;   /**< Configuration, lower */
	QM_RW uint32_t cfg_u;   /**< Configuration, upper */
	QM_RW uint32_t sgr;     /**< Source, gather */
	QM_RW uint32_t dsr;     /**< Destination, scatter */
} qm_dma_channel_t;

typedef struct {
} qm_dma_t;

#if (UNIT_TEST)
qm_dma_t test_dma;

#define QM_DMA ((qm_dma_t *)(&test_dma))

#else

#define QM_DMA_BASE (0xB0700000)
#define QM_DMA ((qm_dma_t *)QM_DMA_BASE)

#endif

/* DMA Channels */
typedef enum {
	QM_DMA_UART_0_TX = 0,
	QM_DMA_UART_0_RX,
	QM_DMA_UART_1_TX,
	QM_DMA_UART_1_RX,
	QM_DMA_SPI_MST_0_TX,
	QM_DMA_SPI_MST_0_RX,
	QM_DMA_SPI_MST_1_TX,
	QM_DMA_SPI_MST_1_RX,
	QM_DMA_SPI_SLV_TX,
	QM_DMA_SPI_SLV_RX,
	QM_DMA_I2S_TX,
	QM_DMA_I2S_RX,
	QM_DMA_I2C_MST_0_TX,
	QM_DMA_I2C_MST_0_RX,
	QM_DMA_I2C_MST_1_TX,
	QM_DMA_I2C_MST_1_RX
} qm_dma_channels_t;

#define QM_OSC0_PD BIT(2)

/** External Clock Control @{*/
#define QM_CCU_EXTERN_DIV_OFFSET (3)
#define QM_CCU_EXT_CLK_DIV_EN BIT(2)
/** End of External clock control @}*/

/** GPIO Debounce Clock Control @{*/
#define QM_CCU_GPIO_DB_DIV_OFFSET (2)
#define QM_CCU_GPIO_DB_CLK_DIV_EN BIT(1)
/** End of GPIO Debounce Clock Control @}*/

/** Peripheral clock divider control 0 @{*/
#define QM_CCU_PERIPH_PCLK_DIV_OFFSET (1)
#define QM_CCU_PERIPH_PCLK_DIV_EN BIT(0)
/** End of Peripheral clock divider control 0 @}*/

/** System clock control @{*/
#define QM_CCU_SYS_CLK_SEL BIT(0)
#define QM_CCU_RTC_CLK_EN BIT(1)
#define QM_CCU_RTC_CLK_DIV_EN BIT(2)
#define QM_CCU_SYS_CLK_DIV_EN BIT(7)

#define QM_CCU_RTC_CLK_DIV_OFFSET (3)
#define QM_CCU_SYS_CLK_DIV_OFFSET (8)

/* Default mask values */
#define CLK_EXTERN_DIV_DEF_MASK (0xFFFFFFE3)
#define CLK_SYS_CLK_DIV_DEF_MASK (0xFFFFF87F)
#define CLK_RTC_DIV_DEF_MASK (0xFFFFFF83)
#define CLK_GPIO_DB_DIV_DEF_MASK (0xFFFFFFE1)
#define CLK_PERIPH_DIV_DEF_MASK (0xFFFFFFF9)

#define SYS_CLK_HYB_OSC_32MHZ_SETTINGS *((volatile uint32_t *)0x00000004)
#define SYS_CLK_HYB_OSC_4MHZ_SETTINGS *((volatile uint32_t *)0x00000008)

/** End of System clock control 0 @}*/

/** IRQs and interrupt vectors.
 *
 * Any IRQ > 1 actually has a SCSS mask register offset of +1.
 * The vector numbers must be defined without arithmetic expressions nor
 * parentheses because they are expanded as token concatenation.
 */

#define QM_IRQ_GPIO_0 (8)
#define QM_IRQ_GPIO_0_MASK_OFFSET (9)
#define QM_IRQ_GPIO_0_VECTOR 44

#define QM_IRQ_PWM_0 (9)
#define QM_IRQ_PWM_0_MASK_OFFSET (10)
#define QM_IRQ_PWM_0_VECTOR 45

#define QM_IRQ_SPI_MASTER_0 (2)
#define QM_IRQ_SPI_MASTER_0_MASK_OFFSET (3)
#define QM_IRQ_SPI_MASTER_0_VECTOR 38

#define QM_IRQ_SPI_MASTER_1 (3)
#define QM_IRQ_SPI_MASTER_1_MASK_OFFSET (4)
#define QM_IRQ_SPI_MASTER_1_VECTOR 39

#define QM_IRQ_RTC_0 (11)
#define QM_IRQ_RTC_0_MASK_OFFSET (12)
#define QM_IRQ_RTC_0_VECTOR 47

#define QM_IRQ_WDT_0 (12)
#define QM_IRQ_WDT_0_MASK_OFFSET (13)
#define QM_IRQ_WDT_0_VECTOR 48

#define QM_IRQ_I2C_0 (0)
#define QM_IRQ_I2C_0_MASK_OFFSET (0)
#define QM_IRQ_I2C_0_VECTOR 36

#define QM_IRQ_I2C_1 (1)
#define QM_IRQ_I2C_1_MASK_OFFSET (1)
#define QM_IRQ_I2C_1_VECTOR 37

#define QM_IRQ_AC (22)
#define QM_IRQ_AC_MASK_OFFSET (26)
#define QM_IRQ_AC_VECTOR 58

#define QM_IRQ_SRAM (25)
#define QM_IRQ_SRAM_MASK_OFFSET (29)
#define QM_IRQ_SRAM_VECTOR 61

#define QM_IRQ_FLASH_0 (26)
#define QM_IRQ_FLASH_0_MASK_OFFSET (30)
#define QM_IRQ_FLASH_0_VECTOR 62

#define QM_IRQ_FLASH_1 (27)
#define QM_IRQ_FLASH_1_MASK_OFFSET (31)
#define QM_IRQ_FLASH_1_VECTOR 63

#define QM_IRQ_AONPT_0 (28)
#define QM_IRQ_AONPT_0_MASK_OFFSET (32)
#define QM_IRQ_AONPT_0_VECTOR 64

#define QM_IRQ_AONGPIO_0 (31)
#define QM_IRQ_AONGPIO_0_MASK_OFFSET (35)
#define QM_IRQ_AONGPIO_0_VECTOR 67

#define QM_IRQ_UART_0 (5)
#define QM_IRQ_UART_0_MASK_OFFSET (6)
#define QM_IRQ_UART_0_VECTOR 41

#define QM_IRQ_UART_1 (6)
#define QM_IRQ_UART_1_MASK_OFFSET (7)
#define QM_IRQ_UART_1_VECTOR 42

#define QM_INT_VECTOR_PIC_TIMER 32

/* Hybrid oscillator output select select (0=Silicon, 1=Crystal)*/
#define QM_OSC0_MODE_SEL BIT(3)
/* Enable Crystal oscillator*/
#define QM_OSC0_EN_CRYSTAL BIT(0)

/* Crystal oscillator parameters */
#define OSC0_CFG1_OSC0_FADJ_XTAL_MASK (0x000F0000)
#define OSC0_CFG1_OSC0_FADJ_XTAL_OFFS (16)
#define OSC0_CFG0_OSC0_XTAL_COUNT_VALUE_MASK (0x00600000)
#define OSC0_CFG0_OSC0_XTAL_COUNT_VALUE_OFFS (21)

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
/* Clock enable/disable register*/
#define QM_CCU_MLAYER_AHB_CTL (REG_VAL(0xB0800034))

/* Ensure CLK divide by one is set*/
#define QM_CCU_SYS_CLK_DIV_BY_ONE (0xFFFFFCFF)
/* SYS CLK register*/
#define QM_CCU_SYS_CLK_CTL (REG_VAL(0xB0800038))

/* Pin MUX slew rate registers and settings */
#define QM_PMUX_SLEW_4MA_DRIVER (0xFFFFFFFF)
#define QM_PMUX_SLEW0 (REG_VAL(0xB0800910))
#define QM_PMUX_SLEW1 (REG_VAL(0xB0800914))
#define QM_PMUX_SLEW2 (REG_VAL(0xB0800918))
#define QM_PMUX_SLEW3 (REG_VAL(0xB080091C))

#endif /* __REGISTERS_H__ */
