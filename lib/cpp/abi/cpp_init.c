/*
 * Copyright (c) 2021 Synopsys, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Author: Evgeniy Paltsev
 */

#ifdef CONFIG_CPP_STATIC_INIT_GNU

void __do_global_ctors_aux(void);
void __do_init_array_aux(void);

void z_cpp_init_static(void)
{
	__do_global_ctors_aux();
	__do_init_array_aux();
}

#else

#ifdef __CCAC__
void __do_global_ctors_aux(void);

void z_cpp_init_static(void)
{
	__do_global_ctors_aux();
}
#endif /* __CCAC__ */

#endif /* CONFIG_CPP_STATIC_INIT_GNU */
