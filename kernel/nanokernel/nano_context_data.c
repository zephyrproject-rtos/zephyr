/* nano_context_data.c - VxMicro nanokernel context custom data module */

/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
DESCRIPTION
This module contains routines for accessing a context's custom data.  The
custom data is not used by the VxMicro kernel itself, and is freely available
for the context's "program" to use as it sees fit.
*/

#ifdef CONFIG_CONTEXT_CUSTOM_DATA

#include <nanok.h>

/*******************************************************************************
*
* context_custom_data_set - set context's custom data
*
* This routine sets the value of context-controlled data for the
* current task or fiber.
*
* RETURNS: N/A
*/

void context_custom_data_set(void *value /* new value */
		      )
{
	_nanokernel.current->custom_data = value;
}

/*******************************************************************************
*
* context_custom_data_get - get context's custom data
*
* This function returns the value of the context-controlled data for the
* current task or fiber.
*
* RETURNS: current handle value
*/

void *context_custom_data_get(void)
{
	return _nanokernel.current->custom_data;
}
#endif /* CONFIG_CONTEXT_CUSTOM_DATA */
