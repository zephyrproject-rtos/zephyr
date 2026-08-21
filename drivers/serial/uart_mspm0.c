/*
 * Copyright (c) 2025-2026 Texas Instruments
 * Copyright (c) 2025 Linumiz
 * Copyright (c) 2025 Bang & Olufsen A/S, Denmark
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_mspm0_uart

/* Zephyr includes */
#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/mspm0_clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/dt-bindings/uart/mspm0_uart.h>
#include <zephyr/irq.h>

/*
 * This driver uses its own native register definitions (mspm0_uart_regs and
 * friends) so it does not depend on the SDK's UART_Regs at all. The macros
 * and delay_cycles below are still guarded to avoid redefinition warnings when
 * CONFIG_HAS_MSPM0_SDK is enabled and the SDK header defines them too.
 */
#ifndef CONFIG_HAS_MSPM0_SDK

/*
 * GPRCM / power and reset
 */
#define UART_PWREN_ENABLE_ENABLE       BIT(0)
#define UART_PWREN_KEY_UNLOCK_W        0x26000000U
#define UART_RSTCTL_RESETASSERT_ASSERT BIT(0)
#define UART_RSTCTL_RESETSTKYCLR_CLR   BIT(1)
#define UART_RSTCTL_KEY_UNLOCK_W       0xB1000000U

/*
 * CTL0 Control Register 0
 */
#define UART_CTL0_ENABLE_MASK   GENMASK(0, 0)
#define UART_CTL0_ENABLE_ENABLE BIT(0)
#define UART_CTL0_HSE_MASK      GENMASK(16, 15)
#define UART_CTL0_HSE_OVS16     FIELD_PREP(UART_CTL0_HSE_MASK, 0)
#define UART_CTL0_HSE_OVS8      FIELD_PREP(UART_CTL0_HSE_MASK, 1)
#define UART_CTL0_HSE_OVS3      FIELD_PREP(UART_CTL0_HSE_MASK, 2)
#define UART_CTL0_RXE_ENABLE    BIT(3)
#define UART_CTL0_TXE_ENABLE    BIT(4)
#define UART_CTL0_RTSEN_MASK    GENMASK(13, 13)
#define UART_CTL0_RTSEN_DISABLE 0U
#define UART_CTL0_RTSEN_ENABLE  BIT(13)
#define UART_CTL0_CTSEN_MASK    GENMASK(14, 14)
#define UART_CTL0_CTSEN_DISABLE 0U
#define UART_CTL0_CTSEN_ENABLE  BIT(14)
#define UART_CTL0_MODE_UART     0U
#define UART_CTL0_FEN_ENABLE    BIT(17)

/*
 * LCRH Line Control Register
 */
#define UART_LCRH_PEN_MASK      GENMASK(1, 1)
#define UART_LCRH_PEN_ENABLE    BIT(1)
#define UART_LCRH_EPS_MASK      GENMASK(2, 2)
#define UART_LCRH_EPS_ODD       0U
#define UART_LCRH_EPS_EVEN      BIT(2)
#define UART_LCRH_STP2_MASK     GENMASK(3, 3)
#define UART_LCRH_STP2_DISABLE  0U
#define UART_LCRH_STP2_ENABLE   BIT(3)
#define UART_LCRH_WLEN_MASK     GENMASK(5, 4)
#define UART_LCRH_WLEN_DATABIT5 FIELD_PREP(UART_LCRH_WLEN_MASK, 0)
#define UART_LCRH_WLEN_DATABIT6 FIELD_PREP(UART_LCRH_WLEN_MASK, 1)
#define UART_LCRH_WLEN_DATABIT7 FIELD_PREP(UART_LCRH_WLEN_MASK, 2)
#define UART_LCRH_WLEN_DATABIT8 FIELD_PREP(UART_LCRH_WLEN_MASK, 3)
#define UART_LCRH_SPS_MASK      GENMASK(6, 6)
#define UART_LCRH_SPS_ENABLE    BIT(6)

/*
 * STAT Status Register
 */
#define UART_STAT_TXFE_MASK GENMASK(6, 6)
#define UART_STAT_TXFF_MASK GENMASK(7, 7)
#define UART_STAT_RXFE_MASK GENMASK(2, 2)

/*
 * IFLS Interrupt FIFO Level Select Register
 */
#define UART_IFLS_TXIFLSEL_MASK      GENMASK(2, 0)
#define UART_IFLS_TXIFLSEL_LVL_3_4   FIELD_PREP(UART_IFLS_TXIFLSEL_MASK, 1)
#define UART_IFLS_TXIFLSEL_LVL_1_2   FIELD_PREP(UART_IFLS_TXIFLSEL_MASK, 2)
#define UART_IFLS_TXIFLSEL_LVL_1_4   FIELD_PREP(UART_IFLS_TXIFLSEL_MASK, 3)
#define UART_IFLS_TXIFLSEL_LVL_EMPTY FIELD_PREP(UART_IFLS_TXIFLSEL_MASK, 5)
#define UART_IFLS_TXIFLSEL_LVL_1     FIELD_PREP(UART_IFLS_TXIFLSEL_MASK, 7)

