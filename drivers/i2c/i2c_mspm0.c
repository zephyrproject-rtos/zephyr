/*
 * Copyright (c) 2026 Texas Instruments Incorporated
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_mspm0_i2c

/* Zephyr includes */
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/mspm0_clock_control.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/dt-bindings/i2c/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/sys_io.h>
#include <soc.h>

#include "i2c-priv.h"

/*
 * MSPM0 I2C hardware register offsets
 */

/* GPRCM (Power/Reset/Clock management) */
#define I2C_REG_PWREN  0x0800
#define I2C_REG_RSTCTL 0x0804

/* Top-level clock and debug */
#define I2C_REG_CLKDIV  0x1000
#define I2C_REG_CLKSEL  0x1004
#define I2C_REG_PDBGCTL 0x1018

/* CPU interrupt registers */
#define I2C_REG_CPU_INT_IIDX  0x1020
#define I2C_REG_CPU_INT_IMASK 0x1028
#define I2C_REG_CPU_INT_RIS   0x1030
#define I2C_REG_CPU_INT_MIS   0x1038
#define I2C_REG_CPU_INT_ISET  0x1040
#define I2C_REG_CPU_INT_ICLR  0x1048

/* Glitch filter and timeout */
#define I2C_REG_GFCTL       0x1200
#define I2C_REG_TIMEOUT_CTL 0x1204
#define I2C_REG_TIMEOUT_CNT 0x1208

/* Controller registers */
#define I2C_REG_MSA      0x1210
#define I2C_REG_MCTR     0x1214
#define I2C_REG_MSR      0x1218
#define I2C_REG_MRXDATA  0x121C
#define I2C_REG_MTXDATA  0x1220
#define I2C_REG_MTPR     0x1224
#define I2C_REG_MCR      0x1228
#define I2C_REG_MFIFOCTL 0x1238
#define I2C_REG_MFIFOSR  0x123C

/* Target registers */
#define I2C_REG_SOAR     0x1250
#define I2C_REG_SCTR     0x1258
#define I2C_REG_SRXDATA  0x1260
#define I2C_REG_STXDATA  0x1264
#define I2C_REG_SACKCTL  0x1268
#define I2C_REG_SFIFOCTL 0x126C
#define I2C_REG_SFIFOSR  0x1270

/* Register bit constants (guarded: SDK's hw_i2c.h defines the same names) */
#ifndef CONFIG_HAS_MSPM0_SDK

/* GPRCM: Power / Reset */
#define I2C_PWREN_KEY_UNLOCK_W        ((uint32_t)0x26000000U)
#define I2C_PWREN_ENABLE_ENABLE       BIT(0)
#define I2C_PWREN_ENABLE_DISABLE      0
#define I2C_RSTCTL_KEY_UNLOCK_W       ((uint32_t)0xB1000000U)
#define I2C_RSTCTL_RESETSTKYCLR_CLR   BIT(1)
#define I2C_RSTCTL_RESETASSERT_ASSERT BIT(0)

/* Clock */
#define I2C_CLKSEL_MFCLK_SEL_MASK  BIT(2)
#define I2C_CLKSEL_BUSCLK_SEL_MASK BIT(3)
#define I2C_CLKDIV_RATIO_MASK      GENMASK(2, 0)
#define I2C_CLKDIV_RATIO_DIV_BY_1  0

/* Glitch filter */
#define I2C_GFCTL_AGFEN_MASK BIT(8)

/* Timeout */
#define I2C_TIMEOUT_CTL_TCNTLA_MASK    GENMASK(7, 0)
#define I2C_TIMEOUT_CTL_TCNTAEN_ENABLE BIT(15)

/* Controller: MSA (Target Address) */
#define I2C_MSA_SADDR_MASK   GENMASK(10, 1)
#define I2C_MSA_DIR_MASK     BIT(0)
#define I2C_MSA_DIR_TRANSMIT 0
#define I2C_MSA_DIR_RECEIVE  BIT(0)
#define I2C_MSA_MMODE_MASK   BIT(15)
#define I2C_MSA_MMODE_MODE7  0
#define I2C_MSA_MMODE_MODE10 BIT(15)

/* Controller: MCTR (Control) */
#define I2C_MCTR_BURSTRUN_MASK   BIT(0)
#define I2C_MCTR_BURSTRUN_ENABLE BIT(0)
#define I2C_MCTR_START_MASK      BIT(1)
#define I2C_MCTR_START_DISABLE   0
#define I2C_MCTR_START_ENABLE    BIT(1)
#define I2C_MCTR_STOP_MASK       BIT(2)
#define I2C_MCTR_STOP_DISABLE    0
#define I2C_MCTR_STOP_ENABLE     BIT(2)
#define I2C_MCTR_ACK_MASK        BIT(3)
#define I2C_MCTR_ACK_DISABLE     0
#define I2C_MCTR_ACK_ENABLE      BIT(3)
#define I2C_MCTR_MBLEN_MASK      GENMASK(27, 16)

/* Controller: MSR (Status) */
#define I2C_MSR_ERR_MASK BIT(1)

/* Controller: RX/TX data */
#define I2C_MRXDATA_VALUE_MASK GENMASK(7, 0)

/* Controller: MCR (Configuration) */
#define I2C_MCR_ACTIVE_MASK       BIT(0)
#define I2C_MCR_ACTIVE_ENABLE     BIT(0)
#define I2C_MCR_CLKSTRETCH_ENABLE BIT(2)

/* Controller: MFIFOCTL / MFIFOSR */
#define I2C_MFIFOCTL_TXTRIG_MASK      GENMASK(2, 0)
#define I2C_MFIFOCTL_TXTRIG_LEVEL_1   FIELD_PREP(GENMASK(2, 0), 1)
#define I2C_MFIFOCTL_TXFLUSH_MASK     BIT(7)
#define I2C_MFIFOCTL_RXTRIG_MASK      GENMASK(10, 8)
#define I2C_MFIFOCTL_RXTRIG_LEVEL_1   FIELD_PREP(GENMASK(10, 8), 0)
#define I2C_MFIFOSR_TXFIFOCNT_MASK    GENMASK(11, 8)
#define I2C_MFIFOSR_TXFIFOCNT_MINIMUM 0
#define I2C_MFIFOSR_RXFIFOCNT_MASK    GENMASK(3, 0)
#define I2C_MFIFOSR_RXFIFOCNT_MINIMUM 0

/* Target: SOAR (Own Address) */
#define I2C_SOAR_OAR_MASK     GENMASK(9, 0)
#define I2C_SOAR_SMODE_MASK   BIT(15)
#define I2C_SOAR_SMODE_MODE7  0
#define I2C_SOAR_SMODE_MODE10 BIT(15)

/* Target: SCTR (Control) */
#define I2C_SCTR_ACTIVE_MASK            BIT(0)
#define I2C_SCTR_ACTIVE_ENABLE          BIT(0)
#define I2C_SCTR_SCLKSTRETCH_ENABLE     BIT(2)
#define I2C_SCTR_TXEMPTY_ON_TREQ_ENABLE BIT(3)
#define I2C_SCTR_TXTRIG_TXMODE_ENABLE   BIT(4)
#define I2C_SCTR_SWUEN_MASK             BIT(10)

/* Target: SACKCTL */
#define I2C_SACKCTL_ACKOVAL_MASK    BIT(1)
#define I2C_SACKCTL_ACKOVAL_DISABLE 0      /* ACK  */
#define I2C_SACKCTL_ACKOVAL_ENABLE  BIT(1) /* NACK */

/* Target: SFIFOCTL / SFIFOSR */
#define I2C_SFIFOCTL_TXTRIG_MASK      GENMASK(2, 0)
#define I2C_SFIFOCTL_TXFLUSH_MASK     BIT(7)
#define I2C_SFIFOCTL_RXTRIG_MASK      GENMASK(10, 8)
#define I2C_SFIFOSR_TXFIFOCNT_MASK    GENMASK(11, 8)
#define I2C_SFIFOSR_TXFIFOCNT_MINIMUM 0
#define I2C_SFIFOSR_RXFIFOCNT_MASK    GENMASK(3, 0)
#define I2C_SFIFOSR_RXFIFOCNT_MINIMUM 0

