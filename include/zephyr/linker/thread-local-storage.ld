/* SPDX-License-Identifier: Apache-2.0 */
/* Please keep in sync with cmake/linker_script/common/thread-local-storage.cmake */

#ifdef CONFIG_THREAD_LOCAL_STORAGE

	SECTION_DATA_PROLOGUE(tdata,,)
	{
		*(.tdata .tdata.* .gnu.linkonce.td.*);
	} GROUP_ROM_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

	SECTION_DATA_PROLOGUE(tbss,,)
	{
		*(.tbss .tbss.* .gnu.linkonce.tb.* .tcommon);
	} GROUP_ROM_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)

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

	PROVIDE(__tls_start = __tdata_start);
	PROVIDE(__tls_end = __tbss_end);
	PROVIDE(__tls_size = __tbss_end - __tdata_start);

#endif /* CONFIG_THREAD_LOCAL_STORAGE */
