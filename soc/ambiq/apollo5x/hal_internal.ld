/*
 * Copyright (c) 2025 Ambiq Micro Inc. <www.ambiq.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#if defined(CONFIG_SOC_APOLLO510) || defined(CONFIG_SOC_APOLLO510B)
		*(.itcm_text)
#else
		*(.dtcm_text)
#endif
