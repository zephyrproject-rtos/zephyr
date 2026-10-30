/*
 * Copyright (c) 2026 MediaTek Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT mediatek_mt8188_gpio

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <zephyr/drivers/interrupt_controller/intc_mtk_eint.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/sys/util.h>

/*
 * The GPIO registers are a single block owned by the pin controller, so a bank
 * reaches them through its parent node rather than through a register range of
 * its own.  Each group holds one word per bank, and writing a bit to the SET or
 * CLR alias of a group changes that bit alone - no read-modify-write, and so no
 * lock between the port-level calls below.
 */
#define GPIO_DIR  0x000
#define GPIO_DOUT 0x100
#define GPIO_DIN  0x200

#define GPIO_BANK_STRIDE 0x10
#define GPIO_GROUP_SET   0x4
#define GPIO_GROUP_CLR   0x8

/* Pins in one bank, which is also the width of one register. */
#define GPIO_PINS_PER_BANK 32U

struct gpio_mt8188_config {
	struct gpio_driver_config common;
	DEVICE_MMIO_NAMED_ROM(reg_base);
	const struct device *eint;
	const struct pinctrl_dev_config *pcfg;
	uint8_t bank;
	uint8_t ngpios;
};

struct gpio_mt8188_data {
	struct gpio_driver_data common;
	DEVICE_MMIO_NAMED_RAM(reg_base);
	struct eint_mtk_callback eint_callback;
	sys_slist_t callbacks;
	/* Pins asked for both edges, which the controller cannot detect alone. */
	uint32_t dual_edge;
};

/* The DEVICE_MMIO_NAMED_* macros reach the register range through these. */
#define DEV_CFG(_dev)  ((const struct gpio_mt8188_config *)(_dev)->config)
#define DEV_DATA(_dev) ((struct gpio_mt8188_data *)(_dev)->data)

static mm_reg_t gpio_mt8188_reg(const struct device *dev, uint32_t group)
{
	const struct gpio_mt8188_config *config = dev->config;

	return DEVICE_MMIO_NAMED_GET(dev, reg_base) + group +
	       ((mm_reg_t)config->bank * GPIO_BANK_STRIDE);
}

static uint8_t gpio_mt8188_line(const struct device *dev, gpio_pin_t pin)
{
	const struct gpio_mt8188_config *config = dev->config;

	return (uint8_t)(((uint32_t)config->bank * GPIO_PINS_PER_BANK) + pin);
}

static bool gpio_mt8188_pin_ok(const struct device *dev, gpio_pin_t pin)
{
	const struct gpio_mt8188_config *config = dev->config;

	/*
	 * Checked here rather than left to the caller: a pin outside the mask is
	 * either past the end of the bank or reserved by the board, and on a
	 * board that hands the rest of the block to another owner, touching one
	 * is not a harmless mistake.
	 */
	if (pin >= GPIO_PINS_PER_BANK) {
		return false;
	}

	return (config->common.port_pin_mask & BIT(pin)) != 0U;
}

/*
 * Both edges, on a controller that detects one: select the edge away from the
 * level the pin sits at, so whichever way it moves next produces an event, and
 * select again after every event.  The level is re-read afterwards because it
 * can move while the choice is being made; the loop settles as soon as the
 * reading either side of the write agrees.
 *
 * Only the polarity changes, and nothing already latched is cleared, so an
 * edge during the loop still fires.  One that lands between a read and the
 * write that follows it is not lost either: the new polarity then matches the
 * level the pin has just reached, which the controller latches as the edge it
 * is.
 */
static int gpio_mt8188_arm_dual_edge(const struct device *dev, gpio_pin_t pin)
{
	const struct gpio_mt8188_config *config = dev->config;
	uint8_t line = gpio_mt8188_line(dev, pin);
	uint32_t level;
	int ret;

	do {
		level = sys_read32(gpio_mt8188_reg(dev, GPIO_DIN)) & BIT(pin);

		ret = eint_mtk_set_polarity(config->eint, line, level == 0U);
		if (ret != 0) {
			return ret;
		}
	} while (level != (sys_read32(gpio_mt8188_reg(dev, GPIO_DIN)) & BIT(pin)));

	return 0;
}

static int gpio_mt8188_pin_configure(const struct device *dev, gpio_pin_t pin, gpio_flags_t flags)
{
	uint32_t mask;

	/* Before BIT(pin) below, which is undefined once pin reaches the word. */
	if (!gpio_mt8188_pin_ok(dev, pin)) {
		return -EINVAL;
	}

	mask = BIT(pin);

	/* Drive strength and open-drain live in the pin controller, not here. */
	if ((flags & GPIO_SINGLE_ENDED) != 0U) {
		return -ENOTSUP;
	}

	/*
	 * So do the bias resistors on this SoC: the GPIO block has no pull
	 * registers, and the pin controller applies whatever the board's pinctrl
	 * state asked for.
	 */
	if ((flags & (GPIO_PULL_UP | GPIO_PULL_DOWN)) != 0U) {
		return -ENOTSUP;
	}

	if ((flags & GPIO_OUTPUT) != 0U) {
		/* Settle the level first so enabling the driver cannot glitch. */
		if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0U) {
			sys_write32(mask, gpio_mt8188_reg(dev, GPIO_DOUT) + GPIO_GROUP_SET);
		} else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0U) {
			sys_write32(mask, gpio_mt8188_reg(dev, GPIO_DOUT) + GPIO_GROUP_CLR);
		}

		sys_write32(mask, gpio_mt8188_reg(dev, GPIO_DIR) + GPIO_GROUP_SET);
	} else {
		/*
		 * GPIO_INPUT and GPIO_DISCONNECTED both land here.  The block
		 * cannot detach a pad, so the closest it offers for the latter
		 * is to stop driving it.
		 */
		sys_write32(mask, gpio_mt8188_reg(dev, GPIO_DIR) + GPIO_GROUP_CLR);
	}

	return 0;
}

