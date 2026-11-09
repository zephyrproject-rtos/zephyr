/* SPDX-FileCopyrightText: Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_CODE_DATA_RELOCATION)
*libcode_relocation_source_lib.a:(.literal .text .literal.* .text.*)
#define LINKER_RELOCATE_SELECTIVE
#define LINKER_RELOCATE_RAM_LITERAL
#define LINKER_RELOCATE_RAM_TEXT
#include <linker_relocate.ld>
#undef LINKER_RELOCATE_RAM_LITERAL
#undef LINKER_RELOCATE_RAM_TEXT
#endif
