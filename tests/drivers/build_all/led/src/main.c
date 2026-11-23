/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_LED_FAKE
#include <zephyr/drivers/led/led_fake.h>
DEFINE_FFF_GLOBALS;
#endif /* CONFIG_LED_FAKE */

int main(void)
{
	return 0;
}
