/*
 * Copyright (c) 2016 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#define SYS_LOG_LEVEL CONFIG_SYS_LOG_DMA_LEVEL

#include <board.h>
#include <device.h>
#include <dma.h>
#include <errno.h>
#include <init.h>
#include <misc/sys_log.h>
#include <stdio.h>
#include <string.h>

#include <clock_control/stm32_clock_control.h>

#define DMA_STM32_MAX_CHANNELS	8	/* Number of channels per controller */
#define DMA_STM32_MAX_DEVS	2	/* Number of controllers */
#define DMA_STM32_1		0	/* First  DMA controller */
#define DMA_STM32_2		1	/* Second DMA controller */

#define DMA_STM32_IRQ_PRI	63	/* DMA controller IRQ priority */

struct dma_stm32_chan_reg {
	/* Shared registers */
	uint32_t lisr;
	uint32_t hisr;
	uint32_t lifcr;
	uint32_t hifcr;

	/* Per channel registers */
	uint32_t scr;
	uint32_t sndtr;
	uint32_t spar;
	uint32_t sm0ar;
	uint32_t sm1ar;
	uint32_t sfcr;
};

struct dma_stm32_chan {
	uint32_t id;
	uint32_t direction;
	struct device *dev;
	struct dma_stm32_chan_reg regs;
	bool busy;

	void (*dma_transfer)(struct device *dev, void *data);
	void (*dma_error)(struct device *dev, void *data);
	void *callback_data;
};

static struct dma_stm32_device {
	uint32_t base;
	struct device *clk;
	struct dma_stm32_chan chan[DMA_STM32_MAX_CHANNELS];
	bool mem2mem;
} ddata[DMA_STM32_MAX_DEVS];

struct dma_stm32_config {
	struct stm32f4x_pclken pclken;
	void (*config)(struct dma_stm32_device *);
};

/* DMA direction */
#define DMA_STM32_DEV_TO_MEM			0
#define DMA_STM32_MEM_TO_DEV			1
#define DMA_STM32_MEM_TO_MEM			2

/* DMA priority level */
#define DMA_STM32_PRIORITY_LOW			0
#define DMA_STM32_PRIORITY_MEDIUM		1
#define DMA_STM32_PRIORITY_HIGH			2
#define DMA_STM32_PRIORITY_VERY_HIGH		3

/* DMA FIFO threshold selection */
#define DMA_STM32_FIFO_THRESHOLD_1QUARTERFULL	0
#define DMA_STM32_FIFO_THRESHOLD_HALFFULL	1
#define DMA_STM32_FIFO_THRESHOLD_3QUARTERSFULL	2
#define DMA_STM32_FIFO_THRESHOLD_FULL		3

/* Maximum data sent in single transfer (Bytes) */
#define DMA_STM32_MAX_DATA_ITEMS		0xffff

#define BITS_PER_LONG		32
#define GENMASK(h, l) (((~0UL) << (l)) & (~0UL >> (BITS_PER_LONG - 1 - (h))))

#define DMA_STM32_1_BASE	0x40026000
#define DMA_STM32_2_BASE	0x40026400

/* Shared registers */
#define DMA_STM32_LISR		0x00   /* DMA low int status reg	      */
#define DMA_STM32_HISR		0x04   /* DMA high int status reg	      */
#define DMA_STM32_LIFCR		0x08   /* DMA low int flag clear reg	      */
#define DMA_STM32_HIFCR		0x0c   /* DMA high int flag clear reg	      */
#define   DMA_STM32_FEI		BIT(0) /* FIFO error interrupt		      */
#define   RESERVED_1		BIT(1)
#define   DMA_STM32_DMEI	BIT(2) /* Direct mode error interrupt	      */
#define   DMA_STM32_TEI		BIT(3) /* Transfer error interrupt	      */
#define   DMA_STM32_HTI		BIT(4) /* Transfer half complete interrupt    */
#define   DMA_STM32_TCI		BIT(5) /* Transfer complete interrupt	      */

