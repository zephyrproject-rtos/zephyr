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

#include <ctype.h>
#include <misc/printk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <zephyr.h>

#include "shell_utils.h"

#ifdef CONFIG_NETWORKING_WITH_IPV6
#define BASE  16
#define SEPARATOR ':'
#else
#define BASE  10
#define SEPARATOR '.'
#endif

int parseIpString(char *str, int res[])
{
	uint32_t index = 0;

	if (str == NULL) {
		printk("ERROR! null str detected\n");
		return -1;
	}

#ifdef CONFIG_NETWORKING_WITH_IPV4
	if (strlen(str) > IPV4_STR_LEN_MAX
			|| strlen(str) < IPV4_STR_LEN_MIN) {
		printk("ERROR! invalid IP address\n");
		return -1;
	}
#endif

	for (int i = 0; i < IP_INDEX_MAX; i++)
		res[i] = 0;

	while (*str) {
		if (isdigit((unsigned char)*str)) {
			res[index] *= BASE;
			res[index] += *str - '0';
#ifdef CONFIG_NETWORKING_WITH_IPV4
			if (res[index] > 255) {
				printk("ERROR! invalid IP address\n");
				return -1;
			}
#endif
		}
#ifdef CONFIG_NETWORKING_WITH_IPV6
		else if (isalpha((unsigned char)*str)) {
			int digit = *str - (isupper(*str) ? 'A' - 10 : 'a' - 10);

			if (digit > BASE) {
				printk("ERROR! invalid IP address\n");
				return -1;
			}
			res[index] *= BASE;
			res[index] += digit;
		}
#endif
		else if ((unsigned char) *str == SEPARATOR) {
#ifdef CONFIG_NETWORKING_WITH_IPV6
			if (str[1] == SEPARATOR) {
				printk("ERROR! I am sorry but I am not yet able to understand "
						"the :: syntax!!\n");
				printk("ERROR! Please expand your @IP or enhance me!!\n");
				return -1;
			}
#endif
			index++;
			if (index >= IP_INDEX_MAX) {
				printk("ERROR! invalid IP address\n");
				return -1;
			}
		} else {
			printk("ERROR! invalid IP address format\n");
			return -1;
		}
		str++;
	}
	return 0;
}

void print_address(int *value)
{
#ifdef CONFIG_NETWORKING_WITH_IPV6
	printk("%x:%x:%x:%x:%x:%x:%x:%x",
			value[0], value[1], value[2], value[3],
			value[4], value[5], value[6], value[7]);
#else
	printk("%d.%d.%d.%d", value[0], value[1], value[2], value[3]);
#endif
}

const int TIME_US[] = { 60 * 1000 * 1000, 1000 * 1000, 1000, 0 };
const char *TIME_US_UNIT[] = { "m", "s", "ms", "us" };
const int KBPS[] = { 1024, 0 };
const char *KBPS_UNIT[] = { "Mbps", "Kbps" };
const int K[] = { 1024 * 1024, 1024, 0 };
const char *K_UNIT[] = { "M", "K", "" };

void print_number(uint32_t value, const uint32_t *divisor,
	const char **units)
{
	const char **unit;
	const uint32_t *div;
	uint32_t dec, radix;

	unit = units;
	div = divisor;

	while (value < *div) {
		div++;
		unit++;
	}

	if (*div != 0) {
		radix = value / *div;
		dec = (value % *div) * 100 / *div;
		printk("%u.%s%u %s", radix, (dec < 10) ? "0" : "", dec, *unit);
	} else {
		printk("%u %s", value, *unit);
	}
}

long parse_number(const char *string, const uint32_t *divisor,
		const char **units)
{
	const char **unit;
	const uint32_t *div;
	char *suffix;
	long dec;
	int cmp;

	dec = strtoul(string, &suffix, 10);
	unit = units;
	div = divisor;

	do {
		cmp = strncasecmp(suffix, *unit++, 1);
	} while (cmp != 0 && *++div != 0);

	return (*div == 0) ? dec : dec * *div;
}
