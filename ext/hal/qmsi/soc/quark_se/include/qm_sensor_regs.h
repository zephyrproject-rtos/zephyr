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

#ifndef __SENSOR_REGISTERS_H__
#define __SENSOR_REGISTERS_H__

#include "qm_common.h"

/**
 * Quark SE SoC Sensor Subsystem Registers.
 *
 * For detailed description please read the SOC datasheet.
 *
 * @defgroup groupSSSEREG SoC Registers (Sensor Subsystem)
 * @{
 */

#if (UNIT_TEST)

#define QM_SS_BASE_AUX_REGS_NUM (0x701)
/* Peripherals auxiliary registers start at
 * 0x80010000 and ends at 0x80018180 */
#define QM_SS_PERIPH_AUX_REGS_BASE (0x80010000)
#define QM_SS_PERIPH_AUX_REGS_SIZE (0x8181)
#define QM_SS_AUX_REGS_SIZE                                                    \
	(QM_SS_BASE_AUX_REGS_NUM + QM_SS_PERIPH_AUX_REGS_SIZE)

uint32_t test_sensor_aux[QM_SS_AUX_REGS_SIZE];

#define __builtin_arc_lr(addr)                                                 \
	({                                                                     \
		uint32_t temp = addr;                                          \
		if (temp >= QM_SS_PERIPH_AUX_REGS_BASE) {                      \
			temp -= QM_SS_PERIPH_AUX_REGS_BASE;                    \
			temp += QM_SS_BASE_AUX_REGS_NUM;                       \
		}                                                              \
		(test_sensor_aux[temp]);                                       \
	})

#define __builtin_arc_sr(val, addr)                                            \
	({                                                                     \
		uint32_t temp = addr;                                          \
		if (temp >= QM_SS_PERIPH_AUX_REGS_BASE) {                      \
			temp -= QM_SS_PERIPH_AUX_REGS_BASE;                    \
			temp += QM_SS_BASE_AUX_REGS_NUM;                       \
		}                                                              \
		(test_sensor_aux[temp] = val);                                 \
	})

#define __builtin_arc_kflag(sreg)
#define __builtin_arc_brk()
#define __builtin_arc_clri()
#define __builtin_arc_seti(val)
#define __builtin_arc_nop()
#endif

/* Bitwise OR operation macro for registers in the auxiliary memory space. */
#define QM_SS_REG_AUX_OR(reg, mask)                                            \
	(__builtin_arc_sr(__builtin_arc_lr(reg) | (mask), reg))
/* Bitwise NAND operation macro for registers in the auxiliary memory space. */
#define QM_SS_REG_AUX_NAND(reg, mask)                                          \
	(__builtin_arc_sr(__builtin_arc_lr(reg) & (~(mask)), reg))

/* Sensor Subsystem status32 register. */
#define QM_SS_AUX_STATUS32 (0xA)
/* Sensor Subsystem control register. */
#define QM_SS_AUX_IC_CTRL (0x11)
/* Sensor Subsystem cache invalidate register. */
#define QM_SS_AUX_IC_IVIL (0x19)
/* Sensor Subsystem vector base register. */
#define QM_SS_AUX_INT_VECTOR_BASE (0x25)

/**
 * @name SS Timer
 * @{
 */

typedef enum {
	QM_SS_TIMER_COUNT = 0,
	QM_SS_TIMER_CONTROL,
	QM_SS_TIMER_LIMIT
} qm_ss_timer_reg_t;

/**
 * Sensor Subsystem Timers.
 */
typedef enum { QM_SS_TIMER_0 = 0, QM_SS_TIMER_NUM } qm_ss_timer_t;

#define QM_SS_TIMER_0_BASE (0x21)
#define QM_SS_TIMER_1_BASE (0x100)
#define QM_SS_TSC_BASE QM_SS_TIMER_1_BASE

#define QM_SS_TIMER_CONTROL_INT_EN_OFFSET (0)
#define QM_SS_TIMER_CONTROL_NON_HALTED_OFFSET (1)
#define QM_SS_TIMER_CONTROL_WATCHDOG_OFFSET (2)
#define QM_SS_TIMER_CONTROL_INT_PENDING_OFFSET (3)
/** @} */