/* DMA Stream x Configuration Register */
#define DMA_STM32_SCR(x)		(0x10 + 0x18 * (x))
#define   DMA_STM32_SCR_EN		BIT(0)  /* Stream Enable	      */
#define   DMA_STM32_SCR_DMEIE		BIT(1)  /* Direct Mode Err Int En     */
#define   DMA_STM32_SCR_TEIE		BIT(2)  /* Transfer Error Int En      */
#define   DMA_STM32_SCR_HTIE		BIT(3)  /* Transfer 1/2 Comp Int En   */
#define   DMA_STM32_SCR_TCIE		BIT(4)  /* Transfer Comp Int En       */
#define   DMA_STM32_SCR_PFCTRL		BIT(5)  /* Peripheral Flow Controller */
#define   DMA_STM32_SCR_DIR_MASK	GENMASK(7, 6)   /* Transfer direction */
#define   DMA_STM32_SCR_CIRC		BIT(8)  /* Circular mode	      */
#define   DMA_STM32_SCR_PINC		BIT(9)  /* Peripheral increment mode  */
#define   DMA_STM32_SCR_MINC		BIT(10) /* Memory increment mode      */
#define   DMA_STM32_SCR_PSIZE_MASK	GENMASK(12, 11) /* Periph data size   */
#define   DMA_STM32_SCR_MSIZE_MASK	GENMASK(14, 13) /* Memory data size   */
#define   DMA_STM32_SCR_PINCOS		BIT(15) /* Periph inc offset size     */
#define   DMA_STM32_SCR_PL_MASK		GENMASK(17, 16) /* Priority level     */
#define   DMA_STM32_SCR_DBM		BIT(18) /* Double Buffer Mode	      */
#define   DMA_STM32_SCR_CT		BIT(19) /* Target in double buffer    */
#define   DMA_STM32_SCR_PBURST_MASK	GENMASK(22, 21) /* Periph burst size  */
#define   DMA_STM32_SCR_MBURST_MASK	GENMASK(24, 23) /* Memory burst size  */
/*	  Setting MACROS */
#define   DMA_STM32_SCR_DIR(n)		((n & 0x3) << 6)
#define   DMA_STM32_SCR_PSIZE(n)	((n & 0x3) << 11)
#define   DMA_STM32_SCR_MSIZE(n)	((n & 0x3) << 13)
#define   DMA_STM32_SCR_PL(n)		((n & 0x3) << 16)
#define   DMA_STM32_SCR_PBURST(n)	((n & 0x3) << 21)
#define   DMA_STM32_SCR_MBURST(n)	((n & 0x3) << 23)
#define   DMA_STM32_SCR_REQ(n)		((n & 0x7) << 25)
/*	  Getting MACROS */
#define   DMA_STM32_SCR_PSIZE_GET(n)    ((n & DMA_STM32_SCR_PSIZE_MASK) >> 11)
#define   DMA_STM32_SCR_CFG_MASK	(DMA_STM32_SCR_PINC  \
					| DMA_STM32_SCR_MINC \
					| DMA_STM32_SCR_PINCOS \
					| DMA_STM32_SCR_PL_MASK)
#define   DMA_STM32_SCR_IRQ_MASK	(DMA_STM32_SCR_TCIE \
					| DMA_STM32_SCR_TEIE \
					| DMA_STM32_SCR_DMEIE)

/* DMA stream x number of data register (len) */
#define DMA_STM32_SNDTR(x)		(0x14 + 0x18 * (x))

/* DMA stream peripheral address register (source) */
#define DMA_STM32_SPAR(x)		(0x18 + 0x18 * (x))

/* DMA stream x memory 0 address register (destination) */
#define DMA_STM32_SM0AR(x)		(0x1c + 0x18 * (x))

/* DMA stream x memory 1 address register (destination - double buffer) */
#define DMA_STM32_SM1AR(x)		(0x20 + 0x18 * (x))

