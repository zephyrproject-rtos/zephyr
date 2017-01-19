/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
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
