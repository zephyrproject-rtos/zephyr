/*
 * Copyright (c) 2025 Michael Estes
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_pcal9722

#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pcal9722, CONFIG_GPIO_LOG_LEVEL);

#include <zephyr/drivers/gpio/gpio_utils.h>

/* PCAL9722 Register addresses */
#define PCAL9722_INPUT_PORT0     0x00
#define PCAL9722_OUTPUT_PORT0    0x04
#define PCAL9722_CONFIG_PORT0    0x0C
#define PCAL9722_PULL_EN0        0x4C
#define PCAL9722_PULL_SEL0       0x50
#define PCAL9722_IRQMASK_PORT0   0x54
#define PCAL9722_IRQ_STAT_PORT0  0x58
#define PCAL9722_IRQEDGE_PORT0_A 0x60
#define PCAL9722_IRQ_CLEAR_PORT0 0x68

/* Bit instructs PCAL9722 to auto-increment register address between SPI bytes */
#define PCAL9722_AUTO_INC BIT(7)

/* Base address of the PCAL9722 sent as the first byte in every SPI transceive */
#define PCAL9722_ADDRESS 0x40

/* Set this bit in PCAL9722_ADDRESS to address another PCAL9722 device on the same chip select */
#define ADDRESS_BIT (BIT(1))

#define PCAL9722_READ_CMD BIT(0)
#define NUM_PINS          22
#define ALL_PINS          ((uint32_t)BIT_MASK(NUM_PINS))

#define PCAL9722_ENABLE_IRQ_SUPPORT DT_ANY_INST_HAS_PROP_STATUS_OKAY(irq_gpios)

struct pcal9722_drv_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	struct k_sem *lock;
	struct gpio_callback gpio_cb;
	struct k_work work;
	const struct device *dev;
	sys_slist_t cb;
};

struct pcal9722_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	struct spi_dt_spec spi;
#if PCAL9722_ENABLE_IRQ_SUPPORT
	const struct gpio_dt_spec gpio_int;
#endif
	const uint8_t addr;
};

static int gpio_pcal9722_reg_read(const struct pcal9722_config *cfg, uint8_t reg, uint8_t *val,
				  uint8_t len)
{
	uint8_t cmd_buf[2 + len];

	cmd_buf[0] = cfg->addr | PCAL9722_READ_CMD;
	cmd_buf[1] = reg;

	const struct spi_buf tx_buf = {.buf = cmd_buf, .len = ARRAY_SIZE(cmd_buf)};
	const struct spi_buf rx_buf = {.buf = cmd_buf, .len = ARRAY_SIZE(cmd_buf)};
	const struct spi_buf_set tx = {.buffers = &tx_buf, .count = 1};
	const struct spi_buf_set rx = {.buffers = &rx_buf, .count = 1};
	int ret;

	ret = spi_transceive_dt(&cfg->spi, &tx, &rx);
	if (ret < 0) {
		return ret;
	}

	memcpy(val, &cmd_buf[2], len);

	return 0;
}

static int gpio_pcal9722_reg_write(const struct pcal9722_config *cfg, uint8_t reg, uint8_t *val,
				   uint8_t len)
{
	uint8_t cmd_buf[2] = {cfg->addr, PCAL9722_AUTO_INC | reg};
	const struct spi_buf tx_buf[2] = {
		{.buf = cmd_buf, .len = sizeof(cmd_buf)},
		{.buf = val, .len = len},
	};
	const struct spi_buf_set tx = {.buffers = &tx_buf[0], .count = 2};

	return spi_transceive_dt(&cfg->spi, &tx, NULL);
}

