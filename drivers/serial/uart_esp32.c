/*
 * Copyright (c) 2019 Mohamed ElShahawi (extremegtx@hotmail.com)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Include esp-idf headers first to avoid redefining BIT() macro */
#include <rom/ets_sys.h>
#include <soc/dport_reg.h>

#include <rom/gpio.h>

#include <soc/gpio_sig_map.h>

#include <device.h>
#include <soc.h>
#include <drivers/uart.h>
#include <errno.h>
#include <sys/util.h>


/*
 * ESP32 UARTx register map structure
 */
struct uart_esp32_regs_t {
	u32_t fifo;
	u32_t int_raw;
	u32_t int_st;
	u32_t int_ena;
	u32_t int_clr;
	u32_t clk_div;
	u32_t auto_baud;
	u32_t status;
	u32_t conf0;
	u32_t conf1;
	u32_t lowpulse;
	u32_t highpulse;
	u32_t rxd_cnt;
	u32_t flow_conf;
	u32_t sleep_conf;
	u32_t swfc_conf;
	u32_t idle_conf;
	u32_t rs485_conf;
	u32_t at_cmd_precnt;
	u32_t at_cmd_postcnt;
	u32_t at_cmd_gaptout;
	u32_t at_cmd_char;
	u32_t mem_conf;
	u32_t mem_tx_status;
	u32_t mem_rx_status;
	u32_t mem_cnt_status;
	u32_t pospulse;
	u32_t negpulse;
	u32_t reserved_0;
	u32_t reserved_1;
	u32_t date;
	u32_t id;
};

struct uart_esp32_config {

	struct uart_device_config dev_conf;

	const struct {
		int tx_out;
		int rx_in;
		int rts_out;
		int cts_in;
	} signals;

	const struct {
		int tx;
		int rx;
		int rts;
		int cts;
	} pins;

	const struct esp32_peripheral peripheral;

	const struct {
		int source;
		int line;
	} irq;
};

/* driver data */
struct uart_esp32_data {
	struct uart_config uart_config;
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_irq_callback_user_data_t irq_cb;
	void *irq_cb_data;
#endif
};

#define DEV_CFG(dev) \
	((const struct uart_esp32_config *const)(dev)->config->config_info)
#define DEV_DATA(dev) \
	((struct uart_esp32_data *)(dev)->driver_data)
#define DEV_BASE(dev) \
	((volatile struct uart_esp32_regs_t  *)(DEV_CFG(dev))->dev_conf.base)

#define UART_TXFIFO_COUNT(status_reg)  ((status_reg >> 16) & 0xFF)
#define UART_RXFIFO_COUNT(status_reg)  ((status_reg >> 0) & 0xFF)

#define UART_FIFO_LIMIT                 127U
#define UART_TX_FIFO_THRESH             0x1
#define UART_RX_FIFO_THRESH             0x1

#define UART_GET_PARITY_ERR(reg)        ((reg >> 2) & 0x1)
#define UART_GET_FRAME_ERR(reg)         ((reg >> 3) & 0x1)

#define UART_GET_PARITY(conf0_reg)      ((conf0_reg >> 0) & 0x1)
#define UART_GET_PARITY_EN(conf0_reg)   ((conf0_reg >> 1) & 0x1)
#define UART_GET_DATA_BITS(conf0_reg)   ((conf0_reg >> 2) & 0x3)
#define UART_GET_STOP_BITS(conf0_reg)   ((conf0_reg >> 4) & 0x3)
#define UART_GET_TX_FLOW(conf0_reg)     ((conf0_reg >> 15) & 0x1)
#define UART_GET_RX_FLOW(conf1_reg)     ((conf1_reg >> 23) & 0x1)

/* FIXME: This should be removed when interrupt support added to ESP32 dts */
#define DT_UART_ESP32_PORT_0_IRQ_0      12
#define DT_UART_ESP32_PORT_1_IRQ_0      17
#define DT_UART_ESP32_PORT_2_IRQ_0      18

/* ESP-IDF Naming is not consistent for UART0 with UART1/2 */
#define DPORT_UART0_CLK_EN DPORT_UART_CLK_EN
#define DPORT_UART0_RST DPORT_UART_RST

