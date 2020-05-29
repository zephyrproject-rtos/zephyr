/*
 * Copyright (c) 2014-2015 Google Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "greybus-manifest.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

#include <list.h>
#include <sys/byteorder.h>
#include <greybus-utils/utils.h>
//#include <nuttx/util.h>

#ifdef CONFIG_GREYBUS_STATIC_MANIFEST
#include <greybus/static-manifest.h>
#endif

#undef ALIGN
#undef PAD
#define PAD(x,pot) (((x) & (pot-1)) ? (((x) & ~(pot-1)) + pot) : (x))
// "align" (really pad) to a 4-byte boundary
#define ALIGN(x) PAD(x,4)

// not yet a part of zephyr minimal libc??
extern char *strtok(char *str, const char *delim);

extern void gb_control_register(int cport, int bundle);
extern void gb_gpio_register(int cport, int bundle);
extern void gb_i2c_register(int cport, int bundle);
extern void gb_power_supply_register(int cport, int bundle);
extern void gb_loopback_register(int cport, int bundle);
extern void gb_vibrator_register(int cport, int bundle);
extern void gb_usb_register(int cport, int bundle);
extern void gb_pwm_register(int cport, int bundle);
extern void gb_spi_register(int cport, int bundle);
extern void gb_uart_register(int cport, int bundle);
extern void gb_hid_register(int cport, int bundle);
extern void gb_lights_register(int cport, int bundle);
extern void gb_sdio_register(int cport, int bundle);
extern void gb_camera_register(int cport, int bundle);
extern void gb_audio_mgmt_register(int cport, int bundle);
extern void gb_audio_data_register(int cport, int bundle);

struct greybus {
    struct list_head cports;
    struct greybus_driver *drv;
    size_t max_bundle_id;
};

static struct greybus g_greybus = {
    .cports = LIST_INIT(g_greybus.cports),
};

#ifdef CONFIG_GREYBUS_STATIC_MANIFEST
static unsigned char *bridge_manifest = (unsigned char *)manifest_mnfb;
#else
static unsigned char *bridge_manifest;
#endif

static void *alloc_cport(void)
{
    struct gb_cport *gb_cport;

    gb_cport = malloc(sizeof(struct gb_cport));
    if (!gb_cport)
        return NULL;

    list_add(&g_greybus.cports, &gb_cport->list);
    return gb_cport;
}

static void free_cport(int cportid)
{
    struct gb_cport *gb_cport;
    struct list_head *iter, *next;
    list_foreach_safe(&g_greybus.cports, iter, next) {
        gb_cport = list_entry(iter, struct gb_cport, list);
        if (gb_cport->id == cportid) {
            list_del(iter);
            free(gb_cport);
        }
    }
}

#ifdef CONFIG_GREYBUS
void enable_cports(void)
{
    struct list_head *iter;
    struct gb_cport *gb_cport;
    __attribute__((unused)) int cport_id;
    __attribute__((unused)) int bundle_id;
    __attribute__((unused)) int protocol;
    list_foreach(&g_greybus.cports, iter) {
        gb_cport = list_entry(iter, struct gb_cport, list);
        cport_id = gb_cport->id;
        bundle_id = gb_cport->bundle;
        protocol = gb_cport->protocol;

#ifdef CONFIG_GREYBUS_CONTROL
        if (protocol == GREYBUS_PROTOCOL_CONTROL) {
            gb_info("Registering CONTROL greybus driver.\n");
            gb_control_register(cport_id, bundle_id);
        }
#endif

#ifdef CONFIG_GREYBUS_GPIO
        if (protocol == GREYBUS_PROTOCOL_GPIO) {
            gb_info("Registering GPIO greybus driver.\n");
            gb_gpio_register(cport_id, bundle_id);
        }
#endif

#ifdef CONFIG_GREYBUS_I2C
        if (protocol == GREYBUS_PROTOCOL_I2C) {
            gb_info("Registering I2C greybus driver.\n");
            gb_i2c_register(cport_id, bundle_id);
        }
#endif

#ifdef CONFIG_GREYBUS_POWER_SUPPLY
        if (protocol == GREYBUS_PROTOCOL_POWER_SUPPLY) {
            gb_info("Registering POWER_SUPPLY greybus driver.\n");
            gb_power_supply_register(cport_id, bundle_id);
        }
#endif

#ifdef CONFIG_GREYBUS_LOOPBACK
        if (protocol == GREYBUS_PROTOCOL_LOOPBACK) {
            gb_info("Registering Loopback greybus driver.\n");
            gb_loopback_register(cport_id, bundle_id);
        }
#endif

#ifdef CONFIG_GREYBUS_VIBRATOR
        if (protocol == GREYBUS_PROTOCOL_VIBRATOR) {
            gb_info("Registering VIBRATOR greybus driver.\n");
            gb_vibrator_register(cport_id, bundle_id);
        }
#endif

#ifdef CONFIG_GREYBUS_USB_HOST
        if (protocol == GREYBUS_PROTOCOL_USB) {
            gb_info("Registering USB greybus driver.\n");
            gb_usb_register(cport_id, bundle_id);
        }
#endif

#ifdef CONFIG_GREYBUS_PWM
        if (protocol == GREYBUS_PROTOCOL_PWM) {
            gb_info("Registering PWM greybus driver.\n");
            gb_pwm_register(cport_id, bundle_id);
        }
#endif

#ifdef CONFIG_GREYBUS_SPI
        if (protocol == GREYBUS_PROTOCOL_SPI) {
            gb_info("Registering SPI greybus driver.\n");
            gb_spi_register(cport_id, bundle_id);
        }
#endif

#ifdef CONFIG_GREYBUS_UART
        if (protocol == GREYBUS_PROTOCOL_UART) {
            gb_info("Registering Uart greybus driver. id= %d\n", cport_id);
            gb_uart_register(cport_id, bundle_id);
        }
#endif

#ifdef CONFIG_GREYBUS_HID
        if (protocol == GREYBUS_PROTOCOL_HID) {
            gb_info("Registering HID greybus driver. id= %d\n", cport_id);
            gb_hid_register(cport_id, bundle_id);
        }
#endif

#ifdef CONFIG_GREYBUS_LIGHTS
        if (protocol == GREYBUS_PROTOCOL_LIGHTS) {
            gb_info("Registering Lights greybus driver. id= %d\n", cport_id);
            gb_lights_register(cport_id, bundle_id);
        }
#endif

#ifdef CONFIG_GREYBUS_SDIO
        if (protocol == GREYBUS_PROTOCOL_SDIO) {
            gb_info("Registering SDIO greybus driver.\n");
            gb_sdio_register(cport_id, bundle_id);
        }
#endif

#ifdef CONFIG_GREYBUS_CAMERA
        if (protocol == GREYBUS_PROTOCOL_CAMERA_MGMT) {
            gb_info("Registering Camera greybus driver. id= %d\n", cport_id);
            gb_camera_register(cport_id, bundle_id);
        }
#endif

#ifdef CONFIG_GREYBUS_AUDIO
        if (protocol == GREYBUS_PROTOCOL_AUDIO_MGMT) {
            gb_info("Registering Audio MGMT greybus driver.\n");
            gb_audio_mgmt_register(cport_id, bundle_id);
        }

        if (protocol == GREYBUS_PROTOCOL_AUDIO_DATA) {
            gb_info("Registering Audio DATA greybus driver.\n");
            gb_audio_data_register(cport_id, bundle_id);
        }
#endif
    }
}
#endif

/*
 * Validate the given descriptor.  Its reported size must fit within
 * the number of bytes reamining, and it must have a recognized
 * type.  Check that the reported size is at least as big as what
 * we expect to see.  (It could be bigger, perhaps for a new version
 * of the format.)
 *
 * Returns the number of bytes consumed by the descriptor, or a
 * negative errno.
 */
