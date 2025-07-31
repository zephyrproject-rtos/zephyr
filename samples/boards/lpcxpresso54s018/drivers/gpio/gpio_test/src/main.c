/*
 * Private Porting , by David Hor - Xtooltech 2025, david.hor@xtooltech.com
 *
 * GPIO Test Application for LPC54S018
 * Tests all 30 GPIO pins from VCI project
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(gpio_test, LOG_LEVEL_INF);

/* GPIO device tree nodes */
#define GPIO0_NODE DT_NODELABEL(gpio0)
#define GPIO1_NODE DT_NODELABEL(gpio1)
#define GPIO2_NODE DT_NODELABEL(gpio2)
#define GPIO3_NODE DT_NODELABEL(gpio3)
#define GPIO4_NODE DT_NODELABEL(gpio4)

/* LED pins (active low) */
#define LED_RED_PIN    14  /* GPIO3_14 */
#define LED_GREEN_PIN  3   /* GPIO3_3 */
#define LED_BLUE_PIN   2   /* GPIO2_2 */

/* Button pins (active low with pull-up) */
#define BUTTON_OK_PIN   4  /* GPIO4_4 */
#define BUTTON_BACK_PIN 5  /* GPIO4_5 */
#define BUTTON_UP_PIN   6  /* GPIO4_6 */
#define BUTTON_DOWN_PIN 7  /* GPIO4_7 */

/* VCI GPIO pins structure */
struct vci_gpio_pin {
    const char *name;
    const struct device *port;
    uint8_t pin;
    bool is_output;
    bool active_low;
};

/* Button callback data */
struct button_callback_data {
    struct gpio_callback cb;
    const char *name;
};

/* Define all 30 GPIO pins from VCI project */
static struct vci_gpio_pin vci_pins[] = {
    /* Outputs */
    {"DC_IN_PWR_CTR", NULL, 1, true, false},      /* GPIO0_1 */
    {"BL_CTR", NULL, 22, true, false},            /* GPIO0_22 */
    {"SW_15_87", NULL, 2, true, false},           /* GPIO2_2 */
    {"SW_CYPWM", NULL, 6, true, false},           /* GPIO2_6 */
    {"PHY_SWITCH", NULL, 8, true, false},         /* GPIO2_8 */
    {"DOIP_OBD3_11", NULL, 9, true, false},       /* GPIO2_9 */
    {"DOIP_OBD1_9", NULL, 10, true, false},       /* GPIO2_10 */
    {"MOS02_SW", NULL, 12, true, false},          /* GPIO2_12 */
    {"LCD_RST", NULL, 15, true, false},           /* GPIO2_15 */
    {"OBD1000_RST", NULL, 17, true, false},       /* GPIO2_17 */
    {"BUZ_CTR", NULL, 2, true, false},            /* GPIO3_2 */
    {"PHY_POWER_EN", NULL, 3, true, false},       /* GPIO3_3 */
    {"ESP32_MODE", NULL, 10, true, false},        /* GPIO3_10 */
    {"ESP32_SPI_CLK", NULL, 20, true, false},     /* GPIO3_20 */
    {"ESP32_POWER_CTR", NULL, 28, true, false},   /* GPIO3_28 */
    {"CYPWM1_CTR", NULL, 29, true, false},        /* GPIO3_29 */
    {"CYPWM2_CTR", NULL, 30, true, false},        /* GPIO3_30 */
    {"DOIP_EN02", NULL, 0, true, false},          /* GPIO4_0 */
    {"DOIP_EN01", NULL, 1, true, false},          /* GPIO4_1 */
    
    /* Inputs */
    {"ESP32_LINK", NULL, 1, false, false},        /* GPIO1_1 */
    {"ESP32_TRANSHAKE", NULL, 4, false, false},   /* GPIO3_4 */
    {"SMODE0_SET", NULL, 23, false, false},       /* GPIO3_23 */
    {"SMODE1_SET", NULL, 24, false, false},       /* GPIO3_24 */
    {"KEY_OK", NULL, 4, false, true},             /* GPIO4_4 */
    {"KEY_BACK", NULL, 5, false, true},           /* GPIO4_5 */
    {"KEY_UP", NULL, 6, false, true},             /* GPIO4_6 */
    {"KEY_DOWN", NULL, 7, false, true},           /* GPIO4_7 */
};

