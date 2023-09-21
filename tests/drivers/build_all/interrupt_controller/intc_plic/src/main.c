/*
 * Copyright (c) 2023 Meta
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/devicetree.h>

#ifdef CONFIG_PLIC_SUPPORTS_EDGE_IRQ
BUILD_ASSERT(DT_PROP(DT_NODELABEL(plic), riscv_trigger_reg_offset) == 0x00001080,
	     "the offset should be the value defined in the board dt overlay");
#else
BUILD_ASSERT(DT_PROP(DT_NODELABEL(plic), riscv_trigger_reg_offset) == 4224,
	     "the offset should be the binding default value");
#endif /* CONFIG_PLIC_SUPPORTS_EDGE_IRQ */

int main(void)
{
	return 0;
}
