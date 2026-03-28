/*
 * Copyright (c) 2025 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_rza2m_scif_uart

#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/interrupt_controller/gic.h>
#include <zephyr/logging/log.h>
#include <zephyr/irq.h>
#include <zephyr/spinlock.h>

LOG_MODULE_REGISTER(uart_renesas_rza2m_scif, CONFIG_UART_LOG_LEVEL);

struct scif_reg {
	uint8_t offset, size;
};

enum {
	RZA2M_SMR,  /* Serial Mode Register */
	RZA2M_BRR,  /* Bit Rate Register */
	RZA2M_SCR,  /* Serial Control Register */
	RZA2M_FSR,  /* Serial Status Register */
	RZA2M_FCR,  /* FIFO Control Register */
	RZA2M_FDR,  /* FIFO Data Count Register */
	RZA2M_FTDR, /* Transmit (FIFO) Data Register */
	RZA2M_FRDR, /* Receive (FIFO) Data Register */
	RZA2M_LSR,  /* Line Status Register */
	RZA2M_TFDR, /* Transmit FIFO Data Count Register */
	RZA2M_RFDR, /* Receive FIFO Data Count Register */
	RZA2M_SPTR, /* Serial Port Register */
	RZA2M_SEMR, /* Serial extended mode register */
	RZA2M_FTCR, /* FIFO Trigger Control Register */

	RZA2M_SCIF_NR_REGS,
};

struct scif_params {
	const struct scif_reg regs[RZA2M_SCIF_NR_REGS];
	uint16_t init_lsr_mask;
	uint16_t init_interrupt_mask;
};

struct uart_rza2m_scif_cfg {
	DEVICE_MMIO_ROM; /* Must be first */
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	const struct pinctrl_dev_config *pcfg;
	const struct scif_params *params;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	void (*irq_config_func)(const struct device *dev);
#endif
};

struct uart_rza2m_scif_int {
	uint16_t rxi_status;
	uint16_t line_status;
};

struct uart_rza2m_scif_data {
	DEVICE_MMIO_RAM; /* Must be first */
	struct uart_config current_config;
	uint8_t channel;
	uint32_t clk_rate;
	struct k_spinlock lock;
	struct uart_rza2m_scif_int int_data;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t irq_cb;
	void *irq_cb_data;
#endif
};

#define RZA2M_SCIF_DEFAULT_SPEED     115200
#define RZA2M_SCIF_DEFAULT_PARITY    UART_CFG_PARITY_NONE
#define RZA2M_SCIF_DEFAULT_STOP_BITS UART_CFG_STOP_BITS_1
#define RZA2M_SCIF_DEFAULT_DATA_BITS UART_CFG_DATA_BITS_8

/* SMR (Serial Mode Register) */
#define RZA2M_SMR_C_A       BIT(7)            /* Communication Mode */
#define RZA2M_SMR_CHR       BIT(6)            /* 7-bit Character Length */
#define RZA2M_SMR_PE        BIT(5)            /* Parity Enable */
#define RZA2M_SMR_O_E       BIT(4)            /* Odd Parity */
#define RZA2M_SMR_STOP      BIT(3)            /* Stop Bit Length */
#define RZA2M_SMR_CKS_MASK  (BIT(0) | BIT(1)) /* Clock Select */
#define RZA2M_SMR_CKS_SHIFT 0                 /* Clock Select shift */

/* SCR (Serial Control Register) */
#define RZA2M_SCR_TIE  BIT(7) /* Transmit Interrupt Enable */
#define RZA2M_SCR_RIE  BIT(6) /* Receive Interrupt Enable */
#define RZA2M_SCR_TE   BIT(5) /* Transmit Enable */
#define RZA2M_SCR_RE   BIT(4) /* Receive Enable */
#define RZA2M_SCR_REIE BIT(3) /* Receive Error Interrupt Enable */
#define RZA2M_SCR_TEIE BIT(2) /* Transmit End Interrupt Enable */
#define RZA2M_SCR_CKE1 BIT(1) /* Clock Enable 1 */
#define RZA2M_SCR_CKE0 BIT(0) /* Clock Enable 0 */

