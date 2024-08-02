/*
 * Copyright (c) 2018 Aapo Vienamo
 * Copyright (c) 2018 Peter Bigot Consulting, LLC
 * Copyright (c) 2019-2020 Nordic Semiconductor ASA
 * Copyright (c) 2020 ZedBlox Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT semtech_sx1509b

#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_sx1509b.h>
#include <zephyr/dt-bindings/gpio/semtech-sx1509b.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/util.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sx1509b, CONFIG_GPIO_LOG_LEVEL);

#include <zephyr/drivers/gpio/gpio_utils.h>

/* Number of pins supported by the device */
#define NUM_PINS 16

/* Max to select all pins supported on the device. */
#define ALL_PINS ((uint16_t)BIT_MASK(NUM_PINS))

/* Reset delay is 2.5 ms, round up for Zephyr resolution */
#define RESET_DELAY_MS 3

/** Cache of the output configuration and data of the pins. */
struct sx1509b_pin_state {
	uint16_t input_disable;    /* 0x00 */
	uint16_t long_slew;        /* 0x02 */
	uint16_t low_drive;        /* 0x04 */
	uint16_t pull_up;          /* 0x06 */
	uint16_t pull_down;        /* 0x08 */
	uint16_t open_drain;       /* 0x0A */
	uint16_t polarity;         /* 0x0C */
	uint16_t dir;              /* 0x0E */
	uint16_t data;             /* 0x10 */
} __packed;

struct sx1509b_irq_state {
	uint16_t interrupt_mask;   /* 0x12 */
	uint32_t interrupt_sense;  /* 0x14, 0x16 */
} __packed;

struct sx1509b_debounce_state {
	uint8_t debounce_config;	/* 0x22 */
	uint16_t debounce_enable;  /* 0x23 */
} __packed;

/** Runtime driver data */
struct sx1509b_drv_data {
	/* gpio_driver_data needs to be first */
	struct gpio_driver_data common;
	struct sx1509b_pin_state pin_state;
	uint16_t led_drv_enable;
	struct sx1509b_debounce_state debounce_state;
	struct k_sem lock;

#ifdef CONFIG_GPIO_SX1509B_INTERRUPT
	struct gpio_callback gpio_cb;
	struct k_work work;
	struct sx1509b_irq_state irq_state;
	const struct device *dev;
	/* user ISR cb */
	sys_slist_t cb;
#endif /* CONFIG_GPIO_SX1509B_INTERRUPT */

};

/** Configuration data */
struct sx1509b_config {
	/* gpio_driver_config needs to be first */
	struct gpio_driver_config common;
	struct i2c_dt_spec bus;
#ifdef CONFIG_GPIO_SX1509B_INTERRUPT
	struct gpio_dt_spec nint_gpio;
#endif /* CONFIG_GPIO_SX1509B_INTERRUPT */
};

/* General configuration register addresses */
enum {
	/* TODO: Add rest of the regs */
	SX1509B_REG_CLOCK       = 0x1e,
	SX1509B_REG_RESET       = 0x7d,
};

/* Magic values for softreset */
enum {
	SX1509B_REG_RESET_MAGIC0        = 0x12,
	SX1509B_REG_RESET_MAGIC1        = 0x34,
};

/* Register bits for SX1509B_REG_CLOCK */
enum {
	SX1509B_REG_CLOCK_FOSC_OFF      = 0 << 5,
	SX1509B_REG_CLOCK_FOSC_EXT      = 1 << 5,
	SX1509B_REG_CLOCK_FOSC_INT_2MHZ = 2 << 5,
};

/* Register bits for SX1509B_REG_MISC */
enum {
	SX1509B_REG_MISC_LOG_A          = 1 << 3,
	SX1509B_REG_MISC_LOG_B          = 1 << 7,
	/* ClkX = fOSC */
	SX1509B_REG_MISC_FREQ           = 1 << 4,
};

