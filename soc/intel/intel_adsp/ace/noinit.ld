/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2026 Intel Corporation
 */
#ifdef CONFIG_LLEXT
*(.llext_heap)
*(.llext_ext_heap)
*(.llext_metadata_heap)
#endif /* CONFIG_LLEXT */
