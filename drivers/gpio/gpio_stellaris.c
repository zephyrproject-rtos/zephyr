/*
 * Copyright (c) 2018 Zilogic Systems.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <device.h>
#include <drivers/gpio.h>
#include <soc.h>
#include <sys/sys_io.h>
#include "gpio_utils.h"

typedef void (*config_func_t)(struct device *dev);

struct gpio_stellaris_config {
	u32_t base;
	u32_t port_map;
	config_func_t config_func;
};

struct gpio_stellaris_runtime {
	sys_slist_t cb;
};

#define DEV_CFG(dev)                                     \
	((const struct gpio_stellaris_config *const)     \
	(dev)->config->config_info)

#define DEV_DATA(dev)					 \
	((struct gpio_stellaris_runtime *const)          \
	(dev)->driver_data)

#define GPIO_REG_ADDR(base, offset) (base + offset)

#define GPIO_RW_ADDR(base, offset, p)			 \
	(GPIO_REG_ADDR(base, offset) | (1 << (p + 2)))

enum gpio_regs {
	GPIO_DATA_OFFSET = 0x000,
	GPIO_DIR_OFFSET = 0x400,
	GPIO_DEN_OFFSET = 0x51C,
	GPIO_IS_OFFSET = 0x404,
	GPIO_IBE_OFFSET = 0x408,
	GPIO_IEV_OFFSET = 0x40C,
	GPIO_IM_OFFSET = 0x410,
	GPIO_MIS_OFFSET = 0x418,
	GPIO_ICR_OFFSET = 0x41C,
};

enum gpio_port_map {
	GPIO_PORT_A_MAP = 0xFF,
	GPIO_PORT_B_MAP = 0xFF,
	GPIO_PORT_C_MAP = 0xFF,
	GPIO_PORT_D_MAP = 0xFF,
	GPIO_PORT_E_MAP = 0x0F,
	GPIO_PORT_F_MAP = 0x0F,
	GPIO_PORT_G_MAP = 0x03,
};

static void gpio_stellaris_isr(void *arg)
{
	struct device *dev = (struct device *)arg;
	const struct gpio_stellaris_config * const cfg = DEV_CFG(dev);
	struct gpio_stellaris_runtime *context = DEV_DATA(dev);
	u32_t base = cfg->base;
	u32_t int_stat = sys_read32(GPIO_REG_ADDR(base, GPIO_MIS_OFFSET));

	gpio_fire_callbacks(&context->cb, dev, int_stat);

	sys_write32(int_stat, GPIO_REG_ADDR(base, GPIO_ICR_OFFSET));
}

static int gpio_stellaris_configure(struct device *dev, int access_op,
				    u32_t pin, int flags)
{
	const struct gpio_stellaris_config *cfg = DEV_CFG(dev);
	u32_t base = cfg->base;
	u32_t port_map = cfg->port_map;

	/* Check for an invalid pin configuration */
	if ((flags & GPIO_INT) && (flags & GPIO_DIR_OUT)) {
		return -EINVAL;
	}

	/* Supports access by pin now,you can add access by port when needed */
	if (access_op != GPIO_ACCESS_BY_PIN) {
		return -EINVAL;
	}

	/* Check for pin availability */
	if (!sys_test_bit((u32_t)&port_map, pin)) {
		return -EINVAL;
	}

	/* Pin digital enable */
	sys_set_bit(GPIO_REG_ADDR(base, GPIO_DEN_OFFSET), pin);

	/* Input-0,output-1 */
	if ((flags & GPIO_DIR_MASK) == GPIO_DIR_IN) {
		sys_clear_bit(GPIO_REG_ADDR(base, GPIO_DIR_OFFSET), pin);
	} else {
		sys_set_bit(GPIO_REG_ADDR(base, GPIO_DIR_OFFSET), pin);
	}

	/* Check if GPIO port needs interrupt support */
	if (flags & GPIO_INT) {

		/* GPIO interrupt edge trigger enable or level trigger enable */
		if (flags & GPIO_INT_EDGE) {
			sys_clear_bit(GPIO_REG_ADDR(base, GPIO_IS_OFFSET), pin);
		} else {
			sys_set_bit(GPIO_REG_ADDR(base, GPIO_IS_OFFSET), pin);
		}

		/* GPIO interrupt both edge sense */
		if (flags & GPIO_INT_DOUBLE_EDGE) {
			sys_set_bit(GPIO_REG_ADDR(base, GPIO_IBE_OFFSET), pin);
		}

		/*
		 * GPIO interrupt rising edge & high level enable or
		 * falling edge & low level enable
		 */
		if (flags & GPIO_INT_ACTIVE_HIGH) {
			sys_set_bit(GPIO_REG_ADDR(base, GPIO_IEV_OFFSET), pin);
		} else {
			sys_clear_bit(GPIO_REG_ADDR(base,
						    GPIO_IEV_OFFSET), pin);
		}
	}
	return 0;
}

