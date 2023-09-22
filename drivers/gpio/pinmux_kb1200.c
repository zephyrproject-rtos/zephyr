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
#define GCFG_REG_BASE       ((GCFG_T*)GCFG_BASE)

/* refer pinmux.h
   PINMUX_FUNC_A : GPIO        Function    PINMUX_FUNC_D : AltOutput 3 Function
   PINMUX_FUNC_B : AltOutput 1 Function    PINMUX_FUNC_E : AltOutput 4 Function
   PINMUX_FUNC_C : AltOutput 2 Function

GPIO Alternate Output Function Selection
  (PINMUX_FUNC_A) (PINMUX_FUNC_B) (PINMUX_FUNC_C) (PINMUX_FUNC_D) (PINMUX_FUNC_E)
   GPIO00          PWMLED0         PWM8
   GPIO22          ESBDAT          PWM9
   GPIO28          32KOUT          SERCLK2
   GPIO36          UARTSOUT        SERTXD2
   GPIO5C          KSO6            P80DAT
   GPIO5D          KSO7            P80CLK
   GPIO5E          KSO8            SERRXD1
   GPIO5F          KSO9            SERTXD1
   GPIO71          SDA8            UARTRTS
   GPIO38          SCL4            PWM1
*/
int gpio_pinmux_set(uint32_t port, uint32_t pin, uint32_t func)
{
	GPIO_T *gpio_regs = GPIO_REG_BASE;
	GCFG_T *gcfg_regs = GCFG_REG_BASE;
	uint32_t pinbit  = BIT(pin);
	uint32_t portnum = port;
	//printk("%s base=%p, pin=%u, func=%u\n", __func__, config->reg_base, pin, func);
	if (func == PINMUX_FUNC_GPIO)      //only GPIO function
	{
		gpio_regs->GPIOFSxx[portnum] &= (~pinbit);  //gpio_regs->GPIOFS &= ~bitpos;
		gpio_regs->GPIOIExx[portnum] |= pinbit;     //gpio_regs->GPIOIE |= bitpos; // Input always enable for loopback
	}
	else
	{
		uint32_t GPIO = ((portnum & 0xF) << 5) | (pin & 0x1F);
		func -= 1;                                  //for change to GPIOALT setting value
		switch (GPIO)
		{
		case GPIO00_PWMLED0_PWM8:
			mWRITEBIT(gcfg_regs->GPIOALT,func,0);     //gcfg_regs->GPIOALT |= (func & 0x01) << 0;
			break;
		case GPIO22_ESBDAT_PWM9:
			mWRITEBIT(gcfg_regs->GPIOALT,func,1);     //gcfg_regs->GPIOALT |= (func & 0x01) << 1;
			break;
		case GPIO28_32KOUT_SERCLK2:
			mWRITEBIT(gcfg_regs->GPIOALT,func,2);     //gcfg_regs->GPIOALT |= (func & 0x01) << 2;
			break;
		case GPIO36_UARTSOUT_SERTXD2:
			mWRITEBIT(gcfg_regs->GPIOALT,func,3);     //gcfg_regs->GPIOALT |= (func & 0x01) << 3;
			break;
		case GPIO5C_KSO6_P80DAT:
			mWRITEBIT(gcfg_regs->GPIOALT,func,4);     //gcfg_regs->GPIOALT |= (func & 0x01) << 4;
			break;
		case GPIO5D_KSO7_P80CLK:
			mWRITEBIT(gcfg_regs->GPIOALT,func,5);     //gcfg_regs->GPIOALT |= (func & 0x01) << 5;
			break;
		case GPIO5E_KSO8_SERRXD1:
			mWRITEBIT(gcfg_regs->GPIOALT,func,6);     //gcfg_regs->GPIOALT |= (func & 0x01) << 6;
			break;
		case GPIO5F_KSO9_SERTXD1:
			mWRITEBIT(gcfg_regs->GPIOALT,func,7);     //gcfg_regs->GPIOALT |= (func & 0x01) << 7;
			break;
		case GPIO71_SDA8_UARTRTS:
			mWRITEBIT(gcfg_regs->GPIOALT,func,8);     //gcfg_regs->GPIOALT |= (func & 0x01) << 8;
			break;
		case GPIO38_SCL4_PWM1:
			mWRITEBIT(gcfg_regs->GPIOALT,func,9);     //gcfg_regs->GPIOALT |= (func & 0x01) << 9;
			break;
		}
		gpio_regs->GPIOFSxx[portnum] |= pinbit;     //gpio_regs->GPIOFS |= bitpos;
		gpio_regs->GPIOIExx[portnum] |= pinbit;     //gpio_regs->GPIOIE |= bitpos; // Input always enable for loopback
	}
	return 0;
}

