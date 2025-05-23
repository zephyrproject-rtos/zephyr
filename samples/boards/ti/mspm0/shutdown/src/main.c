#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/hwinfo.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/poweroff.h>
#include <zephyr/sys/printk.h>
#include <inttypes.h>

#define SLEEP_TIME_MS   1000
#define SW1_NODE	DT_ALIAS(sw1)
static const struct gpio_dt_spec button = GPIO_DT_SPEC_GET_OR(SW1_NODE, gpios, {0});

int main(void)
{
	int ret;
	uint32_t rst_cause;

	ret = hwinfo_get_reset_cause(&rst_cause);
	if (ret != 0) {
		return ret;
	}

	if (RESET_LOW_POWER_WAKE == rst_cause)
	{
		printk("Woke from SHUTDOWN\n");
		ret = gpio_pin_configure_dt(&button, GPIO_DISCONNECTED | GPIO_INT_WAKEUP);
		if (ret != 0) {
			printk("Unable to disable wakeup: %d\n", ret);
		}
		k_msleep(SLEEP_TIME_MS);
	}

	if (!gpio_is_ready_dt(&button)) {
		printk("Error: button device %s is not ready\n",
		       button.port->name);
		return 0;
	}

	ret = gpio_pin_configure_dt(&button, GPIO_INPUT | GPIO_INT_WAKEUP);
	if (ret != 0) {
		printk("Error %d: failed to configure %s pin %d\n",
		       ret, button.port->name, button.pin);
		return 0;
	}

	printk("Powering off\n");
	sys_poweroff();
}
