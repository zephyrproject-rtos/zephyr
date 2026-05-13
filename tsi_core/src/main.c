/*
 * Copyright (c) 2024-2025 TSI
 *
 */

#include <stddef.h>
#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/version.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/logging/log_output.h>

#define DATA_MAX_DLEN 8
#define LOG_MODULE_NAME plat_boot
LOG_MODULE_REGISTER(LOG_MODULE_NAME);
#define PRINT_TSI_BOOT_BANNER() printk("        Tsavorite Scalable Intelligence \n")

int main(void)
{
	/* TSI banner */
	PRINT_TSI_BOOT_BANNER();
	printf("\n");
	printf("    |||||||||||||||||||||||||||||||||||||\n");
	printf("    |||||||||||||||||||||||||||||||||||||\n");
	printf("    ||||||                          |||||\n");
	printf("    ||||||                          |||||\n");
	printf("    |||||||||||||||||   |||||       |||||\n");
	printf("               ||||||   |||||\n");
	printf("    ||||||     ||||||   |||||      ||||||\n");
	printf("     ||||||||  ||||||   |||||   ||||||||\n");
	printf("       ||||||||||||||   ||||||||||||||\n");
	printf("          |||||||||||   ||||||||||||\n");
	printf("            |||||||||   ||||||||||\n");
	printf("              |||||||   ||||||||\n");
	printf("                |||||   |||||\n");
	printf("                  |||   |||\n");
	printf("                        |\n");
	LOG_WRN("TSI Platform Software ");
	LOG_INF("Zephyr OS");
	printk("Zephyr Kernel version: %s\n", KERNEL_VERSION_STRING);
	printk("TSI Logging enabled & printk is functional\n");

	return 0;
}
