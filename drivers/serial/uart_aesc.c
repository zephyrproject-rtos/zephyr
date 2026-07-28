/*
 * SPDX-FileCopyrightText: 2025 Aesc Silicon
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT aesc_uart

#include <errno.h>
#include <ip_identification.h>
#include <soc.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/irq.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/sys_io.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(aesc_uart, CONFIG_UART_LOG_LEVEL);

struct uart_aesc_data {
	DEVICE_MMIO_RAM;
	uintptr_t reg_base;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t irq_cb;
	void *irq_cb_data;
#endif
};

struct uart_aesc_config {
	DEVICE_MMIO_ROM;
	uint64_t sys_clk_freq;
	uint32_t current_speed;
	const struct pinctrl_dev_config *pcfg;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	void (*irq_config)(const struct device *dev);
#endif
};

#define UART_AESC_DATA_WIDTH		0x00
#define UART_AESC_SAMPLING_SIZES	0x04
#define UART_AESC_FIFO_DEPTHS		0x08
#define UART_AESC_PERMISSIONS		0x0c
#define UART_AESC_READ_WRITE		0x10
#define UART_AESC_FIFO_STATUS		0x14
#define UART_AESC_CLOCK_DIV		0x18
#define UART_AESC_FRAME_CFG		0x1c
#define UART_AESC_TX_TRIGGER		0x20
#define UART_AESC_IP			0x24
#define UART_AESC_IE			0x28
#define UART_AESC_ERROR_PENDING		0x2c
#define UART_AESC_ERROR_ENABLE		0x30

#define DEV_CFG(dev) ((struct uart_aesc_config *)(dev)->config)
#define DEV_DATA(dev) ((struct uart_aesc_data *)(dev)->data)

#define AESC_UART_IRQ_TX_EN			BIT(0)
#define AESC_UART_IRQ_RX_EN			BIT(1)
#define AESC_UART_IRQ_TX_COMPLETE_EN		BIT(2)
#define AESC_UART_FIFO_TX_COUNT_MASK		GENMASK(23, 16)
#define AESC_UART_FIFO_RX_COUNT_MASK		GENMASK(31, 24)
#define AESC_UART_READ_FIFO_VALID_BIT		BIT(16)
#define AESC_UART_ERR_FRAMING			BIT(0)
#define AESC_UART_ERR_PARITY			BIT(1)
#define AESC_UART_ERR_OVERFLOW			BIT(2)

static void uart_aesc_poll_out(const struct device *dev, unsigned char c)
{
	struct uart_aesc_data *data = DEV_DATA(dev);

	while ((sys_read32(data->reg_base + UART_AESC_FIFO_STATUS) &
		AESC_UART_FIFO_TX_COUNT_MASK) == 0) {
		/* Wait until transmit fifo is empty */
	}

	sys_write32(c, data->reg_base + UART_AESC_READ_WRITE);
}

static int uart_aesc_poll_in(const struct device *dev, unsigned char *c)
{
	struct uart_aesc_data *data = DEV_DATA(dev);
	uint32_t val;

	val = sys_read32(data->reg_base + UART_AESC_READ_WRITE);
	if (val & AESC_UART_READ_FIFO_VALID_BIT) {
		*c = val & 0xFF;
		return 0;
	}

	return -1;
}

#ifdef CONFIG_UART_INTERRUPT_DRIVEN

static int uart_aesc_fifo_fill(const struct device *dev,
			       const uint8_t *tx_data, int size)
{
	struct uart_aesc_data *data = DEV_DATA(dev);
	int i = 0;

	while (i < size && (sys_read32(data->reg_base + UART_AESC_FIFO_STATUS) &
			     AESC_UART_FIFO_TX_COUNT_MASK)) {
		sys_write32(tx_data[i++], data->reg_base + UART_AESC_READ_WRITE);
	}

	return i;
}

static int uart_aesc_fifo_read(const struct device *dev, uint8_t *rx_data,
			       const int size)
{
	struct uart_aesc_data *data = DEV_DATA(dev);
	int i = 0;
	uint32_t val;

	while (i < size) {
		val = sys_read32(data->reg_base + UART_AESC_READ_WRITE);
		if (!(val & AESC_UART_READ_FIFO_VALID_BIT)) {
			break;
		}
		rx_data[i++] = val & 0xFF;
	}

	return i;
}

