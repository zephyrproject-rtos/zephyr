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

/*
 * @file
 * @brief Execute initialization routines referenced in .init_array section
 */

typedef void (*func_ptr)(void);

extern func_ptr __init_array_start[0];
extern func_ptr __init_array_end[0];

/**
 * @brief Execute initialization routines referenced in .init_array section
 *
 * @return N/A
 */
void __do_init_array_aux(void)
{
	for (func_ptr *func = __init_array_start;
		func < __init_array_end;
		func++) {
		(*func)();
	}
}