/* FCR (FIFO Control Register) */
#define RZA2M_FCR_RTRG1 BIT(7) /* Receive FIFO Data Count Trigger 1 */
#define RZA2M_FCR_RTRG0 BIT(6) /* Receive FIFO Data Count Trigger 0 */
#define RZA2M_FCR_TTRG1 BIT(5) /* Transmit FIFO Data Count Trigger 1 */
#define RZA2M_FCR_TTRG0 BIT(4) /* Transmit FIFO Data Count Trigger 0 */
#define RZA2M_FCR_MCE   BIT(3) /* Modem Control Enable */
#define RZA2M_FCR_TFRST BIT(2) /* Transmit FIFO Data Register Reset */
#define RZA2M_FCR_RFRST BIT(1) /* Receive FIFO Data Register Reset */
#define RZA2M_FCR_LOOP  BIT(0) /* Loopback Test */

/* FSR (Serial Status Register) */
#define RZA2M_FSR_PER3 BIT(15) /* Parity Error Count 3 */
#define RZA2M_FSR_PER2 BIT(14) /* Parity Error Count 2 */
#define RZA2M_FSR_PER1 BIT(13) /* Parity Error Count 1 */
#define RZA2M_FSR_PER0 BIT(12) /* Parity Error Count 0 */
#define RZA2M_FSR_FER3 BIT(11) /* Framing Error Count 3 */
#define RZA2M_FSR_FER2 BIT(10) /* Framing Error Count 2 */
#define RZA2M_FSR_FER1 BIT(9)  /* Framing Error Count 1 */
#define RZA2M_FSR_FER0 BIT(8)  /* Framing Error Count 0 */
#define RZA2M_FSR_ER   BIT(7)  /* Receive Error */
#define RZA2M_FSR_TEND BIT(6)  /* Transmission ended */
#define RZA2M_FSR_TDFE BIT(5)  /* Transmit FIFO Data Empty */
#define RZA2M_FSR_BRK  BIT(4)  /* Break Detect */
#define RZA2M_FSR_FER  BIT(3)  /* Framing Error */
#define RZA2M_FSR_PER  BIT(2)  /* Parity Error */
#define RZA2M_FSR_RDF  BIT(1)  /* Receive FIFO Data Full */
#define RZA2M_FSR_DR   BIT(0)  /* Receive Data Ready */

/* SPTR (Serial Port Register) on SCIFA */
#define RZA2M_SPTR_SPB2IO BIT(1) /* Serial Port Break Input/Output */
#define RZA2M_SPTR_SPB2DT BIT(0) /* Serial Port Break Data Select */

/* LSR (Line Status Register) on SCIFA */
#define RZA2M_LSR_TO_SCIFA BIT(2) /* Timeout on SCIFA */
#define RZA2M_LSR_ORER     BIT(0) /* Overrun Error */

/* Serial Extended Mode Register */
#define RZA2M_SEMR_ABCS0 BIT(0) /* Asynchronous Base Clock Select */
#define RZA2M_SEMR_NFEN  BIT(2) /* Noise Cancellation Enable */
#define RZA2M_SEMR_DIR   BIT(3) /* Data Transfer Direction Select */
#define RZA2M_SEMR_MDDRS BIT(4) /* Modulation Duty Register Select */
#define RZA2M_SEMR_BRME  BIT(5) /* Bit Rate Modulation Enable */
/* Baud Rate Generator Double-Speed Mode Select */
#define RZA2M_SEMR_BGDM  BIT(7)

#define RZA2M_SCIF_DRV_ERR (-1)

/* clang-format off */
/* Registers */
static const struct scif_params port_params = {
	.regs = {
			[RZA2M_SMR] = {0x00, 16},
			[RZA2M_BRR] = {0x02, 8},
			[RZA2M_SCR] = {0x04, 16},
			[RZA2M_FTDR] = {0x06, 8},
			[RZA2M_FSR] = {0x08, 16},
			[RZA2M_FRDR] = {0x0A, 8},
			[RZA2M_FCR] = {0x0C, 16},
			[RZA2M_FDR] = {0x0E, 16},
			[RZA2M_SPTR] = {0x10, 16},
			[RZA2M_LSR] = {0x12, 16},
			[RZA2M_SEMR] = {0x14, 8},
			[RZA2M_FTCR] = {0x16, 16},
		},
	.init_lsr_mask = RZA2M_LSR_ORER,
	.init_interrupt_mask =
		RZA2M_SCR_TIE | RZA2M_SCR_RIE | RZA2M_SCR_REIE | RZA2M_SCR_TEIE,
};
/* clang-format on */

#define RZA2M_NUM_DIVISORS_ASYNC (9)

