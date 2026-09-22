/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2026 Intel Corporation
 */
#if defined(CONFIG_LLEXT) && !defined(CONFIG_LLEXT_CUSTOM_HEAP_PLACEMENT)
*(.llext_heap)
*(.llext_ext_heap)
*(.llext_metadata_heap)
#endif /* CONFIG_LLEXT && !CONFIG_LLEXT_CUSTOM_HEAP_PLACEMENT */