/* DMA stream x FIFO control register */
#define DMA_STM32_SFCR(x)		(0x24 + 0x18 * (x))
#define   DMA_STM32_SFCR_FTH_MASK	GENMASK(1, 0) /* FIFO threshold       */
#define   DMA_STM32_SFCR_DMDIS		BIT(2)	      /* Direct mode disable  */
#define   DMA_STM32_SFCR_STAT_MASK	GENMASK(5, 3) /* FIFO status	  */
#define   RESERVED_6			BIT(6)	      /* Reserved	     */
#define   DMA_STM32_SFCR_FEIE		BIT(7) /* FIFO error interrupt enable */
/*	  Setting MACROS */
#define   DMA_STM32_SFCR_FTH(n)		(n & DMA_STM32_SFCR_FTH_MASK)
#define   DMA_STM32_SFCR_MASK		(DMA_STM32_SFCR_FEIE \
					 | DMA_STM32_SFCR_DMDIS)

#define SYS_LOG_U32			__attribute((__unused__)) uint32_t

static void dma_stm32_1_config(struct dma_stm32_device *ddata);
static void dma_stm32_2_config(struct dma_stm32_device *ddata);

static uint32_t dma_stm32_read(struct dma_stm32_device *ddata, uint32_t reg)
{
	return sys_read32(ddata->base + reg);
}

static void dma_stm32_write(struct dma_stm32_device *ddata,
			    uint32_t reg, uint32_t val)
{
	sys_write32(val, ddata->base + reg);
}

static uint32_t dma_stm32_irq_status(struct dma_stm32_device *ddata,
				     uint32_t channel)
{
	uint32_t irqs;

	if (channel & 4) {
		irqs = dma_stm32_read(ddata, DMA_STM32_HISR);
	} else {
		irqs = dma_stm32_read(ddata, DMA_STM32_LISR);
	}

	return (irqs >> (((channel & 2) << 3) | ((channel & 1) * 6)));
}

static void dma_stm32_irq_clear(struct dma_stm32_device *ddata,
				uint32_t channel, uint32_t irqs)
{
	irqs = irqs << (((channel & 2) << 3) | ((channel & 1) * 6));

	if (channel & 4) {
		dma_stm32_write(ddata, DMA_STM32_HIFCR, irqs);
	} else {
		dma_stm32_write(ddata, DMA_STM32_LIFCR, irqs);
	}
}

static void dma_stm32_irq_handler(void *arg, uint32_t channel)
{
	struct device *dev = arg;
	struct dma_stm32_device *ddata = dev->driver_data;
	struct dma_stm32_chan *chan = &ddata->chan[channel];
	uint32_t irqstatus, config, sfcr;

	irqstatus = dma_stm32_irq_status(ddata, channel);
	config = dma_stm32_read(ddata, DMA_STM32_SCR(channel));
	sfcr = dma_stm32_read(ddata, DMA_STM32_SFCR(channel));

	/* Silently ignore spurious transfer half complete IRQ */
	if (irqstatus & DMA_STM32_HTI) {
		dma_stm32_irq_clear(ddata, channel, DMA_STM32_HTI);
		return;
	}

	if ((irqstatus & DMA_STM32_TCI) && (config & DMA_STM32_SCR_TCIE)) {
		dma_stm32_irq_clear(ddata, channel, DMA_STM32_TCI);

		chan->dma_transfer(chan->dev, chan->callback_data);
	} else {
		SYS_LOG_ERR("Internal error: IRQ status: 0x%x\n", irqstatus);
		dma_stm32_irq_clear(ddata, channel, irqstatus);

		chan->dma_error(chan->dev, chan->callback_data);
	}
	chan->busy = false;
}

static int dma_stm32_disable_chan(struct dma_stm32_device *ddata,
				  uint32_t channel)
{
	uint32_t config;
	int count = 0;
	int ret = 0;

	for (;;) {
		config = dma_stm32_read(ddata, DMA_STM32_SCR(channel));
		/* Channel already disabled */
		if (!(config & DMA_STM32_SCR_EN)) {
			return 0;
		}

		/* Try to disable channel */
		dma_stm32_write(ddata, DMA_STM32_SCR(channel),
				config &= ~DMA_STM32_SCR_EN);

		/* After trying for 5 seconds, give up */
		k_sleep(K_SECONDS(5));
		if (count++ > (5 * 1000) / 50) {
			SYS_LOG_ERR("DMA error: Channel in use\n");
			return -EBUSY;
		}
	}

	return ret;
}