/* Baud divisor info */
/**
 * When ABCS = 0 & BGDM = 0, divisor = 64 x 2^(2n - 1)
 * When ABCS = 1 & BGDM = 0 OR ABCS = 0 & BGDM = 1, divisor = 32 x 2^(2n - 1)
 * When ABCS = 1 & BGDM = 1, divisor = 16 x 2^(2n - 1)
 */

typedef struct {
	int16_t divisor; /* Clock divisor */
	uint8_t abcs;    /* ABCS value to get divisor */
	uint8_t bgdm;    /* BGDM value to get divisor */
	uint8_t cks;     /* CKS value to get divisor (CKS = n) */
} st_baud_divisorb_t;

/* Divisor result, ABCS, BGDM, n */
static const st_baud_divisorb_t gs_scifa_async_baud[RZA2M_NUM_DIVISORS_ASYNC] = {
	{8, 1, 1, 0},   {16, 0, 1, 0},  {32, 0, 0, 0},   {64, 0, 1, 1},  {128, 0, 0, 1},
	{256, 0, 1, 2}, {512, 0, 0, 2}, {1024, 0, 1, 3}, {2048, 0, 0, 3}};

static uint8_t uart_rza2m_scif_read_8(const struct device *dev, uint32_t offs)
{
	const struct uart_rza2m_scif_cfg *config = dev->config;
	uint32_t offset = config->params->regs[offs].offset;

	return sys_read8(DEVICE_MMIO_GET(dev) + offset);
}

static void uart_rza2m_scif_write_8(const struct device *dev, uint32_t offs, uint8_t value)
{
	const struct uart_rza2m_scif_cfg *config = dev->config;
	uint32_t offset = config->params->regs[offs].offset;

	sys_write8(value, DEVICE_MMIO_GET(dev) + offset);
}

static uint16_t uart_rza2m_scif_read_16(const struct device *dev, uint32_t offs)
{
	const struct uart_rza2m_scif_cfg *config = dev->config;
	uint32_t offset = config->params->regs[offs].offset;

	return sys_read16(DEVICE_MMIO_GET(dev) + offset);
}

static void uart_rza2m_scif_write_16(const struct device *dev, uint32_t offs, uint16_t value)
{
	const struct uart_rza2m_scif_cfg *config = dev->config;
	uint32_t offset = config->params->regs[offs].offset;

	sys_write16(value, DEVICE_MMIO_GET(dev) + offset);
}

static uint8_t find_divisor_index(uint8_t channel, uint32_t desired_baud_rate, uint32_t clock_freq)
{
	/* Find the divisor; table has associated ABCS, BGDM and CKS values */
	/* BRR must be 255 or less */
	/* BRR = (PCLK / (divisor * desired_baud)) - 1 */
	/* BRR = (ratio / divisor) - 1 */

	uint32_t ratio = clock_freq / desired_baud_rate;
	uint8_t divisor_index = 0;

	if (channel == 0) {
		/* The hardware manual states that for channel 0, P1f/16 input is not provided */
		/* So the setting CKS [1:0] 1 0 cannot be used */
		/* This restriction may be lifted in future releases */

		while ((divisor_index < RZA2M_NUM_DIVISORS_ASYNC) &&
		       ((ratio >= (uint32_t)(gs_scifa_async_baud[divisor_index].divisor * 256)) ||
			(2 == gs_scifa_async_baud[divisor_index].cks))) {
			divisor_index++;
		}
	} else {
		/* Cast to uint32_t for comparison with ratio */
		while ((divisor_index < RZA2M_NUM_DIVISORS_ASYNC) &&
		       (ratio >= (uint32_t)(gs_scifa_async_baud[divisor_index].divisor * 256))) {
			divisor_index++;
		}
	}

	return divisor_index;
}

