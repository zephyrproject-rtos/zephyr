/*
 * Multi-LED Blink for STM32F407 (Zephyr)
 */
#if 0
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

/* Blink interval */
#define SLEEP_TIME_MS 500

/* Get LED aliases from Devicetree */
#define LED0_NODE DT_ALIAS(led0)
#define LED1_NODE DT_ALIAS(led1)
#define LED2_NODE DT_ALIAS(led2)
#define LED3_NODE DT_ALIAS(led3)

/* Check all aliases exist */
#if !DT_NODE_HAS_STATUS(LED0_NODE, okay) || \
    !DT_NODE_HAS_STATUS(LED1_NODE, okay) || \
    !DT_NODE_HAS_STATUS(LED2_NODE, okay) || \
    !DT_NODE_HAS_STATUS(LED3_NODE, okay)
#error "LED aliases not found! Check board DTS or overlay."
#endif

/* LED GPIO specs */
static const struct gpio_dt_spec led0 = GPIO_DT_SPEC_GET(LED0_NODE, gpios);
static const struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
static const struct gpio_dt_spec led2 = GPIO_DT_SPEC_GET(LED2_NODE, gpios);
static const struct gpio_dt_spec led3 = GPIO_DT_SPEC_GET(LED3_NODE, gpios);

void main(void)
{
    gpio_pin_configure_dt(&led0, GPIO_OUTPUT_ACTIVE);
    gpio_pin_configure_dt(&led1, GPIO_OUTPUT_ACTIVE);
    gpio_pin_configure_dt(&led2, GPIO_OUTPUT_ACTIVE);
    gpio_pin_configure_dt(&led3, GPIO_OUTPUT_ACTIVE);

    bool state = true;

    while (1) {
        /* Toggle all LEDs */
        gpio_pin_set_dt(&led0, state);
        gpio_pin_set_dt(&led1, state);
        gpio_pin_set_dt(&led2, state);
        gpio_pin_set_dt(&led3, state);

        state = !state;
        k_msleep(SLEEP_TIME_MS);
    }
}
#else
/*
 * Ultra-Optimized Multi-LED Blink for STM32F4 (Zephyr)
 * Best-practice structure-based design.
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

/* Blink speed */
#define BLINK_INTERVAL_MS 500

/* ========== Devicetree LED aliases ========== */
#define LED0_NODE DT_ALIAS(led0)
#define LED1_NODE DT_ALIAS(led1)
#define LED2_NODE DT_ALIAS(led2)
#define LED3_NODE DT_ALIAS(led3)

/* Validate availability at compile time */
#if !DT_NODE_HAS_STATUS(LED0_NODE, okay) || \
    !DT_NODE_HAS_STATUS(LED1_NODE, okay) || \
    !DT_NODE_HAS_STATUS(LED2_NODE, okay) || \
    !DT_NODE_HAS_STATUS(LED3_NODE, okay)
#error "LED aliases missing in DTS. Fix board overlay."
#endif

/* ========== LED Structure ========== */
struct led_info {
    struct gpio_dt_spec gpio;
    const char *name;
};

/* ========== LED Table ========== */
/* Add/remove LEDs by editing ONLY this table */
static const struct led_info led_table[] = {
    { GPIO_DT_SPEC_GET(LED0_NODE, gpios), "LED0" },
    { GPIO_DT_SPEC_GET(LED1_NODE, gpios), "LED1" },
    { GPIO_DT_SPEC_GET(LED2_NODE, gpios), "LED2" },
    { GPIO_DT_SPEC_GET(LED3_NODE, gpios), "LED3" },
};

/* Number of LEDs (auto) */
static const size_t LED_COUNT = ARRAY_SIZE(led_table);

/* ========== Helper Functions ========== */

/* Fast inline: Init each LED */
static inline int init_led(const struct led_info *led)
{
    if (!device_is_ready(led->gpio.port)) {
        printk("ERR: %s port not ready\n", led->name);
        return -ENODEV;
    }

    int ret = gpio_pin_configure_dt(&led->gpio, GPIO_OUTPUT_INACTIVE);
    if (ret) {
        printk("ERR: Failed to configure %s (%d)\n", led->name, ret);
    }
    return ret;
}

/* Fast inline: Toggle all LEDs */
static inline void set_all_leds(bool state)
{
    for (size_t i = 0; i < LED_COUNT; i++) {
        gpio_pin_set_dt(&led_table[i].gpio, state);
    }
}

/* Fast inline: Toggle all LEDs */
static inline void set_alt_leds(bool state)
{
    for (size_t i = 0; i < LED_COUNT; i++) {
	state = !state;	
        gpio_pin_set_dt(&led_table[i].gpio, state);
    }
}
/* ========== Main Application ========== */
void main(void)
{
    /* -------- LED Initialization -------- */
    for (size_t i = 0; i < LED_COUNT; i++) {
        if (init_led(&led_table[i]) != 0) {
            printk("LED init failed. System halt.\n");
            return;
        }
    }

    printk("✔ LED system online (%d LEDs)\n", LED_COUNT);

    bool state = false;

    /* -------- LED Blink Loop -------- */
    while (1) {
        set_alt_leds(state);
        state = !state;
        k_msleep(BLINK_INTERVAL_MS);	
        set_all_leds(!state);
        k_msleep(BLINK_INTERVAL_MS);
    	printk("LED system state %d\n", state);
    }
}

#endif
