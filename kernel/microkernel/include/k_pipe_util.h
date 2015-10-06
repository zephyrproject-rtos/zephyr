/* k_pipe_util.h */

/*
 * Copyright (c) 1997-2012, 2014 Wind River Systems, Inc.
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

#ifndef _K_PIPE_UTIL_H
#define _K_PIPE_UTIL_H

/* high-level behavior of the pipe service */

#define CANCEL_TIMERS

typedef uint32_t REQ_TYPE;
#define _ALLREQ ((REQ_TYPE)0x0000FF00)
#define _SYNCREQ ((REQ_TYPE)0x00000100)
#define _SYNCREQL ((REQ_TYPE)0x00000200)
#define _ASYNCREQ ((REQ_TYPE)0x00000400)

typedef uint32_t TIME_TYPE;
#define _ALLTIME ((TIME_TYPE)0x00FF0000)
#define _TIME_NB ((TIME_TYPE)0x00010000)
#define _TIME_B  ((TIME_TYPE)0x00020000)
#define _TIME_BT ((TIME_TYPE)0x00040000)


extern void _k_pipe_process(struct _k_pipe_struct *pipe_ptr,
					 struct k_args *writer_ptr, struct k_args *reader_ptr);

extern void mycopypacket(struct k_args **out, struct k_args *in);

int CalcFreeReaderSpace(struct k_args *pReaderList);
int CalcAvailWriterData(struct k_args *pWriterList);

void DeListWaiter(struct k_args *pReqProc);
void myfreetimer(struct k_timer **ppTimer);

K_PIPE_OPTION _k_pipe_option_get(K_ARGS_ARGS *args);
void _k_pipe_option_set(K_ARGS_ARGS *args, K_PIPE_OPTION option);
REQ_TYPE _k_pipe_request_type_get(K_ARGS_ARGS *args);
void _k_pipe_request_type_set(K_ARGS_ARGS *args, REQ_TYPE req_type);

void _k_pipe_request_status_set(struct _pipe_xfer_req_arg *pipe_xfer_req,
					PIPE_REQUEST_STATUS status);

TIME_TYPE _k_pipe_time_type_get(K_ARGS_ARGS *args);
void _k_pipe_time_type_set(K_ARGS_ARGS *args, TIME_TYPE TimeType);

#endif /* _K_PIPE_UTIL_H */