static void uart_aesc_irq_tx_enable(const struct device *dev)
{
	struct uart_aesc_data *data = DEV_DATA(dev);
	uint32_t ie = sys_read32(data->reg_base + UART_AESC_IE);

	sys_write32(ie | AESC_UART_IRQ_TX_EN, data->reg_base + UART_AESC_IE);
}

static void uart_aesc_irq_tx_disable(const struct device *dev)
{
	struct uart_aesc_data *data = DEV_DATA(dev);
	uint32_t ie = sys_read32(data->reg_base + UART_AESC_IE);

	sys_write32(ie & ~AESC_UART_IRQ_TX_EN, data->reg_base + UART_AESC_IE);
}

static int uart_aesc_irq_tx_ready(const struct device *dev)
{
	struct uart_aesc_data *data = DEV_DATA(dev);

	return !!(sys_read32(data->reg_base + UART_AESC_FIFO_STATUS) &
		  AESC_UART_FIFO_TX_COUNT_MASK);
}

static int uart_aesc_irq_tx_complete(const struct device *dev)
{
	struct uart_aesc_data *data = DEV_DATA(dev);

	return !!(sys_read32(data->reg_base + UART_AESC_IP) &
		  AESC_UART_IRQ_TX_COMPLETE_EN);
}

static void uart_aesc_irq_rx_enable(const struct device *dev)
{
	struct uart_aesc_data *data = DEV_DATA(dev);
	uint32_t ie = sys_read32(data->reg_base + UART_AESC_IE);

	sys_write32(ie | AESC_UART_IRQ_RX_EN, data->reg_base + UART_AESC_IE);
}

static void uart_aesc_irq_rx_disable(const struct device *dev)
{
	struct uart_aesc_data *data = DEV_DATA(dev);
	uint32_t ie = sys_read32(data->reg_base + UART_AESC_IE);

	sys_write32(ie & ~AESC_UART_IRQ_RX_EN, data->reg_base + UART_AESC_IE);
}

static int uart_aesc_irq_rx_ready(const struct device *dev)
{
	struct uart_aesc_data *data = DEV_DATA(dev);

	return !!(sys_read32(data->reg_base + UART_AESC_FIFO_STATUS) &
		  AESC_UART_FIFO_RX_COUNT_MASK);
}

static void uart_aesc_irq_err_enable(const struct device *dev)
{
	struct uart_aesc_data *data = DEV_DATA(dev);
	uint32_t error_enable = sys_read32(data->reg_base + UART_AESC_ERROR_ENABLE);

	error_enable |= AESC_UART_ERR_FRAMING | AESC_UART_ERR_PARITY |
			 AESC_UART_ERR_OVERFLOW;
	sys_write32(error_enable, data->reg_base + UART_AESC_ERROR_ENABLE);
}

static void uart_aesc_irq_err_disable(const struct device *dev)
{
	struct uart_aesc_data *data = DEV_DATA(dev);
	uint32_t error_enable = sys_read32(data->reg_base + UART_AESC_ERROR_ENABLE);

	error_enable &= ~(AESC_UART_ERR_FRAMING | AESC_UART_ERR_PARITY |
			   AESC_UART_ERR_OVERFLOW);
	sys_write32(error_enable, data->reg_base + UART_AESC_ERROR_ENABLE);
}

static int uart_aesc_irq_is_pending(const struct device *dev)
{
	struct uart_aesc_data *data = DEV_DATA(dev);

	/* ip already returns pending & masks, so any set bit means active IRQ */
	return !!sys_read32(data->reg_base + UART_AESC_IP);
}

static void uart_aesc_irq_callback_set(const struct device *dev,
					uart_irq_callback_user_data_t cb,
					void *user_data)
{
	struct uart_aesc_data *data = DEV_DATA(dev);

	data->irq_cb = cb;
	data->irq_cb_data = user_data;
}

