/*
 * Copyright (c) 2015 Wind River Systems, Inc.
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

/**
 * @file
 * @brief Common target reboot functionality
 *
 * @details See misc/Kconfig and the reboot help for details.
 */

#ifndef _misc_reboot__h_
#define _misc_reboot__h_

#ifdef __cplusplus
extern "C" {
#endif

#define SYS_REBOOT_WARM 0
#define SYS_REBOOT_COLD 1

/**
 * @brief Reboot the system
 *
 * Reboot the system in the manner specified by @a type.  Not all architectures
 * or platforms support the various reboot types (SYS_REBOOT_COLD,
 * SYS_REBOOT_WARM).
 *
 * When successful, this routine does not return.
 *
 * @return N/A
 */

extern void sys_reboot(int type);

#ifdef __cplusplus
}
#endif

#endif /* _misc_reboot__h_ */
