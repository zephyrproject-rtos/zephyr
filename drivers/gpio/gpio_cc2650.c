/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * GPIO driver for the CC2650 SOC from Texas Instruments.
 */

#include <toolchain/gcc.h>
#include <device.h>
#include <gpio.h>
#include <init.h>
#include <soc.h>
#include <sys_io.h>

#include "gpio_utils.h"


struct gpio_cc2650_data {
	u32_t pin_callback_enables;
	sys_slist_t callbacks;
};

/* Pre-declarations */
static int gpio_cc2650_init(struct device *dev);
static int gpio_cc2650_config(struct device *port, int access_op,
			      u32_t pin, int flags);
static int gpio_cc2650_write(struct device *port, int access_op,
			     u32_t pin, u32_t value);

static int gpio_cc2650_read(struct device *port, int access_op,
			    u32_t pin, u32_t *value);
static int gpio_cc2650_manage_callback(struct device *port,
				       struct gpio_callback *callback,
				       bool set);
static int gpio_cc2650_enable_callback(struct device *port,
				       int access_op,
				       u32_t pin);
static int gpio_cc2650_disable_callback(struct device *port,
					int access_op,
					u32_t pin);
static u32_t gpio_cc2650_get_pending_int(struct device *dev);

/* GPIO registers */
static const u32_t doutset31_0 =
	REG_ADDR(TI_CC2650_GPIO_40022000_BASE_ADDRESS,
		 CC2650_GPIO_DOUTSET31_0);
static const u32_t doutclr31_0 =
	REG_ADDR(TI_CC2650_GPIO_40022000_BASE_ADDRESS,
		 CC2650_GPIO_DOUTCLR31_0);
static const u32_t din31_0 =
	REG_ADDR(TI_CC2650_GPIO_40022000_BASE_ADDRESS,
		 CC2650_GPIO_DIN31_0);
static const u32_t doe31_0 =
	REG_ADDR(TI_CC2650_GPIO_40022000_BASE_ADDRESS,
		 CC2650_GPIO_DOE31_0);
static const u32_t evflags31_0 =
	REG_ADDR(TI_CC2650_GPIO_40022000_BASE_ADDRESS,
		 CC2650_GPIO_EVFLAGS31_0);

static struct gpio_cc2650_data gpio_cc2650_data = {
	.pin_callback_enables = 0
};

static const struct gpio_driver_api gpio_cc2650_funcs = {
	.config = gpio_cc2650_config,
	.write = gpio_cc2650_write,
	.read = gpio_cc2650_read,
	.manage_callback = gpio_cc2650_manage_callback,
	.enable_callback = gpio_cc2650_enable_callback,
	.disable_callback = gpio_cc2650_disable_callback,
	.get_pending_int = gpio_cc2650_get_pending_int
};

DEVICE_AND_API_INIT(gpio_cc2650_0, CONFIG_GPIO_CC2650_NAME,
		    gpio_cc2650_init, &gpio_cc2650_data, NULL,
		    PRE_KERNEL_1, CONFIG_GPIO_CC2650_INIT_PRIO,
		    &gpio_cc2650_funcs);
static void disconnect(const int pin, u32_t *gpiodoe31_0,
		       u32_t *iocfg)
{
	*gpiodoe31_0 &= ~BIT(pin);

	*iocfg &= ~(CC2650_IOC_IOCFGX_PULL_CTL_MASK |
		     CC2650_IOC_IOCFGX_IE_MASK);
	*iocfg |= CC2650_IOC_INPUT_DISABLED |
		  CC2650_IOC_NO_PULL;
}

/* Configure a single pin.
 * If any asked option is not implementable, rollback entirely to
 * previous configuration.
 *
 * Note: For pin drive strength, the CC2650 devices only support
 * symmetric sink/source capabilities.
 * Thus, you may ONLY determine the common drive strength with
 * GPIO *low output state* flags. Flags for *high output state*
 * will be ignored.
 */
