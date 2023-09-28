/*
 * Copyright (c) ENE
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#define DT_DRV_COMPAT ene_kb1200_gpio

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio/gpio_utils.h>
#include <soc.h>

/* GPIO module instances */
#define KB1200_GPIO_DEV(inst) DEVICE_DT_INST_GET(inst),
static const struct device *const gpio_devs[] = {
	DT_INST_FOREACH_STATUS_OKAY(KB1200_GPIO_DEV)
};
/* Platform specific GPIO functions */
const struct device *kb1200_get_gpio_dev(int port)
{
	if (port >= ARRAY_SIZE(gpio_devs)) {
		return NULL;
	}

	return gpio_devs[port];
}

#define GPIO_REG_BASE       ((GPIO_T*)DT_REG_ADDR_BY_NAME(DT_NODELABEL(gpio0x1x), gpio1x))
#define GPTD_REG_BASE       ((GPTD_T*)DT_REG_ADDR_BY_NAME(DT_NODELABEL(gpio0x1x), gptd1x))

struct gpio_kb1200_data {
    /* gpio_driver_data needs to be first */
    struct gpio_driver_data common;
    sys_slist_t cb;
};

struct gpio_kb1200_config {
    /* gpio_driver_config needs to be first */
    struct gpio_driver_config common;
    /* base address of GPIO port */
    uint32_t reg;
    /* GPIO port number */
    uint8_t port_num;
};


static void gpio_kb1200_isr(const struct device *dev)
{
    const struct gpio_kb1200_config *config = dev->config;
    struct gpio_kb1200_data *context = dev->data;
    GPTD_T *gptd_regs = GPTD_REG_BASE;
    uint32_t pending_flag = gptd_regs->GPTDPFxx[config->port_num];
    //printk("%s port_num=0x%08x,flags=0x%08x\n", __func__, config->port_num,gptd_regs->GPTDPFxx[config->port_num]);
    gpio_fire_callbacks(&context->cb, dev, pending_flag);
    gptd_regs->GPTDPFxx[config->port_num] |= pending_flag;   //*reg_pending_flag |= pending_flag;
}

static int kb1200_gpio_pin_configure(const struct device *dev,
                  gpio_pin_t pin,
                  gpio_flags_t flags)
{
    const struct gpio_kb1200_config *config = dev->config;
    GPIO_T *gpio_regs = GPIO_REG_BASE;
    //printk("%s reg=0x%08x,port_num=0x%08x, pin=%d, flags=0x%08x\n", __func__, config->reg, config->port_num, pin, flags);
    uint32_t pinbit  = BIT(pin);                        //uint32_t pinbit  = (1UL << pin);
    uint32_t portnum = config->port_num;

    gpio_regs->GPIOFSxx[portnum] &= (~pinbit);          //*reg_func_select &= (~pinbit);
    if ((flags & GPIO_OUTPUT) != 0) {
        gpio_regs->GPIOIExx[portnum] |= pinbit;         //*reg_input_enable |= pinbit;
        if ((flags & GPIO_SINGLE_ENDED) != 0) {
            if (flags & GPIO_LINE_OPEN_DRAIN) {
                gpio_regs->GPIOODxx[portnum] |= pinbit;    //*reg_open_drain |= pinbit;
            }
        } else {
            gpio_regs->GPIOODxx[portnum] &= (~pinbit);      //*reg_open_drain &= (~pinbit);
        }
        if (flags & GPIO_PULL_UP) {
            gpio_regs->GPIOPUxx[portnum] |= pinbit;
        } else {
            gpio_regs->GPIOPUxx[portnum] &= (~pinbit);  //*reg_pull_up &= (~pinbit);
        }
        if ((flags & GPIO_OUTPUT_INIT_HIGH) != 0) {
            gpio_regs->GPIODxx[portnum] |= pinbit;      //*reg_output |= pinbit;
        } else if ((flags & GPIO_OUTPUT_INIT_LOW) != 0) {
            gpio_regs->GPIODxx[portnum] &= (~pinbit);   //*reg_output &= (~pinbit);
        }
        gpio_regs->GPIOOExx[portnum] |= pinbit;         //*reg_output_enable |= pinbit;
    }
    else {
        gpio_regs->GPIOOExx[portnum] &= (~pinbit);      //*reg_output_enable &= (~pinbit);
        if (flags & GPIO_PULL_UP) {
            gpio_regs->GPIOPUxx[portnum] |= pinbit;
        } else {
            gpio_regs->GPIOPUxx[portnum] &= (~pinbit);  //*reg_pull_up &= (~pinbit);
        }
        gpio_regs->GPIOIExx[portnum] |= pinbit;         //*reg_input_enable |= pinbit;
    }
    //gptd_regs->GPTDIExx[portnum] &= (~pinbit);          // interrupt disable
    return 0;
}

