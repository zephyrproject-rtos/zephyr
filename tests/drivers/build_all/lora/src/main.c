/*
 * Copyright (c) 2024 David Ullmann
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_LORA_FAKE
#include <zephyr/drivers/lora/lora_fake.h>
DEFINE_FFF_GLOBALS;
#endif /* CONFIG_LORA_FAKE */

int main(void)
{
	return 0;
}
