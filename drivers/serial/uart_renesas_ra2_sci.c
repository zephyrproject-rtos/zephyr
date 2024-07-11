/**
 * uart_renesas_ra2_sci.c
 *
 * @brief Driver for SCI based UART on RA2 series processor.
 *
 * Copyright (c) 2022-2024 MUNIC Car Data
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/irq.h>
#include <zephyr/spinlock.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/lpm/lpm_ra2.h>
#include <zephyr/pm/policy.h>
#include <zephyr/pm/device.h>

#if defined(CONFIG_UART_INTERRUPT_DRIVEN)
#	include <zephyr/drivers/interrupt_controller/intc_ra2_icu.h>
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#include "renesas_ra2_sci_priv.h"

#define DT_DRV_COMPAT renesas_ra2_sci_uart

struct uart_ra_config {
	mm_reg_t base;

	const struct device *clock_control;

#if defined(CONFIG_UART_INTERRUPT_DRIVEN)
	uart_irq_config_func_t  irq_config_func;
#endif

#ifdef CONFIG_PINCTRL
	const struct pinctrl_dev_config *pincfg;
#endif
	uint8_t lpm_id;

	uint8_t event_tx;
	uint8_t event_rx;
	uint8_t event_txe;
	uint8_t event_eri;
};

/* FIXME Even though configuring the driver for 9-bit data transmissions is
 * possible, the driver will not work properly, since 9-bit data must be
 * checked in a different register for reception and sending
 */

struct uart_ra_data {
	struct uart_config  ucfg;
	struct k_spinlock lock;

#if defined(CONFIG_UART_INTERRUPT_DRIVEN)
	const struct device *dev;
#endif

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	/* Interrupt driven data */
	uart_irq_callback_user_data_t user_cb;
	void *user_data;

	struct icu_event *icu_event_tx;
	struct icu_event *icu_event_rx;
	struct icu_event *icu_event_txe;
	struct icu_event *icu_event_eri;
#endif

};

static ALWAYS_INLINE
uint8_t uart_ra_read_8(const struct uart_ra_config *cfg, mm_reg_t offs)
{
	return sys_read8(cfg->base + offs);
}

static ALWAYS_INLINE
void uart_ra_write_8(const struct uart_ra_config *cfg, mm_reg_t offs,
		uint8_t value)
{
	sys_write8(value, cfg->base + offs);
}

static ALWAYS_INLINE
uint16_t uart_ra_read_16(const struct uart_ra_config *cfg, mm_reg_t offs)
{
	return sys_read16(cfg->base + offs);
}

static ALWAYS_INLINE
void uart_ra_write_16(const struct uart_ra_config *cfg, uint32_t offs,
		uint16_t value)
{
	sys_write16(value, cfg->base + offs);
}

static inline
void uart_ra_clear_err_flags(const struct device *dev, uint8_t mask)
{
	const struct uart_ra_config *cfg = dev->config;
	uint8_t err_flags = uart_ra_read_8(cfg, R_SCI_SSR);

	err_flags &= ~mask;
	uart_ra_write_8(cfg, R_SCI_SSR, err_flags);
}

static int uart_ra_poll_in(const struct device *dev, unsigned char *p_char)
{
	const struct uart_ra_config *cfg = dev->config;
	int ret = -1;
	uint8_t ssr = uart_ra_read_8(cfg, R_SCI_SSR);

	if (ssr & R_SCI_SSR_RDRF_Msk) {
		*p_char = uart_ra_read_8(cfg, R_SCI_RDR);
		ret = 0;
	}

	uart_ra_clear_err_flags(dev, R_SCI_SSR_PER_Msk |
					R_SCI_SSR_FER_Msk |
					R_SCI_SSR_ORER_Msk);

	return ret;
}

static void uart_ra_poll_out(const struct device *dev, unsigned char out_char)
{
	const struct uart_ra_config *cfg = dev->config;

	while ((uart_ra_read_8(cfg, R_SCI_SSR) & R_SCI_SSR_TDRE_Msk) == 0)
		;
	uart_ra_write_8(cfg, R_SCI_TDR, out_char);
}

