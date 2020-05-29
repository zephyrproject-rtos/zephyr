/*
 * Copyright (c) 2015 Google, Inc.
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

#ifndef _GREYBUS_SDIO_GB_H_
#define _GREYBUS_SDIO_GB_H_

#include <greybus/types.h>

/* Greybus SDIO Operation Types */
#define GB_SDIO_TYPE_PROTOCOL_VERSION           0x01
#define GB_SDIO_TYPE_PROTOCOL_GET_CAPABILITIES  0x02
#define GB_SDIO_TYPE_PROTOCOL_SET_IOS           0x03
#define GB_SDIO_TYPE_PROTOCOL_COMMAND           0x04
#define GB_SDIO_TYPE_PROTOCOL_TRANSFER          0x05
#define GB_SDIO_TYPE_EVENT                      0x06

/* Transfer Data Flags */
#define GB_SDIO_DATA_WRITE  0x01    /* Request to write */
#define GB_SDIO_DATA_READ   0x02    /* Request to read */
#define GB_SDIO_DATA_STREAM 0x04    /* Read and write until cancel command */

/**
 * SDIO Protocol Version Response
 */
struct gb_sdio_proto_version_response {
    /** SDIO Protocol major version */
    __u8    major;
    /** SDIO Protocol minor version */
    __u8    minor;
} __packed;

/**
 * SDIO Get Capabilities Response
 */
struct gb_sdio_get_capabilities_response {
    /** SDIO capabilities bit masks */
    __le32 caps;
    /** SDIO voltage range bit masks */
    __le32 ocr;
    /** SDIO Minimum frequency supported by the controller */
    __le32 f_min;
    /** SDIO Maximum frequency supported by the controller */
    __le32 f_max;
    /** SDIO maximum number of blocks per data command transfer */
    __le16 max_blk_count;
    /** SDIO maximum size of each block to transfer */
    __le16 max_blk_size;
} __packed;

/**
 * SDIO Set IOS Request
 */
struct gb_sdio_set_ios_request {
    /** clock rate in Hz*/
    __le32 clock;
    /** SDIO voltage range bit mask */
    __le32 vdd;
    /** SDIO bus mode */
    __u8   bus_mode;
    /** power mode */
    __u8   power_mode;
    /** bus width */
    __u8   bus_width;
    /** timing */
    __u8   timing;
    /** signal voltage */
    __u8   signal_voltage;
    /** driver type */
    __u8   drv_type;
} __packed;

/**
 * SDIO Command Request
 */
struct gb_sdio_command_request {
    /** SDIO command operation code, as specified by SD Association */
    __u8   cmd;
    /** Greybus SDIO Protocol Command Flags */
    __u8   cmd_flags;
    /** Greybus SDIO Protocol Command Type */
    __u8   cmd_type;
    /** SDIO command arguments, as specified by SD Association */
    __le32 cmd_arg;
    /** SDIO number of blocks of data to transfer */
    __le16 data_blocks;
    /** SDIO size of the blocks of data to transfer */
    __le16 data_blksz;
} __packed;

/**
 * SDIO Command response
 */
struct gb_sdio_command_response {
    /** SDIO command response, as specified by SD Association */
    __le32 resp[4];
};

/**
 * SDIO Transfer Request
 */
struct gb_sdio_transfer_request {
    /** SDIO data flags */
    __u8   data_flags;
    /** SDIO number of blocks of data to transfer */
    __le16 data_blocks;
    /** SDIO size of the blocks of data to transfer */
    __le16 data_blksz;
    /** SDIO Data */
    __u8   data[0];
} __packed;

/**
 * SDIO Transfer Response
 */
struct gb_sdio_transfer_response {
    /** SDIO number of blocks of data to transfer */
    __le16 data_blocks;
    /** SDIO size of the blocks of data to transfer */
    __le16 data_blksz;
    /** SDIO Data */
    __u8   data[0];
} __packed;

/**
 * SDIO Event Request
 */
struct gb_sdio_event_request {
    __u8    event;
} __packed;

#endif /* _GREYBUS_SDIO_GB_H_ */