static int dma_stm32_config_memcpy(struct device *dev, uint32_t channel,
				       struct dma_channel_config *config)
{
	struct dma_stm32_device *ddata = dev->driver_data;
	struct dma_stm32_chan_reg *regs = &ddata->chan[channel].regs;

	if (!ddata->mem2mem) {
		SYS_LOG_ERR("%s does not support mem-to-mem transfers\n",
		       dev->config->name);
		return -EINVAL;
	}

	/* Reset register values for next transfer */
	memset(regs, 0, sizeof(struct dma_stm32_chan_reg));

	regs->scr = DMA_STM32_SCR_DIR(DMA_STM32_MEM_TO_MEM) |
		DMA_STM32_SCR_MINC |		/* Memory increment mode */
		DMA_STM32_SCR_PINC |		/* Peripheral increment mode */
		DMA_STM32_SCR_TCIE |		/* Transfer comp IRQ enable */
		DMA_STM32_SCR_TEIE;		/* Transfer error IRQ enable */

	regs->sfcr = DMA_STM32_SFCR_DMDIS |	/* Direct mode disable */
		DMA_STM32_SFCR_FTH(DMA_STM32_FIFO_THRESHOLD_FULL) |
		DMA_STM32_SFCR_FEIE;		/* FIFI error IRQ enable */

	return 0;
}

static int dma_stm32_channel_config(struct device *dev, uint32_t channel,
				    struct dma_channel_config *config)
{
	struct dma_stm32_device *ddata = dev->driver_data;
	struct dma_stm32_chan *chan = &ddata->chan[channel];

	if (config->channel_direction != MEMORY_TO_MEMORY) {
		SYS_LOG_ERR("Only mem-to-mem transfers currently supported\n");
		return -ENOTSUP;
	}

	if (chan->busy) {
		return -EBUSY;
	}

	chan->busy	    = true;
	chan->dma_error     = config->dma_error;
	chan->dma_transfer  = config->dma_transfer;
	chan->callback_data = config->callback_data;

	return dma_stm32_config_memcpy(dev, channel, config);
}

static int dma_stm32_transfer_config(struct device *dev, uint32_t channel,
				     struct dma_transfer_config *config)
{
	struct dma_stm32_device *ddata = dev->driver_data;
	struct dma_stm32_chan_reg *regs = &ddata->chan[channel].regs;

	regs->spar  = (uint32_t)config->source_address;
	regs->sm0ar = (uint32_t)config->destination_address;
	regs->sndtr = config->block_size;

	return 0;
}

static int dma_stm32_transfer_start(struct device *dev, uint32_t channel)
{
	struct dma_stm32_device *ddata = dev->driver_data;
	struct dma_stm32_chan_reg *regs = &ddata->chan[channel].regs;
	uint32_t irqstatus;
	int ret;

	ret = dma_stm32_disable_chan(ddata, channel);
	if (ret) {
		return ret;
	}

	dma_stm32_write(ddata, DMA_STM32_SCR(channel),   regs->scr);
	dma_stm32_write(ddata, DMA_STM32_SPAR(channel),  regs->spar);
	dma_stm32_write(ddata, DMA_STM32_SM0AR(channel), regs->sm0ar);
	dma_stm32_write(ddata, DMA_STM32_SFCR(channel),  regs->sfcr);
	dma_stm32_write(ddata, DMA_STM32_SM1AR(channel), regs->sm1ar);
	dma_stm32_write(ddata, DMA_STM32_SNDTR(channel), regs->sndtr);

	/* Clear remanent IRQs from previous transfers */
	irqstatus = dma_stm32_irq_status(ddata, channel);
	if (irqstatus) {
		dma_stm32_irq_clear(ddata, channel, irqstatus);
	}

	/* Push the start button */
	dma_stm32_write(ddata, DMA_STM32_SCR(channel),
			regs->scr | DMA_STM32_SCR_EN);

	return 0;
}