static int gpio_pcal9722_config(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	const struct pcal9722_config *cfg = dev->config;
	struct pcal9722_drv_data *drv_data = dev->data;
	uint32_t pullen = 0;
	uint32_t pull = 0;
	uint32_t dir = 0;
	uint32_t val = 0;
	int rc = 0;

	/* Can't do SPI bus operations from an ISR */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	/* Single Ended lines (Open drain and open source) not supported */
	if ((flags & GPIO_SINGLE_ENDED) != 0) {
		return -ENOTSUP;
	}

	/* Simultaneous input & output mode not supported */
	if (((flags & GPIO_INPUT) != 0) && ((flags & GPIO_OUTPUT) != 0)) {
		return -ENOTSUP;
	}

	k_sem_take(drv_data->lock, K_FOREVER);

	rc = gpio_pcal9722_reg_read(cfg, PCAL9722_CONFIG_PORT0, (uint8_t *)&dir, sizeof(dir));
	if (rc != 0) {
		goto out;
	}
	dir = sys_le32_to_cpu(dir) & ALL_PINS;

	rc = gpio_pcal9722_reg_read(cfg, PCAL9722_OUTPUT_PORT0, (uint8_t *)&val, sizeof(val));
	if (rc != 0) {
		goto out;
	}
	val = sys_le32_to_cpu(val) & ALL_PINS;

	rc = gpio_pcal9722_reg_read(cfg, PCAL9722_PULL_EN0, (uint8_t *)&pullen, sizeof(pullen));
	if (rc != 0) {
		goto out;
	}
	pullen = sys_le32_to_cpu(pullen) & ALL_PINS;

	rc = gpio_pcal9722_reg_read(cfg, PCAL9722_PULL_SEL0, (uint8_t *)&pull, sizeof(pull));
	if (rc != 0) {
		goto out;
	}
	pull = sys_le32_to_cpu(pull) & ALL_PINS;

	/* Ensure either Output or Input is specified */
	WRITE_BIT(dir, pin, (flags & GPIO_INPUT));
	if (flags & GPIO_OUTPUT_INIT_LOW || flags & GPIO_OUTPUT_INIT_HIGH) {
		WRITE_BIT(val, pin, (flags & GPIO_OUTPUT_INIT_HIGH));
	}

	if (flags & GPIO_PULL_UP || flags & GPIO_PULL_DOWN) {
		WRITE_BIT(pullen, pin, 1);
		WRITE_BIT(pull, pin, (flags & GPIO_PULL_UP));
	} else {
		WRITE_BIT(pullen, pin, 0);
	}

	pull = sys_cpu_to_le32(pull & ALL_PINS);
	pullen = sys_cpu_to_le32(pull & ALL_PINS);
	val = sys_cpu_to_le32(val & ALL_PINS);
	dir = sys_cpu_to_le32(dir & ALL_PINS);

	rc = gpio_pcal9722_reg_write(cfg, PCAL9722_OUTPUT_PORT0, (uint8_t *)&val, 3);
	if (rc != 0) {
		goto out;
	}

	rc = gpio_pcal9722_reg_write(cfg, PCAL9722_CONFIG_PORT0, (uint8_t *)&dir, 3);
	if (rc != 0) {
		goto out;
	}

	rc = gpio_pcal9722_reg_write(cfg, PCAL9722_PULL_SEL0, (uint8_t *)&pull, 3);
	if (rc != 0) {
		goto out;
	}

	rc = gpio_pcal9722_reg_write(cfg, PCAL9722_PULL_EN0, (uint8_t *)&pullen, 3);
	if (rc != 0) {
		goto out;
	}

out:
	k_sem_give(drv_data->lock);
	return rc;
}

static int gpio_pcal9722_port_read(const struct device *dev, gpio_port_value_t *value)
{
	const struct pcal9722_config *cfg = dev->config;
	struct pcal9722_drv_data *drv_data = dev->data;
	uint32_t data = 0;
	int rc = 0;

	/* Can't do SPI bus operations from an ISR */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_sem_take(drv_data->lock, K_FOREVER);

	/* Read Input Register */
	rc = gpio_pcal9722_reg_read(cfg, PCAL9722_INPUT_PORT0, (uint8_t *)&data, 3);
	data = sys_le32_to_cpu(data) & ALL_PINS;

	LOG_DBG("read 0x%06X got %d", data, rc);

	if (rc == 0) {
		*value = (gpio_port_value_t)(data);
	}

	k_sem_give(drv_data->lock);
	return rc;
}

static int gpio_pcal9722_port_write(const struct device *dev, gpio_port_pins_t mask,
				    gpio_port_value_t value, gpio_port_value_t toggle)
{
	const struct pcal9722_config *cfg = dev->config;
	struct pcal9722_drv_data *drv_data = dev->data;
	uint32_t data = 0;
	int rc = 0;

	/* Can't do SPI bus operations from an ISR */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_sem_take(drv_data->lock, K_FOREVER);

	gpio_pcal9722_reg_read(cfg, PCAL9722_OUTPUT_PORT0, (uint8_t *)&data, 3);
	data = sys_le32_to_cpu(data) & ALL_PINS;
	data = sys_cpu_to_le32((((data & ~mask) | (value & mask)) ^ toggle) & ALL_PINS);

	rc = gpio_pcal9722_reg_write(cfg, PCAL9722_OUTPUT_PORT0, (uint8_t *)&data, 3);

	k_sem_give(drv_data->lock);

	LOG_DBG("write %x msk %08x val %08x => %x: %d", data, mask, value, data, rc);

	return rc;
}

static int gpio_pcal9722_port_set_masked(const struct device *dev, gpio_port_pins_t mask,
					 gpio_port_value_t value)
{
	return gpio_pcal9722_port_write(dev, mask, value, 0);
}