static int gpio_stellaris_write(struct device *dev,
				int access_op, u32_t pin, u32_t value)
{
	const struct gpio_stellaris_config *cfg = DEV_CFG(dev);
	u32_t base = cfg->base;

	if (access_op != GPIO_ACCESS_BY_PIN) {
		return -EINVAL;
	}

	/* Set/Clear the data output for the respective pin */
	if (value) {
		sys_set_bit(GPIO_RW_ADDR(base, GPIO_DATA_OFFSET, pin),
			    pin);
	} else {
		sys_clear_bit(GPIO_RW_ADDR(base, GPIO_DATA_OFFSET, pin),
			      pin);
	}

	return 0;
}

static int gpio_stellaris_read(struct device *dev,
			       int access_op, u32_t pin, u32_t *value)
{
	const struct gpio_stellaris_config *cfg = DEV_CFG(dev);
	u32_t base = cfg->base;

	if (access_op != GPIO_ACCESS_BY_PIN) {
		return -EINVAL;
	}

	*value = sys_test_bit(GPIO_RW_ADDR(base, GPIO_DATA_OFFSET, pin),
			      pin);

	return 0;
}

static int gpio_stellaris_init(struct device *dev)
{
	const struct gpio_stellaris_config *cfg = DEV_CFG(dev);

	cfg->config_func(dev);
	return 0;
}

static int gpio_stellaris_enable_callback(struct device *dev,
					  int access_op, u32_t pin)
{
	const struct gpio_stellaris_config * const cfg = DEV_CFG(dev);
	u32_t base = cfg->base;

	if (access_op != GPIO_ACCESS_BY_PIN) {
		return -EINVAL;
	}

	sys_set_bit(GPIO_REG_ADDR(base, GPIO_IM_OFFSET), pin);

	return 0;
}

static int gpio_stellaris_disable_callback(struct device *dev,
					   int access_op, u32_t pin)
{
	const struct gpio_stellaris_config * const cfg = DEV_CFG(dev);
	u32_t base = cfg->base;

	if (access_op != GPIO_ACCESS_BY_PIN) {
		return -EINVAL;
	}

	sys_clear_bit(GPIO_REG_ADDR(base, GPIO_IM_OFFSET), pin);

	return 0;
}

static int gpio_stellaris_manage_callback(struct device *dev,
					  struct gpio_callback *callback,
					  bool set)
{
	struct gpio_stellaris_runtime *context = DEV_DATA(dev);

	gpio_manage_callback(&context->cb, callback, set);

	return 0;
}

static const struct gpio_driver_api gpio_stellaris_driver_api = {
	.config = gpio_stellaris_configure,
	.write = gpio_stellaris_write,
	.read = gpio_stellaris_read,
	.manage_callback = gpio_stellaris_manage_callback,
	.enable_callback = gpio_stellaris_enable_callback,
	.disable_callback = gpio_stellaris_disable_callback,
};

#ifdef CONFIG_GPIO_A_STELLARIS

static void port_a_stellaris_config_func(struct device *dev);

