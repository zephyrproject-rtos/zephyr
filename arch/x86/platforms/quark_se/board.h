/*
 * Copyright (c) 2015 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @brief board configuration macros for the Quark SE
 * This header file is used to specify and describe board-level aspects for
 * the Quark SE Platform.
 */

#ifndef __INCboardh
#define __INCboardh

#include <stdint.h>
#include <misc/util.h>
#include <uart.h>

#ifdef CONFIG_IOAPIC
#include <drivers/ioapic.h>
#endif

#define INT_VEC_IRQ0  0x20	/* Vector number for IRQ0 */
#define HPET_TIMER0_IRQ INT_VEC_IRQ0

#ifdef CONFIG_IOAPIC
#include <drivers/ioapic.h>
#ifdef CONFIG_SERIAL_INTERRUPT_LEVEL
#ifdef CONFIG_SERIAL_INTERRUPT_LOW
#define UART_IRQ_FLAGS (IOAPIC_LEVEL | IOAPIC_LOW)
#else
#define UART_IRQ_FLAGS (IOAPIC_LEVEL)
#endif
#else /* edge triggered interrupt */
#ifdef CONFIG_SERIAL_INTERRUPT_LOW
/* generate interrupt on falling edge */
#define UART_IRQ_FLAGS (IOAPIC_LOW)
#else
/* generate interrupt on raising edge */
#define UART_IRQ_FLAGS (0)
#endif
#endif
#endif

/* serial port (aka COM port) information */

#ifdef CONFIG_UART_NS16550

/* UART uses level triggered interrupt, low level */
#define UART_IOAPIC_FLAGS       (IOAPIC_LEVEL | IOAPIC_LOW)

#endif /* CONFIG_UART_NS16550 */


#ifndef _ASMLANGUAGE

/* Core system registers */

struct scss_ccu {
	volatile uint32_t osc0_cfg0;    /**< Hybrid Oscillator Configuration 0 */
	volatile uint32_t osc0_stat1;   /**< Hybrid Oscillator status 1 */
	volatile uint32_t osc0_cfg1;    /**< Hybrid Oscillator configuration 1 */
	volatile uint32_t osc1_stat0;   /**< RTC Oscillator status 0 */
	volatile uint32_t osc1_cfg0;    /**< RTC Oscillator Configuration 0 */
	volatile uint32_t usb_pll_cfg0; /**< USB Phase lock look configuration */
	volatile uint32_t
		ccu_periph_clk_gate_ctl; /**< Peripheral Clock Gate Control */
	volatile uint32_t
		ccu_periph_clk_div_ctl0; /**< Peripheral Clock Divider Control 0 */
	volatile uint32_t
		ccu_gpio_db_clk_ctl; /**< Peripheral Clock Divider Control 1 */
	volatile uint32_t ccu_ext_clock_ctl; /**< External Clock Control Register */
	volatile uint32_t ccu_ss_periph_clk_gate_ctl; /**< Sensor subsustem
							peripheral clock gate
							control */
	volatile uint32_t ccu_lp_clk_ctl; /**< System Low Power Clock Control */
	volatile uint32_t reserved;
	volatile uint32_t ccu_mlayer_ahb_ctl; /**< AHB Control Register */
	volatile uint32_t ccu_sys_clk_ctl;    /**< System Clock Control Register */
	volatile uint32_t osc_lock_0;        /**< Clocks Lock Register */
};

struct scss_peripheral {
	volatile uint32_t usb_phy_cfg0; /**< USB Configuration */
	volatile uint32_t periph_cfg0;  /**< Peripheral Configuration */
	volatile uint32_t reserved[2];
	volatile uint32_t cfg_lock; /**< Configuration Lock */
};

struct int_ss_i2c {
	volatile uint32_t err_mask;
	volatile uint32_t rx_avail_mask;
	volatile uint32_t tx_req_mask;
	volatile uint32_t stop_det_mask;
};

struct int_ss_spi {
	volatile uint32_t err_int_mask;
	volatile uint32_t rx_avail_mask;
	volatile uint32_t tx_req_mask;
};

