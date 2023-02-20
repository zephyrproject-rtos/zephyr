/*
 * Copyright (c) 2021 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
#include <zephyr/pm/pm.h>

#define SLEEP_TIME_MS   3000

/*
 * Get button configuration from the devicetree sw0 alias. This is mandatory.
 */
#define SW0_NODE	DT_ALIAS(sw0)
#if !DT_NODE_HAS_STATUS(SW0_NODE, okay)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios,
							      {0});
#ifdef xxxx
#define irq_pin_NODE	DT_ALIAS(irqpin)



#if !DT_NODE_HAS_STATUS(irq_pin_NODE, okay)
#error "Unsupported board: irqpin devicetree alias is not defined"
#endif
#endif //xxxx

static const struct gpio_dt_spec led =
	GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);




void main(void)
{
	bool led_is_on = true;
	int ret;
	printk(" PWR->SR1 =%x \n",PWR->SR1);
	printk(" PWR->SCR =%x \n",PWR->SCR);

//PWR_LOWPOWERMODE_SHUTDOWN
	
	if ((PWR->SR1 & PWR_SR1_SBF) != RESET)		
	{
		//if we are here, it means that MCU woke up from shutdown
		PWR->SCR  |= (PWR_SCR_CSBF); // clear bit PWR_SR1_SBF
		__HAL_PWR_CLEAR_FLAG(PWR_FLAG_WUF2);
		printk(" Standby mode exit \n");
		printk(" Standby mode exit \n");
		printk(" Standby mode exit \n");
		printk(" Standby mode exit \n");
	}
	if ((PWR->SR1 & PWR_SR1_WUF1) != RESET)
	{
		//if we are here, it means that MCU woke up from shutdown
		__HAL_PWR_CLEAR_FLAG(PWR_FLAG_WUF2);
		printk(" shutdown mode exit \n");
		printk(" shutdown mode exit \n");
		printk(" shutdown mode exit \n");
		printk(" shutdown mode exit \n");
	}

	
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

	//configure_wakeup_irq_pin();


	printk("Device ready\n\n\n");
	printk("press and hold the user button to enter to Shutdown Mode when LED2 is ON \n\n");
	printk("press and hold the user button to enter to Standby Mode when LED2 is OFF \n\n");


	while (true) {
		gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
		gpio_pin_set(led.port, led.pin, (int)led_is_on);		
	
		int val_user_button = gpio_pin_get_dt(&button);
		//printk("button =%d led =%d \n",val_user_button,led_is_on);		
		if (led_is_on == false) {			
			gpio_pin_configure(led.port, led.pin, GPIO_DISCONNECTED);			
			if (val_user_button == 1){				
				printk("user_button pressed\n");
				printk("enter into Shutdown Mode \n");
				printk("release the user button to exit from Shutdown Mode \n\n");
				pm_state_force(0u, &(struct pm_state_info){PM_STATE_SOFT_OFF, 0, 0});

				k_msleep(1000);
				while(1){
					k_msleep(200);
				}				
			}
		}
		k_msleep(SLEEP_TIME_MS);
		if (led_is_on == true) {		
			gpio_pin_configure(led.port, led.pin, GPIO_DISCONNECTED);
			if (val_user_button == 1){				
				printk("user_button pressed\n");
				printk("enter into Standby Mode \n");
				printk("release the user button to exit from Standby Mode \n\n");
				pm_state_force(0u, &(struct pm_state_info){PM_STATE_STANDBY, 0, 0});
				
				k_msleep(1000);
				while(1){
					k_msleep(200);
				}				
			}
		}
		led_is_on = !led_is_on;

		

	}
	
}
