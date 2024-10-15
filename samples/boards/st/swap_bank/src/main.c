/*
 * Copyright (c) 2024 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This sample provides a description of how to swap
 * between the two banks of FLASH of a of a stm32 mcu.
 * Build and download the application for each flash bank
 * by selecting the 'SWAP_FLASH_BANKS' constant
 * and change flash bank by pressing on the button.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>

#include <soc.h>

#ifndef FLASH_BANK_BOTH
#error "No dual bank flash on this target"
#endif

/*
 * Comment this line and download zephyr.bin at address of the bank1 (0x8000000)
 * UnComment this line and download zephyr.bin at addressof the bank2 (0x8100000)
 */
#define SWAP_FLASH_BANKS

#ifdef SWAP_FLASH_BANKS
#define LED_NODE DT_ALIAS(led0)
#else
#define LED_NODE DT_ALIAS(led1)
#endif

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS 400

const struct device *flash_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_flash_controller));

const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED_NODE, gpios);
#define SW0_NODE DT_ALIAS(sw0)
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios, {0});

FLASH_OBProgramInitTypeDef OBInit;
bool pressed;
static struct gpio_callback button_cb_data;

void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	pressed = true;
}

int main(void)
{
	if (!device_is_ready(flash_dev)) {
		printk("%s: device not ready.\n", flash_dev->name);
		return -1;
	}

	if (!gpio_is_ready_dt(&led)) {
		printk("LED not ready.\n");
		return -1;
	}

	if (!gpio_is_ready_dt(&button)) {
		printk("Button not ready\n");
		return -1;
	}

	pressed = true;

	if (gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE) != 0) {
		printk("LED not configured.\n");
		return -1;
	}

	if (gpio_pin_configure_dt(&button, GPIO_INPUT) != 0) {
		printk("Button not configured.\n");
		return -1;
	}
	gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);
	gpio_init_callback(&button_cb_data, button_pressed, BIT(button.pin));
	gpio_add_callback(button.port, &button_cb_data);

	/* Unlock the User Flash area */
	HAL_FLASH_Unlock();
	HAL_FLASH_OB_Unlock();

	HAL_FLASHEx_OBGetConfig(&OBInit);
	if ((OBInit.USERConfig & OB_SWAP_BANK_ENABLE) == OB_SWAP_BANK_DISABLE) {
		printk("Running on bank 1 : ");
	} else {
		printk("Running on bank 2 : ");
	}
	printk("Press button to swap banks\n\n");
	while (1) {
		if (pressed) {
			/* Go swapping flash banks */
			if ((OBInit.USERConfig & OB_SWAP_BANK_ENABLE) == OB_SWAP_BANK_DISABLE) {
				/* Swap to bank2 */
				OBInit.OptionType = OPTIONBYTE_USER;
				/* Set OB SWAP_BANK_OPT to swap Bank2 */
				OBInit.USERType = OB_USER_SWAP_BANK;
				OBInit.USERConfig = OB_SWAP_BANK_ENABLE;
			} else {
				/* Swap to bank1 */
				OBInit.OptionType = OPTIONBYTE_USER;
				/* Set OB SWAP_BANK_OPT to swap Bank1 */
				OBInit.USERType = OB_USER_SWAP_BANK;
				OBInit.USERConfig = OB_SWAP_BANK_DISABLE;
			}

			HAL_FLASHEx_OBProgram(&OBInit);
			/* Launch Option bytes loading */
			HAL_FLASH_OB_Launch();

			pressed = false;
		} else {
			/* Continue to execute the code on this bank */
			gpio_pin_toggle_dt(&led);

			k_msleep(SLEEP_TIME_MS);
		}
	}

	return 0;
}
