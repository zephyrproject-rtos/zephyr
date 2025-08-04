/*
 * Copyright (c) 2021-2025 ATL Electronics
 * Copyright (c) 2024-2025, MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT bflb_uart

/**
 * @brief UART driver for Bouffalo Lab MCU family.
 */

#include <zephyr/drivers/uart.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/pm/device.h>
#include <zephyr/arch/common/sys_io.h>
#include <zephyr/drivers/timer/system_timer.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/irq.h>

#include <zephyr/dt-bindings/clock/bflb_clock_common.h>


#include <soc.h>
#include <bflb_soc.h>
#include <glb_reg.h>
#include <common_defines.h>
#include <bouffalolab/common/uart_reg.h>
#include <bouffalolab/common/bflb_uart.h>



struct bflb_config {
	const struct pinctrl_dev_config *pincfg;
	uint32_t baudrate;
	uint8_t direction;
	uint8_t data_bits;
	uint8_t stop_bits;
	uint8_t parity;
	uint8_t bit_order;
	uint8_t flow_ctrl;
	uint8_t tx_fifo_threshold;
	uint8_t rx_fifo_threshold;
	uint32_t base_reg;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_config_func_t irq_config_func;
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

struct bflb_data {
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t user_cb;
	void *user_data;
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

static void uart_bflb_enabled(const struct device *dev, uint32_t enable)
{
	const struct bflb_config *cfg = dev->config;
	uint32_t rxt = 0;
	uint32_t txt = 0;

	if (enable > 1) {
		enable = 1;
	}

	txt = sys_read32(cfg->base_reg + UART_UTX_CONFIG_OFFSET);
	txt = (txt & ~UART_CR_UTX_EN) | enable;
	rxt = sys_read32(cfg->base_reg + UART_URX_CONFIG_OFFSET);
	rxt = (rxt & ~UART_CR_URX_EN) | enable;
	sys_write32(rxt, cfg->base_reg + UART_URX_CONFIG_OFFSET);
	sys_write32(txt, cfg->base_reg + UART_UTX_CONFIG_OFFSET);
}

static uint32_t uart_bflb_get_clock(void)
{
	uint32_t uart_divider = 0;
	uint32_t uclk;
	const struct device *clock_ctrl =  DEVICE_DT_GET_ANY(bflb_clock_controller);

#if defined(CONFIG_SOC_SERIES_BL60X) || defined(CONFIG_SOC_SERIES_BL70X)
	uart_divider = sys_read32(GLB_BASE + GLB_CLK_CFG2_OFFSET);
	uart_divider = (uart_divider & GLB_UART_CLK_DIV_MSK) >> GLB_UART_CLK_DIV_POS;
	clock_control_get_rate(clock_ctrl, (void *)BFLB_CLKID_CLK_ROOT, &uclk);
#else
	uart_divider = sys_read32(GLB_BASE + GLB_UART_CFG0_OFFSET);
	uart_divider = (uart_divider & GLB_UART_CLK_DIV_MSK) >> GLB_UART_CLK_DIV_POS;
	clock_control_get_rate(clock_ctrl, (void *)BFLB_CLKID_CLK_BCLK, &uclk);
#endif

	return uclk / (uart_divider + 1);
}


#ifdef CONFIG_UART_INTERRUPT_DRIVEN

static int uart_bflb_err_check(const struct device *dev)
{
	const struct bflb_config *const cfg = dev->config;
	uint32_t status = 0;
	uint32_t tmp = 0;
	int errors = 0;

	status = sys_read32(cfg->base_reg + UART_INT_STS_OFFSET);
	tmp = sys_read32(cfg->base_reg + UART_INT_CLEAR_OFFSET);

	if (status & UART_URX_FER_INT) {
		errors |= UART_ERROR_OVERRUN;
	}

	if (status & UART_UTX_FER_INT) {
		errors |= UART_ERROR_OVERRUN;
	}

	if (status & UART_URX_PCE_INT) {
		tmp |= UART_CR_URX_PCE_CLR;

		errors |= UART_ERROR_PARITY;
	}

	sys_write32(tmp, cfg->base_reg + UART_INT_CLEAR_OFFSET);

	return errors;
}

int uart_bflb_irq_tx_ready(const struct device *dev)
{
	const struct bflb_config *const cfg = dev->config;
	uint32_t tmp = 0;

	tmp = sys_read32(cfg->base_reg + UART_FIFO_CONFIG_1_OFFSET);
	return (tmp & UART_TX_FIFO_CNT_MASK) > 0;
}

int uart_bflb_irq_rx_ready(const struct device *dev)
{
	const struct bflb_config *const cfg = dev->config;
	uint32_t tmp = 0;

	tmp = sys_read32(cfg->base_reg + UART_FIFO_CONFIG_1_OFFSET);
	return (tmp & UART_RX_FIFO_CNT_MASK) > 0;
}

int uart_bflb_fifo_fill(const struct device *dev, const uint8_t *tx_data, int len)
{
	const struct bflb_config *const cfg = dev->config;
	uint8_t num_tx = 0U;

	while (num_tx < len && uart_bflb_irq_tx_ready(dev) > 0) {
		sys_write32(tx_data[num_tx++], cfg->base_reg + UART_FIFO_WDATA_OFFSET);
	}

	return num_tx;
}

int uart_bflb_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	const struct bflb_config *const cfg = dev->config;
	uint8_t num_rx = 0U;

