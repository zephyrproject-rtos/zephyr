#include "common.h"

struct device *led_device[4];

struct device *button_device[4];

static struct k_work button_work;

static void button_pressed(struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	k_work_submit(&button_work);
}

void gpio_init(void)
{
	static struct gpio_callback button_cb[4]; 

        //NRF_P0->DIR |= 0x0001E000;
	//NRF_P0->OUT |= 0x0001E000;

	led_device[0] = device_get_binding(LED0_GPIO_PORT);
	gpio_pin_configure(led_device[0], LED0_GPIO_PIN, GPIO_DIR_OUT | GPIO_PUD_PULL_UP);
	gpio_pin_write(led_device[0], LED0_GPIO_PIN, 1);

	led_device[1] = device_get_binding(LED1_GPIO_PORT);
	gpio_pin_configure(led_device[1], LED1_GPIO_PIN, GPIO_DIR_OUT | GPIO_PUD_PULL_UP);
	gpio_pin_write(led_device[1], LED1_GPIO_PIN, 1);

	led_device[2] = device_get_binding(LED2_GPIO_PORT);
	gpio_pin_configure(led_device[2], LED2_GPIO_PIN, GPIO_DIR_OUT | GPIO_PUD_PULL_UP);
	gpio_pin_write(led_device[2], LED2_GPIO_PIN, 1);

	led_device[3] = device_get_binding(LED3_GPIO_PORT);
	gpio_pin_configure(led_device[3], LED3_GPIO_PIN, GPIO_DIR_OUT | GPIO_PUD_PULL_UP);
	gpio_pin_write(led_device[3], LED3_GPIO_PIN, 1);
	
	//***************************************************************************

	k_work_init(&button_work, publish);
	
	button_device[0] = device_get_binding(SW0_GPIO_NAME);
	gpio_pin_configure(button_device[0], SW0_GPIO_PIN, (GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE | GPIO_PUD_PULL_UP | GPIO_INT_DEBOUNCE | GPIO_INT_ACTIVE_LOW));
	gpio_init_callback(&button_cb[0], button_pressed, BIT(SW0_GPIO_PIN));
	gpio_add_callback(button_device[0], &button_cb[0]);
	gpio_pin_enable_callback(button_device[0], SW0_GPIO_PIN);

	button_device[1] = device_get_binding(SW1_GPIO_NAME);
	gpio_pin_configure(button_device[1], SW1_GPIO_PIN, (GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE | GPIO_PUD_PULL_UP | GPIO_INT_DEBOUNCE | GPIO_INT_ACTIVE_LOW));
	gpio_init_callback(&button_cb[1], button_pressed, BIT(SW1_GPIO_PIN));
	gpio_add_callback(button_device[1], &button_cb[1]);
	gpio_pin_enable_callback(button_device[1], SW1_GPIO_PIN);

	button_device[2] = device_get_binding(SW2_GPIO_NAME);
	gpio_pin_configure(button_device[2], SW2_GPIO_PIN, (GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE | GPIO_PUD_PULL_UP | GPIO_INT_DEBOUNCE | GPIO_INT_ACTIVE_LOW));
	gpio_init_callback(&button_cb[2], button_pressed, BIT(SW2_GPIO_PIN));
	gpio_add_callback(button_device[2], &button_cb[2]);
	gpio_pin_enable_callback(button_device[2], SW2_GPIO_PIN);

	button_device[3] = device_get_binding(SW3_GPIO_NAME);
	gpio_pin_configure(button_device[3], SW3_GPIO_PIN, (GPIO_DIR_IN | GPIO_INT | GPIO_INT_EDGE | GPIO_PUD_PULL_UP | GPIO_INT_DEBOUNCE | GPIO_INT_ACTIVE_LOW));
	gpio_init_callback(&button_cb[3], button_pressed, BIT(SW3_GPIO_PIN));
	gpio_add_callback(button_device[3], &button_cb[3]);
	gpio_pin_enable_callback(button_device[3], SW3_GPIO_PIN);
	
	//***************************************************************************
}
