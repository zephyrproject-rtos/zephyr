#include <zephyr/kernel.h>

#include "adsp.h"

int main(void)
{
	printk("Hello World! %s\n", CONFIG_BOARD_TARGET);

	printk("[CM33] Starting HiFi4 DSP... \n");
	dspStart();

	return 0;
}