/* Target: RX/TX data */
#define I2C_SRXDATA_VALUE_MASK GENMASK(7, 0)

/* CPU_INT: IIDX values (pending interrupt index) */
#define I2C_CPU_INT_IIDX_STAT_MRXDONEFG  0x00000001U
#define I2C_CPU_INT_IIDX_STAT_MTXDONEFG  0x00000002U
#define I2C_CPU_INT_IIDX_STAT_MRXFIFOTRG 0x00000003U
#define I2C_CPU_INT_IIDX_STAT_MTXFIFOTRG 0x00000004U
#define I2C_CPU_INT_IIDX_STAT_MNACKFG    0x00000008U
#define I2C_CPU_INT_IIDX_STAT_MSTOPFG    0x0000000AU
#define I2C_CPU_INT_IIDX_STAT_MARBLOSTFG 0x0000000BU
#define I2C_CPU_INT_IIDX_STAT_TIMEOUTA   0x0000000FU
#define I2C_CPU_INT_IIDX_STAT_SRXDONEFG  0x00000011U
#define I2C_CPU_INT_IIDX_STAT_STXEMPTY   0x00000016U
#define I2C_CPU_INT_IIDX_STAT_SSTARTFG   0x00000017U
#define I2C_CPU_INT_IIDX_STAT_SSTOPFG    0x00000018U

/* CPU_INT: IMASK bit positions (interrupt enable/mask) */
#define I2C_CPU_INT_IMASK_MRXDONE_SET    BIT(0)
#define I2C_CPU_INT_IMASK_MTXDONE_SET    BIT(1)
#define I2C_CPU_INT_IMASK_MRXFIFOTRG_SET BIT(2)
#define I2C_CPU_INT_IMASK_MTXFIFOTRG_SET BIT(3)
#define I2C_CPU_INT_IMASK_MNACK_SET      BIT(7)
#define I2C_CPU_INT_IMASK_MSTOP_SET      BIT(9)
#define I2C_CPU_INT_IMASK_MARBLOST_SET   BIT(10)
#define I2C_CPU_INT_IMASK_TIMEOUTA_SET   BIT(14)
#define I2C_CPU_INT_IMASK_SRXDONE_SET    BIT(16)
#define I2C_CPU_INT_IMASK_STXEMPTY_SET   BIT(21)
#define I2C_CPU_INT_IMASK_SSTART_SET     BIT(22)
#define I2C_CPU_INT_IMASK_SSTOP_SET      BIT(23)

#endif /* !CONFIG_HAS_MSPM0_SDK */

/* FIFO depth (device-specific via Kconfig) */
#define I2C_FIFO_DEPTH CONFIG_MSPM0_I2C_FIFO_DEPTH

LOG_MODULE_REGISTER(i2c_mspm0, CONFIG_I2C_LOG_LEVEL);

/* Local macro definitions for register values */
#define I2C_CONTROLLER_STOP_ENABLE              I2C_MCTR_STOP_ENABLE
#define I2C_CONTROLLER_STOP_DISABLE             I2C_MCTR_STOP_DISABLE
#define I2C_CONTROLLER_START_ENABLE             I2C_MCTR_START_ENABLE
#define I2C_CONTROLLER_START_DISABLE            I2C_MCTR_START_DISABLE
#define I2C_CONTROLLER_ACK_ENABLE               I2C_MCTR_ACK_ENABLE
#define I2C_CONTROLLER_ACK_DISABLE              I2C_MCTR_ACK_DISABLE
#define I2C_CONTROLLER_DIRECTION_TX             I2C_MSA_DIR_TRANSMIT
#define I2C_CONTROLLER_DIRECTION_RX             I2C_MSA_DIR_RECEIVE
#define I2C_TX_FIFO_LEVEL_BYTES_1               I2C_MFIFOCTL_TXTRIG_LEVEL_1
#define I2C_RX_FIFO_LEVEL_BYTES_1               I2C_MFIFOCTL_RXTRIG_LEVEL_1
#define I2C_TARGET_RESPONSE_OVERRIDE_VALUE_ACK  I2C_SACKCTL_ACKOVAL_DISABLE
#define I2C_TARGET_RESPONSE_OVERRIDE_VALUE_NACK I2C_SACKCTL_ACKOVAL_ENABLE
#define I2C_CONTROLLER_STATUS_ERROR             I2C_MSR_ERR_MASK
#define I2C_CLOCK_DIVIDE_1                      I2C_CLKDIV_RATIO_DIV_BY_1

/* Helper macro for register field update */
#define I2C_UPDATE_REG(base, off, value, mask)                                                     \
	sys_write32((sys_read32((base) + (off)) & ~(mask)) | ((value) & (mask)), (base) + (off))

/* Local clock configuration struct to replace DL_I2C_ClockConfig */
typedef struct {
	uint32_t clock_sel;
	uint32_t divide_ratio;
} i2c_clock_config_t;

#define TI_MSPM0_CONTROLLER_INTERRUPTS                                                             \
	(I2C_CPU_INT_IMASK_MARBLOST_SET | I2C_CPU_INT_IMASK_MNACK_SET |                            \
	 I2C_CPU_INT_IMASK_MRXFIFOTRG_SET | I2C_CPU_INT_IMASK_MRXDONE_SET |                        \
	 I2C_CPU_INT_IMASK_MTXDONE_SET | I2C_CPU_INT_IMASK_TIMEOUTA_SET |                          \
	 I2C_CPU_INT_IMASK_MSTOP_SET)

/*
 * Mask covering all possible controller interrupt sources, including
 * I2C_CPU_INT_IMASK_MTXFIFOTRG_SET which is dynamically
 * enabled/disabled during multi-byte transmits. Used in the dual-role ISR
 * dispatcher to distinguish controller from target interrupts via MIS.
 */
#define TI_MSPM0_CONTROLLER_INTERRUPTS_ALL                                                         \
	(TI_MSPM0_CONTROLLER_INTERRUPTS | I2C_CPU_INT_IMASK_MTXFIFOTRG_SET)

#define TI_MSPM0_TARGET_INTERRUPTS                                                                 \
	(I2C_CPU_INT_IMASK_SRXDONE_SET | I2C_CPU_INT_IMASK_STXEMPTY_SET |                          \
	 I2C_CPU_INT_IMASK_SSTART_SET | I2C_CPU_INT_IMASK_SSTOP_SET |                              \
	 I2C_CPU_INT_IMASK_TIMEOUTA_SET)

enum i2c_mspm0_state {
	I2C_MSPM0_IDLE,
	I2C_MSPM0_TX_STARTED,
	I2C_MSPM0_TX_INPROGRESS,
	I2C_MSPM0_TX_COMPLETE,
	I2C_MSPM0_RX_STARTED,
	I2C_MSPM0_RX_INPROGRESS,
	I2C_MSPM0_RX_COMPLETE,
	I2C_MSPM0_TARGET_STARTED,
	I2C_MSPM0_TARGET_TX_INPROGRESS,
	I2C_MSPM0_TARGET_RX_INPROGRESS,
	I2C_MSPM0_TARGET_PREEMPTED,
	I2C_MSPM0_TIMEOUT,
	I2C_MSPM0_ERROR,
};

struct i2c_mspm0_config {
	mm_reg_t base;
	uint32_t bitrate;
	uint32_t merge_buf_size;
	uint8_t *merge_buf;
	i2c_clock_config_t clock_config;
	struct mspm0_sys_clock *clock_subsys;
	const struct pinctrl_dev_config *pinctrl;
	void (*irq_config_func)(const struct device *dev);
	uint32_t controller_tx_fifo_threshold; /* pre-encoded TXTRIG field value */
	uint32_t controller_rx_fifo_threshold; /* pre-encoded RXTRIG field value */
};

/*
 * Ready-to-dispatch message entry built by i2c_mspm0_transfer() ahead of time.
 * The ISR consumes these without doing any memcpy or message merging.
 */
struct i2c_mspm0_dispatch_msg {
	uint8_t *buf;  /* buffer pointer (user buf or merge_buf slice) */
	uint16_t len;  /* total byte count for this dispatch entry     */
	uint8_t flags; /* I2C_MSG_* flags of the last msg in the group */
};