static struct gpio_stellaris_runtime port_a_stellaris_runtime;

static const struct gpio_stellaris_config gpio_stellaris_port_a_config = {
	.base = DT_GPIO_A_BASE_ADDRESS,
	.port_map = GPIO_PORT_A_MAP,
	.config_func = port_a_stellaris_config_func,
};

DEVICE_AND_API_INIT(gpio_stellaris_port_a, DT_GPIO_A_LABEL,
		    gpio_stellaris_init,
		    &port_a_stellaris_runtime, &gpio_stellaris_port_a_config,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &gpio_stellaris_driver_api);

static void port_a_stellaris_config_func(struct device *dev)
{
	IRQ_CONNECT(DT_GPIO_A_IRQ,
		    DT_GPIO_A_IRQ_PRIO,
		    gpio_stellaris_isr,
		    DEVICE_GET(gpio_stellaris_port_a),
		    0);

	irq_enable(DT_GPIO_A_IRQ);
}
#endif /* CONFIG_GPIO_A_STELLARIS */


#ifdef CONFIG_GPIO_B_STELLARIS

static void port_b_stellaris_config_func(struct device *dev);

static struct gpio_stellaris_runtime port_b_stellaris_runtime;

static const struct gpio_stellaris_config gpio_stellaris_port_b_config = {
	.base = DT_GPIO_B_BASE_ADDRESS,
	.port_map = GPIO_PORT_B_MAP,
	.config_func = port_b_stellaris_config_func,
};

DEVICE_AND_API_INIT(gpio_stellaris_port_b, DT_GPIO_B_LABEL,
		    gpio_stellaris_init,
		    &port_b_stellaris_runtime, &gpio_stellaris_port_b_config,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &gpio_stellaris_driver_api);

static void port_b_stellaris_config_func(struct device *dev)
{
	IRQ_CONNECT(DT_GPIO_B_IRQ,
		    DT_GPIO_B_IRQ_PRIO,
		    gpio_stellaris_isr,
		    DEVICE_GET(gpio_stellaris_port_b),
		    0);

	irq_enable(DT_GPIO_B_IRQ);
}
#endif /* CONFIG_GPIO_B_STELLARIS */


#ifdef CONFIG_GPIO_C_STELLARIS

static void port_c_stellaris_config_func(struct device *dev);

static struct gpio_stellaris_runtime port_c_stellaris_runtime;

static const struct gpio_stellaris_config gpio_stellaris_port_c_config = {
	.base = DT_GPIO_C_BASE_ADDRESS,
	.port_map = GPIO_PORT_C_MAP,
	.config_func = port_c_stellaris_config_func,
};

DEVICE_AND_API_INIT(gpio_stellaris_port_c, DT_GPIO_C_LABEL,
		    gpio_stellaris_init,
		    &port_c_stellaris_runtime, &gpio_stellaris_port_c_config,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &gpio_stellaris_driver_api);

static void port_c_stellaris_config_func(struct device *dev)
{
	IRQ_CONNECT(DT_GPIO_C_IRQ,
		    DT_GPIO_C_IRQ_PRIO,
		    gpio_stellaris_isr,
		    DEVICE_GET(gpio_stellaris_port_c),
		    0);

	irq_enable(DT_GPIO_C_IRQ);
}
#endif /* CONFIG_GPIO_C_STELLARIS */


#ifdef CONFIG_GPIO_D_STELLARIS

static void port_d_stellaris_config_func(struct device *dev);

static struct gpio_stellaris_runtime port_d_stellaris_runtime;

static const struct gpio_stellaris_config gpio_stellaris_port_d_config = {
	.base = DT_GPIO_D_BASE_ADDRESS,
	.port_map = GPIO_PORT_D_MAP,
	.config_func = port_d_stellaris_config_func,
};

DEVICE_AND_API_INIT(gpio_stellaris_port_d, DT_GPIO_D_LABEL,
		    gpio_stellaris_init,
		    &port_d_stellaris_runtime, &gpio_stellaris_port_d_config,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &gpio_stellaris_driver_api);