#define UART_IFLS_RXIFLSEL_MASK     GENMASK(6, 4)
#define UART_IFLS_RXIFLSEL_LVL_1_4  FIELD_PREP(UART_IFLS_RXIFLSEL_MASK, 1)
#define UART_IFLS_RXIFLSEL_LVL_1_2  FIELD_PREP(UART_IFLS_RXIFLSEL_MASK, 2)
#define UART_IFLS_RXIFLSEL_LVL_3_4  FIELD_PREP(UART_IFLS_RXIFLSEL_MASK, 3)
#define UART_IFLS_RXIFLSEL_LVL_FULL FIELD_PREP(UART_IFLS_RXIFLSEL_MASK, 5)
#define UART_IFLS_RXIFLSEL_LVL_1    FIELD_PREP(UART_IFLS_RXIFLSEL_MASK, 7)

#define UART_IFLS_RXTOSEL_MASK GENMASK(11, 8)

/*
 * CPU_INT Interrupt index (IIDX) values
 */
#define UART_CPU_INT_IIDX_STAT_NO_INTR 0x00000000U
#define UART_CPU_INT_IIDX_STAT_RTFG    0x00000001U
#define UART_CPU_INT_IIDX_STAT_FEFG    0x00000002U
#define UART_CPU_INT_IIDX_STAT_BEFG    0x00000004U
#define UART_CPU_INT_IIDX_STAT_RXIFG   0x0000000BU
#define UART_CPU_INT_IIDX_STAT_TXIFG   0x0000000CU
#define UART_CPU_INT_IIDX_STAT_EOT     0x0000000DU

/*
 * CPU_INT Interrupt mask (IMASK) bits
 */
#define UART_CPU_INT_IMASK_FRMERR_SET BIT(1)
#define UART_CPU_INT_IMASK_BRKERR_SET BIT(3)
#define UART_CPU_INT_IMASK_RXINT_SET  BIT(10)
#define UART_CPU_INT_IMASK_TXINT_SET  BIT(11)
#define UART_CPU_INT_IMASK_EOT_SET    BIT(12)
#define UART_CPU_INT_IMASK_RTOUT_SET  BIT(0)

/*
 * CPU_INT Interrupt set (ISET) bits
 */
#define UART_CPU_INT_ISET_RXINT_SET BIT(10)
#define UART_CPU_INT_ISET_TXINT_SET BIT(11)

#endif /* !CONFIG_HAS_MSPM0_SDK */

/*
 * UART register-map structs.
 */
typedef struct {
	volatile const uint32_t iidx; /* (@ 0x00001020) Interrupt index		*/
	uint32_t reserved0;
	volatile uint32_t imask; /* (@ 0x00001028) Interrupt mask			*/
	uint32_t reserved1;
	volatile const uint32_t ris; /* (@ 0x00001030) Raw interrupt status	*/
	uint32_t reserved2;
	volatile const uint32_t mis; /* (@ 0x00001038) Masked interrupt status */
	uint32_t reserved3;
	volatile uint32_t iset; /* (@ 0x00001040) Interrupt set			*/
	uint32_t reserved4;
	volatile uint32_t iclr; /* (@ 0x00001048) Interrupt clear		*/
} mspm0_uart_cpu_int_regs;

typedef struct {
	volatile uint32_t pwren;  /* (@ 0x00000800) Power enable			*/
	volatile uint32_t rstctl; /* (@ 0x00000804) Reset Control			*/
	volatile uint32_t clkcfg; /* (@ 0x00000808) Clock Configuration	*/
	uint32_t reserved0[2];
	volatile const uint32_t stat; /* (@ 0x00000814) Status Register		*/
} mspm0_uart_gprcm_regs;

typedef struct {
	uint32_t reserved0[512];
	mspm0_uart_gprcm_regs gprcm; /* (@ 0x00000800) */
	uint32_t reserved1[506];
	volatile uint32_t clkdiv; /* (@ 0x00001000) Clock Divider  */
	uint32_t reserved2;
	volatile uint32_t clksel; /* (@ 0x00001008) Clock Select	 */
	uint32_t reserved3[3];
	volatile uint32_t pdbgctl; /* (@ 0x00001018) Debug Control	*/
	uint32_t reserved4;
	mspm0_uart_cpu_int_regs cpu_int; /* (@ 0x00001020) */
	/* DMA_TRIG_RX (0x1050), gap, DMA_TRIG_TX (0x1080), padding to 0x10E0 */
	uint32_t reserved5[37];
	volatile uint32_t evt_mode; /* (@ 0x000010E0) Event Mode	 */
	volatile uint32_t intctl;   /* (@ 0x000010E4) Interrupt ctrl */
	uint32_t reserved6[6];
	volatile uint32_t ctl0;       /* (@ 0x00001100) Control Reg 0  */
	volatile uint32_t lcrh;       /* (@ 0x00001104) Line Control	 */
	volatile const uint32_t stat; /* (@ 0x00001108) Status		 */
	volatile uint32_t ifls;       /* (@ 0x0000110C) FIFO Level Sel */
	volatile uint32_t ibrd;       /* (@ 0x00001110) Int Baud Div	 */
	volatile uint32_t fbrd;       /* (@ 0x00001114) Frac Baud Div  */
	volatile uint32_t gfctl;      /* (@ 0x00001118) Glitch Filter  */
	uint32_t reserved7;
	volatile uint32_t txdata;       /* (@ 0x00001120) TX Data		 */
	volatile const uint32_t rxdata; /* (@ 0x00001124) RX Data		 */
	uint32_t reserved8[2];
	volatile uint32_t lincnt; /* (@ 0x00001130) LIN Counter	 */
	volatile uint32_t linctl; /* (@ 0x00001134) LIN Control	 */
	volatile uint32_t linc0;  /* (@ 0x00001138) LIN Capture 0  */
	volatile uint32_t linc1;  /* (@ 0x0000113C) LIN Capture 1  */
	volatile uint32_t irctl;  /* (@ 0x00001140) IrDA Control	 */
	uint32_t reserved9;
	volatile uint32_t amask; /* (@ 0x00001148) Self Addr Mask */
	volatile uint32_t addr;  /* (@ 0x0000114C) Self Address	 */
	uint32_t reserved10[4];
	volatile uint32_t clkdiv2; /* (@ 0x00001160) Clock Divider2 */
} mspm0_uart_regs;