/* Pin configuration register addresses */
enum {
	SX1509B_REG_INPUT_DISABLE       = 0x00,
	SX1509B_REG_PULL_UP             = 0x06,
	SX1509B_REG_PULL_DOWN           = 0x08,
	SX1509B_REG_OPEN_DRAIN          = 0x0a,
	SX1509B_REG_DIR                 = 0x0e,
	SX1509B_REG_DATA                = 0x10,
	SX1509B_REG_INTERRUPT_MASK      = 0x12,
	SX1509B_REG_INTERRUPT_SENSE     = 0x14,
	SX1509B_REG_INTERRUPT_SENSE_B   = 0x14,
	SX1509B_REG_INTERRUPT_SENSE_A   = 0x16,
	SX1509B_REG_INTERRUPT_SOURCE    = 0x18,
	SX1509B_REG_MISC                = 0x1f,
	SX1509B_REG_LED_DRV_ENABLE      = 0x20,
	SX1509B_REG_DEBOUNCE_CONFIG     = 0x22,
	SX1509B_REG_DEBOUNCE_ENABLE     = 0x23,
};

/* Edge sensitivity types */
enum {
	SX1509B_EDGE_NONE     = 0x00,
	SX1509B_EDGE_RISING   = 0x01,
	SX1509B_EDGE_FALLING  = 0x02,
	SX1509B_EDGE_BOTH     = 0x03,
};

/* Intensity register addresses for all 16 pins */
static const uint8_t intensity_registers[16] = { 0x2a, 0x2d, 0x30, 0x33,
						 0x36, 0x3b, 0x40, 0x45,
						 0x4a, 0x4d, 0x50, 0x53,
						 0x56, 0x5b, 0x60, 0x65 };

/**
 * @brief Write a big-endian word to an internal address of an I2C slave.
 *
 * @param dev Pointer to the I2C bus spec.
 * @param reg_addr Address of the internal register being written.
 * @param value Value to be written to internal register.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 */
static inline int i2c_reg_write_word_be(const struct i2c_dt_spec *bus,
					uint8_t reg_addr, uint16_t value)
{
	uint8_t tx_buf[3] = { reg_addr, value >> 8, value & 0xff };

	return i2c_write_dt(bus, tx_buf, 3);
}

/**
 * @brief Write a big-endian byte to an internal address of an I2C slave.
 *
 * @param bus Pointer to the I2C bus spec.
 * @param reg_addr Address of the internal register being written.
 * @param value Value to be written to internal register.
 *
 * @retval 0 If successful.
 * @retval -EIO General input / output error.
 */
static inline int i2c_reg_write_byte_be(const struct i2c_dt_spec *bus,
					uint8_t reg_addr, uint8_t value)
{
	uint8_t tx_buf[3] = { reg_addr, value };

	return i2c_write_dt(bus, tx_buf, 2);
}

#ifdef CONFIG_GPIO_SX1509B_INTERRUPT
static int sx1509b_handle_interrupt(const struct device *dev)
{
	const struct sx1509b_config *cfg = dev->config;
	struct sx1509b_drv_data *drv_data = dev->data;
	int ret = 0;
	uint16_t int_source;
	uint8_t cmd = SX1509B_REG_INTERRUPT_SOURCE;

	k_sem_take(&drv_data->lock, K_FOREVER);

	ret = i2c_write_read_dt(&cfg->bus, &cmd, sizeof(cmd),
				(uint8_t *)&int_source, sizeof(int_source));
	if (ret != 0) {
		goto out;
	}

	int_source = sys_be16_to_cpu(int_source);

	/* reset interrupts before invoking callbacks */
	ret = i2c_reg_write_word_be(&cfg->bus, SX1509B_REG_INTERRUPT_SOURCE,
				    int_source);

out:
	k_sem_give(&drv_data->lock);

	if (ret == 0) {
		gpio_fire_callbacks(&drv_data->cb, dev, int_source);
	}

	return ret;
}

static void sx1509b_work_handler(struct k_work *work)
{
	struct sx1509b_drv_data *drv_data =
		CONTAINER_OF(work, struct sx1509b_drv_data, work);

	sx1509b_handle_interrupt(drv_data->dev);
}

static void sx1509_int_cb(const struct device *dev,
			   struct gpio_callback *gpio_cb,
			   uint32_t pins)
{
	struct sx1509b_drv_data *drv_data = CONTAINER_OF(gpio_cb,
		struct sx1509b_drv_data, gpio_cb);

	ARG_UNUSED(pins);

	k_work_submit(&drv_data->work);
}
#endif

static int write_pin_state(const struct sx1509b_config *cfg,
			   struct sx1509b_drv_data *drv_data,
			   struct sx1509b_pin_state *pins, bool data_first)
{
	struct {
		uint8_t reg;
		struct sx1509b_pin_state pins;
	} __packed pin_buf;
	int rc;

