/*
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ene_kb1200_peci

#include <errno.h>
#include <zephyr/device.h>
#include <zephyr/drivers/peci.h>
#include <soc.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(peci_kb1200, CONFIG_EC_LOG_LEVEL);


#define PECI_PINMUX PECI_GPIO_Num


//#define CONFIG_PECI_INTERRUPT_DRIVEN  1

/* Maximum PECI core clock is the main clock 48Mhz */
#define MAX_PECI_CORE_CLOCK 48000u
/* 1 ms */
#define PECI_RESET_DELAY    1000u
#define PECI_RESET_DELAY_MS 1u
/* 100 us */
#define PECI_IDLE_DELAY     100u
/* 5 ms */
#define PECI_IDLE_TIMEOUT   50u
#define PECIST_BUS_BUSY    0x04
/* Maximum retries */
#define PECI_TIMEOUT_RETRIES 3u
/* Maximum read buffer fill wait retries */
#define PECI_RX_BUF_FILL_WAIT_RETRY 100u

/* 10 us */
#define PECI_IO_DELAY       10

#define OPT_BIT_TIME_MSB_OFS 8u

#define PECI_FCS_LEN         2

struct peci_kb1200_config {
	PECI_T *regs;
	uint8_t irq_num;
	int (*irq_init)(const struct device *dev);
};

struct peci_kb1200_data {
	struct k_sem tx_lock;
	uint32_t  bitrate;
	int    timeout_retries;
};

static int check_bus_idle(PECI_T * const regs)
{
	uint8_t delay_cnt = PECI_IDLE_TIMEOUT;

	/* Wait until PECI bus becomes idle.
	 * Note that when IDLE bit in the status register changes, HW do not
	 * generate an interrupt, so need to poll.
	 */
	while ((regs->PECIST & PECIST_BUS_BUSY)) {
		k_busy_wait(PECI_IDLE_DELAY);
		delay_cnt--;

		if (!delay_cnt) {
			LOG_WRN("Bus is busy");
			return -EBUSY;
		}
	}
	return 0;
}

static int peci_kb1200_configure(const struct device *dev, uint32_t bitrate)
{
	const struct peci_kb1200_config * const cfg = dev->config;
	struct peci_kb1200_data * const data = dev->data;
	PECI_T * const regs = cfg->regs;
	uint16_t value;

	data->bitrate = bitrate;

	if (bitrate > 250L*1024)
	{
		uint32_t factor = 8L*1024*1024 / bitrate;
		if (factor > 11) factor = 11;
		if (factor < 4) factor = 4;
		printk("%s Factor = %d, bitrate = %dHz\n", __func__, factor, bitrate);
		value = (factor - 4);

		//Output always high mode, data input debounce, PECI function enable
		// Source clock = 32MHz
		regs->PECICFG = 0x0023 | (value << 8);
	}
	else
	{
		uint32_t factor = 1L*1024*1024 / bitrate;
		if (factor > 11) factor = 11;
		if (factor < 4) factor = 4;
		printk("%s Factor = %d, bitrate = %dHz\n", __func__, factor, bitrate);
		value = (factor - 4);

		//Output always high mode, data input debounce, PECI function enable
		// Source clock = 4MHz
		regs->PECICFG = 0x0823 | (value << 8);
	}
	return 0;
}

static int peci_kb1200_disable(const struct device *dev)
{
	const struct peci_kb1200_config * const cfg = dev->config;
	PECI_T * const regs = cfg->regs;
	int ret = 0;

	/* Make sure no transaction is interrupted before disabling the HW */
	ret = check_bus_idle(regs);
	regs->PECICFG &= ~0x0001;

#ifdef CONFIG_PECI_INTERRUPT_DRIVEN
	regs->PECIPF = 0xFF;
	irq_disable(cfg->irq_num);
#endif

	return ret;
}

static int peci_kb1200_enable(const struct device *dev)
{
	const struct peci_kb1200_config * const cfg = dev->config;
	PECI_T * const regs = cfg->regs;

	regs->PECICFG |= 0x0001;
#ifdef CONFIG_PECI_INTERRUPT_DRIVEN
	regs->PECIPF = 0xFF;
	irq_enable(cfg->irq_num);
#endif
	return 0;
}

static void peci_kb1200_bus_recovery(const struct device *dev, bool full_reset)
{
	//const struct peci_kb1200_config * const cfg = dev->config;
	//struct peci_kb1200_data * const data = dev->data;
	//PECI_T * const regs = cfg->regs;

	LOG_WRN("%s full_reset:%d", __func__, full_reset);
	if (full_reset) {
	} else {
	}
}