/*
 * delay_cycles busy-wait loop.
 *
 * Calibrated to the same cycle accounting as DL_Common_delayCycles:
 *	 SUBS + (loop: SUBS, NOP, BHS), each loop iteration costs 4 cycles.
 *
 * k_busy_wait() cannot be used here because this driver initialises at
 * PRE_KERNEL_1, before the Zephyr system clock (cortex_m_systick.c) has
 * started SysTick at PRE_KERNEL_2. Until then sys_clock_cycle_get_32()
 * always returns 0, causing k_busy_wait() to spin forever.
 */
#ifndef CONFIG_HAS_MSPM0_SDK
static inline void delay_cycles(uint32_t cycles)
{
	uint32_t scratch;

	__asm volatile(".syntax unified\n\t"
		       "SUBS %0, %[c], #2\n\t"
		       "1:\n\t"
		       "SUBS %0, %0, #4\n\t"
		       "NOP\n\t"
		       "BHS  1b\n\t"
		       : "=&r"(scratch)
		       : [c] "r"(cycles));
}
#endif /* !CONFIG_HAS_MSPM0_SDK */

/*
 * Highest UART Receive Interrupt Timeout
 */
#define UART_IFLS_RXTOSEL_HIGHEST_VAL FIELD_PREP(UART_IFLS_RXTOSEL_MASK, 15)

/*
 * Driver config and data structs
 */
