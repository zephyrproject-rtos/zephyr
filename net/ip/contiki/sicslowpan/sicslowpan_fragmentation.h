/* sicslowpan_fragmentation.h - 802.15.4 6lowpan fragmentation */

/*
 * Copyright (c) 2015 Intel Corporation
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

#ifndef SICSLOWPAN_FRAGMENTATION_H_
#define SICSLOWPAN_FRAGMENTATION_H_

#include "contiki/sicslowpan/fragmentation.h"

extern const struct fragmentation sicslowpan_fragmentation;

/**
 * \name 6lowpan dispatches
 * @{
 */
#define SICSLOWPAN_DISPATCH_FRAG1                   0xc0 /* 11000xxx */
#define SICSLOWPAN_DISPATCH_FRAGN                   0xe0 /* 11100xxx */
/** @} */

/**
 * \name The 6lowpan "headers" length
 * @{
 */

#define SICSLOWPAN_FRAG1_HDR_LEN                    4
#define SICSLOWPAN_FRAGN_HDR_LEN                    5
/** @} */

#endif /* SICSLOWPAN_FRGAMENTATION_H_ */
