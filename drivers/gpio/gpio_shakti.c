#include <errno.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <soc.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/irq.h>
#include <zephyr/drivers/interrupt_controller/riscv_plic.h>
#include <zephyr/sw_isr_table.h>
#include <zephyr/devicetree.h>
// /home/suneeth/WORK/MG_Z/zephyr/include/zephyr/devicetree.h

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
        gpio->direction &= ~(1 << pin);
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
    volatile unsigned int pin_irq = 0;

    pin_irq = base_irq + pin;

    return pin_irq;
}

static int gpio_shakti_irq_handler(const struct device *dev)
{
    struct gpio_shakti_data *data = DEV_GPIO_DATA(dev);
    volatile struct gpio_shakti_regs_t *gpio_reg = DEV_GPIO(dev);
    const struct gpio_shakti_config *cfg = DEV_GPIO_CFG(dev); 

    uint8_t pin = ((uint8_t)(cfg->gpio_irq_base >> CONFIG_1ST_LEVEL_INTERRUPT_BITS) - 1) - 1 ; // This logic needs fixing
    
    gpio_reg->intr_config &= ~(BIT(pin));

    gpio_fire_callbacks(&data->cb, dev, BIT(pin));
    
    return 0;
}

static void gpio_shakti_isr(const struct device *dev)
{
    printf("Entered GPIO ISR()\n");
}

static int gpio_shakti_pin_interrupt_configure(const struct device *dev, 
                                                gpio_pin_t pin, 
                                                gpio_flags_t flag)
{
    volatile struct gpio_shakti_regs_t *gpio_reg = DEV_GPIO(dev);
    const struct gpio_shakti_config *cfg = DEV_GPIO_CFG(dev);

    // Initially disable interrupt for all 32 GPIOs
    gpio_reg->intr_config &= ~(0xFFFFFFFF); 
    
    if(flag == 1)
    {
        gpio_reg->intr_config &= ~(1 << pin);
    }
    else
    {
        gpio_reg->intr_config |= (1 << pin);
    }

    irq_enable(gpio_shakti_pin_irq(cfg->gpio_irq_base, pin));

    return 0;
}

static const struct gpio_driver_api gpio_shakti_driver = {
    .pin_configure              = gpio_shakti_pin_configure,
    .port_get_raw               = gpio_shakti_pin_get_raw,   
    .port_set_bits_raw          = gpio_shakti_pin_set_raw,    
    .port_clear_bits_raw        = gpio_shakti_pin_clear_raw,
    .port_toggle_bits           = gpio_shakti_pin_toggle, 
    .pin_interrupt_configure    = gpio_shakti_pin_interrupt_configure,  
};

// #define		IRQ_INIT(n)					\
// IRQ_CONNECT(n+32,   \
// 		1,		\
// 		gpio_shakti_irq_handler,    \
// 		NULL,				\
// 		0)