struct uart_mspm0_config {
	mspm0_uart_regs *regs;
	const struct mspm0_sys_clock *clock_subsys;
	const struct pinctrl_dev_config *pinctrl;
	/* Clock configuration */
	uint32_t clk_sel;
	uint32_t clk_div;
	/* Values/actions that are handled differently for standalone and UNICOMM UARTs */
	uint32_t stat_txff_mask;
	uint32_t stat_txfe_mask;
	bool skip_power_on;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	void (*irq_config_func)(const struct device *dev);
	/* UART FIFO thresholds */
	uint8_t rx_fifo_threshold;
	uint8_t tx_fifo_threshold;
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

/*
 * Raw register field values for the UART line configuration.
 * Grouped here so they can be accessed as data->uart_config.parity, etc.
 */
struct uart_mspm0_line_config {
	uint32_t parity;      /* LCRH parity bits */
	uint32_t wordLength;  /* LCRH WLEN field */
	uint32_t stopBits;    /* LCRH STP2 field */
	uint32_t flowControl; /* CTL0 RTSEN/CTSEN bits */
};

struct uart_mspm0_data {
	/* Baud Rate */
	uint32_t current_speed;
	/* UART line configuration - raw register field values */
	struct uart_mspm0_line_config uart_config;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	/* Callback function pointer */
	uart_irq_callback_user_data_t cb;
	/* Callback function arg */
	void *cb_data;
	/* Pending interrupt backup */
	uint32_t pending_interrupt;
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

#define UART_MSPM0_ENABLE_POWER(regs)                                                              \
	((regs)->gprcm.pwren = (UART_PWREN_KEY_UNLOCK_W | UART_PWREN_ENABLE_ENABLE))
#define UART_MSPM0_RESET(regs)                                                                     \
	((regs)->gprcm.rstctl = (UART_RSTCTL_KEY_UNLOCK_W | UART_RSTCTL_RESETSTKYCLR_CLR |         \
				 UART_RSTCTL_RESETASSERT_ASSERT))
#define UART_MSPM0_ENABLE(regs)  ((regs)->ctl0 |= UART_CTL0_ENABLE_ENABLE)
#define UART_MSPM0_DISABLE(regs) ((regs)->ctl0 &= ~UART_CTL0_ENABLE_MASK)

/*
 * Definitions for UNICOMM UART
 */
#define UNICOMMUART_STAT_TXFF_MASK GENMASK(6, 6)
#define UNICOMMUART_STAT_TXFE_MASK GENMASK(5, 5)

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static const uint32_t uart_mspm0_rx_fifo_level[] = {
	[MSPM0_UART_RX_FIFO_LEVEL_ONE_ENTRY] = UART_IFLS_RXIFLSEL_LVL_1,
	[MSPM0_UART_RX_FIFO_LEVEL_1_4_FULL] = UART_IFLS_RXIFLSEL_LVL_1_4,
	[MSPM0_UART_RX_FIFO_LEVEL_1_2_FULL] = UART_IFLS_RXIFLSEL_LVL_1_2,
	[MSPM0_UART_RX_FIFO_LEVEL_3_4_FULL] = UART_IFLS_RXIFLSEL_LVL_3_4,
	[MSPM0_UART_RX_FIFO_LEVEL_FULL] = UART_IFLS_RXIFLSEL_LVL_FULL,
};
static const uint32_t uart_mspm0_tx_fifo_level[] = {
	[MSPM0_UART_TX_FIFO_LEVEL_ONE_ENTRY] = UART_IFLS_TXIFLSEL_LVL_1,
	[MSPM0_UART_TX_FIFO_LEVEL_1_4_EMPTY] = UART_IFLS_TXIFLSEL_LVL_1_4,
	[MSPM0_UART_TX_FIFO_LEVEL_1_2_EMPTY] = UART_IFLS_TXIFLSEL_LVL_1_2,
	[MSPM0_UART_TX_FIFO_LEVEL_3_4_EMPTY] = UART_IFLS_TXIFLSEL_LVL_3_4,
	[MSPM0_UART_TX_FIFO_LEVEL_EMPTY] = UART_IFLS_TXIFLSEL_LVL_EMPTY,
};
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static inline void uart_mspm0_set_clock(mspm0_uart_regs *regs, uint32_t clk_sel, uint32_t clk_div)
{
	regs->clksel = clk_sel;
	regs->clkdiv = clk_div;
}

/* Configure line control: parity, word length, stop bits */
static void uart_mspm0_configure_line(mspm0_uart_regs *regs, uint32_t parity, uint32_t word_length,
				      uint32_t stop_bits, uint32_t flow_control)
{
	uint32_t ctl0 = UART_CTL0_MODE_UART | UART_CTL0_RXE_ENABLE | UART_CTL0_TXE_ENABLE |
			(flow_control & (UART_CTL0_RTSEN_MASK | UART_CTL0_CTSEN_MASK));

	uint32_t lcrh = (parity & (UART_LCRH_PEN_MASK | UART_LCRH_EPS_MASK | UART_LCRH_SPS_MASK)) |
			(word_length & UART_LCRH_WLEN_MASK) | (stop_bits & UART_LCRH_STP2_MASK);

	regs->ctl0 = ctl0;
	regs->lcrh = lcrh;
}

/* Configure baud rate - implements same algorithm as DL_UART_configBaudRate */
static void uart_mspm0_configure_baudrate(mspm0_uart_regs *regs, uint32_t uart_clk,
					  uint32_t baudrate)
{
	uint32_t divisor;
	uint32_t hse_setting;

	/* Determine oversampling rate */
	if ((baudrate * 8U) > uart_clk) {
		/* 3x oversampling */
		hse_setting = UART_CTL0_HSE_OVS3;
		divisor = ((uart_clk * 64U) / (baudrate * 3U)) + (1U / 2U);
	} else if ((baudrate * 16U) > uart_clk) {
		/* 8x oversampling */
		hse_setting = UART_CTL0_HSE_OVS8;

		uint32_t adjusted_baud = baudrate / 2U;

		divisor = (((uart_clk * 8U) / adjusted_baud) + 1U) / 2U;
	} else {
		/* 16x oversampling (default) */
		hse_setting = UART_CTL0_HSE_OVS16;
		divisor = (((uart_clk * 8U) / baudrate) + 1U) / 2U;
	}

	/* Apply oversampling setting */
	regs->ctl0 = (regs->ctl0 & ~UART_CTL0_HSE_MASK) | hse_setting;

	/* Set integer and fractional parts */
	regs->ibrd = divisor >> 6U;   /* Integer part */
	regs->fbrd = divisor & 0x3FU; /* Fractional part */

	/* TRM "UART Operation" section:
	 * When updating the baud-rate divisor (IBRD or IFRD), the
	 * LCRH register must also be written to (any bit in LCRH can
	 * be written to for updating the baud-rate divisor).
	 */
	regs->lcrh = regs->lcrh;
}

static int uart_mspm0_poll_in(const struct device *dev, unsigned char *c)
{
	const struct uart_mspm0_config *config = dev->config;
	mspm0_uart_regs *regs = config->regs;

	if ((regs->stat & UART_STAT_RXFE_MASK) != 0U) {
		return -1;
	}

	*c = (unsigned char)(regs->rxdata & 0xFFU);
	return 0;
}

static void uart_mspm0_poll_out(const struct device *dev, unsigned char c)
{
	const struct uart_mspm0_config *config = dev->config;
	mspm0_uart_regs *regs = config->regs;
	unsigned int key;

	/* Skip the irq_lock()s below when in ISR context. */
	if (k_is_in_isr()) {
		while ((regs->stat & config->stat_txff_mask) != 0U) {
		}
		regs->txdata = (uint32_t)c;
		return;
	}

	/* Wait until TX FIFO has space, then write atomically.
	 *
	 * The check-and-write must be performed under an IRQ lock as without it
	 * a higher-priority context could preempt between the TXFF check and the
	 * TXDATA write, fill the TX FIFO, and cause this write to silently
	 * drop the byte.
	 */
	do {
		key = irq_lock();
		if ((regs->stat & config->stat_txff_mask) == 0U) {
			regs->txdata = (uint32_t)c;
			irq_unlock(key);
			return;
		}
		irq_unlock(key);
	} while (true);
}

static int uart_mspm0_install_configuration(const struct device *dev)
{
	const struct device *clk_dev = DEVICE_DT_GET(DT_NODELABEL(ckm));
	const struct uart_mspm0_config *config = dev->config;
	struct uart_mspm0_data *data = dev->data;
	uint32_t clock_rate;
	uint32_t uart_clk;
	int ret;

	/* Set clock source and divider */
	uart_mspm0_set_clock(config->regs, config->clk_sel, config->clk_div);

	/* Configure line control */
	uart_mspm0_configure_line(config->regs, data->uart_config.parity,
				  data->uart_config.wordLength, data->uart_config.stopBits,
				  data->uart_config.flowControl);

	/* Get source clock rate */
	ret = clock_control_get_rate(clk_dev, (struct mspm0_sys_clock *)config->clock_subsys,
				     &clock_rate);
	if (ret < 0) {
		return ret;
	}

	/* Calculate UART source clock */
	/* CLKDIV=0 means divide by 1, CLKDIV=1 means divide by 2, etc */
	uart_clk = clock_rate / (config->clk_div + 1U);

	/* Configure baud rate */
	uart_mspm0_configure_baudrate(config->regs, uart_clk, data->current_speed);

	/* Re-enable FIFOs: uart_mspm0_configure_line() overwrites CTL0 entirely,
	 * clearing the FEN bit. Always re-enable FIFOs after reconfiguring the
	 * line control registers so that FIFO-based interrupt thresholds (IFLS)
	 * remain operative.
	 */
	config->regs->ctl0 |= UART_CTL0_FEN_ENABLE;

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	/* Restore FIFO interrupt level select thresholds. These are also wiped
	 * when the peripheral is reset/re-initialised, so restore them here in
	 * addition to the init() path.
	 *
	 * RX threshold: use LVL_1 (fire on any received byte). LVL_1_2 (0x20)
	 * is undefined for ULP-domain UARTs (MSPM0L series) where only values
	 * 0 and 4 are specified; LVL_1 (0x70) is valid across all variants and
	 * avoids losing bytes when bursts are smaller than the half-full level.
	 */
	config->regs->ifls = (config->regs->ifls & ~UART_IFLS_RXIFLSEL_MASK) |
			     uart_mspm0_rx_fifo_level[config->rx_fifo_threshold];
	config->regs->ifls = (config->regs->ifls & ~UART_IFLS_TXIFLSEL_MASK) |
			     uart_mspm0_tx_fifo_level[config->tx_fifo_threshold];
	config->regs->ifls = (config->regs->ifls & ~UART_IFLS_RXTOSEL_MASK) |
			     UART_IFLS_RXTOSEL_HIGHEST_VAL; /* Highest possible RX timeout */
#endif                                                      /* CONFIG_UART_INTERRUPT_DRIVEN */

	return 0;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE

static const uint32_t uart_parity_to_mspm0[5] = {
	0,                                                               /* NONE */
	UART_LCRH_PEN_ENABLE | UART_LCRH_EPS_ODD,                        /* ODD */
	UART_LCRH_PEN_ENABLE | UART_LCRH_EPS_EVEN,                       /* EVEN */
	UART_LCRH_PEN_ENABLE | UART_LCRH_SPS_ENABLE | UART_LCRH_EPS_ODD, /* STICK_ONE */
	UART_LCRH_PEN_ENABLE | UART_LCRH_SPS_ENABLE | UART_LCRH_EPS_EVEN /* STICK_ZERO */
};

static const uint32_t uart_stop_bits_to_mspm0[4] = {
	UINT32_MAX,             /* 0.5 stop bits - not supported */
	UART_LCRH_STP2_DISABLE, /* 1 stop bit */
	UINT32_MAX,             /* 1.5 stop bits - not supported */
	UART_LCRH_STP2_ENABLE   /* 2 stop bits */
};

static const uint32_t uart_data_bits_to_mspm0[4] = {
	UART_LCRH_WLEN_DATABIT5, /* 5 bits */
	UART_LCRH_WLEN_DATABIT6, /* 6 bits */
	UART_LCRH_WLEN_DATABIT7, /* 7 bits */
	UART_LCRH_WLEN_DATABIT8  /* 8 bits */
};

static const uint32_t uart_flow_control_to_mspm0[2] = {
	UART_CTL0_RTSEN_DISABLE | UART_CTL0_CTSEN_DISABLE, /* NONE */
	UART_CTL0_RTSEN_ENABLE | UART_CTL0_CTSEN_ENABLE    /* RTS_CTS */
};

static int uart_mspm0_translate_in(const uint32_t value_array[], int value_array_length,
				   uint8_t uart_cfg_value, uint32_t *mspm0_cfg_value)
{
	if (uart_cfg_value >= value_array_length) {
		return -EINVAL;
	}

	if (value_array[uart_cfg_value] == UINT32_MAX) {
		return -ENOSYS;
	}

	*mspm0_cfg_value = value_array[uart_cfg_value];

	return 0;
}

static int uart_mspm0_translate_out(const uint32_t value_array[], int value_array_length,
				    uint32_t mspm0_cfg_value, uint8_t *uart_cfg_value)
{
	int idx;

	for (idx = 0; idx < value_array_length; idx++) {
		if (value_array[idx] == mspm0_cfg_value) {
			break;
		}
	}

	if (idx == value_array_length || value_array[idx] == UINT32_MAX) {
		return -EINVAL;
	}

	*uart_cfg_value = (uint8_t)idx;

	return 0;
}

static int uart_mspm0_configure(const struct device *dev, const struct uart_config *cfg)
{
	const struct uart_mspm0_config *config = dev->config;
	struct uart_mspm0_data *data = dev->data;
	uint32_t value;
	int ret;

	UART_MSPM0_DISABLE(config->regs);

	data->current_speed = cfg->baudrate;

	ret = uart_mspm0_translate_in(uart_parity_to_mspm0, ARRAY_SIZE(uart_parity_to_mspm0),
				      cfg->parity, &value);
	if (ret != 0) {
		return ret;
	}

	data->uart_config.parity = value;

	ret = uart_mspm0_translate_in(uart_stop_bits_to_mspm0, ARRAY_SIZE(uart_stop_bits_to_mspm0),
				      cfg->stop_bits, &value);
	if (ret != 0) {
		return ret;
	}

	data->uart_config.stopBits = value;

	ret = uart_mspm0_translate_in(uart_data_bits_to_mspm0, ARRAY_SIZE(uart_data_bits_to_mspm0),
				      cfg->data_bits, &value);
	if (ret != 0) {
		return ret;
	}

	data->uart_config.wordLength = value;

	ret = uart_mspm0_translate_in(uart_flow_control_to_mspm0,
				      ARRAY_SIZE(uart_flow_control_to_mspm0), cfg->flow_ctrl,
				      &value);
	if (ret != 0) {
		return ret;
	}

	data->uart_config.flowControl = value;

	ret = uart_mspm0_install_configuration(dev);
	if (ret != 0) {
		return ret;
	}

	UART_MSPM0_ENABLE(config->regs);

	return 0;
}

static int uart_mspm0_config_get(const struct device *dev, struct uart_config *cfg)
{
	struct uart_mspm0_data *data = dev->data;
	int ret;

	cfg->baudrate = data->current_speed;

	ret = uart_mspm0_translate_out(uart_parity_to_mspm0, ARRAY_SIZE(uart_parity_to_mspm0),
				       data->uart_config.parity, &cfg->parity);
	if (ret != 0) {
		return ret;
	}

	ret = uart_mspm0_translate_out(uart_stop_bits_to_mspm0, ARRAY_SIZE(uart_stop_bits_to_mspm0),
				       data->uart_config.stopBits, &cfg->stop_bits);
	if (ret != 0) {
		return ret;
	}

	ret = uart_mspm0_translate_out(uart_data_bits_to_mspm0, ARRAY_SIZE(uart_data_bits_to_mspm0),
				       data->uart_config.wordLength, &cfg->data_bits);
	if (ret != 0) {
		return ret;
	}

	ret = uart_mspm0_translate_out(uart_flow_control_to_mspm0,
				       ARRAY_SIZE(uart_flow_control_to_mspm0),
				       data->uart_config.flowControl, &cfg->flow_ctrl);
	if (ret != 0) {
		return ret;
	}

	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static int uart_mspm0_err_check(const struct device *dev)
{
	struct uart_mspm0_data *data = dev->data;

	switch (data->pending_interrupt) {
	case UART_CPU_INT_IIDX_STAT_BEFG: /* Break error */
		return UART_BREAK;
	case UART_CPU_INT_IIDX_STAT_FEFG: /* Framing error */
		return UART_ERROR_FRAMING;
	default:
		return 0;
	}
}

#define UART_MSPM0_TX_INTERRUPTS (UART_CPU_INT_IMASK_TXINT_SET | UART_CPU_INT_IMASK_EOT_SET)
#define UART_MSPM0_RX_INTERRUPTS (UART_CPU_INT_IMASK_RXINT_SET | UART_CPU_INT_IMASK_RTOUT_SET)

static int uart_mspm0_fifo_fill(const struct device *dev, const uint8_t *tx_data, int size)
{
	const struct uart_mspm0_config *config = dev->config;
	int count = 0;

	while (count < size && ((config->regs->stat & config->stat_txff_mask) == 0)) {
		config->regs->txdata = tx_data[count];
		count++;
	}

	return count;
}

static int uart_mspm0_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	const struct uart_mspm0_config *config = dev->config;
	int count = 0;

	while (count < size && ((config->regs->stat & UART_STAT_RXFE_MASK) == 0)) {
		rx_data[count] = (uint8_t)(config->regs->rxdata & 0xFF);
		count++;
	}

	return count;
}

static void uart_mspm0_irq_tx_enable(const struct device *dev)
{
	const struct uart_mspm0_config *config = dev->config;

	config->regs->cpu_int.imask |= UART_MSPM0_TX_INTERRUPTS;

	/* If the TX FIFO is already empty (below the LVL_EMPTY threshold) the
	 * TXIFG flag may have been cleared by a prior IIDX read without a new
	 * FIFO-drain edge occurring. Force the interrupt pending via ISET so
	 * that the ISR fires immediately rather than waiting for the next byte
	 * to drain through the shift register.
	 */
	if ((config->regs->stat & config->stat_txfe_mask) != 0U) {
		config->regs->cpu_int.iset = UART_CPU_INT_ISET_TXINT_SET;
	}
}

static void uart_mspm0_irq_tx_disable(const struct device *dev)
{
	const struct uart_mspm0_config *config = dev->config;

	config->regs->cpu_int.imask &= ~UART_MSPM0_TX_INTERRUPTS;
}

static int uart_mspm0_irq_tx_ready(const struct device *dev)
{
	const struct uart_mspm0_config *config = dev->config;
	struct uart_mspm0_data *data = dev->data;

	return ((data->pending_interrupt == UART_CPU_INT_IIDX_STAT_TXIFG) ||
		(data->pending_interrupt == UART_CPU_INT_IIDX_STAT_EOT)) &&
			       ((config->regs->stat & config->stat_txff_mask) == 0)
		       ? 1
		       : 0;
}

static void uart_mspm0_irq_rx_enable(const struct device *dev)
{
	const struct uart_mspm0_config *config = dev->config;

	config->regs->cpu_int.imask |= UART_MSPM0_RX_INTERRUPTS;

	/* If the RX FIFO already contains data the RXIFG flag may have been
	 * cleared by a prior IIDX read while the interrupt was masked, without
	 * a new FIFO-fill edge occurring to re-assert it.  Force the interrupt
	 * pending via ISET so the ISR fires immediately to drain any bytes that
	 * accumulated while RX interrupts were disabled.
	 */
	if ((config->regs->stat & UART_STAT_RXFE_MASK) == 0U) {
		config->regs->cpu_int.iset = UART_CPU_INT_ISET_RXINT_SET;
	}
}

static void uart_mspm0_irq_rx_disable(const struct device *dev)
{
	const struct uart_mspm0_config *config = dev->config;

	config->regs->cpu_int.imask &= ~UART_MSPM0_RX_INTERRUPTS;
}

static int uart_mspm0_irq_tx_complete(const struct device *dev)
{
	const struct uart_mspm0_config *config = dev->config;

	return ((config->regs->stat & config->stat_txfe_mask) != 0) ? 1 : 0;
}

static int uart_mspm0_irq_rx_ready(const struct device *dev)
{
	const struct uart_mspm0_config *config = dev->config;
	struct uart_mspm0_data *data = dev->data;

	return ((data->pending_interrupt == UART_CPU_INT_IIDX_STAT_RXIFG) ||
		(data->pending_interrupt == UART_CPU_INT_IIDX_STAT_RTFG)) &&
			       ((config->regs->stat & UART_STAT_RXFE_MASK) == 0)
		       ? 1
		       : 0;
}

static int uart_mspm0_irq_is_pending(const struct device *dev)
{
	struct uart_mspm0_data *data = dev->data;

	return data->pending_interrupt != UART_CPU_INT_IIDX_STAT_NO_INTR;
}

static void uart_mspm0_irq_update(const struct device *dev)
{
	struct uart_mspm0_data *data = dev->data;
	const struct uart_mspm0_config *config = dev->config;

	data->pending_interrupt = config->regs->cpu_int.iidx;
}

static void uart_mspm0_irq_callback_set(const struct device *dev, uart_irq_callback_user_data_t cb,
					void *cb_data)
{
	struct uart_mspm0_data *const dev_data = dev->data;

	/* Set callback function and data */
	dev_data->cb = cb;
	dev_data->cb_data = cb_data;
}

#define UART_MSPM0_ERROR_INTERRUPTS (UART_CPU_INT_IMASK_BRKERR_SET | UART_CPU_INT_IMASK_FRMERR_SET)

static void uart_mspm0_irq_error_enable(const struct device *dev)
{
	const struct uart_mspm0_config *config = dev->config;

	config->regs->cpu_int.imask |= UART_MSPM0_ERROR_INTERRUPTS;
}

static void uart_mspm0_irq_error_disable(const struct device *dev)
{
	const struct uart_mspm0_config *config = dev->config;

	config->regs->cpu_int.imask &= ~UART_MSPM0_ERROR_INTERRUPTS;
}

static void uart_mspm0_isr(const struct device *dev)
{
	struct uart_mspm0_data *const dev_data = dev->data;

	/* Perform callback if defined */
	if (dev_data->cb) {
		dev_data->cb(dev, dev_data->cb_data);
	}
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static int uart_mspm0_init(const struct device *dev)
{
	const struct uart_mspm0_config *config = dev->config;
	int ret;

	if (config->skip_power_on == false) {
		/* Reset power */
		UART_MSPM0_RESET(config->regs);
		UART_MSPM0_ENABLE_POWER(config->regs);
		delay_cycles(CONFIG_MSPM0_PERIPH_STARTUP_DELAY);
	}

	/* Init UART pins */
	ret = pinctrl_apply_state(config->pinctrl, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	/* install_configuration() sets FEN and IFLS; just register the IRQ handler */
	ret = uart_mspm0_install_configuration(dev);
	if (ret != 0) {
		return ret;
	}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	config->irq_config_func(dev);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

	/* Enable UART */
	UART_MSPM0_ENABLE(config->regs);

	return 0;
}

static DEVICE_API(uart, uart_mspm0_driver_api) = {
	.poll_in = uart_mspm0_poll_in,
	.poll_out = uart_mspm0_poll_out,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = uart_mspm0_configure,
	.config_get = uart_mspm0_config_get,
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.err_check = uart_mspm0_err_check,
	.fifo_fill = uart_mspm0_fifo_fill,
	.fifo_read = uart_mspm0_fifo_read,
	.irq_tx_enable = uart_mspm0_irq_tx_enable,
	.irq_tx_disable = uart_mspm0_irq_tx_disable,
	.irq_tx_ready = uart_mspm0_irq_tx_ready,
	.irq_rx_enable = uart_mspm0_irq_rx_enable,
	.irq_rx_disable = uart_mspm0_irq_rx_disable,
	.irq_tx_complete = uart_mspm0_irq_tx_complete,
	.irq_rx_ready = uart_mspm0_irq_rx_ready,
	.irq_is_pending = uart_mspm0_irq_is_pending,
	.irq_update = uart_mspm0_irq_update,
	.irq_callback_set = uart_mspm0_irq_callback_set,
	.irq_err_enable = uart_mspm0_irq_error_enable,
	.irq_err_disable = uart_mspm0_irq_error_disable,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define MSP_UART_IRQ_DEFINE(inst)                                                                  \
	static void uart_mspm0_##inst##_irq_register(const struct device *dev)                     \
	{                                                                                          \
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority), uart_mspm0_isr,       \
			    DEVICE_DT_INST_GET(inst), 0);                                          \
		irq_enable(DT_INST_IRQN(inst));                                                    \
	}
#else
#define MSP_UART_IRQ_DEFINE(inst)
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#define MSPM0_CLK_DIV_REG(n) (DT_INST_PROP(n, clk_div) - 1)

#define MSPM0_UART_INIT_FN(index)								\
                                                                                                \
	PINCTRL_DT_INST_DEFINE(index);                                                          \
                                                                                                \
	static const struct mspm0_sys_clock mspm0_uart_sys_clock##index =                       \
		MSPM0_CLOCK_SUBSYS_FN(index);                                                   \
                                                                                                \
	MSP_UART_IRQ_DEFINE(index);                                                             \
                                                                                                \
	static const struct uart_mspm0_config uart_mspm0_cfg_##index = {			\
		.regs = (mspm0_uart_regs *)DT_INST_REG_ADDR(index),                             \
		.pinctrl = PINCTRL_DT_INST_DEV_CONFIG_GET(index),                               \
		.clock_subsys = &mspm0_uart_sys_clock##index,                                   \
		.clk_sel = MSPM0_CLOCK_PERIPH_REG_MASK(DT_INST_CLOCKS_CELL(index, clk)),        \
		.clk_div = MSPM0_CLK_DIV_REG(index),                                            \
		.stat_txff_mask = COND_CODE_1(DT_INST_PROP(index, is_unicomm_uart),		\
				(UNICOMMUART_STAT_TXFF_MASK),					\
				(UART_STAT_TXFF_MASK)),						\
		.stat_txfe_mask = COND_CODE_1(DT_INST_PROP(index, is_unicomm_uart),		\
				(UNICOMMUART_STAT_TXFE_MASK),					\
				(UART_STAT_TXFE_MASK)),						\
		.skip_power_on = COND_CODE_1(DT_INST_PROP(index, is_unicomm_uart),		\
				(true),								\
				(false)),							\
		IF_ENABLED(									\
		  CONFIG_UART_INTERRUPT_DRIVEN,							\
		  (.rx_fifo_threshold = DT_INST_PROP(index, rx_fifo_threshold),))		\
				 IF_ENABLED(							\
			  CONFIG_UART_INTERRUPT_DRIVEN,						\
				(.tx_fifo_threshold = DT_INST_PROP(index, tx_fifo_threshold),))	\
						  IF_ENABLED(					\
				  CONFIG_UART_INTERRUPT_DRIVEN,					\
				  (.irq_config_func = uart_mspm0_##index##_irq_register,))};	\
												\
	static struct uart_mspm0_data uart_mspm0_data_##index = {                               \
		.current_speed = DT_INST_PROP(index, current_speed),                            \
		.uart_config =                                                                  \
			{                                                                       \
				.parity = 0,                                                    \
				.wordLength = UART_LCRH_WLEN_DATABIT8,                          \
				.stopBits = UART_LCRH_STP2_DISABLE,                             \
				.flowControl = (DT_INST_PROP(index, hw_flow_control)            \
							? (UART_CTL0_RTSEN_ENABLE |             \
							   UART_CTL0_CTSEN_ENABLE)              \
							: (UART_CTL0_RTSEN_DISABLE |            \
							   UART_CTL0_CTSEN_DISABLE)),           \
			},                                                                      \
	};                                                                                      \
												\
	DEVICE_DT_INST_DEFINE(index, &uart_mspm0_init, NULL, &uart_mspm0_data_##index,            \
			      &uart_mspm0_cfg_##index, PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY, \
			      &uart_mspm0_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MSPM0_UART_INIT_FN)
