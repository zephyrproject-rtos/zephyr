/*
 * Copyright (c) 2024 TOKITA Hiroshi
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_REGULATOR_FAKE
#include <zephyr/drivers/regulator/fake.h>
DEFINE_FFF_GLOBALS;
#endif /* CONFIG_REGULATOR_FAKE */

int main(void)
{
	return 0;
}
