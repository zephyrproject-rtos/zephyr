/*
 * Xilinx AXI 1G / 2.5G Ethernet Subsystem
 *
 * Copyright(c) 2024, CISPA Helmholtz Center for Information Security
 * SPDX - License - Identifier : Apache-2.0
 */
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(eth_xilinx_axienet_mdio, CONFIG_ETHERNET_LOG_LEVEL);

#include <sys/types.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/mdio.h>
#include <zephyr/sys/barrier.h>

#define XILINX_AXIENET_MDIO_SETUP_REG_OFFSET                  0x00000500
#define XILINX_AXIENET_MDIO_SETUP_REG_MDIO_DISABLE_MASK       BIT_MASK(0)
#define XILINX_AXIENET_MDIO_SETUP_REG_MDIO_ENABLE_MASK        BIT(6)
#define XILINX_AXIENET_MDIO_SETUP_REG_MDIO_CLOCK_DIVIDER_MASK BIT_MASK(6)
#define XILINX_AXIENET_MDIO_CONTROL_REG_OFFSET                0x00000504
#define XILINX_AXIENET_MDIO_CONTROL_REG_MASK_READY            BIT(7)
#define XILINX_AXIENET_MDIO_CONTROL_REG_SHIFT_PHYADDR         24
#define XILINX_AXIENET_MDIO_CONTROL_REG_SHIFT_REGADDR         16
#define XILINX_AXIENET_MDIO_CONTROL_REG_SHIFT_TXOP            14
#define XILINX_AXIENET_MDIO_CONTROL_REG_MASK_READ             BIT(15)
#define XILINX_AXIENET_MDIO_CONTROL_REG_MASK_WRITE            BIT(14)
#define XILINX_AXIENET_MDIO_CONTROL_REG_MASK_INITIATE         BIT(11)
#define XILINX_AXIENET_MDIO_WRITE_DATA_REG_OFFSET             0x00000508
#define XILINX_AXIENET_MDIO_READ_DATA_REG_OFFSET              0x0000050C
#define XILINX_AXIENET_MDIO_READ_DATA_REG_DATA_MASK           BIT_MASK(16)
/* same mask for all interrupt-related registers */
#define XILINX_AXIENET_MDIO_INTERRUPT_MASK                    BIT(0)
#define XILINX_AXIENET_MDIO_INTERRUPT_STATUS_REG_OFFSET       0x00000600
#define XILINX_AXIENET_MDIO_INTERRUPT_PENDING_REG_OFFSET      0x00000610
#define XILINX_AXIENET_MDIO_INTERRUPT_ENABLE_REG_OFFSET       0x00000620
#define XILINX_AXIENET_MDIO_INTERRUPT_DISABLE_ALL_MASK        BIT_MASK(0)
#define XILINX_AXIENET_MDIO_INTERRUPT_CLEAR_REG_OFFSET        0x00000630
#define XILINX_AXIENET_MDIO_INTERRUPT_CLEAR_ALL_MASK          BIT_MASK(8)

/* 2.5 MHz, i.e., max MDIO clock according to IEEE spec */
#define XILINX_AXIENET_MDIO_MDIO_TARGET_FREQUENCY_HZ 2500000
#define XILINX_AXIENET_MDIO_INTERRUPT_TIMEOUT_MS     100

struct mdio_xilinx_axienet_data {
	struct k_sem irq_sema;
	uint16_t clock_divider;
	bool bus_enabled;
};

struct mdio_xilinx_axienet_config {
	void *reg;
	unsigned int clock_frequency_hz;
	void (*config_func)(const struct mdio_xilinx_axienet_data *data);
	bool have_irq;
};

static void xilinx_axienet_mdio_write_register(const struct mdio_xilinx_axienet_config *config,
					       uintptr_t reg_offset, uint32_t value)
{
	volatile uint32_t *reg_addr = (uint32_t *)((uint8_t *)(config->reg) + reg_offset);
	*reg_addr = value;
	barrier_dmem_fence_full(); /* make sure that write commits */
}

static uint32_t xilinx_axienet_read_mdio_register(const struct mdio_xilinx_axienet_config *config,
						  uintptr_t reg_offset)
{
	const volatile uint32_t *reg_addr = (uint32_t *)((uint8_t *)(config->reg) + reg_offset);
	const uint32_t ret = *reg_addr;

	barrier_dmem_fence_full(); /* make sure that read commits */
	return ret;
}

