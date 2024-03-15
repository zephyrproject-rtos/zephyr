/*
 * Copyright (c) 2023 TOKITA Hiroshi <tokita.hiroshi@fujitsu.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT renesas_ra_uart_sci

#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/interrupt_controller/intc_ra_icu.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/irq.h>
#include <zephyr/spinlock.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(ra_uart_sci, CONFIG_UART_LOG_LEVEL);

enum {
	UART_RA_INT_RXI,
	UART_RA_INT_TXI,
	UART_RA_INT_ERI,
	NUM_OF_UART_RA_INT,
};

struct uart_ra_cfg {
	mem_addr_t regs;
	const struct device *clock_dev;
	clock_control_subsys_t clock_id;
	const struct pinctrl_dev_config *pcfg;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	int (*irq_config_func)(const struct device *dev);
#endif
};

struct uart_ra_data {
	struct uart_config current_config;
	uint32_t clk_rate;
	struct k_spinlock lock;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uint32_t irqn[NUM_OF_UART_RA_INT];
	uart_irq_callback_user_data_t callback;
	void *cb_data;
#endif
};

#define REG_MASK(reg) (BIT_MASK(_CONCAT(reg, _LEN)) << _CONCAT(reg, _POS))

/* Registers */
#define SMR  0x00 /*!< Serial Mode Register             */
#define BRR  0x01 /*!< Bit Rate Register                */
#define SCR  0x02 /*!< Serial Control Register          */
#define TDR  0x03 /*!< Transmit Data Register           */
#define SSR  0x04 /*!< Serial Status Register           */
#define RDR  0x05 /*!< Receive Data Register            */
#define SEMR 0x07 /*!< Serial Extended Mode Register    */
#define MDDR 0x12 /*!< Modulation Duty Register         */
#define LSR  0x18 /*!< Line Status Register             */

/*
 * SMR (Serial Mode Register)
 *
 * - CKS[0..2]:  Clock Select
 * - MP[2..3]:   Multi-Processor Mode(Valid only in asynchronous mode)
 * - STOP[3..4]: Stop Bit Length(Valid only in asynchronous mode)
 * - PM[4..5]:   Parity Mode (Valid only when the PE bit is 1)
 * - PE[5..6]:   Parity Enable(Valid only in asynchronous mode)
 * - CHR[6..7]:  Character Length(Valid only in asynchronous mode)
 * - CM[7..8]:   Communication Mode
 */
#define SMR_CKS_POS  (0)
#define SMR_CKS_LEN  (2)
#define SMR_MP_POS   (2)
#define SMR_MP_LEN   (1)
#define SMR_STOP_POS (3)
#define SMR_STOP_LEN (1)
#define SMR_PM_POS   (4)
#define SMR_PM_LEN   (1)
#define SMR_PE_POS   (5)
#define SMR_PE_LEN   (1)
#define SMR_CHR_POS  (6)
#define SMR_CHR_LEN  (1)
#define SMR_CM_POS   (7)
#define SMR_CM_LEN   (1)

/**
 * SCR (Serial Control Register)
 *
 * - CKE[0..2]:  Clock Enable
 * - TEIE[2..3]: Transmit End Interrupt Enable
 * - MPIE[3..4]: Multi-Processor Interrupt Enable (Valid in asynchronous
 * - RE[4..5]:   Receive Enable
 * - TE[5..6]:   Transmit Enable
 * - RIE[6..7]:  Receive Interrupt Enable
 * - TIE[7..8]:  Transmit Interrupt Enable
 */
#define SCR_CKE_POS  (0)
#define SCR_CKE_LEN  (2)
#define SCR_TEIE_POS (2)
#define SCR_TEIE_LEN (1)
#define SCR_MPIE_POS (3)
#define SCR_MPIE_LEN (1)
#define SCR_RE_POS   (4)
#define SCR_RE_LEN   (1)
#define SCR_TE_POS   (5)
#define SCR_TE_LEN   (1)
#define SCR_RIE_POS  (6)
#define SCR_RIE_LEN  (1)
#define SCR_TIE_POS  (7)
#define SCR_TIE_LEN  (1)

/**
 * SSR (Serial Status Register)
 *
 * - MPBT[0..1]: Multi-Processor Bit Transfer
 * - MPB[1..2]:  Multi-Processor
 * - TEND[2..3]: Transmit End Flag
 * - PER[3..4]:  Parity Error Flag
 * - FER[4..5]:  Framing Error Flag
 * - ORER[5..6]: Overrun Error Flag
 * - RDRF[6..7]: Receive Data Full Flag
 * - TDRE[7..8]: Transmit Data Empty Flag
 */
