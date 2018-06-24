struct axi_dmac_dev {
	u32_t src_addr;
	u32_t dest_addr;
	u32_t src_stride;
	u32_t dest_stride;
	u32_t x_len;
	u32_t y_len;
	u32_t id;
	bool schedule_when_free;
};

struct axi_dmac_chan {
	u32_t src_width;
	u32_t dest_width;
	u32_t src_type;
	u32_t dest_type;
	u32_t direction;
	u32_t max_length;
	u32_t align_mask;

	bool hw_cyclic;
	bool hw_2d;
};

struct axi_dmac_dev_data {
	struct axi_dmac_dev *dev;
	struct axi_dmac_chan *chan;
};

/* Device constant configuration parameters */
struct axi_dmac_dev_cfg {
        u32_t base;
        void (*irq_config)(void);
        u32_t irq_id;
};

static ALWAYS_INLINE void axi_dmac_write(u32_t dma_base, u32_t reg, u32_t value)
{
        *((volatile u32_t*)(dma_base + reg)) = value;
}

static ALWAYS_INLINE u32_t axi_dmac_read(u32_t dma_base, u32_t reg)
{
        return *((volatile u32_t*)(dma_base + reg));
}

static int axi_dmac_config(struct device *dev, u32_t channel,
				struct dma_config *cfg)
{
        const struct axi_dmac_dev_cfg *const dev_cfg = DEV_CFG(dev);
        struct axi_dmac_dev_data *const dev_data = DEV_DATA(dev);
        struct axi_dmac_chan *chan = dev_data->chan;
        struct axi_dmac_dev *dev = dev_data->dev;

	/* Only single channel is supported */
        if (!channel) {
                return -EINVAL;
        }

	dev->src_addr = cfg_blocks->source_address;
	dev->dest_addr = cfg_blocks->dest_address;

	chan->direction = cfg->channel_direction;
	switch (chan->direction) {
		case MEMORY_TO_MEMORY:
				
	}
	

}



static int axi_dmac_initialize(struct device *dev)
{
        const struct axi_dmac_dev_cfg *const dev_cfg = DEV_CFG(dev);

        /* Clear and mask all interrupts */
        axi_dmac_write(dev_cfg->base + AXI_DMAC_REG_IRQ_SOURCE, 0xFF);
        axi_dmac_write(dev_cfg->base + AXI_DMAC_REG_IRQ_MASK, 0xFF);

	/* Enable DMA controller */
        axi_dmac_write(dev_cfg->base + AXI_DMAC_REG_CTRL, AXI_DMAC_CTRL_ENABLE);
	
        /* Configure interrupts */
        dev_cfg->irq_config();

        /* Enable module's IRQ */
        irq_enable(dev_cfg->irq_id);

        SYS_LOG_INF("Device %s initialized", DEV_NAME(dev));

        return 0;
}

static const struct dma_driver_api axi_dmac_driver_api = {
        .config = axi_dmac_config,
        .start = axi_dmac_transfer_start,
        .stop = axi_dmac_transfer_stop,
};

static void axi_dmac_irq_config(void)
{
        IRQ_CONNECT(AXI_DMAC_IRQ, CONFIG_AXI_DMAC_IRQ_PRI, axi_dmac_isr,
                    DEVICE_GET(axi_dmac), 0);
}

static const struct axi_dmac_dev_cfg axi_dmac_config = {
        .base = AXI_DMAC_BASE_ADDR,
        .irq_config = axi_dmac_irq_config,
        .irq_id = AXI_DMAC_IRQ,
};

static struct axi_dmac_dev_data axi_dmac_data;

DEVICE_AND_API_INIT(axi_dmac, CONFIG_AXI_DMAC_NAME, &axi_dmac_initialize,
                    &axi_dmac_data, &axi_dmac_config, POST_KERNEL,
                    CONFIG_KERNEL_INIT_PRIORITY_DEVICE, &axi_dmac_driver_api);