	while ((num_rx < size) && uart_bflb_irq_rx_ready(dev) > 0) {
		rx_data[num_rx++] = sys_read32(cfg->base_reg + UART_FIFO_RDATA_OFFSET);
	}

	return num_rx;
}

void uart_bflb_irq_tx_enable(const struct device *dev)
{
	const struct bflb_config *const cfg = dev->config;
	uint32_t tmp = 0;

	tmp = sys_read32(cfg->base_reg + UART_INT_MASK_OFFSET);
	tmp = tmp & ~UART_CR_UTX_FIFO_MASK;
	sys_write32(tmp, cfg->base_reg + UART_INT_MASK_OFFSET);
}

void uart_bflb_irq_tx_disable(const struct device *dev)
{
	const struct bflb_config *const cfg = dev->config;
	uint32_t tmp = 0;

	tmp = sys_read32(cfg->base_reg + UART_INT_MASK_OFFSET);
	tmp = tmp | UART_CR_UTX_FIFO_MASK;
	sys_write32(tmp, cfg->base_reg + UART_INT_MASK_OFFSET);
}


int uart_bflb_irq_tx_complete(const struct device *dev)
{
	const struct bflb_config *const cfg = dev->config;
	uint32_t tmp;

	tmp = sys_read32(cfg->base_reg + UART_STATUS_OFFSET);
	if (tmp & UART_STS_UTX_BUS_BUSY) {
		return 0;
	}

	tmp = sys_read32(cfg->base_reg + UART_FIFO_CONFIG_1_OFFSET);
	if ((tmp & UART_TX_FIFO_CNT_MASK) >= cfg->tx_fifo_threshold) {
		return 1;
	}
	return 0;
}

void uart_bflb_irq_rx_enable(const struct device *dev)
{
	const struct bflb_config *const cfg = dev->config;
	uint32_t tmp = 0;

	tmp = sys_read32(cfg->base_reg + UART_INT_MASK_OFFSET);
	tmp = tmp & ~UART_CR_URX_FIFO_MASK;
	sys_write32(tmp, cfg->base_reg + UART_INT_MASK_OFFSET);
}

void uart_bflb_irq_rx_disable(const struct device *dev)
{
	const struct bflb_config *const cfg = dev->config;
	uint32_t tmp = 0;

	tmp = sys_read32(cfg->base_reg + UART_INT_MASK_OFFSET);
	tmp = tmp | UART_CR_URX_FIFO_MASK;
	sys_write32(tmp, cfg->base_reg + UART_INT_MASK_OFFSET);
}

void uart_bflb_irq_err_enable(const struct device *dev)
{
	const struct bflb_config *const cfg = dev->config;
	uint32_t tmp = 0;

	tmp = sys_read32(cfg->base_reg + UART_INT_MASK_OFFSET);
	tmp = tmp & ~UART_CR_URX_PCE_MASK;
	tmp = tmp & ~UART_CR_UTX_FER_MASK;
	tmp = tmp & ~UART_CR_URX_FER_MASK;
	sys_write32(tmp, cfg->base_reg + UART_INT_MASK_OFFSET);
}

void uart_bflb_irq_err_disable(const struct device *dev)
{
	const struct bflb_config *const cfg = dev->config;
	uint32_t tmp = 0;

	tmp = sys_read32(cfg->base_reg + UART_INT_MASK_OFFSET);
	tmp = tmp | UART_CR_URX_PCE_MASK;
	tmp = tmp | UART_CR_UTX_FER_MASK;
	tmp = tmp | UART_CR_URX_FER_MASK;
	sys_write32(tmp, cfg->base_reg + UART_INT_MASK_OFFSET);
}

int uart_bflb_irq_is_pending(const struct device *dev)
{
	const struct bflb_config *const cfg = dev->config;
	uint32_t tmp = sys_read32(cfg->base_reg + UART_INT_STS_OFFSET);
	uint32_t maskVal = sys_read32(cfg->base_reg + UART_INT_MASK_OFFSET);

	/* only first 8 bits are not reserved */
	return (((tmp & ~maskVal) & 0xFF) != 0 ? 1 : 0);
}

int uart_bflb_irq_update(const struct device *dev)
{
	ARG_UNUSED(dev);

	return 1;
}

void uart_bflb_irq_callback_set(const struct device *dev,
			      uart_irq_callback_user_data_t cb,
			      void *user_data)
{
	struct bflb_data *const data = dev->data;

	data->user_cb = cb;
	data->user_data = user_data;
}

static void uart_bflb_isr(const struct device *dev)
{
	struct bflb_data *const data = dev->data;
	const struct bflb_config *const cfg = dev->config;
	uint32_t tmp = 0;

	if (data->user_cb) {
		data->user_cb(dev, data->user_data);
	}
	/* clear interrupts that require ack*/
	tmp = sys_read32(cfg->base_reg + UART_INT_CLEAR_OFFSET);
	tmp = tmp | UART_CR_URX_RTO_CLR;
	sys_write32(tmp, cfg->base_reg + UART_INT_CLEAR_OFFSET);
}
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static int uart_bflb_configure(const struct device *dev)
{
	const struct bflb_config *cfg = dev->config;
	uint32_t tx_cfg = 0;
	uint32_t rx_cfg = 0;
	uint32_t divider = 0;
	uint32_t tmp = 0;

	divider = (uart_bflb_get_clock() * 10 / cfg->baudrate + 5) / 10;
	if (divider >= 0xFFFF) {
		divider = 0xFFFF - 1;
	}

	uart_bflb_enabled(dev, 0);

	sys_write32(((divider - 1) << 0x10) | ((divider - 1) & 0xFFFF), cfg->base_reg
+ UART_BIT_PRD_OFFSET);


	/* Configure Parity */
	tx_cfg = sys_read32(cfg->base_reg + UART_UTX_CONFIG_OFFSET);
	rx_cfg = sys_read32(cfg->base_reg + UART_URX_CONFIG_OFFSET);

	switch (cfg->parity) {
	case UART_PARITY_NONE:
		tx_cfg &= ~UART_CR_UTX_PRT_EN;
		rx_cfg &= ~UART_CR_URX_PRT_EN;
		break;
	case UART_PARITY_ODD:
		tx_cfg |= UART_CR_UTX_PRT_EN;
		tx_cfg |= UART_CR_UTX_PRT_SEL;
		rx_cfg |= UART_CR_URX_PRT_EN;
		rx_cfg |= UART_CR_URX_PRT_SEL;
		break;
	case UART_PARITY_EVEN:
		tx_cfg |= UART_CR_UTX_PRT_EN;
		tx_cfg &= ~UART_CR_UTX_PRT_SEL;
		rx_cfg |= UART_CR_URX_PRT_EN;
		rx_cfg &= ~UART_CR_URX_PRT_SEL;
		break;
	default:
		break;
	}

	/* Configure data bits */
	tx_cfg &= ~UART_CR_UTX_BIT_CNT_D_MASK;
	tx_cfg |= (cfg->data_bits + 4) << UART_CR_UTX_BIT_CNT_D_SHIFT;
	rx_cfg &= ~UART_CR_URX_BIT_CNT_D_MASK;
	rx_cfg |= (cfg->data_bits + 4) << UART_CR_URX_BIT_CNT_D_SHIFT;

	/* Configure tx stop bits */
	tx_cfg &= ~UART_CR_UTX_BIT_CNT_P_MASK;
	tx_cfg |= cfg->stop_bits << UART_CR_UTX_BIT_CNT_P_SHIFT;

	/* Configure tx cts flow control function */
	if (cfg->flow_ctrl & UART_FLOWCTRL_CTS) {
		tx_cfg |= UART_CR_UTX_CTS_EN;
	} else {
		tx_cfg &= ~UART_CR_UTX_CTS_EN;
	}

	/* disable de-glitch function */
	rx_cfg &= ~UART_CR_URX_DEG_EN;

	/* Write config */
	sys_write32(tx_cfg, cfg->base_reg + UART_UTX_CONFIG_OFFSET);
	sys_write32(rx_cfg, cfg->base_reg + UART_URX_CONFIG_OFFSET);

	/* enable hardware control RTS */
	#if defined(CONFIG_SOC_SERIES_BL60X)
	tmp = sys_read32(cfg->base_reg + UART_URX_CONFIG_OFFSET);
	tmp &= ~UART_CR_URX_RTS_SW_MODE;
	sys_write32(tmp, cfg->base_reg + UART_URX_CONFIG_OFFSET);
	#else
	tmp = sys_read32(cfg->base_reg + UART_SW_MODE_OFFSET);
	tmp &= ~UART_CR_URX_RTS_SW_MODE;
	sys_write32(tmp, cfg->base_reg + UART_SW_MODE_OFFSET);
	#endif

	/* disable inversion */
	tmp = sys_read32(cfg->base_reg + UART_DATA_CONFIG_OFFSET);
	tmp &= ~UART_CR_UART_BIT_INV;
	sys_write32(tmp, cfg->base_reg + UART_DATA_CONFIG_OFFSET);


	/* TX free run enable */
	tmp = sys_read32(cfg->base_reg + UART_UTX_CONFIG_OFFSET);
	tmp |= UART_CR_UTX_FRM_EN;
	sys_write32(tmp, cfg->base_reg + UART_UTX_CONFIG_OFFSET);


	/* Configure FIFO thresholds */
	tmp = sys_read32(cfg->base_reg + UART_FIFO_CONFIG_1_OFFSET);
	tmp &= ~UART_TX_FIFO_TH_MASK;
	tmp &= ~UART_RX_FIFO_TH_MASK;
	tmp |= (cfg->tx_fifo_threshold << UART_TX_FIFO_TH_SHIFT) & UART_TX_FIFO_TH_MASK;
	tmp |= (cfg->rx_fifo_threshold << UART_RX_FIFO_TH_SHIFT) & UART_RX_FIFO_TH_MASK;
	sys_write32(tmp, cfg->base_reg + UART_FIFO_CONFIG_1_OFFSET);

	/* Clear FIFO */
	tmp = sys_read32(cfg->base_reg + UART_FIFO_CONFIG_0_OFFSET);
	tmp |= UART_TX_FIFO_CLR;
	tmp |= UART_RX_FIFO_CLR;
	tmp &= ~UART_DMA_TX_EN;
	tmp &= ~UART_DMA_RX_EN;
	sys_write32(tmp, cfg->base_reg + UART_FIFO_CONFIG_0_OFFSET);

	return 0;
}

static int uart_bflb_init(const struct device *dev)
{
	const struct bflb_config *cfg = dev->config;
	int rc = 0;

	pinctrl_apply_state(cfg->pincfg, PINCTRL_STATE_DEFAULT);
	rc = uart_bflb_configure(dev);
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	/* disable all irqs */
	sys_write32(0x0, cfg->base_reg + UART_INT_EN_OFFSET);
	/* clear all IRQS */
	sys_write32(0xFF, cfg->base_reg + UART_INT_CLEAR_OFFSET);
	/* mask all IRQs */
	sys_write32(0xFFFFFFFFU, cfg->base_reg + UART_INT_MASK_OFFSET);
	/* unmask necessary irqs */
	uart_bflb_irq_rx_enable(dev);
	uart_bflb_irq_err_enable(dev);
	/* enable all irqs */
	sys_write32(0xFF, cfg->base_reg + UART_INT_EN_OFFSET);
	cfg->irq_config_func(dev);
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
	uart_bflb_enabled(dev, 1);
	return rc;
}

static int uart_bflb_poll_in(const struct device *dev, unsigned char *c)
{
	const struct bflb_config *cfg = dev->config;

	if ((sys_read32(cfg->base_reg + UART_FIFO_CONFIG_1_OFFSET) & UART_RX_FIFO_CNT_MASK) != 0) {
		*c = sys_read8(cfg->base_reg + UART_FIFO_RDATA_OFFSET);
		return 0;
	}

	return -1;
}

static void uart_bflb_poll_out(const struct device *dev, unsigned char c)
{
	const struct bflb_config *cfg = dev->config;

	while ((sys_read32(cfg->base_reg + UART_FIFO_CONFIG_1_OFFSET) & UART_TX_FIFO_CNT_MASK) ==
0) {
	}
	sys_write8(c, cfg->base_reg + UART_FIFO_WDATA_OFFSET);
}

#ifdef CONFIG_PM_DEVICE
static int uart_bflb_pm_control(const struct device *dev,
			      enum pm_device_action action)
{
	const struct bflb_config *cfg = dev->config;
	uint32_t tmp;
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_RESUME:
		ret = pinctrl_apply_state(cfg->pincfg, PINCTRL_STATE_DEFAULT);
		if (ret < 0) {
			return ret;
		}
		tmp = sys_read32(GLB_BASE + GLB_CGEN_CFG1_OFFSET);
		/* Ungate clock to peripheral */
		if (cfg->base_reg == UART0_BASE) {
			tmp |= (1 << 16);
		} else if (cfg->base_reg == UART1_BASE) {
			tmp |= (1 << 17);
		} else {
			return -EINVAL;
		}
		sys_write32(tmp, GLB_BASE + GLB_CGEN_CFG1_OFFSET);
		break;
	case PM_DEVICE_ACTION_SUSPEND:
		ret = pinctrl_apply_state(cfg->pincfg, PINCTRL_STATE_SLEEP);
		if (ret < 0) {
			return ret;
		}
		tmp = sys_read32(GLB_BASE + GLB_CGEN_CFG1_OFFSET);
		/* Gate clock to peripheral */
		if (cfg->base_reg == UART0_BASE) {
			tmp &= ~(1 << 16);
		} else if (cfg->base_reg == UART1_BASE) {
			tmp &= ~(1 << 17);
		} else {
			return -EINVAL;
		}
		sys_write32(tmp, GLB_BASE + GLB_CGEN_CFG1_OFFSET);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}
#endif /* CONFIG_PM_DEVICE */

static DEVICE_API(uart, uart_bflb_driver_api) = {
	.poll_in = uart_bflb_poll_in,
	.poll_out = uart_bflb_poll_out,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.err_check = uart_bflb_err_check,
	.fifo_fill = uart_bflb_fifo_fill,
	.fifo_read = uart_bflb_fifo_read,
	.irq_tx_enable = uart_bflb_irq_tx_enable,
	.irq_tx_disable = uart_bflb_irq_tx_disable,
	.irq_tx_ready = uart_bflb_irq_tx_ready,
	.irq_tx_complete = uart_bflb_irq_tx_complete,
	.irq_rx_enable = uart_bflb_irq_rx_enable,
	.irq_rx_disable = uart_bflb_irq_rx_disable,
	.irq_rx_ready = uart_bflb_irq_rx_ready,
	.irq_err_enable = uart_bflb_irq_err_enable,
	.irq_err_disable = uart_bflb_irq_err_disable,
	.irq_is_pending = uart_bflb_irq_is_pending,
	.irq_update = uart_bflb_irq_update,
	.irq_callback_set = uart_bflb_irq_callback_set,
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */
};

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define BFLB_UART_IRQ_HANDLER_DECL(instance)						\
	static void uart_bflb_config_func_##instance(const struct device *dev);
#define BFLB_UART_IRQ_HANDLER_FUNC(instance)						\
	.irq_config_func = uart_bflb_config_func_##instance
#define BFLB_UART_IRQ_HANDLER(instance)							\
	static void uart_bflb_config_func_##instance(const struct device *dev)	\
	{										\
		IRQ_CONNECT(DT_INST_IRQN(instance),					\
			    DT_INST_IRQ(instance, priority),				\
			    uart_bflb_isr,						\
			    DEVICE_DT_INST_GET(instance),				\
			    0);								\
		irq_enable(DT_INST_IRQN(instance));					\
	}
#else /* CONFIG_UART_INTERRUPT_DRIVEN */
#define BFLB_UART_IRQ_HANDLER_DECL(instance)
#define BFLB_UART_IRQ_HANDLER_FUNC(instance)
#define BFLB_UART_IRQ_HANDLER(instance)
#endif /* CONFIG_UART_INTERRUPT_DRIVEN */


#define BFLB_UART_INIT(instance)						\
	PINCTRL_DT_INST_DEFINE(instance);					\
	PM_DEVICE_DT_INST_DEFINE(instance, uart_bflb_pm_control);		\
	BFLB_UART_IRQ_HANDLER_DECL(instance)					\
	static struct bflb_data uart##instance##_bflb_data;			\
	static const struct bflb_config uart##instance##_bflb_config = {	\
		.pincfg = PINCTRL_DT_INST_DEV_CONFIG_GET(instance),		\
		.base_reg = DT_INST_REG_ADDR(instance),				\
										\
		.baudrate = DT_INST_PROP(instance, current_speed),		\
		.data_bits = UART_DATA_BITS_8,					\
		.stop_bits = UART_STOP_BITS_1,					\
		.parity = UART_PARITY_NONE,					\
		.bit_order = UART_MSB_FIRST,					\
		.flow_ctrl = UART_FLOWCTRL_NONE,				\
		/* overflow interrupt threshold, size is 32 bytes*/		\
		.tx_fifo_threshold = 8,						\
		.rx_fifo_threshold = 0,						\
		BFLB_UART_IRQ_HANDLER_FUNC(instance)				\
	};									\
	DEVICE_DT_INST_DEFINE(instance, &uart_bflb_init,			\
			      PM_DEVICE_DT_INST_GET(instance),			\
			      &uart##instance##_bflb_data,			\
			      &uart##instance##_bflb_config, PRE_KERNEL_1,	\
			      CONFIG_KERNEL_INIT_PRIORITY_DEVICE,		\
			      &uart_bflb_driver_api);				\
										\
	BFLB_UART_IRQ_HANDLER(instance)

DT_INST_FOREACH_STATUS_OKAY(BFLB_UART_INIT)
