#include <zephyr/device.h>
#include <zephyr/drivers/led.h>
#include <soc/arm/microchip_mec/mec172x/soc_power_debug.h>

const struct device *led = DEVICE_DT_GET(DT_ALIAS(led0));

void pm_debug_function(uint32_t debug)
{

	if (debug == PM_DEBUG_ENTER) {
		led_on(led, 0);
	} else if (debug == PM_DEBUG_EXIT) {
		led_off(led, 0);
	}
}
