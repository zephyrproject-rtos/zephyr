/*
 * Copyright 2024-2025 HNU-ESNL: Guoqi Xie, Chenglai Xiong, Xingyu Hu and etc.
 * Copyright 2024-2025 openEuler SIG-Zephyr
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

static void zvm_print_log(void)
{
	printk("\n");
	printk("\n");
	printk("█████████╗    ██╗     ██╗    ███╗    ███╗\n");
	printk("╚════███╔╝    ██║     ██║    ████╗  ████║\n");
	printk("   ███╔╝      ╚██╗   ██╔╝    ██╔ ████╔██║\n");
	printk("  ██╔╝         ╚██  ██╔╝     ██║ ╚██╔╝██║\n");
	printk("█████████╗      ╚████╔╝      ██║  ╚═╝ ██║\n");
}

int main(int argc, char **argv)
{
	zvm_print_log();
	return 0;
}
