/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <assert.h>
#include <flash.h>
#include <flash_map.h>
#include <zephyr.h>
#include <soc.h>
#include <init.h>
#include <dfu/mcuboot.h>
#include <dfu/flash_img.h>
#include <mgmt/mgmt.h>
#include <img_mgmt/img_mgmt_impl.h>
#include <img_mgmt/img_mgmt.h>
#include "../../../src/img_mgmt_priv.h"

/**
 * Determines if the specified area of flash is completely unwritten.
 */
static int
zephyr_img_mgmt_flash_check_empty(u8_t fa_id, bool *out_empty)
{
    const struct flash_area *fa;
    uint32_t data[16];
    off_t addr;
    off_t end;
    int bytes_to_read;
    int rc;
    int i;

    rc = flash_area_open(fa_id, &fa);
    if (rc != 0) {
        return MGMT_ERR_EUNKNOWN;
    }

    assert(fa->fa_size % 4 == 0);

    end = fa->fa_size;
    for (addr = 0; addr < end; addr += sizeof data) {
        if (end - addr < sizeof data) {
            bytes_to_read = end - addr;
        } else {
            bytes_to_read = sizeof data;
        }

        rc = flash_area_read(fa, addr, data, bytes_to_read);
        if (rc != 0) {
            flash_area_close(fa);
            return MGMT_ERR_EUNKNOWN;
        }

        for (i = 0; i < bytes_to_read / 4; i++) {
            if (data[i] != 0xffffffff) {
                *out_empty = false;
                flash_area_close(fa);
                return 0;
            }
        }
    }

    *out_empty = true;
    flash_area_close(fa);
    return 0;
}

/**
 * Get flash_area ID for a image slot number.
 */
static u8_t
zephyr_img_mgmt_flash_area_id(int slot)
{
    u8_t fa_id;

    switch (slot) {
    case 0:
        fa_id = DT_FLASH_AREA_IMAGE_0_ID;
        break;

    case 1:
        fa_id = DT_FLASH_AREA_IMAGE_1_ID;
        break;

    default:
        assert(0);
        fa_id = DT_FLASH_AREA_IMAGE_1_ID;
        break;
    }

    return fa_id;
}

int
img_mgmt_impl_erase_slot(void)
{
    bool empty;
    int rc;

    rc = zephyr_img_mgmt_flash_check_empty(DT_FLASH_AREA_IMAGE_1_ID,
                                           &empty);
    if (rc != 0) {
        return MGMT_ERR_EUNKNOWN;
    }

    if (!empty) {
        rc = boot_erase_img_bank(DT_FLASH_AREA_IMAGE_1_ID);
        if (rc != 0) {
            return MGMT_ERR_EUNKNOWN;
        }
    }

    return 0;
}

int
img_mgmt_impl_write_pending(int slot, bool permanent)
{
    int rc;

    if (slot != 1) {
        return MGMT_ERR_EINVAL;
    }

    rc = boot_request_upgrade(permanent);
    if (rc != 0) {
        return MGMT_ERR_EUNKNOWN;
    }

    return 0;
}

int
img_mgmt_impl_write_confirmed(void)
{
    int rc;

    rc = boot_write_img_confirmed();
    if (rc != 0) {
        return MGMT_ERR_EUNKNOWN;
    }

    return 0;
}

int
img_mgmt_impl_read(int slot, unsigned int offset, void *dst,
                   unsigned int num_bytes)
{
    const struct flash_area *fa;
    int rc;

    rc = flash_area_open(zephyr_img_mgmt_flash_area_id(slot), &fa);
    if (rc != 0) {
        return MGMT_ERR_EUNKNOWN;
    }

    rc = flash_area_read(fa, offset, dst, num_bytes);
    flash_area_close(fa);

    if (rc != 0) {
        return MGMT_ERR_EUNKNOWN;
    }

    return 0;
}

int
img_mgmt_impl_write_image_data(unsigned int offset, const void *data,
                               unsigned int num_bytes, bool last)
{
	int rc;
#if (CONFIG_HEAP_MEM_POOL_SIZE > 0)
	static struct flash_img_context *ctx = NULL;
#else
	static struct flash_img_context ctx_data;
#define ctx (&ctx_data)
#endif

#if (CONFIG_HEAP_MEM_POOL_SIZE > 0)
	if (offset != 0 && ctx == NULL) {
		return MGMT_ERR_EUNKNOWN;
	}
#endif

	if (offset == 0) {
#if (CONFIG_HEAP_MEM_POOL_SIZE > 0)
		if (ctx == NULL) {
			ctx = k_malloc(sizeof(*ctx));

			if (ctx == NULL) {
				return MGMT_ERR_ENOMEM;
			}
		}
#endif
		rc = flash_img_init(ctx);

		if (rc != 0) {
			return MGMT_ERR_EUNKNOWN;
		}
	}

	if (offset != ctx->bytes_written + ctx->buf_bytes) {
		return MGMT_ERR_EUNKNOWN;
	}

	/* Cast away const. */
	rc = flash_img_buffered_write(ctx, (void *)data, num_bytes, last);
	if (rc != 0) {
		return MGMT_ERR_EUNKNOWN;
	}

#if (CONFIG_HEAP_MEM_POOL_SIZE > 0)
	if (last) {
		k_free(ctx);
		ctx = NULL;
	}
#endif

	return 0;
}

int
img_mgmt_impl_swap_type(void)
{
    switch (boot_swap_type()) {
    case BOOT_SWAP_TYPE_NONE:
        return IMG_MGMT_SWAP_TYPE_NONE;
    case BOOT_SWAP_TYPE_TEST:
        return IMG_MGMT_SWAP_TYPE_TEST;
    case BOOT_SWAP_TYPE_PERM:
        return IMG_MGMT_SWAP_TYPE_PERM;
    case BOOT_SWAP_TYPE_REVERT:
        return IMG_MGMT_SWAP_TYPE_REVERT;
    default:
        assert(0);
        return IMG_MGMT_SWAP_TYPE_NONE;
    }
}