static int identify_descriptor(struct greybus_descriptor *desc, size_t size,
                               int release)
{
    struct greybus_descriptor_header *desc_header = &desc->header;
    size_t expected_size;
    size_t desc_size;
    struct gb_cport *cport;

    if (size < sizeof(*desc_header)) {
        gb_error("manifest too small\n");
        return -EINVAL;         /* Must at least have header */
    }

    desc_size = (int)sys_le16_to_cpu(desc_header->size);
    if ((size_t) desc_size > size) {
        gb_error("descriptor too big\n");
        return -EINVAL;
    }

    /* Descriptor needs to at least have a header */
    expected_size = sizeof(*desc_header);

    switch (desc_header->type) {
    case GREYBUS_TYPE_STRING:
        expected_size += sizeof(struct greybus_descriptor_string);
        expected_size += desc->string.length;

        /* String descriptors are padded to 4 byte boundaries */
        expected_size = ALIGN(expected_size);
        break;
    case GREYBUS_TYPE_INTERFACE:
        expected_size += sizeof(struct greybus_descriptor_interface);
        break;
    case GREYBUS_TYPE_BUNDLE:
        expected_size += sizeof(struct greybus_descriptor_bundle);
        g_greybus.max_bundle_id = MAX(g_greybus.max_bundle_id, desc->bundle.id);
        break;
    case GREYBUS_TYPE_CPORT:
        expected_size += sizeof(struct greybus_descriptor_cport);
        if (desc_size >= expected_size) {
            if (!release) {
                cport = alloc_cport();
                if (!cport)
                    return -ENOMEM;

                cport->id = desc->cport.id;
                cport->bundle = desc->cport.bundle;
                cport->protocol = desc->cport.protocol_id;
                gb_debug("cport_id = %d\n", cport->id);
            } else {
                free_cport(desc->cport.id);
            }
        }
        break;
    case GREYBUS_TYPE_INVALID:
    default:
        gb_error("invalid descriptor type (%hhu)\n", desc_header->type);
        return -EINVAL;
    }

    if (desc_size < expected_size) {
        gb_error("%d: descriptor too small (%zu < %zu)\n",
                 desc_header->type, desc_size, expected_size);
        return -EINVAL;
    }

    /* Descriptor bigger than what we expect */
    if (desc_size > expected_size) {
        gb_error("%d descriptor size mismatch (want %zu got %zu)\n",
                 desc_header->type, expected_size, desc_size);
    }

    return desc_size;
}

