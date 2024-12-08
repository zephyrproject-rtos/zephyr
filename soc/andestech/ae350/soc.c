/*
 * Copyright (c) 2024 Meta Platforms
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "pma.h"

#ifdef CONFIG_SOC_PER_CORE_INIT_HOOK
void soc_per_core_init_hook(void)
{
#ifdef CONFIG_SOC_ANDES_V5_PMA
	pma_init_per_core();
#endif /* SOC_ANDES_V5_PMA */
}
#endif /* CONFIG_SOC_PER_CORE_INIT_HOOK */

#ifdef CONFIG_SOC_EARLY_INIT_HOOK
void soc_early_init_hook(void)
{
#ifdef CONFIG_SOC_ANDES_V5_PMA
	pma_init();
#endif /* CONFIG_SOC_ANDES_V5_PMA */
}
#endif /* CONFIG_SOC_EARLY_INIT_HOOK */