static int gpio_pcal9722_port_set_bits(const struct device *dev, gpio_port_pins_t pins)
{
	return gpio_pcal9722_port_write(dev, pins, pins, 0);
}

static int gpio_pcal9722_port_clear_bits(const struct device *dev, gpio_port_pins_t pins)
{
	return gpio_pcal9722_port_write(dev, pins, 0, 0);
}

static int gpio_pcal9722_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	return gpio_pcal9722_port_write(dev, 0, 0, pins);
}

#if PCAL9722_ENABLE_IRQ_SUPPORT
static int gpio_pcal9722_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
						 enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	const struct pcal9722_config *cfg = dev->config;
	struct pcal9722_drv_data *drv_data = dev->data;
	uint64_t irqedge = 0;
	uint32_t irqmask = 0;
	int rc = 0;

	k_sem_take(drv_data->lock, K_FOREVER);

	rc = gpio_pcal9722_reg_read(cfg, PCAL9722_IRQEDGE_PORT0_A, (uint8_t *)&irqedge, 6);
	if (rc != 0) {
		goto out;
	}

	rc = gpio_pcal9722_reg_read(cfg, PCAL9722_IRQMASK_PORT0, (uint8_t *)&irqmask, 3);
	if (rc != 0) {
		goto out;
	}

	irqedge = sys_le64_to_cpu(irqedge) & BIT64_MASK(NUM_PINS * 2);
	irqmask = sys_le32_to_cpu(irqmask) & ALL_PINS;

	if (mode == GPIO_INT_MODE_DISABLED) {
		WRITE_BIT(irqmask, pin, 1);
	} else if (mode == GPIO_INT_MODE_LEVEL) {
		WRITE_BIT(irqmask, pin, 0);
		irqedge &= ~(GENMASK64(1, 0) << (pin * 2));
	} else { /* GPIO_INT_MODE_EDGE */
		WRITE_BIT(irqmask, pin, 0);
		if (trig == GPIO_INT_TRIG_BOTH) {
			/* Set both edge trigger bits for this pin */
			irqedge |= (GENMASK64(1, 0) << (pin * 2));
		} else if (trig == GPIO_INT_TRIG_LOW) {
			/* Set the falling-edge bit, clear the rising edge bit */
			irqedge &= ~(GENMASK64(1, 0) << (pin * 2));
			irqedge |= (BIT64(1) << (pin * 2));
		} else { /* GPIO_INT_TRIG_HIGH */
			/* Set the rising-edge bit, clear the falling-edge bit */
			irqedge &= ~(GENMASK64(1, 0) << (pin * 2));
			irqedge |= (BIT64(0) << (pin * 2));
		}
	}

	irqedge = sys_cpu_to_le64(irqedge & BIT64_MASK(NUM_PINS * 2));
	irqmask = sys_cpu_to_le32(irqmask & ALL_PINS);

	rc = gpio_pcal9722_reg_write(cfg, PCAL9722_IRQMASK_PORT0, (uint8_t *)&irqmask, 3);
	if (rc != 0) {
		goto out;
	}
	rc = gpio_pcal9722_reg_write(cfg, PCAL9722_IRQEDGE_PORT0_A, (uint8_t *)&irqedge, 6);

out:
	k_sem_give(drv_data->lock);

	return rc;
}

static int gpio_pcal9722_manage_callback(const struct device *dev, struct gpio_callback *callback,
					 bool set)
{
	struct pcal9722_drv_data *data = dev->data;

	return gpio_manage_callback(&data->cb, callback, set);
}

static void gpio_pcal9722_handle_interrupt(const struct device *dev)
{
	const struct pcal9722_config *cfg = dev->config;
	struct pcal9722_drv_data *drv_data = dev->data;
	int rc = 0;
	uint32_t irq_status = 0;

	k_sem_take(drv_data->lock, K_FOREVER);

	rc = gpio_pcal9722_reg_read(cfg, PCAL9722_IRQ_STAT_PORT0, (uint8_t *)&irq_status, 3);
	if (rc) {
		goto out;
	}

	rc = gpio_pcal9722_reg_write(cfg, PCAL9722_IRQ_CLEAR_PORT0, (uint8_t *)&irq_status, 3);
	if (rc != 0) {
		goto out;
	}

	irq_status = sys_le32_to_cpu(irq_status) & ALL_PINS;

out:
	k_sem_give(drv_data->lock);

	if ((rc == 0) && (irq_status)) {
		gpio_fire_callbacks(&drv_data->cb, dev, irq_status);
	}
}