/**
 * GPIO registers and definitions.
 *
 * @name SS GPIO
 * @{
 */

/** Sensor Subsystem GPIO register block type. */
typedef enum {
	QM_SS_GPIO_SWPORTA_DR = 0,
	QM_SS_GPIO_SWPORTA_DDR,
	QM_SS_GPIO_INTEN = 3,
	QM_SS_GPIO_INTMASK,
	QM_SS_GPIO_INTTYPE_LEVEL,
	QM_SS_GPIO_INT_POLARITY,
	QM_SS_GPIO_INTSTATUS,
	QM_SS_GPIO_DEBOUNCE,
	QM_SS_GPIO_PORTA_EOI,
	QM_SS_GPIO_EXT_PORTA,
	QM_SS_GPIO_LS_SYNC
} qm_ss_gpio_reg_t;

#define QM_SS_GPIO_NUM_PINS (16)
#define QM_SS_GPIO_LS_SYNC_CLK_EN BIT(31)
#define QM_SS_GPIO_LS_SYNC_SYNC_LVL BIT(0)

/** Sensor Subsystem GPIO. */
typedef enum { QM_SS_GPIO_0 = 0, QM_SS_GPIO_1, QM_SS_GPIO_NUM } qm_ss_gpio_t;

#define QM_SS_GPIO_0_BASE (0x80017800)
#define QM_SS_GPIO_1_BASE (0x80017900)

/** @} */

/**
 * I2C registers and definitions.
 *
 * @name SS I2C
 * @{
 */

/** Sensor Subsystem I2C register block type. */
typedef enum {
	QM_SS_I2C_CON = 0,
	QM_SS_I2C_DATA_CMD,
	QM_SS_I2C_SS_SCL_CNT,
	QM_SS_I2C_FS_SCL_CNT = 0x04,
	QM_SS_I2C_INTR_STAT = 0x06,
	QM_SS_I2C_INTR_MASK,
	QM_SS_I2C_TL,
	QM_SS_I2C_INTR_CLR = 0x0A,
	QM_SS_I2C_STATUS,
	QM_SS_I2C_TXFLR,
	QM_SS_I2C_RXFLR,
	QM_SS_I2C_SDA_CONFIG,
	QM_SS_I2C_TX_ABRT_SOURCE,
	QM_SS_I2C_ENABLE_STATUS = 0x11
} qm_ss_i2c_reg_t;

#define QM_SS_I2C_CON_ENABLE BIT(0)
#define QM_SS_I2C_CON_ABORT BIT(1)
#define QM_SS_I2C_CON_SPEED_SS BIT(3)
#define QM_SS_I2C_CON_SPEED_FS BIT(4)
#define QM_SS_I2C_CON_SPEED_MASK (0x18)
#define QM_SS_I2C_CON_IC_10BITADDR BIT(5)
#define QM_SS_I2C_CON_IC_10BITADDR_OFFSET (5)
#define QM_SS_I2C_CON_IC_10BITADDR_MASK (5)
#define QM_SS_I2C_CON_RESTART_EN BIT(7)
#define QM_SS_I2C_CON_TAR_SAR_OFFSET (9)
#define QM_SS_I2C_CON_TAR_SAR_MASK (0x7FE00)
#define QM_SS_I2C_CON_TAR_SAR_10_BIT_MASK (0x3FF)
#define QM_SS_I2C_CON_SPKLEN_OFFSET (22)
#define QM_SS_I2C_CON_SPKLEN_MASK (0x3FC00000)
#define QM_SS_I2C_CON_CLK_ENA BIT(31)

#define QM_SS_I2C_DATA_CMD_CMD BIT(8)
#define QM_SS_I2C_DATA_CMD_STOP BIT(9)
#define QM_SS_I2C_DATA_CMD_PUSH (0xC0000000)
#define QM_SS_I2C_DATA_CMD_POP (0x80000000)

#define QM_SS_I2C_SS_FS_SCL_CNT_HCNT_OFFSET (16)
#define QM_SS_I2C_SS_FS_SCL_CNT_16BIT_MASK (0xFFFF)

