/*
 * Copyright (C) 2026 Microchip Technology Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file uart_mchp_uart_g1.c
 * @brief UART driver for Microchip UART peripheral (compatible: microchip,uart-g1).
 */

#define DT_DRV_COMPAT microchip_uart_g1

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/clock_control/mchp_sam_pmc.h>
#include <zephyr/init.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/sys_io.h>

LOG_MODULE_REGISTER(dma_mchp_xdmac_g1, CONFIG_UART_LOG_LEVEL);

/* Device constant configuration parameters */
struct mchp_uart_dev_cfg {
	DEVICE_MMIO_ROM;
	const struct sam_clk_cfg clock_cfg;
	const struct pinctrl_dev_config *pin_cfg;

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_config_func_t irq_config_func;
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

/* Device run time data */
struct mchp_uart_dev_data {
	DEVICE_MMIO_RAM;
	uint32_t baud_rate;

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t irq_cb;
	void *irq_cb_data;
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

static int mchp_uart_poll_in(const struct device *dev, unsigned char *c)
{
	mm_reg_t base = DEVICE_MMIO_GET(dev);

	if (FIELD_GET(UART_SR_RXRDY_Msk, sys_read32(base + UART_SR_REG_OFST)) == 0) {
		return -ENODATA;
	}

	/* Got a character */
	*c = FIELD_GET(UART_RHR_RXCHR_Msk, sys_read32(base + UART_RHR_REG_OFST));

	return 0;
}

static void mchp_uart_poll_out(const struct device *dev, unsigned char c)
{
	mm_reg_t base = DEVICE_MMIO_GET(dev);

	/* Wait for transmitter to be ready */
	while (FIELD_GET(UART_SR_TXRDY_Msk, sys_read32(base + UART_SR_REG_OFST)) == 0) {
	}

	/* Send a character */
	sys_write32(UART_THR_TXCHR(c), base + UART_THR_REG_OFST);
}

static int mchp_uart_err_check(const struct device *dev)
{
	mm_reg_t base = DEVICE_MMIO_GET(dev);
	uint32_t sr = sys_read32(base + UART_SR_REG_OFST);
	int errors = 0;

	if (FIELD_GET(UART_SR_OVRE_Msk, sr) != 0) {
		errors |= UART_ERROR_OVERRUN;
	}

	if (FIELD_GET(UART_SR_PARE_Msk, sr) != 0) {
		errors |= UART_ERROR_PARITY;
	}

	if (FIELD_GET(UART_SR_FRAME_Msk, sr) != 0) {
		errors |= UART_ERROR_FRAMING;
	}

	/* Clear status flags */
	sys_write32(UART_CR_RSTSTA_Msk, base + UART_CR_REG_OFST);

	return errors;
}

static uint32_t mchp_uart_cfg2sam_parity(uint8_t parity)
{
	switch (parity) {
	case UART_CFG_PARITY_EVEN:
		return UART_MR_PAR_EVEN;
	case UART_CFG_PARITY_ODD:
		return UART_MR_PAR_ODD;
	case UART_CFG_PARITY_SPACE:
		return UART_MR_PAR_SPACE;
	case UART_CFG_PARITY_MARK:
		return UART_MR_PAR_MARK;
	case UART_CFG_PARITY_NONE:
	default:
		return UART_MR_PAR_NO;
	}
}

static uint8_t mchp_uart_get_parity(const struct device *dev)
{
	uint32_t mr = sys_read32(DEVICE_MMIO_GET(dev) + UART_MR_REG_OFST);

	switch (mr & UART_MR_PAR_Msk) {
	case UART_MR_PAR_EVEN:
		return UART_CFG_PARITY_EVEN;
	case UART_MR_PAR_ODD:
		return UART_CFG_PARITY_ODD;
	case UART_MR_PAR_SPACE:
		return UART_CFG_PARITY_SPACE;
	case UART_MR_PAR_MARK:
		return UART_CFG_PARITY_MARK;
	case UART_MR_PAR_NO:
	default:
		return UART_CFG_PARITY_NONE;
	}
}

static int mchp_uart_configure(const struct device *dev, const struct uart_config *uart_cfg)
{
	mm_reg_t base = DEVICE_MMIO_GET(dev);

	/* Only supports 8 data bits, 1 stop bit, and no flow control */
	if (uart_cfg->stop_bits != UART_CFG_STOP_BITS_1 ||
	    uart_cfg->data_bits != UART_CFG_DATA_BITS_8 ||
	    uart_cfg->flow_ctrl != UART_CFG_FLOW_CTRL_NONE) {
		return -ENOTSUP;
	}

	/* Reset and disable UART */
	sys_write32(UART_CR_RSTRX_Msk | UART_CR_RSTTX_Msk | UART_CR_RXDIS_Msk | UART_CR_TXDIS_Msk |
		    UART_CR_RSTSTA_Msk,
		    base + UART_CR_REG_OFST);

	/* Normal channel mode, peripheral clock source, parity from config */
	sys_write32(UART_MR_CHMODE_NORMAL | UART_MR_BRSRCCK_PERIPH_CLK |
		    mchp_uart_cfg2sam_parity(uart_cfg->parity),
		    base + UART_MR_REG_OFST);

	/* Enable receiver and transmitter */
	sys_write32(UART_CR_RXEN_Msk | UART_CR_TXEN_Msk, base + UART_CR_REG_OFST);

	return 0;
}

static int mchp_uart_config_get(const struct device *dev, struct uart_config *uart_cfg)
{
	struct mchp_uart_dev_data *const dev_data = dev->data;

	uart_cfg->baudrate = dev_data->baud_rate;
	uart_cfg->parity = mchp_uart_get_parity(dev);
	/* Only supported mode for this peripheral */
	uart_cfg->stop_bits = UART_CFG_STOP_BITS_1;
	uart_cfg->data_bits = UART_CFG_DATA_BITS_8;
	uart_cfg->flow_ctrl = UART_CFG_FLOW_CTRL_NONE;

	return 0;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

static int mchp_uart_fifo_fill(const struct device *dev, const uint8_t *tx_data, int size)
{
	mm_reg_t base = DEVICE_MMIO_GET(dev);

	/* Wait for transmitter to be ready */
	while (FIELD_GET(UART_SR_TXRDY_Msk, sys_read32(base + UART_SR_REG_OFST)) == 0) {
	}

	sys_write32(UART_THR_TXCHR(*tx_data), base + UART_THR_REG_OFST);

	/* Return number of bytes sent */
	return 1;
}

static int mchp_uart_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	mm_reg_t base = DEVICE_MMIO_GET(dev);
	int bytes_read = 0;

	while (bytes_read < size) {
		if (FIELD_GET(UART_SR_RXRDY_Msk, sys_read32(base + UART_SR_REG_OFST)) != 0) {
			rx_data[bytes_read] =
				FIELD_GET(UART_RHR_RXCHR_Msk, sys_read32(base + UART_RHR_REG_OFST));
			bytes_read++;
		} else {
			break;
		}
	}

	return bytes_read;
}

static void mchp_uart_irq_tx_enable(const struct device *dev)
{
	sys_write32(UART_IER_TXRDY_Msk, DEVICE_MMIO_GET(dev) + UART_IER_REG_OFST);
}

static void mchp_uart_irq_tx_disable(const struct device *dev)
{
	sys_write32(UART_IDR_TXRDY_Msk, DEVICE_MMIO_GET(dev) + UART_IDR_REG_OFST);
}

static int mchp_uart_irq_tx_ready(const struct device *dev)
{
	mm_reg_t base = DEVICE_MMIO_GET(dev);

	/* TX ready only if SR flag is set and the interrupt is also enabled */
	return ((sys_read32(base + UART_SR_REG_OFST) & UART_SR_TXRDY_Msk) &&
		(sys_read32(base + UART_IMR_REG_OFST) & UART_IMR_TXRDY_Msk));
}

static void mchp_uart_irq_rx_enable(const struct device *dev)
{
	sys_write32(UART_IER_RXRDY_Msk, DEVICE_MMIO_GET(dev) + UART_IER_REG_OFST);
}

static void mchp_uart_irq_rx_disable(const struct device *dev)
{
	sys_write32(UART_IDR_RXRDY_Msk, DEVICE_MMIO_GET(dev) + UART_IDR_REG_OFST);
}

static int mchp_uart_irq_tx_complete(const struct device *dev)
{
	mm_reg_t base = DEVICE_MMIO_GET(dev);

	return ((sys_read32(base + UART_SR_REG_OFST) & UART_SR_TXEMPTY_Msk) &&
		(sys_read32(base + UART_IMR_REG_OFST) & UART_IMR_TXEMPTY_Msk));
}

static int mchp_uart_irq_rx_ready(const struct device *dev)
{
	return !!(sys_read32(DEVICE_MMIO_GET(dev) + UART_SR_REG_OFST) & UART_SR_RXRDY_Msk);
}

static void mchp_uart_irq_err_enable(const struct device *dev)
{
	sys_write32(UART_IER_OVRE_Msk | UART_IER_FRAME_Msk | UART_IER_PARE_Msk,
		    DEVICE_MMIO_GET(dev) + UART_IER_REG_OFST);
}

static void mchp_uart_irq_err_disable(const struct device *dev)
{
	sys_write32(UART_IDR_OVRE_Msk | UART_IDR_FRAME_Msk | UART_IDR_PARE_Msk,
		    DEVICE_MMIO_GET(dev) + UART_IDR_REG_OFST);
}

static int mchp_uart_irq_is_pending(const struct device *dev)
{
	mm_reg_t base = DEVICE_MMIO_GET(dev);
	uint32_t imr = sys_read32(base + UART_IMR_REG_OFST);
	uint32_t sr  = sys_read32(base + UART_SR_REG_OFST);

	return !!(imr & (UART_IMR_TXRDY_Msk | UART_IMR_RXRDY_Msk) &
		  sr & (UART_SR_TXRDY_Msk  | UART_SR_RXRDY_Msk));
}

static void mchp_uart_irq_callback_set(const struct device *dev, uart_irq_callback_user_data_t cb,
				       void *cb_data)
{
	struct mchp_uart_dev_data *const dev_data = dev->data;

	dev_data->irq_cb = cb;
	dev_data->irq_cb_data = cb_data;
}

static void mchp_uart_isr(const struct device *dev)
{
	struct mchp_uart_dev_data *const dev_data = dev->data;

	if (dev_data->irq_cb) {
		dev_data->irq_cb(dev, dev_data->irq_cb_data);
	}
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static int mchp_uart_init(const struct device *dev)
{
	__maybe_unused const struct mchp_uart_dev_cfg *const cfg = dev->config;
	struct mchp_uart_dev_data *const dev_data = dev->data;

	/* Map the MMIO region into the address space */
	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	/* Disable all interrupts */
	sys_write32(UART_IDR_Msk, DEVICE_MMIO_GET(dev) + UART_IDR_REG_OFST);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	cfg->irq_config_func(dev);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

	struct uart_config uart_config = {
		.baudrate = dev_data->baud_rate,
		.parity = UART_CFG_PARITY_NONE,
		.stop_bits = UART_CFG_STOP_BITS_1,
		.data_bits = UART_CFG_DATA_BITS_8,
		.flow_ctrl = UART_CFG_FLOW_CTRL_NONE,
	};

	return mchp_uart_configure(dev, &uart_config);
}

static DEVICE_API(uart, mchp_uart_driver_api) = {
	.poll_in = mchp_uart_poll_in,
	.poll_out = mchp_uart_poll_out,
	.err_check = mchp_uart_err_check,
#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
	.configure = mchp_uart_configure,
	.config_get = mchp_uart_config_get,
#endif /* CONFIG_UART_USE_RUNTIME_CONFIGURE */
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = mchp_uart_fifo_fill,
	.fifo_read = mchp_uart_fifo_read,
	.irq_tx_enable = mchp_uart_irq_tx_enable,
	.irq_tx_disable = mchp_uart_irq_tx_disable,
	.irq_tx_ready = mchp_uart_irq_tx_ready,
	.irq_rx_enable = mchp_uart_irq_rx_enable,
	.irq_rx_disable = mchp_uart_irq_rx_disable,
	.irq_tx_complete = mchp_uart_irq_tx_complete,
	.irq_rx_ready = mchp_uart_irq_rx_ready,
	.irq_err_enable = mchp_uart_irq_err_enable,
	.irq_err_disable = mchp_uart_irq_err_disable,
	.irq_is_pending = mchp_uart_irq_is_pending,
	.irq_callback_set = mchp_uart_irq_callback_set,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

/*
 * -----------------------------------------------------------------------------
 * Per-instance instantiation macros
 * -----------------------------------------------------------------------------
 */

#define MCHP_UART_G1_DECLARE_CFG(n, IRQ_FUNC_INIT)					\
	static const struct mchp_uart_dev_cfg mchp_uart##n##_cfg = {			\
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),					\
		.clock_cfg = SAM_DT_INST_CLOCK_PMC_CFG(n),				\
		.pin_cfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),				\
		IRQ_FUNC_INIT								\
	}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

#define MCHP_UART_G1_CONFIG_FUNC(n)							\
	static void mchp_uart##n##_irq_config_func(const struct device *dev)		\
	{										\
		IRQ_CONNECT(DT_INST_IRQN(n), DT_INST_IRQ(n, priority), mchp_uart_isr,	\
			    DEVICE_DT_INST_GET(n), 0);					\
		irq_enable(DT_INST_IRQN(n));						\
	}
#define MCHP_UART_G1_IRQ_CFG_FUNC_INIT(n) .irq_config_func = mchp_uart##n##_irq_config_func
#define MCHP_UART_G1_INIT_CFG(n) MCHP_UART_G1_DECLARE_CFG(n, MCHP_UART_G1_IRQ_CFG_FUNC_INIT(n))

#else /* CONFIG_UART_INTERRUPT_DRIVEN */

#define MCHP_UART_G1_CONFIG_FUNC(n)
#define MCHP_UART_G1_IRQ_CFG_FUNC_INIT
#define MCHP_UART_G1_INIT_CFG(n) MCHP_UART_G1_DECLARE_CFG(n, MCHP_UART_G1_IRQ_CFG_FUNC_INIT)

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

#define MCHP_UART_G1_INIT(n)								\
	PINCTRL_DT_INST_DEFINE(n);							\
											\
	static const struct mchp_uart_dev_cfg mchp_uart##n##_cfg;			\
											\
	static struct mchp_uart_dev_data mchp_uart##n##_data = {			\
		.baud_rate = DT_INST_PROP(n, current_speed),				\
	};										\
											\
	DEVICE_DT_INST_DEFINE(n, mchp_uart_init,					\
			      NULL,							\
			      &mchp_uart##n##_data,					\
			      &mchp_uart##n##_cfg,					\
			      PRE_KERNEL_1,						\
			      CONFIG_SERIAL_INIT_PRIORITY,				\
			      &mchp_uart_driver_api);					\
											\
	MCHP_UART_G1_CONFIG_FUNC(n)							\
											\
	MCHP_UART_G1_INIT_CFG(n);

DT_INST_FOREACH_STATUS_OKAY(MCHP_UART_G1_INIT)