struct scss_interrupt {
	volatile uint32_t int_ss_adc_err_mask;
	volatile uint32_t int_ss_adc_irq_mask;
	volatile uint32_t int_ss_gpio_intr_mask[2];
	struct int_ss_i2c int_ss_i2c[2];
	struct int_ss_spi int_ss_spi[2];
	volatile uint32_t int_i2c_mst_mask[2];
	volatile uint32_t reserved;
	volatile uint32_t int_spi_mst_mask[2];
	volatile uint32_t int_spi_slv_mask[1];
	volatile uint32_t int_uart_mask[2];
	volatile uint32_t int_i2s_mask;
	volatile uint32_t int_gpio_mask;
	volatile uint32_t int_pwm_timer_mask;
	volatile uint32_t int_usb_mask;
	volatile uint32_t int_rtc_mask;
	volatile uint32_t int_watchdog_mask;
	volatile uint32_t int_dma_channel_mask[8];
	volatile uint32_t int_mailbox_mask;
	volatile uint32_t int_comparators_ss_halt_mask;
	volatile uint32_t int_comparators_host_halt_mask;
	volatile uint32_t int_comparators_ss_mask;
	volatile uint32_t int_comparators_host_mask;
	volatile uint32_t int_host_bus_err_mask;
	volatile uint32_t int_dma_error_mask;
	volatile uint32_t int_sram_controller_mask;
	volatile uint32_t int_flash_controller_mask[2];
	volatile uint32_t int_aon_timer_mask;
	volatile uint32_t int_adc_pwr_mask;
	volatile uint32_t int_adc_calib_mask;
	volatile uint32_t int_aon_gpio_mask;
	volatile uint32_t lock_int_mask_reg;
};

#define SCSS_PERIPHERAL_BASE (0xB0800800)
#define SCSS_PERIPHERAL ((struct scss_peripheral *)SCSS_PERIPHERAL_BASE)

#define SCSS_INT_BASE (0xB0800400)
#define SCSS_INTERRUPT ((struct scss_interrupt *)SCSS_INT_BASE)

/* Base Register */
#define SCSS_REGISTER_BASE		0xB0800000
#define SCSS_CCU ((struct scss_ccu *)SCSS_REGISTER_BASE)

#define SCSS_CCU_SYS_CLK_CTL		0x38

/* Peripheral Clock Gate Control */
#define SCSS_CCU_PERIPH_CLK_GATE_CTL	0x18
#define CCU_PERIPH_CLK_EN 		(1 << 1)
#define CCU_PERIPH_CLK_DIV_CTL0		0x1C
#define INT_UNMASK_IA  		        (~0x00000001)

/* PWM */
#define CCU_PWM_PCLK_EN_SW     		(1 << 12)

/* Watchdog */
#define WDT_DW_INT_MASK			(SCSS_INT_BASE + 0x7C)
#define SCSS_PERIPH_CFG0		(0x4)

/* RTC */
#define RTC_DW_INT_MASK			(SCSS_INT_BASE + 0x78)
#define CCU_RTC_CLK_DIV_OFFSET		(3)

/* Clock */
#define CLOCK_PERIPHERAL_BASE_ADDR	(SCSS_REGISTER_BASE + 0x18)
#define CLOCK_EXTERNAL_BASE_ADDR	(SCSS_REGISTER_BASE + 0x24)
#define CLOCK_SENSOR_BASE_ADDR		(SCSS_REGISTER_BASE + 0x28)
#define CLOCK_SYSTEM_CLOCK_CONTROL	(SCSS_REGISTER_BASE + SCSS_CCU_SYS_CLK_CTL)

/* SPI */
#define SPI_DW_PORT_0_INT_MASK		(SCSS_INT_BASE + 0x54)
#define SPI_DW_PORT_1_INT_MASK		(SCSS_INT_BASE + 0x58)

/* GPIO */
#define GPIO_INT_MASK			(SCSS_INT_BASE + 0x6C)

/* Comparator */
#define INT_AIO_CMP_IRQ			(0x16)

/*ARC INIT*/
#define RESET_VECTOR                   	0x40000000
#define SCSS_SS_CFG                    	0x0600
#define SCSS_SS_STS                    	0x0604
#define ARC_HALT_INT_REDIR             	(1 << 26)
#define ARC_HALT_REQ_A                 	(1 << 25)
#define ARC_RUN_REQ_A                  	(1 << 24)
#define ARC_RUN				(ARC_HALT_INT_REDIR | ARC_RUN_REQ_A)
#define ARC_HALT			(ARC_HALT_INT_REDIR | ARC_HALT_REQ_A)


#endif /*  _ASMLANGUAGE */

#endif /* __INCboardh */