struct i2c_mspm0_data {
	uint32_t dev_config;
	volatile enum i2c_mspm0_state state;
	struct k_sem i2c_lock;
	struct k_sem device_sync_sem;
	uint32_t transfer_count;
	uint32_t transfer_len;
	uint8_t *msg_buf;

	/* Pre-built dispatch messages array, consumed by ISR */
	struct i2c_mspm0_dispatch_msg dispatch[CONFIG_MSPM0_I2C_MAX_DISPATCH_MSGS];
	uint8_t dispatch_count;
	uint8_t dispatch_idx;

	uint16_t addr;
	int transfer_ret;

#ifdef CONFIG_I2C_TARGET
	struct i2c_target_config *target_config;
	const struct i2c_target_callbacks *target_callbacks;
#endif /* CONFIG_I2C_TARGET */
	bool is_target;
};

/* Power management helpers */
static inline void i2c_enable_power(mm_reg_t base)
{
	sys_write32(I2C_PWREN_KEY_UNLOCK_W | I2C_PWREN_ENABLE_ENABLE, base + I2C_REG_PWREN);
}

static inline void i2c_disable_power(mm_reg_t base)
{
	sys_write32(I2C_PWREN_KEY_UNLOCK_W | I2C_PWREN_ENABLE_DISABLE, base + I2C_REG_PWREN);
}

static inline void i2c_reset(mm_reg_t base)
{
	sys_write32(I2C_RSTCTL_KEY_UNLOCK_W | I2C_RSTCTL_RESETSTKYCLR_CLR |
			    I2C_RSTCTL_RESETASSERT_ASSERT,
		    base + I2C_REG_RSTCTL);
}

/* Clock configuration helper */
static inline void i2c_set_clock_config(mm_reg_t base, const i2c_clock_config_t *config)
{
	I2C_UPDATE_REG(base, I2C_REG_CLKSEL, config->clock_sel,
		       I2C_CLKSEL_BUSCLK_SEL_MASK | I2C_CLKSEL_MFCLK_SEL_MASK);
	I2C_UPDATE_REG(base, I2C_REG_CLKDIV, config->divide_ratio, I2C_CLKDIV_RATIO_MASK);
}

/* Glitch filter helpers */
static inline void i2c_disable_analog_glitch_filter(mm_reg_t base)
{
	I2C_UPDATE_REG(base, I2C_REG_GFCTL, 0, I2C_GFCTL_AGFEN_MASK);
}

/* Controller control helpers */
static inline void i2c_reset_controller_transfer(mm_reg_t base)
{
	sys_write32(0x00, base + I2C_REG_MCTR);
}

static inline void i2c_enable_controller(mm_reg_t base)
{
	I2C_UPDATE_REG(base, I2C_REG_MCR, I2C_MCR_ACTIVE_ENABLE, I2C_MCR_ACTIVE_MASK);
}

static inline void i2c_disable_controller(mm_reg_t base)
{
	I2C_UPDATE_REG(base, I2C_REG_MCR, 0, I2C_MCR_ACTIVE_MASK);
}

static inline void i2c_enable_controller_clock_stretching(mm_reg_t base)
{
	I2C_UPDATE_REG(base, I2C_REG_MCR, I2C_MCR_CLKSTRETCH_ENABLE, I2C_MCR_CLKSTRETCH_ENABLE);
}

static inline void i2c_set_timer_period(mm_reg_t base, uint8_t period)
{
	sys_write32(period, base + I2C_REG_MTPR);
}

static inline uint32_t i2c_get_controller_status(mm_reg_t base)
{
	return sys_read32(base + I2C_REG_MSR);
}

/* FIFO control helpers */
static inline void i2c_set_controller_tx_fifo_threshold(mm_reg_t base, uint32_t level)
{
	I2C_UPDATE_REG(base, I2C_REG_MFIFOCTL, level, I2C_MFIFOCTL_TXTRIG_MASK);
}

static inline void i2c_set_controller_rx_fifo_threshold(mm_reg_t base, uint32_t level)
{
	I2C_UPDATE_REG(base, I2C_REG_MFIFOCTL, level, I2C_MFIFOCTL_RXTRIG_MASK);
}

static inline void i2c_start_flush_controller_tx_fifo(mm_reg_t base)
{
	I2C_UPDATE_REG(base, I2C_REG_MFIFOCTL, I2C_MFIFOCTL_TXFLUSH_MASK,
		       I2C_MFIFOCTL_TXFLUSH_MASK);
}

static inline void i2c_stop_flush_controller_tx_fifo(mm_reg_t base)
{
	I2C_UPDATE_REG(base, I2C_REG_MFIFOCTL, 0, I2C_MFIFOCTL_TXFLUSH_MASK);
}

static inline bool i2c_is_controller_tx_fifo_empty(mm_reg_t base)
{
	return ((sys_read32(base + I2C_REG_MFIFOSR) & I2C_MFIFOSR_TXFIFOCNT_MASK) ==
		FIELD_PREP(I2C_MFIFOSR_TXFIFOCNT_MASK, I2C_FIFO_DEPTH));
}

static inline bool i2c_is_controller_tx_fifo_full(mm_reg_t base)
{
	return ((sys_read32(base + I2C_REG_MFIFOSR) & I2C_MFIFOSR_TXFIFOCNT_MASK) ==
		I2C_MFIFOSR_TXFIFOCNT_MINIMUM);
}

static inline bool i2c_is_controller_rx_fifo_empty(mm_reg_t base)
{
	return ((sys_read32(base + I2C_REG_MFIFOSR) & I2C_MFIFOSR_RXFIFOCNT_MASK) ==
		I2C_MFIFOSR_RXFIFOCNT_MINIMUM);
}

static inline void i2c_transmit_controller_data(mm_reg_t base, uint8_t data)
{
	sys_write32(data, base + I2C_REG_MTXDATA);
}

static inline uint8_t i2c_receive_controller_data(mm_reg_t base)
{
	return (uint8_t)(sys_read32(base + I2C_REG_MRXDATA) & I2C_MRXDATA_VALUE_MASK);
}

/* Multi-byte FIFO operations */
static uint16_t i2c_fill_controller_tx_fifo(mm_reg_t base, const uint8_t *buffer, uint16_t count)
{
	uint16_t i;

	for (i = 0; i < count; i++) {
		if (!i2c_is_controller_tx_fifo_full(base)) {
			i2c_transmit_controller_data(base, buffer[i]);
		} else {
			break;
		}
	}
	return i;
}

static void i2c_flush_controller_tx_fifo(mm_reg_t base)
{
	i2c_start_flush_controller_tx_fifo(base);
	while (!i2c_is_controller_tx_fifo_empty(base)) {
		/* Wait for FIFO to empty */
	}
	i2c_stop_flush_controller_tx_fifo(base);
}

/* Controller transfer control */
static inline void i2c_start_controller_transfer_advanced(mm_reg_t base, uint32_t targetAddr,
							  uint32_t direction, uint16_t length,
							  uint32_t start, uint32_t stop,
							  uint32_t ack, uint32_t addr_mode)
{
	/* Set target address, direction, and addressing mode (7 or 10-bit) */
	I2C_UPDATE_REG(base, I2C_REG_MSA,
		       (FIELD_PREP(I2C_MSA_SADDR_MASK, targetAddr) | direction | addr_mode),
		       (I2C_MSA_SADDR_MASK | I2C_MSA_DIR_MASK | I2C_MSA_MMODE_MASK));

	/* Set control register */
	I2C_UPDATE_REG(base, I2C_REG_MCTR,
		       (FIELD_PREP(I2C_MCTR_MBLEN_MASK, length) | I2C_MCTR_BURSTRUN_ENABLE | start |
			stop | ack),
		       (I2C_MCTR_MBLEN_MASK | I2C_MCTR_BURSTRUN_MASK | I2C_MCTR_START_MASK |
			I2C_MCTR_STOP_MASK | I2C_MCTR_ACK_MASK));
}

/* Timeout helpers (if CONFIG_MSPM0_I2C_SCL_LOW_TIMEOUT_MS != 0) */
static inline void i2c_enable_timeout_a(mm_reg_t base)
{
	I2C_UPDATE_REG(base, I2C_REG_TIMEOUT_CTL, I2C_TIMEOUT_CTL_TCNTAEN_ENABLE,
		       I2C_TIMEOUT_CTL_TCNTAEN_ENABLE);
}

