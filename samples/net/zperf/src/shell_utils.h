/*
 * Copyright (c) 2016 Intel Corporation
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
#ifndef __SHELL_UTILS_H
#define __SHELL_UTILS_H

#ifdef CONFIG_NETWORKING_WITH_IPV6
#define IP_INDEX_MAX 8
#else
#define IP_INDEX_MAX 4
#endif

#define IPV4_STR_LEN_MAX 15
#define IPV4_STR_LEN_MIN 7

extern const int TIME_US[];
extern const char *TIME_US_UNIT[];
extern const int KBPS[];
extern const char *KBPS_UNIT[];
extern const int K[];
extern const char *K_UNIT[];

extern int parseIpString(char *str, int res[]);
extern void print_address(int *value);
extern void print_number(uint32_t value, const uint32_t *divisor,
		const char **units);
extern long parse_number(const char *string, const uint32_t *divisor,
		const char **units);
#endif /* __SHELL_UTILS_H */