static bool _manifest_parse(void *data, size_t size, int release)
{
    struct greybus_manifest *manifest = data;
    struct greybus_manifest_header *header = &manifest->header;
    struct greybus_descriptor *desc;
    uint16_t manifest_size;

    if (!release) {
        /* we have to have at _least_ the manifest header */
        if (size <= sizeof(manifest->header)) {
            gb_error("short manifest (%zu)\n", size);
            return false;
        }

        /* Make sure the size is right */
        manifest_size = sys_le16_to_cpu(header->size);
        if (manifest_size != size) {
            gb_error("manifest size mismatch %zu != %hu\n", size,
                     manifest_size);
            return false;
        }

        /* Validate major/minor number */
        if (header->version_major > GREYBUS_VERSION_MAJOR) {
            gb_error("manifest version too new (%hhu.%hhu > %hhu.%hhu)\n",
                     header->version_major, header->version_minor,
                     GREYBUS_VERSION_MAJOR, GREYBUS_VERSION_MINOR);
            return false;
        }
    }

    /* OK, find all the descriptors */
    desc = (struct greybus_descriptor *)(header + 1);
    size -= sizeof(*header);
    while (size) {
        int desc_size;

        desc_size = identify_descriptor(desc, size, release);
        if (desc_size <= 0)
            return false;
        desc = (struct greybus_descriptor *)((char *)desc + desc_size);
        size -= desc_size;
    }

    return true;
}

/*
 * Parse a buffer containing a interface manifest.
 *
 * If we find anything wrong with the content/format of the buffer
 * we reject it.
 *
 * The first requirement is that the manifest's version is
 * one we can parse.
 *
 * We make an initial pass through the buffer and identify all of
 * the descriptors it contains, keeping track for each its type
 * and the location size of its data in the buffer.
 *
 * Returns true if parsing was successful, false otherwise.
 */
bool manifest_parse(void *data, size_t size)
{
    return _manifest_parse(data, size, 0);
}

bool manifest_release(void *data, size_t size)
{
    return _manifest_parse(data, size, 1);
}

static int get_interface_id(char *fname)
{
    char *iid_str;
    int iid = 0;
    char tmp[256];

    strcpy(tmp, fname);
    iid_str = strtok(tmp, "-");
    if (!strncmp(iid_str, "IID", 3))
        iid = strtol(fname + 4, NULL, 0);

    return iid;
}

void *get_manifest_blob(void)
{
    return bridge_manifest;
}

void set_manifest_blob(void *blob)
{
    bridge_manifest = blob;
}

void parse_manifest_blob(void *manifest)
{
    struct greybus_manifest_header *mh = manifest;

    manifest_parse(mh, sys_le16_to_cpu(mh->size));
}

void release_manifest_blob(void *manifest)
{
    struct greybus_manifest_header *mh = manifest;

    manifest_release(mh, sys_le16_to_cpu(mh->size));
}

void enable_manifest(char *name, void *manifest, int device_id)
{
    if (!manifest) {
        manifest = get_manifest_blob();
    }

    if (manifest) {
        parse_manifest_blob(manifest);
        int iid = get_interface_id(name);
        if (iid > 0) {
            gb_info("%s interface inserted\n", name);
        } else {
            gb_error("invalid interface ID, no hotplug plug event sent\n");
        }
    } else {
        gb_error("missing manifest blob, no hotplug event sent\n");
    }
}

void disable_manifest(char *name, void *priv, int device_id)
{
    void *manifest;

    manifest = get_manifest_blob();
    if (manifest) {
        release_manifest_blob(manifest);
    }
}

struct list_head *get_manifest_cports(void)
{
    return &g_greybus.cports;
}

int get_manifest_size(void)
{
    struct greybus_manifest_header *mh = get_manifest_blob();

    return mh ? sys_le16_to_cpu(mh->size) : 0;
}

size_t manifest_get_max_bundle_id(void)
{
    return g_greybus.max_bundle_id;
}