static inline void i2c_set_timeout_a_count(mm_reg_t base, uint32_t count)
{
	I2C_UPDATE_REG(base, I2C_REG_TIMEOUT_CTL, count, I2C_TIMEOUT_CTL_TCNTLA_MASK);
}

/* Interrupt helpers */
static inline void i2c_enable_interrupt(mm_reg_t base, uint32_t interruptMask)
{
	I2C_UPDATE_REG(base, I2C_REG_CPU_INT_IMASK, interruptMask, interruptMask);
}

static inline void i2c_disable_interrupt(mm_reg_t base, uint32_t interruptMask)
{
	I2C_UPDATE_REG(base, I2C_REG_CPU_INT_IMASK, 0, interruptMask);
}

static inline void i2c_clear_interrupt_status(mm_reg_t base, uint32_t interruptMask)
{
	sys_write32(interruptMask, base + I2C_REG_CPU_INT_ICLR);
}

static inline uint32_t i2c_get_enabled_interrupt_status(mm_reg_t base, uint32_t interruptMask)
{
	return (sys_read32(base + I2C_REG_CPU_INT_MIS) & interruptMask);
}

static inline uint32_t i2c_get_pending_interrupt(mm_reg_t base)
{
	return sys_read32(base + I2C_REG_CPU_INT_IIDX);
}

/* Workaround for errata I2C_ERR_04: always disable target wakeup on init */
static inline void i2c_disable_target_wakeup(mm_reg_t base)
{
	I2C_UPDATE_REG(base, I2C_REG_SCTR, 0, I2C_SCTR_SWUEN_MASK);
}

/* Target mode helpers (CONFIG_I2C_TARGET) */
#ifdef CONFIG_I2C_TARGET
static inline void i2c_enable_target(mm_reg_t base)
{
	I2C_UPDATE_REG(base, I2C_REG_SCTR, I2C_SCTR_ACTIVE_ENABLE, I2C_SCTR_ACTIVE_MASK);
}

static inline void i2c_disable_target(mm_reg_t base)
{
	I2C_UPDATE_REG(base, I2C_REG_SCTR, 0, I2C_SCTR_ACTIVE_MASK);
}

static inline void i2c_set_target_own_address(mm_reg_t base, uint32_t addr, uint32_t addr_mode)
{
	I2C_UPDATE_REG(base, I2C_REG_SOAR, addr | addr_mode,
		       I2C_SOAR_OAR_MASK | I2C_SOAR_SMODE_MASK);
}

static inline void i2c_enable_target_clock_stretching(mm_reg_t base)
{
	I2C_UPDATE_REG(base, I2C_REG_SCTR, I2C_SCTR_SCLKSTRETCH_ENABLE,
		       I2C_SCTR_SCLKSTRETCH_ENABLE);
}

static inline void i2c_enable_target_tx_trigger_in_tx_mode(mm_reg_t base)
{
	I2C_UPDATE_REG(base, I2C_REG_SCTR, I2C_SCTR_TXTRIG_TXMODE_ENABLE,
		       I2C_SCTR_TXTRIG_TXMODE_ENABLE);
}

static inline void i2c_enable_target_tx_empty_on_tx_request(mm_reg_t base)
{
	I2C_UPDATE_REG(base, I2C_REG_SCTR, I2C_SCTR_TXEMPTY_ON_TREQ_ENABLE,
		       I2C_SCTR_TXEMPTY_ON_TREQ_ENABLE);
}

static inline void i2c_set_target_tx_fifo_threshold(mm_reg_t base, uint32_t level)
{
	I2C_UPDATE_REG(base, I2C_REG_SFIFOCTL, level, I2C_SFIFOCTL_TXTRIG_MASK);
}

static inline void i2c_set_target_rx_fifo_threshold(mm_reg_t base, uint32_t level)
{
	I2C_UPDATE_REG(base, I2C_REG_SFIFOCTL, level, I2C_SFIFOCTL_RXTRIG_MASK);
}

static inline void i2c_flush_target_tx_fifo(mm_reg_t base)
{
	I2C_UPDATE_REG(base, I2C_REG_SFIFOCTL, I2C_SFIFOCTL_TXFLUSH_MASK,
		       I2C_SFIFOCTL_TXFLUSH_MASK);
	while ((sys_read32(base + I2C_REG_SFIFOSR) & I2C_SFIFOSR_TXFIFOCNT_MASK) !=
	       FIELD_PREP(I2C_SFIFOSR_TXFIFOCNT_MASK, I2C_FIFO_DEPTH)) {
		/* Wait */
	}
	I2C_UPDATE_REG(base, I2C_REG_SFIFOCTL, 0, I2C_SFIFOCTL_TXFLUSH_MASK);
}

static inline bool i2c_is_target_rx_fifo_empty(mm_reg_t base)
{
	return ((sys_read32(base + I2C_REG_SFIFOSR) & I2C_SFIFOSR_RXFIFOCNT_MASK) ==
		I2C_SFIFOSR_RXFIFOCNT_MINIMUM);
}

static inline uint8_t i2c_receive_target_data(mm_reg_t base)
{
	return (uint8_t)(sys_read32(base + I2C_REG_SRXDATA) & I2C_SRXDATA_VALUE_MASK);
}

static inline void i2c_transmit_target_data(mm_reg_t base, uint8_t data)
{
	sys_write32(data, base + I2C_REG_STXDATA);
}

static inline bool i2c_is_target_tx_fifo_full(mm_reg_t base)
{
	return ((sys_read32(base + I2C_REG_SFIFOSR) & I2C_SFIFOSR_TXFIFOCNT_MASK) ==
		I2C_SFIFOSR_TXFIFOCNT_MINIMUM);
}

static inline bool i2c_transmit_target_data_check(mm_reg_t base, uint8_t data)
{
	if (i2c_is_target_tx_fifo_full(base)) {
		return false;
	}

	i2c_transmit_target_data(base, data);
	return true;
}

static inline void i2c_set_target_ack_override_value(mm_reg_t base, uint32_t value)
{
	I2C_UPDATE_REG(base, I2C_REG_SACKCTL, value, I2C_SACKCTL_ACKOVAL_MASK);
}
#endif /* CONFIG_I2C_TARGET */

#if CONFIG_MSPM0_I2C_SCL_LOW_TIMEOUT_MS != 0
static int i2c_mspm0_configure_timeout(const struct device *dev, uint32_t period,
				       uint32_t timeout_ms)
{
	const struct i2c_mspm0_config *config = dev->config;
	const mm_reg_t base = config->base;
	const struct device *clk_dev = DEVICE_DT_GET(DT_NODELABEL(ckm));
	uint32_t clock_rate;
	uint32_t tick_cycles;
	uint64_t timeout_cycles;
	uint32_t ticks_needed;
	uint16_t counter_value;
	int ret;

	ret = clock_control_get_rate(clk_dev, config->clock_subsys, &clock_rate);
	if (ret < 0) {
		return ret;
	}

	/* Each count is equal to (1 + TPR) * 12 functional clocks */
	tick_cycles = (period + 1) * 12;
	timeout_cycles = (uint64_t)timeout_ms * (clock_rate / 1000);
	ticks_needed = (timeout_cycles + tick_cycles - 1) / tick_cycles;
	/* Lower 4-bits of counter are automatically set to 0x0 */
	counter_value = ticks_needed >> 4;

	if (counter_value > 0xFF) {
		LOG_WRN("SCL low timeout (%u ms) too large for clock rate; "
			"clamping to maximum hardware value",
			timeout_ms);
		counter_value = 0xFF;
	}

	i2c_enable_timeout_a(base);
	i2c_set_timeout_a_count(base, counter_value);
	return 0;
}
#endif /* CONFIG_MSPM0_I2C_SCL_LOW_TIMEOUT_MS != 0 */