static void uart_aesc_isr(const struct device *dev)
{
	struct uart_aesc_data *data = DEV_DATA(dev);
	uint32_t pending;
	uint32_t err_pending;

	pending = sys_read32(data->reg_base + UART_AESC_IP);
	sys_write32(pending, data->reg_base + UART_AESC_IP);
	err_pending = sys_read32(data->reg_base + UART_AESC_ERROR_PENDING);
	sys_write32(err_pending, data->reg_base + UART_AESC_ERROR_PENDING);

	if (data->irq_cb) {
		data->irq_cb(dev, data->irq_cb_data);
	}
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static int uart_aesc_init(const struct device *dev)
{
	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);
	const struct uart_aesc_config *cfg = DEV_CFG(dev);
	volatile uintptr_t *base_addr =
		(volatile uintptr_t *)DEVICE_MMIO_GET(dev);
	struct uart_aesc_data *data = DEV_DATA(dev);
	int ret;

	LOG_DBG("IP core version: %i.%i.%i.",
		ip_id_get_major_version(base_addr),
		ip_id_get_minor_version(base_addr),
		ip_id_get_patchlevel(base_addr)
	);
	data->reg_base = ip_id_relocate_driver(base_addr);
	LOG_DBG("Relocate driver to address 0x%lx.", data->reg_base);

	ret = pinctrl_apply_state(cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		LOG_ERR("failed to apply pinctrl");
		return ret;
	}

	sys_write32(cfg->sys_clk_freq / cfg->current_speed / 8,
		    data->reg_base + UART_AESC_CLOCK_DIV);
	sys_write32(7, data->reg_base + UART_AESC_FRAME_CFG);
	sys_write32(0, data->reg_base + UART_AESC_TX_TRIGGER);
	sys_write32(0, data->reg_base + UART_AESC_IE);
	sys_write32(0, data->reg_base + UART_AESC_ERROR_ENABLE);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	cfg->irq_config(dev);
#endif

	return 0;
}

static DEVICE_API(uart, uart_aesc_driver_api) = {
	.poll_in          = uart_aesc_poll_in,
	.poll_out         = uart_aesc_poll_out,
	.err_check        = NULL,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill        = uart_aesc_fifo_fill,
	.fifo_read        = uart_aesc_fifo_read,
	.irq_tx_enable    = uart_aesc_irq_tx_enable,
	.irq_tx_disable   = uart_aesc_irq_tx_disable,
	.irq_tx_ready     = uart_aesc_irq_tx_ready,
	.irq_tx_complete  = uart_aesc_irq_tx_complete,
	.irq_rx_enable    = uart_aesc_irq_rx_enable,
	.irq_rx_disable   = uart_aesc_irq_rx_disable,
	.irq_rx_ready     = uart_aesc_irq_rx_ready,
	.irq_err_enable   = uart_aesc_irq_err_enable,
	.irq_err_disable  = uart_aesc_irq_err_disable,
	.irq_is_pending   = uart_aesc_irq_is_pending,
	.irq_callback_set = uart_aesc_irq_callback_set,
#endif
};

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define AESC_UART_IRQ_INIT(no)						     \
	static void uart_aesc_irq_config_##no(const struct device *dev)	     \
	{								     \
		IRQ_CONNECT(DT_INST_IRQN(no),				     \
			    DT_INST_IRQ(no, priority),			     \
			    uart_aesc_isr,				     \
			    DEVICE_DT_INST_GET(no),			     \
			    0);						     \
		irq_enable(DT_INST_IRQN(no));				     \
	}
#define AESC_UART_IRQ_CFG(no) .irq_config = uart_aesc_irq_config_##no,
#else
#define AESC_UART_IRQ_INIT(no)
#define AESC_UART_IRQ_CFG(no)
#endif

#define AESC_UART_INIT(no)						     \
	PINCTRL_DT_INST_DEFINE(no);					     \
	AESC_UART_IRQ_INIT(no)						     \
	static struct uart_aesc_data uart_aesc_dev_data_##no;		     \
	static const struct uart_aesc_config uart_aesc_dev_cfg_##no = {	     \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(no)),			     \
		.sys_clk_freq =						     \
			DT_PROP(DT_INST(no, aesc_uart), clock_frequency),    \
		.current_speed =					     \
			DT_PROP(DT_INST(no, aesc_uart), current_speed),	     \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(no),		     \
		AESC_UART_IRQ_CFG(no)					     \
	};								     \
	DEVICE_DT_INST_DEFINE(no,					     \
			      uart_aesc_init,				     \
			      NULL,					     \
			      &uart_aesc_dev_data_##no,			     \
			      &uart_aesc_dev_cfg_##no,			     \
			      PRE_KERNEL_1,				     \
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE,	     \
			      (void *)&uart_aesc_driver_api);

DT_INST_FOREACH_STATUS_OKAY(AESC_UART_INIT)
