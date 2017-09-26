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
#include <logging/sys_log.h>
#include <stdio.h>
#include <string.h>

#include <clock_control/stm32_clock_control.h>

#define DMA_STM32_MAX_STREAMS	8	/* Number of streams per controller */
#define DMA_STM32_MAX_DEVS	2	/* Number of controllers */
#define DMA_STM32_1		0	/* First  DMA controller */
#define DMA_STM32_2		1	/* Second DMA controller */

#define DMA_STM32_IRQ_PRI	CONFIG_DMA_0_IRQ_PRI

#define DMA_STM32_1_RX_CHANNEL_ID	CONFIG_DMA_1_RX_SUB_CHANNEL_ID
#define DMA_STM32_1_TX_CHANNEL_ID	CONFIG_DMA_1_TX_SUB_CHANNEL_ID
#define DMA_STM32_2_RX_CHANNEL_ID	CONFIG_DMA_2_RX_SUB_CHANNEL_ID
#define DMA_STM32_2_TX_CHANNEL_ID	CONFIG_DMA_2_TX_SUB_CHANNEL_ID

struct dma_stm32_stream_reg {
	/* Shared registers */
	u32_t lisr;
	u32_t hisr;
	u32_t lifcr;
	u32_t hifcr;

	/* Per stream registers */
	u32_t scr;
	u32_t sndtr;
	u32_t spar;
	u32_t sm0ar;
	u32_t sm1ar;
	u32_t sfcr;
};

struct dma_stm32_stream {
	u32_t direction;
	struct device *dev;
	struct dma_stm32_stream_reg regs;
	bool busy;

	void (*dma_callback)(struct device *dev, u32_t id,
			     int error_code);
};

static struct dma_stm32_device {
	u32_t base;
	struct device *clk;
	struct dma_stm32_stream stream[DMA_STM32_MAX_STREAMS];
	bool mem2mem;
	u8_t channel_rx;
	u8_t channel_tx;
} device_data[DMA_STM32_MAX_DEVS];

struct dma_stm32_config {
	struct stm32_pclken pclken;
	void (*config)(struct dma_stm32_device *);
};

/* DMA burst length */
#define BURST_TRANS_LENGTH_1			0

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

#define SYS_LOG_U32			__attribute((__unused__)) u32_t

static void dma_stm32_1_config(struct dma_stm32_device *ddata);
static void dma_stm32_2_config(struct dma_stm32_device *ddata);

static u32_t dma_stm32_read(struct dma_stm32_device *ddata, u32_t reg)
{
	return sys_read32(ddata->base + reg);
}

static void dma_stm32_write(struct dma_stm32_device *ddata,
			    u32_t reg, u32_t val)
{
	sys_write32(val, ddata->base + reg);
}

static void dma_stm32_dump_reg(struct dma_stm32_device *ddata, u32_t id)
{
	SYS_LOG_INF("Using stream: %d\n", id);
	SYS_LOG_INF("SCR:   0x%x \t(config)\n",
		    dma_stm32_read(ddata, DMA_STM32_SCR(id)));
	SYS_LOG_INF("SNDTR:  0x%x \t(length)\n",
		    dma_stm32_read(ddata, DMA_STM32_SNDTR(id)));
	SYS_LOG_INF("SPAR:  0x%x \t(source)\n",
		    dma_stm32_read(ddata, DMA_STM32_SPAR(id)));
	SYS_LOG_INF("SM0AR: 0x%x \t(destination)\n",
		    dma_stm32_read(ddata, DMA_STM32_SM0AR(id)));
	SYS_LOG_INF("SM1AR: 0x%x \t(destination (double buffer mode))\n",
		    dma_stm32_read(ddata, DMA_STM32_SM1AR(id)));
	SYS_LOG_INF("SFCR:  0x%x \t(fifo control)\n",
		    dma_stm32_read(ddata, DMA_STM32_SFCR(id)));
}

static u32_t dma_stm32_irq_status(struct dma_stm32_device *ddata,
				     u32_t id)
{
	u32_t irqs;

	if (id & 4) {
		irqs = dma_stm32_read(ddata, DMA_STM32_HISR);
	} else {
		irqs = dma_stm32_read(ddata, DMA_STM32_LISR);
	}

	return (irqs >> (((id & 2) << 3) | ((id & 1) * 6)));
}

static void dma_stm32_irq_clear(struct dma_stm32_device *ddata,
				u32_t id, u32_t irqs)
{
	irqs = irqs << (((id & 2) << 3) | ((id & 1) * 6));

	if (id & 4) {
		dma_stm32_write(ddata, DMA_STM32_HIFCR, irqs);
	} else {
		dma_stm32_write(ddata, DMA_STM32_LIFCR, irqs);
	}
}

