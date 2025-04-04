
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
    printk("Hello \n");
    k_busy_wait(100);
    const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(watchdog));
    tr = wdt_disable(dev);
    wdt_setup(dev,3);
    return 0;
}