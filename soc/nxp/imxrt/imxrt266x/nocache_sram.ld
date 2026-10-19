/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * MCUX SDK drivers convert their DMA descriptor addresses with
 * MEMORY_ConvertMemoryMapAddress(), which maps PSRAM to its LLC-cached alias,
 * and the LLC is not coherent with DMA. SRAM is not remapped, so place the
 * NonCacheable sections there.
 *
 * NOLOAD, as the SDK's m_ncache: NonCacheable.init is consumed so it is not
 * orphaned, but it is not copied from ROM.
 */

GROUP_START(SRAM_NONCACHE)

SECTION_DATA_PROLOGUE(mcux_noncacheable, (NOLOAD),)
{
	. = ALIGN(32);
	__mcux_noncacheable_start = .;
	KEEP(*(NonCacheable))
	KEEP(*(NonCacheable.init))
	__mcux_noncacheable_end = ALIGN(32);
} GROUP_DATA_LINK_IN(SRAM_NONCACHE, SRAM_NONCACHE)

GROUP_END(SRAM_NONCACHE)
