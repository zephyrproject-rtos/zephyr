/*
 * Copyright (c) 2026 MediaTek Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/irq.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/util.h>

#include "uart_mtk_common.h"

#define UART_OFFSET_RBR          0x0000 /* (RO) RX buffer  if (DLAB == 0) */
#define UART_OFFSET_THR          0x0000 /* (WO) TX holding buffer  if (DLAB == 0) */
#define UART_OFFSET_DLL          0x0000 /* (RW) Divisor latch (LSB)  if (DLAB == 1) */
#define UART_OFFSET_DLM          0x0004 /* (RW) Divisor latch (MSB)  if (DLAB == 1) */
#define UART_OFFSET_IER          0x0004 /* (RW) Interrupt enable */
#define UART_OFFSET_IIR          0x0008 /* (RO) Interrupt identification */
#define UART_OFFSET_FCR          0x0008 /* (WO) FIFO control */
#define UART_OFFSET_LCR          0x000c /* (RW) Line control */
#define UART_OFFSET_MCR          0x0010 /* (RW) Modem control */
#define UART_OFFSET_LSR          0x0014 /* (RO) Line status */
#define UART_OFFSET_MSR          0x0018 /* (RW) Modem status */
#define UART_OFFSET_SCR          0x001c /* (RW) Scratch */
#define UART_OFFSET_AUTOBAUD_EN  0x0020 /* (RW) Autobaud detect enable */
#define UART_OFFSET_HIGH_SPEED   0x0024 /* (RW) Highspeed */
#define UART_OFFSET_SAMPLE_COUNT 0x0028 /* (RW) Sample count */
#define UART_OFFSET_SAMPLE_POINT 0x002c /* (RW) Sample point */
#define UART_OFFSET_AUTOBAUD     0x0030 /* (RO) Autobaud monitor */
#define UART_OFFSET_FRACDIV_L    0x0054 /* (RW) Fractional divider (LSB) */
#define UART_OFFSET_FRACDIV_M    0x0058 /* (RW) Fractional divider (MSB) */
#define UART_OFFSET_EFR          0x0098 /* (RW) Enhanced feature */
#define UART_OFFSET_XON_1        0x00a0 /* (RW) XON_1 */
#define UART_OFFSET_XON_2        0x00a4 /* (RW) XON_2 */
#define UART_OFFSET_XOFF_1       0x00a8 /* (RW) XOFF_1 */
#define UART_OFFSET_XOFF_2       0x00ac /* (RW) XOFF_2 */

#define UART_IER_TX_INTERRUPT (0x02)
#define UART_IER_RX_INTERRUPT (0x01)

#define UART_IIR_RX_INTERRUPT (0x04)
#define UART_IIR_TX_INTERRUPT (0x02)

/* FCR register mask and values */
#define UART_FCR_RX_FIFO_1    (0x00)
#define UART_FCR_RX_FIFO_6    (0x40)
#define UART_FCR_RX_FIFO_12   (0x80)
#define UART_FCR_RX_FIFO_TRIG (0xc0)
#define UART_FCR_TX_FIFO_1    (0x00)
#define UART_FCR_TX_FIFO_4    (0x10)
#define UART_FCR_TX_FIFO_8    (0x20)
#define UART_FCR_TX_FIFO_14   (0x30)
#define UART_FCR_CLR_TX_FIFO  (0x04)
#define UART_FCR_CLR_RX_FIFO  (0x02)
#define UART_FCR_FIFO_EN      (0x01)

/* LCR register mask and values */
#define UART_LCR_DLAB            (0x80) /* LCR [7:7] */
#define UART_LCR_PARITY          (0x38) /* LCR [5:3] */
#define UART_LCR_PARITY_NONE     (0x00)
#define UART_LCR_PARITY_ODD      (0x08)
#define UART_LCR_PARITY_EVEN     (0x18)
#define UART_LCR_PARITY_MARK     (0x28)
#define UART_LCR_PARITY_SPACE    (0x38)
#define UART_LCR_STOP_BITS       (0x04) /* LCR [2:2] */
#define UART_LCR_STOP_BITS_1     (0x00)
#define UART_LCR_STOP_BITS_2     (0x04)
#define UART_LCR_WORD_LEN        (0x03) /* LCR [1:0] */
#define UART_LCR_WORD_LEN_5_BITS (0x00)
#define UART_LCR_WORD_LEN_6_BITS (0x01)
#define UART_LCR_WORD_LEN_7_BITS (0x02)
#define UART_LCR_WORD_LEN_8_BITS (0x03)

