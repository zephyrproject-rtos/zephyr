/*
 * Copyright (c) 2016 Linaro Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Diver for ARM PrimeCell Synchronous Serial Port (PL022)
 *
 * Based on information from the reference manual:
 * DDI0194G_ssp_pl022_r1p3_trm.pdf
 */

#include <board.h>
#include <errno.h>
#include <gpio.h>
#include <spi.h>


/* Values for Control Register 0, SSPCR0 */

#define SSPCR0_DSS(dss)	((dss) << 0)	/* Data Size Select (num bits - 1) */
#define SSPCR0_FRF_SPI	(0 << 4)	/* Motorola SPI frame format */
#define SSPCR0_FRF_TI	(1 << 4)	/* TI synchronous serial frame format */
#define SSPCR0_FRF_MW	(2 << 4)	/* National Microwire frame format */
#define SSPCR0_SPO	(1 << 6)	/* SPI clock polarity */
#define SSPCR0_SPH	(1 << 7)	/* SPI clock phase */
#define SSPCR0_SCR(scr)	((scr) << 8)	/* Serial clock rate */

/* Values for Control Register 1, SSPCR1 */

#define SSPCR1_LBM	(1 << 0)	/* Loop back mode */
#define SSPCR1_SSE	(1 << 1)	/* Synchronous serial port enable */
#define SSPCR1_MS	(1 << 2)	/* Master or slave mode select */
#define SSPCR1_SOD	(1 << 3)	/* Slave-mode output disable */

/* Values for Status Register, SSPSR */

#define SSPSR_TFE	(1 << 0)	/* Transmit FIFO empty */
#define SSPSR_TNF	(1 << 1)	/* Transmit FIFO not full */
#define SSPSR_RNE	(1 << 2)	/* Receive FIFO not empty */
#define SSPSR_RFF	(1 << 3)	/* Receive FIFO full */
#define SSPSR_BSY	(1 << 4)	/* PrimeCell SSP busy flag */

/* Values for interrupt mask/status registers SSPIMSC, SSPRIS and SSPMIS */

#define SSP_INT_ROR	(1 << 0)	/* Receive overrun interrupt flag */
#define SSP_INT_RT	(1 << 1)	/* Receive timeout interrupt flag */
#define SSP_INT_RX	(1 << 2)	/* Receive FIFO interrupt flag */
#define SSP_INT_TX	(1 << 3)	/* Transmit FIFO interrupt flag */

/* Values for Interrupt Clear Register, SSPICR */

#define SSPICR_RORIC	SSP_INT_ROR	/* Clear Receive overrun interrupt */
#define SSPICR_RTIC	SSP_INT_RT	/* Clear Receive timeout interrupt */

/* Values for DMA Control Register, SSPDMACR */

#define SSPDMACR_RXDMAE	(1 << 0)	/* Receive DMA Enable */
#define SSPDMACR_TXDMAE	(1 << 1)	/* Transmit DMA Enable */

#define FIFO_DEPTH	8		/* Number of entries in data FIFO */


/*
 * Hardware register layout,
 */
struct spi_pl022 {
	volatile u32_t cr0;	/* Control register 0, macros SSPCR0_ */
	volatile u32_t cr1;	/* Control register 1, macros SSPCR1_ */
	volatile u32_t dr;	/* Data (Transmit/Receive FIFO) */
	volatile u32_t sr;	/* Status register, macros SSPCR_  */
	volatile u32_t cpsr;	/* Clock prescale register */
	volatile u32_t imsc;	/* Interrupt mask set/clr, macros SSP_INT_ */
	volatile u32_t ris;	/* Raw interrupt status, macros SSP_INT_ */
	volatile u32_t mis;	/* Masked interrupt status, macros SSP_INT_ */
	volatile u32_t icr;	/* Interrupt clear register, macros SSPICR_ */
	volatile u32_t dmacr;	/* DMA control register, macros SSPDMACR_ */
};

struct spi_pl022_context;

