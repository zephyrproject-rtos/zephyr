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

#ifndef _FAT_DISKIO_H_
#define _FAT_DISKIO_H_

#include <diskio.h>
#include <stdint.h>

DSTATUS fat_disk_status(void);
DSTATUS fat_disk_initialize(void);
DRESULT fat_disk_read(uint8_t *buff, unsigned long sector, uint32_t count);
DRESULT fat_disk_write(const uint8_t *buff, unsigned long sector,
		       uint32_t count);
DRESULT fat_disk_ioctl(uint8_t cmd, void *buff);

#endif /* _FAT_DISKIO_H_ */