static int i2c_mspm0_configure(const struct device *dev, uint32_t dev_config)
{
	const struct i2c_mspm0_config *config = dev->config;
	struct i2c_mspm0_data *data = dev->data;
	const mm_reg_t base = config->base;
	const struct device *const clk_dev = DEVICE_DT_GET(DT_NODELABEL(ckm));
	uint32_t clock_rate;
	uint32_t desired_speed;
	int32_t tpr_val;
	int ret;

	ret = clock_control_get_rate(clk_dev, config->clock_subsys, &clock_rate);
	if (ret < 0) {
		return -ENODEV;
	}

	k_sem_take(&data->i2c_lock, K_FOREVER);

	/* Config I2C speed */
	switch (I2C_SPEED_GET(dev_config)) {
	case I2C_SPEED_STANDARD:
		desired_speed = 100000;
		break;
	case I2C_SPEED_FAST:
		desired_speed = 400000;
		break;
	case I2C_SPEED_FAST_PLUS:
		desired_speed = 1000000;
		break;
	default:
		ret = -EINVAL;
		goto sem_give;
	}

	/* Calculate the timer period based on the desired period and clock rate.
	 *
	 * Per TRM, the value stored in the Timer Period (TPR_Val) creates a
	 * period from the following equation:
	 * Desired Period = (1 + TPR_Val) X (10) X clock_rate
	 *
	 * Solving the above equation for TPR_VAL given a desired period yields
	 * the following:
	 *
	 * TPR_val = ceiling(clock_rate/(desired_speed * 10)) - 1
	 *
	 * The ceiling is chosen such that a non-clean divide gives a period
	 * greater than the desired rather than lower. This means that the
	 * frequency will thus be lower, and the frequency bounds are considered
	 * preferred.
	 */
	tpr_val = DIV_ROUND_UP(clock_rate, (desired_speed * 10)) - 1;

	/* Set the I2C speed */
	if (tpr_val <= 0) {
		ret = -EINVAL;
		goto sem_give;
	}

	i2c_set_timer_period(base, tpr_val);

#if CONFIG_MSPM0_I2C_SCL_LOW_TIMEOUT_MS != 0
	ret = i2c_mspm0_configure_timeout(dev, tpr_val, CONFIG_MSPM0_I2C_SCL_LOW_TIMEOUT_MS);
	if (ret < 0) {
		goto sem_give;
	}
#endif /* CONFIG_MSPM0_I2C_SCL_LOW_TIMEOUT_MS */

	/* Config other settings */
	i2c_set_controller_tx_fifo_threshold(base, config->controller_tx_fifo_threshold);
	i2c_set_controller_rx_fifo_threshold(base, config->controller_rx_fifo_threshold);
	i2c_enable_controller_clock_stretching(base);

	/* Configure Interrupts */
	i2c_enable_interrupt(base, TI_MSPM0_CONTROLLER_INTERRUPTS);

	/* Enable module */
	i2c_enable_controller(base);
	data->dev_config = dev_config;

sem_give:
	k_sem_give(&data->i2c_lock);
	return ret;
}

