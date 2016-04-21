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

#include <nano_private.h>

void __reset_handler(void)
{
	/* stub */
}

void __exception_handler(void)
{
	/* stub */
}


struct vector_table {
	uint32_t reset;
	uint32_t exception;
};

/* FIXME not using CONFIG_RESET_VECTOR or CONFIG_EXCEPTION_VECTOR like we
 * should
 */
struct vector_table _vector_table _GENERIC_SECTION(.exc_vector_table) = {
	(uint32_t)__reset_handler,
	(uint32_t)__exception_handler
};

extern struct vector_table __start _ALIAS_OF(_vector_table);