	pin_buf.reg = SX1509B_REG_INPUT_DISABLE;
	pin_buf.pins.input_disable = sys_cpu_to_be16(pins->input_disable);
	pin_buf.pins.long_slew = sys_cpu_to_be16(pins->long_slew);
	pin_buf.pins.low_drive = sys_cpu_to_be16(pins->low_drive);
	pin_buf.pins.pull_up = sys_cpu_to_be16(pins->pull_up);
	pin_buf.pins.pull_down = sys_cpu_to_be16(pins->pull_down);
	pin_buf.pins.open_drain = sys_cpu_to_be16(pins->open_drain);
	pin_buf.pins.polarity = sys_cpu_to_be16(pins->polarity);
	pin_buf.pins.dir = sys_cpu_to_be16(pins->dir);
	pin_buf.pins.data = sys_cpu_to_be16(pins->data);

	if (data_first) {
		rc = i2c_reg_write_word_be(&cfg->bus, SX1509B_REG_DATA,
					   pins->data);
		if (rc == 0) {
			rc = i2c_write_dt(&cfg->bus, &pin_buf.reg,
					  sizeof(pin_buf) - sizeof(pins->data));
		}
	} else {
		rc = i2c_write_dt(&cfg->bus, &pin_buf.reg, sizeof(pin_buf));
	}

	return rc;
}

static int sx1509b_config(const struct device *dev,
			  gpio_pin_t pin,
			  gpio_flags_t flags)
{
	const struct sx1509b_config *cfg = dev->config;
	struct sx1509b_drv_data *drv_data = dev->data;
	struct sx1509b_pin_state *pins = &drv_data->pin_state;
	struct sx1509b_debounce_state *debounce = &drv_data->debounce_state;
	int rc = 0;
	bool data_first = false;

	/* Can't do I2C bus operations from an ISR */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_sem_take(&drv_data->lock, K_FOREVER);

	if (drv_data->led_drv_enable & BIT(pin)) {
		/* Disable LED driver */
		drv_data->led_drv_enable &= ~BIT(pin);
		rc = i2c_reg_write_word_be(&cfg->bus,
					   SX1509B_REG_LED_DRV_ENABLE,
					   drv_data->led_drv_enable);

		if (rc) {
			goto out;
		}
	}

	pins->open_drain &= ~BIT(pin);
	if ((flags & GPIO_SINGLE_ENDED) != 0) {
		if ((flags & GPIO_LINE_OPEN_DRAIN) != 0) {
			pins->open_drain |= BIT(pin);
		} else {
			/* Open source not supported */
			rc = -ENOTSUP;
			goto out;
		}
	}

	if ((flags & GPIO_PULL_UP) != 0) {
		pins->pull_up |= BIT(pin);
	} else {
		pins->pull_up &= ~BIT(pin);
	}
	if ((flags & GPIO_PULL_DOWN) != 0) {
		pins->pull_down |= BIT(pin);
	} else {
		pins->pull_down &= ~BIT(pin);
	}

	if ((flags & GPIO_INPUT) != 0) {
		pins->input_disable &= ~BIT(pin);
	} else {
		pins->input_disable |= BIT(pin);
	}

	if ((flags & GPIO_OUTPUT) != 0) {
		pins->dir &= ~BIT(pin);
		if ((flags & GPIO_OUTPUT_INIT_LOW) != 0) {
			pins->data &= ~BIT(pin);
			data_first = true;
		} else if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0) {
			pins->data |= BIT(pin);
			data_first = true;
		}
	} else {
		pins->dir |= BIT(pin);
	}

	if ((flags & SX1509B_GPIO_DEBOUNCE) != 0) {
		debounce->debounce_enable |= BIT(pin);
	} else {
		debounce->debounce_enable &= ~BIT(pin);
	}

	LOG_DBG("CFG %u %x : ID %04x ; PU %04x ; PD %04x ; DIR %04x ; DAT %04x",
		pin, flags,
		pins->input_disable, pins->pull_up, pins->pull_down,
		pins->dir, pins->data);

	rc = write_pin_state(cfg, drv_data, pins, data_first);

	if (rc == 0) {
		struct {
			uint8_t reg;
			struct sx1509b_debounce_state debounce;
		} __packed debounce_buf;

		debounce_buf.reg = SX1509B_REG_DEBOUNCE_CONFIG;
		debounce_buf.debounce.debounce_config
			= debounce->debounce_config;
		debounce_buf.debounce.debounce_enable
			= sys_cpu_to_be16(debounce->debounce_enable);

		rc = i2c_write_dt(&cfg->bus, &debounce_buf.reg,
				  sizeof(debounce_buf));
	}

