/*
 * Copyright (c) 2021 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/hwinfo.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/pm/pm.h>


#define SLEEP_TIME_MS   300



#if defined(CONFIG_SOC_SERIES_STM32L4X)
#include <stm32l4xx_ll_pwr.h>
#define SW0_NODE	DT_ALIAS(sw0)
#if !DT_NODE_HAS_STATUS(SW0_NODE, okay)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif

#define SEC_SRAM2_BASE			0x10000000

static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios,
							      {0});


static const struct gpio_dt_spec led =
	GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);

int check_SRAM2_integrity()
{	
	uint8_t i,*ptr_sram2_base;
	ptr_sram2_base =(uint8_t*)SEC_SRAM2_BASE;
	for(i=1;i<10;i++){
		if (*ptr_sram2_base++ != 0xA5){
			return false;
		}

	}
	return true;
};

void erase_SRAM2()
{	
	uint8_t i,*ptr_sram2_base;
	ptr_sram2_base =(uint8_t*)SEC_SRAM2_BASE;
	for(i=1;i<10;i++){
		*ptr_sram2_base++ = 0;
	}
}
#endif /* CONFIG_SOC_SERIES_STM32L4X */


void main(void)
{

#if defined(CONFIG_SOC_SERIES_STM32L4X)

	int ret,led_is_on;
	uint32_t cause;
	cause=0;
	hwinfo_get_reset_cause(&cause);
	hwinfo_clear_reset_cause();
	if ((LL_PWR_IsActiveFlag_SB() == true) && (cause == 0))
	{
		if (check_SRAM2_integrity()== true){
			LL_PWR_ClearFlag_SB();
			LL_PWR_ClearFlag_WU();
			printk(" reset from Standby mode exit \n");
			printk(" reset from Standby mode exit \n");	
		}
	} 
	if ((check_SRAM2_integrity() != true) && (cause == (RESET_PIN | RESET_BROWNOUT))){
			LL_PWR_ClearFlag_WU();
			printk(" reset from shutdown mode exit or power up \n");
			printk(" reset from shutdown mode exit or power up \n");
	}
	if ((check_SRAM2_integrity() != true) && (cause == RESET_PIN )){
			LL_PWR_ClearFlag_WU();
			printk(" reset from reset pin \n");
			printk(" reset from reset pin \n");	
	}

	erase_SRAM2();

	__ASSERT_NO_MSG(device_is_ready(led.port));
	if (!gpio_is_ready_dt(&button)) {
		printk("Error: button device %s is not ready\n",
			button.port->name);
		return;
	}

	ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
	if (ret != 0) {
		printk("Error %d: failed to configure %s pin %d\n",
			ret, button.port->name, button.pin);
		return;
	}

	printk(" Device ready :%s\n\n\n",CONFIG_BOARD);

	printk(" press and hold the user button to enter to Shutdown Mode when LED2 is OFF \n\n");
	printk(" press and hold the user button to enter to Standby Mode when LED2 is ON \n\n");

	led_is_on = true;
	while (true) {
		int i;
		for (i=1;i<10;i++){
			gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
			gpio_pin_set(led.port, led.pin, (int)led_is_on);
			int val_user_button = gpio_pin_get_dt(&button);

			if (led_is_on == false) {
				gpio_pin_configure(led.port, led.pin, GPIO_DISCONNECTED);
				if (val_user_button == 1){
					printk(" user_button pressed\n");
					printk(" enter into Shutdown Mode \n");
					printk(" release the user button to exit from Shutdown Mode \n\n");
					pm_state_force(0u, &(struct pm_state_info){PM_STATE_SOFT_OFF, 0, 0});

					k_msleep(200);
					while(1){
						/* stay in Shutdown mode until wakeup line activated */	
						k_msleep(200);
					}
				}
			}
			k_msleep(SLEEP_TIME_MS);
			if (led_is_on == true) {
				gpio_pin_configure(led.port, led.pin, GPIO_DISCONNECTED);
				if (val_user_button == 1){
					printk(" user_button pressed\n");
					printk(" enter into Standby Mode \n");
					printk(" release the user button to exit from Standby Mode \n\n");
					pm_state_force(0u, &(struct pm_state_info){PM_STATE_STANDBY, 0, 0});

					k_msleep(200);
					while(1){
						/* stay in Shutdown mode until wakeup line activated */
						k_msleep(200);
					}
				}
			}
		}
		led_is_on = !led_is_on;
	}
#else
	printk(" Standby mode and shutdown mode not supported in %s\n",CONFIG_BOARD);
#endif /* CONFIG_SOC_SERIES_STM32L4X */

}