static void dma_stm32_irq_handler(void *arg, u32_t id)
{
	struct device *dev = arg;
	struct dma_stm32_device *ddata = dev->driver_data;
	struct dma_stm32_stream *stream = &ddata->stream[id];
	u32_t irqstatus, config, sfcr;

	irqstatus = dma_stm32_irq_status(ddata, id);
	config = dma_stm32_read(ddata, DMA_STM32_SCR(id));
	sfcr = dma_stm32_read(ddata, DMA_STM32_SFCR(id));

	/* Silently ignore spurious transfer half complete IRQ */
	if (irqstatus & DMA_STM32_HTI) {
		dma_stm32_irq_clear(ddata, id, DMA_STM32_HTI);
		return;
	}

	stream->busy = false;

	if ((irqstatus & DMA_STM32_TCI) && (config & DMA_STM32_SCR_TCIE)) {
		dma_stm32_irq_clear(ddata, id, DMA_STM32_TCI);

		stream->dma_callback(stream->dev, id, 0);
	} else {
		SYS_LOG_ERR("Internal error: IRQ status: 0x%x\n", irqstatus);
		dma_stm32_irq_clear(ddata, id, irqstatus);

		stream->dma_callback(stream->dev, id, -EIO);
	}
}

static int dma_stm32_disable_stream(struct dma_stm32_device *ddata,
				  u32_t id)
{
	u32_t config;
	int count = 0;
	int ret = 0;

	for (;;) {
		config = dma_stm32_read(ddata, DMA_STM32_SCR(id));
		/* Stream already disabled */
		if (!(config & DMA_STM32_SCR_EN)) {
			return 0;
		}

		/* Try to disable stream */
		dma_stm32_write(ddata, DMA_STM32_SCR(id),
				config &= ~DMA_STM32_SCR_EN);

		/* After trying for 5 seconds, give up */
		k_sleep(K_SECONDS(5));
		if (count++ > (5 * 1000) / 50) {
			SYS_LOG_ERR("DMA error: Stream in use\n");
			return -EBUSY;
		}
	}

	return ret;
}

static int dma_stm32_config_devcpy(struct device *dev, u32_t id,
				   struct dma_config *config)

{
	struct dma_stm32_device *ddata = dev->driver_data;
	struct dma_stm32_stream_reg *regs = &ddata->stream[id].regs;
	u32_t src_bus_width  = dma_width_index(config->source_data_size);
	u32_t dst_bus_width  = dma_width_index(config->dest_data_size);
	u32_t src_burst_size = dma_burst_index(config->source_burst_length);
	u32_t dst_burst_size = dma_burst_index(config->dest_burst_length);
	enum dma_channel_direction direction = config->channel_direction;

	switch (direction) {
	case MEMORY_TO_PERIPHERAL:
		regs->scr = DMA_STM32_SCR_DIR(DMA_STM32_MEM_TO_DEV) |
			DMA_STM32_SCR_PSIZE(dst_bus_width) |
			DMA_STM32_SCR_MSIZE(src_bus_width) |
			DMA_STM32_SCR_PBURST(dst_burst_size) |
			DMA_STM32_SCR_MBURST(src_burst_size) |
			DMA_STM32_SCR_REQ(ddata->channel_tx) |
			DMA_STM32_SCR_MINC;
		break;
	case PERIPHERAL_TO_MEMORY:
		regs->scr = DMA_STM32_SCR_DIR(DMA_STM32_DEV_TO_MEM) |
			DMA_STM32_SCR_PSIZE(src_bus_width) |
			DMA_STM32_SCR_MSIZE(dst_bus_width) |
			DMA_STM32_SCR_PBURST(src_burst_size) |
			DMA_STM32_SCR_MBURST(dst_burst_size) |
			DMA_STM32_SCR_REQ(ddata->channel_rx);
		break;
	default:
		SYS_LOG_ERR("DMA error: Direction not supported: %d",
			    direction);
		return -EINVAL;
	}

	if (src_burst_size == BURST_TRANS_LENGTH_1 &&
	    dst_burst_size == BURST_TRANS_LENGTH_1) {
		/* Enable 'direct' mode error IRQ, disable 'FIFO' error IRQ */
		regs->scr |= DMA_STM32_SCR_DMEIE;
		regs->sfcr &= ~DMA_STM32_SFCR_MASK;
	} else {
		/* Enable 'FIFO' error IRQ, disable 'direct' mode error IRQ */
		regs->sfcr |= DMA_STM32_SFCR_MASK;
		regs->scr &= ~DMA_STM32_SCR_DMEIE;
	}

	return 0;
}

