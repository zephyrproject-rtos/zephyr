/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef CONFIG_USERSPACE
#ifdef CONFIG_GEN_PRIV_STACKS
	SECTION_DATA_PROLOGUE(priv_stacks_noinit,,)
	{
	z_priv_stacks_ram_start = .;
	*(".priv_stacks.noinit")
	z_priv_stacks_ram_end = .;
	} GROUP_DATA_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
#endif /* CONFIG_GEN_PRIV_STACKS */
#endif /* CONFIG_USERSPACE */
