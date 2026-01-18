/*
 * Copyright 2025 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>
#include <zephyr/platform/hooks.h>
#include <soc.h>

#if CONFIG_BOARD_EARLY_INIT_HOOK
void board_early_init_hook(void)
{
#if defined(CONFIG_CLEAR_DESTRUCTIVE_RESET_FLAG) && (CONFIG_CLEAR_DESTRUCTIVE_RESET_FLAG)
	MC_RGM->DES = CONFIG_CLEAR_DESTRUCTIVE_RESET_FLAG;
#endif /* CONFIG_CLEAR_DESTRUCTIVE_RESET_FLAG */

#if defined(CONFIG_CLEAR_FUNCTIONAL_RESET_FLAG) && (CONFIG_CLEAR_FUNCTIONAL_RESET_FLAG)
	MC_RGM->FES = CONFIG_CLEAR_FUNCTIONAL_RESET_FLAG;
#endif
}

#endif /* CONFIG_BOARD_EARLY_INIT_HOOK */
