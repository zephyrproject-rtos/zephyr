/*
 * Copyright (c) 2016 Linaro Ltd.
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

#include <zephyr.h>
#include <errno.h>
#include <tc_util.h>

void main(void)
{
	volatile long long a = 100;
	volatile long long b = 3;
	volatile long long c = a / b;
	int rv = TC_PASS;

	if (c != 33) {
		rv = TC_FAIL;
	}

	TC_END_RESULT(rv);
	TC_END_REPORT(rv);
}
