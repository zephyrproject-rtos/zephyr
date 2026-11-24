/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * Compiled into both the app and dup_lib targets. RELOC_MULTI_IN_LIB is only
 * defined for dup_lib, so each object provides a different function and both
 * objects have to be read to relocate both functions.
 */
#ifdef RELOC_MULTI_IN_LIB
void reloc_multi_lib_func(void)
{
}
#else
void reloc_multi_app_func(void)
{
}
#endif