static int gpio_mt8188_port_get_raw(const struct device *dev, gpio_port_value_t *value)
{
	const struct gpio_mt8188_config *config = dev->config;

	*value = sys_read32(gpio_mt8188_reg(dev, GPIO_DIN)) & config->common.port_pin_mask;

	return 0;
}

static int gpio_mt8188_port_set_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_mt8188_config *config = dev->config;

	pins &= config->common.port_pin_mask;
	if (pins != 0U) {
		sys_write32((uint32_t)pins, gpio_mt8188_reg(dev, GPIO_DOUT) + GPIO_GROUP_SET);
	}

	return 0;
}

static int gpio_mt8188_port_clear_bits_raw(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_mt8188_config *config = dev->config;

	pins &= config->common.port_pin_mask;
	if (pins != 0U) {
		sys_write32((uint32_t)pins, gpio_mt8188_reg(dev, GPIO_DOUT) + GPIO_GROUP_CLR);
	}

	return 0;
}

static int gpio_mt8188_port_set_masked_raw(const struct device *dev, gpio_port_pins_t mask,
					   gpio_port_value_t value)
{
	int ret;

	ret = gpio_mt8188_port_set_bits_raw(dev, value & mask);
	if (ret != 0) {
		return ret;
	}

	return gpio_mt8188_port_clear_bits_raw(dev, ~value & mask);
}

static int gpio_mt8188_port_toggle_bits(const struct device *dev, gpio_port_pins_t pins)
{
	const struct gpio_mt8188_config *config = dev->config;
	uint32_t out;

	pins &= config->common.port_pin_mask;

	/*
	 * Read what is being driven, not what the pad reads back: an output
	 * whose pad is held by something external would otherwise be seen as
	 * already at the level it is being toggled towards, and never move.
	 */
	out = sys_read32(gpio_mt8188_reg(dev, GPIO_DOUT));

	sys_write32((uint32_t)pins & ~out, gpio_mt8188_reg(dev, GPIO_DOUT) + GPIO_GROUP_SET);
	sys_write32((uint32_t)pins & out, gpio_mt8188_reg(dev, GPIO_DOUT) + GPIO_GROUP_CLR);

	return 0;
}

static int gpio_mt8188_pin_interrupt_configure(const struct device *dev, gpio_pin_t pin,
					       enum gpio_int_mode mode, enum gpio_int_trig trig)
{
	const struct gpio_mt8188_config *config = dev->config;
	struct gpio_mt8188_data *data = dev->data;
	uint8_t line = gpio_mt8188_line(dev, pin);
	enum eint_mtk_trigger eint_trig;
	int ret;

	if (!gpio_mt8188_pin_ok(dev, pin)) {
		return -EINVAL;
	}

	if (mode == GPIO_INT_MODE_DISABLED) {
		data->dual_edge &= ~BIT(pin);

		return eint_mtk_disable(config->eint, line);
	}

	if ((mode != GPIO_INT_MODE_EDGE) && (mode != GPIO_INT_MODE_LEVEL)) {
		return -ENOTSUP;
	}

	/*
	 * A level is acknowledged before the callbacks run and latches again
	 * straight away if it still holds, so it keeps firing until a callback
	 * disables the interrupt or removes the condition - what the GPIO API
	 * asks of a consumer of level interrupts.
	 */
	switch (trig) {
	case GPIO_INT_TRIG_LOW:
		eint_trig = (mode == GPIO_INT_MODE_EDGE) ? EINT_MTK_TRIG_EDGE_FALLING
							 : EINT_MTK_TRIG_LEVEL_LOW;
		break;

	case GPIO_INT_TRIG_HIGH:
		eint_trig = (mode == GPIO_INT_MODE_EDGE) ? EINT_MTK_TRIG_EDGE_RISING
							 : EINT_MTK_TRIG_LEVEL_HIGH;
		break;

	case GPIO_INT_TRIG_BOTH:
		/* Both edges are emulated below; both levels at once mean nothing. */
		if (mode != GPIO_INT_MODE_EDGE) {
			return -ENOTSUP;
		}

		eint_trig = EINT_MTK_TRIG_EDGE_RISING;
		break;

	default:
		return -ENOTSUP;
	}