static int dma_stm32_config_memcpy(struct device *dev, u32_t id)
{
	struct dma_stm32_device *ddata = dev->driver_data;
	struct dma_stm32_stream_reg *regs = &ddata->stream[id].regs;

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

static int dma_stm32_config(struct device *dev, u32_t id,
			    struct dma_config *config)
{
	struct dma_stm32_device *ddata = dev->driver_data;
	struct dma_stm32_stream *stream = &ddata->stream[id];
	struct dma_stm32_stream_reg *regs = &ddata->stream[id].regs;
	int ret;

	if (stream->busy) {
		return -EBUSY;
	}

	if (config->head_block->block_size > DMA_STM32_MAX_DATA_ITEMS) {
		SYS_LOG_ERR("DMA error: Data size too big: %d\n",
		       config->head_block->block_size);
		return -EINVAL;
	}

	stream->busy		= true;
	stream->dma_callback	= config->dma_callback;
	stream->direction	= config->channel_direction;

	if (stream->direction == MEMORY_TO_PERIPHERAL) {
		regs->sm0ar = (u32_t)config->head_block->source_address;
		regs->spar = (u32_t)config->head_block->dest_address;
	} else {
		regs->spar = (u32_t)config->head_block->source_address;
		regs->sm0ar = (u32_t)config->head_block->dest_address;
	}

	if (stream->direction == MEMORY_TO_MEMORY) {
		ret = dma_stm32_config_memcpy(dev, id);
	} else {
		ret = dma_stm32_config_devcpy(dev, id, config);
	}

	regs->sndtr = config->head_block->block_size;

	return ret;
}

static int dma_stm32_start(struct device *dev, u32_t id)
{
	struct dma_stm32_device *ddata = dev->driver_data;
	struct dma_stm32_stream_reg *regs = &ddata->stream[id].regs;
	u32_t irqstatus;
	int ret;

	ret = dma_stm32_disable_stream(ddata, id);
	if (ret) {
		return ret;
	}

	dma_stm32_write(ddata, DMA_STM32_SCR(id),   regs->scr);
	dma_stm32_write(ddata, DMA_STM32_SPAR(id),  regs->spar);
	dma_stm32_write(ddata, DMA_STM32_SM0AR(id), regs->sm0ar);
	dma_stm32_write(ddata, DMA_STM32_SFCR(id),  regs->sfcr);
	dma_stm32_write(ddata, DMA_STM32_SM1AR(id), regs->sm1ar);
	dma_stm32_write(ddata, DMA_STM32_SNDTR(id), regs->sndtr);

	/* Clear remanent IRQs from previous transfers */
	irqstatus = dma_stm32_irq_status(ddata, id);
	if (irqstatus) {
		dma_stm32_irq_clear(ddata, id, irqstatus);
	}

	dma_stm32_dump_reg(ddata, id);

	/* Push the start button */
	dma_stm32_write(ddata, DMA_STM32_SCR(id),
			regs->scr | DMA_STM32_SCR_EN);

	return 0;
}

static int dma_stm32_stop(struct device *dev, u32_t id)
{
	struct dma_stm32_device *ddata = dev->driver_data;
	struct dma_stm32_stream *stream = &ddata->stream[id];
	u32_t scr, sfcr, irqstatus;
	int ret;

	/* Disable all IRQs */
	scr = dma_stm32_read(ddata, DMA_STM32_SCR(id));
	scr &= ~DMA_STM32_SCR_IRQ_MASK;
	dma_stm32_write(ddata, DMA_STM32_SCR(id), scr);

	sfcr = dma_stm32_read(ddata, DMA_STM32_SFCR(id));
	sfcr &= ~DMA_STM32_SFCR_FEIE;
	dma_stm32_write(ddata, DMA_STM32_SFCR(id), sfcr);

	/* Disable stream */
	ret = dma_stm32_disable_stream(ddata, id);
	if (ret)
		return ret;

	/* Clear remanent IRQs from previous transfers */
	irqstatus = dma_stm32_irq_status(ddata, id);
	if (irqstatus) {
		dma_stm32_irq_clear(ddata, id, irqstatus);
	}

	/* Finally, flag stream as free */
	stream->busy = false;

	return 0;
}

static int dma_stm32_init(struct device *dev)
{
	struct dma_stm32_device *ddata = dev->driver_data;
	const struct dma_stm32_config *cdata = dev->config->config_info;
	int i;

	for (i = 0; i < DMA_STM32_MAX_STREAMS; i++) {
		ddata->stream[i].dev  = dev;
		ddata->stream[i].busy = false;
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
	.config		 = dma_stm32_config,
	.start		 = dma_stm32_start,
	.stop		 = dma_stm32_stop,
};

const struct dma_stm32_config dma_stm32_1_cdata = {
	.pclken = { .bus = STM32_CLOCK_BUS_AHB1,
		    .enr = LL_AHB1_GRP1_PERIPH_DMA1 },
	.config = dma_stm32_1_config,
};

DEVICE_AND_API_INIT(dma_stm32_1, CONFIG_DMA_1_NAME, &dma_stm32_init,
		    &device_data[DMA_STM32_1], &dma_stm32_1_cdata,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		    (void *)&dma_funcs);

static const struct dma_stm32_config dma_stm32_2_cdata = {
	.pclken = { .bus = STM32_CLOCK_BUS_AHB1,
		    .enr = LL_AHB1_GRP1_PERIPH_DMA2 },
	.config = dma_stm32_2_config,
};

DEVICE_AND_API_INIT(dma_stm32_2, CONFIG_DMA_2_NAME, &dma_stm32_init,
		    &device_data[DMA_STM32_2], &dma_stm32_2_cdata,
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
	ddata->channel_tx = DMA_STM32_1_TX_CHANNEL_ID;
	ddata->channel_rx = DMA_STM32_1_RX_CHANNEL_ID;

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
	ddata->channel_tx = DMA_STM32_2_TX_CHANNEL_ID;
	ddata->channel_rx = DMA_STM32_2_RX_CHANNEL_ID;

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
