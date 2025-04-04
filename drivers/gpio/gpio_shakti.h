#ifdef __cplusplus
extern "C" {
#endif

// #ifndef ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_SHAKTI_H_
// #define ZEPHYR_INCLUDE_DRIVERS_GPIO_GPIO_SHAKTI_H_

#include <stdint.h>
#include <errno.h>

#include <zephyr/sys/__assert.h>
#include <zephyr/sys/slist.h>

#include <zephyr/types.h>
#include <stddef.h>
#include <zephyr/device.h>
#include <zephyr/dt-bindings/gpio/gpio.h>
#include <zephyr/kernel.h>

typedef uint8_t gpio_pin_t;
typedef uint32_t gpio_flags_t;


#define GPIO_START 0x40200 //GPIO Start Address 
#define GPIO_OFFSET 0x08 /*!Generic offset used to access GPIO registers*/


#define GPIO_DIRECTION_CNTRL_REG  (GPIO_START + (0 * GPIO_OFFSET))
#define GPIO_DATA_REG             (GPIO_START + (1 * GPIO_OFFSET))
#define GPIO_SET_REG              (GPIO_START + (2 * GPIO_OFFSET))
#define GPIO_CLEAR_REG            (GPIO_START + (3 * GPIO_OFFSET))
#define GPIO_TOGGLE_REG           (GPIO_START + (4 * GPIO_OFFSET))
#define GPIO_QUAL_REG             (GPIO_START + (5 * GPIO_OFFSET))
#define GPIO_INTERRUPT_CONFIG_REG (GPIO_START + (6 * GPIO_OFFSET))

#define GPIO_INPUT  0
#define GPIO_OUTPUT 1
#define GPIO_QUAL_MAX_CYCLES 15
#define ALL_GPIO_PINS -1

#define GPIO_IRQ_BASE 32

#define GPIO0  (1 <<  0)
#define GPIO1  (1 <<  1)
#define GPIO2  (1 <<  2)
#define GPIO3  (1 <<  3)
#define GPIO4  (1 <<  4)
#define GPIO5  (1 <<  5)
#define GPIO6  (1 <<  6)
#define GPIO7  (1 <<  7)
#define GPIO8  (1 <<  8)
#define GPIO9  (1 <<  9)
#define GPIO10 (1 << 10)
#define GPIO11 (1 << 11)
#define GPIO12 (1 << 12)
#define GPIO13 (1 << 13)
#define GPIO14 (1 << 14)
#define GPIO15 (1 << 15)
#define GPIO16 (1 << 16)
#define GPIO17 (1 << 17)
#define GPIO18 (1 << 18)
#define GPIO19 (1 << 19)
#define GPIO20 (1 << 20)
#define GPIO21 (1 << 21)
#define GPIO22 (1 << 22)
#define GPIO23 (1 << 23)
#define GPIO24 (1 << 24)
#define GPIO25 (1 << 25)
#define GPIO26 (1 << 26)
#define GPIO27 (1 << 27)
#define GPIO28 (1 << 28)
#define GPIO29 (1 << 29)
#define GPIO30 (1 << 30)
#define GPIO31 (1 << 31)
#define GPIO_COUNT  0x20

#ifdef __cplusplus
}
#endif