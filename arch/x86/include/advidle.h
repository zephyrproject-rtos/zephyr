/* advidle.h - header file for custom advanced idle manager */

/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
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
/*
DESCRIPTION
This header file specifies the custom advanced idle management interface.
All of the APIs declared here must be supplied by the custom advanced idle
management system, namely the _AdvIdleCheckSleep(), _AdvIdleFunc()
and _AdvIdleStart() functions.
 */

#ifndef __INCadvidle
#define __INCadvidle

#ifdef CONFIG_ADVANCED_IDLE

/*
 * @brief Determine if advanced sleep has occurred
 *
 * This routine checks if the system is recovering from advanced
 * sleep or cold booting.
 *
 * @return 0 if the system is cold booting on a non-zero
 * value if the system is recovering from advanced sleep.
 */

extern int _AdvIdleCheckSleep(void);

/*
 * @brief Continue kernel start-up or awaken kernel from sleep
 *
 * This routine checks if the system is recovering from advanced sleep and
 * either continues the kernel's cold boot sequence at _Cstart or resumes
 * kernel operation at the point it went to sleep; in the latter case, control
 * passes to the _AdvIdleFunc() that put the system to sleep, which then
 * finishes executing.
 *
 * @param _Cstart the address of the _Cstart function
 * @param _gdt the address of the global descriptor table in RAM
 * @param _GlobalTss the address of the TSS descriptor
 *
 * @return does not return to caller
 */

extern void _AdvIdleStart(void (*_Cstart)(void), void *_gdt, void *_GlobalTss);

/*
 * @brief Perform advanced sleep
 *
 * This routine checks if the upcoming kernel idle interval is sufficient to
 * justify entering advanced sleep mode. If it is, the routine puts the system
 * to sleep and then later allows it to resume processing; if not, the routine
 * returns immediately without sleeping.
 *
 * @param ticks the upcoming kernel idle time
 *
 * @return  non-zero if advanced sleep occurred; otherwise zero
 */

extern int _AdvIdleFunc(int32_t ticks);

#endif /* CONFIG_ADVANCED_IDLE */

#endif /* __INCadvidle */