static int uart_esp32_poll_in(struct device *dev, unsigned char *p_char)
{

	if (UART_RXFIFO_COUNT(DEV_BASE(dev)->status) == 0) {
		return -1;
	}

	*p_char = DEV_BASE(dev)->fifo;
	return 0;
}

static void uart_esp32_poll_out(struct device *dev,
				unsigned char c)
{
	/* Wait for space in FIFO */
	while (UART_TXFIFO_COUNT(DEV_BASE(dev)->status) >= UART_FIFO_LIMIT) {
		; /* Wait */
	}

	/* Send a character */
	DEV_BASE(dev)->fifo = (u32_t)c;
}

static int uart_esp32_err_check(struct device *dev)
{
	u32_t err = UART_GET_PARITY_ERR(DEV_BASE(dev)->int_st)
		    | UART_GET_FRAME_ERR(DEV_BASE(dev)->int_st);

	return err;
}

static int uart_esp32_config_get(struct device *dev, struct uart_config *cfg)
{
	struct uart_esp32_data *data = DEV_DATA(dev);

	cfg->baudrate = data->uart_config.baudrate;

	if (UART_GET_PARITY_EN(DEV_BASE(dev)->conf0)) {
		cfg->parity = UART_GET_PARITY(DEV_BASE(dev)->conf0);
	} else {
		cfg->parity = UART_CFG_PARITY_NONE;
	}

	cfg->stop_bits = UART_GET_STOP_BITS(DEV_BASE(dev)->conf0);
	cfg->data_bits = UART_GET_DATA_BITS(DEV_BASE(dev)->conf0);

	if (UART_GET_TX_FLOW(DEV_BASE(dev)->conf0)) {
		cfg->flow_ctrl = UART_CFG_FLOW_CTRL_RTS_CTS;
	}

	if (UART_GET_RX_FLOW(DEV_BASE(dev)->conf1)) {
		cfg->flow_ctrl = UART_CFG_FLOW_CTRL_DTR_DSR;
	}
	return 0;
}

static int uart_esp32_set_baudrate(struct device *dev, int baudrate)
{
	u32_t sys_clk_freq = DEV_CFG(dev)->dev_conf.sys_clk_freq;
	u32_t clk_div = (((sys_clk_freq) << 4) / baudrate);

	while (UART_TXFIFO_COUNT(DEV_BASE(dev)->status)) {
		; /* Wait */
	}

	if (clk_div < 16) {
		return -EINVAL;
	}

	DEV_BASE(dev)->clk_div = ((clk_div >> 4) | (clk_div & 0xf));
	return 1;
}

static int uart_esp32_configure_pins(struct device *dev)
{
	const struct uart_esp32_config *const cfg = DEV_CFG(dev);

	esp32_rom_gpio_matrix_out(cfg->pins.tx,
				  cfg->signals.tx_out,
				  false,
				  false);

	esp32_rom_gpio_matrix_in(cfg->pins.rx,
				 cfg->signals.rx_in,
				 false);

	if (cfg->pins.cts) {
		esp32_rom_gpio_matrix_out(cfg->pins.cts,
					  cfg->signals.cts_in,
					  false,
					  false);
	}

	if (cfg->pins.rts) {
		esp32_rom_gpio_matrix_in(cfg->pins.rts,
					 cfg->signals.rts_out,
					 false);
	}

	return 0;
}