#define SSR_MPBT_POS (0)
#define SSR_MPBT_LEN (1)
#define SSR_MPB_POS  (1)
#define SSR_MPB_LEN  (1)
#define SSR_TEND_POS (2)
#define SSR_TEND_LEN (1)
#define SSR_PER_POS  (3)
#define SSR_PER_LEN  (1)
#define SSR_FER_POS  (4)
#define SSR_FER_LEN  (1)
#define SSR_ORER_POS (5)
#define SSR_ORER_LEN (1)
#define SSR_RDRF_POS (6)
#define SSR_RDRF_LEN (1)
#define SSR_TDRE_POS (7)
#define SSR_TDRE_LEN (1)

/**
 * SEMR (Serial Extended Mode Register)
 *
 * - ACS0[0..1]:    Asynchronous Mode Clock Source Select
 * - PADIS[1..2]:   Preamble function Disable
 * - BRME[2..3]:    Bit Rate Modulation Enable
 * - ABCSE[3..4]:   Asynchronous Mode Extended Base Clock Select
 * - ABCS[4..5]:    Asynchronous Mode Base Clock Select
 * - NFEN[5..6]:    Digital Noise Filter Function Enable
 * - BGDM[6..7]:    Baud Rate Generator Double-Speed Mode Select
 * - RXDESEL[7..8]: Asynchronous Start Bit Edge Detection Select
 */
#define SEMR_ACS0_POS    (0)
#define SEMR_ACS0_LEN    (1)
#define SEMR_PADIS_POS   (1)
#define SEMR_PADIS_LEN   (1)
#define SEMR_BRME_POS    (2)
#define SEMR_BRME_LEN    (1)
#define SEMR_ABCSE_POS   (3)
#define SEMR_ABCSE_LEN   (1)
#define SEMR_ABCS_POS    (4)
#define SEMR_ABCS_LEN    (1)
#define SEMR_NFEN_POS    (5)
#define SEMR_NFEN_LEN    (1)
#define SEMR_BGDM_POS    (6)
#define SEMR_BGDM_LEN    (1)
#define SEMR_RXDESEL_POS (7)
#define SEMR_RXDESEL_LEN (1)

/**
 * LSR (Line Status Register)
 *
 * - ORER[0..1]:  Overrun Error Flag
 * - FNUM[2..7]:  Framing Error Count
 * - PNUM[8..13]: Parity Error Count
 */
#define LSR_ORER_POS (0)
#define LSR_ORER_LEN (1)
#define LSR_FNUM_POS (2)
#define LSR_FNUM_LEN (5)
#define LSR_PNUM_POS (8)
#define LSR_PNUM_LEN (5)

static uint8_t uart_ra_read_8(const struct device *dev, uint32_t offs)
{
	const struct uart_ra_cfg *config = dev->config;

	return sys_read8(config->regs + offs);
}

static void uart_ra_write_8(const struct device *dev, uint32_t offs, uint8_t value)
{
	const struct uart_ra_cfg *config = dev->config;

	sys_write8(value, config->regs + offs);
}

static uint16_t uart_ra_read_16(const struct device *dev, uint32_t offs)
{
	const struct uart_ra_cfg *config = dev->config;

	return sys_read16(config->regs + offs);
}

static void uart_ra_write_16(const struct device *dev, uint32_t offs, uint16_t value)
{
	const struct uart_ra_cfg *config = dev->config;

	sys_write16(value, config->regs + offs);
}

static void uart_ra_set_baudrate(const struct device *dev, uint32_t baud_rate)
{
	struct uart_ra_data *data = dev->data;
	uint8_t reg_val;

	reg_val = uart_ra_read_8(dev, SEMR);
	reg_val |= (REG_MASK(SEMR_BGDM) | REG_MASK(SEMR_ABCS));
	reg_val &= ~(REG_MASK(SEMR_BRME) | REG_MASK(SEMR_ABCSE));
	uart_ra_write_8(dev, SEMR, reg_val);

	reg_val = (data->clk_rate / (8 * data->current_config.baudrate)) - 1;
	uart_ra_write_8(dev, BRR, reg_val);
}

