/*
 * Copyright (c) 2017 Jean-Paul Etienne <fractalclone@gmail.com>
 * Copyright (c) 2026 Anuj Deshpande
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>

/*
 * __soc_handle_irq is defined as .weak in the common RISC-V code so that it
 * can be re-implemented per SoC.
 *
 * There is nothing to acknowledge here: every interrupt on this SoC is level
 * sensitive and its pending state is cleared by servicing the peripheral that
 * raised it. The machine external pending bit (mip.MEIP) simply follows the
 * interrupt controller's output line.
 */
WTEXT(__soc_handle_irq)

/*
 * SOC-specific function to acknowledge the pending IRQ given via register a0.
 */
SECTION_FUNC(exception.other, __soc_handle_irq)
	ret