out:
	k_sem_give(&drv_data->lock);
	return rc;
}

static int port_get(const struct device *dev,
		    gpio_port_value_t *value)
{
	const struct sx1509b_config *cfg = dev->config;
	struct sx1509b_drv_data *drv_data = dev->data;
	uint16_t pin_data;
	int rc = 0;

	/* Can't do I2C bus operations from an ISR */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	k_sem_take(&drv_data->lock, K_FOREVER);

	uint8_t cmd = SX1509B_REG_DATA;

	rc = i2c_write_read_dt(&cfg->bus, &cmd, sizeof(cmd), &pin_data,
			       sizeof(pin_data));
	LOG_DBG("read %04x got %d", sys_be16_to_cpu(pin_data), rc);
	if (rc != 0) {
		goto out;
	}

	*value = sys_be16_to_cpu(pin_data);

out:
	k_sem_give(&drv_data->lock);
	return rc;
}

static int port_write(const struct device *dev,
		      gpio_port_pins_t mask,
		      gpio_port_value_t value,
		      gpio_port_value_t toggle)
{
	/* Can't do I2C bus operations from an ISR */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	const struct sx1509b_config *cfg = dev->config;
	struct sx1509b_drv_data *drv_data = dev->data;
	void *data = &drv_data->pin_state.data;
	uint16_t *outp = data;

	__ASSERT_NO_MSG(IS_PTR_ALIGNED(data, uint16_t));

	k_sem_take(&drv_data->lock, K_FOREVER);

	uint16_t orig_out = *outp;
	uint16_t out = ((orig_out & ~mask) | (value & mask)) ^ toggle;
	int rc = i2c_reg_write_word_be(&cfg->bus, SX1509B_REG_DATA, out);
	if (rc == 0) {
		*outp = out;
	}

	k_sem_give(&drv_data->lock);

	LOG_DBG("write %04x msk %04x val %04x => %04x: %d", orig_out, mask, value, out, rc);

	return rc;
}

static int port_set_masked(const struct device *dev,
			   gpio_port_pins_t mask,
			   gpio_port_value_t value)
{
	return port_write(dev, mask, value, 0);
}

static int port_set_bits(const struct device *dev,
			 gpio_port_pins_t pins)
{
	return port_write(dev, pins, pins, 0);
}

static int port_clear_bits(const struct device *dev,
			   gpio_port_pins_t pins)
{
	return port_write(dev, pins, 0, 0);
}

static int port_toggle_bits(const struct device *dev,
			    gpio_port_pins_t pins)
{
	return port_write(dev, 0, 0, pins);
}

#ifdef CONFIG_GPIO_SX1509B_INTERRUPT
static int pin_interrupt_configure(const struct device *dev,
				   gpio_pin_t pin,
				   enum gpio_int_mode mode,
				   enum gpio_int_trig trig)
{
	int rc = 0;

	/* Device does not support level-triggered interrupts. */
	if (mode == GPIO_INT_MODE_LEVEL) {
		return -ENOTSUP;
	}

	const struct sx1509b_config *cfg = dev->config;
	struct sx1509b_drv_data *drv_data = dev->data;
	struct sx1509b_irq_state *irq = &drv_data->irq_state;
	struct {
		uint8_t reg;
		struct sx1509b_irq_state irq;
	} __packed irq_buf;

	/* Only level triggered interrupts are supported, and those
	 * only if interrupt support is enabled.
	 */
	if (IS_ENABLED(CONFIG_GPIO_SX1509B_INTERRUPT)) {
		if (mode == GPIO_INT_MODE_LEVEL) {
			return -ENOTSUP;
		}
	} else if (mode != GPIO_INT_MODE_DISABLED) {
		return -ENOTSUP;
	}

	k_sem_take(&drv_data->lock, K_FOREVER);

