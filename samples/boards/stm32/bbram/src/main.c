/*
 * Copyright (c) 2023 STMicroelectronics
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/bbram.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>
//#include <zephyr/pm/pm.h>
//#include <zephyr/sys/poweroff.h>
//#include <stm32_ll_pwr.h>
#include <zephyr/device.h>
#include <zephyr/drivers/bbram.h>
#include <string.h>
//#include <zephyr/ztest.h>

#if !defined(CONFIG_SOC_SERIES_STM32H7X)
#error Not implemented for other series
#endif  /* CONFIG_SOC_SERIES_STM32H7X */

#if DT_HAS_COMPAT_STATUS_OKAY(st_stm32_bbram)
#define BACKUP_DEV_COMPAT st_stm32_bbram
#endif

#if DT_NODE_HAS_STATUS(DT_ALIAS(bbram_0), okay)
#define PWM_DEV_NODE DT_ALIAS(bbram_0)
#elif DT_HAS_COMPAT_STATUS_OKAY(st_stm32_bbram)
#define BBRAM_DEV_NODE DT_INST(0, st_stm32_bbram)
#endif

#define BBRAM_NODELABEL DT_NODELABEL(bbram)
//#define BBRAM_NODELABEL DT_NODELABEL(backup_regs)
#define BBRAM_SIZE DT_PROP(BBRAM_NODELABEL, size)


//const struct device *const dev = DEVICE_DT_GET(BBRAM_NODELABEL);
uint8_t buffer[BBRAM_SIZE];
//uint8_t buffer[100];










int main(void)
{
	int ret;
	//uint32_t cause;
	size_t size;
	//const struct device *const dev = DEVICE_DT_GET_ONE(BACKUP_DEV_COMPAT);
	
	//const struct device *const dev = DT_CHILD(DT_NODELABEL(bbram));
	//const struct device *const dev = DEVICE_DT_GET(BBRAM_DEV_NODE);
	const struct device *const dev = DEVICE_DT_GET_ANY(st_stm32_bbram);
	//const struct device *const dev = DEVICE_DT_GET_ANY(st_stm32_rtc);

	printf("Hello World!aa %s\n", CONFIG_BOARD);
//#if DT_NODE_EXISTS(backup_regs)
//#if DT_NODE_EXISTS(bbram)
#if DT_NODE_EXISTS(BBRAM_NODELABEL) 
	//printf("BBRAM_NODELABEL %s\n", BBRAM_NODELABEL);
	printf("BBRAM_SIZE %d \n", (int)BBRAM_SIZE );

	//zassert_true(device_is_ready(dev), "Device is not ready");

	//bbram_get_size(dev, &size);
	//bbram_read(dev, 0, BBRAM_SIZE, buffer);
        ret=device_is_ready(dev);
	if (ret==true){
	printk("Device ready: %s\n\n\n", CONFIG_BOARD);
	}
	//printk("Device ready: %s\n\n\n", CONFIG_BOARD);


#endif //DT_NODE_EXISTS(backup_regs)

	return 0;
}