static void mdio_xilinx_axienet_bus_disable(const struct device *dev)
{
	const struct mdio_xilinx_axienet_config *config = dev->config;
	struct mdio_xilinx_axienet_data *data = dev->data;

	LOG_INF("Disable MDIO Bus!");

	xilinx_axienet_mdio_write_register(config, XILINX_AXIENET_MDIO_INTERRUPT_ENABLE_REG_OFFSET,
					   XILINX_AXIENET_MDIO_INTERRUPT_DISABLE_ALL_MASK);

	xilinx_axienet_mdio_write_register(config, XILINX_AXIENET_MDIO_SETUP_REG_OFFSET,
					   XILINX_AXIENET_MDIO_SETUP_REG_MDIO_DISABLE_MASK);
	data->bus_enabled = false;
}

static void enable_mdio_bus(const struct mdio_xilinx_axienet_config *config,
			    struct mdio_xilinx_axienet_data *data)
{

	if ((xilinx_axienet_read_mdio_register(config, XILINX_AXIENET_MDIO_SETUP_REG_OFFSET) &
	     XILINX_AXIENET_MDIO_SETUP_REG_MDIO_ENABLE_MASK) == 0) {
		int err;

		xilinx_axienet_mdio_write_register(config, XILINX_AXIENET_MDIO_SETUP_REG_OFFSET,
						   XILINX_AXIENET_MDIO_SETUP_REG_MDIO_ENABLE_MASK |
							   data->clock_divider);

		xilinx_axienet_mdio_write_register(config,
						   XILINX_AXIENET_MDIO_INTERRUPT_ENABLE_REG_OFFSET,
						   XILINX_AXIENET_MDIO_INTERRUPT_MASK);

		if (config->have_irq) {
			LOG_DBG("Waiting for bus enable!");
			err = k_sem_take(&data->irq_sema,
					 K_MSEC(XILINX_AXIENET_MDIO_INTERRUPT_TIMEOUT_MS));

			if (err != 0) {
				LOG_ERR("Could not enable MDIO bus: %d (%s)", err, strerror(-err));
			}
		}

		while ((xilinx_axienet_read_mdio_register(config,
							  XILINX_AXIENET_MDIO_SETUP_REG_OFFSET) &
			XILINX_AXIENET_MDIO_SETUP_REG_MDIO_ENABLE_MASK) == 0) {
			LOG_DBG("Waiting for bus enable!");
		}

	} else {
		data->bus_enabled = true;
	}
}

static void mdio_xilinx_axienet_bus_enable(const struct device *dev)
{
	const struct mdio_xilinx_axienet_config *config = dev->config;
	struct mdio_xilinx_axienet_data *data = dev->data;
	uint16_t clock_divider, clock_divider_full;

	if (!config->clock_frequency_hz) {
		LOG_ERR("No clock frequency specified for ethernet device!");
		return;
	}

	/* this might result in a MDIO frequency that is a bit lower than the max frequency */
	clock_divider_full = DIV_ROUND_UP(config->clock_frequency_hz,
					  XILINX_AXIENET_MDIO_MDIO_TARGET_FREQUENCY_HZ * 2);
	clock_divider = clock_divider_full & XILINX_AXIENET_MDIO_SETUP_REG_MDIO_CLOCK_DIVIDER_MASK;

	if (clock_divider != clock_divider_full) {
		LOG_ERR("Clock divider overflow!");
		/* maximum divider value - lowest MDIO frequency we can achieve */
		clock_divider = XILINX_AXIENET_MDIO_SETUP_REG_MDIO_CLOCK_DIVIDER_MASK;
	}

	data->clock_divider = clock_divider;

	LOG_INF("Enable MDIO Bus assuming ethernet clock frequency %u divider %u!",
		config->clock_frequency_hz, clock_divider);

	xilinx_axienet_mdio_write_register(config, XILINX_AXIENET_MDIO_SETUP_REG_OFFSET,
					   clock_divider);

	enable_mdio_bus(config, data);

	LOG_INF("MDIO ready!");
}