int gpio_pinmux_get(uint32_t port, uint32_t pin, uint32_t *func)
{
    GPIO_T *gpio_regs = GPIO_REG_BASE;
    GCFG_T *gcfg_regs = GCFG_REG_BASE;
    uint32_t pinbit  = BIT(pin);
    uint32_t portnum = port;
    //printk("%s base=0x%08x, pin=%u\n", __func__, gpio_regs->GPIOFSxx[portnum], pin);
    if (func == NULL)
        return -1;
    if (!(gpio_regs->GPIOFSxx[portnum] & pinbit)){      //!(gpio_regs->GPIOFS & bitpos)
        *func = PINMUX_FUNC_GPIO;                          //*func = PINMUX_FUNC_GPIO;
    }
    else {
        uint32_t GPIO = ((portnum & 0xF) << 5) | (pin & 0x1F);
        uint32_t altfunc = 0;
        switch (GPIO){
            case GPIO00_PWMLED0_PWM8:
                altfunc = mREADBIT(gcfg_regs->GPIOALT,0);     //altfunc = (gcfg_regs->GPIOALT & 0x01) >> 0;
                break;
            case GPIO22_ESBDAT_PWM9:
                altfunc = mREADBIT(gcfg_regs->GPIOALT,1);     //altfunc = (gcfg_regs->GPIOALT & 0x02) >> 1;
                break;
            case GPIO28_32KOUT_SERCLK2:
                altfunc = mREADBIT(gcfg_regs->GPIOALT,2);     //altfunc = (gcfg_regs->GPIOALT & 0x04) >> 2;
                break;
            case GPIO36_UARTSOUT_SERTXD2:
                altfunc = mREADBIT(gcfg_regs->GPIOALT,3);     //altfunc = (gcfg_regs->GPIOALT & 0x08) >> 3;
                break;
            case GPIO5C_KSO6_P80DAT:
                altfunc = mREADBIT(gcfg_regs->GPIOALT,4);     //altfunc = (gcfg_regs->GPIOALT & 0x20) >> 4;
                break;
            case GPIO5D_KSO7_P80CLK:
                altfunc = mREADBIT(gcfg_regs->GPIOALT,5);     //altfunc = (gcfg_regs->GPIOALT & 0x40) >> 5;
                break;
            case GPIO5E_KSO8_SERRXD1:
                altfunc = mREADBIT(gcfg_regs->GPIOALT,6);     //altfunc = (gcfg_regs->GPIOALT & 0x80) >> 6;
                break;
            case GPIO5F_KSO9_SERTXD1:
                altfunc = mREADBIT(gcfg_regs->GPIOALT,7);     //altfunc = (gcfg_regs->GPIOALT & 0x10) >> 7;
                break;
            case GPIO71_SDA8_UARTRTS:
                altfunc = mREADBIT(gcfg_regs->GPIOALT,8);     //altfunc = (gcfg_regs->GPIOALT & 0x20) >> 8;
                break;
            case GPIO38_SCL4_PWM1:
                altfunc = mREADBIT(gcfg_regs->GPIOALT,9);     //altfunc = (gcfg_regs->GPIOALT & 0x40) >> 9;
                break;
        }
        *func = 1+altfunc;                              // *func = 0x10 | altfunc;
        //printk("%s GPIO0x%02x func=%u \n", __func__,GPIO,*func);
    }
    return 0;
}

int gpio_pinmux_pullup(uint32_t port, uint32_t pin, uint8_t func)
{
    GPIO_T *gpio_regs = GPIO_REG_BASE;
    uint32_t pinbit = BIT(pin);
    uint32_t portnum = port;
    //printk("%s base=%p, pin=%u, func=%u\n", __func__, &(gpio_regs->GPIOPUxx[portnum]), pin, func);
    if (func == PINMUX_PULLUP_DISABLE){
        gpio_regs->GPIOPUxx[portnum] &= ~pinbit;    //gpio_regs->GPIOPU &= ~bitpos;
    }
    else if(func == PINMUX_PULLUP_ENABLE){
        gpio_regs->GPIOPUxx[portnum] |= pinbit;     //gpio_regs->GPIOPU |= bitpos;
    }
    return 0;
}

int gpio_pinmux_input(uint32_t port, uint32_t pin, uint8_t func)
{
    GPIO_T *gpio_regs = GPIO_REG_BASE;
    uint32_t pinbit = BIT(pin);
    uint32_t portnum = port;
    //printk("%s base=%p, pin=%u, func=%u\n", __func__, &(gpio_regs->GPIOOExx[portnum]), pin, func);
    if (func == PINMUX_OUTPUT_ENABLED){
        gpio_regs->GPIOOExx[portnum] |= pinbit;     //gpio_regs->GPIOOE |= bitpos;
        gpio_regs->GPIOIExx[portnum] |= pinbit;     //gpio_regs->GPIOIE |= bitpos; // Input always enable for loopback
    }
    else if(func == PINMUX_INPUT_ENABLED){
        gpio_regs->GPIOOExx[portnum] &= (~pinbit);  //gpio_regs->GPIOOE &= ~bitpos;
        gpio_regs->GPIOIExx[portnum] |= pinbit;     //gpio_regs->GPIOIE |= bitpos;
    }
    return 0;
}

