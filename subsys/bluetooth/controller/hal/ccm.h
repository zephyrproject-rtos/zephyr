/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 * Copyright (c) 2016 Vinayak Kariappa Chettimada
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

#ifndef _CCM_H_
#define _CCM_H_

#include <toolchain.h>

struct ccm {
	uint8_t key[16];
	uint64_t counter;
	uint8_t direction:1;
	uint8_t resv1:7;
	uint8_t iv[8];
} __packed;

#endif /* _CCM_H_ */