static int kb1200_gpio_port_get_raw(const struct device *dev,
                 gpio_port_value_t *value)
{
    const struct gpio_kb1200_config *config = dev->config;
    GPIO_T *gpio_regs = GPIO_REG_BASE;
    *value = gpio_regs->GPIOINxx[config->port_num];     //*value = *reg_input;
    return 0;
}

static int kb1200_gpio_port_set_masked_raw(const struct device *dev,
                    gpio_port_pins_t mask,
                    gpio_port_value_t value)
{
    const struct gpio_kb1200_config *config = dev->config;
    GPIO_T *gpio_regs = GPIO_REG_BASE;
    gpio_regs->GPIODxx[config->port_num] |= (value & mask); //*reg_output |= (value & mask);
    return 0;
}

static int kb1200_gpio_port_set_bits_raw(const struct device *dev,
                    gpio_port_pins_t pins)
{
    const struct gpio_kb1200_config *config = dev->config;
    GPIO_T *gpio_regs = GPIO_REG_BASE;
    gpio_regs->GPIODxx[config->port_num] |= pins;           //*reg_output |= pins;
    return 0;
}

static int kb1200_gpio_port_clear_bits_raw(const struct device *dev,
                    gpio_port_pins_t pins)
{
    const struct gpio_kb1200_config *config = dev->config;
    GPIO_T *gpio_regs = GPIO_REG_BASE;
    gpio_regs->GPIODxx[config->port_num] &= ~pins;          //*reg_output &= ~pins;
    return 0;
}

static int kb1200_gpio_port_toggle_bits(const struct device *dev,
                     gpio_port_pins_t pins)
{
    const struct gpio_kb1200_config *config = dev->config;
    GPIO_T *gpio_regs = GPIO_REG_BASE;
    gpio_regs->GPIODxx[config->port_num] ^= pins;          //*reg_output ^= pins;
    return 0;
}

static int kb1200_gpio_pin_interrupt_configure(const struct device *dev,
                        gpio_pin_t pin,
                        enum gpio_int_mode mode,
                        enum gpio_int_trig trig)
{
    const struct gpio_kb1200_config *config = dev->config;
    GPTD_T *gptd_regs = GPTD_REG_BASE;
    uint32_t pinbit   = BIT(pin);                           //uint32_t pinbit   = (1UL << pin);
    uint32_t portnum  = config->port_num;
    //printk("%s reg=0x%08x, pin=%d, mode=0x%08X, trig=0x%08X\n", __func__, config->reg, pin, mode, trig);

