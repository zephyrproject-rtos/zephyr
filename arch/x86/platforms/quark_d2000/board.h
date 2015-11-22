/*
 * Copyright (c) 2015 Wind River Systems, Inc.
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
 * @brief board configuration macros for the Quark D2000
 * This header file is used to specify and describe board-level aspects for
 * the Quark D2000 Platform.
 */

#ifndef __INCboardh
#define __INCboardh

#include <stdint.h>
#include <misc/util.h>
#include <uart.h>
#include <drivers/ioapic.h>

#define INT_VEC_IRQ0  0x20 /* Vector number for IRQ0 */
#define FIXED_HARDWARE_IRQ_TO_VEC_MAPPING(x) (INT_VEC_IRQ0 + x)
#define IOAPIC_LO32_RTE_SUPPORTED_MASK (IOAPIC_INT_MASK | IOAPIC_TRIGGER_MASK)

/* Base Register */
#define SCSS_REGISTER_BASE              0xB0800000

/* Clock */
#define CLOCK_PERIPHERAL_BASE_ADDR	(SCSS_REGISTER_BASE + 0x18)
#define CLOCK_EXTERNAL_BASE_ADDR	(SCSS_REGISTER_BASE + 0x24)
#define CLOCK_SENSOR_BASE_ADDR		(SCSS_REGISTER_BASE + 0x28)
#define CLOCK_SYSTEM_CLOCK_CONTROL      (SCSS_REGISTER_BASE + 0x38)

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

/* Peripheral Clock Gate Control */
#define SCSS_CCU_PERIPH_CLK_GATE_CTL	0x18
#define CCU_PERIPH_CLK_EN		(1 << 1)
#define CCU_PERIPH_CLK_DIV_CTL0		0x1C
#define INT_UNMASK_IA			(~0x00000001)
/*
 * Local APIC (LOAPIC) device information (Intel loapic)
 */

#define LOAPIC_IRQ_BASE			CONFIG_LOAPIC_TIMER_IRQ
#define LOAPIC_IRQ_COUNT		1
#define LOAPIC_LVT_REG_SPACING  0x10

/* serial port (aka COM port) information */
#define SYNOPSIS_UART_DLF_OFFSET		0xc0
#define SYNOPSIS_UART_DLF_115200_VAL	0x06

#define COM1_BASE_ADRS		0xB0002000
#define COM1_INT_LVL		0x08		/* UART_A connected to IRQ8 */
#define COM1_INT_VEC		(INT_VEC_IRQ0 + COM1_INT_LVL)
#define COM1_INT_PRI		3
#define COM1_BAUD_RATE		115200
#define COM1_DLF		SYNOPSIS_UART_DLF_115200_VAL

#define COM2_BASE_ADRS		0xB0002400
#define COM2_INT_LVL		0x06		/* UART_B connected to IRQ6 */
#define COM2_INT_VEC		(INT_VEC_IRQ0 + COM2_INT_LVL)
#define COM2_INT_PRI		3
#define COM2_BAUD_RATE		115200
#define COM2_DLF		SYNOPSIS_UART_DLF_115200_VAL

#define UART_REG_ADDR_INTERVAL  4       /* address diff of adjacent regs. */

/*
 * On the board the UART works on the same clock frequency as CPU.
 */
#define UART_XTAL_FREQ	        CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC

/* UART uses level triggered interrupt, low level */
#define UART_IOAPIC_FLAGS       (IOAPIC_LEVEL)

/* uart configuration settings */

#define CONFIG_UART0_CONSOLE_REGS	COM1_BASE_ADRS
#define CONFIG_UART0_CONSOLE_IRQ	COM1_INT_LVL
#define CONFIG_UART0_CONSOLE_INT_PRI	COM1_INT_PRI

#define CONFIG_UART1_CONSOLE_REGS	COM2_BASE_ADRS
#define CONFIG_UART1_CONSOLE_IRQ	COM2_INT_LVL
#define CONFIG_UART1_CONSOLE_INT_PRI	COM2_INT_PRI

#ifndef _ASMLANGUAGE
extern struct device * const uart_devs[];
#endif

/* Setup console from config value, */
#if defined(CONFIG_UART_CONSOLE)

#if (CONFIG_UART_CONSOLE_INDEX == 0)
#define CONFIG_UART_BAUDRATE		COM1_BAUD_RATE
#define CONFIG_UART_CONSOLE_INT_PRI	CONFIG_UART0_CONSOLE_INT_PRI
#define CONFIG_UART_CONSOLE_IRQ CONFIG_UART0_CONSOLE_IRQ
#elif (CONFIG_UART_CONSOLE_INDEX == 1)
#define CONFIG_UART_BAUDRATE		COM2_BAUD_RATE
#define CONFIG_UART_CONSOLE_IRQ CONFIG_UART1_CONSOLE_IRQ
#define CONFIG_UART_CONSOLE_INT_PRI	CONFIG_UART1_CONSOLE_INT_PRI
#endif  /* CONFIG_UART_CONSOLE_INDEX */

#define UART_CONSOLE_DEV (uart_devs[CONFIG_UART_CONSOLE_INDEX])

#endif /* CONFIG_UART_CONSOLE */

/* Watchdog */
#define WDT_BASE_ADDR			0xB0000000
#define INT_WDT_IRQ			0x10
#define INT_WATCHDOG_MASK		0x47C
#define SCSS_PERIPH_CFG0		0x804
#define SCSS_PERIPH_CFG0_WDT_ENABLE	(1 << 1)
#define CCU_WDT_PCLK_EN_SW		(1 << 10)


/* RTC */
#define RTC_BASE_ADDR                   0xB0000400
#define CCU_RTC_CLK_DIV_OFFSET          0x3
#define INT_RTC_MASK			0x478
#define INT_RTC_IRQ                     0x2
#define CCU_RTC_PCLK_EN_SW              (1 << 11)

#ifndef _ASMLANGUAGE

#endif /* _ASMLANGUAGE */

#endif /* __INCboardh */
