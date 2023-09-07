/*
 * Copyright 2023 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/syscall_handler.h>

#if defined(CONFIG_ASSEMBLER_ISA_THUMB2) && !defined(CONFIG_DYNAMIC_OBJECTS)
/*
 * When kernel objects are not allocated at run-time, these two functions
 * definitions won't be present during intermediate build phases, leading the
 * linker to generate veneers under the assumption these are Arm functions.
 * The veneers will be eventually removed from the final image because these
 * functions are now defined, causing a size mismatch between the intermediate
 * and final binaries. Hence, convince the linker these are Thumb functions by
 * providing a weak definition during intermediate build.
 */
__weak struct z_object *z_object_find(const void *obj)
{
	return NULL;
}

__weak void z_object_wordlist_foreach(_wordlist_cb_func_t func, void *context)
{

}
#endif