#define QM_SS_I2C_INTR_STAT_RX_UNDER BIT(0)
#define QM_SS_I2C_INTR_STAT_RX_OVER BIT(1)
#define QM_SS_I2C_INTR_STAT_RX_FULL BIT(2)
#define QM_SS_I2C_INTR_STAT_TX_OVER BIT(3)
#define QM_SS_I2C_INTR_STAT_TX_EMPTY BIT(4)
#define QM_SS_I2C_INTR_STAT_TX_ABRT BIT(6)

#define QM_SS_I2C_INTR_MASK_ALL (0x0)
#define QM_SS_I2C_INTR_MASK_RX_UNDER BIT(0)
#define QM_SS_I2C_INTR_MASK_RX_OVER BIT(1)
#define QM_SS_I2C_INTR_MASK_RX_FULL BIT(2)
#define QM_SS_I2C_INTR_MASK_TX_OVER BIT(3)
#define QM_SS_I2C_INTR_MASK_TX_EMPTY BIT(4)
#define QM_SS_I2C_INTR_MASK_TX_ABRT BIT(6)

#define QM_SS_I2C_TL_TX_TL_OFFSET (16)
#define QM_SS_I2C_TL_RX_TL_MASK (0xFF)
#define QM_SS_I2C_TL_TX_TL_MASK (0xFF0000)

#define QM_SS_I2C_INTR_CLR_ALL (0xFF)
#define QM_SS_I2C_INTR_CLR_TX_ABRT BIT(6)

#define QM_SS_I2C_TX_ABRT_SOURCE_NAK_MASK (0x09)
#define QM_SS_I2C_TX_ABRT_SOURCE_ALL_MASK (0x1FFFF)
#define QM_SS_I2C_TX_ABRT_SBYTE_NORSTRT BIT(9)
#define QM_SS_I2C_TX_ABRT_SOURCE_ART_LOST BIT(12)

#define QM_SS_I2C_ENABLE_CONTROLLER_EN BIT(0)
#define QM_SS_I2C_ENABLE_STATUS_IC_EN BIT(0)

#define QM_SS_I2C_STATUS_BUSY_MASK (0x21)
#define QM_SS_I2C_STATUS_RFNE BIT(3)
#define QM_SS_I2C_STATUS_TFE BIT(2)
#define QM_SS_I2C_STATUS_TFNF BIT(1)

#define QM_SS_I2C_IC_LCNT_MAX (65525)
#define QM_SS_I2C_IC_LCNT_MIN (8)
#define QM_SS_I2C_IC_HCNT_MAX (65525)
#define QM_SS_I2C_IC_HCNT_MIN (6)

#define QM_SS_I2C_FIFO_SIZE (8)

/** Sensor Subsystem I2C */
typedef enum { QM_SS_I2C_0 = 0, QM_SS_I2C_1, QM_SS_I2C_NUM } qm_ss_i2c_t;

#define QM_SS_I2C_0_BASE (0x80012000)
#define QM_SS_I2C_1_BASE (0x80012100)

/** @} */
/** Sensor Subsystem ADC @{*/

/** Sensor Subsystem ADC registers */
typedef enum {
	QM_SS_ADC_SET = 0,    /**< ADC and sequencer settings register. */
	QM_SS_ADC_DIVSEQSTAT, /**< ADC clock and sequencer status register. */
	QM_SS_ADC_SEQ,	/**< ADC sequence entry register. */
	QM_SS_ADC_CTRL,       /**< ADC control register. */
	QM_SS_ADC_INTSTAT,    /**< ADC interrupt status register. */
	QM_SS_ADC_SAMPLE      /**< ADC sample register. */
} qm_ss_adc_reg_t;

/** Sensor Subsystem ADC */
typedef enum {
	QM_SS_ADC_0 = 0, /**< ADC first module. */
	QM_SS_ADC_NUM
} qm_ss_adc_t;

/* SS ADC register base. */
#define QM_SS_ADC_BASE (0x80015000)

/* For 1MHz, the max divisor is 7. */
#define QM_SS_ADC_DIV_MAX (7)

#define QM_SS_ADC_FIFO_LEN (32)

