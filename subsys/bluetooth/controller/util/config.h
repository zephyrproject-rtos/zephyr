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

#ifndef _CONFIG_H_
#define _CONFIG_H_

#define MAYFLY_CALL_ID_0       0
#define MAYFLY_CALL_ID_1       1
#define MAYFLY_CALL_ID_2       2
#define MAYFLY_CALL_ID_PROGRAM 3
#define MAYFLY_CALLER_COUNT    4
#define MAYFLY_CALLEE_COUNT    4

#define TICKER_MAYFLY_CALL_ID_TRIGGER MAYFLY_CALL_ID_0
#define TICKER_MAYFLY_CALL_ID_WORKER0 MAYFLY_CALL_ID_0
#define TICKER_MAYFLY_CALL_ID_WORKER1 MAYFLY_CALL_ID_2
#define TICKER_MAYFLY_CALL_ID_JOB0    MAYFLY_CALL_ID_1
#define TICKER_MAYFLY_CALL_ID_JOB1    MAYFLY_CALL_ID_2
#define TICKER_MAYFLY_CALL_ID_PROGRAM MAYFLY_CALL_ID_PROGRAM

#define TICKER_MAYFLY_CALL_ID_WORKER0_PRIO 0
#define TICKER_MAYFLY_CALL_ID_WORKER1_PRIO 1
#define TICKER_MAYFLY_CALL_ID_JOB0_PRIO    0
#define TICKER_MAYFLY_CALL_ID_JOB1_PRIO    1

#endif /* _CONFIG_H_ */
