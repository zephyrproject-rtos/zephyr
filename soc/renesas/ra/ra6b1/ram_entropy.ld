/*
 * SPDX-FileCopyrightText: 2026 Renesas Electronics Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
SECTION_DATA_PROLOGUE(__ram_entropy$$,(NOLOAD),)
{
	. = ORIGIN(ENTROPY);

	. = ALIGN(4);
	__iid_es_start__ = .;
	*(.iid.es)
	. = ALIGN(4);
	__iid_es_end__ = .;
	. = ALIGN(4);
	__iid_data_start__ = .;
	*(.iid.data)
	. = ALIGN(4);
	__iid_data_end__ = .;
} GROUP_LINK_IN(ENTROPY)