static int dma_stm32_init(struct device *dev)
{
	struct dma_stm32_device *ddata = dev->driver_data;
	const struct dma_stm32_config *cdata = dev->config->config_info;
	int i;

	for (i = 0; i < DMA_STM32_MAX_CHANNELS; i++) {
		ddata->chan[i].id   = i;
		ddata->chan[i].dev  = dev;
		ddata->chan[i].busy = false;
	}

	/* Enable DMA clock */
	ddata->clk = device_get_binding(STM32_CLOCK_CONTROL_NAME);

	__ASSERT_NO_MSG(ddata->clk);

	clock_control_on(ddata->clk, (clock_control_subsys_t *) &cdata->pclken);

	/* Set controller specific configuration */
	cdata->config(ddata);

	return 0;
}

static const struct dma_driver_api dma_funcs = {
	.channel_config  = dma_stm32_channel_config,
	.transfer_config = dma_stm32_transfer_config,
	.transfer_start  = dma_stm32_transfer_start,
};

const struct dma_stm32_config dma_stm32_1_cdata = {
	.pclken = { .bus = STM32F4X_CLOCK_BUS_AHB1,
		    .enr = STM32F4X_CLOCK_ENABLE_DMA1 },
	.config = dma_stm32_1_config,
};

DEVICE_AND_API_INIT(dma_stm32_1, "DMA_1", &dma_stm32_init,
		    &ddata[DMA_STM32_1], &dma_stm32_1_cdata,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    (void *)&dma_funcs);

static const struct dma_stm32_config dma_stm32_2_cdata = {
	.pclken = { .bus = STM32F4X_CLOCK_BUS_AHB1,
		    .enr = STM32F4X_CLOCK_ENABLE_DMA2 },
	.config = dma_stm32_2_config,
};

DEVICE_AND_API_INIT(dma_stm32_2, "DMA_2", &dma_stm32_init,
		    &ddata[DMA_STM32_2], &dma_stm32_2_cdata,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    (void *)&dma_funcs);

static void dma_stm32_irq_0(void *arg) { dma_stm32_irq_handler(arg, 0); }
static void dma_stm32_irq_1(void *arg) { dma_stm32_irq_handler(arg, 1); }
static void dma_stm32_irq_2(void *arg) { dma_stm32_irq_handler(arg, 2); }
static void dma_stm32_irq_3(void *arg) { dma_stm32_irq_handler(arg, 3); }
static void dma_stm32_irq_4(void *arg) { dma_stm32_irq_handler(arg, 4); }
static void dma_stm32_irq_5(void *arg) { dma_stm32_irq_handler(arg, 5); }
static void dma_stm32_irq_6(void *arg) { dma_stm32_irq_handler(arg, 6); }
static void dma_stm32_irq_7(void *arg) { dma_stm32_irq_handler(arg, 7); }