static int uart_esp32_configure(struct device *dev,
				const struct uart_config *cfg)
{
	u32_t conf0 = UART_TICK_REF_ALWAYS_ON;
	u32_t conf1 = (UART_RX_FIFO_THRESH << UART_RXFIFO_FULL_THRHD_S)
		      | (UART_TX_FIFO_THRESH << UART_TXFIFO_EMPTY_THRHD_S);

	uart_esp32_configure_pins(dev);
	esp32_enable_peripheral(&DEV_CFG(dev)->peripheral);

	/*
	 * Reset RX Buffer by reading all received bytes
	 * Hardware Reset functionality can't be used with UART 1/2
	 */
	while (UART_RXFIFO_COUNT(DEV_BASE(dev)->status) != 0) {
		(void) DEV_BASE(dev)->fifo;
	}

	switch (cfg->parity) {
	case UART_CFG_PARITY_NONE:
		conf0 &= ~(UART_PARITY_EN);
		conf0 &= ~(UART_PARITY);
		break;
	case UART_CFG_PARITY_EVEN:
		conf0 &= ~(UART_PARITY);
		break;
	case UART_CFG_PARITY_ODD:
		conf0 |= UART_PARITY;
		break;
	default:
		return -ENOTSUP;
	}

	switch (cfg->stop_bits) {
	case UART_CFG_STOP_BITS_1:
	case UART_CFG_STOP_BITS_1_5:
	case UART_CFG_STOP_BITS_2:
		conf0 |= cfg->stop_bits << UART_STOP_BIT_NUM_S;
		break;
	default:
		return -ENOTSUP;
	}

	if (cfg->data_bits <= UART_CFG_DATA_BITS_8) {
		conf0 |= cfg->data_bits << UART_BIT_NUM_S;
	} else {
		return -ENOTSUP;
	}

	switch (cfg->flow_ctrl) {
	case UART_CFG_FLOW_CTRL_NONE:
		conf0 &= ~(UART_TX_FLOW_EN);
		conf1 &= ~(UART_RX_FLOW_EN);
		break;
	case UART_CFG_FLOW_CTRL_RTS_CTS:
		conf0 |= UART_TX_FLOW_EN;
		conf1 |= UART_RX_FLOW_EN;
		break;
	default:
		return -ENOTSUP;
	}

	if (uart_esp32_set_baudrate(dev, cfg->baudrate)) {
		DEV_DATA(dev)->uart_config.baudrate = cfg->baudrate;
	} else {
		return -ENOTSUP;
	}

	DEV_BASE(dev)->conf0 = conf0;
	DEV_BASE(dev)->conf1 = conf1;

	return 0;
}

static int uart_esp32_init(struct device *dev)
{
	uart_esp32_configure(dev, &DEV_DATA(dev)->uart_config);

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	DEV_CFG(dev)->dev_conf.irq_config_func(dev);
#endif
	return 0;
}


#ifdef CONFIG_UART_INTERRUPT_DRIVEN

static int uart_esp32_fifo_fill(struct device *dev,
				const u8_t *tx_data, int len)
{
	u8_t num_tx = 0U;

	while ((len - num_tx > 0) &&
	       UART_TXFIFO_COUNT(DEV_BASE(dev)->status) < UART_FIFO_LIMIT) {
		DEV_BASE(dev)->fifo = (u32_t)tx_data[num_tx++];
	}

	return num_tx;
}

static int uart_esp32_fifo_read(struct device *dev,
				u8_t *rx_data, const int len)
{
	u8_t num_rx = 0U;

	while ((len - num_rx > 0) &&
	       (UART_RXFIFO_COUNT(DEV_BASE(dev)->status) != 0)) {
		rx_data[num_rx++] = DEV_BASE(dev)->fifo;
	}

	return num_rx;
}

static void uart_esp32_irq_tx_enable(struct device *dev)
{
	DEV_BASE(dev)->int_clr |= UART_TXFIFO_EMPTY_INT_ENA;
	DEV_BASE(dev)->int_ena |= UART_TXFIFO_EMPTY_INT_ENA;
}

static void uart_esp32_irq_tx_disable(struct device *dev)
{
	DEV_BASE(dev)->int_ena &= ~(UART_TXFIFO_EMPTY_INT_ENA);
}

static int uart_esp32_irq_tx_ready(struct device *dev)
{
	return (UART_TXFIFO_COUNT(DEV_BASE(dev)->status) < UART_FIFO_LIMIT);
}

static void uart_esp32_irq_rx_enable(struct device *dev)
{
	DEV_BASE(dev)->int_clr |= UART_RXFIFO_FULL_INT_ENA;
	DEV_BASE(dev)->int_ena |= UART_RXFIFO_FULL_INT_ENA;
}

static void uart_esp32_irq_rx_disable(struct device *dev)
{
	DEV_BASE(dev)->int_ena &= ~(UART_RXFIFO_FULL_INT_ENA);
}