#define QM_SS_ADC_SET_POP_RX BIT(31)
#define QM_SS_ADC_SET_FLUSH_RX BIT(30)
#define QM_SS_ADC_SET_THRESHOLD_MASK (0x3F000000)
#define QM_SS_ADC_SET_THRESHOLD_OFFSET (24)
#define QM_SS_ADC_SET_SEQ_ENTRIES_MASK (0x3F0000)
#define QM_SS_ADC_SET_SEQ_ENTRIES_OFFSET (16)
#define QM_SS_ADC_SET_SEQ_MODE BIT(13)
#define QM_SS_ADC_SET_SAMPLE_WIDTH_MASK (0x1F)

#define QM_SS_ADC_DIVSEQSTAT_CLK_RATIO_MASK (0x1FFFFF)

#define QM_SS_ADC_CTRL_CLR_SEQERROR BIT(19)
#define QM_SS_ADC_CTRL_CLR_UNDERFLOW BIT(18)
#define QM_SS_ADC_CTRL_CLR_OVERFLOW BIT(17)
#define QM_SS_ADC_CTRL_CLR_DATA_A BIT(16)
#define QM_SS_ADC_CTRL_MSK_SEQERROR BIT(11)
#define QM_SS_ADC_CTRL_MSK_UNDERFLOW BIT(10)
#define QM_SS_ADC_CTRL_MSK_OVERFLOW BIT(9)
#define QM_SS_ADC_CTRL_MSK_DATA_A BIT(8)
#define QM_SS_ADC_CTRL_SEQ_TABLE_RST BIT(6)
#define QM_SS_ADC_CTRL_SEQ_PTR_RST BIT(5)
#define QM_SS_ADC_CTRL_SEQ_START BIT(4)
#define QM_SS_ADC_CTRL_CLK_ENA BIT(2)
#define QM_SS_ADC_CTRL_ADC_ENA BIT(1)

#define QM_SS_ADC_CTRL_MSK_ALL_INT (0xF00)
#define QM_SS_ADC_CTRL_CLR_ALL_INT (0xF0000)

#define QM_SS_ADC_SEQ_DELAYODD_OFFSET (21)
#define QM_SS_ADC_SEQ_MUXODD_OFFSET (16)
#define QM_SS_ADC_SEQ_DELAYEVEN_OFFSET (5)

#define QM_SS_ADC_SEQ_DUMMY (0x480)

#define QM_SS_ADC_INTSTAT_SEQERROR BIT(3)
#define QM_SS_ADC_INTSTAT_UNDERFLOW BIT(2)
#define QM_SS_ADC_INTSTAT_OVERFLOW BIT(1)
#define QM_SS_ADC_INTSTAT_DATA_A BIT(0)

/** End of Sensor Subsystem ADC @}*/

/**
 * CREG Registers.
 *
 * @name SS CREG
 * @{
 */

/* Sensor Subsystem CREG */
typedef enum {
	QM_SS_IO_CREG_MST0_CTRL = 0x0,  /**< Master control register. */
	QM_SS_IO_CREG_SLV0_OBSR = 0x80, /**< Slave control register. */
	QM_SS_IO_CREG_SLV1_OBSR = 0x180 /**< Slave control register. */
} qm_ss_creg_reg_t;

/* MST0_CTRL fields */
#define QM_SS_IO_CREG_MST0_CTRL_ADC_PWR_MODE_OFFSET (1)
#define QM_SS_IO_CREG_MST0_CTRL_ADC_PWR_MODE_MASK (0x7)
#define QM_SS_IO_CREG_MST0_CTRL_ADC_DELAY_OFFSET (3)
#define QM_SS_IO_CREG_MST0_CTRL_ADC_DELAY_MASK (0xFFF8)
#define QM_SS_IO_CREG_MST0_CTRL_ADC_CAL_REQ BIT(16)
#define QM_SS_IO_CREG_MST0_CTRL_ADC_CAL_CMD_OFFSET (17)
#define QM_SS_IO_CREG_MST0_CTRL_ADC_CAL_CMD_MASK (0xE0000)
#define QM_SS_IO_CREG_MST0_CTRL_ADC_CAL_VAL_OFFSET (20)
#define QM_SS_IO_CREG_MST0_CTRL_ADC_CAL_VAL_MASK (0x7F00000)
#define QM_SS_IO_CREG_MST0_CTRL_ADC_CAL_VAL_MAX (0x7F)
#define QM_SS_IO_CREG_MST0_CTRL_SPI1_CLK_GATE BIT(27)
#define QM_SS_IO_CREG_MST0_CTRL_SPI0_CLK_GATE BIT(28)
#define QM_SS_IO_CREG_MST0_CTRL_I2C0_CLK_GATE BIT(29)
#define QM_SS_IO_CREG_MST0_CTRL_I2C1_CLK_GATE BIT(30)
#define QM_SS_IO_CREG_MST0_CTRL_ADC_CLK_GATE BIT(31)
/* SLV0_OBSR fields */
#define QM_SS_IO_CREG_SLV0_OBSR_ADC_CAL_VAL_OFFSET (5)
#define QM_SS_IO_CREG_SLV0_OBSR_ADC_CAL_VAL_MASK (0xFE0)
#define QM_SS_IO_CREG_SLV0_OBSR_ADC_CAL_ACK BIT(4)
#define QM_SS_IO_CREG_SLV0_OBSR_ADC_PWR_MODE_STS BIT(3)