/* Button callback functions */
static struct button_callback_data button_ok_cb_data = {.name = "OK"};
static struct button_callback_data button_back_cb_data = {.name = "BACK"};
static struct button_callback_data button_up_cb_data = {.name = "UP"};
static struct button_callback_data button_down_cb_data = {.name = "DOWN"};

static void button_callback(const struct device *dev, struct gpio_callback *cb,
                           uint32_t pins)
{
    struct button_callback_data *data = 
        CONTAINER_OF(cb, struct button_callback_data, cb);
    
    LOG_INF("Button %s pressed!", data->name);
}

/**
 * @brief Initialize a GPIO pin
 */
static int init_gpio_pin(struct vci_gpio_pin *pin)
{
    int ret;
    gpio_flags_t flags;
    
    if (!pin->port) {
        LOG_ERR("GPIO port not ready for %s", pin->name);
        return -ENODEV;
    }
    
    /* Configure pin flags */
    if (pin->is_output) {
        flags = GPIO_OUTPUT;
        if (pin->active_low) {
            flags |= GPIO_OUTPUT_INIT_HIGH;  /* Off initially */
        } else {
            flags |= GPIO_OUTPUT_INIT_LOW;   /* Off initially */
        }
    } else {
        flags = GPIO_INPUT;
        if (pin->active_low) {
            flags |= GPIO_PULL_UP;
        }
    }
    
    /* Configure the pin */
    ret = gpio_pin_configure(pin->port, pin->pin, flags);
    if (ret < 0) {
        LOG_ERR("Failed to configure %s: %d", pin->name, ret);
        return ret;
    }
    
    LOG_DBG("Configured %s as %s", pin->name, 
            pin->is_output ? "output" : "input");
    
    return 0;
}

/**
 * @brief Test GPIO output by toggling
 */
static void test_gpio_output(struct vci_gpio_pin *pin)
{
    int ret;
    
    if (!pin->is_output) {
        return;
    }
    
    LOG_INF("Testing output: %s", pin->name);
    
    /* Toggle the pin 3 times */
    for (int i = 0; i < 3; i++) {
        /* Turn on */
        ret = gpio_pin_set(pin->port, pin->pin, pin->active_low ? 0 : 1);
        if (ret < 0) {
            LOG_ERR("Failed to set %s high: %d", pin->name, ret);
            return;
        }
        k_msleep(100);
        
        /* Turn off */
        ret = gpio_pin_set(pin->port, pin->pin, pin->active_low ? 1 : 0);
        if (ret < 0) {
            LOG_ERR("Failed to set %s low: %d", pin->name, ret);
            return;
        }
        k_msleep(100);
    }
}

/**
 * @brief Test GPIO input by reading
 */
static void test_gpio_input(struct vci_gpio_pin *pin)
{
    int val;
    
    if (pin->is_output) {
        return;
    }
    
    val = gpio_pin_get(pin->port, pin->pin);
    if (val < 0) {
        LOG_ERR("Failed to read %s: %d", pin->name, val);
        return;
    }
    
    LOG_INF("Input %s = %d %s", pin->name, val,
            pin->active_low ? (val ? "(inactive)" : "(active)") : 
                             (val ? "(active)" : "(inactive)"));
}