static int uart_esp32_irq_tx_complete(struct device *dev)
{
	/* check if TX FIFO is empty */
	return (UART_TXFIFO_COUNT(DEV_BASE(dev)->status) == 0 ? 1 : 0);
}

static int uart_esp32_irq_rx_ready(struct device *dev)
{
	return (UART_RXFIFO_COUNT(DEV_BASE(dev)->status) > 0);
}

static void uart_esp32_irq_err_enable(struct device *dev)
{
	/* enable framing, parity */
	DEV_BASE(dev)->int_ena |= UART_FRM_ERR_INT_ENA
				  | UART_PARITY_ERR_INT_ENA;
}

static void uart_esp32_irq_err_disable(struct device *dev)
{
	DEV_BASE(dev)->int_ena &= ~(UART_FRM_ERR_INT_ENA);
	DEV_BASE(dev)->int_ena &= ~(UART_PARITY_ERR_INT_ENA);
}

static int uart_esp32_irq_is_pending(struct device *dev)
{
	return uart_esp32_irq_rx_ready(dev) || uart_esp32_irq_tx_ready(dev);
}

static int uart_esp32_irq_update(struct device *dev)
{
	DEV_BASE(dev)->int_clr |= UART_RXFIFO_FULL_INT_ENA;
	DEV_BASE(dev)->int_clr |= UART_TXFIFO_EMPTY_INT_ENA;

	return 1;
}

static void uart_esp32_irq_callback_set(struct device *dev,
					uart_irq_callback_user_data_t cb,
					void *cb_data)
{
	DEV_DATA(dev)->irq_cb = cb;
	DEV_DATA(dev)->irq_cb_data = cb_data;
}

void uart_esp32_isr(void *arg)
{
	struct device *dev = arg;
	struct uart_esp32_data *data = DEV_DATA(dev);

	/* Verify if the callback has been registered */
	if (data->irq_cb) {
		data->irq_cb(data->irq_cb_data);
	}
}

#endif /* CONFIG_UART_INTERRUPT_DRIVEN */

static const struct uart_driver_api uart_esp32_api = {
	.poll_in = uart_esp32_poll_in,
	.poll_out = uart_esp32_poll_out,
	.err_check = uart_esp32_err_check,
	.configure =  uart_esp32_configure,
	.config_get = uart_esp32_config_get,
#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	.fifo_fill = uart_esp32_fifo_fill,
	.fifo_read = uart_esp32_fifo_read,
	.irq_tx_enable = uart_esp32_irq_tx_enable,
	.irq_tx_disable = uart_esp32_irq_tx_disable,
	.irq_tx_ready = uart_esp32_irq_tx_ready,
	.irq_rx_enable = uart_esp32_irq_rx_enable,
	.irq_rx_disable = uart_esp32_irq_rx_disable,
	.irq_tx_complete = uart_esp32_irq_tx_complete,
	.irq_rx_ready = uart_esp32_irq_rx_ready,
	.irq_err_enable = uart_esp32_irq_err_enable,
	.irq_err_disable = uart_esp32_irq_err_disable,
	.irq_is_pending = uart_esp32_irq_is_pending,
	.irq_update = uart_esp32_irq_update,
	.irq_callback_set = uart_esp32_irq_callback_set,
#endif  /* CONFIG_UART_INTERRUPT_DRIVEN */
};


#ifdef CONFIG_UART_INTERRUPT_DRIVEN
#define ESP32_UART_IRQ_HANDLER_DECL(idx) \
	static void uart_esp32_irq_config_func_##idx(struct device *dev)

#define ESP32_UART_IRQ_HANDLER_FUNC(idx) \
	.irq_config_func = uart_esp32_irq_config_func_##idx,

