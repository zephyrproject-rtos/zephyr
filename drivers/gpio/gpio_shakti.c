#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <soc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/irq.h>

#include "gpio_shakti.h"

#include <zephyr/drivers/gpio/gpio_utils.h>


#define DT_DRV_COMPAT shakti_gpio

typedef void (*shakti_cfg_func_t)(void);

typedef void (*gpio_shakti_cfg_func_t)(void);

typedef struct gpio_shakti_regs_t
{
    uint32_t  direction;               /*! direction register */
    uint32_t  reserved0;                /*! reserved for future use */
    uint32_t  data;                    /*! data register */
    uint32_t  reserved1;                /*! reserved for future use */
    uint32_t  set;                 /*! set register */
    uint32_t  reserved2;                /*! reserved for future use */
    uint32_t  clear;                   /*! clear register */
    uint32_t  reserved3;                /*! reserved for future use */
    uint32_t  toggle;               /*! toggle register */
    uint32_t  reserved4;                /*! reserved for future use */
    uint8_t  qualification;    /*! qualification register */
    uint8_t  reserved5;                /*! reserved for future use */
    uint16_t  reserved6;              /*! reserved for future use */
    uint32_t  reserved12;              /*! reserved for future use */
    uint32_t  intr_config;     /*! interrupt configuration register */
    uint32_t  reserved7;              /*! reserved for future use */
};


struct gpio_shakti_config{
/* gpio_driver_config needs to be first */
    struct gpio_driver_config common;
    uintptr_t gpio_base_addr;
    uint32_t gpio_irq_base;
    gpio_shakti_cfg_func_t gpio_cfg_func;
    uint32_t gpio_mode;
};

struct gpio_shakti_data {

    struct gpio_driver_data common;
    sys_slist_t cb;

};

/* Helper Macros for GPIO */
#define DEV_GPIO_CFG(dev)						\
	((const struct gpio_shakti_config * const)(dev)->config)
#define DEV_GPIO(dev)							\
	((volatile struct gpio_shakti_regs_t *)(DEV_GPIO_CFG(dev))->gpio_base_addr)
#define DEV_GPIO_DATA(dev)				\
	((struct gpio_shakti_data *)(dev)->data)


int gpio_shakti_init(const struct device *dev){
    
    volatile struct gpio_shakti_regs_t *gpio = DEV_GPIO(dev);
    const struct gpio_shakti_config *cfg = DEV_GPIO_CFG(dev);

    // gpio = GPIO_START;
    // printk("Initialization Done\n");
    return 0;
}


static int gpio_shakti_pin_configure (const struct device *dev, 
                        gpio_pin_t pin, 
                        gpio_flags_t flags){

    volatile struct gpio_shakti_regs_t *gpio = DEV_GPIO(dev);
    const struct gpio_shakti_config *cfg = DEV_GPIO_CFG(dev);
    // printk("GPIO MODE: %d\n", cfg->gpio_mode);
    
    if(flags & GPIO_OUTPUT){
        gpio->direction =-1;
        // printk("GPIO Output Mode.\n");
    }
    else{
        gpio->direction |= (0 << pin);
        // printk("GPIO Input Mode.\n");
    }

    // printk("Configuration Done2\n");

    return 0;
}

static int gpio_shakti_pin_get_raw(const struct device *dev,
                    gpio_pin_t pin)
{
    volatile struct gpio_shakti_regs_t *gpio = DEV_GPIO(dev);
    return gpio->data;

}

static int gpio_shakti_pin_set_raw(const struct device *dev,
                    gpio_pin_t pin)
{
    volatile struct gpio_shakti_regs_t *gpio = DEV_GPIO(dev);   
    const struct gpio_shakti_config *cfg = DEV_GPIO_CFG(dev);
    // printf("GPIO Set Addr:%#x, Pin: %d",&(gpio ->set), pin);
    gpio ->set = pin;
    // printk("set has been done \n");
    // printk("gpio addr: 0x%x", gpio);

    return 0;
}

static int gpio_shakti_pin_toggle(const struct device *dev,
                    gpio_pin_t pin)
{
    // printf("toggle pin\n");
    volatile struct gpio_shakti_regs_t *gpio = DEV_GPIO(dev);
    gpio ->toggle = pin;

    return 0;
}

