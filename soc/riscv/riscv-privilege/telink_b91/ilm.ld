SECTION_DATA_PROLOGUE(aes_data,,)
{
	. = ALIGN(4);
	*(.aes_data)
	*(".aes_data.*")

	PROVIDE (_AES_DATA_VMA_END = .);
	PROVIDE (_AES_DATA_VMA_START = ADDR(aes_data));
} GROUP_DATA_LINK_IN(RAMILM, ROMABLE_REGION)

SECTION_DATA_PROLOGUE(retention_data,,)
{
	. = ALIGN(4);
	*(.retention_data)
	*(".retention_data.*")

	PROVIDE (_RETENTION_DATA_VMA_END = .);
	PROVIDE (_RETENTION_DATA_VMA_START = ADDR(retention_data));
	PROVIDE (_RETENTION_DATA_LMA_START = LOADADDR(retention_data));
} GROUP_DATA_LINK_IN(RAMILM, ROMABLE_REGION)

SECTION_DATA_PROLOGUE(ram_code,,)
{
	. = ALIGN(4);
	*(.ram_code)
	*(".ram_code.*")

	PROVIDE (_RAMCODE_VMA_END = .);
	PROVIDE (_RAMCODE_VMA_START = ADDR(ram_code));
	PROVIDE (_RAMCODE_LMA_START = LOADADDR(ram_code));
} GROUP_DATA_LINK_IN(RAMILM, ROMABLE_REGION)

SECTION_DATA_PROLOGUE(noinit_ilm, (NOLOAD),)
{
	. = ALIGN(4);
	*(.noinit)
	*(".noinit.*")
} GROUP_DATA_LINK_IN(RAMILM, ROMABLE_REGION)