#define ESP32_UART_IRQ_HANDLER(idx)					     \
	static void uart_esp32_irq_config_func_##idx(struct device *dev)     \
	{								     \
		esp32_rom_intr_matrix_set(0, ETS_UART##idx##_INTR_SOURCE,    \
					  DT_UART_ESP32_PORT_##idx##_IRQ_0); \
		IRQ_CONNECT(DT_UART_ESP32_PORT_##idx##_IRQ_0,		     \
			    1,						     \
			    uart_esp32_isr,				     \
			    DEVICE_GET(uart_esp32_##idx),		     \
			    0);						     \
		irq_enable(DT_UART_ESP32_PORT_##idx##_IRQ_0);		     \
	}
#else
#define ESP32_UART_IRQ_HANDLER_DECL(idx)
#define ESP32_UART_IRQ_HANDLER_FUNC(idx)
#define ESP32_UART_IRQ_HANDLER(idx)

#endif

#define ESP32_UART_INIT(idx)								      \
ESP32_UART_IRQ_HANDLER_DECL(idx);							      \
static const struct uart_esp32_config uart_esp32_cfg_port_##idx = {			      \
	.dev_conf = {									      \
		.base = (u8_t *)DT_INST_##idx##_ESPRESSIF_ESP32_UART_BASE_ADDRESS,	      \
		.sys_clk_freq = DT_INST_0_CADENCE_TENSILICA_XTENSA_LX6_CLOCK_FREQUENCY,	      \
		ESP32_UART_IRQ_HANDLER_FUNC(idx)					      \
	},										      \
											      \
	.peripheral = {									      \
		.clk = DPORT_UART##idx##_CLK_EN,					      \
		.rst = DPORT_UART##idx##_RST,						      \
	},										      \
											      \
	.signals = {									      \
		.tx_out = U##idx##TXD_OUT_IDX,						      \
		.rx_in = U##idx##RXD_IN_IDX,						      \
		.rts_out = U##idx##RTS_OUT_IDX,						      \
		.cts_in = U##idx##CTS_IN_IDX,						      \
	},										      \
											      \
	.pins = {									      \
		.tx = DT_INST_##idx##_ESPRESSIF_ESP32_UART_TX_PIN,			      \
		.rx = DT_INST_##idx##_ESPRESSIF_ESP32_UART_RX_PIN,			      \
		COND_CODE_1(IS_ENABLED(DT_INST_##idx##_ESPRESSIF_ESP32_UART_HW_FLOW_CONTROL), \
			    (.rts = DT_INST_##idx##_ESPRESSIF_ESP32_UART_RTS_PIN,	      \
			     .cts = DT_INST_##idx##_ESPRESSIF_ESP32_UART_CTS_PIN,	      \
			    ),								      \
			    (.rts = 0,							      \
			     .cts = 0							      \
			    ))								      \
	},										      \
											      \
	.irq = {									      \
		.source = ETS_UART##idx##_INTR_SOURCE,					      \
		.line = DT_UART_ESP32_PORT_##idx##_IRQ_0,				      \
	}										      \
};											      \
											      \
static struct uart_esp32_data uart_esp32_data_##idx = {					      \
	.uart_config = {								      \
		.baudrate = DT_INST_##idx##_ESPRESSIF_ESP32_UART_CURRENT_SPEED,		      \
		.parity = UART_CFG_PARITY_NONE,						      \
		.stop_bits = UART_CFG_STOP_BITS_1,					      \
		.data_bits = UART_CFG_DATA_BITS_8,					      \
		COND_CODE_1(IS_ENABLED(DT_INST_##idx##_ESPRESSIF_ESP32_UART_HW_FLOW_CONTROL), \
			    (.flow_ctrl = UART_CFG_FLOW_CTRL_RTS_CTS),			      \
			    (.flow_ctrl = UART_CFG_FLOW_CTRL_NONE))			      \
	}										      \
};											      \
											      \
DEVICE_AND_API_INIT(uart_esp32_##idx,							      \
		    DT_INST_##idx##_ESPRESSIF_ESP32_UART_LABEL,				      \
		    uart_esp32_init,							      \
		    &uart_esp32_data_##idx,						      \
		    &uart_esp32_cfg_port_##idx,						      \
		    PRE_KERNEL_1,							      \
		    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,					      \
		    &uart_esp32_api);							      \
											      \
ESP32_UART_IRQ_HANDLER(idx)

#ifdef DT_INST_0_ESPRESSIF_ESP32_UART
ESP32_UART_INIT(0);
#endif

#ifdef DT_INST_1_ESPRESSIF_ESP32_UART
ESP32_UART_INIT(1);
#endif

#ifdef DT_INST_2_ESPRESSIF_ESP32_UART
ESP32_UART_INIT(2);
#endif
