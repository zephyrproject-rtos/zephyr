/* k_pipe_buffer.h */

/*
 * Copyright (c) 1997-2010, 2014-2015 Wind River Systems, Inc.
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

#ifndef _K_PIPE_BUFFER_H
#define _K_PIPE_BUFFER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <micro_private_types.h>

void BuffInit(unsigned char *pBuffer,
	      int *piBuffSize,
	      struct _k_pipe_desc *desc);

void BuffGetFreeSpaceTotal(struct _k_pipe_desc *desc, int *piTotalFreeSpace);
void BuffGetFreeSpace(struct _k_pipe_desc *desc,
		      int *piTotalFreeSpace,
		      int *free_space_count_ptr,
		      int *free_space_post_wrap_around_ptr);

void BuffGetAvailDataTotal(struct _k_pipe_desc *desc, int *piAvailDataTotal);
void BuffGetAvailData(struct _k_pipe_desc *desc,
		      int *piAvailDataTotal,
		      int *available_data_count_ptr,
		      int *available_data_post_wrap_around_ptr);

int BuffEmpty(struct _k_pipe_desc *desc);
int BuffFull(struct _k_pipe_desc *desc);

int BuffEnQ(struct _k_pipe_desc *desc, int size, unsigned char **ppWrite);
int BuffEnQA(struct _k_pipe_desc *desc, int size, unsigned char **ppWrite,
			 int *piTransferID);
void BuffEnQA_End(struct _k_pipe_desc *desc, int iTransferID,
				  int size /* optional */);

int BuffDeQ(struct _k_pipe_desc *desc, int size, unsigned char **ppRead);
int BuffDeQA(struct _k_pipe_desc *desc, int size, unsigned char **ppRead,
			 int *piTransferID);
void BuffDeQA_End(struct _k_pipe_desc *desc, int iTransferID,
				  int size /* optional */);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _K_PIPE_BUFFER_H */
