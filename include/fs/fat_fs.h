/*
 * Copyright (c) 2016 Intel Corporation.
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

#ifndef _FAT_FS_H_
#define _FAT_FS_H_

#include <sys/types.h>
#include <ff.h>

#ifdef __cplusplus
extern "C" {
#endif

FS_FILE_DEFINE(FIL fp);
FS_DIR_DEFINE(DIR dp);

#define MAX_FILE_NAME 12 /* Uses 8.3 SFN */

static inline off_t fs_tell(struct _fs_file_object *zfp)
{
	return f_tell(&zfp->fp);
}

#ifdef __cplusplus
}
#endif

#endif /* _FAT_FS_H_ */
