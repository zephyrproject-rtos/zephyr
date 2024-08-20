
#include <stdio.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/watchdog.h>
#include <zephyr/sys/printk.h>
#include <stdbool.h>

int main()
{
	// const struct device *wdt = DEVICE_DT_GET(DT_ALIAS(watchdog));
    int tr;
    const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(watchdog));
    printk("Hello \n");
    tr = wdt_disable(dev);
    wdt_setup(dev,3);
    return 0;
}