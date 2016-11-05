/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
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

#ifndef _NRF5_CLOCK_CONTROL_H_
#define _NRF5_CLOCK_CONTROL_H_

/* TODO: move all these to clock_control.h ? */

/* Define 32KHz clock source */
#ifdef CONFIG_CLOCK_CONTROL_NRF5_K32SRC_RC
#define CLOCK_CONTROL_NRF5_K32SRC 0
#endif
#ifdef CONFIG_CLOCK_CONTROL_NRF5_K32SRC_XTAL
#define CLOCK_CONTROL_NRF5_K32SRC 1
#endif

/* Define 32KHz clock accuracy */
#ifdef CONFIG_CLOCK_CONTROL_NRF5_K32SRC_500PPM
#define CLOCK_CONTROL_NRF5_K32SRC_ACCURACY 0
#endif
#ifdef CONFIG_CLOCK_CONTROL_NRF5_K32SRC_250PPM
#define CLOCK_CONTROL_NRF5_K32SRC_ACCURACY 1
#endif
#ifdef CONFIG_CLOCK_CONTROL_NRF5_K32SRC_150PPM
#define CLOCK_CONTROL_NRF5_K32SRC_ACCURACY 2
#endif
#ifdef CONFIG_CLOCK_CONTROL_NRF5_K32SRC_100PPM
#define CLOCK_CONTROL_NRF5_K32SRC_ACCURACY 3
#endif
#ifdef CONFIG_CLOCK_CONTROL_NRF5_K32SRC_75PPM
#define CLOCK_CONTROL_NRF5_K32SRC_ACCURACY 4
#endif
#ifdef CONFIG_CLOCK_CONTROL_NRF5_K32SRC_50PPM
#define CLOCK_CONTROL_NRF5_K32SRC_ACCURACY 5
#endif
#ifdef CONFIG_CLOCK_CONTROL_NRF5_K32SRC_30PPM
#define CLOCK_CONTROL_NRF5_K32SRC_ACCURACY 6
#endif
#ifdef CONFIG_CLOCK_CONTROL_NRF5_K32SRC_20PPM
#define CLOCK_CONTROL_NRF5_K32SRC_ACCURACY 7
#endif

#endif /* _NRF5_CLOCK_CONTROL_H_ */