static int uart_ra_poll_in(const struct device *dev, unsigned char *p_char)
{
	struct uart_ra_data *data = dev->data;
	int ret = 0;

	k_spinlock_key_t key = k_spin_lock(&data->lock);

	/* If interrupts are enabled, return -EINVAL */
	if ((uart_ra_read_8(dev, SCR) & REG_MASK(SCR_RIE))) {
		ret = -EINVAL;
		goto unlock;
	}

	if ((uart_ra_read_8(dev, SSR) & REG_MASK(SSR_RDRF)) == 0) {
		ret = -1;
		goto unlock;
	}

	*p_char = uart_ra_read_8(dev, RDR);
unlock:
	k_spin_unlock(&data->lock, key);

	return ret;
}

static void uart_ra_poll_out(const struct device *dev, unsigned char out_char)
{
	struct uart_ra_data *data = dev->data;
	uint8_t reg_val;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	while (!(uart_ra_read_8(dev, SSR) & REG_MASK(SSR_TEND)) ||
	       !(uart_ra_read_8(dev, SSR) & REG_MASK(SSR_TDRE))) {
		;
	}

	/* If interrupts are enabled, temporarily disable them */
	reg_val = uart_ra_read_8(dev, SCR);
	uart_ra_write_8(dev, SCR, reg_val & ~REG_MASK(SCR_TIE));

	uart_ra_write_8(dev, TDR, out_char);
	while (!(uart_ra_read_8(dev, SSR) & REG_MASK(SSR_TEND)) ||
	       !(uart_ra_read_8(dev, SSR) & REG_MASK(SSR_TDRE))) {
		;
	}

	uart_ra_write_8(dev, SCR, reg_val);
	k_spin_unlock(&data->lock, key);
}

static int uart_ra_err_check(const struct device *dev)
{
	struct uart_ra_data *data = dev->data;

	uint8_t reg_val;
	int errors = 0;
	k_spinlock_key_t key;

	key = k_spin_lock(&data->lock);
	reg_val = uart_ra_read_8(dev, SSR);

	if (reg_val & REG_MASK(SSR_PER)) {
		errors |= UART_ERROR_PARITY;
	}

	if (reg_val & REG_MASK(SSR_FER)) {
		errors |= UART_ERROR_FRAMING;
	}

	if (reg_val & REG_MASK(SSR_ORER)) {
		errors |= UART_ERROR_OVERRUN;
	}

	reg_val &= ~(REG_MASK(SSR_PER) | REG_MASK(SSR_FER) | REG_MASK(SSR_ORER));
	uart_ra_write_8(dev, SSR, reg_val);

	k_spin_unlock(&data->lock, key);

	return errors;
}

