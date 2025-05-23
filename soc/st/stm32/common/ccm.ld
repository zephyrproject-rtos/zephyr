/* Copied from linker.ld */

GROUP_START(CCM)

	SECTION_PROLOGUE(_CCM_BSS_SECTION_NAME, (NOLOAD),SUBALIGN(4))
	{
		__ccm_start = .;
		__ccm_bss_start = .;
		*(.ccm_bss)
		*(".ccm_bss.*")
		__ccm_bss_end = .;
	} GROUP_LINK_IN(LINKER_DT_NODE_REGION_NAME(DT_CHOSEN(zephyr_ccm)))

	SECTION_PROLOGUE(_CCM_NOINIT_SECTION_NAME, (NOLOAD),SUBALIGN(4))
	{
		__ccm_noinit_start = .;
		*(.ccm_noinit)
		*(".ccm_noinit.*")
		__ccm_noinit_end = .;
	} GROUP_LINK_IN(LINKER_DT_NODE_REGION_NAME(DT_CHOSEN(zephyr_ccm)))

	SECTION_PROLOGUE(_CCM_DATA_SECTION_NAME,,SUBALIGN(4))
	{
		__ccm_data_start = .;
		*(.ccm_data)
		*(".ccm_data.*")
		__ccm_data_end = .;
	} GROUP_LINK_IN(LINKER_DT_NODE_REGION_NAME(DT_CHOSEN(zephyr_ccm)) AT> ROMABLE_REGION)

	__ccm_end = .;

	__ccm_data_load_start = LOADADDR(_CCM_DATA_SECTION_NAME);

GROUP_END(CCM)
