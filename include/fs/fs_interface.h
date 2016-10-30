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

#ifdef CONFIG_FILE_SYSTEM_FAT
#include <fs/fat_fs.h>
#endif

#ifdef __cplusplus
}
#endif

#endif /* _FS_INTERFACE_H_ */
