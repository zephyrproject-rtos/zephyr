/* k_pipe_buffer.h */

/*
 * Copyright (c) 1997-2010, 2014-2015 Wind River Systems, Inc.
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

#ifndef _K_PIPE_BUFFER_H
#define _K_PIPE_BUFFER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <micro_private_types.h>

void BuffInit(unsigned char *pBuffer, int *piBuffSize, struct _k_pipe_desc *desc);

void BuffGetFreeSpaceTotal(struct _k_pipe_desc *desc, int *piTotalFreeSpace);
void BuffGetFreeSpace(struct _k_pipe_desc *desc, int *piTotalFreeSpace,
					  int *free_space_count_ptr, int *piFreeSpaceAWA);

void BuffGetAvailDataTotal(struct _k_pipe_desc *desc, int *piAvailDataTotal);
void BuffGetAvailData(struct _k_pipe_desc *desc, int *piAvailDataTotal,
					  int *piAvailDataCont, int *piAvailDataAWA);

int BuffEmpty(struct _k_pipe_desc *desc);
int BuffFull(struct _k_pipe_desc *desc);

int BuffEnQ(struct _k_pipe_desc *desc, int iSize, unsigned char **ppWrite);
int BuffEnQA(struct _k_pipe_desc *desc, int iSize, unsigned char **ppWrite,
			 int *piTransferID);
void BuffEnQA_End(struct _k_pipe_desc *desc, int iTransferID,
				  int iSize /* optional */);

int BuffDeQ(struct _k_pipe_desc *desc, int iSize, unsigned char **ppRead);
int BuffDeQA(struct _k_pipe_desc *desc, int iSize, unsigned char **ppRead,
			 int *piTransferID);
void BuffDeQA_End(struct _k_pipe_desc *desc, int iTransferID,
				  int iSize /* optional */);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _K_PIPE_BUFFER_H */