#define SS_CLK_PERIPH_ALL_IN_CREG                                              \
	(SS_CLK_PERIPH_ADC | SS_CLK_PERIPH_I2C_1 | SS_CLK_PERIPH_I2C_0 |       \
	 SS_CLK_PERIPH_SPI_1 | SS_CLK_PERIPH_SPI_0)

/* SS CREG base. */
#define QM_SS_CREG_BASE (0x80018000)

/** @} */

/**
 * IRQs and interrupt vectors.
 *
 * @name SS Interrupt
 * @{
 */

#define QM_SS_EXCEPTION_NUM (16)  /* Exceptions and traps in ARC EM core. */
#define QM_SS_INT_TIMER_NUM (2)   /* Internal interrupts in ARC EM core. */
#define QM_SS_IRQ_SENSOR_NUM (18) /* IRQ's from the Sensor Subsystem. */
#define QM_SS_IRQ_COMMON_NUM (32) /* IRQ's from the common SoC fabric. */
#define QM_SS_INT_VECTOR_NUM                                                   \
	(QM_SS_EXCEPTION_NUM + QM_SS_INT_TIMER_NUM + QM_SS_IRQ_SENSOR_NUM +    \
	 QM_SS_IRQ_COMMON_NUM)
#define QM_SS_IRQ_NUM (QM_SS_IRQ_SENSOR_NUM + QM_SS_IRQ_COMMON_NUM)

/*
 * The following definitions are Sensor Subsystem interrupt irq and vector
 * numbers:
 * #define QM_SS_xxx          - irq number
 * #define QM_SS_xxx_VECTOR   - vector number
 */
#define QM_SS_INT_TIMER_0 16
#define QM_SS_INT_TIMER_1 17

#define QM_SS_IRQ_ADC_ERR 0
#define QM_SS_IRQ_ADC_ERR_VECTOR 18

#define QM_SS_IRQ_ADC_IRQ 1
#define QM_SS_IRQ_ADC_IRQ_VECTOR 19

#define QM_SS_IRQ_GPIO_INTR_0 2
#define QM_SS_IRQ_GPIO_INTR_0_VECTOR 20

#define QM_SS_IRQ_GPIO_INTR_1 3
#define QM_SS_IRQ_GPIO_INTR_1_VECTOR 21

#define QM_SS_IRQ_I2C_0_ERR 4
#define QM_SS_IRQ_I2C_0_ERR_VECTOR 22

#define QM_SS_IRQ_I2C_0_RX_AVAIL 5
#define QM_SS_IRQ_I2C_0_RX_AVAIL_VECTOR 23

#define QM_SS_IRQ_I2C_0_TX_REQ 6
#define QM_SS_IRQ_I2C_0_TX_REQ_VECTOR 24

#define QM_SS_IRQ_I2C_0_STOP_DET 7
#define QM_SS_IRQ_I2C_0_STOP_DET_VECTOR 25

#define QM_SS_IRQ_I2C_1_ERR 8
#define QM_SS_IRQ_I2C_1_ERR_VECTOR 26

#define QM_SS_IRQ_I2C_1_RX_AVAIL 9
#define QM_SS_IRQ_I2C_1_RX_AVAIL_VECTOR 27

#define QM_SS_IRQ_I2C_1_TX_REQ 10
#define QM_SS_IRQ_I2C_1_TX_REQ_VECTOR 28