static void port_d_stellaris_config_func(struct device *dev)
{
	IRQ_CONNECT(DT_GPIO_D_IRQ,
		    DT_GPIO_D_IRQ_PRIO,
		    gpio_stellaris_isr,
		    DEVICE_GET(gpio_stellaris_port_d), 0);

	irq_enable(DT_GPIO_D_IRQ);
}
#endif /* CONFIG_GPIO_D_STELLARIS */


#ifdef CONFIG_GPIO_E_STELLARIS

static void port_e_stellaris_config_func(struct device *dev);

static const struct gpio_stellaris_config gpio_stellaris_port_e_config = {
	.base = DT_GPIO_E_BASE_ADDRESS,
	.port_map = GPIO_PORT_E_MAP,
	.config_func = port_e_stellaris_config_func,
};

static struct gpio_stellaris_runtime port_e_stellaris_runtime;

DEVICE_AND_API_INIT(gpio_stellaris_port_e, DT_GPIO_E_LABEL,
		    gpio_stellaris_init,
		    &port_e_stellaris_runtime, &gpio_stellaris_port_e_config,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &gpio_stellaris_driver_api);

static void port_e_stellaris_config_func(struct device *dev)
{
	IRQ_CONNECT(DT_GPIO_E_IRQ,
		    DT_GPIO_E_IRQ_PRIO,
		    gpio_stellaris_isr,
		    DEVICE_GET(gpio_stellaris_port_e),
		    0);

	irq_enable(DT_GPIO_E_IRQ);
}
#endif /* CONFIG_GPIO_E_STELLARIS */


#ifdef CONFIG_GPIO_F_STELLARIS

static void port_f_stellaris_config_func(struct device *dev);

static struct gpio_stellaris_runtime port_f_stellaris_runtime;

static const struct gpio_stellaris_config gpio_stellaris_port_f_config = {
	.base = DT_GPIO_F_BASE_ADDRESS,
	.port_map = GPIO_PORT_F_MAP,
	.config_func = port_f_stellaris_config_func,
};

DEVICE_AND_API_INIT(gpio_stellaris_port_f, DT_GPIO_F_LABEL,
		    gpio_stellaris_init,
		    &port_f_stellaris_runtime, &gpio_stellaris_port_f_config,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &gpio_stellaris_driver_api);

static void port_f_stellaris_config_func(struct device *dev)
{
	IRQ_CONNECT(DT_GPIO_F_IRQ,
		    DT_GPIO_F_IRQ_PRIO,
		    gpio_stellaris_isr,
		    DEVICE_GET(gpio_stellaris_port_f),
		    0);
	irq_enable(DT_GPIO_F_IRQ);
}
#endif /* CONFIG_GPIO_F_STELLARIS */


#ifdef CONFIG_GPIO_G_STELLARIS

static void port_g_stellaris_config_func(struct device *dev);

static struct gpio_stellaris_runtime port_g_stellaris_runtime;

static const struct gpio_stellaris_config gpio_stellaris_port_g_config = {
	.base = DT_GPIO_G_BASE_ADDRESS,
	.port_map = GPIO_PORT_G_MAP,
	.config_func = port_g_stellaris_config_func,
};

DEVICE_AND_API_INIT(gpio_stellaris_port_g, DT_GPIO_G_LABEL,
		    gpio_stellaris_init,
		    &port_g_stellaris_runtime, &gpio_stellaris_port_g_config,
		    POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
		    &gpio_stellaris_driver_api);

static void port_g_stellaris_config_func(struct device *dev)
{
	IRQ_CONNECT(DT_GPIO_G_IRQ,
		    DT_GPIO_G_IRQ_PRIO,
		    gpio_stellaris_isr,
		    DEVICE_GET(gpio_stellaris_port_g),
		    0);
	irq_enable(DT_GPIO_G_IRQ);
}
#endif /* CONFIG_GPIO_G_STELLARIS */
