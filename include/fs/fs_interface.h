/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _FS_INTERFACE_H_
#define _FS_INTERFACE_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * @brief Macro to define _zfile_object structure
 *
 * This structure contains information about the open files. This
 * structure will be passed to the api functions as an opaque
 * pointer.
 *
 * @param _file_object File structure used by underlying file system
 */

#define FS_FILE_DEFINE(_file_object) \
	struct _fs_file_object { \
		_file_object; \
	}

/*
 * @brief Macro to define _zdir_object structure
 *
 * This structure contains information about the open directories. This
 * structure will be passed to the directory api functions as an opaque
 * pointer.
 *
 * @param _dir_object Directory structure used by underlying file system
 */

#define FS_DIR_DEFINE(_dir_object) \
	struct _fs_dir_object { \
		_dir_object; \
	}

#ifdef CONFIG_FAT_FILESYSTEM_ELM
#include <fs/fat_fs.h>
#elif CONFIG_FILE_SYSTEM_NFFS
#include <fs/nffs_fs.h>
#endif

#ifdef __cplusplus
}
#endif

#endif /* _FS_INTERFACE_H_ */