    /* Check if GPIO port needs interrupt support */
    if ((mode & GPIO_INT_DISABLE) || (mode & GPIO_INT_ENABLE) == 0) {
        /* Set the mask to disable the interrupt */
        gptd_regs->GPTDIExx[portnum] &= (~pinbit);          //*((volatile uint32_t *)(config->reg + OFFSET_GPTDIE)) &= (~pinbit);
    } else {
        if (mode & GPIO_INT_EDGE) {
            gptd_regs->GPTDELxx[portnum] &= (~pinbit);      //*((volatile uint32_t *)(config->reg + OFFSET_GPTDLTS)) &= (~pinbit);
            if (trig & GPIO_INT_HIGH_1) {
                if (trig & GPIO_INT_LOW_0) {   // Falling & Rising edge trigger
                    gptd_regs->GPTDCHGxx[portnum] |= pinbit;  // Enable toggle triggeer
                }
                else {  // Rising edge
                    gptd_regs->GPTDCHGxx[portnum] &= (~pinbit); // Disable Toggle trigger
                    gptd_regs->GPTDPSxx[portnum] |= pinbit;
                }
            }
            else {  // Falling edge
                gptd_regs->GPTDCHGxx[portnum] &= (~pinbit);  // Disable Toggle trigger
                gptd_regs->GPTDPSxx[portnum] &= (~pinbit);
            }
        } else {
            gptd_regs->GPTDELxx[portnum] |= pinbit;         //*((volatile uint32_t *)(config->reg + OFFSET_GPTDLTS)) |= pinbit;
            gptd_regs->GPTDCHGxx[portnum] &= (~pinbit);  // Disable Toggle trigger
            if (trig & GPIO_INT_HIGH_1) {
                gptd_regs->GPTDPSxx[portnum] |= pinbit;         //*((volatile uint32_t *)(config->reg + OFFSET_GPTDHPS)) |= pinbit;
            } else {
                gptd_regs->GPTDPSxx[portnum] &= (~pinbit);      //*((volatile uint32_t *)(config->reg + OFFSET_GPTDHPS)) &= (~pinbit);
            }
        }
        /* clear pending flag */
        gptd_regs->GPTDPFxx[portnum] |= pinbit;             //*((volatile uint32_t *)(config->reg + OFFSET_GPTDEPF)) |= pinbit; // clear pending flag
        /* Enable the interrupt */
        gptd_regs->GPTDIExx[portnum] |= pinbit;             //*((volatile uint32_t *)(config->reg + OFFSET_GPTDIE)) |= pinbit;
    }
    //*((volatile uint32_t *)(config->reg + OFFSET_GPTDEPF)) |= pinbit; // clear pending flag
    return 0;
}

static int kb1200_gpio_manage_callback(const struct device *dev,
                    struct gpio_callback *cb,
                    bool set)
{
    struct gpio_kb1200_data *context = dev->data;
    gpio_manage_callback(&context->cb, cb, set);
    return 0;
}

static uint32_t kb1200_gpio_get_pending_int(const struct device *dev)
{
    const struct gpio_kb1200_config * const config = dev->config;
    GPTD_T *gptd_regs = GPTD_REG_BASE;
    return gptd_regs->GPTDPFxx[config->port_num];
}

static const struct gpio_driver_api kb1200_gpio_api = {
    .pin_configure = kb1200_gpio_pin_configure,
    .port_get_raw = kb1200_gpio_port_get_raw,
    .port_set_masked_raw = kb1200_gpio_port_set_masked_raw,
    .port_set_bits_raw = kb1200_gpio_port_set_bits_raw,
    .port_clear_bits_raw = kb1200_gpio_port_clear_bits_raw,
    .port_toggle_bits = kb1200_gpio_port_toggle_bits,
    .pin_interrupt_configure = kb1200_gpio_pin_interrupt_configure,
    .manage_callback = kb1200_gpio_manage_callback,
    .get_pending_int = kb1200_gpio_get_pending_int
};

#define KB1200_GPIO_INIT(n)                                     \
    static int kb1200_gpio_##n##_init(const struct device *dev) \
    {   IRQ_CONNECT(DT_INST_IRQN(n),                            \
                    DT_INST_IRQ(n, priority),                   \
                    gpio_kb1200_isr,                            \
                    DEVICE_DT_INST_GET(n), 0);                  \
        irq_enable(DT_INST_IRQN(n));                            \
        IRQ_CONNECT((DT_INST_IRQN(n)+1),                        \
                    DT_INST_IRQ(n, priority),                   \
                    gpio_kb1200_isr,                            \
                    DEVICE_DT_INST_GET(n), 0);                  \
        irq_enable((DT_INST_IRQN(n)+1));                        \
        return 0;                                               \
    };                                                          \
    static const struct gpio_kb1200_config port_##n##_kb1200_config = { \
        .common = { .port_pin_mask = (gpio_port_pins_t)(-1) },  \
        .reg = DT_INST_REG_ADDR(n),                             \
        .port_num = (uint8_t)DT_INST_PROP(n, port_num),         \
    };                                                          \
    static struct gpio_kb1200_data gpio_kb1200_##n##_data;      \
    DEVICE_DT_INST_DEFINE(n, &kb1200_gpio_##n##_init, NULL,     \
                &gpio_kb1200_##n##_data,                        \
                &port_##n##_kb1200_config, POST_KERNEL,         \
                CONFIG_KERNEL_INIT_PRIORITY_DEVICE,             \
                &kb1200_gpio_api);

DT_INST_FOREACH_STATUS_OKAY(KB1200_GPIO_INIT)