#define QM_SS_IRQ_I2C_1_STOP_DET 11
#define QM_SS_IRQ_I2C_1_STOP_DET_VECTOR 29

#define QM_SS_IRQ_SPI_0_ERR_INT 12
#define QM_SS_IRQ_SPI_0_ERR_INT_VECTOR 30

#define QM_SS_IRQ_SPI_0_RX_AVAIL 13
#define QM_SS_IRQ_SPI_0_RX_AVAIL_VECTOR 31

#define QM_SS_IRQ_SPI_0_TX_REQ 14
#define QM_SS_IRQ_SPI_0_TX_REQ_VECTOR 32

#define QM_SS_IRQ_SPI_1_ERR_INT 15
#define QM_SS_IRQ_SPI_1_ERR_INT_VECTOR 33

#define QM_SS_IRQ_SPI_1_RX_AVAIL 16
#define QM_SS_IRQ_SPI_1_RX_AVAIL_VECTOR 34

#define QM_SS_IRQ_SPI_1_TX_REQ 17
#define QM_SS_IRQ_SPI_1_TX_REQ_VECTOR 35

typedef enum {
	QM_SS_INT_PRIORITY_0 = 0,
	QM_SS_INT_PRIORITY_1 = 1,
	QM_SS_INT_PRIORITY_15 = 15,
	QM_SS_INT_PRIORITY_NUM
} qm_ss_irq_priority_t;

typedef enum { QM_SS_INT_DISABLE = 0, QM_SS_INT_ENABLE = 1 } qm_ss_irq_mask_t;

typedef enum {
	QM_SS_IRQ_LEVEL_SENSITIVE = 0,
	QM_SS_IRQ_EDGE_SENSITIVE = 1
} qm_ss_irq_trigger_t;

#define QM_SS_AUX_IRQ_CTRL (0xE)
#define QM_SS_AUX_IRQ_HINT (0x201)
#define QM_SS_AUX_IRQ_PRIORITY (0x206)
#define QM_SS_AUX_IRQ_STATUS (0x406)
#define QM_SS_AUX_IRQ_SELECT (0x40B)
#define QM_SS_AUX_IRQ_ENABLE (0x40C)
#define QM_SS_AUX_IRQ_TRIGGER (0x40D)

/** @} */

/**
 * I2C registers and definitions.
 *
 * @name SS SPI
 * @{
 */

/** Sensor Subsystem SPI register map. */
typedef enum {
	QM_SS_SPI_CTRL = 0,   /**< SPI control register. */
	QM_SS_SPI_SPIEN = 2,  /**< SPI enable register. */
	QM_SS_SPI_TIMING = 4, /**< SPI serial clock divider value. */
	QM_SS_SPI_FTLR,       /**< Threshold value for TX/RX FIFO. */
	QM_SS_SPI_TXFLR = 7,  /**< Number of valid data entries in TX FIFO. */
	QM_SS_SPI_RXFLR,      /**< Number of valid data entries in RX FIFO. */
	QM_SS_SPI_SR,	 /**< SPI status register.	*/
	QM_SS_SPI_INTR_STAT,  /**< Interrupt status register. */
	QM_SS_SPI_INTR_MASK,  /**< Interrupt mask register. */
	QM_SS_SPI_CLR_INTR,   /**< Interrupt clear register. */
	QM_SS_SPI_DR,	 /**< RW buffer for FIFOs. */
} qm_ss_spi_reg_t;

/** Sensor Subsystem SPI modules. */
typedef enum {
	QM_SS_SPI_0 = 0, /**< SPI module 0 */
	QM_SS_SPI_1,     /**< SPI module 1 */
	QM_SS_SPI_NUM
} qm_ss_spi_t;

#define QM_SS_SPI_0_BASE (0x80010000)
#define QM_SS_SPI_1_BASE (0x80010100)