static int peci_kb1200_write(const struct device *dev, struct peci_msg *msg)
{
	const struct peci_kb1200_config * const cfg = dev->config;
	struct peci_kb1200_data * const data = dev->data;
	PECI_T * const regs = cfg->regs;
	int i;
	int ret;

	ret = check_bus_idle(regs);
	if (ret) {
		return ret;
	}

	struct peci_buf *tx_buf = &msg->tx_buffer;
	struct peci_buf *rx_buf = &msg->rx_buffer;

	regs->PECICTL = 0x00;   //  AWFCS_Function_Disable
	/* Add PECI transaction header to TX FIFO */
	regs->PECIADR = msg->addr;
	regs->PECILENW = tx_buf->len;  //tx_buf->len = cmd_code(one byte) + tx_buf(Max. 14 bytes) +AWFCS 
	regs->PECILENR = rx_buf->len;


	/* Add PECI payload to Tx FIFO only if write length is valid */
	if (tx_buf->len) {
		for (i = 0; i < tx_buf->len - 1; i++) {
			regs->PECIWD = tx_buf->buf[i];
		}
	}
	regs->PECIWD = msg->cmd_code;
	regs->PECIPF = 0x1F;  // Clear pending flags
	regs->PECICTL |= 0x01; // Issue_CMD

	k_busy_wait(PECI_IO_DELAY);

	/* Wait for transmission to complete */
#ifdef CONFIG_PECI_INTERRUPT_DRIVEN
	regs->PECIIE = 0x19;  // Enable Command complete, Client Abort, FIFO error interrupt, 
	if (k_sem_take(&data->tx_lock, K_MSEC(500))) {
		return -ETIMEDOUT;
	}
#else
	/* In worst case, overall timeout will be 1msec (100 * 10usec) */
	uint8_t wait_timeout_cnt = 100;

	while (!(regs->PECIPF & 0x10)) {   // Command Complete
		if (regs->PECIPF & 0x09) { // Some error.
			return -EIO;
		}
		k_busy_wait(PECI_IO_DELAY);
		wait_timeout_cnt--;
		if (!wait_timeout_cnt) {
			LOG_WRN("Tx timeout");
			data->timeout_retries++;
			/* Full reset only if multiple consecutive failures */
			if (data->timeout_retries > PECI_TIMEOUT_RETRIES) {
				peci_kb1200_bus_recovery(dev, true);
			} else {
				peci_kb1200_bus_recovery(dev, false);
			}
			return -ETIMEDOUT;
		}
	}
#endif
	data->timeout_retries = 0;
	if (regs->PECIPF & 0x09) { // Some error.
		return -EIO;
	}

	return 0;
}

static int peci_kb1200_read(const struct device *dev, struct peci_msg *msg)
{
	const struct peci_kb1200_config * const cfg = dev->config;
	PECI_T * const regs = cfg->regs;
	//uint8_t tx_fcs;
	struct peci_buf *rx_buf = &msg->rx_buffer;

	for(int i = 0; i < rx_buf->len; i++)  {
		rx_buf->buf[i] = regs->PECIRD;
	}

	/* Once write-read transaction is complete, ensure bus is idle
	 * before resetting the internal FIFOs
	 */
	int ret = check_bus_idle(regs);
	if (ret) {
		return ret;
	}

	return 0;
}

static int peci_kb1200_transfer(const struct device *dev, struct peci_msg *msg)
{
	int ret;

	ret = peci_kb1200_write(dev, msg);
	if (ret) {
		return ret;
	}

	/* If a PECI transmission is successful, it may or not involve
	 * a read operation, check if transaction expects a response
	 */
	if (msg->rx_buffer.len) {
		ret = peci_kb1200_read(dev, msg);
		if (ret) {
			return ret;
		}
	}

	/* Reset Tx/Rx FIFO for successsful peci transactions */
	peci_kb1200_bus_recovery(dev, false);

	return 0;
}

static void peci_kb1200_isr(const void *arg)
{
#ifdef CONFIG_PECI_INTERRUPT_DRIVEN
	//const struct device *dev = arg;
	//struct peci_kb1200_config * const cfg = dev->config;
	//struct peci_kb1200_data * const data = dev->data;
	//PECI_T * const regs = cfg->regs;
#endif
}

static const struct peci_driver_api peci_kb1200_driver_api = {
	.config = peci_kb1200_configure,
	.enable = peci_kb1200_enable,
	.disable = peci_kb1200_disable,
	.transfer = peci_kb1200_transfer,
};

static int peci_kb1200_init(const struct device *dev)
{
	PINMUX_DEV_T peci_pinmux = gpio_pinmux(PECI_PINMUX);
	gpio_pinmux_set(peci_pinmux.port, peci_pinmux.pin, PINMUX_FUNC_B);
	gpio_pinmux_pullup(peci_pinmux.port, peci_pinmux.pin, 1);

#ifdef CONFIG_PECI_INTERRUPT_DRIVEN
	const struct peci_kb1200_config * const cfg = dev->config;
	struct peci_kb1200_data * const data = dev->data;
	k_sem_init(&data->tx_lock, 0, 1);
	cfg->irq_init(dev);
#endif
	return 0;
}

#define PECI_KB1200_DEVICE(n)						\
	static int peci_irq_init_##n(const struct device *dev)		\
	{	IRQ_CONNECT(DT_INST_IRQN(n),			\
			    DT_INST_IRQ(n, priority),		\
			    peci_kb1200_isr,						\
			    DEVICE_DT_INST_GET(n), 0);					\
		return 0;						     \
	};												\
	static struct peci_kb1200_data peci_kb1200_data_##n;			\
	static const struct peci_kb1200_config peci_kb1200_config_##n = {	\
		.regs = (PECI_T * const)DT_INST_REG_ADDR(n),				\
		.irq_num = DT_INST_IRQN(n),				\
		.irq_init = peci_irq_init_##n,				\
	};								\
	DEVICE_DT_INST_DEFINE(n, &peci_kb1200_init, NULL,			\
		&peci_kb1200_data_##n, &peci_kb1200_config_##n,			\
		POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,			\
		&peci_kb1200_driver_api);					\

DT_INST_FOREACH_STATUS_OKAY(PECI_KB1200_DEVICE)

