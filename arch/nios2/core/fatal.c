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

#include <nanokernel.h>
#include <arch/cpu.h>
#include <nano_private.h>

const NANO_ESF _default_esf;

FUNC_NORETURN void _NanoFatalErrorHandler(unsigned int reason,
					  const NANO_ESF *esf)
{
	ARG_UNUSED(reason);
	ARG_UNUSED(esf);

	/* STUB */
	while (1) {
		/* whee! */
	}
}

