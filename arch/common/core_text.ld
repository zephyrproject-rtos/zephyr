/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

*(.text)
*(".text.*")
*(".TEXT.*")
*(.stub)

#ifdef CONFIG_ARCH_USE_GNU_LINKONCE
*(.gnu.linkonce.t.*)
#endif /* CONFIG_ARCH_USE_GNU_LINKONCE */
