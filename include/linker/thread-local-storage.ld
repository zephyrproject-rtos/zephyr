/* SPDX-License-Identifier: Apache-2.0 */

#ifdef CONFIG_THREAD_LOCAL_STORAGE

	SECTION_DATA_PROLOGUE(tdata,,)
	{
		*(.tdata .tdata.* .gnu.linkonce.td.*);
	} GROUP_LINK_IN(ROMABLE_REGION)

	SECTION_DATA_PROLOGUE(tbss,,)
	{
		*(.tbss .tbss.* .gnu.linkonce.tb.* .tcommon);
	} GROUP_LINK_IN(ROMABLE_REGION)

	/*
	 * These needs to be outside of the tdata/tbss
	 * sections or else they would be considered
	 * thread-local variables, and the code would use
	 * the wrong values.
	 */
	PROVIDE(__tdata_start = LOADADDR(tdata));
	PROVIDE(__tdata_size = SIZEOF(tdata));
	PROVIDE(__tdata_end = __tdata_start + __tdata_size);
	PROVIDE(__tdata_align = ALIGNOF(tdata));

	PROVIDE(__tbss_start = LOADADDR(tbss));
	PROVIDE(__tbss_size = SIZEOF(tbss));
	PROVIDE(__tbss_end = __tbss_start + __tbss_size);
	PROVIDE(__tbss_align = ALIGNOF(tbss));

	PROVIDE(__tls_start = __tdata_start);
	PROVIDE(__tls_end = __tbss_end);
	PROVIDE(__tls_size = __tbss_end - __tdata_start);

#endif /* CONFIG_THREAD_LOCAL_STORAGE */