static int uart_ra_configure(const struct device *dev, const struct uart_config *cfg)
{
	struct uart_ra_data *data = dev->data;

	uint16_t reg_val;
	k_spinlock_key_t key;

	if (cfg->parity != UART_CFG_PARITY_NONE || cfg->stop_bits != UART_CFG_STOP_BITS_1 ||
	    cfg->data_bits != UART_CFG_DATA_BITS_8 || cfg->flow_ctrl != UART_CFG_FLOW_CTRL_NONE) {
		return -ENOTSUP;
	}

	key = k_spin_lock(&data->lock);

	/* Disable Transmit and Receive */
	reg_val = uart_ra_read_8(dev, SCR);
	reg_val &= ~(REG_MASK(SCR_TE) | REG_MASK(SCR_RE));
	uart_ra_write_8(dev, SCR, reg_val);

	/* Resetting Errors Registers */
	reg_val = uart_ra_read_8(dev, SSR);
	reg_val &= ~(REG_MASK(SSR_PER) | REG_MASK(SSR_FER) | REG_MASK(SSR_ORER) |
		     REG_MASK(SSR_RDRF) | REG_MASK(SSR_TDRE));
	uart_ra_write_8(dev, SSR, reg_val);

	reg_val = uart_ra_read_16(dev, LSR);
	reg_val &= ~(REG_MASK(LSR_ORER));
	uart_ra_write_16(dev, LSR, reg_val);

	/* Select internal clock */
	reg_val = uart_ra_read_8(dev, SCR);
	reg_val &= ~(REG_MASK(SCR_CKE));
	uart_ra_write_8(dev, SCR, reg_val);

	/* Serial Configuration (8N1) & Clock divider selection */
	reg_val = uart_ra_read_8(dev, SMR);
	reg_val &= ~(REG_MASK(SMR_CM) | REG_MASK(SMR_CHR) | REG_MASK(SMR_PE) | REG_MASK(SMR_PM) |
		     REG_MASK(SMR_STOP) | REG_MASK(SMR_CKS));
	uart_ra_write_8(dev, SMR, reg_val);

	/* Set baudrate */
	uart_ra_set_baudrate(dev, cfg->baudrate);

	/* Enable Transmit & Receive + disable Interrupts */
	reg_val = uart_ra_read_8(dev, SCR);
	reg_val |= (REG_MASK(SCR_TE) | REG_MASK(SCR_RE));
	reg_val &=
		~(REG_MASK(SCR_TIE) | REG_MASK(SCR_RIE) | REG_MASK(SCR_MPIE) | REG_MASK(SCR_TEIE));
	uart_ra_write_8(dev, SCR, reg_val);

	data->current_config = *cfg;

	k_spin_unlock(&data->lock, key);

	return 0;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int uart_ra_config_get(const struct device *dev, struct uart_config *cfg)
{
	struct uart_ra_data *data = dev->data;

	*cfg = data->current_config;

	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

static int uart_ra_init(const struct device *dev)
{
	const struct uart_ra_cfg *config = dev->config;
	struct uart_ra_data *data = dev->data;
	int ret;

	/* Configure dt provided device signals when available */
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}

	if (!device_is_ready(config->clock_dev)) {
		return -ENODEV;
	}

	ret = clock_control_on(config->clock_dev, config->clock_id);
	if (ret < 0) {
		return ret;
	}

	ret = clock_control_get_rate(config->clock_dev, config->clock_id, &data->clk_rate);
	if (ret < 0) {
		return ret;
	}

	ret = uart_ra_configure(dev, &data->current_config);
	if (ret != 0) {
		return ret;
	}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	ret = config->irq_config_func(dev);
	if (ret != 0) {
		return ret;
	}
#endif

	return 0;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

static bool uart_ra_irq_is_enabled(const struct device *dev, uint32_t irq)
{
	return uart_ra_read_8(dev, SCR) & irq;
}

static int uart_ra_fifo_fill(const struct device *dev, const uint8_t *tx_data, int len)
{
	struct uart_ra_data *data = dev->data;
	uint8_t reg_val;
	k_spinlock_key_t key;

	if (len <= 0 || tx_data == NULL) {
		return 0;
	}

	key = k_spin_lock(&data->lock);
	reg_val = uart_ra_read_8(dev, SCR);
	reg_val &= ~(REG_MASK(SCR_TIE));
	uart_ra_write_8(dev, SCR, reg_val);

	uart_ra_write_8(dev, TDR, tx_data[0]);

	reg_val |= REG_MASK(SCR_TIE);
	uart_ra_write_8(dev, SCR, reg_val);

	k_spin_unlock(&data->lock, key);

	return 1;
}

static int uart_ra_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	uint8_t data;

	if (size <= 0) {
		return 0;
	}

	if ((uart_ra_read_8(dev, SSR) & REG_MASK(SSR_RDRF)) == 0) {
		return 0;
	}

	data = uart_ra_read_8(dev, RDR);

	if (rx_data) {
		rx_data[0] = data;
	}

	return 1;
}

static void uart_ra_irq_tx_enable(const struct device *dev)
{
	struct uart_ra_data *data = dev->data;
	k_spinlock_key_t key;
	uint16_t reg_val;

	key = k_spin_lock(&data->lock);

	reg_val = uart_ra_read_8(dev, SCR);
	reg_val |= (REG_MASK(SCR_TIE));
	uart_ra_write_8(dev, SCR, reg_val);

	irq_enable(data->irqn[UART_RA_INT_TXI]);

	k_spin_unlock(&data->lock, key);
}

static void uart_ra_irq_tx_disable(const struct device *dev)
{
	struct uart_ra_data *data = dev->data;
	k_spinlock_key_t key;
	uint16_t reg_val;

	key = k_spin_lock(&data->lock);

	reg_val = uart_ra_read_8(dev, SCR);
	reg_val &= ~(REG_MASK(SCR_TIE));
	uart_ra_write_8(dev, SCR, reg_val);

	irq_disable(data->irqn[UART_RA_INT_TXI]);

	k_spin_unlock(&data->lock, key);
}

static int uart_ra_irq_tx_ready(const struct device *dev)
{
	const uint8_t reg_val = uart_ra_read_8(dev, SSR);
	const uint8_t mask = REG_MASK(SSR_TEND) & REG_MASK(SSR_TDRE);

	return (reg_val & mask) == mask;
}

static void uart_ra_irq_rx_enable(const struct device *dev)
{
	struct uart_ra_data *data = dev->data;
	k_spinlock_key_t key;
	uint16_t reg_val;

	key = k_spin_lock(&data->lock);

	reg_val = uart_ra_read_8(dev, SCR);
	reg_val |= REG_MASK(SCR_RIE);
	uart_ra_write_8(dev, SCR, reg_val);

	irq_enable(data->irqn[UART_RA_INT_RXI]);

	k_spin_unlock(&data->lock, key);
}

static void uart_ra_irq_rx_disable(const struct device *dev)
{
	struct uart_ra_data *data = dev->data;
	k_spinlock_key_t key;
	uint16_t reg_val;

	key = k_spin_lock(&data->lock);

	reg_val = uart_ra_read_8(dev, SCR);
	reg_val &= ~REG_MASK(SCR_RIE);
	uart_ra_write_8(dev, SCR, reg_val);

	irq_disable(data->irqn[UART_RA_INT_RXI]);

	k_spin_unlock(&data->lock, key);
}

static int uart_ra_irq_rx_ready(const struct device *dev)
{
	return !!(uart_ra_read_8(dev, SSR) & REG_MASK(SSR_RDRF));
}

static void uart_ra_irq_err_enable(const struct device *dev)
{
	struct uart_ra_data *data = dev->data;

	irq_enable(data->irqn[UART_RA_INT_ERI]);
}

static void uart_ra_irq_err_disable(const struct device *dev)
{
	struct uart_ra_data *data = dev->data;

	irq_disable(data->irqn[UART_RA_INT_ERI]);
}

static int uart_ra_irq_is_pending(const struct device *dev)
{
	return (uart_ra_irq_rx_ready(dev) && uart_ra_irq_is_enabled(dev, REG_MASK(SCR_RIE))) ||
	       (uart_ra_irq_tx_ready(dev) && uart_ra_irq_is_enabled(dev, REG_MASK(SCR_TIE)));
}

static int uart_ra_irq_update(const struct device *dev)
{
	return 1;
}

static void uart_ra_irq_callback_set(const struct device *dev, uart_irq_callback_user_data_t cb,
				     void *cb_data)
{
	struct uart_ra_data *data = dev->data;

	data->callback = cb;
	data->cb_data = cb_data;
}

/**
 * @brief Interrupt service routine.
 *
 * This simply calls the callback function, if one exists.
 *
 * @param arg Argument to ISR.
 */
static inline void uart_ra_isr(const struct device *dev)
{
	struct uart_ra_data *data = dev->data;

	if (data->callback) {
		data->callback(dev, data->cb_data);
	}
}

static void uart_ra_isr_rxi(const void *param)
{
	const struct device *dev = param;
	struct uart_ra_data *data = dev->data;

	uart_ra_isr(dev);
	ra_icu_clear_int_flag(data->irqn[UART_RA_INT_RXI]);
}

static void uart_ra_isr_txi(const void *param)
{
	const struct device *dev = param;
	struct uart_ra_data *data = dev->data;

	uart_ra_isr(dev);
	ra_icu_clear_int_flag(data->irqn[UART_RA_INT_TXI]);
}

static void uart_ra_isr_eri(const void *param)
{
	const struct device *dev = param;
	struct uart_ra_data *data = dev->data;

	uart_ra_isr(dev);
	ra_icu_clear_int_flag(data->irqn[UART_RA_INT_ERI]);
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static const struct uart_driver_api uart_ra_driver_api = {
	.poll_in = uart_ra_poll_in,
	.poll_out = uart_ra_poll_out,
	.err_check = uart_ra_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = uart_ra_configure,
	.config_get = uart_ra_config_get,
#endif
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_ra_fifo_fill,
	.fifo_read = uart_ra_fifo_read,
	.irq_tx_enable = uart_ra_irq_tx_enable,
	.irq_tx_disable = uart_ra_irq_tx_disable,
	.irq_tx_ready = uart_ra_irq_tx_ready,
	.irq_rx_enable = uart_ra_irq_rx_enable,
	.irq_rx_disable = uart_ra_irq_rx_disable,
	.irq_rx_ready = uart_ra_irq_rx_ready,
	.irq_err_enable = uart_ra_irq_err_enable,
	.irq_err_disable = uart_ra_irq_err_disable,
	.irq_is_pending = uart_ra_irq_is_pending,
	.irq_update = uart_ra_irq_update,
	.irq_callback_set = uart_ra_irq_callback_set,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

/* Device Instantiation */
#define UART_RA_INIT_CFG(n)                                                                        \
	PINCTRL_DT_DEFINE(DT_INST_PARENT(n));                                                      \
	static const struct uart_ra_cfg uart_ra_cfg_##n = {                                        \
		.regs = DT_REG_ADDR(DT_INST_PARENT(n)),                                            \
		.clock_dev = DEVICE_DT_GET(DT_CLOCKS_CTLR(DT_INST_PARENT(n))),                     \
		.clock_id =                                                                        \
			(clock_control_subsys_t)DT_CLOCKS_CELL_BY_IDX(DT_INST_PARENT(n), 0, id),   \
		.pcfg = PINCTRL_DT_DEV_CONFIG_GET(DT_INST_PARENT(n)),                              \
		IF_ENABLED(CONFIG_UART_INTERRUPT_DRIVEN, (                                         \
			.irq_config_func = irq_config_func_##n,                                    \
		))                                                                                 \
	}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

#define RA_IRQ_CONNECT_DYNAMIC(n, name, dev, isr)                                                  \
	ra_icu_irq_connect_dynamic(DT_IRQ_BY_NAME(DT_INST_PARENT(n), name, irq),                   \
				   DT_IRQ_BY_NAME(DT_INST_PARENT(n), name, priority), isr, dev,    \
				   DT_IRQ_BY_NAME(DT_INST_PARENT(n), name, flags));

#define RA_IRQ_DISCONNECT_DYNAMIC(n, name, dev, isr)                                               \
	ra_icu_irq_disconnect_dynamic(irqn, 0, NULL, NULL, 0)

#define UART_RA_CONFIG_FUNC(n)                                                                     \
	static int irq_config_func_##n(const struct device *dev)                                   \
	{                                                                                          \
		struct uart_ra_data *data = dev->data;                                             \
		int irqn;                                                                          \
                                                                                                   \
		irqn = RA_IRQ_CONNECT_DYNAMIC(n, rxi, dev, uart_ra_isr_rxi);                       \
		if (irqn < 0) {                                                                    \
			return irqn;                                                               \
		}                                                                                  \
		data->irqn[UART_RA_INT_RXI] = irqn;                                                \
		irqn = RA_IRQ_CONNECT_DYNAMIC(n, txi, dev, uart_ra_isr_txi);                       \
		if (irqn < 0) {                                                                    \
			goto err_txi;                                                              \
		}                                                                                  \
		data->irqn[UART_RA_INT_TXI] = irqn;                                                \
		irqn = RA_IRQ_CONNECT_DYNAMIC(n, eri, dev, uart_ra_isr_eri);                       \
		if (irqn < 0) {                                                                    \
			goto err_eri;                                                              \
		}                                                                                  \
		data->irqn[UART_RA_INT_ERI] = irqn;                                                \
		return 0;                                                                          \
                                                                                                   \
err_eri:                                                                                           \
		RA_IRQ_DISCONNECT_DYNAMIC(data->irq[UART_RA_INT_TXI], eri, dev, uart_ra_isr_eri);  \
err_txi:                                                                                           \
		RA_IRQ_DISCONNECT_DYNAMIC(data->irq[UART_RA_INT_RXI], txi, dev, uart_ra_isr_txi);  \
                                                                                                   \
		return irqn;                                                                       \
	}
#else
#define UART_RA_CONFIG_FUNC(n)
#endif

#define UART_RA_INIT(n)                                                                            \
	UART_RA_CONFIG_FUNC(n)                                                                     \
	UART_RA_INIT_CFG(n);                                                                       \
                                                                                                   \
	static struct uart_ra_data uart_ra_data_##n = {                                            \
		.current_config = {                                                                \
			.baudrate = DT_INST_PROP(n, current_speed),                                \
			.parity = UART_CFG_PARITY_NONE,                                            \
			.stop_bits = UART_CFG_STOP_BITS_1,                                         \
			.data_bits = UART_CFG_DATA_BITS_8,                                         \
			.flow_ctrl = UART_CFG_FLOW_CTRL_NONE,                                      \
		},                                                                                 \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, uart_ra_init, NULL, &uart_ra_data_##n, &uart_ra_cfg_##n,          \
			      PRE_KERNEL_1, CONFIG_SERIAL_INIT_PRIORITY, &uart_ra_driver_api);

DT_INST_FOREACH_STATUS_OKAY(UART_RA_INIT)
