/*
 * Private Porting
 * by David Hor - Xtooltech 2025
 * david.hor@xtooltech.com
 */

#include <stdint.h>
#include <stdbool.h>
#include <zephyr/drivers/gpio.h>

/* LPC54S018 GPIO registers - direct register access */
#ifndef GPIO_BASE
#define GPIO_BASE           0x4008C000UL
#endif
#ifndef SYSCON_BASE
#define SYSCON_BASE         0x40000000UL
#endif
#ifndef IOCON_BASE
#define IOCON_BASE          0x40001000UL
#endif

/* GPIO register offsets */
#define GPIO_B_OFFSET       0x0000  /* Byte pin registers */
#define GPIO_W_OFFSET       0x1000  /* Word pin registers */
#define GPIO_DIR_OFFSET     0x2000  /* Direction registers */
#define GPIO_PIN_OFFSET     0x2100  /* Port pin registers */
#define GPIO_SET_OFFSET     0x2200  /* Write: Set register */
#define GPIO_CLR_OFFSET     0x2280  /* Clear port */
#define GPIO_NOT_OFFSET     0x2300  /* Toggle port */
#define GPIO_DIRSET_OFFSET  0x2380  /* Set pin direction bits */
#define GPIO_DIRCLR_OFFSET  0x2400  /* Clear pin direction bits */

/* SYSCON AHBCLKCTRL0 register */
#define SYSCON_AHBCLKCTRL0  (*(volatile uint32_t *)(SYSCON_BASE + 0x200))

/* Clock enable bits for GPIO ports */
#define GPIO0_CLK_EN    (1 << 14)
#define GPIO1_CLK_EN    (1 << 15)
#define GPIO2_CLK_EN    (1 << 16)
#define GPIO3_CLK_EN    (1 << 17)
#define GPIO4_CLK_EN    (1 << 18)
#define GPIO5_CLK_EN    (1 << 19)

/* IOCON settings */
#define IOCON_FUNC0         (0x0)    /* Function 0 - GPIO */
#define IOCON_MODE_INACTIVE (0x0)    /* No pull resistor */
#define IOCON_DIGITAL_EN    (1 << 7) /* Digital mode */

/* GPIO register access macros */
#define GPIO_DIR(port)      (*(volatile uint32_t *)(GPIO_BASE + GPIO_DIR_OFFSET + ((port) * 4)))
#define GPIO_PIN(port)      (*(volatile uint32_t *)(GPIO_BASE + GPIO_PIN_OFFSET + ((port) * 4)))
#define GPIO_SET(port)      (*(volatile uint32_t *)(GPIO_BASE + GPIO_SET_OFFSET + ((port) * 4)))
#define GPIO_CLR(port)      (*(volatile uint32_t *)(GPIO_BASE + GPIO_CLR_OFFSET + ((port) * 4)))
#define GPIO_NOT(port)      (*(volatile uint32_t *)(GPIO_BASE + GPIO_NOT_OFFSET + ((port) * 4)))
#define GPIO_DIRSET(port)   (*(volatile uint32_t *)(GPIO_BASE + GPIO_DIRSET_OFFSET + ((port) * 4)))
#define GPIO_DIRCLR(port)   (*(volatile uint32_t *)(GPIO_BASE + GPIO_DIRCLR_OFFSET + ((port) * 4)))
#define GPIO_B(port, pin)   (*(volatile uint8_t *)(GPIO_BASE + GPIO_B_OFFSET + ((port) * 0x20) + (pin)))

/* IOCON register access - each pin has 4 bytes */
#define IOCON_PIO(port, pin) (*(volatile uint32_t *)(IOCON_BASE + (((port) * 32 + (pin)) * 4)))

/* Enable clock for GPIO port */
static void gpio_port_init(uint32_t port)
{
    uint32_t clock_bit = 0;
    
    switch (port) {
        case 0: clock_bit = GPIO0_CLK_EN; break;
        case 1: clock_bit = GPIO1_CLK_EN; break;
        case 2: clock_bit = GPIO2_CLK_EN; break;
        case 3: clock_bit = GPIO3_CLK_EN; break;
        case 4: clock_bit = GPIO4_CLK_EN; break;
        case 5: clock_bit = GPIO5_CLK_EN; break;
        default: return;
    }
    
    /* Enable GPIO port clock */
    SYSCON_AHBCLKCTRL0 |= clock_bit;
}

/* Initialize GPIO pin */
void gpio_pin_init(uint32_t port, uint32_t pin, bool output)
{
    if (port > 5 || pin > 31) {
        return;
    }
    
    /* Enable port clock */
    gpio_port_init(port);
    
    /* Configure IOCON for GPIO function */
    IOCON_PIO(port, pin) = IOCON_FUNC0 | IOCON_MODE_INACTIVE | IOCON_DIGITAL_EN;
    
    /* Set direction */
    if (output) {
        GPIO_DIRSET(port) = (1U << pin);
    } else {
        GPIO_DIRCLR(port) = (1U << pin);
    }
}

/* Write to GPIO pin */
void gpio_pin_write(uint32_t port, uint32_t pin, bool value)
{
    if (port > 5 || pin > 31) {
        return;
    }
    
    /* Use byte register for single pin access */
    GPIO_B(port, pin) = value ? 1 : 0;
}

/* Read GPIO pin */
bool gpio_pin_read(uint32_t port, uint32_t pin)
{
    if (port > 5 || pin > 31) {
        return false;
    }
    
    return (GPIO_PIN(port) & (1U << pin)) != 0;
}

/* Toggle GPIO pin - renamed to avoid conflict */
void gpio_pin_toggle_lpc(uint32_t port, uint32_t pin)
{
    if (port > 5 || pin > 31) {
        return;
    }
    
    GPIO_NOT(port) = (1U << pin);
}

/* Override the weak stub implementations in gpio.h */
void gpio_lpc_init(void)
{
    /* Initialize blue LED on GPIO2, Pin 2 (USER LED, LED3) */
    gpio_pin_init(2, 2, true);
    
    /* Turn LED off initially (active low) */
    gpio_pin_write(2, 2, 1);
}

void gpio_lpc_toggle_led(void)
{
    /* Toggle blue LED on GPIO2, Pin 2 */
    gpio_pin_toggle_lpc(2, 2);
}