/* LSR register mask and values */
#define UART_LSR_TX_IDLE    (0x40) /* LSR [6:6] */
#define UART_LSR_TX_EMPTY   (0x20) /* LSR [5:5] */
#define UART_LSR_RX_BREAK   (0x10) /* LSR [4:4] */
#define UART_LSR_RX_FRAME   (0x08) /* LSR [3:3] */
#define UART_LSR_RX_PARITY  (0x04) /* LSR [2:2] */
#define UART_LSR_RX_OVERRUN (0x02) /* LSR [1:1] */
#define UART_LSR_RX_READY   (0x01) /* LSR [0:0] */

static const uint32_t valid_baudrates[] = {
	50,     75,     110,    134,    150,     200,     300,     600,    1200,
	1800,   2400,   4800,   9600,   19200,   38400,   57600,   115200, 230400,
	460800, 500000, 576000, 921600, 1000000, 1152000, 1500000, 2000000};

static const size_t valid_baudrates_size = ARRAY_SIZE(valid_baudrates);

static const uint8_t fraction_L_map[] = {0, 1, 5, 21, 85, 87, 87, 119, 127, 255, 255};

static const uint8_t fraction_M_map[] = {0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 3};

static bool is_rx_avail(mem_addr_t base);
static bool is_tx_avail(mem_addr_t base);

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
static void set_dl(const struct device *dev, uint32_t divisor);

static int set_baudrate(const struct device *dev, const struct uart_config *cfg);
static int set_parity(const struct device *dev, const struct uart_config *cfg);
static int set_stop_bits(const struct device *dev, const struct uart_config *cfg);
static int set_data_bits(const struct device *dev, const struct uart_config *cfg);
#endif

static bool is_rx_avail(mem_addr_t base)
{
	return (sys_read32(base + UART_OFFSET_LSR) & UART_LSR_RX_READY) != 0;
}

