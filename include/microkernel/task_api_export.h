/* task macros/types/values exportable to user-space */

/*
 * Copyright (c) 2015 Wind River Systems, Inc.
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

#ifndef _task_api_export__h_
#define _task_api_export__h_

#define TASK_START 0
#define TASK_ABORT 1
#define TASK_SUSPEND 2
#define TASK_RESUME 3

#define TASK_GROUP_START 0
#define TASK_GROUP_ABORT 1
#define TASK_GROUP_SUSPEND 2
#define TASK_GROUP_RESUME 3

#define task_abort(t) _task_ioctl(t, TASK_ABORT)
#define task_suspend(t) _task_ioctl(t, TASK_SUSPEND)
#define task_resume(t) _task_ioctl(t, TASK_RESUME)
#define task_group_abort(g) _task_group_ioctl(g, TASK_GROUP_ABORT)
#define task_group_suspend(g) _task_group_ioctl(g, TASK_GROUP_SUSPEND)
#define task_group_resume(g) _task_group_ioctl(g, TASK_GROUP_RESUME)

#define task_start(t) _task_ioctl(t, TASK_START)
#define task_group_start(g) _task_group_ioctl(g, TASK_GROUP_START)

#endif /* _task_api_export__h_ */
