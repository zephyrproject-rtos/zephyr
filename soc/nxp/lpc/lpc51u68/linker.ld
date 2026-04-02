/*
 * Copyright (c) 2021 metraTec GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 /* The SDK CMIS SystemInit function sets VTOR equal to &__Vectors,
  * so we create an alias for the symbol to Zephyr's vector table
  */
 __Vectors = _vector_table;

#include <zephyr/arch/arm/cortex_m/scripts/linker.ld>
