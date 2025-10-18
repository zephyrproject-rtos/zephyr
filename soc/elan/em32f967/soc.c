#include "soc.h"
#include <zephyr/kernel.h>
#include <zephyr/init.h>

/* Minimal SoC init for reviewer and build */
static int soc_elan_em32f967_init(void)
{
	return 0;
}

SYS_INIT(soc_elan_em32f967_init, PRE_KERNEL_1, 0);