static int gpio_cc2650_config_pin(int pin, int flags)
{
	const u32_t iocfg = REG_ADDR(TI_CC2650_PINMUX_40081000_BASE_ADDRESS,
				     CC2650_IOC_IOCFG0 + 0x4 * pin);
	u32_t iocfg_config = sys_read32(iocfg);
	u32_t gpio_doe31_0_config = sys_read32(doe31_0);

	/* Reset all configurable fields to 0 */
	iocfg_config &= ~(CC2650_IOC_IOCFGX_IOSTR_MASK |
		 CC2650_IOC_IOCFGX_PULL_CTL_MASK |
		 CC2650_IOC_IOCFGX_EDGE_DET_MASK |
		 CC2650_IOC_IOCFGX_EDGE_IRQ_EN_MASK |
		 CC2650_IOC_IOCFGX_IOMODE_MASK |
		 CC2650_IOC_IOCFGX_IE_MASK |
		 CC2650_IOC_IOCFGX_HYST_EN_MASK);

	if (flags & GPIO_DIR_OUT) {
		gpio_doe31_0_config |= BIT(pin);
		iocfg_config |= CC2650_IOC_INPUT_DISABLED;
	} else {
		gpio_doe31_0_config &= ~BIT(pin);
		iocfg_config |= CC2650_IOC_INPUT_ENABLED;
	}

	if (flags & GPIO_INT) {
		if (!(flags & GPIO_INT_EDGE) &&
		    !(flags & GPIO_INT_DOUBLE_EDGE)) {
			/* Can't do level-based interrupt */
			/* Don't commit changes */
			return -ENOTSUP;
		}

		iocfg_config |= BIT(CC2650_IOC_IOCFGX_EDGE_IRQ_EN_POS);

		if (flags & GPIO_INT_EDGE) {
			if (flags & GPIO_INT_ACTIVE_HIGH) {
				iocfg_config |= CC2650_IOC_POS_EDGE_DET;
			} else {
				iocfg_config |= CC2650_IOC_NEG_EDGE_DET;
			}
		} else if (flags & GPIO_INT_DOUBLE_EDGE) {
			iocfg_config |= CC2650_IOC_NEG_AND_POS_EDGE_DET;
		}

		if (flags & GPIO_INT_CLOCK_SYNC) {
			/* Don't commit changes */
			return -ENOTSUP;
		}

		if (flags & GPIO_INT_DEBOUNCE) {
			iocfg_config |= CC2650_IOC_HYSTERESIS_ENABLED;
		} else {
			iocfg_config |= CC2650_IOC_HYSTERESIS_DISABLED;
		}
	}

	if (flags & GPIO_POL_INV) {
		iocfg_config |= CC2650_IOC_INVERTED_IO;
	} else {
		iocfg_config |= CC2650_IOC_NORMAL_IO;
	}

	if (flags & GPIO_PUD_PULL_UP) {
		iocfg_config |= CC2650_IOC_PULL_UP;
	} else if (flags & GPIO_PUD_PULL_DOWN) {
		iocfg_config |= CC2650_IOC_PULL_DOWN;
	} else {
		iocfg_config |= CC2650_IOC_NO_PULL;
	}

	/* Remember, we only look at GPIO_DS_*_LOW ! */
	if (flags & GPIO_DS_DISCONNECT_LOW) {
		disconnect(pin, &gpio_doe31_0_config, &iocfg_config);
	}
	if (flags & GPIO_DS_ALT_LOW) {
		iocfg_config |= CC2650_IOC_MAX_DRIVE_STRENGTH;
	} else {
		iocfg_config |= CC2650_IOC_MIN_DRIVE_STRENGTH;
	}

	/* Commit changes */
	sys_write32(iocfg_config, iocfg);
	sys_write32(gpio_doe31_0_config, doe31_0);
	return 0;
}

static inline void gpio_cc2650_write_pin(int pin, u32_t value)
{
	value ? sys_write32(BIT(pin), doutset31_0) :
		sys_write32(BIT(pin), doutclr31_0);
}

static inline void gpio_cc2650_read_pin(int pin, u32_t *value)
{
	*value = sys_read32(din31_0) & BIT(pin);
}