static int uart_ra_err_check(const struct device *dev)
{
	const struct uart_ra_config *cfg = dev->config;
	int err = 0;
	uint8_t ssr = uart_ra_read_8(cfg, R_SCI_SSR);

	if (ssr & R_SCI_SSR_PER_Msk) {
		err |= UART_ERROR_PARITY;
	}

	if (ssr & R_SCI_SSR_FER_Msk) {
		err |= UART_ERROR_FRAMING;
	}

	if (ssr & R_SCI_SSR_ORER_Msk) {
		err |= UART_ERROR_OVERRUN;
	}

	uart_ra_clear_err_flags(dev, R_SCI_SSR_PER_Msk |
					R_SCI_SSR_FER_Msk |
					R_SCI_SSR_ORER_Msk);

	return err;
}

/* TODO This algorithm is not optimal... It might fail to find an appropriate
 * configuration (i.e with specified error rate) for a certain baudrate, even
 * though that configuration exists.
 */

/* Some helper defines for baud rate settings */
#define Z_Z_Z 64
#define Z_O_Z 32
#define O_O_Z 16
#define X_X_O 12

static inline
int uart_ra_set_baudrate(const struct device *dev, uint32_t baudrate)
{
	int ret;
	const struct uart_ra_config *cfg = dev->config;
	uint8_t semr_reg;
	uint32_t pclkb_hz;
	int64_t N;
	uint16_t M;
	uint8_t n, semr_cfg;

	N = -1; /* Invalid initial value */
	semr_cfg = X_X_O;
	M = 256; /* 128 <= M <= 256 */
	n = 0;

	/* This is the base clock frequency for the baudrate generator */
	ret = clock_control_get_rate(cfg->clock_control, NULL, &pclkb_hz);
	if (ret) {
		return ret;
	}

	/* Degrees of freedom:
	 * 1/ n: SMR clock factor: n: 0->3 => PCLKB / (1->64)
	 * 2/ First equation factor (depends on BGDM, ABCS, ABCSE): 64->12
	 * 3/ M: Modulation correction (MDDR) 128->256
	 * 4/ N: BRR baud rate generatior
	 */

	/* Note that I reordered the equation in 25.19, so that we may lose the
	 * least precision possible. Also note that the value of N is
	 * multiplied by 1024, to lose less precision
	 */

	/* Finally note that I multiplied num and denum by 2, because the given
	 * equation multiplies by 2 if n == 0
	 */
	N = ((int64_t)1024 * pclkb_hz * M)
			/ (semr_cfg * (1 << (2 * n + 7)) * (int64_t)baudrate)
			- 1024;

	/* We started with the config that gives us the largest N. Going
	 * forwards, we need to reduce it until it fits between 0 and 255
	 */

	/* NOTE I've inverted all divisions in comparisons so that they'd
	 * become multiplications. The compared term should be N / 255.
	 */

	if (N < 0) {
		return -EINVAL;
	}

	if (N > 1024 * 255 * 64) {
		n = 3;
	} else if (N > 1024 * 255 * 16) {
		n = 2;
	} else if (N > 1024 * 255 * 4) {
		n = 1;
	}

	N = ((int64_t)1024 * pclkb_hz * M) /
			(semr_cfg * (1 << (2 * n + 7)) * (int64_t)baudrate) -
			1024;

	if (N * 12 > 1024 * 64 * 255) {
		semr_cfg = Z_Z_Z;
	} else if (N * 12 > 1024 * 32 * 255) {
		semr_cfg = Z_O_Z;
	} else if (N * 12 > 1024 * 16 * 255) {
		semr_cfg = O_O_Z;
	}

	N = ((int64_t)1024 * pclkb_hz * M) /
			(semr_cfg * (1 << (2 * n + 7)) * (int64_t)baudrate) -
			1024;

	/* At this point we can only vary M. So if the ratio is still > 2, we
	 * cannot achieve the baudrate
	 */
	if (N * 2 > 1024 * 255) {
		return -ERANGE;
	}

	/* N Now we need N to be a whole integer. The second condition puts a
	 * limit of 1/64 on the error
	 */
	while (N > 1024 * 255 || (N % 1024 > 8 && N % 1024 < 1016)) {
		/* If we get here, it means we could not find an M that gives
		 * us an acceptable error rate
		 */
		if (--M < 128) {
			return -EDOM;
		}

		N = ((int64_t)1024 * pclkb_hz * M) /
			(semr_cfg * (1 << (2 * n + 7)) *
					(int64_t)baudrate) - 1024;

	}

	/* Phew, if we got here, we've determined all our variables, let's
	 * set them
	 */
	semr_reg = uart_ra_read_8(cfg, R_SCI_SEMR);
	switch (semr_cfg) {
	case Z_Z_Z:
		semr_reg &= ~(R_SCI_SEMR_ABCSE_Msk |
		R_SCI_SEMR_ABCS_Msk |
		R_SCI_SEMR_BGDM_Msk);
		break;
	case Z_O_Z:
		semr_reg &= ~(R_SCI_SEMR_ABCSE_Msk |
		R_SCI_SEMR_BGDM_Msk);
		semr_reg |= R_SCI_SEMR_ABCS_Msk;
		break;
	case O_O_Z:
		semr_reg &= ~R_SCI_SEMR_ABCSE_Msk;
		semr_reg |= R_SCI_SEMR_ABCS_Msk | R_SCI_SEMR_BGDM_Msk;
		break;
	case X_X_O:
		semr_reg |= R_SCI_SEMR_ABCSE_Msk;
		break;
	default:
		return -EFAULT;
	}

	uart_ra_write_8(cfg, R_SCI_SMR,
		(uart_ra_read_8(cfg, R_SCI_SMR) & ~R_SCI_SMR_CKS_Msk) | n);

	/* By default we do not use modulation */
	if (M < 256) {
		/* But if we do we set it here */
		semr_reg |= R_SCI_SEMR_BRME_Msk;
		uart_ra_write_8(cfg, R_SCI_MDDR, M & R_SCI_MDDR_MDDR_Msk);
	} else {
		semr_reg &= ~R_SCI_SEMR_BRME_Msk;
	}


	uart_ra_write_8(cfg, R_SCI_SEMR, semr_reg);
	uart_ra_write_8(cfg, R_SCI_BRR, (N + 512) / 1024);

	return 0;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static int uart_ra_configure(const struct device *dev,
		const struct uart_config *ucfg)
{
	const struct uart_ra_config *cfg = dev->config;
	struct uart_ra_data *dat = (struct uart_ra_data *)dev->data;
	uint8_t scr = 0, smr, scmr;

	/* Stop the uart controller */
	K_SPINLOCK(&dat->lock) {
		scr = uart_ra_read_8(cfg, R_SCI_SCR);
		uart_ra_write_8(cfg, R_SCI_SCR, 0);
	}

	if (uart_ra_set_baudrate(dev, ucfg->baudrate) < 0)
		return -ENOSYS;

	smr  = uart_ra_read_8(cfg, R_SCI_SMR);
	scmr = uart_ra_read_8(cfg, R_SCI_SCMR);

	switch (ucfg->parity) {
	case UART_CFG_PARITY_NONE:
		smr &= ~(R_SCI_SMR_PM_Msk | R_SCI_SMR_PE_Msk);
		break;
	case UART_CFG_PARITY_ODD:
		smr |= R_SCI_SMR_PM_Msk | R_SCI_SMR_PE_Msk;
		break;
	case UART_CFG_PARITY_EVEN:
		smr &= ~R_SCI_SMR_PM_Msk;
		smr |= R_SCI_SMR_PE_Msk;
		break;
	default:
		return -ENOSYS;
	}

	switch (ucfg->stop_bits) {
	case UART_CFG_STOP_BITS_1:
		smr &= ~R_SCI_SMR_STOP_Msk;
		break;
	case UART_CFG_STOP_BITS_2:
		smr |= R_SCI_SMR_STOP_Msk;
		break;
	default:
		return -ENOSYS;
	}

	switch (ucfg->data_bits) {
	case UART_CFG_DATA_BITS_7:
		smr |= R_SCI_SMR_CHR_Msk;
		scmr |= R_SCI_SCMR_CHR1_Msk;
		break;
	case UART_CFG_DATA_BITS_8:
		smr &= ~R_SCI_SMR_CHR_Msk;
		scmr |= R_SCI_SCMR_CHR1_Msk;
		break;
	case UART_CFG_DATA_BITS_9:
		smr &= ~R_SCI_SMR_CHR_Msk;
		scmr &= ~R_SCI_SCMR_CHR1_Msk;
		break;
	default:
		return -ENOSYS;
	}

	switch (ucfg->flow_ctrl) {
	case UART_CFG_FLOW_CTRL_NONE:
		break;
	default:
		/* TODO: add CTR/RTS support */
		return -ENOSYS;
	}

	uart_ra_write_8(cfg, R_SCI_SMR, smr);
	uart_ra_write_8(cfg, R_SCI_SCMR, scmr);

	dat->ucfg = *ucfg;

	/* Restore the uart controller */
	uart_ra_write_8(cfg, R_SCI_SCR, scr);

	return 0;
}

static int uart_ra_config_get(const struct device *dev, struct uart_config *cfg)
{
	struct uart_ra_data *dat = (struct uart_ra_data *)dev->data;

	*cfg = dat->ucfg;
	return 0;
}
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
/* This function is always called from an interrupt context. It never blocks,
 * and writes as many characters to the serial port as possible.
 * Note that using this function in SCI_TXI is what allows continuous uart
 * writes using the double registers (cf. TDR, TSR)
 */
static
int uart_ra_fifo_fill(const struct device *dev, const uint8_t *tx_data,
		int size)
{
	const struct uart_ra_config *cfg = dev->config;
	int num_tx;

	for (num_tx = 0; num_tx < size; num_tx++) {
		if ((uart_ra_read_8(cfg, R_SCI_SSR_FIFO) &
				R_SCI_SSR_FIFO_TDFE_Msk) == 0)
			break;
		uart_ra_write_8(cfg, R_SCI_TDR, *tx_data++);
	}

	return num_tx;
}

/* This function non-blockingly reads from the serial controller to the rx_data
 * param. It is always called from an interrupt context.
 * Note that using this function in SCI_RXI is what allows continuous uart
 * reads using the double registers (cf. RDR, RSR)
 */
static
int uart_ra_fifo_read(const struct device *dev, uint8_t *rx_data,
		const int size)
{
	const struct uart_ra_config *cfg = dev->config;
	int num_rx;

	for (num_rx = 0; num_rx < size; num_rx++) {
		if (!(uart_ra_read_8(cfg, R_SCI_SSR) & R_SCI_SSR_RDRF_Msk))
			break;
		*rx_data++ = uart_ra_read_8(cfg, R_SCI_FTDRL);
	}

	/* Clear error flags */
	/* TODO Maybe we should notify in case of overrun?! */
	uart_ra_clear_err_flags(dev, R_SCI_SSR_PER_Msk |
					R_SCI_SSR_FER_Msk |
					R_SCI_SSR_ORER_Msk);
	return num_rx;
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#if defined(CONFIG_UART_INTERRUPT_DRIVEN)
/* Enables sending data over uart, with interruptions. Causes an interruption
 * in order to start the sending process.
 */
static void uart_ra_irq_tx_enable(const struct device *dev)
{
	const struct uart_ra_config *cfg = dev->config;
	struct uart_ra_data *dat = (struct uart_ra_data *)dev->data;

	K_SPINLOCK(&dat->lock) {
		uart_ra_write_8(cfg, R_SCI_SCR,
			uart_ra_read_8(cfg, R_SCI_SCR) |
				R_SCI_SCR_TEIE_Msk |
				R_SCI_SCR_TIE_Msk);

	}
}

static void uart_ra_irq_tx_disable(const struct device *dev)
{
	const struct uart_ra_config *cfg = dev->config;
	struct uart_ra_data *dat = (struct uart_ra_data *)dev->data;

	K_SPINLOCK(&dat->lock) {
		uart_ra_write_8(cfg, R_SCI_SCR,
			uart_ra_read_8(cfg, R_SCI_SCR) &
				~(R_SCI_SCR_TEIE_Msk | R_SCI_SCR_TIE_Msk));

	}
}

/* Returns true if TX is ready to be used, i.e if we may write in TDR and TX
 * interrupts are enabled
 */
static int uart_ra_irq_tx_ready(const struct device *dev)
{
	int ret = 0;
	const struct uart_ra_config *cfg = dev->config;
	struct uart_ra_data *dat = (struct uart_ra_data *)dev->data;

	/* We are ready if TX is enabled */
	K_SPINLOCK(&dat->lock) {
		ret = (uart_ra_read_8(cfg, R_SCI_SSR) &
				R_SCI_SSR_TDRE_Msk) != 0;
		ret = ret && (uart_ra_read_8(cfg, R_SCI_SCR) &
					R_SCI_SCR_TIE_Msk) != 0;
	}

	return ret;
}

/* Returns true if transmit is complete, i.e if TEND flag is set to 1 */
static int uart_ra_irq_tx_complete(const struct device *dev)
{
	const struct uart_ra_config *cfg = dev->config;

	return (uart_ra_read_8(cfg, R_SCI_SSR) & R_SCI_SSR_TEND_Msk) != 0;
}

static void uart_ra_irq_rx_enable(const struct device *dev)
{
	const struct uart_ra_config *cfg = dev->config;
	struct uart_ra_data *dat = (struct uart_ra_data *)dev->data;

	K_SPINLOCK(&dat->lock) {
		uart_ra_write_8(cfg, R_SCI_SCR,
				uart_ra_read_8(cfg,
						R_SCI_SCR) | R_SCI_SCR_RIE_Msk);

	}
}

static void uart_ra_irq_rx_disable(const struct device *dev)
{
	const struct uart_ra_config *cfg = dev->config;
	struct uart_ra_data *dat = (struct uart_ra_data *)dev->data;

	K_SPINLOCK(&dat->lock) {
		uart_ra_write_8(cfg, R_SCI_SCR,
			uart_ra_read_8(cfg, R_SCI_SCR) & ~R_SCI_SCR_RIE_Msk);
	}
}

/* RX is ready when the RDR is full (there is something to read) */
static int uart_ra_irq_rx_ready(const struct device *dev)
{
	const struct uart_ra_config *cfg = dev->config;

	return (uart_ra_read_8(cfg, R_SCI_SSR) & R_SCI_SSR_RDRF_Msk) != 0;
}

/* For RA this is controlled by irq_rx_enable */
static void uart_ra_irq_err_enable(const struct device *dev)
{
}

/* For RA this is controlled by irq_rx_enable */
static void uart_ra_irq_err_disable(const struct device *dev)
{
}

/* Pending == RX ready or TX ready... I am REALLY not sure about this! */
static int uart_ra_irq_is_pending(const struct device *dev)
{
	const struct uart_ra_config *cfg = dev->config;
	struct uart_ra_data *dat = (struct uart_ra_data *)dev->data;
	uint8_t ssr, scr;

	k_spinlock_key_t key = k_spin_lock(&dat->lock);

	ssr = uart_ra_read_8(cfg, R_SCI_SSR);
	scr = uart_ra_read_8(cfg, R_SCI_SCR);

	k_spin_unlock(&dat->lock, key);

	return ((ssr & R_SCI_SSR_RDRF_Msk) && (scr & R_SCI_SCR_RIE_Msk)) ||
		((ssr & R_SCI_SSR_TDRE_Msk) && (scr & R_SCI_SCR_TIE_Msk));
}

static int uart_ra_irq_update(const struct device *dev)
{
	return 1;
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_ra_irq_callback_set(const struct device *dev,
			uart_irq_callback_user_data_t cb, void *user_data)
{
	struct uart_ra_data *data = dev->data;

	K_SPINLOCK(&data->lock) {
		data->user_cb = cb;
		data->user_data = user_data;

	}
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
static void uart_ra_isr(struct icu_event *icu_evt, const struct device *dev)
{
	struct uart_ra_data *data;

	ra_icu_clear_event(icu_evt);

	data = (struct uart_ra_data *) dev->data;

	if (data->user_cb) {
		data->user_cb(dev, data->user_data);
	}
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#if defined(CONFIG_UART_INTERRUPT_DRIVEN)
static void uart_ra_eri_isr(struct icu_event *icu_evt, const struct device *dev)
{
	/* Clear parity error, framing error and overrun error flags */
	ra_icu_clear_event(icu_evt);
	uart_ra_clear_err_flags(dev, R_SCI_SSR_PER_Msk |
					R_SCI_SSR_FER_Msk |
					R_SCI_SSR_ORER_Msk);
}
#endif

void uart_ra_set_break(const struct device *dev, bool on)
{
	const struct uart_ra_config *cfg = dev->config;
	struct uart_ra_data *dat = (struct uart_ra_data *)dev->data;

	K_SPINLOCK(&dat->lock) {
		if (on) {
			uint8_t sptr;

			uart_ra_write_8(cfg, R_SCI_SCR,
					uart_ra_read_8(cfg, R_SCI_SCR) &
						~R_SCI_SCR_TE_Msk);
			sptr = uart_ra_read_8(cfg, R_SCI_SPTR);
			sptr &= ~R_SCI_SPTR_SPB2DT_Msk;
			sptr |= R_SCI_SPTR_SPB2IO_Msk;
			uart_ra_write_8(cfg, R_SCI_SPTR, sptr);

		} else {
			uart_ra_write_8(cfg, R_SCI_SCR,
				uart_ra_read_8(cfg, R_SCI_SCR) |
					R_SCI_SCR_TE_Msk);
			uart_ra_write_8(cfg, R_SCI_SPTR,
				uart_ra_read_8(cfg, R_SCI_SPTR) &
					~R_SCI_SPTR_SPB2IO_Msk);
		}

	}
}

static const struct uart_driver_api uart_ra_driver_api = {
	.poll_in = uart_ra_poll_in,
	.poll_out = uart_ra_poll_out,
	.err_check = uart_ra_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = uart_ra_configure,
	.config_get = uart_ra_config_get,
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_ra_fifo_fill,
	.fifo_read = uart_ra_fifo_read,
	.irq_tx_enable = uart_ra_irq_tx_enable,
	.irq_tx_disable = uart_ra_irq_tx_disable,
	.irq_tx_ready = uart_ra_irq_tx_ready,
	.irq_tx_complete = uart_ra_irq_tx_complete,
	.irq_rx_enable = uart_ra_irq_rx_enable,
	.irq_rx_disable = uart_ra_irq_rx_disable,
	.irq_rx_ready = uart_ra_irq_rx_ready,
	.irq_err_enable = uart_ra_irq_err_enable,
	.irq_err_disable = uart_ra_irq_err_disable,
	.irq_is_pending = uart_ra_irq_is_pending,
	.irq_update = uart_ra_irq_update,
	.irq_callback_set = uart_ra_irq_callback_set,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
#ifdef CONFIG_UART_LINE_CTRL
	.line_ctrl_set = NULL,
	.line_ctrl_get = NULL,
#endif /* CONFIG_UART_LINE_CTRL */
#ifdef CONFIG_UART_DRV_CMD
	.drv_cmd = NULL,
#endif /* CONFIG_UART_DRV_CMD */
};

static int uart_ra_init(const struct device *dev)
{
	const struct uart_ra_config *cfg = dev->config;
	struct uart_ra_data *data = (struct uart_ra_data *) dev->data;
	int ret;

	/* Exit low power */
	ret = clock_control_on(cfg->clock_control, NULL);
	if (ret < 0) {
		return ret;
	}

	lpm_ra_activate_module(cfg->lpm_id);

	uart_ra_write_8(cfg, R_SCI_SCR, 0);

	uart_ra_write_8(cfg, R_SCI_SMR, 0);
	uart_ra_write_8(cfg, R_SCI_SCMR, 0);

	uart_ra_write_8(cfg, R_SCI_SEMR,
			uart_ra_read_8(cfg, R_SCI_SEMR) |
				R_SCI_SEMR_NFEN_Msk |
				R_SCI_SEMR_RXDESEL_Msk
			);

	uart_ra_configure(dev, &data->ucfg);

	/* Configure IO pins for tx and rx */
#ifdef CONFIG_PINCTRL
	ret = pinctrl_apply_state(cfg->pincfg, PINCTRL_STATE_DEFAULT);
	if (ret) {
		return ret;
	}
#endif
	/* Start without interrupts, the config function will activate them if
	 * necessary
	 */
	uart_ra_write_8(cfg, R_SCI_SCR,
			uart_ra_read_8(cfg, R_SCI_SCR) |
				R_SCI_SCR_TE_Msk |
				R_SCI_SCR_RE_Msk
			);

#if defined(CONFIG_UART_INTERRUPT_DRIVEN)
	cfg->irq_config_func(dev);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
	return ret;
}

#if defined(CONFIG_UART_INTERRUPT_DRIVEN)
static void uart_ra_irq_config_func(const struct device *dev)
{
	const struct uart_ra_config *conf = dev->config;
	struct uart_ra_data *data = dev->data;

	data->dev = dev;

	data->icu_event_eri = ra_icu_setup_event_irq(
		conf->event_eri,
		(event_cb_t) uart_ra_eri_isr,
		(void *) dev
	);

	ra_icu_enable_event(data->icu_event_eri);

	data->icu_event_tx = ra_icu_setup_event_irq(
		conf->event_tx,
		(event_cb_t) uart_ra_isr,
		(void *) dev
	);

	data->icu_event_rx = ra_icu_setup_event_irq(
		conf->event_rx,
		(event_cb_t) uart_ra_isr,
		(void *) dev
	);

	data->icu_event_txe = ra_icu_setup_event_irq(
		conf->event_txe,
		(event_cb_t) uart_ra_isr,
		(void *) dev
	);

	ra_icu_enable_event(data->icu_event_rx);
	ra_icu_enable_event(data->icu_event_tx);
	ra_icu_enable_event(data->icu_event_txe);
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#ifdef CONFIG_PM_DEVICE
static int uart_ra_pm_control(const struct device *dev,
				enum pm_device_action action)
{
	const struct uart_ra_config *conf = dev->config;
	int ret = 0;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		ret = clock_control_on(conf->clock_control, NULL);
		if (ret) {
			break;
		}

		lpm_ra_activate_module(conf->lpm_id);
#ifdef CONFIG_PINCTRL
		ret = pinctrl_apply_state(conf->pincfg,
		PINCTRL_STATE_DEFAULT);
#endif
		break;

	case PM_DEVICE_ACTION_SUSPEND:
	case PM_DEVICE_ACTION_TURN_OFF:
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
		while (!uart_ra_irq_tx_complete(dev))
			;
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
#ifdef CONFIG_PINCTRL
		ret = pinctrl_apply_state(conf->pincfg,
		PINCTRL_STATE_SLEEP);
		if (ret < 0) {
			break;
		}
#endif
		lpm_ra_deactivate_module(conf->lpm_id);

		ret = clock_control_off(conf->clock_control, NULL);
		break;
	default:
		ret = -ENOTSUP;
		break;
	}
	return ret;
}
#endif /* CONFIG_PM_DEVICE */

#if defined(CONFIG_UART_INTERRUPT_DRIVEN)
#	define IRQ_DEF .irq_config_func = uart_ra_irq_config_func,
#else
#	define IRQ_DEF
#endif

#ifdef CONFIG_PINCTRL
#define PINCTRL_INIT(n) .pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),
#define PINCTRL_DEFINE(n) PINCTRL_DT_INST_DEFINE(n)
#else
#define PINCTRL_DEFINE(n)
#define PINCTRL_INIT(n)
#endif

#define RA_UART_INIT(index)						\
PINCTRL_DEFINE(index);							\
PM_DEVICE_DT_INST_DEFINE(index, uart_ra_pm_control);			\
									\
	static struct uart_ra_data uart_ra_data_##index = {		\
		.ucfg = {						\
			.baudrate = DT_INST_PROP_OR(index,		\
					current_speed, 115200),		\
			.parity = UART_CFG_PARITY_NONE,			\
			.stop_bits = UART_CFG_STOP_BITS_1,		\
			.data_bits = UART_CFG_DATA_BITS_8,		\
			.flow_ctrl = UART_CFG_FLOW_CTRL_NONE,		\
		},							\
	};								\
	static const struct uart_ra_config uart_ra_cfg_##index = {	\
		.base = DT_INST_REG_ADDR(index),			\
		.clock_control = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(index)), \
		IRQ_DEF							\
		.lpm_id = DT_INST_PROP(index, lpm_id),			\
		.event_tx = DT_INST_IRQ_BY_NAME(index, txi, irq),	\
		.event_rx = DT_INST_IRQ_BY_NAME(index, rxi, irq),	\
		.event_txe = DT_INST_IRQ_BY_NAME(index, tei, irq),	\
		.event_eri = DT_INST_IRQ_BY_NAME(index, eri, irq),	\
		PINCTRL_INIT(index)					\
	};								\
	DEVICE_DT_INST_DEFINE(index,					\
			&uart_ra_init,					\
			PM_DEVICE_DT_INST_GET(index),			\
			&uart_ra_data_##index,				\
			&uart_ra_cfg_##index,				\
			/* Initialize UART device before UART console. */ \
			PRE_KERNEL_1,					\
			CONFIG_SERIAL_INIT_PRIORITY,			\
			&uart_ra_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RA_UART_INIT)
