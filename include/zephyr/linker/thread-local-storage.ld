/* Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries. */
/* SPDX-License-Identifier: Apache-2.0 */
/* Please keep in sync with cmake/linker_script/common/thread-local-storage.cmake */

#ifdef CONFIG_THREAD_LOCAL_STORAGE

#if defined(CONFIG_STACK_CANARIES_TLS_PREPEND)
    /* Stack canary section placed before tdata so the canary is at a
     * fixed offset (0) from the TLS base pointer.
     */
    SECTION_DATA_PROLOGUE(stack_chk,,)
    {
        KEEP(*(.stack_chk.guard));
    } GROUP_ROM_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)
#endif /* CONFIG_STACK_CANARIES_TLS_PREPEND */
	SECTION_DATA_PROLOGUE(tdata,,)
	{
		*(.tdata .tdata.* .gnu.linkonce.td.*);
	} GROUP_ROM_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	SECTION_DATA_PROLOGUE(tbss,(NOLOAD),)
	{
		*(.tbss .tbss.* .gnu.linkonce.tb.* .tcommon);
	} GROUP_ROM_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

#if defined(CONFIG_STACK_CANARIES_TLS_PREPEND)
#ifdef CONFIG_XIP
        PROVIDE(__stack_chk_start = LOADADDR(stack_chk));
#else
        PROVIDE(__stack_chk_start = ADDR(stack_chk));
#endif
	PROVIDE(__stack_chk_align = ALIGNOF(stack_chk));
	PROVIDE(__stack_chk_size = (SIZEOF(stack_chk) + __stack_chk_align - 1) & ~(__stack_chk_align - 1));
	PROVIDE(__stack_chk_end = __stack_chk_start + __stack_chk_size);
#endif /* CONFIG_STACK_CANARIES_TLS_PREPEND */

	/*
	 * These needs to be outside of the tdata/tbss
	 * sections or else they would be considered
	 * thread-local variables, and the code would use
	 * the wrong values.
	 */
#ifdef CONFIG_XIP
	/* The "master copy" of tdata should be only in flash on XIP systems */
	PROVIDE(__tdata_start = LOADADDR(tdata));
#else
	PROVIDE(__tdata_start = ADDR(tdata));
#endif
	PROVIDE(__tdata_align = ALIGNOF(tdata));
	PROVIDE(__tdata_size = (SIZEOF(tdata) + __tdata_align - 1) & ~(__tdata_align - 1));
	PROVIDE(__tdata_end = __tdata_start + __tdata_size);

	PROVIDE(__tbss_align = ALIGNOF(tbss));
	PROVIDE(__tbss_start = ADDR(tbss));
	PROVIDE(__tbss_size = (SIZEOF(tbss) + __tbss_align - 1) & ~(__tbss_align - 1));
	PROVIDE(__tbss_end = __tbss_start + __tbss_size);

#if defined(CONFIG_STACK_CANARIES_TLS_PREPEND)
	PROVIDE(__tls_start = __stack_chk_start);
	PROVIDE(__tls_size = __tbss_end - __stack_chk_start);
#else
	PROVIDE(__tls_start = __tdata_start);
	PROVIDE(__tls_size = __tbss_end - __tdata_start);
#endif /* CONFIG_STACK_CANARIES_TLS_PREPEND */
	PROVIDE(__tls_end = __tbss_end);

#endif /* CONFIG_THREAD_LOCAL_STORAGE */
