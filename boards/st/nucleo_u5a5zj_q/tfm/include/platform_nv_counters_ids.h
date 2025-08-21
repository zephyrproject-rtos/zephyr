/*
 * Copyright (c) 2023 Arm Limited. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef __PLATFORM_NV_COUNTERS_IDS_H__
#define __PLATFORM_NV_COUNTERS_IDS_H__
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum tfm_nv_counter_t {
	PLAT_NV_COUNTER_PS_0,			/* Used by PS service */
	PLAT_NV_COUNTER_PS_1,			/* Used by PS service */
	PLAT_NV_COUNTER_PS_2,			/* Used by PS service */

	PLAT_NV_COUNTER_BL2_0,			/* Used by bootloader */
	PLAT_NV_COUNTER_BL2_1,			/* Used by bootloader */
	PLAT_NV_COUNTER_BL2_2,			/* Used by bootloader */
	PLAT_NV_COUNTER_BL2_3,			/* Used by bootloader */

	PLAT_NV_COUNTER_BL1_0,			/* Used by bootloader */

	/* NS counters must be contiguous */
	PLAT_NV_COUNTER_NS_0,			/* Used by NS */
	PLAT_NV_COUNTER_NS_1,			/* Used by NS */
	PLAT_NV_COUNTER_NS_2,			/* Used by NS */

	PLAT_NV_COUNTER_MAX,
	PLAT_NV_COUNTER_BOUNDARY = UINT32_MAX	/* tfm_nv_counter_t size to 4 bytes */
};

#ifdef __cplusplus
}
#endif

#endif /* __PLATFORM_NV_COUNTERS_IDS_H__ */