	/* Quiet while the condition changes, so no stale event survives it. */
	ret = eint_mtk_disable(config->eint, line);
	if (ret != 0) {
		return ret;
	}

	ret = eint_mtk_set_trigger(config->eint, line, eint_trig);
	if (ret != 0) {
		return ret;
	}

	if (trig == GPIO_INT_TRIG_BOTH) {
		data->dual_edge |= BIT(pin);

		ret = gpio_mt8188_arm_dual_edge(dev, pin);
		if (ret != 0) {
			return ret;
		}
	} else {
		data->dual_edge &= ~BIT(pin);
	}

	return eint_mtk_enable(config->eint, line);
}

static int gpio_mt8188_manage_callback(const struct device *dev, struct gpio_callback *callback,
				       bool set)
{
	struct gpio_mt8188_data *data = dev->data;

	return gpio_manage_callback(&data->callbacks, callback, set);
}

static void gpio_mt8188_eint_handler(const struct device *dev, uint8_t line, void *arg)
{
	const struct gpio_mt8188_config *config = dev->config;
	struct gpio_mt8188_data *data = dev->data;
	uint32_t first = (uint32_t)config->bank * GPIO_PINS_PER_BANK;
	uint32_t pin;

	ARG_UNUSED(arg);

	if ((line < first) || ((line - first) >= GPIO_PINS_PER_BANK)) {
		return;
	}

	pin = line - first;

	/*
	 * Re-arm before the callbacks run: the opposite edge can arrive while
	 * they are running, and it is only detected if the condition has already
	 * been turned around.
	 */
	if ((data->dual_edge & BIT(pin)) != 0U) {
		(void)gpio_mt8188_arm_dual_edge(dev, (gpio_pin_t)pin);
	}

	gpio_fire_callbacks(&data->callbacks, dev, BIT(pin));
}

static int gpio_mt8188_init(const struct device *dev)
{
	const struct gpio_mt8188_config *config = dev->config;
	struct gpio_mt8188_data *data = dev->data;
	int ret;

	/*
	 * Registering below writes into the controller's list, so it has to have
	 * run first.  GPIO_MT8188_INIT_PRIORITY orders the two; this catches the
	 * case where it has been set to something that does not.
	 */
	if (!device_is_ready(config->eint)) {
		return -ENODEV;
	}

	DEVICE_MMIO_NAMED_MAP(dev, reg_base, K_MEM_CACHE_NONE);

	/*
	 * A pin only reaches these registers while the pin controller has it in
	 * its GPIO function, and on this SoC most pins have an alternate one
	 * selected instead.  A board that uses a bank says which pins it wants
	 * as GPIO in a default pinctrl state; a bank without one is left as the
	 * pin controller already had it.
	 */
	ret = pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);
	if ((ret != 0) && (ret != -ENOENT)) {
		return ret;
	}

	sys_slist_init(&data->callbacks);

	ret = eint_mtk_init_callback(&data->eint_callback,
				     (uint8_t)((uint32_t)config->bank * GPIO_PINS_PER_BANK),
				     config->ngpios, gpio_mt8188_eint_handler, dev, NULL);
	if (ret != 0) {
		return ret;
	}

	return eint_mtk_add_callback(config->eint, &data->eint_callback);
}

static DEVICE_API(gpio, gpio_mt8188_driver_api) = {
	.pin_configure = gpio_mt8188_pin_configure,
	.port_get_raw = gpio_mt8188_port_get_raw,
	.port_set_masked_raw = gpio_mt8188_port_set_masked_raw,
	.port_set_bits_raw = gpio_mt8188_port_set_bits_raw,
	.port_clear_bits_raw = gpio_mt8188_port_clear_bits_raw,
	.port_toggle_bits = gpio_mt8188_port_toggle_bits,
	.pin_interrupt_configure = gpio_mt8188_pin_interrupt_configure,
	.manage_callback = gpio_mt8188_manage_callback,
};

#define GPIO_MT8188_INIT(n)                                                                        \
	PINCTRL_DT_INST_DEFINE(n);                                                                 \
                                                                                                   \
	static struct gpio_mt8188_data gpio_mt8188_data_##n;                                       \
                                                                                                   \
	static const struct gpio_mt8188_config gpio_mt8188_config_##n = {                          \
		.common = GPIO_COMMON_CONFIG_FROM_DT_INST(n),                                      \
		DEVICE_MMIO_NAMED_ROM_INIT(reg_base, DT_INST_PARENT(n)),                           \
		.eint = DEVICE_DT_GET(DT_INST_PHANDLE(n, mediatek_eint)),                          \
		.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(n),                                         \
		.bank = DT_INST_PROP(n, mediatek_gpio_bank),                                       \
		.ngpios = DT_INST_PROP(n, ngpios),                                                 \
	};                                                                                         \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, gpio_mt8188_init, NULL, &gpio_mt8188_data_##n,                    \
			      &gpio_mt8188_config_##n, PRE_KERNEL_1,                               \
			      CONFIG_GPIO_MT8188_INIT_PRIORITY, &gpio_mt8188_driver_api);

DT_INST_FOREACH_STATUS_OKAY(GPIO_MT8188_INIT)
