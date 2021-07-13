/*
 * Copyright (c) 2021 Synopsys
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef EMBARC_MLI_ROM_REGION
#define EMBARC_MLI_ROM_REGION		ROMABLE_REGION
#endif

#ifndef EMBARC_MLI_DATA_REGION
#define EMBARC_MLI_DATA_REGION		DCCM
#endif

#ifndef EMBARC_MLI_BSS_REGION
#define EMBARC_MLI_BSS_REGION		DCCM
#endif

#ifndef EMBARC_MLI_ZDATA_REGION
#define EMBARC_MLI_ZDATA_REGION		DCCM
#endif

#ifndef EMBARC_MLI_MODEL_P2_REGION
#define EMBARC_MLI_MODEL_P2_REGION	DCCM
#endif

#ifndef EMBARC_MLI_MODEL_REGION
#define EMBARC_MLI_MODEL_REGION		XCCM
#endif

#ifndef EMBARC_MLI_XDATA_REGION
#define EMBARC_MLI_XDATA_REGION		XCCM
#endif

#ifndef EMBARC_MLI_YDATA_REGION
#define EMBARC_MLI_YDATA_REGION		YCCM
#endif

/DISCARD/ : {
	/*.arcextmap contains debug information not used by gnu */
	*libmli*.a:*(.arcextmap*)
	*libmli*.a:*(.debug_frame*)
}

SECTION_PROLOGUE(embarc_mli_rom,,)
{
	__embarc_mli_rom_start = .;
	*libmli*.a:*(.mli_lib*)
	*libmli*.a:*(.rodata .rodata.*)
	__embarc_mli_rom_end = .;
} GROUP_DATA_LINK_IN(EMBARC_MLI_ROM_REGION, ROMABLE_REGION)
__embarc_mli_loadaddr_rom = LOADADDR(embarc_mli_rom);

SECTION_PROLOGUE(embarc_mli_data,,)
{
	__embarc_mli_data_start = .;
	*libmli*.a:*(.rodata_in_data .rodata_in_data.*)
	*libmli*.a:*(.data .data.*)
	__embarc_mli_data_end = .;
} GROUP_DATA_LINK_IN(EMBARC_MLI_DATA_REGION, ROMABLE_REGION)
__embarc_mli_loadaddr_data = LOADADDR(embarc_mli_data);

SECTION_PROLOGUE(embarc_mli_zdata,,)
{
	__embarc_mli_zdata_start = .;
	*(.Zdata)
	__embarc_mli_zdata_end = .;
} GROUP_DATA_LINK_IN(EMBARC_MLI_ZDATA_REGION, ROMABLE_REGION)
__embarc_mli_loadaddr_zdata = LOADADDR(embarc_mli_zdata);

SECTION_PROLOGUE(embarc_mli_model_p2,,)
{
	__embarc_mli_model_p2_start = .;
	*(.mli_model_p2 .mli_model_p2.*)
	__embarc_mli_model_p2_end = .;
} GROUP_DATA_LINK_IN(EMBARC_MLI_MODEL_P2_REGION, ROMABLE_REGION)
__embarc_mli_loadaddr_model_p2 = LOADADDR(embarc_mli_model_p2);

SECTION_PROLOGUE(embarc_mli_model,,)
{
	__embarc_mli_model_start = .;
	*(.mli_model .mli_model.*)
	__embarc_mli_model_end = .;
} GROUP_DATA_LINK_IN(EMBARC_MLI_MODEL_REGION, ROMABLE_REGION)
__embarc_mli_loadaddr_model = LOADADDR(embarc_mli_model);


SECTION_PROLOGUE(embarc_mli_bss,(NOLOAD),)
{
	__embarc_mli_bss_start = .;
	*libmli*.a:*(.bss .bss.*)
	__embarc_mli_bss_end = .;
} GROUP_DATA_LINK_IN(EMBARC_MLI_BSS_REGION, EMBARC_MLI_BSS_REGION)


SECTION_PROLOGUE(embarc_mli_xdata,(NOLOAD),)
{
	__embarc_mli_xdata_start = .;
	*(.Xdata*)
	__embarc_mli_xdata_end = .;
} GROUP_DATA_LINK_IN(EMBARC_MLI_XDATA_REGION, EMBARC_MLI_XDATA_REGION)

SECTION_PROLOGUE(embarc_mli_ydata,(NOLOAD),)
{
	__embarc_mli_ydata_start = .;
	*(.Ydata*)
	__embarc_mli_ydata_end = .;
} GROUP_DATA_LINK_IN(EMBARC_MLI_YDATA_REGION, EMBARC_MLI_YDATA_REGION)
