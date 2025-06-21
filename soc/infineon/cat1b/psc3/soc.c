#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>

#include <cy_sysint.h>
#include <system_cat1b.h>
#include "cy_pdl.h"

void soc_early_init_hook(void)
{

	/* Initializes the system */
	SystemInit();
}