static int mdio_xilinx_axienet_read(const struct device *dev, uint8_t prtad, uint8_t devad,
				    uint16_t *data)
{
	const struct mdio_xilinx_axienet_config *config = dev->config;
	struct mdio_xilinx_axienet_data *dev_data = dev->data;
	int err;

	if (k_is_in_isr()) {
		LOG_ERR("Called MDIO read in ISR!");
		return -EWOULDBLOCK;
	}

	enable_mdio_bus(config, dev_data);

	if (!dev_data->bus_enabled) {
		LOG_ERR("Bus needs to be enabled!");
		return -EIO;
	}

	LOG_DBG("Waiting for IRQ from MDIO!");

	xilinx_axienet_mdio_write_register(
		config, XILINX_AXIENET_MDIO_CONTROL_REG_OFFSET,
		XILINX_AXIENET_MDIO_CONTROL_REG_MASK_INITIATE |
			(prtad << XILINX_AXIENET_MDIO_CONTROL_REG_SHIFT_PHYADDR) |
			(devad << XILINX_AXIENET_MDIO_CONTROL_REG_SHIFT_REGADDR) |
			XILINX_AXIENET_MDIO_CONTROL_REG_MASK_READ);

	xilinx_axienet_mdio_write_register(config, XILINX_AXIENET_MDIO_INTERRUPT_ENABLE_REG_OFFSET,
					   XILINX_AXIENET_MDIO_INTERRUPT_MASK);

	if (config->have_irq) {
		err = k_sem_take(&dev_data->irq_sema,
				 K_MSEC(XILINX_AXIENET_MDIO_INTERRUPT_TIMEOUT_MS));

		if (err != 0) {
			LOG_DBG("Error %d (%s) from IRQ semaphore - polling!", err, strerror(-err));
		}
	}

	while ((xilinx_axienet_read_mdio_register(config, XILINX_AXIENET_MDIO_CONTROL_REG_OFFSET) &
		XILINX_AXIENET_MDIO_CONTROL_REG_MASK_READY) == 0x0) {
		LOG_DBG("Transfer is not yet ready!");
	}

	LOG_DBG("IRQ from MDIO received - read complete!");

	*data = (uint16_t)(xilinx_axienet_read_mdio_register(
				   config, XILINX_AXIENET_MDIO_READ_DATA_REG_OFFSET) &
			   XILINX_AXIENET_MDIO_READ_DATA_REG_DATA_MASK);

	LOG_DBG("Read %" PRIu16 " from MDIO!", *data);

	return 0;
}

static int mdio_xilinx_axienet_write(const struct device *dev, uint8_t prtad, uint8_t devad,
				     uint16_t data)
{
	const struct mdio_xilinx_axienet_config *config = dev->config;
	struct mdio_xilinx_axienet_data *dev_data = dev->data;
	int err;

	if (k_is_in_isr()) {
		LOG_ERR("Called MDIO write in ISR!");
		return -EWOULDBLOCK;
	}

	enable_mdio_bus(config, dev_data);

	if (!dev_data->bus_enabled) {
		LOG_ERR("Bus needs to be enabled!");
		return -EIO;
	}

	LOG_DBG("Waiting for IRQ from MDIO!");

	xilinx_axienet_mdio_write_register(config, XILINX_AXIENET_MDIO_WRITE_DATA_REG_OFFSET, data);

	xilinx_axienet_mdio_write_register(config, XILINX_AXIENET_MDIO_INTERRUPT_ENABLE_REG_OFFSET,
					   XILINX_AXIENET_MDIO_INTERRUPT_MASK);

	xilinx_axienet_mdio_write_register(
		config, XILINX_AXIENET_MDIO_CONTROL_REG_OFFSET,
		XILINX_AXIENET_MDIO_CONTROL_REG_MASK_INITIATE |
			(prtad << XILINX_AXIENET_MDIO_CONTROL_REG_SHIFT_PHYADDR) |
			(devad << XILINX_AXIENET_MDIO_CONTROL_REG_SHIFT_REGADDR) |
			XILINX_AXIENET_MDIO_CONTROL_REG_MASK_WRITE);

	if (config->have_irq) {
		err = k_sem_take(&dev_data->irq_sema,
				 K_MSEC(XILINX_AXIENET_MDIO_INTERRUPT_TIMEOUT_MS));
		if (err != 0) {
			LOG_DBG("Error %d (%s) from IRQ semaphore - polling!", err, strerror(-err));
		}
	}
	while ((xilinx_axienet_read_mdio_register(config, XILINX_AXIENET_MDIO_CONTROL_REG_OFFSET) &
		XILINX_AXIENET_MDIO_CONTROL_REG_MASK_READY) == 0x0) {
		LOG_DBG("IRQ from MDIO received but transfer is not yet ready!");
	}

	LOG_DBG("IRQ from MDIO received - write complete!");

	return 0;
}

