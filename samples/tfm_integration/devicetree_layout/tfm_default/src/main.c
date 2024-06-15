/**
 * @file
 * @copyright 2024 Embeint Inc
 * @author Jordan Yates <jordan@embeint.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

int main(void)
{
	printk("Booted TF-M from devicetree partitions\n");

	for (;;) {
		printk("Uptime %6d\n", k_uptime_seconds());
		k_sleep(K_SECONDS(1));
	}
	return 0;
}