	irq->interrupt_sense &= ~(SX1509B_EDGE_BOTH << (pin * 2));
	if (mode == GPIO_INT_MODE_DISABLED) {
		irq->interrupt_mask |= BIT(pin);
	} else { /* GPIO_INT_MODE_EDGE */
		irq->interrupt_mask &= ~BIT(pin);
		if (trig == GPIO_INT_TRIG_BOTH) {
			irq->interrupt_sense |= (SX1509B_EDGE_BOTH <<
								(pin * 2));
		} else if (trig == GPIO_INT_TRIG_LOW) {
			irq->interrupt_sense |= (SX1509B_EDGE_FALLING <<
								(pin * 2));
		} else if (trig == GPIO_INT_TRIG_HIGH) {
			irq->interrupt_sense |= (SX1509B_EDGE_RISING <<
								(pin * 2));
		}
	}

	irq_buf.reg = SX1509B_REG_INTERRUPT_MASK;
	irq_buf.irq.interrupt_mask = sys_cpu_to_be16(irq->interrupt_mask);
	irq_buf.irq.interrupt_sense = sys_cpu_to_be32(irq->interrupt_sense);

	rc = i2c_write_dt(&cfg->bus, &irq_buf.reg, sizeof(irq_buf));

	k_sem_give(&drv_data->lock);

	return rc;
}
#endif /* CONFIG_GPIO_SX1509B_INTERRUPT */

/**
 * @brief Initialization function of SX1509B
 *
 * @param dev Device struct
 * @return 0 if successful, failed otherwise.
 */
static int sx1509b_init(const struct device *dev)
{
	const struct sx1509b_config *cfg = dev->config;
	struct sx1509b_drv_data *drv_data = dev->data;
	int rc;

	if (!device_is_ready(cfg->bus.bus)) {
		LOG_ERR("I2C bus not ready");
		rc = -ENODEV;
		goto out;
	}

#ifdef CONFIG_GPIO_SX1509B_INTERRUPT
	drv_data->dev = dev;

	if (!gpio_is_ready_dt(&cfg->nint_gpio)) {
		rc = -ENODEV;
		goto out;
	}
	k_work_init(&drv_data->work, sx1509b_work_handler);

	gpio_pin_configure_dt(&cfg->nint_gpio, GPIO_INPUT);
	gpio_pin_interrupt_configure_dt(&cfg->nint_gpio,
					GPIO_INT_EDGE_TO_ACTIVE);

	gpio_init_callback(&drv_data->gpio_cb, sx1509_int_cb,
			   BIT(cfg->nint_gpio.pin));
	gpio_add_callback(cfg->nint_gpio.port, &drv_data->gpio_cb);

	drv_data->irq_state = (struct sx1509b_irq_state) {
		.interrupt_mask = ALL_PINS,
	};
#endif

	rc = i2c_reg_write_byte_dt(&cfg->bus, SX1509B_REG_RESET,
				   SX1509B_REG_RESET_MAGIC0);
	if (rc != 0) {
		LOG_ERR("%s: reset m0 failed: %d\n", dev->name, rc);
		goto out;
	}
	rc = i2c_reg_write_byte_dt(&cfg->bus, SX1509B_REG_RESET,
				   SX1509B_REG_RESET_MAGIC1);
	if (rc != 0) {
		goto out;
	}

	k_sleep(K_MSEC(RESET_DELAY_MS));

	/* Reset state mediated by initial configuration */
	drv_data->pin_state = (struct sx1509b_pin_state) {
		.dir = (ALL_PINS
			& ~(DT_INST_PROP(0, init_out_low)
			    | DT_INST_PROP(0, init_out_high))),
		.data = (ALL_PINS
			 & ~DT_INST_PROP(0, init_out_low)),
	};
	drv_data->debounce_state = (struct sx1509b_debounce_state) {
		.debounce_config = CONFIG_GPIO_SX1509B_DEBOUNCE_TIME,
	};

	rc = i2c_reg_write_byte_dt(&cfg->bus, SX1509B_REG_CLOCK,
				   SX1509B_REG_CLOCK_FOSC_INT_2MHZ);
	if (rc == 0) {
		rc = i2c_reg_write_word_be(&cfg->bus, SX1509B_REG_DATA,
					   drv_data->pin_state.data);
	}
	if (rc == 0) {
		rc = i2c_reg_write_word_be(&cfg->bus, SX1509B_REG_DIR,
					   drv_data->pin_state.dir);
	}
	if (rc == 0) {
		rc = i2c_reg_write_byte_be(&cfg->bus, SX1509B_REG_MISC,
					   SX1509B_REG_MISC_LOG_A |
					   SX1509B_REG_MISC_LOG_B |
					   SX1509B_REG_MISC_FREQ);
	}

out:
	if (rc != 0) {
		LOG_ERR("%s init failed: %d", dev->name, rc);
	} else {
		LOG_INF("%s init ok", dev->name);
	}
	k_sem_give(&drv_data->lock);
	return rc;
}

