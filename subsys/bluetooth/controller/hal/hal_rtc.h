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

#ifndef _RTC_H_
#define _RTC_H_

void rtc_init(void);
uint32_t rtc_start(void);
uint32_t rtc_stop(void);
uint32_t rtc_tick_get(void);
uint32_t rtc_compare_set(uint8_t instance, uint32_t value);

#endif /* _RTC_H_ */