static void dma_stm32_1_config(struct dma_stm32_device *ddata)
{
	ddata->base = DMA_STM32_1_BASE;

	IRQ_CONNECT(STM32F4_IRQ_DMA1_STREAM0, DMA_STM32_IRQ_PRI,
		    dma_stm32_irq_0, DEVICE_GET(dma_stm32_1), 0);
	irq_enable(STM32F4_IRQ_DMA1_STREAM0);

	IRQ_CONNECT(STM32F4_IRQ_DMA1_STREAM1, DMA_STM32_IRQ_PRI,
		    dma_stm32_irq_1, DEVICE_GET(dma_stm32_1), 0);
	irq_enable(STM32F4_IRQ_DMA1_STREAM1);

	IRQ_CONNECT(STM32F4_IRQ_DMA1_STREAM2, DMA_STM32_IRQ_PRI,
		    dma_stm32_irq_2, DEVICE_GET(dma_stm32_1), 0);
	irq_enable(STM32F4_IRQ_DMA1_STREAM2);

	IRQ_CONNECT(STM32F4_IRQ_DMA1_STREAM3, DMA_STM32_IRQ_PRI,
		    dma_stm32_irq_3, DEVICE_GET(dma_stm32_1), 0);
	irq_enable(STM32F4_IRQ_DMA1_STREAM3);

	IRQ_CONNECT(STM32F4_IRQ_DMA1_STREAM4, DMA_STM32_IRQ_PRI,
		    dma_stm32_irq_4, DEVICE_GET(dma_stm32_1), 0);
	irq_enable(STM32F4_IRQ_DMA1_STREAM4);

	IRQ_CONNECT(STM32F4_IRQ_DMA1_STREAM5, DMA_STM32_IRQ_PRI,
		    dma_stm32_irq_5, DEVICE_GET(dma_stm32_1), 0);
	irq_enable(STM32F4_IRQ_DMA1_STREAM5);

	IRQ_CONNECT(STM32F4_IRQ_DMA1_STREAM6, DMA_STM32_IRQ_PRI,
		    dma_stm32_irq_6, DEVICE_GET(dma_stm32_1), 0);
	irq_enable(STM32F4_IRQ_DMA1_STREAM6);

	IRQ_CONNECT(STM32F4_IRQ_DMA1_STREAM7, DMA_STM32_IRQ_PRI,
		    dma_stm32_irq_7, DEVICE_GET(dma_stm32_1), 0);
	irq_enable(STM32F4_IRQ_DMA1_STREAM7);
}

static void dma_stm32_2_config(struct dma_stm32_device *ddata)
{
	ddata->base = DMA_STM32_2_BASE;
	ddata->mem2mem = true;

	IRQ_CONNECT(STM32F4_IRQ_DMA2_STREAM0, DMA_STM32_IRQ_PRI,
		    dma_stm32_irq_0, DEVICE_GET(dma_stm32_2), 0);
	irq_enable(STM32F4_IRQ_DMA2_STREAM0);

	IRQ_CONNECT(STM32F4_IRQ_DMA2_STREAM1, DMA_STM32_IRQ_PRI,
		    dma_stm32_irq_1, DEVICE_GET(dma_stm32_2), 0);
	irq_enable(STM32F4_IRQ_DMA2_STREAM1);

	IRQ_CONNECT(STM32F4_IRQ_DMA2_STREAM2, DMA_STM32_IRQ_PRI,
		    dma_stm32_irq_2, DEVICE_GET(dma_stm32_2), 0);
	irq_enable(STM32F4_IRQ_DMA2_STREAM2);

	IRQ_CONNECT(STM32F4_IRQ_DMA2_STREAM3, DMA_STM32_IRQ_PRI,
		    dma_stm32_irq_3, DEVICE_GET(dma_stm32_2), 0);
	irq_enable(STM32F4_IRQ_DMA2_STREAM3);

	IRQ_CONNECT(STM32F4_IRQ_DMA2_STREAM4, DMA_STM32_IRQ_PRI,
		    dma_stm32_irq_4, DEVICE_GET(dma_stm32_2), 0);
	irq_enable(STM32F4_IRQ_DMA2_STREAM4);

	IRQ_CONNECT(STM32F4_IRQ_DMA2_STREAM5, DMA_STM32_IRQ_PRI,
		    dma_stm32_irq_5, DEVICE_GET(dma_stm32_2), 0);
	irq_enable(STM32F4_IRQ_DMA2_STREAM5);

	IRQ_CONNECT(STM32F4_IRQ_DMA2_STREAM6, DMA_STM32_IRQ_PRI,
		    dma_stm32_irq_6, DEVICE_GET(dma_stm32_2), 0);
	irq_enable(STM32F4_IRQ_DMA2_STREAM6);

	IRQ_CONNECT(STM32F4_IRQ_DMA2_STREAM7, DMA_STM32_IRQ_PRI,
		    dma_stm32_irq_7, DEVICE_GET(dma_stm32_2), 0);
	irq_enable(STM32F4_IRQ_DMA2_STREAM7);
}