static bool is_tx_avail(mem_addr_t base)
{
	return (sys_read32(base + UART_OFFSET_LSR) & UART_LSR_TX_EMPTY) != 0;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
/* The following function must only be called with the lock grabed. */
static void set_dl(const struct device *dev, uint32_t divisor)
{
	uint8_t lcr = (uint8_t)sys_read32(DEVICE_MMIO_GET(dev) + UART_OFFSET_LCR);

	/* Set DLAB in LCR */
	sys_write32((uint32_t)(lcr | UART_LCR_DLAB), (DEVICE_MMIO_GET(dev) + UART_OFFSET_LCR));

	sys_write32((divisor & 0x000000ff), (DEVICE_MMIO_GET(dev) + UART_OFFSET_DLL));
	sys_write32(((divisor >> 8) & 0x000000ff), (DEVICE_MMIO_GET(dev) + UART_OFFSET_DLM));

	/* Restore LCR. */
	sys_write32((uint32_t)lcr, (DEVICE_MMIO_GET(dev) + UART_OFFSET_LCR));
}

/* The following function must only be called with the lock grabed. */
static int set_baudrate(const struct device *dev, const struct uart_config *cfg)
{
	size_t idx = 0;
	uint32_t divisor;
	const uart_mtk_config_t *uart_config = dev->config;

	/* Check baudrate against valid list. */
	while ((idx < valid_baudrates_size) && (cfg->baudrate != valid_baudrates[idx])) {
		idx++;
	}

	if (idx >= valid_baudrates_size) {
		return -EINVAL;
	}

	if (cfg->baudrate < 115200) {
		divisor = DIV_ROUND_CLOSEST(uart_config->clock_freq, (16 * cfg->baudrate));

		sys_write32((uint32_t)0x00, (DEVICE_MMIO_GET(dev) + UART_OFFSET_HIGH_SPEED));

		set_dl(dev, divisor);

		sys_write32((uint32_t)0x00, (DEVICE_MMIO_GET(dev) + UART_OFFSET_SAMPLE_COUNT));
		sys_write32((uint32_t)0xff, (DEVICE_MMIO_GET(dev) + UART_OFFSET_SAMPLE_POINT));

		sys_write32((uint32_t)0x00, (DEVICE_MMIO_GET(dev) + UART_OFFSET_FRACDIV_L));
		sys_write32((uint32_t)0x00, (DEVICE_MMIO_GET(dev) + UART_OFFSET_FRACDIV_M));
	} else {
		uint32_t sample_count;
		uint32_t fraction;

		divisor = DIV_ROUND_UP(uart_config->clock_freq, (256 * cfg->baudrate));

		sample_count = (uart_config->clock_freq / (cfg->baudrate * divisor)) - 1;

		fraction = ((uart_config->clock_freq * 100) / cfg->baudrate / divisor) % 100;
		fraction = DIV_ROUND_CLOSEST(fraction, 10);

		sys_write32((uint32_t)0x03, (DEVICE_MMIO_GET(dev) + UART_OFFSET_HIGH_SPEED));

		set_dl(dev, divisor);

		sys_write32(sample_count, (DEVICE_MMIO_GET(dev) + UART_OFFSET_SAMPLE_COUNT));
		sys_write32(((sample_count >> 1) - 1),
			    (DEVICE_MMIO_GET(dev) + UART_OFFSET_SAMPLE_POINT));

		sys_write32((uint32_t)fraction_L_map[fraction],
			    (DEVICE_MMIO_GET(dev) + UART_OFFSET_FRACDIV_L));
		sys_write32((uint32_t)fraction_M_map[fraction],
			    (DEVICE_MMIO_GET(dev) + UART_OFFSET_FRACDIV_M));
	}

	return 0;
}

/* The following function must only be called with the lock grabed. */
static int set_parity(const struct device *dev, const struct uart_config *cfg)
{
	uint8_t lcr = (uint8_t)sys_read32(DEVICE_MMIO_GET(dev) + UART_OFFSET_LCR);

	lcr &= (~(UART_LCR_PARITY));

	switch (cfg->parity) {
	case UART_CFG_PARITY_NONE:
		lcr |= UART_LCR_PARITY_NONE;
		break;

	case UART_CFG_PARITY_ODD:
		lcr |= UART_LCR_PARITY_ODD;
		break;

	case UART_CFG_PARITY_EVEN:
		lcr |= UART_LCR_PARITY_EVEN;
		break;

	case UART_CFG_PARITY_MARK:
		lcr |= UART_LCR_PARITY_MARK;
		break;

	case UART_CFG_PARITY_SPACE:
		lcr |= UART_LCR_PARITY_SPACE;
		break;

	default:
		return -EINVAL;
	}

	sys_write32((uint32_t)lcr, (DEVICE_MMIO_GET(dev) + UART_OFFSET_LCR));

	return 0;
}

/* The following function must only be called with the lock grabed. */
static int set_stop_bits(const struct device *dev, const struct uart_config *cfg)
{
	uint8_t lcr = (uint8_t)sys_read32(DEVICE_MMIO_GET(dev) + UART_OFFSET_LCR);

	lcr &= (~(UART_LCR_STOP_BITS));

	switch (cfg->stop_bits) {
	case UART_CFG_STOP_BITS_1:
		lcr |= UART_LCR_STOP_BITS_1;
		break;

	case UART_CFG_STOP_BITS_2:
		lcr |= UART_LCR_STOP_BITS_2;
		break;

	default:
		return -EINVAL;
	}

	sys_write32((uint32_t)lcr, (DEVICE_MMIO_GET(dev) + UART_OFFSET_LCR));

	return 0;
}

/* The following function must only be called with the lock grabed. */
static int set_data_bits(const struct device *dev, const struct uart_config *cfg)
{
	uint8_t lcr = (uint8_t)sys_read32(DEVICE_MMIO_GET(dev) + UART_OFFSET_LCR);

	lcr &= (~(UART_LCR_WORD_LEN));

	switch (cfg->data_bits) {
	case UART_CFG_DATA_BITS_5:
		lcr |= UART_LCR_WORD_LEN_5_BITS;
		break;

	case UART_CFG_DATA_BITS_6:
		lcr |= UART_LCR_WORD_LEN_6_BITS;
		break;

	case UART_CFG_DATA_BITS_7:
		lcr |= UART_LCR_WORD_LEN_7_BITS;
		break;

	case UART_CFG_DATA_BITS_8:
		lcr |= UART_LCR_WORD_LEN_8_BITS;
		break;

	default:
		return -EINVAL;
	}

	sys_write32((uint32_t)lcr, (DEVICE_MMIO_GET(dev) + UART_OFFSET_LCR));

	return 0;
}
#endif

void uart_mtk_poll_out(const struct device *dev, unsigned char c)
{
	uart_mtk_data_t *uart_data = dev->data;
	k_spinlock_key_t key;

	key = k_spin_lock(&(uart_data->lock));

	/* Wait until there is space in the FIFO */
	while (!(is_tx_avail(DEVICE_MMIO_GET(dev)))) {
		/* Ensure there is always some time for other threads. */
		k_spin_unlock(&(uart_data->lock), key);
		key = k_spin_lock(&(uart_data->lock));
	}

	/* Send the character */
	sys_write32((uint32_t)c, (DEVICE_MMIO_GET(dev) + UART_OFFSET_THR));

	k_spin_unlock(&(uart_data->lock), key);
}

int uart_mtk_poll_in(const struct device *dev, unsigned char *c)
{
	uart_mtk_data_t *uart_data = dev->data;
	k_spinlock_key_t key;

	key = k_spin_lock(&(uart_data->lock));

	if (!(is_rx_avail(DEVICE_MMIO_GET(dev)))) {
		k_spin_unlock(&(uart_data->lock), key);

		return -1;
	}

	(*c) = (unsigned char)(sys_read32(DEVICE_MMIO_GET(dev) + UART_OFFSET_RBR) & 0x000000ff);

	k_spin_unlock(&(uart_data->lock), key);

	return 0;
}

int uart_mtk_err_check(const struct device *dev)
{
	int errors = 0;
	uint8_t lsr = (uint8_t)sys_read32(DEVICE_MMIO_GET(dev) + UART_OFFSET_LSR);

	if (lsr & UART_LSR_RX_BREAK) {
		errors |= UART_BREAK;
	}

	if (lsr & UART_LSR_RX_FRAME) {
		errors |= UART_ERROR_FRAMING;
	}

	if (lsr & UART_LSR_RX_PARITY) {
		errors |= UART_ERROR_PARITY;
	}

	if (lsr & UART_LSR_RX_OVERRUN) {
		errors |= UART_ERROR_OVERRUN;
	}

	return errors;
}

#ifdef CONFIG_UART_USE_RUNTIME_CONFIGURE
int uart_mtk_configure(const struct device *dev, const struct uart_config *cfg)
{
	int ret;
	uart_mtk_data_t *uart_data = dev->data;
	k_spinlock_key_t key;

	key = k_spin_lock(&(uart_data->lock));

	ret = 0;
	set_baudrate(dev, cfg);
	if (ret != 0) {
		k_spin_unlock(&(uart_data->lock), key);
		return ret;
	}

	ret = set_parity(dev, cfg);
	if (ret != 0) {
		k_spin_unlock(&(uart_data->lock), key);
		return ret;
	}

	ret = set_stop_bits(dev, cfg);
	if (ret != 0) {
		k_spin_unlock(&(uart_data->lock), key);
		return ret;
	}

	ret = set_data_bits(dev, cfg);
	if (ret != 0) {
		k_spin_unlock(&(uart_data->lock), key);
		return ret;
	}

	memcpy(&(uart_data->uart_cfg), cfg, sizeof(struct uart_config));
	uart_data->uart_cfg.flow_ctrl = UART_CFG_FLOW_CTRL_NONE;

	k_spin_unlock(&(uart_data->lock), key);

	return 0;
}

int uart_mtk_config_get(const struct device *dev, struct uart_config *cfg)
{
	uart_mtk_data_t *uart_data = dev->data;

	memcpy(cfg, &(uart_data->uart_cfg), sizeof(struct uart_config));

	return 0;
}
#endif

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
int uart_mtk_fifo_fill(const struct device *dev, const uint8_t *tx_data, int size)
{
	int idx = 0;
	uart_mtk_data_t *uart_data = dev->data;
	k_spinlock_key_t key;

	key = k_spin_lock(&(uart_data->lock));

	while ((idx < size) && (is_tx_avail(DEVICE_MMIO_GET(dev)))) {
		/* Send the character */
		sys_write32((uint32_t)(tx_data[idx]), (DEVICE_MMIO_GET(dev) + UART_OFFSET_THR));

		idx++;
	}

	k_spin_unlock(&(uart_data->lock), key);

	return idx;
}

int uart_mtk_fifo_read(const struct device *dev, uint8_t *rx_data, const int size)
{
	int idx = 0;
	uart_mtk_data_t *uart_data = dev->data;
	k_spinlock_key_t key;

	key = k_spin_lock(&(uart_data->lock));

	while ((idx < size) && (is_rx_avail(DEVICE_MMIO_GET(dev)))) {
		rx_data[idx] =
			(uint8_t)(sys_read32(DEVICE_MMIO_GET(dev) + UART_OFFSET_RBR) & 0x000000ff);

		idx++;
	}

	k_spin_unlock(&(uart_data->lock), key);

	return idx;
}

void uart_mtk_irq_tx_enable(const struct device *dev)
{
	uint8_t ier;
	uart_mtk_data_t *uart_data = dev->data;
	k_spinlock_key_t key;

	key = k_spin_lock(&(uart_data->lock));

	ier = (uint8_t)sys_read32(DEVICE_MMIO_GET(dev) + UART_OFFSET_IER);

	if ((ier & UART_IER_TX_INTERRUPT) == 0) {
		sys_write32((uint32_t)(ier | UART_IER_TX_INTERRUPT),
			    (DEVICE_MMIO_GET(dev) + UART_OFFSET_IER));
	}

	k_spin_unlock(&(uart_data->lock), key);
}

void uart_mtk_irq_tx_disable(const struct device *dev)
{
	uint8_t ier;
	uart_mtk_data_t *uart_data = dev->data;
	k_spinlock_key_t key;

	key = k_spin_lock(&(uart_data->lock));

	ier = (uint8_t)sys_read32(DEVICE_MMIO_GET(dev) + UART_OFFSET_IER);

	if ((ier & UART_IER_TX_INTERRUPT) != 0) {
		sys_write32((uint32_t)(ier & (~(UART_IER_TX_INTERRUPT))),
			    (DEVICE_MMIO_GET(dev) + UART_OFFSET_IER));
	}

	k_spin_unlock(&(uart_data->lock), key);
}

int uart_mtk_irq_tx_ready(const struct device *dev)
{
	int tx_is_ready;
	uart_mtk_data_t *uart_data = dev->data;
	k_spinlock_key_t key;

	key = k_spin_lock(&(uart_data->lock));

	tx_is_ready = (is_tx_avail(DEVICE_MMIO_GET(dev)) ? 1 : 0);

	k_spin_unlock(&(uart_data->lock), key);

	return tx_is_ready;
}

void uart_mtk_irq_rx_enable(const struct device *dev)
{
	uint8_t ier;
	uart_mtk_data_t *uart_data = dev->data;
	k_spinlock_key_t key;

	key = k_spin_lock(&(uart_data->lock));

	ier = (uint8_t)sys_read32(DEVICE_MMIO_GET(dev) + UART_OFFSET_IER);

	if ((ier & UART_IER_RX_INTERRUPT) == 0) {
		sys_write32((uint32_t)(ier | UART_IER_RX_INTERRUPT),
			    (DEVICE_MMIO_GET(dev) + UART_OFFSET_IER));
	}

	k_spin_unlock(&(uart_data->lock), key);
}

void uart_mtk_irq_rx_disable(const struct device *dev)
{
	uint8_t ier;
	uart_mtk_data_t *uart_data = dev->data;
	k_spinlock_key_t key;

	key = k_spin_lock(&(uart_data->lock));

	ier = (uint8_t)sys_read32(DEVICE_MMIO_GET(dev) + UART_OFFSET_IER);

	if ((ier & UART_IER_RX_INTERRUPT) != 0) {
		sys_write32((uint32_t)(ier & (~(UART_IER_RX_INTERRUPT))),
			    (DEVICE_MMIO_GET(dev) + UART_OFFSET_IER));
	}

	k_spin_unlock(&(uart_data->lock), key);
}

int uart_mtk_irq_rx_ready(const struct device *dev)
{
	int rx_is_ready;
	uart_mtk_data_t *uart_data = dev->data;
	k_spinlock_key_t key;

	key = k_spin_lock(&(uart_data->lock));

	rx_is_ready = (is_rx_avail(DEVICE_MMIO_GET(dev)) ? 1 : 0);

	k_spin_unlock(&(uart_data->lock), key);

	return rx_is_ready;
}

int uart_mtk_irq_is_pending(const struct device *dev)
{
	uint8_t iir;
	uart_mtk_data_t *uart_data = dev->data;
	k_spinlock_key_t key;

	key = k_spin_lock(&(uart_data->lock));

	iir = (uint8_t)sys_read32(DEVICE_MMIO_GET(dev) + UART_OFFSET_IIR);

	k_spin_unlock(&(uart_data->lock), key);

	return (iir & (UART_IIR_RX_INTERRUPT | UART_IIR_TX_INTERRUPT)) != 0;
}

void uart_mtk_irq_update(const struct device *dev)
{
	ARG_UNUSED(dev);
}

void uart_mtk_irq_callback_set(const struct device *dev, uart_irq_callback_user_data_t cb,
			       void *cb_data)
{
	uart_mtk_data_t *uart_data = dev->data;
	k_spinlock_key_t key;

	key = k_spin_lock(&(uart_data->lock));

	uart_data->callback = cb;
	uart_data->cb_data = cb_data;

	k_spin_unlock(&(uart_data->lock), key);
}

void uart_mtk_isr(const struct device *dev)
{
	uart_mtk_data_t *uart_data = dev->data;

	if (uart_data->callback != NULL) {
		uart_data->callback(dev, uart_data->cb_data);
	}
}
#endif

int uart_mtk_init(const struct device *dev)
{
	int ret;
	uart_mtk_data_t *uart_data = dev->data;
	const uart_mtk_config_t *uart_config = dev->config;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

#ifdef CONFIG_CLOCK_CONTROL
	ret = clock_control_on(uart_config->clock_dev, uart_config->clock_subsys);
	if (ret < 0) {
		return ret;
	}
#endif

	sys_write32((uint32_t)(UART_FCR_RX_FIFO_1 | UART_FCR_TX_FIFO_4 | UART_FCR_CLR_TX_FIFO |
			       UART_FCR_CLR_RX_FIFO | UART_FCR_FIFO_EN),
		    (DEVICE_MMIO_GET(dev) + UART_OFFSET_FCR));

	uart_mtk_configure(dev, &(uart_data->uart_cfg));

#ifdef CONFIG_PINCTRL
	ret = pinctrl_apply_state(uart_config->pinctrl_config, PINCTRL_STATE_DEFAULT);
	if (ret < 0) {
		return ret;
	}
#endif

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	uart_config->irq_config_func(dev);
#endif
	return 0;
}