static void gpio_cc2650_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	struct gpio_cc2650_data *data = dev->driver_data;

	const u32_t events = sys_read32(evflags31_0);
	const u32_t call_mask = events & data->pin_callback_enables;

	/* Clear GPIO trigger events */
	u32_t evflags = sys_read32(evflags31_0);

	sys_write32(evflags | call_mask, evflags31_0);

	_gpio_fire_callbacks(&data->callbacks, dev, call_mask);
}


static int gpio_cc2650_init(struct device *dev)
{
	ARG_UNUSED(dev);

	/* ISR setup */
	IRQ_CONNECT(TI_CC2650_GPIO_40022000_IRQ_0,
		    TI_CC2650_GPIO_40022000_IRQ_0_PRIORITY,
		    gpio_cc2650_isr, DEVICE_GET(gpio_cc2650_0),
		    0);
	irq_enable(TI_CC2650_GPIO_40022000_IRQ_0);

	return 0;
}

static int gpio_cc2650_config(struct device *port, int access_op,
			      u32_t pin, int flags)
{
	ARG_UNUSED(port);

	if (access_op == GPIO_ACCESS_BY_PIN) {
		return gpio_cc2650_config_pin(pin, flags);
	}

	const u32_t nb_pins = 32;

	for (u8_t i = 0; i < nb_pins; ++i) {
		if (pin & 0x1 &&
		    gpio_cc2650_config_pin(i, flags) == -ENOTSUP) {
			/* The flags being treated the same for
			 * every pin, if we get here then it's
			 * necessarily the first pin on which we act.
			 *
			 * We expect gpio_cc2650_config_pin() to
			 * NOT commit its changes if any problem
			 * arises, thus we do nothing special here
			 * to implement rollback to previous
			 * configuration.
			 */
			return -ENOTSUP;
		}
		pin >>= 1;
	}
	return 0;
}

static int gpio_cc2650_write(struct device *port, int access_op,
			     u32_t pin, u32_t value)
{
	ARG_UNUSED(port);

	if (access_op == GPIO_ACCESS_BY_PIN) {
		gpio_cc2650_write_pin(pin, value);
	} else {
		const u32_t nb_pins = 32;

		for (u32_t i = 0; i < nb_pins; ++i) {
			if (pin & 0x1) {
				gpio_cc2650_write_pin(i, value);
			}
			pin >>= 1;
		}
	}

	return 0;
}

static int gpio_cc2650_read(struct device *port, int access_op,
			    u32_t pin, u32_t *value)
{
	ARG_UNUSED(port);

	if (access_op == GPIO_ACCESS_BY_PIN) {
		gpio_cc2650_read_pin(pin, value);
		*value >>= pin;
	} else  {
		const u32_t nb_pins = 32;

		for (u32_t i = 0; i < nb_pins; ++i) {
			if (pin & 0x1) {
				gpio_cc2650_read_pin(i, value);
			}
			pin >>= 1;
		}
	}
	return 0;
}

static int gpio_cc2650_manage_callback(struct device *port,
				       struct gpio_callback *callback,
				       bool set)
{
	struct gpio_cc2650_data *data = port->driver_data;

	_gpio_manage_callback(&data->callbacks, callback, set);
	return 0;
}

static int gpio_cc2650_enable_callback(struct device *port,
				       int access_op,
				       u32_t pin)
{
	struct gpio_cc2650_data *data = port->driver_data;

	if (access_op == GPIO_ACCESS_BY_PIN) {
		data->pin_callback_enables |= BIT(pin);
	} else {
		data->pin_callback_enables |= pin;
	}
	return 0;
}

static int gpio_cc2650_disable_callback(struct device *port,
					int access_op,
					u32_t pin)
{
	struct gpio_cc2650_data *data = port->driver_data;

	if (access_op == GPIO_ACCESS_BY_PIN) {
		data->pin_callback_enables &= ~BIT(pin);
	} else {
		data->pin_callback_enables &= ~pin;
	}
	return 0;
}

static u32_t gpio_cc2650_get_pending_int(struct device *dev)
{
	ARG_UNUSED(dev);

	return sys_read32(evflags31_0);
}