static int uart_rza2m_scif_set_baudrate(const struct device *dev, uint8_t channel,
					uint32_t baud_rate)
{
	struct uart_rza2m_scif_data *data = dev->data;
	uint16_t reg_16;
	uint8_t reg_8;
	uint32_t clk_freq = data->clk_rate;
	uint8_t divisor_index;
	uint32_t divisor;
	uint32_t brr;

	divisor_index = find_divisor_index(channel, baud_rate, clk_freq);
	divisor = (uint32_t)gs_scifa_async_baud[divisor_index].divisor;

	brr = clk_freq / (divisor * baud_rate);

	if (brr == 0) {
		return RZA2M_SCIF_DRV_ERR; /* Illegal value; return error */
	}

	/* Divide by half the divisor */
	brr = clk_freq / ((divisor * baud_rate) / 2);

	/* Formula: BRR = (PCLK / (divisor * desired_baud)) - 1 */
	/* If (BRR * 2) is odd, "round up" by ignoring -1; divide by 2 again for rest of divisor */
	brr = ((brr & 0x01) ? (brr / 2) : ((brr / 2) - 1));

	/* Write BRR */
	uart_rza2m_scif_write_8(dev, RZA2M_BRR, brr);

	/* Write CKS[1:0] */
	reg_16 = uart_rza2m_scif_read_16(dev, RZA2M_SMR);
	reg_16 &= ~(RZA2M_SMR_CKS_MASK << RZA2M_SMR_CKS_SHIFT);
	reg_16 |= (gs_scifa_async_baud[divisor_index].cks & RZA2M_SMR_CKS_MASK)
		  << RZA2M_SMR_CKS_SHIFT;
	uart_rza2m_scif_write_16(dev, RZA2M_SMR, reg_16);

	/* Write ABCS0 and BGDM */
	reg_8 = uart_rza2m_scif_read_8(dev, RZA2M_SEMR);
	reg_8 = reg_8 | (gs_scifa_async_baud[divisor_index].abcs ? RZA2M_SEMR_ABCS0 : 0);
	reg_8 = reg_8 | (gs_scifa_async_baud[divisor_index].bgdm ? RZA2M_SEMR_BGDM : 0);
	uart_rza2m_scif_write_8(dev, RZA2M_SEMR, reg_8);

	return 0;
}

static int uart_rza2m_scif_poll_in(const struct device *dev, unsigned char *p_char)
{
	struct uart_rza2m_scif_data *data = dev->data;
	uint16_t reg_16;
	int ret = 0;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	/* Receive FIFO empty */
	if (!(uart_rza2m_scif_read_16(dev, RZA2M_FSR) & RZA2M_FSR_RDF)) {
		ret = -1;
		goto unlock;
	}

	*p_char = uart_rza2m_scif_read_8(dev, RZA2M_FRDR);

	reg_16 = uart_rza2m_scif_read_16(dev, RZA2M_FSR);
	reg_16 &= ~RZA2M_FSR_RDF;
	uart_rza2m_scif_write_16(dev, RZA2M_FSR, reg_16);

unlock:
	k_spin_unlock(&data->lock, key);

	return ret;
}

static void uart_rza2m_scif_poll_out(const struct device *dev, unsigned char out_char)
{
	struct uart_rza2m_scif_data *data = dev->data;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	/* Wait for empty space in transmit FIFO */
	while (!(uart_rza2m_scif_read_16(dev, RZA2M_FSR) & RZA2M_FSR_TDFE)) {
	}

	uart_rza2m_scif_write_8(dev, RZA2M_FTDR, out_char);

	while (!(uart_rza2m_scif_read_16(dev, RZA2M_FSR) & RZA2M_FSR_TEND)) {
	}

	k_spin_unlock(&data->lock, key);
}

static int uart_rza2m_scif_err_check(const struct device *dev)
{
	struct uart_rza2m_scif_data *data = dev->data;
	struct uart_rza2m_scif_int int_data = data->int_data;
	int err = 0;

	if (int_data.line_status & RZA2M_LSR_ORER) {
		err |= UART_ERROR_OVERRUN;
	}
	if (int_data.rxi_status & RZA2M_FSR_FER) {
		err |= UART_ERROR_FRAMING;
	}
	if (int_data.rxi_status & RZA2M_FSR_PER) {
		err |= UART_ERROR_PARITY;
	}
	if (int_data.rxi_status & RZA2M_FSR_BRK) {
		err |= UART_BREAK;
	}

	return err;
}