typedef	void (*spi_pl022_pio_fn_t)(struct spi_pl022_context *state,
				   struct spi_pl022 *ssp);

/* Device config */
struct spi_pl022_config {
	struct spi_pl022 *ssp;
	unsigned int clk_freq;
	void (*irq_config_func)();
	const char *cs_gpio_name;
	u32_t cs_gpio_pin;

};

/* Driver state */
struct spi_pl022_context {
	struct spi_pl022 *ssp;

	struct device *cs_gpio;
	u32_t cs_gpio_pin;

	const u8_t *tx_buf;
	u32_t tx_remain;
	u8_t *rx_buf;
	u32_t rx_remain;
	spi_pl022_pio_fn_t transfer_fn;
	struct k_sem transfer_sem;

	unsigned int fifo_drain_time;
	u8_t data_bits;
	u8_t polled_mode;
};

/*
 * Configure a host controller for operating against slaves.
 * This is an implementation of a function for spi_driver_api::configure
 */
static int spi_pl022_configure(struct device *dev,
			       struct spi_config *spi_config)
{
	struct spi_pl022_context *context = dev->driver_data;
	const struct spi_pl022_config *config = dev->config->config_info;
	struct spi_pl022 *ssp = config->ssp;

	u32_t cr0, cr1;
	unsigned int div, CPSDVR, SCR1;

	context->data_bits = SPI_WORD_SIZE_GET(spi_config->config);

	context->fifo_drain_time = context->data_bits * FIFO_DEPTH
				   * USEC_PER_SEC / spi_config->max_sys_freq;

	/*
	 * The SPI clock is config->clk_freq / CPSDVR / (1+SCR)
	 * where CPSDVR must be even and in range 2..254 and SCR is 0..255.
	 *
	 * In the following calculations 'div' is the total required clock
	 * division (rounded up) i.e. the value for CPSDVR * (1+SCR).
	 * The variable SCR1 holds the value of (1+SCR).
	 */
	div = (config->clk_freq - 1) / spi_config->max_sys_freq + 1;
	CPSDVR = ((div >> 8) + 2) & ~1;
	SCR1 = (div + CPSDVR - 1) / CPSDVR;
	if (CPSDVR > 254 || SCR1 > 256) {
		return -EINVAL;
	}

	ssp->cpsr = CPSDVR;

	cr0 = SSPCR0_FRF_SPI;			/* Use SPI frame format */
	cr0 |= SSPCR0_SCR(SCR1 - 1);		/* Serial clock rate */

	if (spi_config->config & SPI_MODE_CPOL) {
		cr0 |= SSPCR0_SPO;		/* Clock polarity */
	}

	if (spi_config->config & SPI_MODE_CPHA) {
		cr0 |= SSPCR0_SPH;		/* Clock phase */
	}

	cr0 |= SSPCR0_DSS(context->data_bits - 1); /* Data bits */

	cr1 = 0;

	if (spi_config->config & SPI_MODE_LOOP) {
		cr1 |= SSPCR1_LBM;		/* Loopback */
	}

	cr1 |= SSPCR1_SSE;			/* Enable port */

	ssp->cr0 = cr0;
	ssp->cr1 = cr1;

	return 0;
}

/*
 * Define a programmed I/O function optimised for a given data size (8/16-bit)
 * and whether we want to transmit data, receive data, or do both.
 */
#define DEFINE_PIO_FUNCTION(_name, _size, _tx, _rx)			\
									\
