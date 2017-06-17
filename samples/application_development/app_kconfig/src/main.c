/*
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <misc/printk.h>

void main(void)
{
#if defined(CONFIG_MERGE_PATCH)
	printk("This patch will be merged!\n");
#else
	printk("This patch will not be merged!\n");
#endif
}