static int gpio_shakti_pin_clear_raw(const struct device *dev,
                    gpio_pin_t pin)
{
    volatile struct gpio_shakti_regs_t *gpio = DEV_GPIO(dev);   
    // printf("GPIO Clear Addr:%#x, Pin: %d",&(gpio ->clear), pin);
    gpio ->clear = pin;
    // *((uint32_t*)(0x40218))=(1<<pin);
    // printk("Cleared \n");

    return 0;
}

//-------Function WIP----------

static inline unsigned int gpio_shakti_pin_irq(unsigned int base_irq, int pin)
{
    unsigned int level = irq_get_level(base_irq);
    unsigned int pin_irq = 0;

    if (level == 1)
    {
        pin_irq = base_irq + pin;
    }

    return pin_irq;
}



static int gpio_shakti_pin_interrupt_configure(const struct device *dev, 
                                                gpio_pin_t pin, 
                                                enum gpio_int_mode mode,
                                                enum gpio_int_trig trig)
{
    volatile struct gpio_shakti_regs_t *gpio_reg = DEV_GPIO(dev);
    const struct gpio_shakti_config *cfg = DEV_GPIO_CFG(dev);

    // Initially disable interrupt for all 32 GPIOs
    gpio_reg->intr_config &= ~(0xFFFFFFFF); 

    switch (mode) {
	case GPIO_INT_MODE_DISABLED:
		irq_disable(gpio_shakti_pin_irq(cfg->gpio_irq_base, pin));
		break;
	case GPIO_INT_MODE_LEVEL:
		/* Board supports both levels, but Zephyr does not. */
		if (trig == GPIO_INT_TRIG_HIGH) {
			gpio_reg->intr_config |= (1 << pin);
		} else {
			__ASSERT_NO_MSG(trig == GPIO_INT_TRIG_LOW);
			gpio_reg->intr_config &= ~(1 << pin);
		}
		irq_enable(gpio_shakti_pin_irq(cfg->gpio_irq_base, pin));
		break;
	// case GPIO_INT_MODE_EDGE:
	// 	__ASSERT_NO_MSG(GPIO_INT_TRIG_BOTH ==
	// 			(GPIO_INT_LOW_0 | GPIO_INT_HIGH_1));

	// 	if ((trig & GPIO_INT_HIGH_1) != 0) {
	// 		gpio->rise_ip = BIT(pin);
	// 		gpio->rise_ie |= BIT(pin);
	// 	}
	// 	if ((trig & GPIO_INT_LOW_0) != 0) {
	// 		gpio->fall_ip = BIT(pin);
	// 		gpio->fall_ie |= BIT(pin);
	// 	}
	// 	irq_enable(gpio_sifive_pin_irq(cfg->gpio_irq_base, pin));
	// 	break;
	default:
		__ASSERT(false, "Invalid MODE %d passed to driver", mode);
		return -ENOTSUP;
	}
}

static const struct gpio_driver_api gpio_shakti_driver = {
    .pin_configure              = gpio_shakti_pin_configure,
    .port_get_raw               = gpio_shakti_pin_get_raw,   
    .port_set_bits_raw          = gpio_shakti_pin_set_raw,    
    .port_clear_bits_raw        = gpio_shakti_pin_clear_raw,
    .port_toggle_bits           = gpio_shakti_pin_toggle, 
    .pin_interrupt_configure    = gpio_shakti_pin_interrupt_configure,  
};

static void gpio_shakti_cfg(uint32_t gpio_pin){
    gpio_pin = (1 << gpio_pin);
}

static const struct gpio_shakti_config gpio_shakti_config0 ={
    // .common = {
    //     .port_pin_mask  = GPIO_PORT_PIN_MASK_FROM_DT_INST(0),
    // },
    .gpio_base_addr     = GPIO_START,
    .gpio_irq_base      = DT_INST_IRQ(0),
    .gpio_cfg_func      = gpio_shakti_cfg,
    .gpio_mode          = DT_PROP(DT_NODELABEL(gpio0), config_gpio)
};

static struct gpio_shakti_data gpio_shakti_data0;

#define GPIO_INIT(inst)	\
DEVICE_DT_INST_DEFINE(inst, \
                gpio_shakti_init,  \
                NULL, \
                &gpio_shakti_data0, &gpio_shakti_config0, \
                PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY, \
                &gpio_shakti_driver); 

DT_INST_FOREACH_STATUS_OKAY(GPIO_INIT)