static void gpio_shakti_cfg(uint32_t gpio_pin){

    // static const int irq_line= gpio_pin + 1;
    gpio_pin = (1 << gpio_pin);
    // IRQ_CONNECT(1, 1,
    //             gpio_shakti_irq_handler,
    //             NULL, 0);     

    // IRQ_INIT(gpio_pin);
    // irq_enable(gpio_sha);

    // IRQ_CONNECT(irq_line, 1,
    //             gpio_shakti_irq_handler,
    //             NULL, 0);
// #if DT_INST_IRQ_HAS_IDX(0, 0)
// 	IRQ_INIT(0);
// #endif
// #if DT_INST_IRQ_HAS_IDX(0, 1)
// 	IRQ_INIT(1);
// #endif
// #if DT_INST_IRQ_HAS_IDX(0, 2)
// 	IRQ_INIT(2);
// #endif
// #if DT_INST_IRQ_HAS_IDX(0, 3)
// 	IRQ_INIT(3);
// #endif
// #if DT_INST_IRQ_HAS_IDX(0, 4)
// 	IRQ_INIT(4);
// #endif
// #if DT_INST_IRQ_HAS_IDX(0, 5)
// 	IRQ_INIT(5);
// #endif
// #if DT_INST_IRQ_HAS_IDX(0, 6)
// 	IRQ_INIT(6);
// #endif
// #if DT_INST_IRQ_HAS_IDX(0, 7)
// 	IRQ_INIT(7);
// #endif
// #if DT_INST_IRQ_HAS_IDX(0, 8)
// 	IRQ_INIT(8);
// #endif
// #if DT_INST_IRQ_HAS_IDX(0, 9)
// 	IRQ_INIT(9);
// #endif
// #if DT_INST_IRQ_HAS_IDX(0, 10)
// 	IRQ_INIT(10);
// #endif
// #if DT_INST_IRQ_HAS_IDX(0, 11)
// 	IRQ_INIT(11);
// #endif
// #if DT_INST_IRQ_HAS_IDX(0, 12)
// 	IRQ_INIT(12);
// #endif
// #if DT_INST_IRQ_HAS_IDX(0, 13)
// 	IRQ_INIT(13);
// #endif
// #if DT_INST_IRQ_HAS_IDX(0, 14)
// 	IRQ_INIT(14);
// #endif
// #if DT_INST_IRQ_HAS_IDX(0, 15)
// 	IRQ_INIT(15);
// #endif
// #if DT_INST_IRQ_HAS_IDX(0, 16)
// 	IRQ_INIT(16);
// #endif
// #if DT_INST_IRQ_HAS_IDX(0, 17)
// 	IRQ_INIT(17);
// #endif
// #if DT_INST_IRQ_HAS_IDX(0, 18)
// 	IRQ_INIT(18);
// #endif
// #if DT_INST_IRQ_HAS_IDX(0, 19)
// 	IRQ_INIT(19);
// #endif
// #if DT_INST_IRQ_HAS_IDX(0, 20)
// 	IRQ_INIT(20);
// #endif
// #if DT_INST_IRQ_HAS_IDX(0, 21)
// 	IRQ_INIT(21);
// #endif
// #if DT_INST_IRQ_HAS_IDX(0, 22)
// 	IRQ_INIT(22);
// #endif
// #if DT_INST_IRQ_HAS_IDX(0, 23)
// 	IRQ_INIT(23);
// #endif
// #if DT_INST_IRQ_HAS_IDX(0, 24)
// 	IRQ_INIT(24);
// #endif
// #if DT_INST_IRQ_HAS_IDX(0, 25)
// 	IRQ_INIT(25);
// #endif
// #if DT_INST_IRQ_HAS_IDX(0, 26)
// 	IRQ_INIT(26);
// #endif
// #if DT_INST_IRQ_HAS_IDX(0, 27)
// 	IRQ_INIT(27);
// #endif
// #if DT_INST_IRQ_HAS_IDX(0, 28)
// 	IRQ_INIT(28);
// #endif
// #if DT_INST_IRQ_HAS_IDX(0, 29)
// 	IRQ_INIT(29);
// #endif
// #if DT_INST_IRQ_HAS_IDX(0, 30)
// 	IRQ_INIT(30);
// #endif
// #if DT_INST_IRQ_HAS_IDX(0, 31)
// 	IRQ_INIT(31);
// #endif
}

static const struct gpio_shakti_config gpio_shakti_config0 ={
    // .common = {
    //     .port_pin_mask  = GPIO_PORT_PIN_MASK_FROM_DT_INST(0),
    // },
    .gpio_base_addr     = GPIO_START,
    .gpio_irq_base      = GPIO_IRQ_BASE,
    .gpio_cfg_func      = gpio_shakti_cfg,
    .gpio_mode          = DT_PROP(DT_NODELABEL(gpio0), config_gpio)
};

static struct gpio_shakti_data gpio_shakti_data0 ={

    .cb = gpio_shakti_isr
};

#define GPIO_INIT(inst)	\
DEVICE_DT_INST_DEFINE(inst, \
                gpio_shakti_init,  \
                NULL, \
                &gpio_shakti_data0, &gpio_shakti_config0, \
                PRE_KERNEL_1, CONFIG_GPIO_INIT_PRIORITY, \
                &gpio_shakti_driver); 

// IRQ_CONNECT(1, 1, gpio_shakti_isr, DEVICE_DT_GET(DT_NODELABEL(gpio0)), 0));

DT_INST_FOREACH_STATUS_OKAY(GPIO_INIT)