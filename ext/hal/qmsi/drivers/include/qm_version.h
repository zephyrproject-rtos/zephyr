/*
 * Copyright (c) 2016, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the Intel Corporation nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL CORPORATION OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __QM_VERSION_H__
#define __QM_VERSION_H__

#include "qm_common.h"
#include "qm_soc_regs.h"

/**
 * Version number functions for API.
 *
 * @defgroup groupVersion Version
 * @{
 */

/* Create a single version number from the major, minor and patch numbers. */
#define QM_VER_API_UINT                                                        \
	((QM_VER_API_MAJOR * 10000) + (QM_VER_API_MINOR * 100) +               \
	 QM_VER_API_PATCH)

/* Create a version number string from the major, minor and patch numbers. */
#define QM_VER_API_STRING                                                      \
	QM_VER_STRINGIFY(QM_VER_API_MAJOR, QM_VER_API_MINOR, QM_VER_API_PATCH)

/**
 * Get the ROM version number.
 *
 * Reads the ROM version information from flash and returns it.
 *
 * @return uint32_t ROM version.
 */
uint32_t qm_ver_rom(void);

/**
 * @}
 */

#endif /* __QM_VERSION_H__ */