static int uart_rza2m_scif_configure(const struct device *dev, const struct uart_config *cfg)
{
	const struct uart_rza2m_scif_cfg *config = dev->config;
	struct uart_rza2m_scif_data *data = dev->data;
	uint16_t reg_16;
	k_spinlock_key_t key;
	int err;

	err = 0;

	if (cfg->data_bits < UART_CFG_DATA_BITS_7 || cfg->data_bits > UART_CFG_DATA_BITS_8 ||
	    cfg->stop_bits == UART_CFG_STOP_BITS_0_5 || cfg->stop_bits == UART_CFG_STOP_BITS_1_5 ||
	    cfg->flow_ctrl != UART_CFG_FLOW_CTRL_NONE) {
		return -ENOTSUP;
	}

	key = k_spin_lock(&data->lock);

	/* Set the TXD output high */
	reg_16 = uart_rza2m_scif_read_16(dev, RZA2M_SPTR);
	reg_16 |= (RZA2M_SPTR_SPB2DT | RZA2M_SPTR_SPB2IO);
	uart_rza2m_scif_write_16(dev, RZA2M_SPTR, reg_16);

	/* Disable Transmit and Receive */
	reg_16 = uart_rza2m_scif_read_16(dev, RZA2M_SCR);
	reg_16 &= ~(RZA2M_SCR_TE | RZA2M_SCR_RE | RZA2M_SCR_TIE | RZA2M_SCR_RIE);
	reg_16 &= ~(RZA2M_SCR_TEIE);
	uart_rza2m_scif_write_16(dev, RZA2M_SCR, reg_16);

	/* Emptying Transmit and Receive FIFO */
	reg_16 = uart_rza2m_scif_read_16(dev, RZA2M_FCR);
	reg_16 |= (RZA2M_FCR_TFRST | RZA2M_FCR_RFRST);
	uart_rza2m_scif_write_16(dev, RZA2M_FCR, reg_16);

	/* Resetting Errors Registers */
	reg_16 = uart_rza2m_scif_read_16(dev, RZA2M_FSR);
	reg_16 &= ~(RZA2M_FSR_ER | RZA2M_FSR_DR | RZA2M_FSR_BRK | RZA2M_FSR_RDF);
	uart_rza2m_scif_write_16(dev, RZA2M_FSR, reg_16);

	reg_16 = uart_rza2m_scif_read_16(dev, RZA2M_LSR);
	reg_16 &= ~(config->params->init_lsr_mask);
	uart_rza2m_scif_write_16(dev, RZA2M_LSR, reg_16);

	/* Select internal clock */
	reg_16 = uart_rza2m_scif_read_16(dev, RZA2M_SCR);
	reg_16 &= ~(RZA2M_SCR_CKE1 | RZA2M_SCR_CKE0);
	uart_rza2m_scif_write_16(dev, RZA2M_SCR, reg_16);

	/* Serial Configuration (8N1) & Clock divider selection */
	reg_16 = uart_rza2m_scif_read_16(dev, RZA2M_SMR);
	reg_16 &= ~(RZA2M_SMR_C_A | RZA2M_SMR_CHR | RZA2M_SMR_PE | RZA2M_SMR_O_E | RZA2M_SMR_STOP);
	switch (cfg->parity) {
	case UART_CFG_PARITY_NONE:
		break;
	case UART_CFG_PARITY_ODD:
		reg_16 |= RZA2M_SMR_PE | RZA2M_SMR_O_E;
		break;
	case UART_CFG_PARITY_EVEN:
		reg_16 |= RZA2M_SMR_PE;
		break;
	default:
		return -ENOTSUP;
	}
	if (cfg->stop_bits == UART_CFG_STOP_BITS_2) {
		reg_16 |= RZA2M_SMR_STOP;
	}
	if (cfg->data_bits == UART_CFG_DATA_BITS_7) {
		reg_16 |= RZA2M_SMR_CHR;
	}
	uart_rza2m_scif_write_16(dev, RZA2M_SMR, reg_16);

	/* Set baudrate */
	err = uart_rza2m_scif_set_baudrate(dev, data->channel, cfg->baudrate);
	if (err) {
		return -EIO;
	}

	/* FIFOs data count trigger configuration */
	reg_16 = uart_rza2m_scif_read_16(dev, RZA2M_FCR);
	reg_16 &= ~(RZA2M_FCR_RTRG1 | RZA2M_FCR_RTRG0 | RZA2M_FCR_TTRG1 | RZA2M_FCR_TTRG0 |
		    RZA2M_FCR_MCE | RZA2M_FCR_TFRST | RZA2M_FCR_RFRST);
	uart_rza2m_scif_write_16(dev, RZA2M_FCR, reg_16);

	/* Enable Transmit & Receive + disable Interrupts */
	reg_16 = uart_rza2m_scif_read_16(dev, RZA2M_SCR);
	reg_16 |= (RZA2M_SCR_TE | RZA2M_SCR_RE);
	reg_16 &= ~(config->params->init_interrupt_mask);
	uart_rza2m_scif_write_16(dev, RZA2M_SCR, reg_16);

	data->current_config = *cfg;

	k_spin_unlock(&data->lock, key);

	return err;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int uart_rza2m_scif_config_get(const struct device *dev, struct uart_config *cfg)
{
	struct uart_rza2m_scif_data *data = dev->data;

	*cfg = data->current_config;

	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

static int uart_rza2m_scif_init(const struct device *dev)
{
	const struct uart_rza2m_scif_cfg *config = dev->config;
	struct uart_rza2m_scif_data *data = dev->data;
	int ret;

	/* Configure dt provided device signals when available */
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	if (!device_is_ready(config->clock_dev)) {
		return -ENODEV;
	}

	ret = clock_control_on(config->clock_dev, (clock_control_subsys_t)config->clock_subsys);
	if (ret < 0) {
		return ret;
	}

	ret = clock_control_get_rate(config->clock_dev,
				     (clock_control_subsys_t)config->clock_subsys, &data->clk_rate);
	if (ret < 0) {
		return ret;
	}

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	ret = uart_rza2m_scif_configure(dev, &data->current_config);
	if (ret != 0) {
		return ret;
	}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	config->irq_config_func(dev);
#endif

	return 0;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

static bool uart_rza2m_scif_irq_is_enabled(const struct device *dev, uint32_t irq)
{
	return !!(uart_rza2m_scif_read_16(dev, RZA2M_SCR) & irq);
}

static int uart_rza2m_scif_fifo_fill(const struct device *dev, const uint8_t *tx_data, int len)
{
	struct uart_rza2m_scif_data *data = dev->data;
	int num_tx = 0;
	uint16_t reg_16;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	while (((len - num_tx) > 0) && (uart_rza2m_scif_read_16(dev, RZA2M_FSR) & RZA2M_FSR_TDFE)) {
		/* Send current byte */
		uart_rza2m_scif_write_8(dev, RZA2M_FTDR, tx_data[num_tx]);

		reg_16 = uart_rza2m_scif_read_16(dev, RZA2M_FSR);
		reg_16 &= ~(RZA2M_FSR_TDFE | RZA2M_FSR_TEND);
		uart_rza2m_scif_write_16(dev, RZA2M_FSR, reg_16);

		num_tx++;
	}

	k_spin_unlock(&data->lock, key);

	return num_tx;
}

static int uart_rza2m_scif_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	struct uart_rza2m_scif_data *data = dev->data;
	int num_rx = 0;
	uint16_t reg_16;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	while (((size - num_rx) > 0) && (uart_rza2m_scif_read_16(dev, RZA2M_FSR) & RZA2M_FSR_RDF)) {
		/* Receive current byte */
		rx_data[num_rx++] = uart_rza2m_scif_read_8(dev, RZA2M_FRDR);

		reg_16 = uart_rza2m_scif_read_16(dev, RZA2M_FSR);
		reg_16 &= ~(RZA2M_FSR_RDF);
		uart_rza2m_scif_write_16(dev, RZA2M_FSR, reg_16);
	}

	k_spin_unlock(&data->lock, key);

	return num_rx;
}

static void uart_rza2m_scif_irq_tx_enable(const struct device *dev)
{
	struct uart_rza2m_scif_data *data = dev->data;

	uint16_t reg_16;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	reg_16 = uart_rza2m_scif_read_16(dev, RZA2M_SCR);
	reg_16 |= (RZA2M_SCR_TIE);
	uart_rza2m_scif_write_16(dev, RZA2M_SCR, reg_16);

	k_spin_unlock(&data->lock, key);
}

static void uart_rza2m_scif_irq_tx_disable(const struct device *dev)
{
	struct uart_rza2m_scif_data *data = dev->data;

	uint16_t reg_16;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	reg_16 = uart_rza2m_scif_read_16(dev, RZA2M_SCR);
	reg_16 &= ~(RZA2M_SCR_TIE);
	uart_rza2m_scif_write_16(dev, RZA2M_SCR, reg_16);

	k_spin_unlock(&data->lock, key);
}

static int uart_rza2m_scif_irq_tx_ready(const struct device *dev)
{
	return !!(uart_rza2m_scif_read_16(dev, RZA2M_FSR) & RZA2M_FSR_TDFE);
}

static void uart_rza2m_scif_irq_rx_enable(const struct device *dev)
{
	struct uart_rza2m_scif_data *data = dev->data;

	uint16_t reg_16;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	reg_16 = uart_rza2m_scif_read_16(dev, RZA2M_SCR);
	reg_16 |= (RZA2M_SCR_RIE);
	uart_rza2m_scif_write_16(dev, RZA2M_SCR, reg_16);

	k_spin_unlock(&data->lock, key);
}

static void uart_rza2m_scif_irq_rx_disable(const struct device *dev)
{
	struct uart_rza2m_scif_data *data = dev->data;

	uint16_t reg_16;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	reg_16 = uart_rza2m_scif_read_16(dev, RZA2M_SCR);
	reg_16 &= ~(RZA2M_SCR_RIE);
	uart_rza2m_scif_write_16(dev, RZA2M_SCR, reg_16);

	k_spin_unlock(&data->lock, key);
}

static int uart_rza2m_scif_irq_rx_ready(const struct device *dev)
{
	return !!(uart_rza2m_scif_read_16(dev, RZA2M_FSR) & RZA2M_FSR_RDF);
}

static void uart_rza2m_scif_irq_err_enable(const struct device *dev)
{
	struct uart_rza2m_scif_data *data = dev->data;

	uint16_t reg_16;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	reg_16 = uart_rza2m_scif_read_16(dev, RZA2M_SCR);
	reg_16 |= (RZA2M_SCR_REIE);
	uart_rza2m_scif_write_16(dev, RZA2M_SCR, reg_16);

	k_spin_unlock(&data->lock, key);
}

static void uart_rza2m_scif_irq_err_disable(const struct device *dev)
{
	struct uart_rza2m_scif_data *data = dev->data;

	uint16_t reg_16;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	reg_16 = uart_rza2m_scif_read_16(dev, RZA2M_SCR);
	reg_16 &= ~(RZA2M_SCR_REIE);
	uart_rza2m_scif_write_16(dev, RZA2M_SCR, reg_16);

	k_spin_unlock(&data->lock, key);
}

static int uart_rza2m_scif_irq_is_pending(const struct device *dev)
{
	return (uart_rza2m_scif_irq_rx_ready(dev) &&
		uart_rza2m_scif_irq_is_enabled(dev, RZA2M_SCR_RIE)) ||
	       (uart_rza2m_scif_irq_tx_ready(dev) &&
		uart_rza2m_scif_irq_is_enabled(dev, RZA2M_SCR_TIE));
}

static int uart_rza2m_scif_irq_update(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 1;
}

static void uart_rza2m_scif_irq_callback_set(const struct device *dev,
					     uart_irq_callback_user_data_t cb, void *cb_data)
{
	struct uart_rza2m_scif_data *data = dev->data;

	data->irq_cb = cb;
	data->irq_cb_data = cb_data;
}

void uart_rza2m_scif_isr(const struct device *dev)
{
	struct uart_rza2m_scif_data *data = dev->data;

	if (data->irq_cb) {
		data->irq_cb(dev, data->irq_cb_data);
	}

	/* Get interrupt status */
	data->int_data.rxi_status = uart_rza2m_scif_read_16(dev, RZA2M_FSR);
	data->int_data.rxi_status &= (RZA2M_FSR_FER | RZA2M_FSR_PER | RZA2M_FSR_BRK);
	data->int_data.line_status = uart_rza2m_scif_read_16(dev, RZA2M_LSR);
	data->int_data.line_status &= RZA2M_LSR_ORER;
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static DEVICE_API(uart, uart_rza2m_scif_driver_api) = {
	.poll_in = uart_rza2m_scif_poll_in,
	.poll_out = uart_rza2m_scif_poll_out,
	.err_check = uart_rza2m_scif_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = uart_rza2m_scif_configure,
	.config_get = uart_rza2m_scif_config_get,
#endif
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_rza2m_scif_fifo_fill,
	.fifo_read = uart_rza2m_scif_fifo_read,
	.irq_tx_enable = uart_rza2m_scif_irq_tx_enable,
	.irq_tx_disable = uart_rza2m_scif_irq_tx_disable,
	.irq_tx_ready = uart_rza2m_scif_irq_tx_ready,
	.irq_rx_enable = uart_rza2m_scif_irq_rx_enable,
	.irq_rx_disable = uart_rza2m_scif_irq_rx_disable,
	.irq_rx_ready = uart_rza2m_scif_irq_rx_ready,
	.irq_err_enable = uart_rza2m_scif_irq_err_enable,
	.irq_err_disable = uart_rza2m_scif_irq_err_disable,
	.irq_is_pending = uart_rza2m_scif_irq_is_pending,
	.irq_update = uart_rza2m_scif_irq_update,
	.irq_callback_set = uart_rza2m_scif_irq_callback_set,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

/* Device Instantiation */
#define UART_RZA2M_DECLARE_CFG(n, IRQ_FUNC_INIT)                                                   \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
	uint32_t clock_subsys##n = DT_INST_CLOCKS_CELL(n, clk_id);                                 \
	static const struct uart_rza2m_scif_cfg uart_rza2m_scif_cfg_##n = {                        \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),                                              \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),                                \
		.clock_subsys = (clock_control_subsys_t)(&clock_subsys##n),                        \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.params = &port_params,                                                            \
		IRQ_FUNC_INIT}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define UART_RZA2M_SET_IRQ(n, name)                                                                \
	IRQ_CONNECT(DT_INST_IRQ_BY_NAME(n, name, irq) - GIC_SPI_INT_BASE,                          \
		    DT_INST_IRQ_BY_NAME(n, name, priority), uart_rza2m_scif_isr,                   \
		    DEVICE_DT_INST_GET(n), 0);                                                     \
                                                                                                   \
	irq_enable(DT_INST_IRQ_BY_NAME(n, name, irq) - GIC_SPI_INT_BASE);

#define UART_RZA2M_IRQ_CONFIG_FUNC(n)                                                              \
	static void irq_config_func_##n(const struct device *dev)                                  \
	{                                                                                          \
		UART_RZA2M_SET_IRQ(n, eri);                                                        \
		UART_RZA2M_SET_IRQ(n, rxi);                                                        \
		UART_RZA2M_SET_IRQ(n, txi);                                                        \
		UART_RZA2M_SET_IRQ(n, tei);                                                        \
	}
#define UART_RZA2M_IRQ_CFG_FUNC_INIT(n) .irq_config_func = irq_config_func_##n
#define UART_RZA2M_INIT_CFG(n)          UART_RZA2M_DECLARE_CFG(n, UART_RZA2M_IRQ_CFG_FUNC_INIT(n))
#else
#define UART_RZA2M_IRQ_CONFIG_FUNC(n)
#define UART_RZA2M_IRQ_CFG_FUNC_INIT
#define UART_RZA2M_INIT_CFG(n) UART_RZA2M_DECLARE_CFG(n, UART_RZA2M_IRQ_CFG_FUNC_INIT)
#endif

#define UART_RZA2M_INIT(n)                                                                         \
	static struct uart_rza2m_scif_data uart_rza2m_scif_data_##n = {                            \
		.current_config =                                                                  \
			{                                                                          \
				.baudrate = DT_INST_PROP_OR(n, current_speed,                      \
							    RZA2M_SCIF_DEFAULT_SPEED),             \
				.parity =                                                          \
					DT_INST_ENUM_IDX_OR(n, parity, RZA2M_SCIF_DEFAULT_PARITY), \
				.stop_bits = DT_INST_ENUM_IDX_OR(n, stop_bits,                     \
								 RZA2M_SCIF_DEFAULT_STOP_BITS),    \
				.data_bits = DT_INST_ENUM_IDX_OR(n, data_bits,                     \
								 RZA2M_SCIF_DEFAULT_DATA_BITS),    \
				.flow_ctrl = UART_CFG_FLOW_CTRL_NONE,                              \
			},                                                                         \
		.channel = DT_INST_PROP(n, channel),                                               \
	};                                                                                         \
	static const struct uart_rza2m_scif_cfg uart_rza2m_scif_cfg_##n;                           \
	DEVICE_DT_INST_DEFINE(n, uart_rza2m_scif_init, NULL, &uart_rza2m_scif_data_##n,            \
			      &uart_rza2m_scif_cfg_##n, PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY, \
			      &uart_rza2m_scif_driver_api);                                        \
                                                                                                   \
	UART_RZA2M_IRQ_CONFIG_FUNC(n)                                                              \
	UART_RZA2M_INIT_CFG(n);

DT_INST_FOREACH_STATUS_OKAY(UART_RZA2M_INIT)
