/* Copied from linker.ld */

GROUP_START(CCM)

	SECTION_PROLOGUE(_CCM_BSS_SECTION_NAME, (NOLOAD),SUBALIGN(4))
	{
		__ccm_start = .;
		__ccm_bss_start = .;
		*(.ccm_bss)
		*(".ccm_bss.*")
		__ccm_bss_end = .;
	} GROUP_LINK_IN(CCM)

	SECTION_PROLOGUE(_CCM_NOINIT_SECTION_NAME, (NOLOAD),SUBALIGN(4))
	{
		__ccm_noinit_start = .;
		*(.ccm_noinit)
		*(".ccm_noinit.*")
		__ccm_noinit_end = .;
	} GROUP_LINK_IN(CCM)

	SECTION_PROLOGUE(_CCM_DATA_SECTION_NAME,,SUBALIGN(4))
	{
		__ccm_data_start = .;
		*(.ccm_data)
		*(".ccm_data.*")
		__ccm_data_end = .;
	} GROUP_LINK_IN(CCM AT> ROMABLE_REGION)

	__ccm_end = .;

	__ccm_data_rom_start = LOADADDR(_CCM_DATA_SECTION_NAME);

GROUP_END(CCM)