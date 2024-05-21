#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/pm/device.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(app);


int main(void)
{
	const struct device *display_dev,*parent_dev;
	// display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
	display_dev = DEVICE_DT_GET(DT_NODELABEL(gc9x01_display));
	parent_dev =  DEVICE_DT_GET(DT_PARENT(DT_NODELABEL(gc9x01_display)));
	printf("display dev name:%s \n",display_dev->name);
	printf("display dev config: 0x%x \n",display_dev->config);
	printf("display dev api: 0x%x \n",display_dev->api);
	printf("display dev name:%s \n",parent_dev->name);
	printf("display dev config: 0x%x \n",parent_dev->config);
	printf("display dev api: 0x%x \n",parent_dev->api);
	while(1);
}