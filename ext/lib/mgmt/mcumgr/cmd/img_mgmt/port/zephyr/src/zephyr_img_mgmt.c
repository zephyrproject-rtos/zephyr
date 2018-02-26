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
#include <zephyr.h>
#include <soc.h>
#include <init.h>
#include <dfu/mcuboot.h>
#include <dfu/flash_img.h>
#include <mgmt/mgmt.h>
#include <img_mgmt/img_mgmt_impl.h>
#include <img_mgmt/img_mgmt.h>
#include "../../../src/img_mgmt_priv.h"

static struct device *zephyr_img_mgmt_flash_dev;
static struct flash_img_context zephyr_img_mgmt_flash_ctxt;

/**
 * Determines if the specified area of flash is completely unwritten.
 */
static int
zephyr_img_mgmt_flash_check_empty(off_t offset, size_t size, bool *out_empty)
{
    uint32_t data[16];
    off_t addr;
    off_t end;
    int bytes_to_read;
    int rc;
    int i;

    assert(size % 4 == 0);

    end = offset + size;
    for (addr = offset; addr < end; addr += sizeof data) {
        if (end - addr < sizeof data) {
            bytes_to_read = end - addr;
        } else {
            bytes_to_read = sizeof data;
        }

        rc = flash_read(zephyr_img_mgmt_flash_dev, addr, data, bytes_to_read);
        if (rc != 0) {
            return MGMT_ERR_EUNKNOWN;
        }

        for (i = 0; i < bytes_to_read / 4; i++) {
            if (data[i] != 0xffffffff) {
                *out_empty = false;
                return 0;
            }
        }
    }

    *out_empty = true;
    return 0;
}

/**
 * Converts an offset within an image slot to an absolute address.
 */
static off_t
zephyr_img_mgmt_abs_offset(int slot, off_t sub_offset)
{
    off_t slot_start;

    switch (slot) {
    case 0:
        slot_start = FLASH_AREA_IMAGE_0_OFFSET;
        break;

    case 1:
        slot_start = FLASH_AREA_IMAGE_1_OFFSET;
        break;

    default:
        assert(0);
        slot_start = FLASH_AREA_IMAGE_1_OFFSET;
        break;
    }

    return slot_start + sub_offset;
}

int
img_mgmt_impl_erase_slot(void)
{
    bool empty;
    int rc;

    rc = zephyr_img_mgmt_flash_check_empty(FLASH_AREA_IMAGE_1_OFFSET,
                                           FLASH_AREA_IMAGE_1_SIZE,
                                           &empty);
    if (rc != 0) {
        return MGMT_ERR_EUNKNOWN;
    }

    if (!empty) {
        rc = boot_erase_img_bank(FLASH_AREA_IMAGE_1_OFFSET);
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
    off_t abs_offset;
    int rc;

    abs_offset = zephyr_img_mgmt_abs_offset(slot, offset);
    rc = flash_read(zephyr_img_mgmt_flash_dev, abs_offset, dst, num_bytes);
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

    if (offset == 0) {
        flash_img_init(&zephyr_img_mgmt_flash_ctxt, zephyr_img_mgmt_flash_dev);
    }

    /* Cast away const. */
    rc = flash_img_buffered_write(&zephyr_img_mgmt_flash_ctxt, (void *)data,
                                  num_bytes, false);
    if (rc != 0) {
        return MGMT_ERR_EUNKNOWN;
    }

    if (last) {
        rc = flash_img_buffered_write(&zephyr_img_mgmt_flash_ctxt,
                                      NULL, 0, true);
        if (rc != 0) {
            return MGMT_ERR_EUNKNOWN;
        }
    }

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

static int
zephyr_img_mgmt_init(struct device *dev)
{
    ARG_UNUSED(dev);

    zephyr_img_mgmt_flash_dev = device_get_binding(FLASH_DEV_NAME);
    if (zephyr_img_mgmt_flash_dev == NULL) {
        return -ENODEV;
    }
    return 0;
}

SYS_INIT(zephyr_img_mgmt_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