static int i2c_mspm0_init(const struct device *dev)
{
	const struct i2c_mspm0_config *config = dev->config;
	struct i2c_mspm0_data *data = dev->data;
	const mm_reg_t base = config->base;
	int ret;
	uint32_t speed_config;

	/* initialize semaphores */
	k_sem_init(&data->i2c_lock, 1, 1);
	k_sem_init(&data->device_sync_sem, 0, 1);

	/* Init power */
	i2c_reset(base);
	i2c_enable_power(base);

	/* Wait for peripheral power to stabilize (POST_KERNEL, so k_busy_wait is available) */
	k_busy_wait(1);
	i2c_reset_controller_transfer(base);
#ifdef CONFIG_I2C_TARGET
	/* Workaround for errata I2C_ERR_04 */
	i2c_disable_target_wakeup(base);
#endif
	/* Init GPIO */
	ret = pinctrl_apply_state(config->pinctrl, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	/* Config clocks and analog filter */
	i2c_set_clock_config(base, &config->clock_config);
	i2c_disable_analog_glitch_filter(base);

	/* Set frequency */
	speed_config = i2c_map_dt_bitrate(config->bitrate);
	ret = i2c_mspm0_configure(dev, speed_config);
	if (ret < 0) {
		return ret;
	}

	/* Enable interrupts */
	config->irq_config_func(dev);

	return 0;
}

static int i2c_mspm0_get_config(const struct device *dev, uint32_t *dev_config)
{
	struct i2c_mspm0_data *data = dev->data;
	*dev_config = data->dev_config;

	return 0;
}

static int i2c_mspm0_reset_controller(const struct device *dev)
{
	const struct i2c_mspm0_config *config = dev->config;
	const mm_reg_t base = config->base;

	i2c_reset(base);
	i2c_disable_power(base);

	i2c_enable_power(base);
	/* Wait for peripheral power to stabilize (POST_KERNEL, so k_busy_wait is available) */
	k_busy_wait(1);

#ifdef CONFIG_I2C_TARGET
	/* Workaround for errata I2C_ERR_04 */
	i2c_disable_target_wakeup(base);
#endif

	/* Config clocks and analog filter */
	i2c_set_clock_config(base, &config->clock_config);
	i2c_disable_analog_glitch_filter(base);

	/* Configure Controller Mode */
	i2c_reset_controller_transfer(base);

	/* Config other settings */
	i2c_set_controller_tx_fifo_threshold(base, config->controller_tx_fifo_threshold);
	i2c_set_controller_rx_fifo_threshold(base, config->controller_rx_fifo_threshold);
	i2c_enable_controller_clock_stretching(base);

	/* Configure Interrupts */
	i2c_clear_interrupt_status(base, TI_MSPM0_CONTROLLER_INTERRUPTS);
	i2c_enable_interrupt(base, TI_MSPM0_CONTROLLER_INTERRUPTS);

	/* Enable module */
	i2c_enable_controller(base);

	return 0;
}

/*
 * Returns true if message i and message i+1 can be merged into a single
 * transfer (same direction, no STOP or RESTART on the next message).
 */
static bool i2c_mspm0_is_merge_next(const struct i2c_msg *msgs, const uint8_t num_msgs, int i)
{
	if (i + 1 >= num_msgs) {
		return false;
	}

	bool flag_allow = !(msgs[i].flags & I2C_MSG_STOP) && !(msgs[i + 1].flags & I2C_MSG_RESTART);
	bool same_dir = ((msgs[i].flags & I2C_MSG_READ) == (msgs[i + 1].flags & I2C_MSG_READ));

	return flag_allow && same_dir;
}

/* Start transfer for dispatch[dispatch_idx] */
static void i2c_mspm0_start_dispatch(const struct device *dev)
{
	const struct i2c_mspm0_config *config = dev->config;
	struct i2c_mspm0_data *data = dev->data;
	const mm_reg_t base = config->base;
	const struct i2c_mspm0_dispatch_msg *d = &data->dispatch[data->dispatch_idx];
	uint32_t addr_mode =
		(d->flags & I2C_MSG_ADDR_10_BITS) ? I2C_MSA_MMODE_MODE10 : I2C_MSA_MMODE_MODE7;
	uint32_t stop = (d->flags & I2C_MSG_STOP) ? I2C_CONTROLLER_STOP_ENABLE
						  : I2C_CONTROLLER_STOP_DISABLE;

	data->msg_buf = d->buf;
	data->transfer_count = 0;
	data->transfer_len = d->len;

	if ((d->flags & I2C_MSG_RW_MASK) == I2C_MSG_READ) {
		data->state = I2C_MSPM0_RX_STARTED;
		i2c_start_controller_transfer_advanced(
			base, data->addr, I2C_CONTROLLER_DIRECTION_RX, d->len,
			I2C_CONTROLLER_START_ENABLE, stop, I2C_CONTROLLER_ACK_DISABLE, addr_mode);
	} else {
		data->state = I2C_MSPM0_IDLE;
		i2c_flush_controller_tx_fifo(base);
		data->transfer_count = i2c_fill_controller_tx_fifo(base, d->buf, d->len);

		if (data->transfer_count < data->transfer_len) {
			i2c_enable_interrupt(base, I2C_CPU_INT_IMASK_MTXFIFOTRG_SET);
		} else {
			i2c_disable_interrupt(base, I2C_CPU_INT_IMASK_MTXFIFOTRG_SET);
		}

		data->state = I2C_MSPM0_TX_STARTED;
		i2c_start_controller_transfer_advanced(
			base, data->addr, I2C_CONTROLLER_DIRECTION_TX, d->len,
			I2C_CONTROLLER_START_ENABLE, stop, I2C_CONTROLLER_ACK_ENABLE, addr_mode);
	}
}

static int i2c_mspm0_transfer(const struct device *dev, struct i2c_msg *msgs, uint8_t num_msgs,
			      uint16_t addr)
{
	const struct i2c_mspm0_config *config = dev->config;
	struct i2c_mspm0_data *data = dev->data;
	uint8_t *merge_buf = config->merge_buf;
	uint16_t merge_buf_size = config->merge_buf_size;
	uint16_t merge_buf_used = 0;
	uint32_t dev_config;
	int ret = 0;

	k_sem_take(&data->i2c_lock, K_FOREVER);

	/* Pre-process all messages into the dispatch buffer */
	data->dispatch_count = 0;
	data->dispatch_idx = 0;
	data->addr = addr;
	data->transfer_ret = 0;

	int i = 0;

	while (i < num_msgs) {
		int group_start = i;
		uint16_t group_len = 0;
		bool use_merge = false;

		/* Accumulate a merge group starting at index i */
		while (i < num_msgs) {
			group_len += msgs[i].len;
			bool merge_with_next = i2c_mspm0_is_merge_next(msgs, num_msgs, i);

			i++;
			if (!merge_with_next) {
				break; /* end of this merge group */
			}
			use_merge = true; /* at least two messages in this group */
		}

		if (data->dispatch_count >= CONFIG_MSPM0_I2C_MAX_DISPATCH_MSGS) {
			LOG_ERR("i2c_mspm0: too many dispatch entries (max %d)",
				CONFIG_MSPM0_I2C_MAX_DISPATCH_MSGS);
			ret = -ENOMEM;
			goto unlock;
		}

		if (use_merge) {
			if ((merge_buf_used + group_len) > merge_buf_size) {
				LOG_ERR("i2c_mspm0: merge buffer too small "
					"(%u + %u > %u)",
					merge_buf_used, group_len, merge_buf_size);
				ret = -ENOSPC;
				goto unlock;
			}

			/* For TX groups: copy all message payloads now */
			if (!(msgs[group_start].flags & I2C_MSG_READ)) {
				uint16_t off = merge_buf_used;

				for (int j = group_start; j < i; j++) {
					memcpy(merge_buf + off, msgs[j].buf, msgs[j].len);
					off += msgs[j].len;
				}
			}

			/* RX groups: the ISR will write into merge_buf + merge_buf_used */
			data->dispatch[data->dispatch_count].buf = merge_buf + merge_buf_used;
			data->dispatch[data->dispatch_count].len = group_len;
			data->dispatch[data->dispatch_count].flags = msgs[i - 1].flags;
			merge_buf_used += group_len;
		} else {
			/* Single message: use the caller's buffer directly */
			data->dispatch[data->dispatch_count].buf = msgs[group_start].buf;
			data->dispatch[data->dispatch_count].len = msgs[group_start].len;
			data->dispatch[data->dispatch_count].flags = msgs[group_start].flags;
		}

		data->dispatch_count++;
	}

	/* Send the first dispatch entry */
	i2c_mspm0_start_dispatch(dev);

	/*
	 * Block once for the entire multi-message transfer. The ISR transmits
	 * all dispatch[..] messages and gives device_sync_sem.
	 */
	if (k_sem_take(&data->device_sync_sem, K_MSEC(CONFIG_I2C_TRANSFER_TIMEOUT_MS))) {
		data->transfer_ret = -ETIMEDOUT;
	}

	ret = data->transfer_ret;

	/* Scatter merged RX data back to user buffers */
	if (ret == 0) {
		merge_buf_used = 0;
		i = 0;

		for (int d = 0; d < data->dispatch_count; d++) {
			int group_start = i;

			/* Re-identify the group boundaries */
			while (i < num_msgs) {
				bool merged = i2c_mspm0_is_merge_next(msgs, num_msgs, i);

				i++;
				if (!merged) {
					break;
				}
			}

			/* Scatter back only for merged RX groups */
			if ((data->dispatch[d].flags & I2C_MSG_RW_MASK) == I2C_MSG_READ &&
			    data->dispatch[d].buf == merge_buf + merge_buf_used) {
				uint16_t off = merge_buf_used;

				for (int j = group_start; j < i; j++) {
					memcpy(msgs[j].buf, merge_buf + off, msgs[j].len);
					off += msgs[j].len;
				}
			}

			merge_buf_used += data->dispatch[d].len;
		}
	}

	/* Timeout recovery */
	if (ret == -ETIMEDOUT) {
		/*
		 * The ISR may or may not have already released i2c_lock via the
		 * TIMEOUTA/MSTOPFG path. Give it unconditionally here before
		 * calling reset_controller() and configure(), both of which take
		 * i2c_lock themselves. The semaphore is initialised with max=1
		 * so an extra give is harmless if the ISR already released it.
		 */
		k_sem_give(&data->i2c_lock);
		i2c_mspm0_get_config(dev, &dev_config);
		i2c_mspm0_reset_controller(dev);
		i2c_mspm0_configure(dev, dev_config);
	}

	return ret;

unlock:
	k_sem_give(&data->i2c_lock);
	return ret;
}

#ifdef CONFIG_I2C_TARGET
static int i2c_mspm0_target_register(const struct device *dev, struct i2c_target_config *cfg)
{
	const struct i2c_mspm0_config *config = dev->config;
	struct i2c_mspm0_data *data = dev->data;
	const mm_reg_t base = config->base;

	/* Device is already registered as target */
	if (data->is_target || data->target_config == cfg) {
		return -EBUSY;
	}

	k_sem_take(&data->i2c_lock, K_FOREVER);

	data->target_config = cfg;
	data->target_callbacks = cfg->callbacks;

	if (data->state == I2C_MSPM0_TARGET_PREEMPTED) {
		i2c_clear_interrupt_status(base, TI_MSPM0_TARGET_INTERRUPTS);
	}

	/*
	 * The controller can remain enabled for dual-role operation
	 * (simultaneous controller and target on the same bus). We keep the
	 * controller and its interrupts active so that controller transfers
	 * can still be initiated while the target is registered.
	 */
	i2c_set_target_tx_fifo_threshold(base, I2C_TX_FIFO_LEVEL_BYTES_1);
	i2c_set_target_rx_fifo_threshold(base, I2C_RX_FIFO_LEVEL_BYTES_1);
	i2c_enable_target_tx_trigger_in_tx_mode(base);
	i2c_enable_target_tx_empty_on_tx_request(base);
	i2c_enable_target_clock_stretching(base);
	i2c_set_target_own_address(base, data->target_config->address,
				   (data->target_config->flags & I2C_TARGET_FLAGS_ADDR_10_BITS)
					   ? I2C_SOAR_SMODE_MODE10
					   : I2C_SOAR_SMODE_MODE7);

	i2c_clear_interrupt_status(base, I2C_CPU_INT_IMASK_STXEMPTY_SET);
	i2c_enable_interrupt(base, TI_MSPM0_TARGET_INTERRUPTS);
	i2c_enable_target(base);

	data->is_target = true;
	data->state = I2C_MSPM0_IDLE;

	k_sem_give(&data->i2c_lock);
	return 0;
}

static int i2c_mspm0_target_unregister(const struct device *dev, struct i2c_target_config *cfg)
{
	const struct i2c_mspm0_config *config = dev->config;
	struct i2c_mspm0_data *data = dev->data;
	const mm_reg_t base = config->base;

	if (data->is_target == false) {
		return 0;
	}

	k_sem_take(&data->i2c_lock, K_FOREVER);

	data->target_config = NULL;
	data->is_target = false;

	i2c_disable_target(base);
	i2c_disable_interrupt(base, TI_MSPM0_TARGET_INTERRUPTS);

	k_sem_give(&data->i2c_lock);
	return 0;
}

static int i2c_mspm0_reset_target(const struct device *dev)
{
	const struct i2c_mspm0_config *config = dev->config;
	struct i2c_mspm0_data *data = dev->data;
	const mm_reg_t base = config->base;

	i2c_reset(base);
	i2c_disable_power(base);

	i2c_enable_power(base);
	/* Wait for peripheral power to stabilize (POST_KERNEL, so k_busy_wait is available) */
	k_busy_wait(1);

	i2c_disable_target_wakeup(base);

	/* Config clocks and analog filter */
	i2c_set_clock_config(base, &config->clock_config);
	i2c_disable_analog_glitch_filter(base);

	i2c_set_target_own_address(base, data->target_config->address,
				   (data->target_config->flags & I2C_TARGET_FLAGS_ADDR_10_BITS)
					   ? I2C_SOAR_SMODE_MODE10
					   : I2C_SOAR_SMODE_MODE7);
	i2c_set_target_tx_fifo_threshold(base, I2C_TX_FIFO_LEVEL_BYTES_1);
	i2c_set_target_rx_fifo_threshold(base, I2C_RX_FIFO_LEVEL_BYTES_1);
	i2c_enable_target_tx_trigger_in_tx_mode(base);
	i2c_enable_target_tx_empty_on_tx_request(base);

	i2c_clear_interrupt_status(base, I2C_CPU_INT_IMASK_STXEMPTY_SET);

	i2c_enable_interrupt(base, TI_MSPM0_TARGET_INTERRUPTS);

	data->state = I2C_MSPM0_IDLE;

	/* Enable module */
	i2c_enable_target(base);

	return 0;
}

static void i2c_mspm0_isr_target(const struct device *dev)
{
	const struct i2c_mspm0_config *config = dev->config;
	struct i2c_mspm0_data *data = dev->data;
	const mm_reg_t base = config->base;
	int ret;
	uint8_t tx_byte;
	uint8_t rx_byte;

	switch (i2c_get_pending_interrupt(base)) {
	case I2C_CPU_INT_IIDX_STAT_SSTARTFG:
		data->state = I2C_MSPM0_TARGET_STARTED;
		/* Flush TX FIFO to clear out any stale data */
		i2c_flush_target_tx_fifo(base);
		break;
	case I2C_CPU_INT_IIDX_STAT_SRXDONEFG:
		if (data->state == I2C_MSPM0_TARGET_STARTED) {
			data->state = I2C_MSPM0_TARGET_RX_INPROGRESS;
			if (data->target_callbacks->write_requested != NULL) {
				ret = data->target_callbacks->write_requested(data->target_config);
				if (ret == 0) {
					i2c_set_target_ack_override_value(
						base, I2C_TARGET_RESPONSE_OVERRIDE_VALUE_ACK);
				} else {
					i2c_set_target_ack_override_value(
						base, I2C_TARGET_RESPONSE_OVERRIDE_VALUE_NACK);
				}
			}
		}
		/* Store received data in buffer */
		if (data->target_callbacks->write_received != NULL) {
			while (i2c_is_target_rx_fifo_empty(base) != true) {
				rx_byte = i2c_receive_target_data(base);
				ret = data->target_callbacks->write_received(data->target_config,
									     rx_byte);
				if (ret == 0) {
					i2c_set_target_ack_override_value(
						base, I2C_TARGET_RESPONSE_OVERRIDE_VALUE_ACK);
				} else {
					i2c_set_target_ack_override_value(
						base, I2C_TARGET_RESPONSE_OVERRIDE_VALUE_NACK);
				}
			}
		} else {
			i2c_receive_target_data(base);
			i2c_set_target_ack_override_value(base,
							  I2C_TARGET_RESPONSE_OVERRIDE_VALUE_NACK);
		}
		break;
	case I2C_CPU_INT_IIDX_STAT_STXEMPTY:
		if (data->state == I2C_MSPM0_TARGET_STARTED) {
			/* First byte detected from a read request */
			data->state = I2C_MSPM0_TARGET_TX_INPROGRESS;
			if (data->target_callbacks->read_requested != NULL) {
				ret = data->target_callbacks->read_requested(data->target_config,
									     &tx_byte);
				if (ret == 0) {
					i2c_transmit_target_data(base, tx_byte);
				} else {
					/* In this case, no new data is desired to be filled, thus
					 * 0's are transmitted
					 */
					i2c_transmit_target_data(base, 0x00);
				}
			} else {
				/* read_requested function is not found. The target data will
				 * continue to transmit to fulfill the error and not hang
				 * the controller by stretching indefinitely
				 */
				i2c_transmit_target_data_check(base, 0xFF);
			}
		} else {
			/* still using the FIFO, we call read_processed in order to add
			 * additional data rather than from a buffer. If the write-received
			 * function chooses to return 0 (no more data present), then 0's will
			 * be filled in
			 */
			if (data->target_callbacks->read_processed != NULL) {
				ret = data->target_callbacks->read_processed(data->target_config,
									     &tx_byte);

				if (ret == 0) {
					i2c_transmit_target_data(base, tx_byte);
				} else {
					/* In this case, no new data is desired to be filled, thus
					 * 0's are transmitted
					 */
					i2c_transmit_target_data(base, 0x00);
				}
			} else {
				/* Read_processed function is not found. The target data will
				 * continue to transmit to fulfill the error and not hang
				 * the controller by stretching indefinitely
				 */
				i2c_transmit_target_data_check(base, 0xFF);
			}
		}
		break;
	case I2C_CPU_INT_IIDX_STAT_SSTOPFG:
		data->state = I2C_MSPM0_IDLE;
		if (data->target_callbacks->stop) {
			data->target_callbacks->stop(data->target_config);
		}
		break;
	case I2C_CPU_INT_IIDX_STAT_TIMEOUTA:
		i2c_disable_interrupt(base, TI_MSPM0_TARGET_INTERRUPTS);
		i2c_clear_interrupt_status(base, TI_MSPM0_TARGET_INTERRUPTS);
		if (data->target_callbacks->error) {
			data->target_callbacks->error(data->target_config, I2C_ERROR_TIMEOUT);
		}
		if (data->target_callbacks->stop) {
			data->target_callbacks->stop(data->target_config);
		}
		i2c_mspm0_reset_target(dev);
		k_sem_give(&data->i2c_lock);
		break;
	default:
		break;
	}
}
#endif /* CONFIG_I2C_TARGET */

static inline void i2c_mspm0_isr_controller(const struct device *dev)
{
	const struct i2c_mspm0_config *config = dev->config;
	struct i2c_mspm0_data *data = dev->data;
	const mm_reg_t base = config->base;

	switch (i2c_get_pending_interrupt(base)) {
	case I2C_CPU_INT_IIDX_STAT_MRXDONEFG:
		/*
		 * Transfer complete. Drain any bytes remaining in the RX FIFO.
		 * With a threshold > 1, MRXFIFOTRG only fires when >= threshold
		 * bytes are present; bytes that arrived after the last trigger
		 * (or transfers shorter than the threshold) sit in the FIFO
		 * and must be drained here.
		 */
		while (i2c_is_controller_rx_fifo_empty(base) != true) {
			if (data->transfer_count < data->transfer_len) {
				data->msg_buf[data->transfer_count++] =
					i2c_receive_controller_data(base);
			} else {
				i2c_receive_controller_data(base);
			}
		}
		data->state = I2C_MSPM0_RX_COMPLETE;

		/* Advance the state machine to the next dispatch entry */
		data->dispatch_idx++;
		if (data->dispatch_idx < data->dispatch_count) {
			i2c_mspm0_start_dispatch(dev);
		} else {
			/* All dispatch entries complete: wake the transfer thread */
			data->state = I2C_MSPM0_IDLE;
			k_sem_give(&data->device_sync_sem);
		}
		break;
	case I2C_CPU_INT_IIDX_STAT_MTXDONEFG:
		i2c_disable_interrupt(base, I2C_CPU_INT_IMASK_MTXFIFOTRG_SET);
		data->state = I2C_MSPM0_TX_COMPLETE;

		data->dispatch_idx++;
		if (data->dispatch_idx < data->dispatch_count) {
			i2c_mspm0_start_dispatch(dev);
		} else {
			data->state = I2C_MSPM0_IDLE;
			k_sem_give(&data->device_sync_sem);
		}
		break;
	case I2C_CPU_INT_IIDX_STAT_MRXFIFOTRG:
		/* Receive all bytes from target */
		if (data->state != I2C_MSPM0_RX_COMPLETE) {
			data->state = I2C_MSPM0_RX_INPROGRESS;
		}
		while (i2c_is_controller_rx_fifo_empty(base) != true) {
			if (data->transfer_count < data->transfer_len) {
				data->msg_buf[data->transfer_count++] =
					i2c_receive_controller_data(base);
			} else {
				/* Ignore if transaction length exceeded */
				i2c_receive_controller_data(base);
			}
		}
		break;
	case I2C_CPU_INT_IIDX_STAT_MTXFIFOTRG:
		data->state = I2C_MSPM0_TX_INPROGRESS;
		/* Fill TX FIFO with next bytes to send */
		if (data->transfer_count < data->transfer_len) {
			data->transfer_count += i2c_fill_controller_tx_fifo(
				base, &data->msg_buf[data->transfer_count],
				data->transfer_len - data->transfer_count);
		}
		break;
	case I2C_CPU_INT_IIDX_STAT_MNACKFG:
		if ((data->state == I2C_MSPM0_RX_STARTED) ||
		    (data->state == I2C_MSPM0_TX_STARTED)) {
			/* NACK interrupt if I2C Target is disconnected */
			data->state = I2C_MSPM0_ERROR;
			data->transfer_ret = -EIO;
			k_sem_give(&data->device_sync_sem);
			/*
			 * Release the bus lock now. The hardware may still
			 * generate a STOP after a NACK, but the subsequent
			 * MSTOPFG give is harmless (semaphore max=1, clamped).
			 */
			k_sem_give(&data->i2c_lock);
		}
		break;
	case I2C_CPU_INT_IIDX_STAT_MARBLOSTFG:
		/*
		 * Arbitration lost: another controller won the bus. Signal the
		 * error to the waiting transfer so it returns -EIO promptly
		 * rather than blocking until the software timeout expires.
		 * Release i2c_lock immediately; a subsequent STOP give is safe
		 * since the semaphore is capped at max=1.
		 */
		data->state = I2C_MSPM0_ERROR;
		data->transfer_ret = -EIO;
		k_sem_give(&data->device_sync_sem);
		k_sem_give(&data->i2c_lock);
		break;
	case I2C_CPU_INT_IIDX_STAT_TIMEOUTA:
		data->state = I2C_MSPM0_TIMEOUT;
		data->transfer_ret = -ETIMEDOUT;
		k_sem_give(&data->device_sync_sem);
		i2c_disable_interrupt(base, TI_MSPM0_CONTROLLER_INTERRUPTS);
		i2c_clear_interrupt_status(base, TI_MSPM0_CONTROLLER_INTERRUPTS);
		i2c_flush_controller_tx_fifo(base);
		__fallthrough;
	case I2C_CPU_INT_IIDX_STAT_MSTOPFG:
		k_sem_give(&data->i2c_lock);
	default:
		break;
	}
}

static inline void i2c_mspm0_isr(const struct device *dev)
{
	struct i2c_mspm0_data *data = dev->data;

#ifdef CONFIG_I2C_TARGET
	if (data->is_target) {
		const struct i2c_mspm0_config *config = dev->config;
		const mm_reg_t base = config->base;

		/*
		 * In dual-role mode the controller and target hardware are
		 * independent, so both controller and target interrupts can fire.
		 * Use the MIS register (non-destructive read) to classify the
		 * pending interrupt before dispatching to the appropriate handler.
		 * TIMEOUT_A is shared; when a controller transfer is active
		 * treat it as a controller interrupt.
		 */
		uint32_t mis =
			i2c_get_enabled_interrupt_status(base, TI_MSPM0_CONTROLLER_INTERRUPTS_ALL);

		if (mis != 0) {
			i2c_mspm0_isr_controller(dev);
		} else {
			i2c_mspm0_isr_target(dev);
		}
		return;
	}
#endif
	i2c_mspm0_isr_controller(dev);
}

static DEVICE_API(i2c, i2c_mspm0_driver_api) = {
	.configure = i2c_mspm0_configure,
	.get_config = i2c_mspm0_get_config,
	.transfer = i2c_mspm0_transfer,
#ifdef CONFIG_I2C_RTIO
	.iodev_submit = i2c_iodev_submit_fallback,
#endif
#ifdef CONFIG_I2C_TARGET
	.target_register = i2c_mspm0_target_register,
	.target_unregister = i2c_mspm0_target_unregister,
#endif
};

/* Macros to assist with the device-specific initialization */
#define MERGE_BUF_SIZE(index)                                                                      \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(index, merge_buf_size),                                  \
			(DT_INST_PROP(index, merge_buf_size)), (0))

