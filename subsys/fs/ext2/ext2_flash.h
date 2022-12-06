/*
 * Copyright (c) 2022 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __EXT2_FLASH_H__
#define __EXT2_FLASH_H__

#include "ext2_struct.h"

int flash_init_backend(struct ext2_data *fs, const void *storage_dev, int flags);

#endif /* __EXT2_FLASH_H__ */
