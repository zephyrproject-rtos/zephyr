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