#define USES_MERGE_BUF(index) COND_CODE_0(MERGE_BUF_SIZE(index), (0), (1))

/*
 * Convert the DT byte-count property to the pre-encoded FIFO register field
 * values written into MFIFOCTL/SFIFOCTL at init time.
 *
 * TX: TXTRIG field = raw byte count, stored in bits [2:0].
 * RX: RXTRIG field = (byte count - 1), stored in bits [10:8].
 */
#define TX_FIFO_THRESHOLD(index)                                                                   \
	FIELD_PREP(I2C_MFIFOCTL_TXTRIG_MASK, DT_INST_PROP(index, controller_tx_fifo_threshold))

#define RX_FIFO_THRESHOLD(index)                                                                   \
	FIELD_PREP(I2C_MFIFOCTL_RXTRIG_MASK, DT_INST_PROP(index, controller_rx_fifo_threshold) - 1)

#define I2C_MSPM0_CONFIG_IRQ_FUNC(index)                                                           \
	static void i2c_mspm0_irq_config_func_##index(const struct device *dev)                    \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(index), DT_INST_IRQ(index, priority), i2c_mspm0_isr,      \
			    DEVICE_DT_INST_GET(index), 0);                                         \
		irq_enable(DT_INST_IRQN(index));                                                   \
	}

