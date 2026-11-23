/*
 * Copyright (c) 2026 sevenlab engineering GmbH

 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SILABS_COMMON_SOC_COMMON_H_
#define _SILABS_COMMON_SOC_COMMON_H_

#include <soc.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_DT_HAS_SILABS_SERIES0_CMU_ENABLED
void silabs_gecko_init_clocks(void);
#endif

#ifdef __cplusplus
}
#endif

#endif /* _SILABS_COMMON_SOC_COMMON_H_ */