static void _name##_size(struct spi_pl022_context *context,		\
			 struct spi_pl022 *ssp)				\
{									\
	uint##_size##_t * ptr; /* Wrong spacing to pass checkpatch */	\
	unsigned int remain;						\
	unsigned int limit;						\
									\
	ptr = (uint##_size##_t *)context->rx_buf;			\
	remain = context->rx_remain;					\
									\
	while ((ssp->sr & SSPSR_RNE) && remain) {			\
		if (_rx) {						\
			*ptr++ = ssp->dr;				\
		} else {						\
			ssp->dr; /* Discard unwanted received data */	\
		}							\
									\
		--remain;						\
	}								\
									\
	context->rx_buf = (u8_t *)ptr;					\
	context->rx_remain = remain;					\
									\
	/* limit transmission so it can't overflow receive FIFO */	\
	limit = remain > FIFO_DEPTH ? remain - FIFO_DEPTH : 0;		\
									\
	ptr = (uint##_size##_t *)context->tx_buf;			\
	remain = context->tx_remain;					\
									\
	while ((ssp->sr & SSPSR_TNF) && remain > limit) {		\
		ssp->dr = _tx ? *ptr++ : 0;				\
		--remain;						\
	}								\
									\
	context->tx_buf = (u8_t *)ptr;					\
	context->tx_remain = remain;					\
}

DEFINE_PIO_FUNCTION(_transceive, 8, true, true)
DEFINE_PIO_FUNCTION(_transceive, 16, true, true)
DEFINE_PIO_FUNCTION(_transmit, 8, true, false)
DEFINE_PIO_FUNCTION(_transmit, 16, true, false)
DEFINE_PIO_FUNCTION(_receive, 8, false, true)
DEFINE_PIO_FUNCTION(_receive, 16, false, true)

/*
 * Interrupt service routine called for all interrupt types the PL022 can raise.
 */
void spi_pl022_isr(void *arg)
{
	struct device *dev = arg;
	struct spi_pl022_context *context = dev->driver_data;
	struct spi_pl022 *ssp = context->ssp;

	/* Transfer as much data as FIFO permits */
	context->transfer_fn(context, context->ssp);

	if (!context->rx_remain) {
		/* Received everything, so we're done... */
		ssp->imsc = 0; /* Disable all our interrupts */
		k_sem_give(&context->transfer_sem);
	} else if (!context->tx_remain) {
		/*
		 * Nothing more to send, so disable tx interrupt
		 * and enable receive timeout interrupt so we get notified
		 * after last item of data has been received.
		 */
		ssp->imsc = SSP_INT_RT;
	}
}

/*
 * Try and sleep task/fibre for a period of time (in microseconds)
 */
static void idle_for(unsigned int usec)
{
	if (k_is_in_isr()) {
		return;
	}
	/*
	 * Unfortunately, the kernel only provides coarse grained tick sleeping
	 * so in practice, with the typical clock frequencies of SPI, we are
	 * unlikely to ever sleep. However, sleeping for zero time does
	 * perform k_yield allowing other threads of the same priority some
	 * CPU time.
	 */
	k_sleep(usec / 1000);
}

/*
 * Transfer required amount of data using a polling method.
 */
static int do_transfer_polled(struct device *dev)
{
	struct spi_pl022_context *context = dev->driver_data;
	struct spi_pl022 *ssp = context->ssp;

	do {
		/* Transfer as much as we can */
		context->transfer_fn(context, ssp);

		if (!(ssp->sr & SSPSR_TNF)) {
			/* Transmit FIFO full, idle for half of it to drain */
			idle_for(context->fifo_drain_time / 2);
		}

	} while (context->rx_remain || context->tx_remain);

	return 0;
}

/*
 * Transfer required amount of data using an interrupt driven method.
 */
static int do_transfer_with_irqs(struct device *dev)
{
	struct spi_pl022_context *context = dev->driver_data;
	struct spi_pl022 *ssp = context->ssp;

	 /* Enable transmit interrupt */
	ssp->imsc = SSP_INT_TX;

	 /* Wait for transfer to finish */
	k_sem_take(&context->transfer_sem, K_FOREVER);

	return 0;
}

/*
 * Transfer requested amount of data using the appropriate method, that is,
 * depending on data size and whether interrupts are supported.
 */
static int do_transfer(struct device *dev, u32_t len,
		       spi_pl022_pio_fn_t xfer8, spi_pl022_pio_fn_t xfer16)
{
	struct spi_pl022_context *context = dev->driver_data;

	if (context->data_bits <= 8) {
		context->transfer_fn = xfer8;
	} else {
		context->transfer_fn = xfer16;
		len /= 2;
	}

	context->tx_remain = len;
	context->rx_remain = len;

	if (context->polled_mode || k_is_in_isr()) {
		do_transfer_polled(dev);
	} else {
		do_transfer_with_irqs(dev);
	}

	return 0;
}

/* Simultaneously transmit and receive the requested length of data */
static int transceive(struct device *dev, u32_t len)
{
	return do_transfer(dev, len, _transceive8, _transceive16);
}

/* Transmit the requested length of data */
static int transmit(struct device *dev, u32_t len)
{
	return do_transfer(dev, len, _transmit8, _transmit16);
}

/* Receive the requested length of data */
static int receive(struct device *dev, u32_t len)
{
	return do_transfer(dev, len, _receive8, _receive16);
}

/* Set the sate of the slave's chip select line */
static void spi_pl022_set_cs(struct spi_pl022_context *context, bool active)
{
	if (context->cs_gpio) {
		/*
		 * CS lines are normally active low. If not, we expect the
		 * platform to invert the polarity of the GPIO to pin.
		 */
		gpio_pin_write(context->cs_gpio, context->cs_gpio_pin,
			       active ? 0 : 1);
		return;
	}
}

/*
 * Read and/or write a defined amount of data through SPI driver.
 * This is an implementation of a function for spi_driver_api::transceive
 */
static int spi_pl022_transceive(struct device *dev,
				const void *tx_buf, u32_t tx_buf_len,
				void *rx_buf, u32_t rx_buf_len)
{
	struct spi_pl022_context *context = dev->driver_data;

	context->tx_buf = tx_buf;
	context->rx_buf = rx_buf;

	spi_pl022_set_cs(context, true);

	if (tx_buf_len > rx_buf_len) {
		transceive(dev, rx_buf_len);
		transmit(dev, tx_buf_len - rx_buf_len);
	} else {
		transceive(dev, tx_buf_len);
		receive(dev, rx_buf_len - tx_buf_len);
	}

	spi_pl022_set_cs(context, false);

	return 0;
}


struct spi_driver_api spi_pl022_driver_api = {
	.configure = spi_pl022_configure,
	.transceive = spi_pl022_transceive,
};

int spi_pl022_init(struct device *dev)
{
	struct spi_pl022_context *context = dev->driver_data;
	const struct spi_pl022_config *config = dev->config->config_info;

	context->ssp = config->ssp;

	/* Initialise GPIO pin used for chip select (CS) if there is one */
	if (config->cs_gpio_name) {
		context->cs_gpio = device_get_binding(config->cs_gpio_name);
		if (!context->cs_gpio) {
			return -ENODEV;
		}

		context->cs_gpio_pin = config->cs_gpio_pin;
		gpio_pin_configure(context->cs_gpio, context->cs_gpio_pin,
				   GPIO_DIR_OUT);

		spi_pl022_set_cs(context, false);
	}

	if (config->irq_config_func) {
		config->irq_config_func();
		k_sem_init(&context->transfer_sem, 0, 1);
		context->polled_mode = false;
	} else {
		context->polled_mode = true;
	}

	/*
	 * Set driver_api at very end of init so device can't be found by
	 * device_get_binding until after driver is initialised.
	 */
	dev->driver_api = &spi_pl022_driver_api;
	return 0;
}


/*
 * Define a driver for device, this is a helper for subsequent macros
 */
#define	DEFINE_SPI_PL022(_num, _irq_cfg)				\
									\
static struct spi_pl022_context spi_pl022_dev_data_##_num;		\
									\
static const struct spi_pl022_config spi_pl022_dev_cfg_##_num = {	\
	.ssp = (struct spi_pl022 *)SPI_PL022_##_num##_BASE_ADDR,	\
	.clk_freq = SPI_PL022_##_num##_CLK_FREQ,			\
	.irq_config_func = _irq_cfg,					\
	.cs_gpio_name = SPI_PL022_##_num##_CS_GPIO_PORT,		\
	.cs_gpio_pin = SPI_PL022_##_num##_CS_GPIO_PIN,			\
};									\
									\
DEVICE_INIT(spi_pl022_##_num, CONFIG_SPI_PL022_##_num##_NAME,		\
	    spi_pl022_init,						\
	    &spi_pl022_dev_data_##_num,					\
	    &spi_pl022_dev_cfg_##_num,					\
	    PRE_KERNEL_2, CONFIG_SPI_INIT_PRIORITY)

/*
 * Define a driver for device with no interrupts
 */
#define	DEFINE_SPI_PL022_NO_IRQ(_num)					\
									\
DEFINE_SPI_PL022(_num, 0)

/*
 * Define a driver for device with a single combined interrupt
 */
#define	DEFINE_SPI_PL022_COMBINED_IRQ(_num)				\
									\
static void spi_pl022_irq_config_func_##_num(void);			\
									\
DEFINE_SPI_PL022(_num, spi_pl022_irq_config_func_##_num);		\
									\
static void spi_pl022_irq_config_func_##_num(void)			\
{									\
	IRQ_CONNECT(SPI_PL022_##_num##_IRQ,				\
		CONFIG_SPI_PL022_##_num##_IRQ_PRI,			\
		spi_pl022_isr,						\
		DEVICE_GET(spi_pl022_##_num),				\
		0);							\
	irq_enable(SPI_PL022_##_num##_IRQ);				\
}


#ifdef CONFIG_SPI_PL022_0
#ifndef SPI_PL022_0_CS_GPIO_PORT
#define SPI_PL022_0_CS_GPIO_PORT 0
#define SPI_PL022_0_CS_GPIO_PIN 0
#endif
#ifdef SPI_PL022_0_IRQ
DEFINE_SPI_PL022_COMBINED_IRQ(0);
#else
DEFINE_SPI_PL022_NO_IRQ(0);
#endif
#endif

#ifdef CONFIG_SPI_PL022_1
#ifndef SPI_PL022_1_CS_GPIO_PORT
#define SPI_PL022_1_CS_GPIO_PORT 0
#define SPI_PL022_1_CS_GPIO_PIN 0
#endif
#ifdef SPI_PL022_1_IRQ
DEFINE_SPI_PL022_COMBINED_IRQ(1);
#else
DEFINE_SPI_PL022_NO_IRQ(1);
#endif
#endif

#ifdef CONFIG_SPI_PL022_2
#ifndef SPI_PL022_2_CS_GPIO_PORT
#define SPI_PL022_2_CS_GPIO_PORT 0
#define SPI_PL022_2_CS_GPIO_PIN 0
#endif
#ifdef SPI_PL022_2_IRQ
DEFINE_SPI_PL022_COMBINED_IRQ(2);
#else
DEFINE_SPI_PL022_NO_IRQ(2);
#endif
#endif

#ifdef CONFIG_SPI_PL022_3
#ifndef SPI_PL022_3_CS_GPIO_PORT
#define SPI_PL022_3_CS_GPIO_PORT 0
#define SPI_PL022_3_CS_GPIO_PIN 0
#endif
#ifdef SPI_PL022_3_IRQ
DEFINE_SPI_PL022_COMBINED_IRQ(3);
#else
DEFINE_SPI_PL022_NO_IRQ(3);
#endif
#endif

#ifdef CONFIG_SPI_PL022_4
#ifndef SPI_PL022_4_CS_GPIO_PORT
#define SPI_PL022_4_CS_GPIO_PORT 0
#define SPI_PL022_4_CS_GPIO_PIN 0
#endif
#ifdef SPI_PL022_4_IRQ
DEFINE_SPI_PL022_COMBINED_IRQ(4);
#else
DEFINE_SPI_PL022_NO_IRQ(4);
#endif
#endif