int main(void)
{
    const struct device *gpio0, *gpio1, *gpio2, *gpio3, *gpio4;
    int ret;
    int pin_idx;
    
    LOG_INF("GPIO Test Application Starting");
    LOG_INF("Testing all 30 GPIO pins from VCI project");
    
    /* Get GPIO devices */
    gpio0 = DEVICE_DT_GET(GPIO0_NODE);
    gpio1 = DEVICE_DT_GET(GPIO1_NODE);
    gpio2 = DEVICE_DT_GET(GPIO2_NODE);
    gpio3 = DEVICE_DT_GET(GPIO3_NODE);
    gpio4 = DEVICE_DT_GET(GPIO4_NODE);
    
    /* Check devices are ready */
    if (!device_is_ready(gpio0) || !device_is_ready(gpio1) ||
        !device_is_ready(gpio2) || !device_is_ready(gpio3) ||
        !device_is_ready(gpio4)) {
        LOG_ERR("One or more GPIO devices not ready");
        return -1;
    }
    
    /* Assign port devices to pins based on their GPIO port */
    for (pin_idx = 0; pin_idx < ARRAY_SIZE(vci_pins); pin_idx++) {
        struct vci_gpio_pin *pin = &vci_pins[pin_idx];
        
        /* Determine port based on pin name */
        if (strstr(pin->name, "DC_IN") || strstr(pin->name, "BL_CTR")) {
            pin->port = gpio0;
        } else if (strstr(pin->name, "ESP32_LINK")) {
            pin->port = gpio1;
        } else if (strstr(pin->name, "SW_") || strstr(pin->name, "DOIP_OBD") ||
                   strstr(pin->name, "MOS02") || strstr(pin->name, "LCD_RST") ||
                   strstr(pin->name, "OBD1000_RST") || 
                   (pin->pin == 2 && strstr(pin->name, "SW_15_87"))) {
            pin->port = gpio2;
        } else if (strstr(pin->name, "BUZ") || strstr(pin->name, "PHY_POWER") ||
                   strstr(pin->name, "ESP32") || strstr(pin->name, "SMODE") ||
                   strstr(pin->name, "CYPWM")) {
            pin->port = gpio3;
        } else if (strstr(pin->name, "DOIP_EN") || strstr(pin->name, "KEY_")) {
            pin->port = gpio4;
        }
    }
    
    /* Initialize all GPIO pins */
    LOG_INF("Initializing GPIO pins...");
    for (pin_idx = 0; pin_idx < ARRAY_SIZE(vci_pins); pin_idx++) {
        ret = init_gpio_pin(&vci_pins[pin_idx]);
        if (ret < 0) {
            LOG_ERR("Failed to init pin %s", vci_pins[pin_idx].name);
        }
    }
    
    /* Configure button interrupts */
    LOG_INF("Configuring button interrupts...");
    
    /* OK button */
    ret = gpio_pin_interrupt_configure(gpio4, BUTTON_OK_PIN,
                                      GPIO_INT_EDGE_TO_ACTIVE);
    if (ret == 0) {
        gpio_init_callback(&button_ok_cb_data.cb, button_callback,
                          BIT(BUTTON_OK_PIN));
        gpio_add_callback(gpio4, &button_ok_cb_data.cb);
    }
    
    /* BACK button */
    ret = gpio_pin_interrupt_configure(gpio4, BUTTON_BACK_PIN,
                                      GPIO_INT_EDGE_TO_ACTIVE);
    if (ret == 0) {
        gpio_init_callback(&button_back_cb_data.cb, button_callback,
                          BIT(BUTTON_BACK_PIN));
        gpio_add_callback(gpio4, &button_back_cb_data.cb);
    }
    
    /* UP button */
    ret = gpio_pin_interrupt_configure(gpio4, BUTTON_UP_PIN,
                                      GPIO_INT_EDGE_TO_ACTIVE);
    if (ret == 0) {
        gpio_init_callback(&button_up_cb_data.cb, button_callback,
                          BIT(BUTTON_UP_PIN));
        gpio_add_callback(gpio4, &button_up_cb_data.cb);
    }
    
    /* DOWN button */
    ret = gpio_pin_interrupt_configure(gpio4, BUTTON_DOWN_PIN,
                                      GPIO_INT_EDGE_TO_ACTIVE);
    if (ret == 0) {
        gpio_init_callback(&button_down_cb_data.cb, button_callback,
                          BIT(BUTTON_DOWN_PIN));
        gpio_add_callback(gpio4, &button_down_cb_data.cb);
    }
    
    LOG_INF("Starting GPIO tests...");
    
    /* Test all outputs */
    LOG_INF("Testing output pins...");
    for (pin_idx = 0; pin_idx < ARRAY_SIZE(vci_pins); pin_idx++) {
        if (vci_pins[pin_idx].is_output) {
            test_gpio_output(&vci_pins[pin_idx]);
        }
    }
    
    /* Main loop - read inputs and blink LED */
    LOG_INF("Entering main loop - press buttons to test interrupts");
    LOG_INF("Blue LED will blink, inputs will be read every 2 seconds");
    
    while (1) {
        /* Toggle blue LED */
        gpio_pin_toggle(gpio2, LED_BLUE_PIN);
        
        /* Read all inputs */
        for (pin_idx = 0; pin_idx < ARRAY_SIZE(vci_pins); pin_idx++) {
            if (!vci_pins[pin_idx].is_output) {
                test_gpio_input(&vci_pins[pin_idx]);
            }
        }
        
        k_sleep(K_SECONDS(2));
    }
    
    return 0;
}