#define QM_SS_SPI_CTRL_DFS_OFFS (0)
#define QM_SS_SPI_CTRL_DFS_MASK (0x0000000F)
#define QM_SS_SPI_CTRL_BMOD_OFFS (6)
#define QM_SS_SPI_CTRL_BMOD_MASK (0x000000C0)
#define QM_SS_SPI_CTRL_SCPH BIT(6)
#define QM_SS_SPI_CTRL_SCPOL BIT(7)
#define QM_SS_SPI_CTRL_TMOD_OFFS (8)
#define QM_SS_SPI_CTRL_TMOD_MASK (0x00000300)
#define QM_SS_SPI_CTRL_SRL BIT(11)
#define QM_SS_SPI_CTRL_CLK_ENA BIT(15)
#define QM_SS_SPI_CTRL_NDF_OFFS (16)
#define QM_SS_SPI_CTRL_NDF_MASK (0xFFFF0000)

#define QM_SS_SPI_SPIEN_EN BIT(0)
#define QM_SS_SPI_SPIEN_SER_OFFS (4)
#define QM_SS_SPI_SPIEN_SER_MASK (0x000000F0)

#define QM_SS_SPI_TIMING_SCKDV_OFFS (0)
#define QM_SS_SPI_TIMING_SCKDV_MASK (0x0000FFFF)
#define QM_SS_SPI_TIMING_RSD_OFFS (16)
#define QM_SS_SPI_TIMING_RSD_MASK (0x00FF0000)

#define QM_SS_SPI_FTLR_RFT_OFFS (0)
#define QM_SS_SPI_FTLR_RFT_MASK (0x0000FFFF)
#define QM_SS_SPI_FTLR_TFT_OFFS (16)
#define QM_SS_SPI_FTLR_TFT_MASK (0xFFFF0000)

#define QM_SS_SPI_SR_BUSY BIT(0)
#define QM_SS_SPI_SR_TFNF BIT(1)
#define QM_SS_SPI_SR_TFE BIT(2)
#define QM_SS_SPI_SR_RFNE BIT(3)
#define QM_SS_SPI_SR_RFF BIT(4)

#define QM_SS_SPI_INTR_TXEI BIT(0)
#define QM_SS_SPI_INTR_TXOI BIT(1)
#define QM_SS_SPI_INTR_RXUI BIT(2)
#define QM_SS_SPI_INTR_RXOI BIT(3)
#define QM_SS_SPI_INTR_RXFI BIT(4)
#define QM_SS_SPI_INTR_ALL (0x0000001F)

#define QM_SS_SPI_INTR_STAT_TXEI QM_SS_SPI_INTR_TXEI
#define QM_SS_SPI_INTR_STAT_TXOI QM_SS_SPI_INTR_TXOI
#define QM_SS_SPI_INTR_STAT_RXUI QM_SS_SPI_INTR_RXUI
#define QM_SS_SPI_INTR_STAT_RXOI QM_SS_SPI_INTR_RXOI
#define QM_SS_SPI_INTR_STAT_RXFI QM_SS_SPI_INTR_RXFI

#define QM_SS_SPI_INTR_MASK_TXEI QM_SS_SPI_INTR_TXEI
#define QM_SS_SPI_INTR_MASK_TXOI QM_SS_SPI_INTR_TXOI
#define QM_SS_SPI_INTR_MASK_RXUI QM_SS_SPI_INTR_RXUI
#define QM_SS_SPI_INTR_MASK_RXOI QM_SS_SPI_INTR_RXOI
#define QM_SS_SPI_INTR_MASK_RXFI QM_SS_SPI_INTR_RXFI

#define QM_SS_SPI_CLR_INTR_TXEI QM_SS_SPI_INTR_TXEI
#define QM_SS_SPI_CLR_INTR_TXOI QM_SS_SPI_INTR_TXOI
#define QM_SS_SPI_CLR_INTR_RXUI QM_SS_SPI_INTR_RXUI
#define QM_SS_SPI_CLR_INTR_RXOI QM_SS_SPI_INTR_RXOI
#define QM_SS_SPI_CLR_INTR_RXFI QM_SS_SPI_INTR_RXFI

#define QM_SS_SPI_DR_DR_OFFS (0)
#define QM_SS_SPI_DR_DR_MASK (0x0000FFFF)
#define QM_SS_SPI_DR_WR BIT(30)
#define QM_SS_SPI_DR_STROBE BIT(31)
#define QM_SS_SPI_DR_W_MASK (0xc0000000)
#define QM_SS_SPI_DR_R_MASK (0x80000000)

/** @} */
/** @} */

#endif /* __SENSOR_REGISTERS_H__ */