static void gpio_pcal9722_work_handler(struct k_work *work)
{
	struct pcal9722_drv_data *drv_data = CONTAINER_OF(work, struct pcal9722_drv_data, work);

	gpio_pcal9722_handle_interrupt(drv_data->dev);
}

static void gpio_pcal9722_init_cb(const struct device *dev, struct gpio_callback *gpio_cb,
				  uint32_t pins)
{
	struct pcal9722_drv_data *drv_data =
		CONTAINER_OF(gpio_cb, struct pcal9722_drv_data, gpio_cb);
	ARG_UNUSED(pins);

	k_work_submit(&drv_data->work);
}
#endif /* PCAL9722_ENABLE_IRQ_SUPPORT */

static int gpio_pcal9722_init(const struct device *dev)
{
	const struct pcal9722_config *cfg = dev->config;
#if PCAL9722_ENABLE_IRQ_SUPPORT
	struct pcal9722_drv_data *drv_data = dev->data;
#endif
	uint32_t pins = 0;
	int rc = 0;

	if (!device_is_ready(cfg->spi.bus)) {
		LOG_ERR("SPI bus not ready");
		goto out;
	}

	/* Clear interrupts */
	pins = sys_cpu_to_le32(ALL_PINS);
	rc = gpio_pcal9722_reg_write(cfg, PCAL9722_IRQ_CLEAR_PORT0, (uint8_t *)&pins, 3);
	if (rc != 0) {
		goto out;
	}

#if PCAL9722_ENABLE_IRQ_SUPPORT
	if (!gpio_is_ready_dt(&cfg->gpio_int)) {
		LOG_ERR("Interrupt GPIO not ready");
		rc = -EINVAL;
		goto out;
	}

	drv_data->dev = dev;

	k_work_init(&drv_data->work, gpio_pcal9722_work_handler);

	rc = gpio_pin_configure_dt(&cfg->gpio_int, GPIO_INPUT);
	if (rc) {
		goto out;
	}

	rc = gpio_pin_interrupt_configure_dt(&cfg->gpio_int, GPIO_INT_EDGE_TO_ACTIVE);
	if (rc) {
		goto out;
	}

	gpio_init_callback(&drv_data->gpio_cb, gpio_pcal9722_init_cb, BIT(cfg->gpio_int.pin));
	rc = gpio_add_callback(cfg->gpio_int.port, &drv_data->gpio_cb);

	if (rc) {
		goto out;
	}
#endif /* PCAL9722_ENABLE_IRQ_SUPPORT */

out:
	if (rc) {
		LOG_ERR("%s failed to initialize: %d", dev->name, rc);
	}

	return rc;
}

static DEVICE_API(gpio, api_table) = {
	.pin_configure = gpio_pcal9722_config,
	.port_get_raw = gpio_pcal9722_port_read,
	.port_set_masked_raw = gpio_pcal9722_port_set_masked,
	.port_set_bits_raw = gpio_pcal9722_port_set_bits,
	.port_clear_bits_raw = gpio_pcal9722_port_clear_bits,
	.port_toggle_bits = gpio_pcal9722_port_toggle_bits,
#if PCAL9722_ENABLE_IRQ_SUPPORT
	.pin_interrupt_configure = gpio_pcal9722_pin_interrupt_configure,
	.manage_callback = gpio_pcal9722_manage_callback,
#endif
};

#if PCAL9722_ENABLE_IRQ_SUPPORT
#define IRQ_GPIO(n) .gpio_int = GPIO_DT_SPEC_INST_GET(n, irq_gpios)
#else
#define IRQ_GPIO(n)
#endif

#define GPIO_PCAL9722_INIT(n)                                                                      \
	static const struct pcal9722_config pcal9722_cfg_##n = {                                   \
		.spi = SPI_DT_SPEC_INST_GET(n, SPI_OP_MODE_MASTER | SPI_WORD_SET(8), 0),           \
		.common =                                                                          \
			{                                                                          \
				.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(n),               \
			},                                                                         \
		.addr = DT_INST_PROP(n, addr),                                                     \
		IRQ_GPIO(n)};                                                                      \
	static K_SEM_DEFINE(pcal9722_drvdata_##n_lock, 1, 1);                                      \
	static struct pcal9722_drv_data pcal9722_drvdata_##n = {                                   \
		.lock = &pcal9722_drvdata_##n_lock};                                               \
	DEVICE_DT_INST_DEFINE(n, gpio_pcal9722_init, NULL, &pcal9722_drvdata_##n,                  \
			      &pcal9722_cfg_##n, POST_KERNEL, CONFIG_GPIO_PCAL9722_INIT_PRIORITY,  \
			      &api_table);

DT_INST_FOREACH_STATUS_OKAY(GPIO_PCAL9722_INIT)