static inline void xilinx_axienet_mdio_isr(const struct device *dev)
{
	const struct mdio_xilinx_axienet_config *config = dev->config;
	struct mdio_xilinx_axienet_data *data = dev->data;
	uint32_t interrupt_status;

	interrupt_status = xilinx_axienet_read_mdio_register(
		config, XILINX_AXIENET_MDIO_INTERRUPT_STATUS_REG_OFFSET);

	if (interrupt_status & XILINX_AXIENET_MDIO_INTERRUPT_MASK) {
		k_sem_give(&data->irq_sema);
		LOG_DBG("MDIO interrupt received!");
	} else {
		LOG_DBG("Unknown interrupt received: %x!", interrupt_status);
	}
	xilinx_axienet_mdio_write_register(config, XILINX_AXIENET_MDIO_INTERRUPT_CLEAR_REG_OFFSET,
					   XILINX_AXIENET_MDIO_INTERRUPT_MASK);
}

static int xilinx_axienet_mdio_probe(const struct device *dev)
{
	const struct mdio_xilinx_axienet_config *config = dev->config;
	struct mdio_xilinx_axienet_data *data = dev->data;
	int err;

	if (config->have_irq) {
		err = k_sem_init(&data->irq_sema, 0, K_SEM_MAX_LIMIT);

		if (err != 0) {
			LOG_ERR("Could not init semaphore: error %d (%s)", err, strerror(-err));
			return err;
		}
	}

	LOG_INF("Enabling IRQ!");
	config->config_func(data);

	return 0;
}

static const struct mdio_driver_api mdio_xilinx_axienet_api = {
	.bus_disable = mdio_xilinx_axienet_bus_disable,
	.bus_enable = mdio_xilinx_axienet_bus_enable,
	.read = mdio_xilinx_axienet_read,
	.write = mdio_xilinx_axienet_write};

#define SETUP_IRQS(inst)                                                                           \
	IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority), xilinx_axienet_mdio_isr,      \
		    DEVICE_DT_INST_GET(inst), 0);                                                  \
                                                                                                   \
	irq_enable(DT_INST_IRQN(inst))

#define XILINX_AXIENET_MDIO_INIT(inst)                                                             \
                                                                                                   \
	static void xilinx_axienet_mdio_config_##inst(const struct mdio_xilinx_axienet_data *data) \
	{                                                                                          \
                                                                                                   \
		COND_CODE_1(DT_INST_NODE_HAS_PROP(inst, interrupts), (SETUP_IRQS(inst)),           \
			    (LOG_INF("No IRQs defined!")));        \
	}                                                                                          \
	static const struct mdio_xilinx_axienet_config mdio_xilinx_axienet_config##inst = {        \
		.config_func = xilinx_axienet_mdio_config_##inst,                                  \
		.reg = (void *)(uintptr_t)DT_REG_ADDR(DT_INST_PARENT(inst)),                       \
		.clock_frequency_hz = DT_INST_PROP(inst, clock_frequency),                         \
		.have_irq = DT_INST_NODE_HAS_PROP(inst, interrupts)};                              \
	static struct mdio_xilinx_axienet_data mdio_xilinx_axienet_data##inst = {0};               \
	DEVICE_DT_INST_DEFINE(inst, xilinx_axienet_mdio_probe, NULL,                               \
			      &mdio_xilinx_axienet_data##inst, &mdio_xilinx_axienet_config##inst,  \
			      POST_KERNEL, CONFIG_MDIO_INIT_PRIORITY, &mdio_xilinx_axienet_api);

#define DT_DRV_COMPAT xlnx_axi_ethernet_1_00_a_mdio
DT_INST_FOREACH_STATUS_OKAY(XILINX_AXIENET_MDIO_INIT)
