/* SPDX-FileCopyrightText: Copyright (c) 2026 Espressif Systems (Shanghai) Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_CODE_DATA_RELOCATION)
#define LINKER_RELOCATE_SELECTIVE
#define LINKER_RELOCATE_EXTMEM_NOINIT
#include <linker_relocate.ld>
#undef LINKER_RELOCATE_EXTMEM_NOINIT
#endif