#ifdef CONFIG_GPIO_SX1509B_INTERRUPT
static int gpio_sx1509b_manage_callback(const struct device *dev,
					  struct gpio_callback *callback,
					  bool set)
{
	struct sx1509b_drv_data *data = dev->data;

	return gpio_manage_callback(&data->cb, callback, set);
}
#endif

static const struct gpio_driver_api api_table = {
	.pin_configure = sx1509b_config,
	.port_get_raw = port_get,
	.port_set_masked_raw = port_set_masked,
	.port_set_bits_raw = port_set_bits,
	.port_clear_bits_raw = port_clear_bits,
	.port_toggle_bits = port_toggle_bits,
#ifdef CONFIG_GPIO_SX1509B_INTERRUPT
	.pin_interrupt_configure = pin_interrupt_configure,
	.manage_callback = gpio_sx1509b_manage_callback,
#endif
};

int sx1509b_led_intensity_pin_configure(const struct device *dev,
					gpio_pin_t pin)
{
	const struct sx1509b_config *cfg = dev->config;
	struct sx1509b_drv_data *drv_data = dev->data;
	struct sx1509b_pin_state *pins = &drv_data->pin_state;
	int rc;

	/* Can't do I2C bus operations from an ISR */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	if (pin >= ARRAY_SIZE(intensity_registers)) {
		return -ERANGE;
	}

	k_sem_take(&drv_data->lock, K_FOREVER);

	/* Enable LED driver */
	drv_data->led_drv_enable |= BIT(pin);
	rc = i2c_reg_write_word_be(&cfg->bus, SX1509B_REG_LED_DRV_ENABLE,
				   drv_data->led_drv_enable);

	/* Set intensity to 0 */
	if (rc == 0) {
		rc = i2c_reg_write_byte_be(&cfg->bus, intensity_registers[pin], 0);
	} else {
		goto out;
	}

	pins->input_disable |= BIT(pin);
	pins->pull_up &= ~BIT(pin);
	pins->dir &= ~BIT(pin);
	pins->data &= ~BIT(pin);

	if (rc == 0) {
		rc = write_pin_state(cfg, drv_data, pins, false);
	}

out:
	k_sem_give(&drv_data->lock);
	return rc;
}

int sx1509b_led_intensity_pin_set(const struct device *dev, gpio_pin_t pin,
				  uint8_t intensity_val)
{
	const struct sx1509b_config *cfg = dev->config;
	struct sx1509b_drv_data *drv_data = dev->data;
	int rc;

	/* Can't do I2C bus operations from an ISR */
	if (k_is_in_isr()) {
		return -EWOULDBLOCK;
	}

	if (pin >= ARRAY_SIZE(intensity_registers)) {
		return -ERANGE;
	}

	k_sem_take(&drv_data->lock, K_FOREVER);

	rc = i2c_reg_write_byte_be(&cfg->bus, intensity_registers[pin],
				   intensity_val);

	k_sem_give(&drv_data->lock);

	return rc;
}

#define GPIO_SX1509B_DEFINE(inst)                                              \
	static const struct sx1509b_config sx1509b_cfg##inst = {               \
		.common = {                                                    \
			.port_pin_mask = GPIO_PORT_PIN_MASK_FROM_DT_INST(inst),\
		},                                                             \
		.bus = I2C_DT_SPEC_INST_GET(inst),                             \
		IF_ENABLED(CONFIG_GPIO_SX1509B_INTERRUPT,                      \
			   (GPIO_DT_SPEC_INST_GET(inst, nint_gpios)))          \
	};                                                                     \
                                                                               \
	static struct sx1509b_drv_data sx1509b_drvdata##inst = {               \
		.lock = Z_SEM_INITIALIZER(sx1509b_drvdata##inst.lock, 1, 1),   \
	};                                                                     \
                                                                               \
	DEVICE_DT_INST_DEFINE(inst, sx1509b_init, NULL, &sx1509b_drvdata##inst,\
			      &sx1509b_cfg##inst, POST_KERNEL,                 \
			      CONFIG_GPIO_SX1509B_INIT_PRIORITY, &api_table);

DT_INST_FOREACH_STATUS_OKAY(GPIO_SX1509B_DEFINE)