#define MSP_I2C_INIT_FN(index)                                                                     \
	PINCTRL_DT_INST_DEFINE(index);                                                             \
	static struct mspm0_sys_clock mspm0_i2c_clockSys##index = MSPM0_CLOCK_SUBSYS_FN(index);    \
	I2C_MSPM0_CONFIG_IRQ_FUNC(index)                                                           \
                                                                                                   \
	IF_ENABLED(USES_MERGE_BUF(index), (                                                        \
	static uint8_t mspm0_i2c_msg_buf_##index[MERGE_BUF_SIZE(index)];                           \
	));                                       \
                                                                                                   \
	static const struct i2c_mspm0_config i2c_mspm0_cfg_##index = {                             \
		.base = DT_INST_REG_ADDR(index),                                                   \
		.clock_subsys = &mspm0_i2c_clockSys##index,                                        \
		.bitrate = DT_INST_PROP(index, clock_frequency),                                   \
		.merge_buf_size = MERGE_BUF_SIZE(index),                                           \
		IF_ENABLED(USES_MERGE_BUF(index), (                                                \
		.merge_buf = mspm0_i2c_msg_buf_##index,                                            \
		)) .pinctrl =                 \
					PINCTRL_DT_INST_DEV_CONFIG_GET(index),                     \
			.irq_config_func = i2c_mspm0_irq_config_func_##index,                      \
			.controller_tx_fifo_threshold = TX_FIFO_THRESHOLD(index),                  \
			.controller_rx_fifo_threshold = RX_FIFO_THRESHOLD(index),                  \
			.clock_config = {                                                          \
				.clock_sel = MSPM0_CLOCK_PERIPH_REG_MASK(                          \
					DT_INST_CLOCKS_CELL(index, clk)),                          \
				.divide_ratio = I2C_CLOCK_DIVIDE_1,                                \
			}};                                                                        \
	static struct i2c_mspm0_data i2c_mspm0_data_##index;                                       \
	I2C_DEVICE_DT_INST_DEFINE(index, i2c_mspm0_init, NULL, &i2c_mspm0_data_##index,            \
				  &i2c_mspm0_cfg_##index, POST_KERNEL, CONFIG_I2C_INIT_PRIORITY,   \
				  &i2c_mspm0_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MSP_I2C_INIT_FN)
