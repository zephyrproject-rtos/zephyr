/*
 * Copyright (c) 2018 Findlay Feng
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef H_ZFFS_AREA_
#define H_ZFFS_AREA_

#include "zffs.h"
#include <flash_map.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ZFFS_AREA_ID_TYPE_DATA 0
#define ZFFS_AREA_ID_TYPE_SWAP 1
#define ZFFS_AREA_ID_TYPE_DATA_GC 2
#define ZFFS_AREA_ID_TYPE_SWAP_GC 3

#define ZFFS_AREA_ID_TYPE_BIT 6
#define ZFFS_AREA_ID_TYPE_MAKS (0x3 << ZFFS_AREA_ID_TYPE_BIT)
#define ZFFS_AREA_ID_TYPE_GC_MAKS 0x80
#define ZFFS_AREA_ID_NONE 0xff

struct zffs_area_pointer {
	struct zffs_area ***area_index;
	struct zffs_area *area;
	u32_t offset;
};

#define zffs_pointer(__zffs, __name)                                         \
	{                                                                      \
		.area_index = &(__zffs)->__name##_area                        \
	}

#define zffs_data_pointer(__zffs) zffs_pointer((__zffs), data)
#define zffs_swap_pointer(__zffs) zffs_pointer((__zffs), swap)

int zffs_area_load(struct zffs_data *zffs, u32_t offset, u32_t *length);
int zffs_area_init(struct zffs_data *zffs, u32_t offset, u32_t length);
int zffs_area_list_init(struct zffs_data *zffs,
			 struct zffs_area **area_list, u8_t type);

void zffs_area_addr_to_pointer(struct zffs_data *zffs, u32_t addr,
				struct zffs_area_pointer *pointer);
u32_t zffs_area_pointer_to_addr(struct zffs_data *zffs,
				 const struct zffs_area_pointer *pointer);

u32_t zffs_area_list_size(struct zffs_area **area_list);
int zffs_area_random_read(struct zffs_data *zffs,
			   struct zffs_area **area_list, u32_t addr,
			   void *data, size_t len);
int zffs_area_read(struct zffs_data *zffs,
		    struct zffs_area_pointer *pointer, void *data, size_t len);
int zffs_area_write(struct zffs_data *zffs,
		     struct zffs_area_pointer *pointer, const void *data,
		     size_t len);
int zffs_area_crc(struct zffs_data *zffs, struct zffs_area_pointer *pointer,
		   size_t len, u16_t *crc);
int zffs_area_copy_crc(struct zffs_data *zffs,
			struct zffs_area_pointer *from,
			struct zffs_area_pointer *to, size_t len, u16_t *crc);
int zffs_area_copy(struct zffs_data *zffs, struct zffs_area_pointer *from,
		    struct zffs_area_pointer *to, size_t len);
int zffs_area_erase_list(struct zffs_data *zffs,
			  struct zffs_area ***area_list);
int zffs_area_is_not_empty(struct zffs_data *zffs,
			    struct zffs_area_pointer *pointer, size_t len);



#ifdef __cplusplus
}
#endif

#endif
